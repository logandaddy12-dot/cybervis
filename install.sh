#!/bin/bash

# 1. Define the exact source
# Verify this URL is correct for your 'cybervis.c'
SOURCE="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "--- INSTALLING CYBERVIS ---"

# 2. Download directly to the compiler
# This pipes the web code straight into gcc
echo "Step 1: Downloading and Compiling..."
curl -sL "$SOURCE" | gcc -x c -O3 - -o cybervis -lm -lpthread

# 3. Check if the binary was actually created
if [ ! -f "cybervis" ]; then
    echo "ERROR: Compilation failed! Is gcc installed?"
    echo "Try: sudo apt update && sudo apt install build-essential -y"
    exit 1
fi

# 4. Move it to the system path so you can run 'cybervis'
echo "Step 2: Moving to /usr/local/bin..."
sudo mv cybervis /usr/local/bin/cybervis
sudo chmod +x /usr/local/bin/cybervis

# 5. Final Confirmation
if [ -f "/usr/local/bin/cybervis" ]; then
    echo "--- SUCCESS ---"
    echo "You can now run the app by typing: cybervis"
else
    echo "ERROR: Move failed. Try running the curl command with sudo."
fi
