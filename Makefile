CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

# Source files
SRCS = mythreads.c
TEST_SRC = test_mythreads.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executables
TEST_EXEC = test_mythreads

.PHONY: all clean test

all: $(TEST_EXEC)

$(TEST_EXEC): $(TEST_SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c mythreads.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -f $(OBJS) $(TEST_EXEC)
