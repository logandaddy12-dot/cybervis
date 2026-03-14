#!/bin/bash

# Configuration
SOURCE_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"
OUT_FILE="cybervis"

echo "--- Installing Cybervis ---"

# 1. Download the C file
echo "Downloading source..."
curl -sL "$SOURCE_URL" -o cybervis.c

# 2. Check if download actually worked
if [ ! -f "cybervis.c" ]; then
    echo "Error: Could not download cybervis.c. Check your internet or URL."
    exit 1
fi

# 3. Compile the file
echo "Compiling..."
gcc cybervis.c -o "$OUT_FILE"

# 4. Move to bin so you can run 'cybervis' anywhere
if [ -f "$OUT_FILE" ]; then
    sudo mv "$OUT_FILE" /usr/local/bin/
    chmod +x /usr/local/bin/cybervis
    echo "Success! You can now run the app by typing: cybervis"
else
    echo "Compilation failed. Do you have gcc installed?"
fi

# 5. Clean up the source file
rm cybervis.c
