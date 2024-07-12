#pragma once
#include "base_utils.h"

struct queue_s;

// create a queue instance
struct queue_s* queue_create(u32 num_bytes_per_elmnt);

// destroy a queue instance
void queue_destroy(struct queue_s* queue_p);

// get the number of elements stored in the queue
u64 queue_num_elmnts(struct queue_s* queue_p);

// returns true when queue has 0 elements
bool queue_is_empty(struct queue_s* queue_p);

void queue_peek_head(struct queue_s* queue_p, byte* dst);
#define QUEUE_PEEK_HEAD(queue_p, x) (queue_peek_head(queue_p, (byte*)(&(x))))

void queue_pop_head(struct queue_s* queue_p, byte* dst);
#define QUEUE_POP_HEAD(queue_p, x) (queue_pop_head(queue_p, (byte*)(&(x))))

void queue_push_head(struct queue_s* queue_p, byte* src);
#define QUEUE_PUSH_HEAD(queue_p, x) (queue_push_head(queue_p, (byte*)(&(x))))

void queue_peek_tail(struct queue_s* queue_p, byte* dst);
#define QUEUE_PEEK_TAIL(queue_p, x) (queue_peek_tail(queue_p, (byte*)(&(x))))

void queue_pop_tail(struct queue_s* queue_p, byte* dst);
#define QUEUE_POP_TAIL(queue_p, x) (queue_pop_tail(queue_p, (byte*)(&(x))))

void queue_push_tail(struct queue_s* queue_p, byte* src);
#define QUEUE_PUSH_TAIL(queue_p, x) (queue_push_tail(queue_p, (byte*)(&(x))))

// implementation test
void queue_test(void);
