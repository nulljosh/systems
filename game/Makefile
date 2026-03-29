CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lncurses
SRC = src/main.c src/render.c src/input.c src/entity.c src/map.c src/physics.c
OBJ = $(SRC:.c=.o)
BIN = game

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
