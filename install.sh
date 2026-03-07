#!/bin/bash
set -e
REPO="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main"
echo "Installing cybervis..."
if ! command -v gcc &>/dev/null; then
    sudo apt-get install -y gcc
fi
curl -sL "$REPO/Cybervis.c" -o /tmp/Cybervis.c
echo "Compiling..."
gcc -O3 -o /tmp/cybervis /tmp/Cybervis.c -lm -lpthread
sudo install -m 755 /tmp/cybervis /usr/local/bin/cybervis
rm /tmp/Cybervis.c /tmp/cybervis
echo "Done! Run: cybervis"
