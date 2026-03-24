CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2 -lm
SRC = src/main.c src/world.c src/body.c src/collision.c src/solver.c src/broadphase.c
OBJ = $(SRC:.c=.o)
BIN = physics_demo

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
