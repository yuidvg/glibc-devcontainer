CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -std=c99 -I./include
SRC_DIR = src
TEST_DIR = test
OBJ_DIR = obj
BIN_DIR = bin

# Targets
LIB_TARGET = $(BIN_DIR)/libmy_malloc.a
TEST_TARGET = $(BIN_DIR)/test_my_malloc
MAIN_TARGET = $(BIN_DIR)/my_malloc_demo

# Source files
LIB_SRCS = $(SRC_DIR)/my_malloc.c
TEST_SRCS = $(TEST_DIR)/test_my_malloc.c
MAIN_SRCS = $(SRC_DIR)/main.c

# Object files
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/test_%.o,$(TEST_SRCS))
MAIN_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MAIN_SRCS))

# Directories
DIRS = $(OBJ_DIR) $(BIN_DIR)

# Default target
all: $(DIRS) $(LIB_TARGET) $(TEST_TARGET) $(MAIN_TARGET)

# Create directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Compile library source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test source files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(LIB_TARGET): $(LIB_OBJS)
	ar rcs $@ $^

# Build test executable
$(TEST_TARGET): $(TEST_OBJS) $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $< -L$(BIN_DIR) -lmy_malloc

# Build main program
$(MAIN_TARGET): $(MAIN_OBJS) $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $< -L$(BIN_DIR) -lmy_malloc

# Run main program
run: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Clean and rebuild
rebuild: clean all

# Run Valgrind memory check
memcheck: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_TARGET)

.PHONY: all clean rebuild test memcheck run
