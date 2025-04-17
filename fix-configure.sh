#!/bin/bash

# Create a directory for patches
mkdir -p patches

# Create a patch to skip the PDE load address check
cat > patches/skip-pde-check.patch << 'EOF'
--- glibc-2.41/configure.orig	2023-08-01 00:00:00.000000000 +0000
+++ glibc-2.41/configure	2023-08-01 00:00:00.000000000 +0000
@@ -8840,7 +8840,8 @@
 { printf "%s\n" "$as_me:${as_lineno-$LINENO}: checking PDE load address" >&5
 printf %s "checking PDE load address... " >&6; }
 if test ${libc_cv_pde_load_address+y}
-then :
+then
+  :
 else case e in #(
   e) cat > conftest.S <<EOF
 .globl _start
@@ -8861,6 +8862,7 @@
+  libc_cv_pde_load_address=0x400000 ; printf "%s\n" "skipped check, using default 0x400000" >&6
   ;;
 esac
 fi
EOF

# Apply the patch
cd /workspaces/glibc-devcontainer
patch -p0 < patches/skip-pde-check.patch

# Make the script executable
chmod +x fix-configure.sh