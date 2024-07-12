#include "stack.h"
#include "arena.h"

struct stack_s
{
    struct arena_s* arena_p;
    u64 num_bytes_per_elmnt;
    u64 num_elmnts;
};

struct stack_s* stack_create(u64 num_bytes_per_elmnt)
{
    struct stack_s* ret;
    assert(0 < num_bytes_per_elmnt);
    ret = malloc(sizeof(*ret));
    ret->arena_p = arena_create(0);
    ret->num_bytes_per_elmnt = num_bytes_per_elmnt;
    ret->num_elmnts = 0;
    return ret;
}

void stack_destroy(struct stack_s* stack_p)
{
    arena_destroy(stack_p->arena_p);
    free(stack_p);
}

u64 stack_num_elmnts(struct stack_s* stack_p)
{
    return stack_p->num_elmnts;
}

bool stack_is_empty(struct stack_s* stack_p)
{
    return 0 == stack_num_elmnts(stack_p);
}

void stack_peek(struct stack_s* stack_p, byte* dst)
{
    assert(!stack_is_empty(stack_p));
    arena_read(dst, stack_p->arena_p, (stack_p->num_elmnts - 1) * stack_p->num_bytes_per_elmnt, stack_p->num_bytes_per_elmnt);
}

void stack_pop(struct stack_s* stack_p, byte* dst)
{
    stack_peek(stack_p, dst);
    stack_p->num_elmnts -= 1;
}

void stack_push(struct stack_s* stack_p, byte* src)
{
    arena_ensure_space(stack_p->arena_p, (stack_p->num_elmnts + 1) * stack_p->num_bytes_per_elmnt);
    arena_write(stack_p->arena_p, stack_p->num_elmnts * stack_p->num_bytes_per_elmnt, stack_p->num_bytes_per_elmnt, src);
    stack_p->num_elmnts += 1;
}

// implementation test
void stack_test()
{
    u64 arr[5] = {0, 1, 2, 3, 4};
    u64 cur = UINT64_MAX;
    struct stack_s* stack_p = stack_create(sizeof(u64)); // []
    assert(0 == stack_num_elmnts(stack_p));
    STACK_PUSH(stack_p, arr[0]); // [0]
    STACK_PUSH(stack_p, arr[1]); // [0,1]
    STACK_PUSH(stack_p, arr[2]); // [0,1,2]
    STACK_PEEK(stack_p, cur); // [0,1,2]
    assert(2 == cur);
    assert(3 == stack_num_elmnts(stack_p));
    STACK_PUSH(stack_p, arr[3]); // [0,1,2,3]
    STACK_PUSH(stack_p, arr[4]); // [0,1,2,3,4]
    STACK_POP(stack_p, cur); // [0,1,2,3]
    assert(4 == cur);
    assert(4 == stack_num_elmnts(stack_p));
    STACK_POP(stack_p, cur); // [0,1,2]
    assert(3 == cur);
    assert(3 == stack_num_elmnts(stack_p));

    stack_destroy(stack_p);
    printf("stack_test passed\n");
}
