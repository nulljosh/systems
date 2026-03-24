CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2
SRC = src/main.c src/parser.c src/nfa.c src/dfa.c src/match.c
OBJ = $(SRC:.c=.o)
BIN = regex

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(BIN)
	./tests/run_tests.sh

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all test clean
