/*
 * Copyright (c) 2026, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BoardWidget.h"
#include <AK/Assertions.h>
#include <AK/StdLibExtras.h>
#include <AK/StringView.h>
#include <LibGUI/Painter.h>
#include <LibGfx/Font/FontDatabase.h>

namespace Sudoku {

// A muted, theme-independent palette so the board stays readable anywhere.
static constexpr auto BOARD_BACKGROUND_COLOR = Gfx::Color::White;
static constexpr auto GIVEN_TEXT_COLOR = Gfx::Color::from_rgb(0x212121);
static constexpr auto USER_TEXT_COLOR = Gfx::Color::from_rgb(0x1565c0);
static constexpr auto CONFLICT_TEXT_COLOR = Gfx::Color::from_rgb(0xc62828);
static constexpr auto NOTE_TEXT_COLOR = Gfx::Color::from_rgb(0x607d8b);
static constexpr auto GRID_LINE_COLOR = Gfx::Color::from_rgb(0xcfd8dc);
static constexpr auto BOX_LINE_COLOR = Gfx::Color::from_rgb(0x546e7a);
static constexpr auto PEER_HIGHLIGHT_COLOR = Gfx::Color::from_rgb(0xedf4fb);
static constexpr auto SAME_VALUE_HIGHLIGHT_COLOR = Gfx::Color::from_rgb(0xdcedc8);
static constexpr auto SELECTION_HIGHLIGHT_COLOR = Gfx::Color::from_rgb(0xbbdefb);
static constexpr auto SELECTION_BORDER_COLOR = Gfx::Color::from_rgb(0x1e88e5);

static StringView digit_text(u8 digit)
{
    VERIFY(digit >= 1 && digit <= 9);
    static constexpr Array<StringView, 9> texts { "1"sv, "2"sv, "3"sv, "4"sv, "5"sv, "6"sv, "7"sv, "8"sv, "9"sv };
    return texts[digit - 1];
}

static RefPtr<Gfx::Font const> find_font(char const* family, int target_size, int weight)
{
    auto& database = Gfx::FontDatabase::the();
    ByteString best_name;
    int best_difference = 1 << 30;
    database.for_each_font([&](Gfx::Font const& font) {
        if (font.family() != family)
            return;
        if (weight >= 0 && static_cast<int>(font.weight()) != weight)
            return;
        int size = static_cast<int>(font.pixel_size_rounded_up());
        int difference = size > target_size ? size - target_size : target_size - size;
        if (difference < best_difference) {
            best_difference = difference;
            best_name = font.qualified_name().to_byte_string();
        }
    });
    if (best_name.is_empty())
        return nullptr;
    return database.get_by_name(best_name);
}

BoardWidget::BoardWidget()
{
    set_focus_policy(GUI::FocusPolicy::StrongFocus);
    set_fixed_size({ board_pixel_size(), board_pixel_size() });
    m_digit_font = find_font("Liza", 24, 700);
    m_note_font = find_font("Liza", 10, -1);

    m_game.on_change = [this] {
        update();
        if (on_state_change)
            on_state_change();
    };
}

void BoardWidget::new_game(Difficulty difficulty)
{
    m_game.new_game(difficulty);
    m_selected_row = -1;
    m_selected_col = -1;
    update();
}

void BoardWidget::restart()
{
    m_game.restart();
    m_selected_row = -1;
    m_selected_col = -1;
    update();
}

void BoardWidget::set_markup_enabled(bool enabled)
{
    if (m_markup == enabled)
        return;
    m_markup = enabled;
    update();
}

void BoardWidget::set_show_conflicts(bool show)
{
    if (m_show_conflicts == show)
        return;
    m_show_conflicts = show;
    update();
}

Gfx::IntRect BoardWidget::cell_rect(int row, int col) const
{
    return { col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE };
}

void BoardWidget::select(int row, int col)
{
    m_selected_row = clamp(row, 0, Game::size() - 1);
    m_selected_col = clamp(col, 0, Game::size() - 1);
    update();
}

void BoardWidget::move_selection(int delta_x, int delta_y)
{
    if (!has_selection()) {
        select(0, 0);
        return;
    }
    select(m_selected_row + delta_y, m_selected_col + delta_x);
}

void BoardWidget::enter_digit(u8 digit)
{
    if (!has_selection())
        return;

    if (m_markup)
        m_game.toggle_note(m_selected_row, m_selected_col, digit);
    else
        m_game.set_value(m_selected_row, m_selected_col, digit);

    if (!m_markup && m_game.is_solved() && on_solved)
        on_solved();
}

void BoardWidget::mousedown_event(GUI::MouseEvent& event)
{
    if (event.button() != GUI::MouseButton::Primary) {
        GUI::Widget::mousedown_event(event);
        return;
    }

    auto position = event.position();
    if (position.x() < 0 || position.y() < 0 || position.x() >= board_pixel_size() || position.y() >= board_pixel_size())
        return;

    select(position.y() / CELL_SIZE, position.x() / CELL_SIZE);
    set_focus(true);
}

void BoardWidget::keydown_event(GUI::KeyEvent& event)
{
    if (!has_selection()) {
        event.ignore();
        return;
    }

    switch (event.key()) {
    case KeyCode::Key_Up:
        move_selection(0, -1);
        event.accept();
        return;
    case KeyCode::Key_Down:
        move_selection(0, 1);
        event.accept();
        return;
    case KeyCode::Key_Left:
        move_selection(-1, 0);
        event.accept();
        return;
    case KeyCode::Key_Right:
        move_selection(1, 0);
        event.accept();
        return;
    case KeyCode::Key_Backspace:
    case KeyCode::Key_Delete:
        m_game.clear_cell(m_selected_row, m_selected_col);
        event.accept();
        return;
    case KeyCode::Key_Space:
        set_markup_enabled(!m_markup);
        if (on_state_change)
            on_state_change();
        event.accept();
        return;
    default:
        break;
    }

    if (event.code_point() == '0') {
        m_game.clear_cell(m_selected_row, m_selected_col);
        event.accept();
        return;
    }

    if (event.code_point() == 'm' || event.code_point() == 'M') {
        set_markup_enabled(!m_markup);
        if (on_state_change)
            on_state_change();
        event.accept();
        return;
    }

    if (event.code_point() >= '1' && event.code_point() <= '9') {
        enter_digit(static_cast<u8>(event.code_point() - '0'));
        event.accept();
        return;
    }

    event.ignore();
}

void BoardWidget::focusin_event(GUI::FocusEvent& event)
{
    m_has_focus = true;
    update();
    GUI::Widget::focusin_event(event);
}

void BoardWidget::focusout_event(GUI::FocusEvent& event)
{
    m_has_focus = false;
    update();
    GUI::Widget::focusout_event(event);
}

void BoardWidget::paint_event(GUI::PaintEvent& event)
{
    GUI::Widget::paint_event(event);
    GUI::Painter painter(*this);
    painter.add_clip_rect(event.rect());

    auto const& digit_font = m_digit_font ? *m_digit_font : Gfx::FontDatabase::default_font();
    auto const& note_font = m_note_font ? *m_note_font : Gfx::FontDatabase::default_font();

    painter.fill_rect({ 0, 0, board_pixel_size(), board_pixel_size() }, BOARD_BACKGROUND_COLOR);

    if (has_selection()) {
        u8 selected_value = m_game.value(m_selected_row, m_selected_col);
        for (int row = 0; row < Game::size(); ++row) {
            for (int col = 0; col < Game::size(); ++col) {
                bool same_box = (row / 3 == m_selected_row / 3) && (col / 3 == m_selected_col / 3);
                bool is_peer = row == m_selected_row || col == m_selected_col || same_box;
                if (row == m_selected_row && col == m_selected_col)
                    painter.fill_rect(cell_rect(row, col), SELECTION_HIGHLIGHT_COLOR);
                else if (selected_value != 0 && m_game.value(row, col) == selected_value)
                    painter.fill_rect(cell_rect(row, col), SAME_VALUE_HIGHLIGHT_COLOR);
                else if (is_peer)
                    painter.fill_rect(cell_rect(row, col), PEER_HIGHLIGHT_COLOR);
            }
        }
    }

    for (int row = 0; row < Game::size(); ++row) {
        for (int col = 0; col < Game::size(); ++col) {
            auto rect = cell_rect(row, col);
            u8 value = m_game.value(row, col);
            if (value != 0) {
                Gfx::Color color = GIVEN_TEXT_COLOR;
                if (!m_game.is_given(row, col)) {
                    if (m_show_conflicts && m_game.conflicts(row, col))
                        color = CONFLICT_TEXT_COLOR;
                    else
                        color = USER_TEXT_COLOR;
                }
                painter.draw_text(rect, digit_text(value), digit_font, Gfx::TextAlignment::Center, color);
            } else if (m_game.notes(row, col) != 0) {
                for (u8 digit = 1; digit <= 9; ++digit) {
                    if (!m_game.has_note(row, col, digit))
                        continue;
                    int sub_col = (digit - 1) % 3;
                    int sub_row = (digit - 1) / 3;
                    Gfx::IntRect sub_rect {
                        rect.x() + sub_col * rect.width() / 3,
                        rect.y() + sub_row * rect.height() / 3,
                        rect.width() / 3,
                        rect.height() / 3
                    };
                    painter.draw_text(sub_rect, digit_text(digit), note_font, Gfx::TextAlignment::Center, NOTE_TEXT_COLOR);
                }
            }
        }
    }

    for (int i = 0; i <= Game::size(); ++i) {
        int position = i * CELL_SIZE;
        bool is_box_line = (i % 3) == 0;
        auto color = is_box_line ? BOX_LINE_COLOR : GRID_LINE_COLOR;
        int clamped_position = min(position, board_pixel_size() - 1);
        painter.draw_line({ clamped_position, 0 }, { clamped_position, board_pixel_size() - 1 }, color);
        painter.draw_line({ 0, clamped_position }, { board_pixel_size() - 1, clamped_position }, color);
        if (is_box_line && clamped_position + 1 < board_pixel_size()) {
            painter.draw_line({ clamped_position + 1, 0 }, { clamped_position + 1, board_pixel_size() - 1 }, color);
            painter.draw_line({ 0, clamped_position + 1 }, { board_pixel_size() - 1, clamped_position + 1 }, color);
        }
    }

    if (has_selection()) {
        auto rect = cell_rect(m_selected_row, m_selected_col);
        painter.draw_rect(rect, SELECTION_BORDER_COLOR);
        painter.draw_rect(rect.shrunken(1, 1), SELECTION_BORDER_COLOR);
        if (m_has_focus) {
            painter.draw_rect(rect.shrunken(2, 2), SELECTION_BORDER_COLOR);
            painter.draw_rect(rect.shrunken(3, 3), SELECTION_BORDER_COLOR);
        }
    }
}

}
