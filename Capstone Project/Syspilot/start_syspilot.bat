@echo off
title SysPilot - OS Intelligence & CPU Scheduling Engine
color 0b
echo ======================================================================
echo    SysPilot: CPU Scheduling & Linux Resource Monitoring System
echo              Capstone Project Engine (Localhost Edition)
echo ======================================================================
echo.
echo Starting SysPilot Web Application on http://localhost:5000 ...
echo Press Ctrl+C in this terminal window to stop the server.
echo.

start "" "http://localhost:5000"
python "%~dp0backend\server.py"
pause
