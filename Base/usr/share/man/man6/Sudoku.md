## Name

![Icon](/res/icons/16x16/app-sudoku.png) Sudoku

[Open](launch:///bin/Sudoku)

## Synopsis

```**sh
$ Sudoku
```

## Description

`Sudoku` is a logic puzzle played on a 9x9 grid, divided into nine 3x3 boxes.

The goal is to fill every empty cell with a digit from 1 to 9 so that each row, each column and each box contains every digit exactly once. The starting clues (`given` cells) cannot be changed.

Click a cell to select it and type a digit to place it. Press `Backspace` or `Delete` to clear a cell, or `0` to erase it. Use the arrow keys to move the selection.

Press `Space` (or `M`) to toggle *Pencil Marks* mode: instead of placing a digit, typing a number toggles it as a candidate in the selected cell.

## Options

* `&Game`
  * `New Game` (`F2`): start a fresh puzzle at the current difficulty.
  * `Difficulty`: choose between Easy, Medium, Hard and Expert.
  * `Restart`: clear your entries and return to the initial clues.
  * `Undo` (`Ctrl+Z`) / `Redo` (`Ctrl+Y`): step through your entries.
  * `Check`: report how many of your entries differ from the solution.
  * `Solve`: reveal the entire solution.
* `&Settings`
  * `Pencil Marks`: toggle candidate marks.
  * `Highlight Conflicts`: highlight cells that clash with a peer.
* `&View`
  * `Fullscreen`: toggle fullscreen mode.
