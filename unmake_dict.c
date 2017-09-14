#include "huffman.h"
#include <stdio.h>

typedef int action(char, bitstring, void *);

static int print_bits(FILE *out, bitstring bits)
{
    char str[65] = "0";

    if (bits.len >= sizeof str)
        return -1;

    for (int i = 0; i < bits.len; i++)
        str[bits.len - i - 1] = ((bits.bits >> i) & 1) + '0';

    return fprintf(out, "%d'b%s", bits.len, str);
}

static int print_node(char byte, bitstring bits, FILE *out)
{
    fprintf(out, "0x%hhX = ", byte);
    print_bits(out, bits);
    fputc('\n', out);

    return 0;
}

static int process(long offset, FILE *in, bitstring b, action act, void *data)
{
    unsigned char arr[2];
    fseek(in, offset, SEEK_SET);
    size_t result = fread(arr, sizeof arr, 1, in);
    // TODO check result
    if (arr[1] != 0) {
        // internal node
        bitstring l = { .bits = (b.bits << 1) | 0, .len = b.len + 1 },
                  r = { .bits = (b.bits << 1) | 1, .len = b.len + 1 };

        process(offset + arr[0], in, l, act, data);
        process(offset + arr[1], in, r, act, data);
    } else {
        // leaf node
        act(arr[0], b, data);
    }

    return 0;
}

int main()
{
    FILE *in  = stdin;
    FILE *out = stdout;

    bitstring bits = { .len = 0 };

    process(ftell(in), in, bits, (action*)print_node, out);
}

