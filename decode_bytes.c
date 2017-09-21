#include "huffman.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct dict_node {
    struct dict_node *parent;
    struct dict_node *children[2];
    char val;
};

struct dict_state {
    struct dict_node **curr;
    struct dict_node *prev;
};

static int build_dict(char byte, bitstring bits, enum huff_walker_order order, void *data)
{
    struct dict_state *state = data;
    struct dict_node *self;
    (void)bits;

    if (order == HUFF_PRE_ORDER || order == HUFF_LEAF) {
        // First visit to a node, so allocate
        *state->curr = calloc(1, sizeof **state->curr);
    }

    self = *state->curr;
    self->parent = state->prev;
    state->prev = self;

    switch (order) {
        case HUFF_PRE_ORDER:    state->curr = &self->children[0];   break;
        case HUFF_IN_ORDER:     state->curr = &self->children[1];   break;
        case HUFF_LEAF:         self->val = byte;                   break;
        default:                /* no operation */                  break;
    }

    if (order == HUFF_POST_ORDER || order == HUFF_LEAF) {
        // Last visit to a node, so pop
        if (self->parent) {
            // root does not have a parent, so this would crash
            // TODO somehow obviate the check for NULL
            state->prev = self->parent->parent;
        }
        state->curr = &self->parent; // pop self
    }

    return 0;
}

static void free_dict_node(struct dict_node *node, int recurse)
{
    if (!node) {
        return;
    }

    if (recurse) {
        free_dict_node(node->children[0], recurse);
        free_dict_node(node->children[1], recurse);
    }

    free(node);
}

static int decode_stream(struct dict_node *dict, FILE *in, FILE *out)
{
    struct dict_node *node = dict;
    bitstring next;
    unsigned long long chars = 0; // number of characters output
    unsigned long long max = 0;
    if (huff_read_length(in, &max)) {
        perror("Failed to read length");
        return 1;
    }

    // read only one char at a time to avoid endian issues
    while (chars < max && !feof(in) && fread(&next.bits, 1, 1, in) == 1) {
        // Pretend we always get a full char's worth of bits. We will stop
        // before end-of-stream because we count characters emitted.
        next.len = CHAR_BIT;
        // consume byte until it is gone
        while (next.len > 0) {
            if (node->children[0] == NULL) { // leaf node
                fputc(node->val, out); // emit code byte
                node = dict; // reset to root for next bits
                chars++;
            } else {
                node = node->children[next.bits & 1];
                next.bits >>= 1;
                next.len--; // consumed a bit
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Specify dictionary and input file as arguments\n");
        return EXIT_FAILURE;
    }

    FILE *out  = stdout;
    FILE *dict = fopen(argv[1], "rb");
    FILE *data = fopen(argv[2], "rb");
    if (!dict || !data) {
        fprintf(stderr, "Unable to open dictionary or input data\n");
        return EXIT_FAILURE;
    }

    struct dict_node *root = NULL;
    struct dict_state state = {
        .curr = &root,
        .prev = NULL,
    };

    huff_load_dict(dict, build_dict, &state);
    fclose(dict);

    int rc = decode_stream(root, data, out);

    free_dict_node(root, 1);
    fclose(data);

    return rc;
}

