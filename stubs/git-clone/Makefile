CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lz -lcrypto
SRC = src/main.c src/object.c src/index.c src/refs.c src/pack.c src/diff.c
OBJ = $(SRC:.c=.o)
BIN = mygit

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(BIN)
	./tests/run_tests.sh

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all test clean
