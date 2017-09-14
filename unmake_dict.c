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

static int print_node(char byte, bitstring bits, FILE *out)
{
    fprintf(out, "0x%hhX = ", byte);
    print_bits(out, bits);
    fputc('\n', out);

    return 0;
}

int main()
{
    return huff_load_dict(stdin, (huff_dict_cb*)print_node, stdout);
}

