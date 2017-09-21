#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <stdio.h>

struct huff_state;
typedef unsigned int valtype;

typedef struct {
    unsigned long long bits;
    unsigned char len;
} bitstring;

typedef struct huff_value_node {
    valtype val;
    unsigned char internal;
} huff_value_node;

enum huff_walker_order {
    HUFF_LEAF       = (1 << 0),
    HUFF_PRE_ORDER  = (1 << 1),
    HUFF_IN_ORDER   = (1 << 2),
    HUFF_POST_ORDER = (1 << 3),
};

typedef int huff_dict_cb(char, bitstring, enum huff_walker_order, void *);

typedef int huff_walker(valtype val, bitstring key, double weight, int flags, void *userdata);

int  huff_init(struct huff_state **s);
int  huff_add(struct huff_state *s, valtype val, double weight);
int  huff_build(struct huff_state *s);
int  huff_walk(struct huff_state *s, huff_walker *w, void *userdata);
void huff_destroy(struct huff_state *s);

int  huff_load_dict(FILE *in, huff_dict_cb act, void *data);
int  huff_emit_length(FILE *stream, unsigned long long size);
int  huff_read_length(FILE *stream, unsigned long long *size);

#endif
