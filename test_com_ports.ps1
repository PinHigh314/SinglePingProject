Write-Host "Testing COM port detection..."
Write-Host ""

Write-Host "All COM ports:"
Get-WmiObject -Query 'SELECT * FROM Win32_SerialPort' | Select-Object DeviceID, Description

Write-Host ""
Write-Host "First COM port DeviceID:"
Get-WmiObject -Query 'SELECT * FROM Win32_SerialPort' | Select-Object -First 1 -ExpandProperty DeviceID

Write-Host ""
Write-Host "JLink ports:"
Get-WmiObject -Query 'SELECT * FROM Win32_SerialPort WHERE Description LIKE "%JLink%"' | Select-Object DeviceID, Description
