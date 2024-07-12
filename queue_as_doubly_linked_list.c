#include "queue.h"

struct doubly_linked_node_s
{
    struct doubly_linked_node_s* neighbours[2];
	// the element is stored here, after the struct
};

static byte* doubly_linked_node_get_element(struct doubly_linked_node_s* node_p)
{
    assert(GORNISHT != node_p);
	return ((byte*) node_p) + sizeof(struct doubly_linked_node_s);
}

static struct doubly_linked_node_s* doubly_linked_node_create(byte* element, u32 num_bytes_per_elmnt)
{
    struct doubly_linked_node_s* ret = malloc(sizeof(*ret) + num_bytes_per_elmnt);
    ret->neighbours[0] = ret->neighbours[1] = GORNISHT;
    memcpy(doubly_linked_node_get_element(ret), element, num_bytes_per_elmnt);
    return ret;
}

struct queue_s
{
    struct doubly_linked_node_s* head_and_tail[2];
    u32 num_elements;
    u32 num_bytes_per_elmnt;
};

struct queue_s* queue_create(u32 num_bytes_per_elmnt)
{
    assert(0 < num_bytes_per_elmnt);
    struct queue_s* ret;
    ret = malloc(sizeof(*ret));
    ret->head_and_tail[0] = ret->head_and_tail[1] = GORNISHT;
    ret->num_elements = 0;
    ret->num_bytes_per_elmnt = num_bytes_per_elmnt;
    return ret;
}

void queue_destroy(struct queue_s* queue_p)
{
    struct doubly_linked_node_s* cur = queue_p->head_and_tail[0];
    struct doubly_linked_node_s* tmp;
    while(GORNISHT != cur)
    {
        tmp = cur->neighbours[1];
        free(cur);
        cur = tmp;
    }
    free(queue_p);
}

u64 queue_num_elmnts(struct queue_s* queue_p)
{
    return queue_p->num_elements;
}

bool queue_is_empty(struct queue_s* queue_p)
{
    bool ret1 = 0 == queue_p->num_elements;
    bool ret2 = GORNISHT == queue_p->head_and_tail[0];
    bool ret3 = GORNISHT == queue_p->head_and_tail[1];
    assert(ret1 == ret2);
    assert(ret2 == ret3);
    return ret1;
}

static void queue_peek(struct queue_s* queue_p, byte* dst, bool dir)
{
    assert(!queue_is_empty(queue_p));
    memcpy(dst, doubly_linked_node_get_element(queue_p->head_and_tail[dir]), queue_p->num_bytes_per_elmnt);
}

static void queue_pop(struct queue_s* queue_p, byte* dst, bool dir)
{
    queue_peek(queue_p, dst, dir);

    struct doubly_linked_node_s* head_or_tail = queue_p->head_and_tail[dir];
    struct doubly_linked_node_s* next_or_prev = head_or_tail->neighbours[1 - dir];

    assert(GORNISHT == head_or_tail->neighbours[dir]);
    assert((queue_p->num_elements > 1) == (GORNISHT != next_or_prev));

    if(queue_p->num_elements > 1)
    {
        assert(next_or_prev->neighbours[dir] == head_or_tail);
        next_or_prev->neighbours[dir] = GORNISHT;
    }
    else
        queue_p->head_and_tail[1 - dir] = GORNISHT;
    queue_p->head_and_tail[dir] = next_or_prev;
    queue_p->num_elements -= 1;
    free(head_or_tail);
}

static void queue_push(struct queue_s* queue_p, byte* src, bool dir)
{
    struct doubly_linked_node_s* new = doubly_linked_node_create(src, queue_p->num_bytes_per_elmnt);
    if(queue_is_empty(queue_p))
        queue_p->head_and_tail[1 - dir] = new;
    else
    {
        new->neighbours[1 - dir] = queue_p->head_and_tail[dir];
        assert(GORNISHT == queue_p->head_and_tail[dir]->neighbours[dir]);
        queue_p->head_and_tail[dir]->neighbours[dir] = new;
    }
    queue_p->head_and_tail[dir] = new;
    queue_p->num_elements += 1;
}


void queue_peek_head(struct queue_s* queue_p, byte* dst)
{
    queue_peek(queue_p, dst, 0);
}

void queue_peek_tail(struct queue_s* queue_p, byte* dst)
{
    queue_peek(queue_p, dst, 1);
}

void queue_pop_head(struct queue_s* queue_p, byte* dst)
{
    queue_pop(queue_p, dst, 0);
}

void queue_pop_tail(struct queue_s* queue_p, byte* dst)
{
    queue_pop(queue_p, dst, 1);
}

void queue_push_head(struct queue_s* queue_p, byte* src)
{
    queue_push(queue_p, src, 0);
}

void queue_push_tail(struct queue_s* queue_p, byte* src)
{
    queue_push(queue_p, src, 1);
}

void queue_test()
{
    u64 arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, tmp;
    struct queue_s* queue_p = queue_create(sizeof(u64));
    assert(queue_is_empty(queue_p));
    for(u64 i = 0; i < 10; i += 1)
    {
        assert(queue_num_elmnts(queue_p) == 2 * i);
        tmp = 10 + arr[i];
        QUEUE_PUSH_TAIL(queue_p, arr[i]);
        QUEUE_PUSH_HEAD(queue_p, tmp);
    }

    for(u64 i = 0; i < 20; i += 1)
    {
        QUEUE_POP_HEAD(queue_p, tmp);
        //printf("%lu\n", tmp);
        if(i < 10)
            assert(tmp == 19 - i);
        else
            assert(tmp == i - 10);
    }
    assert(queue_is_empty(queue_p));
    QUEUE_PUSH_HEAD(queue_p, arr[4]);
    QUEUE_PUSH_HEAD(queue_p, arr[3]);

    queue_destroy(queue_p);
    printf("queue_test passed\n");
}
