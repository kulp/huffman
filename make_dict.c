#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

struct walk_state {
    FILE *stream;
    long stream_pos;
    unsigned long long cache;
    unsigned char len;
    struct walk_state *parent;
};

static int walker(valtype val, bitstring key, double weight, int flags, void *userdata)
{
    struct walk_state **wp = userdata;
    struct walk_state *w = *wp;
    (void)val;
    (void)weight;

    if (w->len + key.len >= CHAR_BIT * sizeof w->cache)
        return 1; // too big

    if (flags & HUFF_PRE_ORDER) {
        // push new state onto stack
        struct walk_state *st = malloc(sizeof *st);
        memcpy(st, w, sizeof *st);
        st->parent = w;
        *wp = st;

        fseek(st->stream, w->stream_pos, SEEK_SET);
        st->stream_pos = ftell(st->stream);
        // TODO emit internal node
    } else if (flags & HUFF_POST_ORDER) {
        // pop state from stack
        struct walk_state *st = w->parent;
        free(w);
        *wp = st;
    } else if (flags & HUFF_LEAF) {
        char arr[2] = { val, 0 };
        fwrite(arr, sizeof arr, 1, w->stream); // emit valtype as a character
    }

    // TODO update parent nodes

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
        .cache      = 0ll,
        .len        = 0l,
        .parent     = NULL
    };
    struct walk_state *sp = &state;
    struct walk_state **spp = &sp;

    if (huff_walk(st, walker, spp))
        fprintf(stderr, "error while walking : did you huff_build() ?\n");

    if (sp->len > 0) {
        fprintf(sp->stream, "%c", (char)(sp->cache & 0xff));
    }

    huff_destroy(st);

    return 0;
}
