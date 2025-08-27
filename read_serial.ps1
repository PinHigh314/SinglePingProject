$port = New-Object System.IO.Ports.SerialPort('COM3', 115200, 'None', 8, 'One')
$port.Open()
Write-Host "Connected to COM3 at 115200 baud"
Write-Host "Waiting for serial data..."

try {
    while($true) {
        if($port.BytesToRead -gt 0) {
            $line = $port.ReadLine()
            Write-Host $line
        }
        Start-Sleep -Milliseconds 100
    }
}
finally {
    $port.Close()
}
