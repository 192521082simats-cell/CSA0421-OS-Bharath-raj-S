#!/usr/bin/env bash
# SysPilot Launch Script for Linux / WSL / macOS
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
cd "$DIR"

echo "======================================================================"
echo "   SysPilot: CPU Scheduling & Linux Resource Monitoring System"
echo "             Capstone Project Engine (POSIX / Linux)"
echo "======================================================================"

if command -v gcc >/dev/null 2>&1; then
    echo "[+] Building C Engine..."
    make -s
    echo "[+] C binary 'syspilot' compiled successfully."
fi

echo "[+] Starting Web Application on http://localhost:5000 ..."
if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "http://localhost:5000" &
elif command -v open >/dev/null 2>&1; then
    open "http://localhost:5000" &
fi

python3 "$DIR/backend/server.py"
