#include "arena.h"

struct arena_s
{
    byte* arr;
    u64 num_bytes;
};

struct arena_s* arena_create(u64 num_bytes)
{
    struct arena_s* ret = malloc(sizeof(*ret));
    ret->arr = malloc(num_bytes);
    ret->num_bytes = num_bytes;
    return ret;
}

void arena_destroy(struct arena_s* arena_p)
{
    free(arena_p->arr);
    free(arena_p);
}

u64 arena_num_bytes(struct arena_s* arena_p)
{
    return arena_p->num_bytes;
}

void arena_read(byte* dst, struct arena_s* arena_p, u64 starting_byte_index, u64 num_bytes)
{
    memcpy(dst, arena_p->arr + starting_byte_index, num_bytes);
}

void arena_write(struct arena_s* arena_p, u64 starting_byte_index, u64 num_bytes, byte* src)
{
    memcpy(arena_p->arr + starting_byte_index, src, num_bytes);
}

void arena_ensure_space(struct arena_s* arena_p, u64 num_bytes)
{
    if(num_bytes <= arena_p->num_bytes)
        return;
    
    num_bytes = MAX(num_bytes, 2 * arena_p->num_bytes);
    byte* new_arr = malloc(num_bytes);
    memcpy(new_arr, arena_p->arr, arena_p->num_bytes);
    free(arena_p->arr);
    arena_p->arr = new_arr;
    arena_p->num_bytes = num_bytes;
}

void arena_test()
{
    char s1[40] = "hello world";
    char s2[40] = "cats";
    char s3[40] = "dogs";
    char s4[40] = "dogs world";
    struct arena_s* arena_p = arena_create(100);
    arena_write(arena_p, 0, strlen(s1) + 1, (byte*)s1);
    arena_read((byte*)s2, arena_p, 0, strlen(s1) + 1);
    assert(strcmp(s1, s2) == 0);
    arena_read((byte*)(s3) + strlen(s3), arena_p, 5, strlen(s1) + 1 - 5);
    assert(strcmp(s3, s4) == 0);
    arena_destroy(arena_p);
    printf("arena_test passed\n");
}
