// this is a modified version of Knuth's https://www-cs-faculty.stanford.edu/~knuth/programs/dance.w

#include "exact_cover.h"
#define MAX_ROWS_IN_COVER 1234

// ******* 1) data structures *******

// all lists are non-empty circular doubly linked lists

// each 1 entry in the exact cover matrix gets a node
// also each column holds a dummy node
struct dlx_node
{
    u32 row_id;
    struct dlx_col* col;
    
    // left and right pointers won't change as rows stay the same
    struct dlx_node* left;
    struct dlx_node* right;
    
    struct dlx_node* up;
    struct dlx_node* down;
    
    // for debugging
    u32 node_id;
};

// a column data structure, one for each column and an additional dummy column called root as the head of the list of uncovered columns
struct dlx_col
{
    // a dummy node is the head of the list of the nodes per column
    struct dlx_node head;
    
    // number of (non-header) nodes
    u32 len;
    
    // previous and next columns
    struct dlx_col* prev;
    struct dlx_col* next;

    // for debugging
    u32 col_id;
};

// ******* 2) algorithm *******


static void cover(struct dlx_col* c)
{
    struct dlx_col* l; struct dlx_col* r;
    struct dlx_node* rr; struct dlx_node* nn; struct dlx_node* uu; struct dlx_node* dd;
    
    assert(c->col_id != UINT32_MAX); // valid column

    // remove column c from list of uncovered columns
    l = c->prev;
    r = c->next;
    l->next = r;
    r->prev = l;

    for(rr = c->head.down; rr != &(c->head); rr = rr->down)
    {
        for(nn = rr->right; nn != rr; nn = nn->right)
        {
            // removing node nn from other columns, as c is already covered
            uu = nn->up;
            dd = nn->down;
            uu->down = dd;
            dd->up = uu;

            assert(nn->col->len != 0);
            nn->col->len -= 1;
        }
    }
}

static void uncover(struct dlx_col* c)
{
    struct dlx_col* l; struct dlx_col* r;
    struct dlx_node* rr; struct dlx_node* nn; struct dlx_node* uu; struct dlx_node* dd;

    assert(c->col_id != UINT32_MAX); // valid column

    for(rr = c->head.up; rr != &(c->head); rr = rr->up)
    {
        for(nn = rr->left; nn != rr; nn = nn->left)
        {
            // insert node nn, it can be used to cover again
            uu = nn->up;
            dd = nn->down;
            uu->down = nn;
            dd->up = nn;

            nn->col->len += 1;
        }
    }

    // insert column c into list of uncovered columns
    l = c->prev;
    r = c->next;
    l->next = c;
    r->prev = c;
}

static struct dlx_col* find_best_col(struct dlx_col* root)
{
    u32 minlen = UINT32_MAX;
    struct dlx_col* cur_col;
    struct dlx_col* ret = GORNISHT;
    
    assert(root->next != root); // at least one column needs covering

    for(cur_col = root->next; cur_col != root; cur_col = cur_col->next)
    {
        assert(cur_col->col_id != UINT32_MAX); // valid column
        if(cur_col->len < minlen)
        {
            ret = cur_col;
            minlen = cur_col->len;
        }
    }
    assert(ret != GORNISHT);
    return ret;
}

static u32 dlx(struct dlx_col* root, bool stop_at_first_solution, void exact_cover_callback(u32, u32*, byte*), byte* extra)
{
    u32 ret = 0, level = 0, ii; 
    struct dlx_col* best_col;
    struct dlx_node* cur_node;
    struct dlx_node* pp;
    
    struct dlx_node* history[MAX_ROWS_IN_COVER]; // this array is a stack with number of elements = level
    u32 history_row_ids[MAX_ROWS_IN_COVER];

forward:
    best_col = find_best_col(root);
    cover(best_col);
    cur_node = history[level] = best_col->head.down;
    history_row_ids[level] = history[level]->row_id;

advance:
    assert(cur_node->col == best_col);

    if(cur_node == &(best_col->head))
        goto backup;


    for(pp = cur_node->right; pp != cur_node; pp = pp->right)
        cover(pp->col);

    if(root->next == root)
    {
        ret += 1;
        if(exact_cover_callback != GORNISHT)
            exact_cover_callback(level + 1, history_row_ids, extra);
        else
        {
            printf("solution: ");
            for(ii = 0; ii <= level; ii += 1)
                printf("%d ", history_row_ids[ii]);
            printf("\n");
        }
        
        if(stop_at_first_solution) { goto done; }
        goto recover;
    }

    level += 1;

    assert(level < MAX_ROWS_IN_COVER);
    goto forward;

backup:
    uncover(best_col);
    if(level == 0)
        goto done;

    level -= 1;
    cur_node = history[level];
    best_col = cur_node->col;

recover: 
    for(pp = cur_node->left; pp != cur_node; pp = pp->left)
        uncover(pp->col);

    cur_node = history[level] = cur_node->down;
    history_row_ids[level] = history[level]->row_id;

    goto advance;

done:
    return ret;
}


// ******* 3) preparation *******

// changes a bool array to store count of ones, i.e. changes 0 0 1 0 1 1 to 0 0 1 0 2 3
static u32 make_cntr_mat(bool* mat, u32* cntr_mat, u32 nrows, u32 ncols)
{
    u32 cntr = 0;
    u32 idx;
    for(idx = 0; idx < nrows * ncols; idx += 1)
    {
        if(mat[idx])
        {
            cntr += 1;
            cntr_mat[idx] = cntr;
        }
        else { cntr_mat[idx] = 0; }
    }
    return cntr;
}

static void set_dummy_node(struct dlx_node* node)
{
    node->node_id = node->row_id = UINT32_MAX;
    node->left = node->right = node->up = node->down = GORNISHT;
    node->col = GORNISHT;
}

// we have 1 + cntr nodes in an array, zeroth node is junk
static void set_node_ids(struct dlx_node* node_arr, u32 cntr)
{
    u32 idx;
    for(idx = 1; idx <= cntr; idx += 1) { node_arr[idx].node_id = idx; }
    set_dummy_node(&(node_arr[0]));
}

static void link_rows(u32* cntr_mat, struct dlx_node* node_arr, u32 nrows, u32 ncols, u32* scratch)
{
    u32 num_nodes, node_id, i, j, k;

    for(i = 0; i < nrows; i += 1)
    {
        num_nodes = 0;
        for(j = 0; j < ncols; j += 1)
        {
            node_id = cntr_mat[i * ncols + j];
            if(0 < node_id)
            {
                node_arr[node_id].row_id = i;
                scratch[num_nodes] = node_id;
                num_nodes += 1;
            }
        }
    
        for(k = 0; k < num_nodes; k += 1)
        {
            node_arr[scratch[k]].right = &(node_arr[scratch[(k+1) % num_nodes]]);
            node_arr[scratch[(k+1) % num_nodes]].left = &(node_arr[scratch[k]]);
        }
    }
}

static void link_cols(u32* cntr_mat, struct dlx_node* node_arr, struct dlx_col* col_arr, struct dlx_col* root, u32 nrows, u32 ncols, u32* scratch)
{
    u32 num_nodes, node_id, i, j, k;
    for(j = 0; j < ncols; j += 1)
    {
        col_arr[j].col_id = j;
        set_dummy_node(&(col_arr[j].head));
        col_arr[j].head.col = &(col_arr[j]);

        num_nodes = 0;
        for(i = 0; i < nrows; i += 1)
        {
            node_id = cntr_mat[i * ncols + j];
            if(node_id != 0)
            {
                node_arr[node_id].col = &(col_arr[j]);
                scratch[num_nodes] = node_id;
                num_nodes += 1;
            }
        }
        col_arr[j].len = num_nodes;

        if(0 == num_nodes) { col_arr[j].head.down = col_arr[j].head.up = &(col_arr[j].head); }
        else
        {
            for(k = 0; k + 1 < num_nodes; k += 1)
            {
                node_arr[scratch[k]].down = &(node_arr[scratch[k+1]]);
                node_arr[scratch[k+1]].up = &(node_arr[scratch[k]]);
            }
            col_arr[j].head.down = &(node_arr[scratch[0]]);
            node_arr[scratch[0]].up = &(col_arr[j].head);
            col_arr[j].head.up = &(node_arr[scratch[num_nodes - 1]]);
            node_arr[scratch[num_nodes - 1]].down = &(col_arr[j].head);
        }
    }

    set_dummy_node(&(root->head)); // we're never supposed to use this node
    root->len = root->col_id = UINT32_MAX;
    assert(ncols > 0);
    root->next = &(col_arr[0]);
    col_arr[0].prev = root;
    root->prev = &(col_arr[ncols - 1]);
    col_arr[ncols - 1].next = root;

    for(k = 0; k + 1 < ncols; k += 1)
    {
        col_arr[k].next = &(col_arr[k + 1]);
        col_arr[k + 1].prev = &(col_arr[k]);
    }
}

// ******* 4) api *******

u32 find_exact_covers(bool* mat, u32 nrows, u32 ncols, bool stop_at_first_solution, void exact_cover_callback(u32, u32*, byte*), byte* extra)
{
    assert(nrows > 0); assert(ncols > 0);

    u32 ret;
    u32* cntr_mat = malloc(sizeof(u32) * nrows * ncols);
    u32 cntr = make_cntr_mat(mat, cntr_mat, nrows, ncols);
    
    struct dlx_node* node_arr = malloc(sizeof(struct dlx_node) * (cntr + 1)); // we're being wasetful to avoid off by one errors
    set_node_ids(node_arr, cntr);
    u32* scratch = malloc(sizeof(u32) * MAX(nrows, ncols));
    link_rows(cntr_mat, node_arr, nrows, ncols, scratch);

    struct dlx_col* col_arr = malloc(sizeof(struct dlx_col) * ncols);
    struct dlx_col root;
    link_cols(cntr_mat, node_arr, col_arr, &root, nrows, ncols, scratch);
    
    ret = dlx(&root, stop_at_first_solution, exact_cover_callback, extra);

    free(col_arr);
    free(scratch);
    free(node_arr);
    free(cntr_mat); 

    return ret;
}
