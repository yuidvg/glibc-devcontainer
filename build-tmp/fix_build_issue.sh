#!/bin/bash

# Script to fix glibc build issue with 'mv: are the same file' error
# This modifies the build system to use a safer file operation method

# Check if directories exist
if [ ! -d "src-glibc" ] || [ ! -d "build-glibc" ]; then
  echo "Error: src-glibc or build-glibc directories not found!"
  exit 1
fi

# Replace problematic mv commands in o-iterator.mk
sed -i 's|mv -f|rm -f $@; cp -f|g' src-glibc/o-iterator.mk

# Clean up any existing temporary files that might cause issues
find build-glibc -name "stamp.*T" -delete
find build-glibc -name "*.T" -delete
rm -f build-glibc/csu/stamp.*

echo "Build system modified to avoid file move issues."
echo "Temporary files cleaned up."
echo "Now try building again with 'make -C build-glibc -j'"