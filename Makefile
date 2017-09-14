CFLAGS += -g
CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -pedantic-errors

# ignore unused expressions in assertions (used for labeling)
CPPFLAGS += -Wno-unused

TARGETS += make_dict unmake_dict test_huffman
all: $(TARGETS)

make_dict test_huffman unmake_dict: huffman.o

clean:
	$(RM) *.o $(TARGETS)

