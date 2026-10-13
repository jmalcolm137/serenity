/*
 * Copyright (c) 2026, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BoardWidget.h"
#include "Game.h"
#include <AK/String.h>
#include <LibConfig/Client.h>
#include <LibCore/System.h>
#include <LibCore/Timer.h>
#include <LibDesktop/Launcher.h>
#include <LibGUI/Action.h>
#include <LibGUI/ActionGroup.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Menu.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Statusbar.h>
#include <LibGUI/Window.h>
#include <LibGfx/Bitmap.h>
#include <LibMain/Main.h>
#include <LibURL/URL.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio rpath recvfd sendfd unix"));

    auto app = TRY(GUI::Application::create(arguments));

    Config::pledge_domain("Sudoku");

    TRY(Desktop::Launcher::add_allowed_handler_with_only_specific_urls("/bin/Help", { URL::create_with_file_scheme("/usr/share/man/man6/Sudoku.md") }));
    TRY(Desktop::Launcher::seal_allowlist());

    TRY(Core::System::pledge("stdio rpath recvfd sendfd"));

    TRY(Core::System::unveil("/tmp/session/%sid/portal/launch", "rw"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto app_icon = TRY(GUI::Icon::try_create_default_icon("app-sudoku"sv));

    auto window = GUI::Window::construct();
    window->set_title("Sudoku");
    window->set_resizable(true);
    window->set_icon(app_icon.bitmap_for_size(16));

    auto main_widget = window->set_main_widget<GUI::Widget>();
    main_widget->set_layout<GUI::VerticalBoxLayout>(GUI::Margins { }, 0);
    auto& board = main_widget->add<Sudoku::BoardWidget>();
    auto& statusbar = main_widget->add<GUI::Statusbar>();

    auto difficulty = Sudoku::Difficulty::Easy;
    {
        auto stored_difficulty = Config::read_i32("Sudoku"sv, ""sv, "difficulty"sv, 0);
        if (stored_difficulty >= 0 && stored_difficulty <= static_cast<i32>(Sudoku::Difficulty::Expert))
            difficulty = static_cast<Sudoku::Difficulty>(stored_difficulty);
    }
    board.set_markup_enabled(Config::read_bool("Sudoku"sv, ""sv, "pencil_marks"sv, false));
    board.set_show_conflicts(Config::read_bool("Sudoku"sv, ""sv, "highlight_conflicts"sv, true));

    int elapsed_seconds = 0;
    auto update_status = [&]() {
        auto minutes = elapsed_seconds / 60;
        auto seconds = elapsed_seconds % 60;
        statusbar.set_text(String::formatted("{}    Time: {}:{:02}    Remaining: {}    Mistakes: {}",
            Sudoku::difficulty_name(board.game().difficulty()),
            minutes,
            seconds,
            board.game().remaining_count(),
            board.game().mistake_count())
                .release_value_but_fixme_should_propagate_errors());
    };

    auto timer = Core::Timer::create_repeating(1000, [&]() {
        ++elapsed_seconds;
        update_status();
    });

    auto start_new_game = [&]() {
        board.new_game(difficulty);
        elapsed_seconds = 0;
        timer->start();
        update_status();
    };

    auto game_menu = window->add_menu("&Game"_string);
    game_menu->add_action(GUI::Action::create("&New Game", { Mod_None, Key_F2 }, TRY(Gfx::Bitmap::load_from_uri("resource://icons/16x16/reload.png"sv)), [&](auto&) {
        start_new_game();
    }));

    auto difficulty_menu = game_menu->add_submenu("&Difficulty"_string);
    static constexpr struct {
        char const* label;
        Sudoku::Difficulty difficulty;
    } difficulty_entries[] = {
        { "&Easy", Sudoku::Difficulty::Easy },
        { "&Medium", Sudoku::Difficulty::Medium },
        { "&Hard", Sudoku::Difficulty::Hard },
        { "&Expert", Sudoku::Difficulty::Expert },
    };
    GUI::ActionGroup difficulty_group;
    difficulty_group.set_exclusive(true);
    difficulty_group.set_unchecking_allowed(false);
    for (auto const& entry : difficulty_entries) {
        auto action = GUI::Action::create_checkable(entry.label, [&, selected = entry.difficulty](auto&) {
            difficulty = selected;
            Config::write_i32("Sudoku"sv, ""sv, "difficulty"sv, static_cast<i32>(selected));
            start_new_game();
        });
        action->set_checked(difficulty == entry.difficulty);
        difficulty_group.add_action(action);
        difficulty_menu->add_action(action);
    }

    game_menu->add_action(GUI::Action::create("&Restart", [&](auto&) {
        board.restart();
        elapsed_seconds = 0;
        timer->start();
        update_status();
    }));
    game_menu->add_separator();

    auto undo_action = GUI::CommonActions::make_undo_action([&](auto&) {
        board.game().undo();
        update_status();
    });
    game_menu->add_action(undo_action);

    auto redo_action = GUI::CommonActions::make_redo_action([&](auto&) {
        board.game().redo();
        update_status();
    });
    game_menu->add_action(redo_action);

    game_menu->add_separator();
    game_menu->add_action(GUI::Action::create("&Check", [&](auto&) {
        auto mistakes = board.game().mistake_count();
        if (mistakes == 0)
            GUI::MessageBox::show(window, "No mistakes so far!"sv, "Sudoku"sv);
        else
            GUI::MessageBox::show(window, String::formatted("{} mistake(s) so far.", mistakes).release_value_but_fixme_should_propagate_errors(), "Sudoku"sv);
    }));
    game_menu->add_action(GUI::Action::create("&Solve", [&](auto&) {
        if (GUI::MessageBox::show(window, "Reveal the whole solution?"sv, "Sudoku"sv, GUI::MessageBox::Type::Warning, GUI::MessageBox::InputType::OKCancel) != GUI::MessageBox::ExecResult::OK)
            return;
        board.game().reveal_solution();
        timer->stop();
        update_status();
    }));
    game_menu->add_separator();
    game_menu->add_action(GUI::CommonActions::make_quit_action([](auto&) {
        GUI::Application::the()->quit();
    }));

    auto settings_menu = window->add_menu("&Settings"_string);
    auto pencil_action = GUI::Action::create_checkable("&Pencil Marks", [&](auto& action) {
        board.set_markup_enabled(action.is_checked());
        Config::write_bool("Sudoku"sv, ""sv, "pencil_marks"sv, action.is_checked());
    });
    pencil_action->set_checked(board.markup_enabled());
    settings_menu->add_action(pencil_action);

    auto conflict_action = GUI::Action::create_checkable("&Highlight Conflicts", [&](auto& action) {
        board.set_show_conflicts(action.is_checked());
        Config::write_bool("Sudoku"sv, ""sv, "highlight_conflicts"sv, action.is_checked());
    });
    conflict_action->set_checked(board.show_conflicts());
    settings_menu->add_action(conflict_action);

    auto view_menu = window->add_menu("&View"_string);
    view_menu->add_action(GUI::CommonActions::make_fullscreen_action([&](auto&) {
        window->set_fullscreen(!window->is_fullscreen());
    }));

    auto help_menu = window->add_menu("&Help"_string);
    help_menu->add_action(GUI::CommonActions::make_command_palette_action(window));
    help_menu->add_action(GUI::CommonActions::make_help_action([](auto&) {
        Desktop::Launcher::open(URL::create_with_file_scheme("/usr/share/man/man6/Sudoku.md"), "/bin/Help");
    }));
    help_menu->add_action(GUI::CommonActions::make_about_action("Sudoku"_string, app_icon, window));

    board.on_state_change = [&]() {
        update_status();
        pencil_action->set_checked(board.markup_enabled());
        conflict_action->set_checked(board.show_conflicts());
        undo_action->set_enabled(board.game().can_undo());
        redo_action->set_enabled(board.game().can_redo());
    };
    board.on_solved = [&]() {
        timer->stop();
        update_status();
        GUI::MessageBox::show(window, "Congratulations, you solved the puzzle!"sv, "Sudoku"sv);
    };

    board.new_game(difficulty);
    undo_action->set_enabled(false);
    redo_action->set_enabled(false);
    timer->start();
    update_status();

    window->resize(480, 500);
    window->show();
    board.set_focus(true);

    return app->exec();
}
