@echo off
setlocal

:: ====================================================================
:: nRF54L15 Deep Recover Script
:: This script uses the --recover command for the deepest possible clean.
:: ====================================================================

echo.
echo ========================================
echo      nRF54L15 Deep Recover Utility
echo ========================================
echo.
echo This script will perform a full RECOVER operation on the board.
echo This is the deepest clean possible and will unlock a secured device.
echo.

:: Check if nrfjprog is available in the system's PATH
where nrfjprog >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: 'nrfjprog' command not found.
    echo Please try restarting your computer if you just installed the tools.
    echo.
    pause
    exit /b 1
)

echo nrfjprog command found successfully!
echo.
echo Checking for connected J-Link devices...
nrfjprog --ids >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: No J-Link devices found.
    echo Please connect your nRF54L15 Development Kit.
    echo.
    pause
    exit /b 1
)

echo.
echo A connected device has been found.
echo.
echo Press any key to begin the DEEP RECOVER process...
echo You will be prompted by nrfjprog to confirm the operation.
pause >nul

:: Execute the recover command
echo.
echo Starting the recover process...
nrfjprog --recover

if %errorlevel% neq 0 (
    echo.
    echo ERROR: The recover operation failed.
) else (
    echo.
    echo SUCCESS: The nRF54L15 has been recovered and is completely blank.
)

echo.
pause
