/*
 * Copyright (c) 2026, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Array.h>
#include <AK/ByteString.h>
#include <AK/Function.h>
#include <AK/Types.h>
#include <AK/Vector.h>

namespace Sudoku {

enum class Difficulty {
    Easy,
    Medium,
    Hard,
    Diabolical,
};

ByteString difficulty_name(Difficulty);

// The puzzle model: a 9x9 grid of values plus pencil marks, the unique solution
// to check against, and an undo/redo history.
class Game {
public:
    Game();

    static constexpr int size() { return 9; }
    static u16 note_mask(u8 digit) { return static_cast<u16>(1u << digit); }

    void new_game(Difficulty);
    // Restore the puzzle to its initial clues, discarding user entries.
    void restart();

    Difficulty difficulty() const { return m_difficulty; }

    u8 value(int row, int col) const { return m_values[cell_index(row, col)]; }
    bool is_given(int row, int col) const { return m_given[cell_index(row, col)]; }
    u16 notes(int row, int col) const { return m_notes[cell_index(row, col)]; }
    bool has_note(int row, int col, u8 digit) const { return (notes(row, col) & note_mask(digit)) != 0; }

    // Enter a digit, or clear the cell when digit is 0.
    void set_value(int row, int col, u8 digit);
    void clear_cell(int row, int col);
    void toggle_note(int row, int col, u8 digit);

    // Whether the cell's value clashes with another cell in its row, column or box.
    bool conflicts(int row, int col) const;
    bool is_solved() const;

    int remaining_count() const;
    // The number of non-given entries that differ from the solution.
    int mistake_count() const;

    void reveal_solution();

    bool can_undo() const { return !m_undo.is_empty(); }
    bool can_redo() const { return !m_redo.is_empty(); }
    void undo();
    void redo();

    Function<void()> on_change;

private:
    static constexpr int cell_index(int row, int col) { return row * 9 + col; }

    struct Move {
        int cell { 0 };
        u8 old_value { 0 };
        u16 old_notes { 0 };
        u8 new_value { 0 };
        u16 new_notes { 0 };
    };

    void record_and_apply(int cell, u8 new_value, u16 new_notes);

    Array<u8, 81> m_puzzle { };
    Array<u8, 81> m_values { };
    Array<u8, 81> m_solution { };
    Array<bool, 81> m_given { };
    Array<u16, 81> m_notes { };
    Difficulty m_difficulty { Difficulty::Easy };
    Vector<Move> m_undo;
    Vector<Move> m_redo;
};

}
