#!/bin/bash

# CYBERVIS v5.0 — Absolute Fix Installer
REPO_URL="https://github.com/logandaddy12-dot/cybervis.git"
BUILD_DIR="/tmp/cybervis_build"

echo "─── CYBERVIS SYSTEM INSTALL ───"

# 1. Clean up and setup workspace
sudo rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit

# 2. Use Git to pull the full project
echo "Cloning repository..."
if ! git clone --depth 1 "$REPO_URL" . ; then
    echo "Error: Git clone failed. Is 'git' installed?"
    echo "Try: sudo apt install git -y"
    exit 1
fi

# 3. Verify files are actually here
if [ ! -f "cybervis.c" ]; then
    echo "Error: cybervis.c not found in repository!"
    exit 1
fi

# 4. Use your Makefile
echo "Building and Installing..."
sudo make install

# 5. Final Cleanup
if [ $? -eq 0 ]; then
    echo "─── INSTALLATION SUCCESSFUL ───"
    echo "Type 'cybervis' to launch."
    cd /tmp
    sudo rm -rf "$BUILD_DIR"
else
    echo "Error: 'make install' failed."
    exit 1
fi
