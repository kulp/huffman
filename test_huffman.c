#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>

struct walk_state {
    double weights;
    double count;
};

static int walker(valtype val, bitstring key, double weight, void *userdata)
{
    struct walk_state *w = userdata;

    char str[65] = "0";
    if (key.len >= sizeof str)
        return -1;

    for (int i = 0; i < key.len; i++)
        str[key.len - i - 1] = ((key.bits >> i) & 1) + '0';

    const char *sign = val < 0 ? "-" : "";
    printf("%s0x%x = %d'b%s, weight %4.2f\n", sign, abs(val), key.len, str, weight);

    w->weights += weight * key.len;
    w->count += weight;

    return 0;
}

int main(int argc, char *argv[])
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

    struct walk_state state = { 0 };

    if (huff_walk(st, walker, &state))
        fprintf(stderr, "error while walking : did you huff_build() ?\n");

    printf("Probable bitlen is %f for a total of %.0f entries in %d bits\n",
            state.weights / state.count, state.count, (int)(state.weights + .5));

    huff_destroy(st);

    return 0;
}
