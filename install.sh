#!/bin/bash

# CYBERVIS v5.0 — Remote Installer
# This URL must point to your RAW cybervis.c file
SRC_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "─── CYBERVIS v5.0 INSTALLER ───"

# 1. Download the file to a temporary location
echo "Downloading cybervis.c..."
curl -sL "$SRC_URL" -o /tmp/cybervis.c

# 2. Check if the download actually worked
if [ ! -f "/tmp/cybervis.c" ]; then
    echo "Error: Could not download the source file. Check the URL!"
    exit 1
fi

# 3. Compile it using your specific CFLAGS
echo "Compiling..."
gcc -O3 -o /tmp/cybervis /tmp/cybervis.c -lm -lpthread

# 4. Install the binary
if [ -f "/tmp/cybervis" ]; then
    echo "Installing to /usr/local/bin..."
    sudo mv /tmp/cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    
    # Cleanup the temp source
    rm /tmp/cybervis.c
    
    echo "Success! Run it now by typing: cybervis"
else
    echo "Compilation failed. Do you have gcc installed?"
    exit 1
fi
