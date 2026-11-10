@echo off
rem Chip Chatbot launcher (Windows)
rem Builds the bot if needed, then runs it.

cd /d "%~dp0"

if not exist chatbot.exe (
    echo Building Chip...
    where gcc >nul 2>nul
    if errorlevel 1 (
        echo GCC not found on PATH. Install it or add it to PATH, then retry.
        pause
        exit /b 1
    )
    gcc -std=c99 -Wall -Wextra -O2 -o chatbot.exe src/chatbot.c
    if errorlevel 1 (
        echo Build failed.
        pause
        exit /b 1
    )
)

chatbot.exe
pause
