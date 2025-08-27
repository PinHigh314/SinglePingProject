@echo off
echo ========================================
echo    nRF54L15DK Serial Monitor Launcher
echo ========================================
echo.

echo Checking for connected nRF54L15DK devices...
echo.

:: Use PowerShell to find JLink CDC UART ports
for /f "tokens=*" %%i in ('powershell -Command "Get-WmiObject -Query 'SELECT * FROM Win32_SerialPort WHERE Description LIKE \"%%JLink%%\"' | Select-Object -ExpandProperty DeviceID"') do (
    set "COMPORT=%%i"
)

if defined COMPORT (
    echo Found nRF54L15DK on %COMPORT%
    echo Starting serial monitor at 115200 baud...
    echo Press Ctrl+C to stop
    echo.
    powershell -ExecutionPolicy Bypass -File read_serial.ps1 %COMPORT% 115200
) else (
    echo ERROR: No nRF54L15DK found!
    echo.
    echo Please check:
    echo 1. Board is connected via USB
    echo 2. JLink drivers are installed
    echo 3. Board is powered on
    echo.
    echo Available COM ports:
    powershell -Command "Get-WmiObject -Query 'SELECT * FROM Win32_SerialPort' | Select-Object DeviceID, Description"
)

pause
