CFLAGS += -g
CFLAGS += -Wall -Wextra -pedantic-errors

TARGETS += test_huffman
all: $(TARGETS)

test_huffman: huffman.o

clean:
	$(RM) *.o $(TARGETS)

