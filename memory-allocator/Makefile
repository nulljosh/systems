CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2
LIB = libmymalloc.a

all: $(LIB)

$(LIB): src/malloc.o
	ar rcs $@ $^

src/malloc.o: src/malloc.c src/malloc.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(LIB)
	$(CC) $(CFLAGS) -o test_malloc tests/test_malloc.c -L. -lmymalloc
	./test_malloc

clean:
	rm -f src/*.o $(LIB) test_malloc

.PHONY: all test clean
