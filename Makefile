ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

CC = gcc
CFLAGS_COMMON = -Wall -Wextra -Werror  -std=c11
CFLAGS_DEBUG = -g -O0
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)

# Build Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Debug
DEBUG_BIN = $(BIN_DIR)/debug.out

DEBUG_DIR = debug
DEBUG_SRC = $(wildcard $(DEBUG_DIR)/*.c)

DEBUG_OBJ_DIR = $(OBJ_DIR)/debug
DEBUG_OBJ = $(patsubst $(DEBUG_DIR)/%.c,$(DEBUG_OBJ_DIR)/%.o,$(DEBUG_SRC))

# Tests
TEST_BIN = $(BIN_DIR)/test.out

TEST_OBJ_DIR = $(OBJ_DIR)/test
TEST_OBJ = $(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SRC))

TEST_DIR = test
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)

# Malloc
MALLOC_BIN = $(BIN_DIR)/libft_malloc_$(HOSTTYPE).so
MALLOC_BIN_SYM = $(BIN_DIR)/libft_malloc.so
MALLOC_INCLUDE = include

MALLOC_OBJ_DIR = $(OBJ_DIR)/malloc
MALLOC_OBJ = $(patsubst $(MALLOC_DIR)/%.c,$(MALLOC_OBJ_DIR)/%.o,$(MALLOC_SRC))

MALLOC_DIR = src
MALLOC_SRC = $(wildcard $(MALLOC_DIR)/*.c)

# Libft
LIBFT_DIR = libft
LIBFT_BIN = $(LIBFT_DIR)/libft.a
LIBFT_INCLUDE = $(LIBFT_DIR)/include
LIBFT_SRC = $(wildcard $(LIBFT_DIR)/*.c)


#-----------------------------------------------------------------------------------------------------------#
# Rules
#-----------------------------------------------------------------------------------------------------------#
all:  $(MALLOC_BIN) $(DEBUG_BIN) $(TEST_BIN)

malloc: $(MALLOC_BIN_SYM)

debug: $(DEBUG_BIN)

test: $(TEST_BIN)

run-test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(DEBUG_BIN) $(DEBUG_OBJ) $(TEST_BIN) $(TEST_OBJ) $(MALLOC_BIN_SYM) $(MALLOC_BIN) $(MALLOC_OBJ)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: clean
	$(MAKE) all

.PHONY: all malloc debug test clean re run-test

# Debug
$(DEBUG_BIN): $(DEBUG_OBJ) $(MALLOC_BIN_SYM)
	$(CC) $(CFLAGS) -o $@ $(DEBUG_OBJ) -L$(BIN_DIR) -lft_malloc -Wl,-rpath=$(BIN_DIR)
$(DEBUG_OBJ): $(DEBUG_OBJ_DIR)/%.o: $(DEBUG_DIR)/%.c
	$(CC) $(CFLAGS) -c -I$(MALLOC_INCLUDE) $< -o $@

# Tests
$(TEST_BIN): $(TEST_OBJ) $(MALLOC_BIN_SYM)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) -L$(BIN_DIR) -lft_malloc -Wl,-rpath=$(BIN_DIR)
$(TEST_OBJ): $(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c -I$(MALLOC_INCLUDE) $< -o $@

# Malloc
$(MALLOC_BIN_SYM): $(MALLOC_BIN)
	ln -sf $(notdir $(MALLOC_BIN)) $(MALLOC_BIN_SYM)
$(MALLOC_BIN): $(LIBFT_BIN) $(MALLOC_OBJ)
	$(CC) -shared -o $@ $(MALLOC_OBJ) -L$(LIBFT_DIR) -lft -Wl,--version-script=libft_malloc.map
$(MALLOC_OBJ): $(MALLOC_OBJ_DIR)/%.o: $(MALLOC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(MALLOC_INCLUDE) -I$(LIBFT_INCLUDE) -fPIC -c $< -o $@

# Libft
$(LIBFT_BIN):
	$(MAKE) -C $(LIBFT_DIR)

#-----------------------------------------------------------------------------------------------------------#
