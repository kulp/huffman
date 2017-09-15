#include "huffman.h"

#include <stdio.h>
#include <stdlib.h>

struct dict_node {
    struct dict_node *children[2];
    struct dict_node *parent;
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

    switch (order) {
        case HUFF_PRE_ORDER:
            // We just entered an internal node for the first time.
            // Allocate space for a new node.
            self = *state->curr = malloc(sizeof **state->curr);
            // Point parent to the most recently-visited node.
            self->parent = state->prev;
            // self is now the most recently-visited node.
            state->prev = self;
            // The next node to visit will be the left child.
            state->curr = &self->children[0];
            break;
        case HUFF_LEAF:
            // We just entered a leaf node for the only time.
            // Allocate space for a new node.
            self = *state->curr = malloc(sizeof **state->curr);
            // Point parent to the most recently-visited node.
            self->parent = state->prev;
            // Update value slot.
            self->val = byte;
            break;
        case HUFF_IN_ORDER:
            // We just entered an internal node for the second time.
            // Get just-exited child.
            self = *state->curr;
            // Get parent of just-exited child (our old self).
            self = *state->curr = self->parent;
            // self is now the most recently-visited node.
            state->prev = self;
            // The next node to visit will be the right child.
            state->curr = &self->children[1];
            break;
        case HUFF_POST_ORDER:
            // We just entered an internal node for the third and last time.
            // Get just-exited child.
            self = *state->curr;
            // Get parent of just-exited child (our old self).
            self = *state->curr = self->parent;
            // Parent is now the most recently-visited node.
            state->prev = self->parent;
            break;
        default:
            // TODO handle error condition
            break;
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

