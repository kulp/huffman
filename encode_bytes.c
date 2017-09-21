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

// Bits are emitted LSB to MSB -- i.e. earlier bits are lower in a byte.
// Returns number of bits emitted.
static int emit_bitstring(struct emit_state *state, bitstring b, FILE *out)
{
    // Receiving a bitstring of 0 length is acceptable at any time, although
    // only useful at the end of encoding. It means "encode what you have
    // remaining in `state` even if it is not a full byte".

    int count = 0;
    int flush = b.len == 0;

    if (state->count > 0) {
        // TODO this code is convenient but assumes bitstring does not overflow
        b.len += state->count;
        b.bits = (b.bits << state->count) | state->bits;
    }

    while (b.len >= CHAR_BIT) {
        fputc(b.bits & CHAR_MASK, out);
        count   += CHAR_BIT;
        b.bits >>= CHAR_BIT;
        b.len   -= CHAR_BIT;
    }

    if (flush) { // if so, flush state
        fputc(b.bits & CHAR_MASK, out);
        count   += b.len;
        b.bits >>= b.len;
        b.len    = 0;
    }

    state->count = b.len;
    state->bits  = b.bits;

    return count;
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
            bitstring b = table[(unsigned char)ch];
            if (b.len == 0) {
                fprintf(stderr, "Read byte %#hhx not in dictionary\n", (unsigned char)ch);
                return EXIT_FAILURE;
            }
            emit_bitstring(&state, b, out);
        }
    }

    // flush leftover bits
    bitstring blank = { .len = 0 };
    emit_bitstring(&state, blank, out);

    return EXIT_SUCCESS;
}

