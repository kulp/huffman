#include "huffman.h"
#include <stdio.h>

typedef int huff_dict_cb(char, bitstring, void *);

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

static int process_chunk_(long offset, FILE *in, bitstring b, huff_dict_cb act, void *data)
{
    unsigned char arr[2];
    fseek(in, offset, SEEK_SET);
    size_t result = fread(arr, sizeof arr, 1, in);
    // TODO check result
    if (arr[1] != 0) {
        // internal node
        bitstring l = { .bits = (b.bits << 1) | 0, .len = b.len + 1 },
                  r = { .bits = (b.bits << 1) | 1, .len = b.len + 1 };

        process_chunk_(offset + arr[0], in, l, act, data);
        process_chunk_(offset + arr[1], in, r, act, data);
    } else {
        // leaf node
        act(arr[0], b, data);
    }

    return 0;
}

int huff_load_dict(FILE *in, huff_dict_cb act, void *data)
{
    bitstring bits = { .len = 0 };

    return process_chunk_(ftell(in), in, bits, act, data);
}

int main()
{
    return huff_load_dict(stdin, (huff_dict_cb*)print_node, stdout);
}

