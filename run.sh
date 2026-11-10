#!/bin/sh
# Chip Chatbot launcher (Linux / macOS / Git Bash)
# Builds the bot if needed, then runs it.
cd "$(dirname "$0")"

if [ ! -x ./chatbot ]; then
    echo "Building Chip..."
    if ! command -v gcc >/dev/null 2>&1; then
        echo "gcc not found. Install it, then retry."
        exit 1
    fi
    gcc -std=c99 -Wall -Wextra -O2 -o chatbot src/chatbot.c || exit 1
fi

./chatbot
