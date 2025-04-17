#!/bin/bash

# This script works around the PDE load address issue by setting the variable
# before running the configure script

# Set the PDE load address to a default value
export libc_cv_pde_load_address=0x400000

# Run the original configure with our arguments
exec ../glibc-2.41/configure "$@"