# Makefile for building glibc with debug symbols and testing

# Base directory (absolute path to current directory)

# Directories
SRCDIR = glibc-2.31/
BUILDDIR = build-glibc/
TESTDIR = test/

# Compiler flags
CFLAGS_FOR_DEBUG = -O1 -g3 -ggdb
CONFIGURE_FLAGS = --prefix=/workspaces/glibc-devcontainer/glibc-install

# LD_LIBRARY_PATH for running the test program
export LD_LIBRARY_PATH = $(BUILDDIR):$(BUILDDIR)math:$(BUILDDIR)elf:$(BUILDDIR)dlfcn:$(BUILDDIR)nss:$(BUILDDIR)nis:$(BUILDDIR)rt:$(BUILDDIR)resolv:$(BUILDDIR)mathvec:$(BUILDDIR)support:$(BUILDDIR)crypt:$(BUILDDIR)nptl

# Default target
all: configure-glibc build-glibc build-test

# GLIBC
# Configure glibc
configure-glibc: clean-glibc
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && ../$(SRCDIR)configure $(CONFIGURE_FLAGS) CFLAGS="$(CFLAGS_FOR_DEBUG)"

build-glibc: configure-glibc
	cd $(BUILDDIR) && make -j

clean-glibc:
	rm -rf $(BUILDDIR)*

# TEST
build-test:
	cd $(TESTDIR) && gcc -g3 -O0 malloc_test.c -o malloc_test

clean-test:
	rm -f $(TESTDIR)malloc_test

# Clean all generated files
clean: clean-glibc clean-test

.PHONY: all clean-glibc configure-glibc build-glibc build-test clean-test clean
