#pragma once
#include "base_utils.h"

struct arena_s;

// create an arena instance
struct arena_s* arena_create(u64 num_bytes);

// destroy an arena instance
void arena_destroy(struct arena_s* arena_p);

// get the number of bytes allocated to the arena
u64 arena_num_bytes(struct arena_s* arena_p);

// read from the arena
void arena_read(byte* dst, struct arena_s* arena_p, u64 starting_byte_index, u64 num_bytes);

// write to the arena
void arena_write(struct arena_s* arena_p, u64 starting_byte_index, u64 num_bytes, byte* src);

// ensure the arena has at least N bytes
void arena_ensure_space(struct arena_s* arena_p, u64 num_bytes);

// implementation test
void arena_test(void);
