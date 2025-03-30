#!/bin/bash

# Define variables
SRC_DIR="/workspaces/glibc-devcontainer/src"
BUILD_DIR="/workspaces/glibc-devcontainer/build"
PREFIX="/opt/glibc-debug"

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

echo "== Starting glibc build with debugging symbols =="

# Clean any previous build
echo "Cleaning previous build artifacts..."
cd ${BUILD_DIR}
rm -rf *

# Configure glibc with debug symbols
echo "Configuring glibc with debug symbols..."
cd ${BUILD_DIR}
${SRC_DIR}/configure \
  --prefix=${PREFIX} \
  --enable-debug \
  --with-pic \
  CFLAGS="-O1 -g3 -ggdb"

# Build glibc
echo "Building glibc (this may take a while)..."
cd ${BUILD_DIR}
make -j$(nproc)

echo "== Build complete =="
echo "To use the custom glibc, set LD_LIBRARY_PATH to include:"
echo "${BUILD_DIR}:${BUILD_DIR}/math:${BUILD_DIR}/elf:${BUILD_DIR}/dlfcn:${BUILD_DIR}/nss:${BUILD_DIR}/nis:${BUILD_DIR}/rt:${BUILD_DIR}/resolv:${BUILD_DIR}/mathvec:${BUILD_DIR}/support:${BUILD_DIR}/crypt:${BUILD_DIR}/nptl"
