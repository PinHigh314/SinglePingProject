# PowerShell Serial Monitor for nRF54L15DK
# Usage: .\read_serial.ps1 [COM_PORT] [BAUD_RATE]
# Example: .\read_serial.ps1 COM3 115200

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "    nRF54L15DK Serial Monitor" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if COM port exists
$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
if ($availablePorts.Count -eq 0) {
    Write-Host "ERROR: No COM ports found!" -ForegroundColor Red
    Write-Host "Please check that your device is connected." -ForegroundColor Yellow
    exit 1
}

Write-Host "Available COM ports: $($availablePorts -join ', ')" -ForegroundColor Green

if ($ComPort -notin $availablePorts) {
    Write-Host "WARNING: $ComPort not found in available ports." -ForegroundColor Yellow
    Write-Host "Attempting to use it anyway..." -ForegroundColor Yellow
}

try {
    # Create and configure serial port
    $port = New-Object System.IO.Ports.SerialPort $ComPort, $BaudRate, None, 8, One
    $port.ReadTimeout = 1000
    $port.WriteTimeout = 1000
    
    Write-Host "Opening $ComPort at $BaudRate baud..." -ForegroundColor Green
    $port.Open()
    
    Write-Host "Serial monitor started. Press Ctrl+C to stop." -ForegroundColor Green
    Write-Host "----------------------------------------" -ForegroundColor Gray
    Write-Host ""
    
    # Create a timestamp function
    function Get-Timestamp {
        return (Get-Date).ToString("HH:mm:ss.fff")
    }
    
    # Read loop
    while ($true) {
        try {
            if ($port.BytesToRead -gt 0) {
                $line = $port.ReadLine()
                # Add timestamp and color coding based on log level
                $timestamp = Get-Timestamp
                
                if ($line -match "ERR|ERROR") {
                    Write-Host "[$timestamp] $line" -ForegroundColor Red
                } elseif ($line -match "WRN|WARNING") {
                    Write-Host "[$timestamp] $line" -ForegroundColor Yellow
                } elseif ($line -match "INF|INFO") {
                    Write-Host "[$timestamp] $line" -ForegroundColor Cyan
                } elseif ($line -match "DBG|DEBUG") {
                    Write-Host "[$timestamp] $line" -ForegroundColor Gray
                } else {
                    Write-Host "[$timestamp] $line"
                }
            }
        } catch [System.TimeoutException] {
            # Timeout is normal when no data is available
            Start-Sleep -Milliseconds 10
        } catch {
            # Handle other read errors
            if ($_.Exception.Message -notmatch "timeout") {
                Write-Host "Read error: $_" -ForegroundColor Red
            }
        }
        
        # Check if user pressed Ctrl+C
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq "Q" -or $key.Key -eq "Escape") {
                break
            }
        }
    }
} catch {
    Write-Host "ERROR: Failed to open serial port: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Possible causes:" -ForegroundColor Yellow
    Write-Host "  1. Device not connected" -ForegroundColor Yellow
    Write-Host "  2. Wrong COM port number" -ForegroundColor Yellow
    Write-Host "  3. Port already in use by another program" -ForegroundColor Yellow
    Write-Host "  4. Driver not installed" -ForegroundColor Yellow
    exit 1
} finally {
    if ($port -and $port.IsOpen) {
        Write-Host ""
        Write-Host "Closing serial port..." -ForegroundColor Yellow
        $port.Close()
        $port.Dispose()
    }
}

Write-Host "Serial monitor stopped." -ForegroundColor Green
