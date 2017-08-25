CFLAGS += -g
CFLAGS += -Wall -Wextra -pedantic-errors

# ignore unused expressions in assertions (used for labeling)
CPPFLAGS += -Wno-unused

TARGETS += make_dict test_huffman
all: $(TARGETS)

make_dict test_huffman: huffman.o

clean:
	$(RM) *.o $(TARGETS)
