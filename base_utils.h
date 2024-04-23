#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "assert.h"
#include "stdbool.h"

typedef uint64_t u64;
typedef uint32_t u32;

// a byte pointer could point to any generic/unknown data type
typedef uint8_t byte;

// Jewish null pointer
#define GORNISHT 0

#define MAX(a, b) ( (a) < (b) ? (b) : (a) )
#define MIN(a, b) ( (a) < (b) ? (a) : (b) )
