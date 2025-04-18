#!/bin/bash

# Fix script for glibc build issue with mv: 'stamp.osT' and 'stamp.os' are the same file
# This is a common error with certain filesystem configurations

# Check if directories exist
if [ ! -d "src-glibc" ] || [ ! -d "build-glibc" ]; then
  echo "Error: src-glibc or build-glibc directories not found!"
  exit 1
fi

# Create a patch for Makerules to fix the issue with mv commands
cat > build-tmp/makerules.patch << 'EOF'
--- a/src-glibc/Makerules
+++ b/src-glibc/Makerules
@@ -1222,7 +1222,7 @@
 # containing an explicit list of all the needed object files when such
 # information is available.
 $(objpfx)stamp.%: files-list-% 2>/dev/null
-	echo '$$(foreach object,$(words $($(basename $(basename $(@F)))-$(suffix $(basename $@)))),$(subst .$$(%),$(suffix $(basename $@)),$(object)))' > $@T
-	mv -f $@T $@
+	echo '$$(foreach object,$(words $($(basename $(basename $(@F)))-$(suffix $(basename $@)))),$(subst .$$(%),$(suffix $(basename $@)),$(object)))' > $@T
+	rm -f $@; cp -f $@T $@; rm -f $@T

 $(objpfx)stamp.S: files-list-S 2>/dev/null
EOF

# Apply the patch
patch -p1 < build-tmp/makerules.patch

# Create a fix for o-iterator.mk as well
cat > build-tmp/o-iterator.patch << 'EOF'
--- a/src-glibc/o-iterator.mk
+++ b/src-glibc/o-iterator.mk
@@ -8,3 +8,6 @@
 o := $(firstword $(object-suffixes-left))
 object-suffixes-left := $(filter-out $o,$(object-suffixes-left))

-$(o-iterator-doit)
+# Modified to avoid "mv: files are the same" error
+# by replacing mv -f with rm+cp
+$(subst mv -f,rm -f $@; cp -f,$(o-iterator-doit))
EOF

# Apply the patch
patch -p1 < build-tmp/o-iterator.patch

echo "Patches applied. Now try building again with 'make -C build-glibc -j'"