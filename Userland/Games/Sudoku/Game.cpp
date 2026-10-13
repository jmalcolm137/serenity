/*
 * Copyright (c) 2026, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Game.h"
#include <AK/Assertions.h>
#include <AK/Random.h>
#include <AK/StdLibExtras.h>

namespace Sudoku {

ByteString difficulty_name(Difficulty difficulty)
{
    switch (difficulty) {
    case Difficulty::Beginner:
        return "Beginner";
    case Difficulty::Intermediate:
        return "Intermediate";
    case Difficulty::Expert:
        return "Expert";
    case Difficulty::Madwoman:
        return "Madwoman";
    }
    VERIFY_NOT_REACHED();
}

namespace {

int target_clue_count(Difficulty difficulty)
{
    switch (difficulty) {
    case Difficulty::Beginner:
        return 40;
    case Difficulty::Intermediate:
        return 33;
    case Difficulty::Expert:
        return 28;
    case Difficulty::Madwoman:
        return 24;
    }
    VERIFY_NOT_REACHED();
}

// A bitmask-based Sudoku solver. It always branches on the empty cell with the
// fewest candidates, which keeps solving and uniqueness checking fast.
class Solver {
public:
    void reset()
    {
        m_grid.fill(0);
        m_rows.fill(0);
        m_cols.fill(0);
        m_boxes.fill(0);
    }

    void load(Array<u8, 81> const& source)
    {
        reset();
        for (int i = 0; i < 81; ++i) {
            if (source[i] != 0)
                place(i / 9, i % 9, source[i]);
        }
    }

    bool solve()
    {
        int row = 0;
        int col = 0;
        u16 candidate_mask = 0;
        switch (find_best_empty(row, col, candidate_mask)) {
        case Search::Solved:
            return true;
        case Search::DeadEnd:
            return false;
        case Search::Branch:
            break;
        }

        for (u8 digit = 1; digit <= 9; ++digit) {
            if ((candidate_mask & (1u << digit)) == 0)
                continue;
            place(row, col, digit);
            if (solve())
                return true;
            remove(row, col);
        }
        return false;
    }

    bool solve_random()
    {
        int row = 0;
        int col = 0;
        u16 candidate_mask = 0;
        switch (find_best_empty(row, col, candidate_mask)) {
        case Search::Solved:
            return true;
        case Search::DeadEnd:
            return false;
        case Search::Branch:
            break;
        }

        Array<u8, 9> digits { };
        int digit_count = 0;
        for (u8 digit = 1; digit <= 9; ++digit) {
            if ((candidate_mask & (1u << digit)) != 0)
                digits[digit_count++] = digit;
        }
        for (int i = digit_count - 1; i > 0; --i)
            swap(digits[i], digits[get_random_uniform(i + 1)]);

        for (int i = 0; i < digit_count; ++i) {
            place(row, col, digits[i]);
            if (solve_random())
                return true;
            remove(row, col);
        }
        return false;
    }

    int count_solutions(int limit)
    {
        if (limit <= 0)
            return 0;

        int row = 0;
        int col = 0;
        u16 candidate_mask = 0;
        switch (find_best_empty(row, col, candidate_mask)) {
        case Search::Solved:
            return 1;
        case Search::DeadEnd:
            return 0;
        case Search::Branch:
            break;
        }

        int found = 0;
        for (u8 digit = 1; digit <= 9; ++digit) {
            if ((candidate_mask & (1u << digit)) == 0)
                continue;
            place(row, col, digit);
            found += count_solutions(limit - found);
            remove(row, col);
            if (found >= limit)
                break;
        }
        return found;
    }

    Array<u8, 81> const& result() const { return m_grid; }

private:
    enum class Search {
        Solved,
        DeadEnd,
        Branch,
    };

    static int box_index(int row, int col) { return (row / 3) * 3 + col / 3; }

    void place(int row, int col, u8 digit)
    {
        u16 mask = static_cast<u16>(1u << digit);
        m_grid[row * 9 + col] = digit;
        m_rows[row] |= mask;
        m_cols[col] |= mask;
        m_boxes[box_index(row, col)] |= mask;
    }

    void remove(int row, int col)
    {
        u16 mask = static_cast<u16>(1u << m_grid[row * 9 + col]);
        m_grid[row * 9 + col] = 0;
        m_rows[row] &= static_cast<u16>(~mask);
        m_cols[col] &= static_cast<u16>(~mask);
        m_boxes[box_index(row, col)] &= static_cast<u16>(~mask);
    }

    u16 candidates(int row, int col) const
    {
        return static_cast<u16>(~(m_rows[row] | m_cols[col] | m_boxes[box_index(row, col)]) & 0x3fe);
    }

    Search find_best_empty(int& row, int& col, u16& out_candidates) const
    {
        bool found_empty = false;
        int best_count = 10;
        for (int i = 0; i < 81; ++i) {
            if (m_grid[i] != 0)
                continue;
            found_empty = true;
            int candidate_row = i / 9;
            int candidate_col = i % 9;
            u16 candidate_mask = candidates(candidate_row, candidate_col);
            int count = __builtin_popcount(candidate_mask);
            if (count == 0)
                return Search::DeadEnd;
            if (count < best_count) {
                best_count = count;
                row = candidate_row;
                col = candidate_col;
                out_candidates = candidate_mask;
                if (count == 1)
                    break;
            }
        }
        return found_empty ? Search::Branch : Search::Solved;
    }

    Array<u8, 81> m_grid { };
    Array<u16, 9> m_rows { };
    Array<u16, 9> m_cols { };
    Array<u16, 9> m_boxes { };
};

void generate_solution(Array<u8, 81>& solution)
{
    Solver solver;
    solver.reset();
    solver.solve_random();
    solution = solver.result();
}

// Remove cells from a full grid while keeping the solution unique. We aim for
// target_clues, but never remove a cell that would make the puzzle ambiguous.
void carve_puzzle(Array<u8, 81>& grid, int target_clues)
{
    Array<int, 81> order;
    for (int i = 0; i < 81; ++i)
        order[i] = i;
    for (int i = 80; i > 0; --i)
        swap(order[i], order[get_random_uniform(i + 1)]);

    int clues = 81;
    for (int i = 0; i < 81 && clues > target_clues; ++i) {
        int cell = order[i];
        u8 saved = grid[cell];
        grid[cell] = 0;

        Solver solver;
        solver.load(grid);
        if (solver.count_solutions(2) != 1)
            grid[cell] = saved;
        else
            --clues;
    }
}

}

Game::Game()
{
    new_game(Difficulty::Beginner);
}

void Game::new_game(Difficulty difficulty)
{
    m_difficulty = difficulty;
    m_undo.clear();
    m_redo.clear();

    Array<u8, 81> solution;
    generate_solution(solution);

    Array<u8, 81> puzzle = solution;
    carve_puzzle(puzzle, target_clue_count(difficulty));

    for (int i = 0; i < 81; ++i) {
        m_puzzle[i] = puzzle[i];
        m_solution[i] = solution[i];
        m_values[i] = puzzle[i];
        m_given[i] = puzzle[i] != 0;
        m_notes[i] = 0;
    }

    if (on_change)
        on_change();
}

void Game::restart()
{
    m_undo.clear();
    m_redo.clear();
    for (int i = 0; i < 81; ++i) {
        m_values[i] = m_puzzle[i];
        m_notes[i] = 0;
    }
    if (on_change)
        on_change();
}

void Game::set_value(int row, int col, u8 digit)
{
    int cell = cell_index(row, col);
    if (m_given[cell] || digit > 9)
        return;

    // Entering the same digit again clears the cell.
    u8 new_value = m_values[cell] == digit ? 0 : digit;
    u16 new_notes = new_value == 0 ? m_notes[cell] : 0;
    record_and_apply(cell, new_value, new_notes);
}

void Game::clear_cell(int row, int col)
{
    int cell = cell_index(row, col);
    if (m_given[cell])
        return;
    record_and_apply(cell, 0, 0);
}

void Game::toggle_note(int row, int col, u8 digit)
{
    int cell = cell_index(row, col);
    if (m_given[cell] || m_values[cell] != 0 || digit < 1 || digit > 9)
        return;
    record_and_apply(cell, 0, static_cast<u16>(m_notes[cell] ^ note_mask(digit)));
}

void Game::record_and_apply(int cell, u8 new_value, u16 new_notes)
{
    if (m_values[cell] == new_value && m_notes[cell] == new_notes)
        return;

    m_undo.append(Move { cell, m_values[cell], m_notes[cell], new_value, new_notes });
    m_redo.clear();
    m_values[cell] = new_value;
    m_notes[cell] = new_notes;

    if (on_change)
        on_change();
}

bool Game::conflicts(int row, int col) const
{
    u8 digit = value(row, col);
    if (digit == 0)
        return false;

    for (int i = 0; i < size(); ++i) {
        if (i != col && value(row, i) == digit)
            return true;
        if (i != row && value(i, col) == digit)
            return true;
    }

    int box_row = (row / 3) * 3;
    int box_col = (col / 3) * 3;
    for (int r = box_row; r < box_row + 3; ++r) {
        for (int c = box_col; c < box_col + 3; ++c) {
            if ((r != row || c != col) && value(r, c) == digit)
                return true;
        }
    }
    return false;
}

bool Game::is_solved() const
{
    for (int i = 0; i < 81; ++i) {
        if (m_values[i] == 0)
            return false;
    }
    for (int row = 0; row < size(); ++row) {
        for (int col = 0; col < size(); ++col) {
            if (conflicts(row, col))
                return false;
        }
    }
    return true;
}

int Game::remaining_count() const
{
    int count = 0;
    for (int i = 0; i < 81; ++i) {
        if (m_values[i] == 0)
            ++count;
    }
    return count;
}

int Game::mistake_count() const
{
    int count = 0;
    for (int i = 0; i < 81; ++i) {
        if (!m_given[i] && m_values[i] != 0 && m_values[i] != m_solution[i])
            ++count;
    }
    return count;
}

void Game::reveal_solution()
{
    for (int i = 0; i < 81; ++i) {
        m_values[i] = m_solution[i];
        m_notes[i] = 0;
    }
    m_undo.clear();
    m_redo.clear();
    if (on_change)
        on_change();
}

void Game::undo()
{
    if (m_undo.is_empty())
        return;
    auto move = m_undo.take_last();
    m_values[move.cell] = move.old_value;
    m_notes[move.cell] = move.old_notes;
    m_redo.append(move);
    if (on_change)
        on_change();
}

void Game::redo()
{
    if (m_redo.is_empty())
        return;
    auto move = m_redo.take_last();
    m_values[move.cell] = move.new_value;
    m_notes[move.cell] = move.new_notes;
    m_undo.append(move);
    if (on_change)
        on_change();
}

}
