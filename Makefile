CFLAGS += -g
CFLAGS += -Wall -Wextra -pedantic-errors

TARGETS += make_dict test_huffman
all: $(TARGETS)

make_dict test_huffman: huffman.o

clean:
	$(RM) *.o $(TARGETS)

