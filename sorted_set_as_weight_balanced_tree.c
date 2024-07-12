#include "sorted_set.h"
#include "stack.h"

// ******* 1) binary nodes *******

struct binary_node_struct
{
    struct binary_node_struct* child[2];
    u32 size;   // size of subtree rooted at this node
	// the element is stored here, after the struct
};

#define NIL ((struct binary_node_struct*) (&nil_))
static struct binary_node_struct nil_ = {{NIL, NIL}, 0};

static byte* binary_node_get_element(struct binary_node_struct* node_p)
{
    assert(node_p != NIL);
	return ((byte*) node_p) + sizeof(struct binary_node_struct);
}

static struct binary_node_struct* leaf_create(byte* element, u32 num_bytes_per_elmnt)
{
    struct binary_node_struct* ret = malloc(sizeof(*ret) + num_bytes_per_elmnt);
    ret->child[0] = ret->child[1] = NIL;
    ret->size = 1;
    memcpy(binary_node_get_element(ret), element, num_bytes_per_elmnt);
    return ret;
}

static void subtree_destroy(struct binary_node_struct* node_p, u32 num_bytes_per_elmnt)
{
    if(node_p == NIL)
        return;
    subtree_destroy(node_p->child[0], num_bytes_per_elmnt);
    subtree_destroy(node_p->child[1], num_bytes_per_elmnt);
    free(node_p);
}

// dir = 0 for in order walk, dir = 1 for in reverse order walk
static void walk(struct binary_node_struct* node_p, bool dir, void (*fnc)(byte*))
{
    if(node_p == NIL)
        return;
    walk(node_p->child[dir], dir, fnc);
    fnc(binary_node_get_element(node_p));
    walk(node_p->child[1 - dir], dir, fnc);
}

static void update_size(struct binary_node_struct* node_p)
{
    assert(node_p != NIL);
    node_p->size = 1 + node_p->child[0]->size + node_p->child[1]->size;
}

/*
              B x C y A       
        y                   x
       / \                 / \
      x   A               B   y
     / \                     / \
    B   C                   C   A 
*/
static struct binary_node_struct* rotation(struct binary_node_struct* y, bool dir)
{
    struct binary_node_struct* x = y->child[1 - dir];
    u32 temp = y->size;
    assert(y != NIL); assert(x != NIL);
    y->child[1 - dir] = x->child[dir];
    x->child[dir] = y;
    update_size(y); update_size(x);
    assert(x->size == temp);
    return x;
}

/*
            B y C x D z A       
        z                   x
       / \                /   \
      y   A              y     z
     / \                / \   / \
    B   x              B   C D   A 
       / \
      C   D
*/
static struct binary_node_struct* double_rotation(struct binary_node_struct* z, bool dir)
{
    struct binary_node_struct* y = z->child[1 - dir];
    struct binary_node_struct* x = y->child[dir];
    u32 temp = z->size;
    assert(z != NIL); assert(y != NIL); assert(x != NIL);
    y->child[dir] = x->child[1 - dir];
    x->child[1 - dir] = y;
    z->child[1 - dir] = x->child[dir]; 
    x->child[dir] = z;
    update_size(z); update_size(y); update_size(x);
    assert(x->size == temp);
    return x;
}

// we will only use 2:3 and 2:5 ratios, see https://yoichihirai.com/bst.pdf
static bool node_imbalanced(struct binary_node_struct* node_p, u32 n1, u32 n2, bool dir)
{
    assert(node_p != NIL);
    assert(0 < MIN(n1, n2));
    return n1 * (node_p->child[1 - dir]->size + 1) < n2 * (node_p->child[dir]->size + 1);
}

// each node in the tree has one owner (the address where we store the pointer to the node)
// it could be the root of the tree or one of the sons of a parent node
static void node_rebalance(struct binary_node_struct* node_p, struct binary_node_struct** owner)
{
    int dir;
    assert(node_p != NIL);
    assert(*owner == node_p);
    for(dir = 0; dir <= 1; dir += 1)
    {
        if(node_imbalanced(node_p, 5, 2, dir))
        {
            if(node_imbalanced(node_p->child[dir], 2, 3, dir))
                *owner = rotation(node_p, 1 - dir);
            else
                *owner = double_rotation(node_p, 1 - dir);
            return;
        }
    }
}

// ******* 2) sorted set *******

struct sorted_set_s
{
    struct binary_node_struct* root_p;
    bool (*is_match)(byte*, byte*);	// equal (=) [we can't compare bytes if the element is a (key,val) pair or a pointer to a string etc]
	bool (*is_less)(byte*, byte*); // less than (<)
    u32 num_bytes_per_elmnt;
};

u32 sorted_set_get_num_elements(struct sorted_set_s* sorted_set_p)
{
    return sorted_set_p->root_p->size;
}

struct sorted_set_s* sorted_set_create(u32 num_bytes_per_elmnt, bool (*is_match)(byte*, byte*), bool (*is_less)(byte*, byte*))
{
    struct sorted_set_s* ret = malloc(sizeof(*ret));
    ret->root_p = NIL;
    ret->num_bytes_per_elmnt = num_bytes_per_elmnt;
    ret->is_less = is_less;
    ret->is_match = is_match;
    return ret;
}

void sorted_set_destroy(struct sorted_set_s* sorted_set_p)
{
    subtree_destroy(sorted_set_p->root_p, sorted_set_p->num_bytes_per_elmnt);
    free(sorted_set_p);
}

// when looking for an element in the sorted set, we construct a search path
// (nodes + directions) stemming from the root 
//   if the element is found we'll have k + 1 nodes and k directions, the last node storing the element
//   otherwise we'll have k nodes and k directions, the last node's son in the last direction is NIL and is where the element should be

static struct binary_node_struct** get_owner(struct sorted_set_s* sorted_set_p, struct stack_s* search_nodes, struct stack_s* search_dirs)
{
    struct binary_node_struct* parent;
    bool dir;
    assert(stack_num_elmnts(search_nodes) == stack_num_elmnts(search_dirs));
    if(stack_is_empty(search_nodes))
        return &(sorted_set_p->root_p);
    STACK_PEEK(search_nodes, parent); assert(parent != NIL);
    STACK_PEEK(search_dirs, dir);
    return &(parent->child[dir]);
}

static void search_path_rebalance(struct sorted_set_s* sorted_set_p, struct stack_s* search_nodes, struct stack_s* search_dirs)
{
    struct binary_node_struct** owner;
    struct binary_node_struct* current;
    bool junk;
    assert(stack_num_elmnts(search_nodes) == stack_num_elmnts(search_dirs));
    while(!stack_is_empty(search_nodes))
    {
        STACK_POP(search_nodes, current);
        STACK_POP(search_dirs, junk);
        owner = get_owner(sorted_set_p, search_nodes, search_dirs);
        update_size(current);
        node_rebalance(current, owner);
    }
}

static void remove_helper(struct sorted_set_s* sorted_set_p, struct binary_node_struct* node_p, struct stack_s* search_nodes, struct stack_s* search_dirs)
{
    struct binary_node_struct* replacement;
    struct binary_node_struct* orig_node_p = node_p;
    struct binary_node_struct** owner;
    bool dir;
    assert(node_p != NIL); 
    assert(stack_num_elmnts(search_nodes) == stack_num_elmnts(search_dirs));
    if(node_p->child[0] == NIL)
        replacement = node_p->child[1];
    else if(node_p->child[1] == NIL)
        replacement = node_p->child[0];
    else
    {
        // rewrite node's element with the smallest element in the right subtree
        dir = 1;
        STACK_PUSH(search_nodes, node_p);
        STACK_PUSH(search_dirs, dir);
        node_p = node_p->child[1];
        dir = 0;
        while(node_p->child[0] != NIL)
        {
            STACK_PUSH(search_nodes, node_p);
            STACK_PUSH(search_dirs, dir);
            node_p = node_p->child[0];
        }
        memcpy(binary_node_get_element(orig_node_p), binary_node_get_element(node_p), sorted_set_p->num_bytes_per_elmnt);
        replacement = node_p->child[1];
    }
    owner = get_owner(sorted_set_p, search_nodes, search_dirs);
    assert(*owner == node_p);
    *owner = replacement;
    free(node_p);
}

static bool sorted_set_search(struct sorted_set_s* sorted_set_p, byte* element, bool construct_search_path, struct stack_s* search_nodes, struct stack_s* search_dirs, byte* element_copy_p, u32* rank_copy_p)
{
    struct binary_node_struct* current = sorted_set_p->root_p;
    bool dir;
    bool calc_rank = (rank_copy_p != GORNISHT);
    bool copy_if_found = (element_copy_p != GORNISHT);
    if(calc_rank)
        *rank_copy_p = 1;
    if(construct_search_path)
        assert(stack_is_empty(search_nodes) && stack_is_empty(search_dirs));
    else 
        assert(search_nodes == GORNISHT && search_dirs == GORNISHT);

    while(current != NIL)
    {
        if(construct_search_path)
            STACK_PUSH(search_nodes, current);
        if(sorted_set_p->is_match(element, binary_node_get_element(current)))
        {
            if(copy_if_found == true)
                memcpy(element_copy_p, binary_node_get_element(current), sorted_set_p->num_bytes_per_elmnt);
            if(calc_rank)
                *rank_copy_p += current->child[0]->size;
            return true;
        }
        dir = sorted_set_p->is_less(element, binary_node_get_element(current)) ? 0 : 1;
        if(calc_rank && dir == 1)
            *rank_copy_p += (1 + current->child[0]->size);
        if(construct_search_path)
            STACK_PUSH(search_dirs, dir);
        current = current->child[dir];
    }
    return false;
}

bool sorted_set_contains(struct sorted_set_s* sorted_set_p, byte* element, byte* element_found, u32* rank_copy_p)
{
    return sorted_set_search(sorted_set_p, element, false, GORNISHT, GORNISHT, element_found, rank_copy_p);
}

static bool sorted_set_insert_or_remove(struct sorted_set_s* sorted_set_p, byte* element, bool op_insert, u32* rank_copy_p)
{
    struct binary_node_struct* node_p;
    struct binary_node_struct** owner;
    struct stack_s* search_nodes = stack_create(sizeof(struct binary_node_struct*));
    struct stack_s* search_dirs = stack_create(sizeof(bool));
    bool already_inside = sorted_set_search(sorted_set_p, element, true, search_nodes, search_dirs, GORNISHT, rank_copy_p);

    if(already_inside == true)
    {
        assert(stack_num_elmnts(search_nodes) == 1 + stack_num_elmnts(search_dirs));
        STACK_POP(search_nodes, node_p);
        assert(sorted_set_p->is_match(element, binary_node_get_element(node_p)));
        
        if(op_insert == true)
        {
            memcpy(binary_node_get_element(node_p), element, sorted_set_p->num_bytes_per_elmnt);
        }
        else
        {
            assert(op_insert == false);
            remove_helper(sorted_set_p, node_p, search_nodes, search_dirs);
            search_path_rebalance(sorted_set_p, search_nodes, search_dirs);
        }
    }
    else if(op_insert == true)
    {
        owner = get_owner(sorted_set_p, search_nodes, search_dirs);
        assert(*owner == NIL);
        *owner = leaf_create(element, sorted_set_p->num_bytes_per_elmnt);
        search_path_rebalance(sorted_set_p, search_nodes, search_dirs);
    }
    stack_destroy(search_nodes);
    stack_destroy(search_dirs);
    return already_inside;
}

bool sorted_set_insert(struct sorted_set_s* sorted_set_p, byte* element, u32* rank_copy_p)
{
    return sorted_set_insert_or_remove(sorted_set_p, element, true, rank_copy_p);
}

bool sorted_set_remove(struct sorted_set_s* sorted_set_p, byte* element, u32* rank_copy_p)
{
    return sorted_set_insert_or_remove(sorted_set_p, element, false, rank_copy_p);
}

static void sorted_set_search_by_rank(struct sorted_set_s* sorted_set_p, u32 rank, bool construct_search_path, struct stack_s* search_nodes, struct stack_s* search_dirs, byte* element_copy_p)
{
    struct binary_node_struct* current = sorted_set_p->root_p;
    bool dir;

    bool copy_when_found = (element_copy_p != GORNISHT);
    if(construct_search_path)
        assert(stack_is_empty(search_nodes) && stack_is_empty(search_dirs));
    else
        assert(search_nodes == GORNISHT && search_dirs == GORNISHT);

    while(true)
    {
        assert(1 <= rank);
        assert(rank <= current->size);
        if(construct_search_path)
            STACK_PUSH(search_nodes, current);
        if(rank == 1 + current->child[0]->size)
        {
            if(copy_when_found == true)
                memcpy(element_copy_p, binary_node_get_element(current), sorted_set_p->num_bytes_per_elmnt);
            return;
        }
        if(rank < 1 + current->child[0]->size)
            dir = 0;
        else
        {
            dir = 1; 
            rank -= (1 + current->child[0]->size);
        }
        if(construct_search_path)
            STACK_PUSH(search_dirs, dir);
        current = current->child[dir];
    }
}

void sorted_set_get_element_by_rank(struct sorted_set_s* sorted_set_p, u32 rank, byte* element_copy_p)
{
    sorted_set_search_by_rank(sorted_set_p, rank, false, GORNISHT, GORNISHT, element_copy_p);
}

void sorted_set_remove_by_rank(struct sorted_set_s* sorted_set_p, u32 rank, byte* element_copy_p)
{
    struct binary_node_struct* node_p;
    struct stack_s* search_nodes = stack_create(sizeof(struct binary_node_struct*));
    struct stack_s* search_dirs = stack_create(sizeof(bool));
    sorted_set_search_by_rank(sorted_set_p, rank, true, search_nodes, search_dirs, element_copy_p);
    assert(stack_num_elmnts(search_nodes) == 1 + stack_num_elmnts(search_dirs));
    STACK_POP(search_nodes, node_p);

    remove_helper(sorted_set_p, node_p, search_nodes, search_dirs);
    search_path_rebalance(sorted_set_p, search_nodes, search_dirs);
    stack_destroy(search_nodes);
    stack_destroy(search_dirs);
}

void walk_in_order(struct sorted_set_s* sorted_set_p, void (*fnc)(byte*))
{
    walk(sorted_set_p->root_p, 0, fnc);
}

void walk_in_reverse(struct sorted_set_s* sorted_set_p, void (*fnc)(byte*))
{
    walk(sorted_set_p->root_p, 1, fnc);
}

// ******* 3) test *******

static bool is_match_u64(byte* a, byte* b)
{
    return *((u64*) a) == *((u64*) b);
}

static bool is_less_u64(byte* a, byte* b)
{
    return *((u64*) a) < *((u64*) b);
}

static void pre_order_u64_helper(struct binary_node_struct* node_p, struct stack_s* elm_stack_p, struct stack_s* tree_size_stack_p)
{
    if(node_p == NIL)
        return;
    stack_push(elm_stack_p, binary_node_get_element(node_p));
    STACK_PUSH(tree_size_stack_p, node_p->size);
    //printf("elm = %lu  tree size = %u\n", *((u64*) binary_node_get_element(node_p)), node_p->size);
    pre_order_u64_helper(node_p->child[0], elm_stack_p, tree_size_stack_p);
    pre_order_u64_helper(node_p->child[1], elm_stack_p, tree_size_stack_p);
}

static void pre_order_u64(struct sorted_set_s* sorted_set_p, struct stack_s* elm_stack_p, struct stack_s* tree_size_stack_p)
{
    pre_order_u64_helper(sorted_set_p->root_p, elm_stack_p, tree_size_stack_p);
}

void sorted_set_test()
{
    struct sorted_set_s* sorted_set_p = sorted_set_create(sizeof(u64), is_match_u64, is_less_u64);
    struct stack_s* elm_stack_p = stack_create(sizeof(u64));
    struct stack_s* tree_size_stack_p = stack_create(sizeof(u32));
    u64 j;
    u64 arr[1000] = {304, 188, 364, 259, 43, 96, 317, 176, 256, 276, 221, 232, 119, 183, 228, 474, 391, 304, 176, 201, 285, 381, 83, 472, 470, 41, 37, 192, 361, 51, 425, 244, 154, 23, 370, 279, 390, 228, 40, 248, 198, 365, 162, 301, 340, 1, 54, 484, 276, 86, 175, 508, 20, 499, 306, 124, 162, 430, 488, 502, 370, 23, 141, 441, 83, 161, 333, 424, 370, 174, 509, 345, 394, 160, 295, 477, 358, 33, 476, 492, 146, 169, 273, 196, 364, 123, 309, 401, 500, 98, 95, 205, 432, 72, 27, 141, 408, 50, 58, 194, 249, 333, 308, 186, 413, 447, 8, 6, 168, 48, 382, 485, 320, 170, 55, 118, 447, 370, 264, 112, 126, 119, 385, 40, 460, 275, 136, 394, 215, 214, 171, 95, 46, 321, 46, 218, 446, 208, 204, 506, 231, 265, 234, 423, 204, 130, 47, 299, 333, 43, 1, 112, 182, 239, 303, 123, 153, 508, 509, 315, 299, 268, 280, 119, 87, 16, 320, 493, 240, 91, 104, 390, 252, 151, 23, 371, 350, 371, 176, 180, 308, 414, 299, 151, 23, 269, 373, 55, 488, 149, 141, 82, 277, 403, 396, 249, 411, 277, 411, 353, 367, 415, 360, 168, 155, 236, 429, 415, 487, 386, 171, 61, 154, 409, 23, 268, 166, 423, 178, 399, 74, 425, 277, 394, 66, 394, 366, 140, 82, 84, 276, 419, 442, 350, 361, 436, 364, 257, 107, 161, 65, 344, 234, 145, 172, 316, 48, 323, 119, 320, 95, 150, 277, 20, 179, 95, 52, 214, 211, 19, 71, 152, 378, 407, 384, 226, 396, 500, 7, 286, 441, 334, 427, 498, 458, 495, 306, 240, 203, 415, 209, 136, 168, 262, 100, 68, 406, 389, 183, 19, 369, 209, 414, 348, 325, 254, 264, 301, 507, 409, 201, 190, 196, 412, 32, 155, 97, 253, 281, 172, 395, 456, 406, 216, 197, 205, 492, 191, 151, 89, 89, 360, 71, 249, 24, 208, 182, 378, 142, 27, 3, 226, 304, 3, 379, 477, 283, 382, 432, 259, 292, 325, 154, 179, 497, 391, 322, 162, 115, 453, 500, 64, 460, 1, 501, 3, 267, 491, 249, 346, 261, 220, 150, 272, 77, 472, 43, 63, 307, 191, 370, 16, 323, 146, 113, 403, 200, 18, 225, 118, 110, 340, 156, 30, 242, 142, 446, 72, 248, 21, 511, 477, 439, 362, 477, 183, 144, 11, 138, 10, 245, 272, 20, 114, 459, 406, 189, 229, 371, 86, 367, 243, 285, 176, 184, 392, 294, 204, 104, 341, 210, 302, 14, 186, 107, 32, 36, 314, 444, 218, 40, 52, 429, 284, 380, 358, 43, 195, 305, 153, 258, 403, 492, 43, 120, 375, 320, 317, 156, 190, 282, 306, 158, 51, 468, 225, 97, 18, 44, 232, 239, 469, 73, 259, 473, 483, 224, 424, 313, 485, 257, 98, 181, 52, 209, 101, 1, 279, 215, 116, 139, 54, 173, 240, 259, 7, 93, 40, 462, 449, 499, 360, 197, 403, 85, 371, 3, 46, 194, 488, 34, 391, 480, 102, 124, 252, 278, 259, 329, 339, 501, 243, 67, 46, 186, 82, 64, 371, 21, 183, 398, 253, 25, 6, 285, 472, 196, 78, 175, 194, 465, 295, 37, 55, 371, 382, 241, 65, 132, 38, 484, 451, 208, 253, 483, 89, 88, 114, 270, 208, 461, 311, 251, 260, 297, 465, 267, 76, 486, 479, 107, 251, 153, 504, 362, 138, 178, 275, 233, 357, 359, 266, 458, 286, 294, 298, 317, 164, 185, 408, 20, 209, 137, 49, 282, 162, 503, 505, 53, 306, 322, 215, 228, 348, 139, 477, 39, 487, 285, 62, 318, 415, 149, 378, 30, 230, 58, 80, 309, 134, 339, 154, 311, 14, 93, 253, 90, 385, 272, 362, 207, 479, 132, 25, 31, 23, 373, 276, 183, 72, 177, 5, 395, 169, 294, 313, 19, 131, 192, 40, 53, 262, 407, 64, 412, 325, 146, 56, 130, 440, 276, 367, 192, 97, 415, 206, 166, 138, 192, 349, 180, 398, 178, 210, 365, 2, 218, 94, 30, 9, 27, 272, 90, 242, 474, 121, 7, 23, 179, 332, 207, 509, 299, 307, 456, 54, 142, 313, 221, 461, 477, 58, 235, 280, 225, 387, 337, 203, 327, 219, 374, 431, 131, 429, 37, 227, 207, 482, 384, 331, 62, 382, 338, 501, 129, 380, 62, 212, 391, 448, 407, 489, 228, 206, 170, 115, 281, 252, 446, 462, 341, 159, 103, 433, 116, 182, 276, 375, 142, 375, 449, 153, 241, 345, 115, 408, 353, 59, 237, 232, 241, 244, 428, 444, 285, 437, 212, 242, 146, 470, 413, 464, 346, 222, 92, 182, 37, 439, 446, 312, 21, 355, 122, 314, 420, 461, 301, 114, 244, 416, 166, 380, 17, 500, 17, 486, 83, 494, 234, 355, 69, 344, 467, 487, 218, 66, 265, 509, 37, 341, 461, 323, 44, 315, 320, 401, 45, 302, 348, 220, 500, 382, 155, 420, 122, 184, 114, 48, 245, 290, 45, 209, 397, 196, 240, 18, 359, 139, 6, 58, 129, 107, 1, 80, 23, 473, 206, 56, 506, 476, 155, 224, 382, 125, 374, 154, 197, 283, 270, 506, 309, 411, 492, 357, 116, 509, 257, 225, 473, 440, 189, 471, 210, 153, 402, 99, 155, 111, 438, 111, 76, 241, 272, 242, 452, 301, 169, 345, 383, 116, 117, 417, 411, 139, 103, 207, 365, 119, 337, 287, 14, 217, 422, 10, 414, 20, 427, 43, 278, 326, 499, 201, 313, 166, 211, 65, 173, 381, 148, 316, 508, 320, 47, 264, 256, 202, 109, 447, 9, 43, 415, 417, 465, 206, 65, 124, 462, 176, 113, 414, 488, 252, 46, 468, 85, 391, 263, 213, 379, 206, 296, 122, 99, 228, 186, 193, 120, 130, 92, 3, 388, 401, 146, 24, 248, 63, 170, 316, 392, 338, 187, 385, 458, 500, 362, 60, 491, 232, 452, 163, 256, 484, 457, 327, 83, 425, 310, 24, 283, 423, 256, 295, 407, 263, 184, 422, 473, 189, 188, 260, 283, 247, 337, 390, 381, 252, 16, 495, 171, 482};
    for(u32 i = 0; i < 1000; i += 1)
    {
        j = arr[i];
        if(sorted_set_contains(sorted_set_p, (byte*) (&j), GORNISHT, GORNISHT))
            sorted_set_remove(sorted_set_p, (byte*) (&j), GORNISHT);
        else
            sorted_set_insert(sorted_set_p, (byte*) (&j), GORNISHT);
    }
    pre_order_u64(sorted_set_p, elm_stack_p, tree_size_stack_p);
    u64 expected_elm[250] = {511, 508, 509, 506, 507, 504, 502, 503, 505, 498, 499, 494, 497, 489, 484, 487, 493, 501, 471, 472, 465, 467, 462, 464, 469, 458, 453, 457, 447, 448, 451, 438, 437, 431, 433, 436, 442, 459, 429, 425, 419, 423, 428, 406, 408, 401, 402, 416, 430, 480, 397, 390, 391, 388, 387, 389, 385, 381, 380, 383, 386, 370, 375, 367, 369, 378, 365, 360, 364, 348, 345, 337, 334, 341, 349, 331, 332, 326, 323, 325, 329, 333, 366, 318, 316, 317, 310, 309, 304, 305, 312, 298, 296, 294, 295, 297, 303, 290, 285, 284, 287, 272, 269, 273, 264, 261, 258, 257, 259, 252, 247, 248, 237, 244, 254, 266, 292, 321, 399, 235, 233, 234, 230, 228, 227, 229, 231, 219, 216, 217, 213, 206, 209, 210, 215, 202, 200, 201, 197, 194, 193, 195, 198, 204, 222, 187, 184, 183, 185, 179, 176, 177, 178, 181, 171, 169, 170, 163, 164, 168, 174, 189, 159, 155, 153, 154, 158, 151, 146, 148, 144, 145, 152, 140, 137, 130, 134, 138, 141, 125, 122, 124, 119, 115, 117, 121, 109, 102, 97, 100, 101, 110, 126, 160, 91, 94, 88, 89, 82, 84, 77, 78, 87, 73, 69, 68, 72, 62, 64, 67, 60, 59, 54, 55, 50, 49, 52, 61, 74, 46, 41, 43, 39, 40, 37, 36, 38, 33, 30, 31, 34, 24, 20, 21, 18, 14, 16, 19, 8, 6, 7, 3, 1, 2, 5, 11, 27, 48, 96, 236};
    u32 expected_tree_size[250] = {1, 1, 3, 1, 5, 1, 1, 3, 9, 1, 2, 1, 4, 1, 1, 3, 8, 18, 1, 2, 1, 2, 1, 4, 7, 1, 1, 3, 1, 2, 6, 1, 2, 1, 2, 5, 12, 20, 1, 1, 1, 3, 5, 1, 2, 1, 4, 10, 31, 50, 1, 1, 3, 1, 2, 6, 1, 1, 2, 4, 11, 1, 2, 1, 4, 16, 1, 1, 3, 1, 2, 1, 2, 5, 9, 1, 2, 1, 1, 3, 6, 16, 33, 1, 1, 3, 1, 2, 1, 4, 8, 1, 1, 1, 3, 5, 14, 1, 1, 2, 4, 1, 2, 7, 1, 2, 1, 2, 5, 1, 1, 3, 1, 5, 11, 19, 34, 68, 119, 1, 1, 3, 1, 1, 2, 4, 8, 1, 1, 3, 1, 1, 2, 4, 8, 1, 1, 3, 1, 1, 2, 4, 8, 17, 26, 1, 1, 2, 4, 1, 1, 2, 4, 9, 1, 1, 3, 1, 2, 6, 16, 43, 1, 1, 1, 3, 5, 1, 1, 3, 1, 5, 11, 1, 1, 1, 3, 5, 17, 1, 1, 3, 1, 1, 3, 7, 1, 2, 1, 2, 5, 13, 31, 75, 1, 2, 1, 4, 1, 2, 1, 4, 9, 1, 1, 2, 4, 1, 2, 7, 1, 2, 1, 4, 1, 2, 7, 15, 25, 1, 1, 3, 1, 5, 1, 2, 8, 1, 1, 3, 12, 1, 1, 3, 1, 1, 3, 7, 1, 1, 3, 1, 1, 3, 7, 15, 28, 54, 130, 250};
    
    assert((250 == stack_num_elmnts(elm_stack_p)) && (250 == stack_num_elmnts(tree_size_stack_p)));
    u64 x;
    u32 y;
    for(u32 i = 0; i < 250; i += 1)
    {
        STACK_POP(elm_stack_p, x);
        STACK_POP(tree_size_stack_p, y);
        assert(x == expected_elm[i]);
        assert(y == expected_tree_size[i]);
        //printf("%ld %ld %u %u\n", x, expected_elm[i], y, expected_tree_size[i]);
    }
    stack_destroy(elm_stack_p);
    stack_destroy(tree_size_stack_p);
    sorted_set_destroy(sorted_set_p);
    printf("sorted_set_test passed\n");
}
