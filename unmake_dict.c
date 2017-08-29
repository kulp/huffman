#include "huffman.h"
#include <stdio.h>

static int print_bits(FILE *out, bitstring bits)
{
    char str[65] = "0";

    if (bits.len >= sizeof str)
        return -1;

    for (int i = 0; i < bits.len; i++)
        str[bits.len - i - 1] = ((bits.bits >> i) & 1) + '0';

    return fprintf(out, "%d'b%s", bits.len, str);
}

static int process(long offset, FILE *in, FILE *out, bitstring b)
{
    unsigned char arr[2];
    fseek(in, offset, SEEK_SET);
    size_t result = fread(arr, sizeof arr, 1, in);
    // TODO check result
    if (arr[1] != 0) {
        // internal node
        bitstring l = { .bits = (b.bits << 1) | 0, .len = b.len + 1 },
                  r = { .bits = (b.bits << 1) | 1, .len = b.len + 1 };

        process(offset + arr[0], in, out, l);
        process(offset + arr[1], in, out, r);
    } else {
        // leaf node
        fprintf(out, "%#hhx = ", arr[0]);
        print_bits(out, b);
        fputc('\n', out);
    }

    return 0;
}

int main()
{
    FILE *in  = stdin;
    FILE *out = stdout;

    bitstring bits = { .len = 0 };

    process(ftell(in), in, out, bits);
}

