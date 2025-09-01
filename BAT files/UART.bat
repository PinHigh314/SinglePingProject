@echo off
echo ========================================
echo    nRF54L15DK Serial Monitor
echo ========================================
echo.

echo Starting serial monitor on COM3 at 115200 baud...
echo Press Ctrl+C to stop
echo.

:: Use the existing PowerShell script with COM3 (nRF54L15DK default)
powershell -ExecutionPolicy Bypass -File "%~dp0..\read_serial.ps1" COM3 115200

pause
