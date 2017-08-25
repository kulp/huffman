#include "huffman.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct walk_state {
    FILE *stream;
    long stream_pos;
    struct walk_state *parent;
};

static int walker(valtype val, bitstring key, double weight, int flags, void *userdata)
{
    struct walk_state **selfp = userdata;
    struct walk_state *self = *selfp;
    (void)key;
    (void)weight;

    if (flags & HUFF_PRE_ORDER) {
        // add new blank internal node to end of stream
        {
            char arr[2] = { 0, 0 };
            fseek(self->stream, 0, SEEK_END);
            self->stream_pos = ftell(self->stream);
            fwrite(arr, sizeof arr, 1, self->stream);
        }

        // push new state onto stack, copying old state
        {
            struct walk_state *st = malloc(sizeof *st);
            memcpy(st, self, sizeof *st);
            st->parent = self;
            *selfp = st;
            // self is not updated
        }
    } else if (flags & HUFF_IN_ORDER) {
        // update state to reflect that the next node is the right node
        self->stream_pos++;
    } else if (flags & HUFF_LEAF) {
        char arr[2] = { val, 0 };
        fseek(self->stream, 0, SEEK_END);
        fwrite(arr, sizeof arr, 1, self->stream); // emit valtype as a character
    }

    if (flags & HUFF_POST_ORDER || flags & HUFF_LEAF) {
        // update parent nodes
        long offset = self->stream_pos - self->parent->stream_pos;
        unsigned char small = offset;
        assert(("no loss of precision", small == offset));
        fseek(self->stream, self->parent->stream_pos, SEEK_SET);
        fwrite(&small, 1, 1, self->stream);
    }

    if (flags & HUFF_POST_ORDER) {
        // pop state from stack
        *selfp = self->parent;
        free(self);
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
        .stream_pos = 0,
        .parent     = &(struct walk_state){
            // parent node is not emitted
            .stream_pos = 0,
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
