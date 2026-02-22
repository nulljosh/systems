CXX = clang++
CXXFLAGS = -std=c++14 -g -O2 -Wall

SRCS = main.cpp debugger.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = debugger

all: $(TARGET) test_prog

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

test_prog: test.c
	gcc -g -O0 -o test_prog test.c

clean:
	rm -f $(OBJS) $(TARGET) test_prog

.PHONY: all clean
