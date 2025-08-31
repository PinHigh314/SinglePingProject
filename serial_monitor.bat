@echo off
echo ========================================
echo    nRF54L15DK Serial Monitor Launcher
echo ========================================
echo.

echo Checking for connected nRF54L15DK devices...
echo.

:: Try to find JLink CDC UART ports
set "COMPORT="
for /f "tokens=*" %%i in ('wmic path Win32_SerialPort where "Description like '%%JLink%%'" get DeviceID /value ^| find "DeviceID="') do (
    set "COMPORT=%%i"
    set "COMPORT=!COMPORT:DeviceID=!"
)

if defined COMPORT (
    echo Found nRF54L15DK on %COMPORT%
    echo.
    echo Starting serial monitor...
    echo.
    echo Options:
    echo 1. Use PuTTY (recommended)
    echo 2. Use PowerShell script
    echo 3. Manual COM port selection
    echo.
    set /p "choice=Enter your choice (1-3): "
    
    if "!choice!"=="1" (
        echo Starting PuTTY on %COMPORT% at 115200 baud...
        echo If PuTTY is not installed, please install it from: https://www.putty.org/
        echo.
        putty -serial %COMPORT% -sercfg 115200,8,n,1,N
    ) else if "!choice!"=="2" (
        echo Starting PowerShell serial monitor...
        powershell -ExecutionPolicy Bypass -File read_serial.ps1 %COMPORT% 115200
    ) else if "!choice!"=="3" (
        echo.
        echo Available COM ports:
        wmic path Win32_SerialPort get DeviceID, Description
        echo.
        set /p "manual_port=Enter COM port (e.g., COM3): "
        if defined manual_port (
            echo Starting PuTTY on !manual_port! at 115200 baud...
            putty -serial !manual_port! -sercfg 115200,8,n,1,N
        ) else (
            echo No port specified.
        )
    ) else (
        echo Invalid choice.
    )
) else (
    echo No JLink CDC UART port found automatically.
    echo.
    echo Available COM ports:
    wmic path Win32_SerialPort get DeviceID, Description
    echo.
    echo Please select a COM port manually:
    set /p "manual_port=Enter COM port (e.g., COM3): "
    if defined manual_port (
        echo Starting PuTTY on !manual_port! at 115200 baud...
        echo If PuTTY is not installed, please install it from: https://www.putty.org/
        echo.
        putty -serial !manual_port! -sercfg 115200,8,n,1,N
    ) else (
        echo No port specified.
    )
)

echo.
echo Serial monitor closed.
pause
