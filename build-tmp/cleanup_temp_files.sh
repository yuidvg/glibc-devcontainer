#!/bin/bash

# Clean up temporary files that might be causing the build issue

# Remove all temporary stamp files
find build-glibc -name "stamp.*T" -delete
find build-glibc -name "*.T" -delete

# Clean up any problematic stamp files in csu directory
rm -f build-glibc/csu/stamp.*

echo "Temporary files cleaned up. Now try building again with 'make -C build-glibc -j'"