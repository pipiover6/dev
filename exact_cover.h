#pragma once
#include "base_utils.h"

// when a cover is found a callback is called with the number of rows found in the cover and an array of the rows in cover, and additionaly one can pass it generic parameters using byte* extra
// if no callback is given (null pointer) the solutions are printed to stdout
// return value is the number of exact covers found
u32 find_exact_covers(bool* mat, u32 nrows, u32 ncols, bool stop_at_first_solution, void exact_cover_callback(u32, u32*, byte*), byte* extra);
