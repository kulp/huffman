#include "huffman.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define CHAR_MASK ((1ul << CHAR_BIT) - 1)

static int collect_nodes(char byte, bitstring bits, enum huff_walker_order order, void *data)
{
    if (order != HUFF_LEAF) {
        return 0;
    }

    bitstring *table = data;
    table[(unsigned char)byte] = bits;
    return 0;
}

struct emit_state {
    unsigned char bits;
    unsigned char count;
};

// Bits are emitted LSB to MSB -- i.e. earlier bits are lower in a byte
static int emit_bitstring(struct emit_state *state, bitstring b, FILE *out)
{
    if (b.len == 0) {
        // We got an invalid bitstring -- we don't know how to encode
        return 1;
    }

    if (state->count > 0) {
        // TODO this code is convenient but assumes bitstring does not overflow
        b.len += state->count;
        b.bits = (b.bits << state->count) | state->bits;
    }

    while (b.len >= CHAR_BIT) {
        fputc(b.bits & CHAR_MASK, out);
        b.len   -= CHAR_BIT;
        b.bits >>= CHAR_BIT;
    }

    state->count = b.len;
    state->bits  = b.bits;

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
        perror("Unable to open dictionary or input data");
        return EXIT_FAILURE;
    }

    bitstring table[1 << CHAR_BIT]; // look up bitstring using byte as index

    int rc = huff_load_dict(dict, collect_nodes, table);

    if (fseek(data, 0, SEEK_END)) {
        perror("Failed to seek input stream");
        return EXIT_FAILURE;
    }

    long size = ftell(data);
    fseek(data, 0, SEEK_SET);
    if (huff_emit_length(out, size)) {
        perror("Failed to emit length");
        return EXIT_FAILURE;
    }

    struct emit_state state = { .count = 0 };
    while (!feof(data) && !ferror(data)) {
        int ch = fgetc(data);
        if (ch != EOF) {
            if (emit_bitstring(&state, table[(unsigned char)ch], out)) {
                fprintf(stderr, "Read byte %#hhx not in dictionary\n", (unsigned char)ch);
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

