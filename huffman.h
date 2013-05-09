#ifndef HUFFMAN_H_
#define HUFFMAN_H_

struct huff_state;
typedef int valtype;

typedef struct {
    long long bits;
    unsigned char len;
} bitstring;

typedef int huff_walker(valtype val, bitstring key, double weight, void *userdata);

int  huff_init(struct huff_state **s);
int  huff_add(struct huff_state *s, valtype val, double weight);
int  huff_build(struct huff_state *s);
int  huff_walk(struct huff_state *s, huff_walker *w, void *userdata);
void huff_destroy(struct huff_state *s);

#endif
