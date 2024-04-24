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
    walk(node_p->child[1-dir], dir, fnc);
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
    struct binary_node_struct* x = y->child[1-dir];
    u32 temp = y->size;
    assert(y != NIL); assert(x != NIL);
    y->child[1-dir] = x->child[dir];
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
    struct binary_node_struct* y = z->child[1-dir];
    struct binary_node_struct* x = y->child[dir];
    u32 temp = z->size;
    assert(z != NIL); assert(y != NIL); assert(x != NIL);
    y->child[dir] = x->child[1-dir];
    x->child[1-dir] = y;
    z->child[1-dir] = x->child[dir]; 
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
    return n1 * (node_p->child[1-dir]->size + 1) < n2 * (node_p->child[dir]->size + 1);
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
                *owner = rotation(node_p, 1-dir);
            else
                *owner = double_rotation(node_p, 1-dir);
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

bool is_match_u64(byte* a, byte* b)
{
    return *((u64*) a) == *((u64*) b);
}

bool is_less_u64(byte* a, byte* b)
{
    return *((u64*) a) < *((u64*) b);
}

void print_u64(byte* a)
{
    printf("%ld\n", *((u64*) a));
}

void sorted_set_tmp_test()
{
    struct sorted_set_s* sorted_set_p = sorted_set_create(sizeof(u64), is_match_u64, is_less_u64);
    u64 arr[100] = {
         5,  8, 22, 46, 19, 93, 80,  6, 64, 97,
        71, 63, 85, 11, 44, 43, 41, 59, 68, 91, 
        87, 72, 15, 30, 77,100, 83, 61, 12, 98, 
        13, 73, 48, 16, 84, 28, 32, 82, 24, 50, 
         7, 53, 26, 78, 60, 92, 95, 18, 67, 21, 
        33, 89, 38, 27, 17, 52, 79, 86, 56, 76, 
        45, 54, 20, 58, 66, 37,  2, 70, 81,  9, 
         1, 94, 40, 55, 42, 36, 96, 14, 74, 51, 
         4, 57, 75, 39, 62, 90, 29, 69,  3, 49, 
        31, 47, 34, 65, 25, 10, 23, 99, 88, 35};
    for(u64 i = 0; i < 100; i += 1)
        sorted_set_insert(sorted_set_p, (byte*)(&arr[i]), GORNISHT);
    walk_in_order(sorted_set_p, print_u64);
    u64 x = -1;
    sorted_set_get_element_by_rank(sorted_set_p, 7, (byte*) &x);
    assert(7 == x);
    sorted_set_remove_by_rank(sorted_set_p, 52, (byte*) &x);
    assert(52 == x);
    printf("\n");
    walk_in_reverse(sorted_set_p, print_u64);
    for(u64 i = 0; i < 120; i += 1)
        assert(sorted_set_contains(sorted_set_p, (byte*) &i, GORNISHT, GORNISHT) == ((i <= 100) && (i != 0) && (i != 52)));
    sorted_set_destroy(sorted_set_p);
}
