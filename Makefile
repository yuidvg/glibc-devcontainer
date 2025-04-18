ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

CC = gcc
CFLAGS = -Wall -Wextra -Werror -I./include
CFLAGS_DEBUG = -g -std=c99


# Src
DIR_LIB = src
SRCS = $(wildcard $(DIR_LIB)/*.c)

# Test
DIR_TEST = test
TEST_SRCS = $(wildcard $(DIR_TEST)/*.c)


# Build
DIR_BUILD = build

# Object
DIR_OBJ = $(DIR_BUILD)/obj

DIR_OBJ_LIB = $(DIR_OBJ)/lib
OBJS_LIB = $(patsubst $(DIR_LIB)/%.c,$(DIR_OBJ_LIB)/$(DIR_LIB)/%.o,$(SRCS))

DIR_OBJ_TEST = $(DIR_OBJ)/test
OBJS_TEST = $(patsubst $(DIR_TEST)/%.c,$(DIR_OBJ_TEST)/$(DIR_TEST)/%.o,$(TEST_SRCS))

# Bin
DIR_BIN = $(DIR_BUILD)/bin

BIN_LIB = $(DIR_BIN)/libft_malloc_$(HOSTTYPE).so
BIN_LIB_SYM = $(DIR_BIN)/libft_malloc.so

BIN_TEST = $(DIR_BIN)/test.out


# Directories
$(DIR_BUILD) $(DIR_OBJ) $(DIR_BIN):
	mkdir -p $@

# Object files
$(DIR_OBJ_LIB)/%.o: $(DIR_LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DIR_OBJ_TEST)/%.o: $(DIR_TEST)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# Bin
$(BIN_LIB): $(OBJS_LIB)
	$(CC) -shared -o $@ $^
	ln -sf $(notdir $@) $(BIN_LIB_SYM)
$(BIN_TEST): $(OBJS_LIB) $(OBJS_TEST) $(BIN_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(DIR_BIN) -lmyMalloc

$(BIN_TEST): $(OBJS_LIB) $(OBJS_TEST) $(BIN_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(DIR_BIN) -lmyMalloc

all: $(DIRS) $(BIN_LIB) $(BIN_TEST) $(MAIN_TARGET)

test: $(BIN_TEST)
	./$(BIN_TEST)

clean:
	rm -rf $(DIR_OBJ) $(DIR_BIN)

re: clean all

leak: $(BIN_TEST)
	valgrind --leak-check=full --show-leak-kinds=all ./$(BIN_TEST)

.PHONY: all clean rebuild test leak
