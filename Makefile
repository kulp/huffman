CFLAGS += -g
CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -pedantic-errors

# ignore unused expressions in assertions (used for labeling)
CPPFLAGS += -Wno-unused

TARGETS += make_dict unmake_dict test_huffman encode_bytes decode_bytes
all: $(TARGETS)

$(TARGETS): huffman.o

clean:
	$(RM) *.o $(TARGETS)

