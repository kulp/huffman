#include "huffman.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct huff_node {
    struct huff_value_node v;
    double weight;
    struct huff_node *left, *right;
};

struct huff_state {
    // size is the allocated size of queue ; used is the number of slots used
    // in queue. When used grows to be equal to size, realloc().
    size_t size, used;
    // All nodes are stored consecutively in queue, but only the ones between
    // head and tail are considered "in the queue" ; that is, when head ==
    // tail, the queue is empty, and when (tail - head == 1), there is one
    // node in the queue (end condition). The queue is realloc'ed as needed
    // when new nodes are added, whether leaf nodes or internal.
    struct huff_node *queue, *head, *tail;
    unsigned built:1;
};

int huff_init(struct huff_state **_s)
{
    struct huff_state *s = *_s = malloc(sizeof *s);
    s->built = 0;
    s->size = 64;
    s->used = 0;
    s->queue = calloc(s->size, sizeof *s->queue);
    s->head = s->tail = s->queue;

    return 0;
}

static int dbl_cmp(const void *_a, const void *_b)
{
    const struct huff_node *a = _a,
                           *b = _b;
    return a->weight - b->weight;
}

static void ensure_order(struct huff_state *s)
{
    // It would be better to insert in place instead of resorting an
    // almost-sorted list every time, especially since this is a worst-case
    // scenario for quicksort.
    qsort(s->head, s->tail - s->head, sizeof *s->queue, dbl_cmp);
}

// ensures there is space for at least one more node
static int ensure_space(struct huff_state *s)
{
    if (s->used < s->size)
        return 0;

    ptrdiff_t h = s->head - s->queue,
              t = s->tail - s->queue;

    // Assume two's-complement integers and round up to a nice round figure
    s->size = -(-s->used & ~0xf) * 2;
    struct huff_node *re = calloc(s->size, sizeof *s->queue);
    memcpy(re, s->queue, s->used * sizeof *s->queue);

    // Update pointers inside realloced memory. They all pointed inside of
    // s->queue's allocated memory, so we can use their old offsets to
    // point into their new location.
    for (struct huff_node *n = re; n != &re[t]; n++) {
        n->left  = n->left  ? &re[n->left  - s->queue] : NULL;
        n->right = n->right ? &re[n->right - s->queue] : NULL;
    }

    free(s->queue);
    s->queue = re;
    s->head = &s->queue[h];
    s->tail = &s->queue[t];

    return !s->queue;
}

int huff_add(struct huff_state *s, valtype val, double weight)
{
    if (s->built || ensure_space(s))
        return -1;

    s->used++;
    struct huff_node *q = s->tail++;
    q->left       = NULL;
    q->right      = NULL;
    q->v.val      = val;
    q->weight     = weight;
    q->v.internal = 0;

    ensure_order(s);

    return 0;
}

int huff_build(struct huff_state *s)
{
    while ((ensure_order(s), s->tail - s->head > 1)) {
        if (ensure_space(s))
            return -1;

        struct huff_node *a = s->head++,
                         *b = s->head++;

        // TODO coalesce nodes to the beginning of queue (reuse space)
        s->used++;
        struct huff_node *c = s->tail++;
        c->left       = a;
        c->right      = b;
        c->weight     = a->weight + b->weight;
        c->v.internal = 1;
    }

    s->built = 1;

    return 0;
}

static int walk(struct huff_node *n, bitstring b, huff_walker *w, void *userdata)
{
    if (!n)
        return 0;

    bitstring l = { .bits = (b.bits << 1) | 0, .len = b.len + 1 },
              r = { .bits = (b.bits << 1) | 1, .len = b.len + 1 };

    // Slight optimization -- huffman trees are full, so each node will have
    // either 0 or 2 children. Therefore we need to check only one child to
    // determine if we are a leaf node.
    int is_leaf = !n->left;

    return 0
        || ( is_leaf ? 0 : w(n->v.val, b, n->weight, HUFF_PRE_ORDER , userdata))
        || walk(n->left, l, w, userdata)

        || (!is_leaf ? 0 : w(n->v.val, b, n->weight, HUFF_LEAF      , userdata))

        || ( is_leaf ? 0 : w(n->v.val, b, n->weight, HUFF_IN_ORDER  , userdata))
        || walk(n->right, r, w, userdata)

        || ( is_leaf ? 0 : w(n->v.val, b, n->weight, HUFF_POST_ORDER, userdata));
}

int huff_walk(struct huff_state *s, huff_walker *w, void *userdata)
{
    if (!s->built)
        return -1;

    bitstring b = { .bits = 0, .len = 0 };
    return walk(s->head, b, w, userdata);
}

void huff_destroy(struct huff_state *s)
{
    free(s->queue);
    free(s);
}
