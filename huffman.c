#include "huffman.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct huff_node {
    struct huff_value_node v;
    double weight;
    struct huff_node *left, *right; // full binary tree
    struct huff_node *next; // singly linked list
};

struct huff_state {
    struct huff_node *head;
    unsigned built:1;
};

static struct huff_node *new_node()
{
    return calloc(1, sizeof *new_node());
}

int huff_init(struct huff_state **_s)
{
    struct huff_state *s = *_s = malloc(sizeof *s);
    s->built = 0;
    s->head = NULL;

    return 0;
}

static void insert_node(struct huff_node **start, struct huff_node *node)
{
    while (*start != NULL && node->weight >= (*start)->weight) {
        start = &(*start)->next;
    }

    node->next = *start;
    *start = node;
}

int huff_add(struct huff_state *s, valtype val, double weight)
{
    if (s->built)
        return -1;

    struct huff_node *q = new_node();
    q->left       = NULL;
    q->right      = NULL;
    q->v.val      = val;
    q->weight     = weight;
    q->v.internal = 0;

    insert_node(&s->head, q);

    return 0;
}

int huff_build(struct huff_state *s)
{
    while (s->head->next != NULL) { // until we have one node
        struct huff_node *a = s->head,
                         *b = a->next;

        // drop two nodes from front of list
        s->head = b->next;

        struct huff_node *c = new_node();
        c->left       = a;
        c->right      = b;
        c->weight     = a->weight + b->weight;
        c->v.internal = 1;

        insert_node(&s->head, c);
    }

    s->built = 1;

    return 0;
}

static int walk(struct huff_node *n, bitstring b, huff_walker *w, void *userdata)
{
    if (!n)
        return 0;

    bitstring l = { .bits = b.bits | (0ull << b.len), .len = b.len + 1 },
              r = { .bits = b.bits | (1ull << b.len), .len = b.len + 1 };

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

static void destroy_node(struct huff_node *node, int recurse)
{
    if (!node) {
        return;
    }

    if (recurse) {
        destroy_node(node->left, recurse);
        destroy_node(node->right, recurse);
    }

    free(node);
}

void huff_destroy(struct huff_state *s)
{
    for (struct huff_node *next, *node = s->head; node != NULL; node = next) {
        next = node->next;
        destroy_node(node, 1);
    }

    free(s);
}

static int process_chunk_(long offset, FILE *in, bitstring b, huff_dict_cb act, void *data)
{
    unsigned char arr[2];
    fseek(in, offset, SEEK_SET);
    size_t result = fread(arr, sizeof arr, 1, in);
    // TODO check result
    if (arr[1] != 0) {
        // internal node
        bitstring l = { .bits = b.bits | (0ull << b.len), .len = b.len + 1 },
                  r = { .bits = b.bits | (1ull << b.len), .len = b.len + 1 };

        act(arr[0], b, HUFF_PRE_ORDER, data);
        process_chunk_(offset + arr[0], in, l, act, data);
        act(arr[0], b, HUFF_IN_ORDER, data);
        process_chunk_(offset + arr[1], in, r, act, data);
        act(arr[0], b, HUFF_POST_ORDER, data);
    } else {
        // leaf node
        act(arr[0], b, HUFF_LEAF, data);
    }

    return 0;
}

int huff_load_dict(FILE *in, huff_dict_cb act, void *data)
{
    bitstring bits = { .len = 0 };

    return process_chunk_(ftell(in), in, bits, act, data);
}

const unsigned long long TOP_BIT = 1ull << (CHAR_BIT - 1);

// Length is stored as a variable-length integer. The most significant bit of
// each byte is 1 if there are more bytes to come. Each successive byte
// contains (CHAR_BIT - 1) bits of the original size, starting with the least
// significant.
int huff_emit_length(FILE *stream, unsigned long long size)
{
    const int byte = size & (TOP_BIT - 1);
    const int more = size != (unsigned long long)byte;
    if (more) {
        int rc = fputc(byte | TOP_BIT, stream);
        if (rc != EOF) {
            huff_emit_length(stream, size >> (CHAR_BIT - 1));
        }
        return rc == EOF;
    } else {
        return fputc(byte, stream) == EOF;
    }
}

int huff_read_length(FILE *stream, unsigned long long *size)
{
    unsigned index = 0;
    unsigned char byte;
    const unsigned char mask = TOP_BIT - 1;
    while (!feof(stream) && fread(&byte, 1, 1, stream) == 1) {
        *size |= (byte & mask) << index;
        index += CHAR_BIT - 1;
        if (!(byte & TOP_BIT)) {
            break;
        }
    }

    return 0;
}

