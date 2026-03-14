#!/bin/bash

# CYBERVIS v5.0 — Direct Installer
# Raw URLs for your files
BASE_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main"

echo "─── CYBERVIS INSTALLER ───"

# 1. Create a temporary build directory
mkdir -p cybervis_build
cd cybervis_build

# 2. Download the required files
echo "Fetching source files..."
curl -sL "$BASE_URL/cybervis.c" -o cybervis.c
curl -sL "$BASE_URL/Makefile" -o Makefile

# 3. Verify files exist
if [ ! -f "cybervis.c" ] || [ ! -f "Makefile" ]; then
    echo "Error: Could not download source files from GitHub."
    exit 1
fi

# 4. Run your Makefile
echo "Compiling..."
make

# 5. Install using your Makefile's install target
if [ -f "cybervis" ]; then
    echo "Installing..."
    sudo make install
    
    # Cleanup
    cd ..
    rm -rf cybervis_build
    echo "─── DONE! Type 'cybervis' to start ───"
else
    echo "Compilation failed. Check if 'build-essential' is installed."
    exit 1
fi
