#!/bin/bash

# CYBERVIS v5.0 — Installer
# Repository details
REPO_URL="https://github.com/logandaddy12-dot/cybervis.git"
TEMP_DIR="cybervis_build"

echo "─── CYBERVIS INSTALLER ───"

# 1. Clean up any previous failed attempts
rm -rf "$TEMP_DIR"

# 2. Clone the repo to get the Makefile and source
echo "Cloning repository..."
git clone --depth 1 "$REPO_URL" "$TEMP_DIR"

if [ $? -ne 0 ]; then
    echo "Error: Failed to clone repository."
    exit 1
fi

# 3. Enter directory and run the Makefile
cd "$TEMP_DIR"

echo "Compiling and installing..."
# This runs the 'install' target in your Makefile
sudo make install

# 4. Success check and cleanup
if [ $? -eq 0 ]; then
    echo "─── INSTALLATION COMPLETE ───"
    echo "Run it now by typing: cybervis"
    cd ..
    rm -rf "$TEMP_DIR"
else
    echo "Error: Build or installation failed."
    exit 1
fi
