#!/bin/bash

# CYBERVIS v5.0 — Direct Installer
# URL for the source code
SRC="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "─── CYBERVIS INSTALLER ───"

# 1. Download the source directly into the current path
echo "Fetching: cybervis.c"
curl -sL "$SRC" -o cybervis.c

# 2. Compile immediately
echo "Building binary..."
gcc -O3 cybervis.c -o cybervis -lm -lpthread

# 3. Check if build worked and move to system path
if [ -f "cybervis" ]; then
    echo "Installing to /usr/local/bin..."
    sudo mv cybervis /usr/local/bin/
    sudo chmod +x /usr/local/bin/cybervis
    
    # Cleanup source file
    rm cybervis.c
    
    echo "Done! Type 'cybervis' to launch."
else
    echo "Error: Compilation failed. Make sure 'gcc' is installed."
    # Clean up anyway to keep dir tidy
    [ -f "cybervis.c" ] && rm cybervis.c
    exit 1
fi
