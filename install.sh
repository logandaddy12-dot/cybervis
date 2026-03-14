#!/bin/bash

# CYBERVIS v5.0 - Installer
# Target: /usr/local/bin/cybervis

BINARY_NAME="cybervis"
SOURCE_FILE="cybervis.c"
INSTALL_PATH="/usr/local/bin/$BINARY_NAME"

echo "─── CYBERVIS v5.0 INSTALLER ───"

# 1. Check if source exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: $SOURCE_FILE not found in current directory."
    exit 1
fi

# 2. Compile with optimized flags
echo "Compiling $SOURCE_FILE..."
gcc -O3 -o "$BINARY_NAME" "$SOURCE_FILE" -lm -lpthread

if [ $? -eq 0 ]; then
    echo "Build successful."
else
    echo "Build failed. Do you have gcc and build-essential installed?"
    exit 1
fi

# 3. Install to /usr/local/bin
echo "Moving binary to $INSTALL_PATH (requires sudo)..."
sudo mv "$BINARY_NAME" "$INSTALL_PATH"
sudo chmod +x "$INSTALL_PATH"

if [ $? -eq 0 ]; then
    echo "Installation complete!"
    echo "You can now run 'cybervis' from any terminal."
else
    echo "Installation failed during file move."
    exit 1
fi
