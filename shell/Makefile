CC      = cc
CFLAGS  = -O2 -Wall -Wextra -std=c99 -pedantic
TARGET  = shell
SRC     = src/shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
