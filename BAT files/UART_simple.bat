@echo off
echo ========================================
echo    Simple UART Monitor
echo ========================================
echo.

echo Starting serial monitor on COM3...
echo Press Ctrl+C to stop
echo.

:: Try using mode command first to check if COM3 exists
mode COM3 >nul 2>&1
if errorlevel 1 (
    echo ERROR: COM3 not available
    echo Available ports:
    mode | findstr "COM"
    pause
    exit /b 1
)

:: Use a simple approach with PowerShell
powershell -Command "try { $port = [System.IO.Ports.SerialPort]::new('COM3', 115200); $port.Open(); Write-Host 'Connected to COM3'; while($true) { if($port.BytesToRead) { Write-Host $port.ReadLine() } Start-Sleep -Milliseconds 100 } } catch { Write-Host 'Error: ' $_.Exception.Message }"

pause

