# Build script for v6 using Invoke-Expression approach that has worked before

Write-Host "Building Host Device Test Firmware v6 (Alternating RSSI)..." -ForegroundColor Cyan

# First copy the v6 files
Write-Host "Installing v6 source files..." -ForegroundColor Yellow
Copy-Item "src/main_test_fixed_rssi_v6.c" "src/main.c" -Force
Copy-Item "src/ble/ble_central_test.c" "src/ble/ble_central.c" -Force

# Now invoke the regular build script
Write-Host "Running build using Invoke-Expression..." -ForegroundColor Yellow
Invoke-Expression (Get-Content ./build.ps1 -Raw)

# After build completes, copy and rename the output
if (Test-Path "build/zephyr/zephyr.hex") {
    $timestamp = Get-Date -Format "yyyyMMdd"
    $outputName = "host_device_test_alternating_rssi_v6_${timestamp}_rev019.hex"
    $destPath = "../../compiled_code/$outputName"
    
    Copy-Item "build/zephyr/zephyr.hex" $destPath -Force
    Write-Host ""
    Write-Host "===== v6 Build Complete =====" -ForegroundColor Green
    Write-Host "Output: $destPath" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Features:" -ForegroundColor Cyan
    Write-Host "- Alternates: Fixed -55 (LED3 flash) / Real RSSI (LED2 flash)"
    Write-Host "- LED1: MotoApp connection status"
    Write-Host ""
    Write-Host "To flash: nrfjprog --program compiled_code/$outputName --chiperase --verify -r" -ForegroundColor Yellow
}
