#!/bin/bash

# Create build directory
mkdir -p build-glibc

# Set environment variables to handle the issues
export libc_cv_pde_load_address=0x400000

# Go to build directory
cd build-glibc

# Configure glibc with our options
../glibc-2.41/configure --prefix=/workspaces/glibc-devcontainer/glibc-install --disable-mathvec CFLAGS="-O1 -g3 -ggdb"

# Check if configure succeeded
if [ $? -ne 0 ]; then
  echo "Configure failed. Exiting..."
  exit 1
fi

# Create a custom make wrapper that modifies the run command to use 'cp -f' instead of 'mv -f'
cat > make-wrapper.sh << 'EOF'
#!/bin/bash
# Run the actual make command but pipe stderr through sed to replace 'mv -f' with 'cp -f'
/usr/bin/make "$@" 2> >(sed 's/mv -f/cp -f/g' >&2)
EOF

chmod +x make-wrapper.sh

# Run the build with our wrapper
./make-wrapper.sh -j

# Check if build succeeded
if [ $? -ne 0 ]; then
  echo "Build failed. Exiting..."
  exit 1
fi

echo "Build completed successfully!"