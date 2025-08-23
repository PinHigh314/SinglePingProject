# Build script for test firmware with enhanced debugging (v3)
# This builds using the v3 test file with detailed logging

$ErrorActionPreference = "Stop"

# Get the current date and revision number
$date = Get-Date -Format "yyyyMMdd"
$revision = "016"  # v3 diagnostic build
$outputName = "host_device_test_diagnostic_v3_${date}_rev${revision}"

Write-Host "Building Host Device Test Firmware v3 (Diagnostic)..." -ForegroundColor Green
Write-Host "Output: $outputName.hex" -ForegroundColor Yellow
Write-Host ""
Write-Host "Enhanced debugging features:" -ForegroundColor Cyan
Write-Host "- Detailed logging of every notification attempt" -ForegroundColor White
Write-Host "- Status reports every 5 seconds" -ForegroundColor White
Write-Host "- Warning if streaming but no packets sent" -ForegroundColor White
Write-Host "- Complete error code analysis" -ForegroundColor White
Write-Host ""

# Set environment variables for nRF Connect SDK v3.1.0
Write-Host "Setting up environment variables..." -ForegroundColor Yellow
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Add toolchain to PATH
$toolchainPath = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts"
if ($env:PATH -notlike "*$toolchainPath*") {
    $env:PATH = "$toolchainPath;$env:PATH"
}

# Clean build directory
Write-Host "Cleaning build directory..." -ForegroundColor Cyan
if (Test-Path "build") {
    Remove-Item -Path "build" -Recurse -Force
}

# Backup current main.c
Write-Host "Backing up current main.c..." -ForegroundColor Cyan
Copy-Item -Path "src/main.c" -Destination "src/main.c.backup" -Force

# Replace with v3 test version
Write-Host "Installing v3 diagnostic version..." -ForegroundColor Cyan
Copy-Item -Path "src/main_test_fixed_rssi_v3.c" -Destination "src/main.c" -Force

# Build with the v3 source
Write-Host "Configuring with CMake..." -ForegroundColor Cyan
$configCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Write-Host "Running: $configCmd" -ForegroundColor Gray
$configResult = Invoke-Expression $configCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Write-Host $configResult
    # Restore original main.c
    Copy-Item -Path "src/main.c.backup" -Destination "src/main.c" -Force
    Remove-Item -Path "src/main.c.backup" -Force
    exit 1
}

Write-Host "CMake configuration successful!" -ForegroundColor Green

# Build with Ninja
Write-Host "Building with Ninja..." -ForegroundColor Cyan
$buildCmd = "ninja -C build"
Write-Host "Running: $buildCmd" -ForegroundColor Gray
$buildResult = Invoke-Expression $buildCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Write-Host $buildResult
    # Restore original main.c
    Copy-Item -Path "src/main.c.backup" -Destination "src/main.c" -Force
    Remove-Item -Path "src/main.c.backup" -Force
    exit 1
}

# Restore original main.c
Write-Host "Restoring original main.c..." -ForegroundColor Cyan
Copy-Item -Path "src/main.c.backup" -Destination "src/main.c" -Force
Remove-Item -Path "src/main.c.backup" -Force

# Check if hex file was created
$hexPath = "build/zephyr/zephyr.hex"
if (Test-Path $hexPath) {
    # Copy to compiled_code with descriptive name
    $destPath = "../../compiled_code/$outputName.hex"
    Copy-Item -Path $hexPath -Destination $destPath -Force
    
    # Get file size
    $fileSize = (Get-Item $destPath).Length / 1KB
    
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "Firmware saved to: $destPath" -ForegroundColor Yellow
    Write-Host "File size: $([math]::Round($fileSize, 2)) KB" -ForegroundColor Yellow
    Write-Host "`nDiagnostic Features:" -ForegroundColor Cyan
    Write-Host "- Fixed RSSI value: -55 dBm" -ForegroundColor White
    Write-Host "- Transmission rate: 1 second" -ForegroundColor White
    Write-Host "- LED3: Solid ON during streaming" -ForegroundColor White
    Write-Host "- Monitor timer ensures LED3 turns OFF" -ForegroundColor White
    Write-Host "- Debug status printed every 5 seconds" -ForegroundColor White
    Write-Host "- Detailed logging of notification process" -ForegroundColor White
    Write-Host "`nThis version will help diagnose why data isn't flowing" -ForegroundColor Yellow
    Write-Host "Watch the debug logs to see where the process fails" -ForegroundColor Yellow
    Write-Host "`nTo flash the device, use:" -ForegroundColor Cyan
    Write-Host "  nrfjprog --program $destPath --chiperase --verify -r" -ForegroundColor White
} else {
    Write-Host "Build failed - hex file not found!" -ForegroundColor Red
    exit 1
}

Write-Host "`nDone!" -ForegroundColor Green
