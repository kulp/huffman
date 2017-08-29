#include "huffman.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct walk_state {
    FILE *stream;
    long stream_pos;
    int rightness;
    struct walk_state *parent;
};

static struct walk_state *create_child(struct walk_state *parent)
{
    struct walk_state *st = calloc(1, sizeof *st);
    st->stream = parent->stream;
    st->stream_pos = -1;
    st->rightness = 0;
    st->parent = parent;

    return st;
}

static size_t write_node(struct walk_state *self, char arr[2])
{
    fseek(self->stream, 0, SEEK_END);
    self->stream_pos = ftell(self->stream);
    return fwrite(arr, 2, 1, self->stream);
}

// Every time `walker` is entered, `userdata` will contain a double pointer to
// a `struct walk_state` that describes the state for the node being walked.
// `walker` is responsible for pushing and popping states in the linked-list
// stack in order to keep this true for the next node, and `walker` can do
// this because it knows whether or not it is visiting an internal node based
// on the value in `flags` (HUFF_{PRE,IN,POST}_ORDER or HUFF_LEAF).
static int walker(valtype val, bitstring key, double weight, int flags, void *userdata)
{
    struct walk_state **selfp = userdata;
    struct walk_state *self = *selfp;
    (void)key;
    (void)weight;

    if (flags & HUFF_PRE_ORDER) {
        // add new blank internal node to end of stream
        char arr[2] = { 0, 0 };
        write_node(self, arr);

        // push new state for left child
        self = *selfp = create_child(self);
    } else if (flags & HUFF_IN_ORDER) {
        // pop state for left child
        *selfp = self->parent;
        free(self);
        self = *selfp;

        // push new state for right child
        self = *selfp = create_child(self);

        // update state to reflect that the next node is the right node
        self->rightness = 1;
    } else {
        // we just finished either an internal node (HUFF_POST_ORDER) or a
        // leaf node (HUFF_LEAF), so we can fix up the pointer in our parent
        if (flags & HUFF_LEAF) {
            char arr[2] = { val, 0 };
            write_node(self, arr);
        } else if (flags & HUFF_POST_ORDER) {
            // pop state from right child
            *selfp = self->parent;
            free(self);
            self = *selfp;
        }

        // update parent node
        long offset = self->stream_pos - self->parent->stream_pos;
        unsigned char small = offset;
        assert(("no loss of precision", small == offset));
        if (self->parent->stream_pos >= 0) { // check for sentinel
            fseek(self->stream, self->parent->stream_pos + self->rightness, SEEK_SET);
            fwrite(&small, 1, 1, self->stream);
        }
    }

    return 0;
}

int main()
{
    struct huff_state *st;
    huff_init(&st);

    while (!feof(stdin)) {
        char buf[BUFSIZ];
        if (fgets(buf, sizeof buf, stdin)) {
            valtype v;
            double p;
            if (sscanf(buf, "%x = %lf", &v, &p) == 2) {
                huff_add(st, v, p);
            } else {
                fprintf(stderr, "Bad input `%s'\n", buf);
                break;
            }
        } else break;
    }

    huff_build(st);

    struct walk_state state = {
        .stream     = stdout,
        .stream_pos = ftell(state.stream),
        .rightness  = 0,
        .parent     = &(struct walk_state){
            // parent node is not emitted
            // TODO why do we need this node at all ?
            .stream     = state.stream,
            .stream_pos = -1, // intentionally invalid, sentinel value
            .rightness  = 0,
        },
    };
    struct walk_state *sp = &state;
    struct walk_state **spp = &sp;

    // Test seeking so we know if we cannot succeed
    if (fseek(sp->stream, 0, SEEK_SET) == -1) {
        perror("Error while seeking");
        return EXIT_FAILURE;
    }

    if (huff_walk(st, walker, spp))
        fprintf(stderr, "error while walking : did you huff_build() ?\n");

    huff_destroy(st);

    return 0;
}
