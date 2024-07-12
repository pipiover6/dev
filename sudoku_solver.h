#pragma once
#include "base_utils.h"

// a sudoku is encoded as an array of 81 numbers (0-9) with 0 representing anything
// we return the first solution we find (if any)
bool solve_sudoku(u32* sudoku_puzzle, u32* sudoku_solution);

void sudoku_test(void);
