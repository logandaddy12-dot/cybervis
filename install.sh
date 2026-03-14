#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Terminal aesthetics
CYAN='\033[1;36m'
GREEN='\033[1;32m'
RED='\033[1;31m'
BOLD='\033[1m'
NC='\033[0m' # No Color

PREFIX="/usr/local/bin"
REPO_URL="https://raw.githubusercontent.com/logandaddy12-dot/cybervis/main"

echo -e "${CYAN}${BOLD}CYBERVIS v5.0 Installer${NC}"
echo -e "=========================\n"

# 1. Dependency Check
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}[!] Error: 'gcc' is not installed. Please install 'build-essential' or 'gcc' first.${NC}"
    exit 1
fi

if ! command -v curl &> /dev/null; then
    echo -e "${RED}[!] Error: 'curl' is required to download the source file.${NC}"
    exit 1
fi

# 2. Setup Temporary Directory
echo -e "${CYAN}[*] Setting up build environment...${NC}"
TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

# 3. Download Source
echo -e "${CYAN}[*] Downloading source from GitHub...${NC}"
if ! curl -sO "$REPO_URL/cybervis.c"; then
    echo -e "${RED}[!] Failed to download cybervis.c. Check your internet connection or repo URL.${NC}"
    exit 1
fi

# 4. Compile
echo -e "${CYAN}[*] Compiling cybervis.c...${NC}"
if gcc -O3 -o cybervis cybervis.c -lm -lpthread; then
    echo -e "${GREEN}[+] Compilation successful!${NC}"
else
    echo -e "${RED}[!] Compilation failed.${NC}"
    exit 1
fi

# 5. Installation
echo -e "${CYAN}[*] Installing to ${PREFIX}...${NC}"

# Check if running as root, if not, use sudo for the install command
if [ "$EUID" -ne 0 ]; then
    echo -e "${CYAN}[*] Requesting sudo privileges to move executable...${NC}"
    if sudo install -m 755 cybervis "$PREFIX/cybervis"; then
        echo -e "${GREEN}[+] Installed successfully via sudo.${NC}"
    else
         echo -e "${RED}[!] Installation failed. Do you have sudo privileges?${NC}"
         exit 1
    fi
else
    install -m 755 cybervis "$PREFIX/cybervis"
    echo -e "${GREEN}[+] Installed successfully.${NC}"
fi

# 6. Cleanup
echo -e "${CYAN}[*] Cleaning up temporary files...${NC}"
cd - > /dev/null
rm -rf "$TMP_DIR"

echo -e "\n${GREEN}${BOLD}[✔] CYBERVIS is ready!${NC}"
echo -e "Run it anywhere by typing: ${CYAN}cybervis${NC}"
