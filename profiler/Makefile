CC = clang
CFLAGS = -fPIC -O2 -Wall -Wextra
LDFLAGS = -dynamiclib -pthread -ldl
TARGET = libprofiler.dylib
SOURCES = profiler.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✓ Built $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✓ Cleaned"

test: $(TARGET)
	python3 profiler_binding.py profile ./output
	@echo "✓ Test completed"

.DEFAULT_GOAL := all
