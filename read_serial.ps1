param(
    [string]$comPort = "COM3",
    [int]$baudRate = 115200
)

Write-Host "========================================"
Write-Host "   nRF54L15DK Serial Monitor Launcher"
Write-Host "========================================"
Write-Host ""

# Check if the specified COM port exists
$availablePorts = [System.IO.Ports.SerialPort]::getportnames()
Write-Host "Available COM ports: $($availablePorts -join ', ')"

if ($availablePorts -notcontains $comPort) {
    Write-Host "WARNING: Port $comPort not found in available ports!"
    
    # Try to find alternative JLink ports
    $jlinkPorts = $availablePorts | Where-Object { $_ -match "^COM[0-9]+$" }
    if ($jlinkPorts.Count -gt 0) {
        $alternativePort = $jlinkPorts[0]
        Write-Host "Trying alternative port: $alternativePort"
        $comPort = $alternativePort
    } else {
        Write-Host "ERROR: No available COM ports found!"
        Write-Host "Please check your nRF54L15DK connection and try again."
        exit 1
    }
}

Write-Host "Using port: $comPort"
Write-Host "Baud rate: $baudRate"
Write-Host ""

try {
    $port = New-Object System.IO.Ports.SerialPort($comPort, $baudRate, 'None', 8, 'One')
    $port.Open()
    Write-Host "Connected to $comPort at $baudRate baud"
    Write-Host "Waiting for serial data..."
    Write-Host "Press Ctrl+C to stop"
    Write-Host "----------------------------------------"

    while($true) {
        if($port.BytesToRead -gt 0) {
            try {
                $line = $port.ReadLine()
                Write-Host $line
            } catch {
                # Handle read errors (timeouts, etc.)
                Start-Sleep -Milliseconds 100
            }
        }
        Start-Sleep -Milliseconds 100
    }
}
catch [System.IO.IOException] {
    Write-Host "ERROR: Failed to open port $comPort"
    Write-Host "The port may be in use by another application or the device is disconnected."
    Write-Host "Exception: $($_.Exception.Message)"
    exit 1
}
catch {
    Write-Host "ERROR: Unexpected error: $($_.Exception.Message)"
    exit 1
}
finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "Serial port closed."
    }
}
