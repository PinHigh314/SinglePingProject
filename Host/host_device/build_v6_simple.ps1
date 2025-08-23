# Simple build script for v6 - no path complications

Write-Host "Building Host Device Test Firmware v6..." -ForegroundColor Cyan

# First, copy the v6 files into place
Copy-Item "src/main_test_fixed_rssi_v6.c" "src/main.c" -Force
Copy-Item "src/ble/ble_central_test.c" "src/ble/ble_central.c" -Force

# Clean build directory
if (Test-Path "build") {
    Remove-Item -Path "build" -Recurse -Force
}

# Run cmake with full path
& "C:/ncs/toolchains/b8b84efebd/opt/bin/cmake.exe" -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if ($LASTEXITCODE -eq 0) {
    Write-Host "CMake configuration successful!" -ForegroundColor Green
    
    # Build with ninja
    & "C:/ncs/toolchains/b8b84efebd/opt/bin/ninja.exe" -C build
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build successful!" -ForegroundColor Green
        
        # Copy to compiled_code
        $timestamp = Get-Date -Format "yyyyMMdd"
        $outputName = "host_device_test_alternating_rssi_v6_${timestamp}_rev019.hex"
        Copy-Item "build/zephyr/zephyr.hex" "../../compiled_code/$outputName" -Force
        
        Write-Host ""
        Write-Host "===== Build Complete =====" -ForegroundColor Green
        Write-Host "Output: compiled_code/$outputName" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Features:" -ForegroundColor Cyan
        Write-Host "- Alternates: Fixed -55 (LED3) / Real RSSI (LED2)"
        Write-Host "- LED1: MotoApp connection"
    }
}
