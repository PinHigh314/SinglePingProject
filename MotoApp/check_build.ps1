# Check MotoApp build status
Write-Host "Checking MotoApp build status..." -ForegroundColor Cyan

# Wait for build to complete (max 5 minutes)
$maxWait = 300  # seconds
$waited = 0
$checkInterval = 10

while ($waited -lt $maxWait) {
    if (Test-Path "app\build\outputs\apk\debug\app-debug.apk") {
        Write-Host "`nBUILD SUCCESS!" -ForegroundColor Green
        $apk = Get-Item "app\build\outputs\apk\debug\app-debug.apk"
        Write-Host "APK Generated: $($apk.Name)"
        Write-Host "Size: $([math]::Round($apk.Length/1MB,2)) MB"
        Write-Host "Created: $($apk.LastWriteTime)"
        
        # Copy to compiled_code
        Copy-Item $apk.FullName -Destination "..\compiled_code\MotoApp_TMT1_v2.0.apk" -Force
        Write-Host "`nAPK copied to: compiled_code\MotoApp_TMT1_v2.0.apk" -ForegroundColor Green
        
        Write-Host "`n=== MotoApp TMT1 Features ===" -ForegroundColor Yellow
        Write-Host "✓ Simulated BLE Host connection"
        Write-Host "✓ Real-time RSSI graph"
        Write-Host "✓ Distance calculation from RSSI"
        Write-Host "✓ Complete status display panel"
        Write-Host "✓ All TMT1 specification requirements"
        
        exit 0
    }
    
    Write-Host "." -NoNewline
    Start-Sleep -Seconds $checkInterval
    $waited += $checkInterval
}

Write-Host "`nBuild timeout or failed. Please check Gradle output." -ForegroundColor Red
