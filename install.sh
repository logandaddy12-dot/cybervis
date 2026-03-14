#!/bin/bash

# CYBERVIS v5.0 — Direct Stream Installer
# This URL must be the RAW source code link
SRC="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis"

echo "─── CYBERVIS INSTALLER ───"

# 1. Compile directly from the curl stream and output to /usr/local/bin
# We use '-' to tell gcc to read from stdin (the pipe)
echo "Downloading and Compiling..."
curl -sL "$SRC" | gcc -x c -O3 - -o cybervis -lm -lpthread

# 2. Move to bin and set permissions
if [ -f "cybervis" ]; then
    sudo mv cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    echo "─── DONE: Run it with 'cybervis' ───"
else
    echo "Error: Installation failed. Make sure 'gcc' and 'build-essential' are installed."
    exit 1
fi
