@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    Simple UART Monitor for nRF54L15DK
echo ========================================
echo.

echo Available COM ports:
wmic path Win32_SerialPort get DeviceID, Description
echo.

set /p "comport=Enter COM port (e.g., COM3): "
if not defined comport (
    echo No port specified. Exiting.
    pause
    exit /b 1
)

echo.
echo Starting UART monitor on !comport! at 115200 baud...
echo.
echo This will open a simple serial connection.
echo Press Ctrl+C to stop the connection.
echo.

:: Try to use mode command to set up the port
mode !comport! BAUD=115200 PARITY=N DATA=8 STOP=1

:: Use copy command to read from serial port
echo Reading from !comport!... (Press Ctrl+C to stop)
copy !comport! CON: >nul 2>&1

echo.
echo UART connection closed.
pause
