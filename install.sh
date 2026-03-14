#!/bin/bash

# 1. THE URL - DOUBLE CHECK THIS! 
# If your branch is 'master', change 'main' to 'master' below.
URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main/cybervis.c"

echo "--- INSTALLING ---"

# 2. Download to a guaranteed location
curl -sL "$URL" -o /tmp/cybervis.c

# 3. Validation: If the file is empty or missing, stop now.
if [ ! -s /tmp/cybervis.c ]; then
    echo "ERROR: The file at $URL is empty or 404."
    echo "Please check if your branch name is 'main' or 'master'."
    exit 1
fi

# 4. Compile using the absolute path
gcc /tmp/cybervis.c -O3 -o /tmp/cybervis -lm -lpthread

# 5. Move it
if [ -f "/tmp/cybervis" ]; then
    sudo mv /tmp/cybervis /usr/local/bin/cybervis
    sudo chmod +x /usr/local/bin/cybervis
    echo "--- SUCCESS! Run 'cybervis' ---"
else
    echo "ERROR: gcc failed to build the binary."
    exit 1
fi
