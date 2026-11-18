# SysPilot Makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude
LDFLAGS ?= -lm

SRC = src/monitor.c src/analyzer.c src/scheduler.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = syspilot

TEST_SRC = tests/test_scheduler.c src/scheduler.c
TEST_TARGET = test_scheduler_bin

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_TARGET) $(LDFLAGS)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)
