/*
 * Copyright (c) 2026, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Game.h"
#include <AK/Function.h>
#include <AK/RefPtr.h>
#include <LibGUI/Widget.h>
#include <LibGfx/Font/Font.h>

namespace Sudoku {

class BoardWidget : public GUI::Widget {
    C_OBJECT(BoardWidget);

public:
    virtual ~BoardWidget() override = default;

    static constexpr int board_pixel_size() { return CELL_SIZE * Game::size(); }

    void new_game(Difficulty);
    void restart();

    Game& game() { return m_game; }
    Game const& game() const { return m_game; }

    bool markup_enabled() const { return m_markup; }
    void set_markup_enabled(bool);

    bool show_conflicts() const { return m_show_conflicts; }
    void set_show_conflicts(bool);

    bool has_selection() const { return m_selected_row >= 0; }
    void select(int row, int col);

    Function<void()> on_state_change;
    Function<void()> on_solved;

private:
    BoardWidget();

    virtual void paint_event(GUI::PaintEvent&) override;
    virtual void mousedown_event(GUI::MouseEvent&) override;
    virtual void keydown_event(GUI::KeyEvent&) override;
    virtual void focusin_event(GUI::FocusEvent&) override;
    virtual void focusout_event(GUI::FocusEvent&) override;

    void move_selection(int delta_x, int delta_y);
    void enter_digit(u8 digit);
    Gfx::IntRect cell_rect(int row, int col) const;

    static constexpr int CELL_SIZE { 54 };

    Game m_game;
    int m_selected_row { -1 };
    int m_selected_col { -1 };
    bool m_markup { false };
    bool m_show_conflicts { true };
    bool m_has_focus { false };

    RefPtr<Gfx::Font const> m_digit_font;
    RefPtr<Gfx::Font const> m_note_font;
};

}
