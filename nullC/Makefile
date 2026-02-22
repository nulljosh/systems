CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc
SRC = src/main.c src/lexer.c src/parser.c src/ast.c src/codegen.c
TARGET = nullc
PEEPHOLE = peephole

all: $(TARGET) $(PEEPHOLE)

$(TARGET): $(SRC) src/lexer.h src/parser.h src/ast.h src/codegen.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

$(PEEPHOLE): src/peephole.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) *.s *.o output

test: $(TARGET)
	@for f in examples/level*.c; do \
		echo "=== $$f ==="; \
		./$(TARGET) $$f 2>&1 || true; \
		echo ""; \
	done

.PHONY: all clean test
