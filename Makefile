ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

CC = gcc
CFLAGS_DEBUG = -g -std=c99
CFLAGS = -Wall -Wextra -Werror -I./include -I./libft/includes $(CFLAGS_DEBUG)

# libft
DIR_LIBFT = libft
BIN_LIBFT = $(DIR_LIBFT)/libft.a
SRCS_LIBFT = $(wildcard $(DIR_LIBFT)/*.c)

# libft_malloc
DIR_LIB = src
SRCS = $(wildcard $(DIR_LIB)/*.c)

# Test
DIR_TEST = test
TEST_SRCS = $(DIR_TEST)/test.c


# Build
DIR_BUILD = build

# Object
DIR_OBJ = $(DIR_BUILD)/obj

DIR_OBJ_LIB = $(DIR_OBJ)/lib
OBJS_LIB = $(patsubst $(DIR_LIB)/%.c,$(DIR_OBJ_LIB)/%.o,$(SRCS))

DIR_OBJ_TEST = $(DIR_OBJ)/test
OBJS_TEST = $(patsubst $(DIR_TEST)/%.c,$(DIR_OBJ_TEST)/%.o,$(TEST_SRCS))

# Bin
DIR_BIN = $(DIR_BUILD)/bin

BIN_LIB = $(DIR_BIN)/libft_malloc_$(HOSTTYPE).so
BIN_LIB_SYM = $(DIR_BIN)/libft_malloc.so

BIN_TEST = $(DIR_BIN)/test.out


all: $(DIR_BUILD) $(DIR_OBJ) $(DIR_BIN) $(DIR_OBJ_LIB) $(DIR_OBJ_TEST) $(BIN_LIB) $(BIN_TEST)

# libft
$(BIN_LIBFT):
	make -C $(DIR_LIBFT)

# Directories
$(DIR_BUILD) $(DIR_OBJ) $(DIR_BIN) $(DIR_OBJ_LIB) $(DIR_OBJ_TEST):
	mkdir -p $@

# Object files
$(DIR_OBJ_LIB)/%.o: $(DIR_LIB)/%.c | $(DIR_OBJ_LIB)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(DIR_OBJ_TEST)/%.o: $(DIR_TEST)/%.c | $(DIR_OBJ_TEST)
	$(CC) $(CFLAGS) -c $< -o $@


# Bin
$(BIN_LIB): $(BIN_LIBFT) $(OBJS_LIB) | $(DIR_BIN)
	$(CC) -shared -o $@ $^
	ln -sf $(notdir $@) $(BIN_LIB_SYM)

$(BIN_TEST): $(OBJS_TEST) $(BIN_LIB) | $(DIR_BIN)
	$(CC) $(CFLAGS) -o $@ $(OBJS_TEST) -L$(DIR_BIN) -lft_malloc


test: $(BIN_TEST)
	LD_LIBRARY_PATH=./$(DIR_BIN) ./$(BIN_TEST)

clean:
	rm -rf $(DIR_BUILD)

re: clean all

leak: $(BIN_TEST)
	LD_LIBRARY_PATH=./$(DIR_BIN) valgrind --leak-check=full --show-leak-kinds=all ./$(BIN_TEST)

.PHONY: all clean re test leak
