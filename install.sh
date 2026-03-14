#!/bin/bash

# CYBERVIS v5.0 — Direct Path Fix
# Using absolute paths to prevent "not found" errors
SRC_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"
WORK_DIR="/tmp/cybervis_build"

echo "─── CYBERVIS SYSTEM INSTALL ───"

# 1. Force create and enter a clean directory
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR" || { echo "Failed to create directory"; exit 1; }

# 2. Download the source
echo "Downloading source code..."
curl -sL "$SRC_URL" -o cybervis.c

# 3. Check if the file is actually there
if [ ! -f "cybervis.c" ]; then
    echo "Error: Source file not found. Check your URL: $SRC_URL"
    exit 1
fi

# 4. Compile using your exact flags
echo "Compiling..."
gcc -O3 cybervis.c -o cybervis -lm -lpthread

# 5. Move to local bin
if [ -f "cybervis" ]; then
    echo "Installing to /usr/local/bin..."
    sudo mv cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    echo "─── SUCCESS: Type 'cybervis' to launch ───"
else
    echo "Error: Compilation failed."
    exit 1
fi

# 6. Cleanup
cd /
rm -rf "$WORK_DIR"
