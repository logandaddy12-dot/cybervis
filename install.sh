#!/bin/bash

# CYBERVIS v5.0 — Remote Installer
# This script downloads the source from GitHub and installs the binary.

REPO_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"
BINARY_NAME="cybervis"
INSTALL_PATH="/usr/local/bin/$BINARY_NAME"

echo "─── CYBERVIS v5.0 REMOTE INSTALLER ───"

# 1. Check for compiler
if ! command -v gcc &> /dev/null; then
    echo "Error: gcc is not installed. Run 'sudo apt install build-essential' first."
    exit 1
fi

# 2. Download the source file
echo "Fetching source from GitHub..."
curl -s -O "$REPO_URL"

if [ ! -f "cybervis.c" ]; then
    echo "Error: Failed to download cybervis.c from $REPO_URL"
    exit 1
fi

# 3. Compile
echo "Compiling CYBERVIS..."
gcc -O3 -o "$BINARY_NAME" cybervis.c -lm -lpthread

if [ $? -eq 0 ]; then
    echo "Build successful."
else
    echo "Compilation failed."
    exit 1
fi

# 4. Install to system path
echo "Installing to $INSTALL_PATH (requires sudo)..."
sudo mv "$BINARY_NAME" "$INSTALL_PATH"
sudo chmod +x "$INSTALL_PATH"

# 5. Cleanup
rm cybervis.c

if [ $? -eq 0 ]; then
    echo "─── INSTALLATION COMPLETE ───"
    echo "Run it now by typing: cybervis"
else
    echo "Installation failed during move."
    exit 1
fi
