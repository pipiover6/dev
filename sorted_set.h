#pragma once

#include "base_utils.h"

struct sorted_set_s;

// an element in the sorted set is generic, namely any byte concatenation of fixed width
// one has to provide == and < functions between any pair of elements
// the sorted set api allows to get and remove elements by rank (which is 1-based, meaning rank = 1 <--> minimum, rank = num_elements <--> maximum)

// create a sorted set instance
struct sorted_set_s* sorted_set_create(u32 num_bytes_per_elmnt, bool (*is_match)(byte*, byte*), bool (*is_less)(byte*, byte*));

// destroy a sorted set instance
void sorted_set_destroy(struct sorted_set_s* sorted_set_p);

// get the number of elements stored in the sorted set
u32 sorted_set_get_num_elements(struct sorted_set_s* sorted_set_p);

// insert an element to the sorted set
// if element is already in the set, we still 'insert it' in place of the matching element, useful for key-value maps
// pass rank_p = GORNISHT if you don't care for the rank
// return value is if element already existed in the set
bool sorted_set_insert(struct sorted_set_s* sorted_set_p, byte* element, u32* rank_copy_p);

// remove an element from the sorted set
// pass rank_p = GORNISHT if you don't care for the rank
// return value is if element already existed in the set
bool sorted_set_remove(struct sorted_set_s* sorted_set_p, byte* element, u32* rank_copy_p);

// search if the sorted set contains a certain element
// if element is in the set, we copy it to byte* element_found, unless GORNISHT is passed
// pass rank_p = GORNISHT if you don't care for the rank
// return value is if element already existed in the set
bool sorted_set_contains(struct sorted_set_s* sorted_set_p, byte* element, byte* element_found, u32* rank_copy_p);

// get the element of a certain rank
void sorted_set_get_element_by_rank(struct sorted_set_s* sorted_set_p, u32 rank, byte* element_copy_p);

// remove the element of a certain rank
void sorted_set_remove_by_rank(struct sorted_set_s* sorted_set_p, u32 rank, byte* element_copy_p);

// apply a function to each of element in the sorted set, from min to max
void walk_in_order(struct sorted_set_s* sorted_set_p, void (*fnc)(byte*));

// apply a function to each of element in the sorted set, from max to min
void walk_in_reverse(struct sorted_set_s* sorted_set_p, void (*fnc)(byte*));

void sorted_set_test(void);
