#!/bin/bash
set -e
REPO="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main"
echo "Installing cybervis..."
if ! command -v gcc &>/dev/null; then
    sudo apt-get install -y gcc
fi
curl -sL "$REPO/cybervis.c" -o /tmp/cybervis.c
echo "Compiling..."
gcc -O3 -o /tmp/cybervis /tmp/cybervis.c -lm -lpthread
sudo install -m 755 /tmp/cybervis /usr/local/bin/cybervis
rm /tmp/cybervis.c /tmp/cybervis
echo "Done! Run: cybervis"
