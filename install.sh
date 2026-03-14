#!/bin/bash
# wake — Wake-on-LAN TUI installer
set -e

echo ""
echo " ██╗    ██╗ ██████╗ ██╗     "
echo " ██║    ██║██╔═══██╗██║     "
echo " ██║ █╗ ██║██║   ██║██║     "
echo " ╚███╔███╔╝╚██████╔╝███████╗"
echo "  Wake-on-LAN — Installer   "
echo ""

if ! command -v python3 &>/dev/null; then
    echo "❌  python3 not found. Install: sudo apt install python3"
    exit 1
fi
PY=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
echo "✓  Python $PY"

if [ ! -f "wake" ]; then
    echo "❌  'wake' script not found in current directory."
    exit 1
fi

head -1 wake | grep -q "python3" || sed -i '1s|^|#!/usr/bin/env python3\n|' wake

echo ""
echo "Install location:"
echo "  1) /usr/local/bin/wake  (system-wide, needs sudo)"
echo "  2) ~/.local/bin/wake    (current user only)"
read -rp "Choose [1/2]: " CHOICE

if [ "$CHOICE" = "1" ]; then
    DEST="/usr/local/bin/wake"
    sudo cp wake "$DEST"
    sudo chmod +x "$DEST"
else
    DEST="$HOME/.local/bin/wake"
    mkdir -p "$HOME/.local/bin"
    cp wake "$DEST"
    chmod +x "$DEST"
    if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
        echo ""
        echo "⚠  Add to ~/.bashrc:  export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
fi

echo ""
echo "✓  Installed → $DEST"
echo "Run with:    wake"
echo "Config:      ~/.config/wol_tui/"
echo ""
