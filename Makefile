# Makefile for building glibc with debug symbols and testing

# Base directory (current directory)
BASE_DIR = .

# Directories (relative paths)
SRCDIR = $(BASE_DIR)/src
BUILDDIR = $(BASE_DIR)/build
TESTDIR = $(BASE_DIR)/test

# Compiler flags
CFLAGS = -O1 -g3 -ggdb
CONFIGURE_FLAGS = --prefix=/opt/glibc-debug --enable-debug --with-pic

# LD_LIBRARY_PATH for running the test program
export LD_LIBRARY_PATH = $(BUILDDIR):$(BUILDDIR)/math:$(BUILDDIR)/elf:$(BUILDDIR)/dlfcn:$(BUILDDIR)/nss:$(BUILDDIR)/nis:$(BUILDDIR)/rt:$(BUILDDIR)/resolv:$(BUILDDIR)/mathvec:$(BUILDDIR)/support:$(BUILDDIR)/crypt:$(BUILDDIR)/nptl

# Default target
all: configure build compile-test

# Clean the build directory
clean-build:
	rm -rf $(BUILDDIR)/*

# Configure glibc
configure: clean-build
	# Ensure build directory exists after cleaning
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && $(SRCDIR)/configure $(CONFIGURE_FLAGS) CFLAGS="$(CFLAGS)"

# Build glibc
build:
	cd $(BUILDDIR) && make -j$$(nproc)

# Compile the test program
compile-test:
	cd $(TESTDIR) && gcc -g3 -O0 malloc_test.c -o malloc_test

# Run the test program
run-test: compile-test
	cd $(TESTDIR) && ./malloc_test

# Clean test artifacts
clean-test:
	rm -f $(TESTDIR)/malloc_test

# Complete setup
setup: all
	@echo "Setup complete. You can now use VSCode to debug the malloc implementation."

# Clean all generated files
clean: clean-build clean-test

.PHONY: all clean-build configure build compile-test run-test clean-test setup clean
