CC = cc
CFLAGS = -O2 -Wall -Wextra -std=c99 -pedantic
SRCS = main.c lexer.c parser.c value.c table.c interpreter.c builtins.c
TARGET = jot

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lm

test: $(TARGET)
	bash tests_run.sh

clean:
	rm -f $(TARGET)

.PHONY: clean test
