#include "huffman.h"

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
    // TODO
    return EXIT_SUCCESS;
}

