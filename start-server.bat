@echo off
REM Simple server launcher for Windows

echo ========================================
echo   Starting WASM Test Server
echo ========================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python is not installed or not in PATH
    echo Please install Python 3 from https://www.python.org/
    pause
    exit /b 1
)

REM Start the server
python serve.py

pause
