param(
    [string]$comPort = "COM3",
    [int]$baudRate = 115200
)

$port = New-Object System.IO.Ports.SerialPort($comPort, $baudRate, 'None', 8, 'One')
$port.Open()
Write-Host "Connected to $comPort at $baudRate baud"
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
