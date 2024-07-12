#include "exact_cover.h"
#include "sudoku_solver.h"

// building block for the exact cover matrix
static void sudoku_ecm_helper(bool* ecm, u32 s_col, u32 s_row, u32 s_idx, u32 s_box, u32 s_val)
{
    assert((1 <= s_val) && (s_val <= 9));
    s_val -= 1;

    // row stays the same
    u32 ecm_row = s_val + 9 * s_col + 9 * 9 * s_row;
    assert(ecm_row < 9 * 9 * 9);

    /* cover cell */
    u32 ecm_col = s_idx;
    assert(ecm_col < 9 * 9);
    ecm[ecm_col + 4 * 9 * 9 * ecm_row] = 1;

    /* cover row val */
    ecm_col = (9 * 9) + s_row + 9 * s_val;
    assert(ecm_col < 2 * 9 * 9);
    ecm[ecm_col + 4 * 9 * 9 * ecm_row] = 1;

    /* cover col val */
    ecm_col = 2 * 9 * 9 + s_col + 9 * s_val;
    assert(ecm_col < 3 * 9 * 9);
    ecm[ecm_col + 4 * 9 * 9 * ecm_row] = 1;

    /* cover box val */
    ecm_col = 3 * 9 * 9 + s_box + 9 * s_val;
    assert(ecm_col < 4 * 9 * 9);
    ecm[ecm_col + 4 * 9 * 9 * ecm_row] = 1;
}

// callback
static void sudoku_cb(u32 n, u32* arr, byte* extra)
{
    u32* solution = (u32*) extra;
    u32 x,i,j,v;
    for(u32 idx = 0; idx < n; idx += 1)
    {
        x = arr[idx];
        v = (x % 9) + 1;
        j = (x / 9) % 9;
        i = x / (9 * 9);
        assert(i < 9);
        assert(j < 9);
        solution[j + 9 * i] = v;
    }

    /*
    for(i = 0; i < 9; i += 1)
    {
        for(j = 0; j < 9; j += 1)
        {
            printf("%d ", sudoku[j + 9 * i]);
        }
        printf("\n");
    }
    printf("\n");
    */
}

bool solve_sudoku(u32* sudoku_puzzle, u32* sudoku_solution)
{
    /*                      val col row     cell  row val col val box val */
    bool exact_cover_mat[(9 * 9 * 9) * 4 * 9 * 9] = {0};

    u32 s_row, s_col, s_idx, s_box, s_val;

    for(s_row = 0; s_row < 9; s_row += 1)
    {
        for(s_col = 0; s_col < 9; s_col += 1)
        {
            s_idx = s_col + 9 * s_row;
            s_box = (s_col / 3) + 3 * (s_row / 3);
            s_val = sudoku_puzzle[s_idx];
            if(s_val != 0)
                sudoku_ecm_helper(exact_cover_mat, s_col, s_row, s_idx, s_box, s_val);
            else
            {
                for(s_val = 1; s_val <= 9; s_val += 1)
                {
                    sudoku_ecm_helper(exact_cover_mat, s_col, s_row, s_idx, s_box, s_val);
                }
            }
        }
    }

    return find_exact_covers(exact_cover_mat, 9 * 9 * 9, 4 * 9 * 9, 1, sudoku_cb, (byte*) sudoku_solution);
}

void sudoku_test(void)
{
    u32 puzzle[81] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 6, 2, 0, 0, 0, 0, 5,
    3, 7, 0, 0, 0, 0, 9, 0, 2,
    0, 2, 0, 0, 7, 3, 0, 0, 0,
    0, 3, 0, 4, 0, 1, 0, 7, 0,
    0, 0, 0, 8, 2, 0, 0, 5, 0,
    1, 0, 5, 0, 0, 0, 0, 2, 6,
    7, 0, 0, 0, 0, 9, 5, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0};

    u32 solution[81] = {
    2, 5, 9, 1, 3, 4, 8, 6, 7, 
    4, 8, 6, 2, 9, 7, 1, 3, 5, 
    3, 7, 1, 6, 8, 5, 9, 4, 2, 
    5, 2, 4, 9, 7, 3, 6, 1, 8, 
    6, 3, 8, 4, 5, 1, 2, 7, 9, 
    9, 1, 7, 8, 2, 6, 4, 5, 3, 
    1, 9, 5, 7, 4, 8, 3, 2, 6, 
    7, 4, 2, 3, 6, 9, 5, 8, 1, 
    8, 6, 3, 5, 1, 2, 7, 9, 4};

    solve_sudoku(puzzle, puzzle);
    for(u32 i = 0; i < 81; i += 1)
        assert(puzzle[i] == solution[i]);
    printf("sudoku_test passed\n");
}
