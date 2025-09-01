@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    nRF54L15DK Auto-Detection Monitor
echo ========================================
echo.
echo Board ID: 1057780836
echo Board Type: PCA10156 (nRF54L15DK)
echo.

:: Check for J-Link devices first
echo Checking for SEGGER J-Link devices...
nrfjprog --ids >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: nrfjprog not found or no J-Link devices detected!
    echo Please ensure:
    echo   1. nRF Command Line Tools are installed
    echo   2. Board is connected via USB
    echo   3. Drivers are properly installed
    pause
    exit /b 1
)

:: Get connected device IDs
for /f "tokens=*" %%i in ('nrfjprog --ids 2^>nul') do (
    set DEVICE_ID=%%i
    if "!DEVICE_ID!"=="1057780836" (
        echo [OK] Found target board: !DEVICE_ID!
        set BOARD_FOUND=1
    )
)

if not defined BOARD_FOUND (
    echo ERROR: Target board 1057780836 not found!
    echo.
    echo Connected devices:
    nrfjprog --ids
    echo.
    echo Please connect the correct board and try again.
    pause
    exit /b 1
)

:: Now detect which COM ports are available
echo.
echo Detecting COM ports for board 1057780836...

:: Create a temporary PowerShell script to detect SEGGER COM ports
echo $ports = Get-WmiObject Win32_PnPEntity ^| Where-Object { > temp_detect.ps1
echo     $_.Name -match "JLink CDC UART Port" -or >> temp_detect.ps1
echo     $_.Name -match "SEGGER" -or >> temp_detect.ps1
echo     $_.Name -match "J-Link" >> temp_detect.ps1
echo } >> temp_detect.ps1
echo. >> temp_detect.ps1
echo if ($ports) { >> temp_detect.ps1
echo     foreach ($port in $ports) { >> temp_detect.ps1
echo         if ($port.Name -match "COM\d+") { >> temp_detect.ps1
echo             $matches[0] >> temp_detect.ps1
echo         } >> temp_detect.ps1
echo     } >> temp_detect.ps1
echo } else { >> temp_detect.ps1
echo     # Fallback: list all COM ports >> temp_detect.ps1
echo     [System.IO.Ports.SerialPort]::GetPortNames() >> temp_detect.ps1
echo } >> temp_detect.ps1

:: Run the detection script
set COM_FOUND=
for /f "tokens=*" %%i in ('powershell -ExecutionPolicy Bypass -File temp_detect.ps1 2^>nul') do (
    set TEST_PORT=%%i
    echo Found port: !TEST_PORT!
    
    :: Try COM3 first (UART0), then COM4 (UART1)
    if "!TEST_PORT!"=="COM3" (
        set PRIMARY_PORT=!TEST_PORT!
        set COM_FOUND=1
    )
    if "!TEST_PORT!"=="COM4" (
        set SECONDARY_PORT=!TEST_PORT!
        set COM_FOUND=1
    )
)

:: Clean up temp file
del temp_detect.ps1 >nul 2>&1

if not defined COM_FOUND (
    echo ERROR: No SEGGER COM ports detected!
    echo.
    echo Available COM ports:
    powershell -Command "[System.IO.Ports.SerialPort]::GetPortNames()"
    echo.
    echo The board may not be properly connected or drivers not installed.
    pause
    exit /b 1
)

:: Determine which port to use
set USE_PORT=
if defined PRIMARY_PORT (
    set USE_PORT=!PRIMARY_PORT!
    echo.
    echo [OK] Using primary UART port: !USE_PORT! ^(UART0^)
)
if not defined USE_PORT if defined SECONDARY_PORT (
    set USE_PORT=!SECONDARY_PORT!
    echo.
    echo [OK] Using secondary UART port: !USE_PORT! ^(UART1^)
)
if not defined USE_PORT (
    echo ERROR: Could not determine which COM port to use!
    pause
    exit /b 1
)

:: Check if PowerShell monitoring script exists
if not exist "%~dp0..\read_serial.ps1" (
    echo ERROR: read_serial.ps1 not found!
    pause
    exit /b 1
)

:: Start monitoring
echo.
echo ========================================
echo Starting UART monitor for PCA10156
echo   Board ID: 1057780836
echo   COM Port: !USE_PORT!
echo   Baud Rate: 115200
echo.
echo Press Ctrl+C to stop monitoring
echo ========================================
echo.

:: Launch the serial monitor
powershell -ExecutionPolicy Bypass -File "%~dp0..\read_serial.ps1" !USE_PORT! 115200

if %ERRORLEVEL% neq 0 (
    echo.
    echo ========================================
    echo Failed to connect to !USE_PORT!
    echo The port may be in use by another program.
    echo ========================================
    pause
    exit /b 1
)

echo.
echo Session ended.
pause
