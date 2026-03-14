#!/bin/bash

# CYBERVIS v5.0 — Direct Path Installer
# Ensure this URL is the RAW link to your code
RAW_SRC="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "─── CYBERVIS INSTALLER ───"

# 1. Jump to /tmp to ensure we have write permissions
cd /tmp

# 2. Download the source code
echo "Downloading source..."
curl -sL "$RAW_SRC" -o cybervis.c

# 3. Check if the file actually exists now
if [ ! -f "cybervis.c" ]; then
    echo "Fatal Error: cybervis.c could not be downloaded."
    echo "Check your URL: $RAW_SRC"
    exit 1
fi

# 4. Compile (using your Makefile flags)
echo "Compiling..."
gcc -O3 cybervis.c -o cybervis -lm -lpthread

# 5. Move to local bin
if [ -f "cybervis" ]; then
    echo "Installing to /usr/local/bin..."
    sudo mv cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    rm cybervis.c
    echo "─── SUCCESS: Type 'cybervis' to launch ───"
else
    echo "Error: Compilation failed. Is 'gcc' installed?"
    exit 1
fi
