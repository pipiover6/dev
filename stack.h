#pragma once
#include "base_utils.h"

struct stack_s;

// create a stack instance
struct stack_s* stack_create(u64 num_bytes_per_elmnt);

// destroy a stack instance
void stack_destroy(struct stack_s* stack_p);

// get the number of elements stored in the stack
u64 stack_num_elmnts(struct stack_s* stack_p);

// returns true when stack has 0 elements
bool stack_is_empty(struct stack_s* stack_p);

// get the top element of the stack
void stack_peek(struct stack_s* stack_p, byte* dst);
#define STACK_PEEK(stack_p, x) (stack_peek(stack_p, (byte*)(&(x))))

// pop the top element of the stack
void stack_pop(struct stack_s* stack_p, byte* dst);
#define STACK_POP(stack_p, x) (stack_pop(stack_p, (byte*)(&(x))))

// push an element to the top of the stack
void stack_push(struct stack_s* stack_p, byte* src);
#define STACK_PUSH(stack_p, x) (stack_push(stack_p, (byte*)(&(x))))

// implementation test
void stack_test(void);
