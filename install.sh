#!/bin/bash

# CYBERVIS v5.0 — The "Actually Works" Version
# This URL MUST be the raw link to your code
RAW_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "─── STARTING INSTALL ───"

# 1. Download the code to /tmp (Safe zone)
curl -sL "$RAW_URL" -o /tmp/cybervis.c

# 2. Check if the download worked
if [ ! -s /tmp/cybervis.c ]; then
    echo "ERROR: Could not download code. Check your URL or Internet."
    exit 1
fi

# 3. Compile from the safe zone to a binary
gcc /tmp/cybervis.c -O3 -o /tmp/cybervis -lm -lpthread

# 4. Move the finished app to your bin
if [ -f /tmp/cybervis ]; then
    sudo mv /tmp/cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    rm /tmp/cybervis.c
    echo "─── SUCCESS! TYPE: cybervis ───"
else
    echo "ERROR: GCC failed to compile. Make sure 'build-essential' is installed."
    exit 1
fi
