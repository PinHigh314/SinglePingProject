# Build script for SinglePing Host Device TEST VERSION - Fixed RSSI
# This builds a test firmware that sends fixed RSSI = -55 dBm

Write-Host "===== SinglePing Host Device TEST BUILD (Fixed RSSI) =====" -ForegroundColor Cyan
Write-Host "This will build a test firmware that sends fixed RSSI = -55 dBm" -ForegroundColor Yellow
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

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE"
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR"
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT"
Write-Host ""

# Navigate to the script directory (host_device folder)
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Cyan
Write-Host ""

# Clean build directory
if (Test-Path "build") {
    Write-Host "Removing existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Configure with CMake
Write-Host "Configuring project with CMake..." -ForegroundColor Yellow
Write-Host "Board: nrf54l15dk/nrf54l15/cpuapp" -ForegroundColor Cyan
Write-Host ""

$configCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Write-Host "Running: $configCmd" -ForegroundColor Gray
$configResult = Invoke-Expression $configCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Write-Host $configResult
    exit 1
}

Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# Build with Ninja
Write-Host "Building project with Ninja..." -ForegroundColor Yellow
$buildCmd = "ninja -C build"
Write-Host "Running: $buildCmd" -ForegroundColor Gray
$buildResult = Invoke-Expression $buildCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Write-Host $buildResult
    exit 1
}

Write-Host ""
Write-Host "===== Build Completed Successfully! =====" -ForegroundColor Green
Write-Host ""

# Copy the hex file to compiled_code with proper naming convention
$timestamp = Get-Date -Format "yyyyMMdd"
$hexFile = "build/zephyr/zephyr.hex"
$outputFile = "../../compiled_code/host_device_test_fixed_rssi_${timestamp}_rev014.hex"

if (Test-Path $hexFile) {
    Copy-Item $hexFile $outputFile
    Write-Host "TEST Firmware saved to: $outputFile" -ForegroundColor Green
    
    # Display build artifacts
    $hexFileInfo = Get-Item $hexFile
    Write-Host ""
    Write-Host "Build artifacts:" -ForegroundColor Cyan
    Write-Host "  HEX file: $($hexFileInfo.FullName)" -ForegroundColor White
    Write-Host "  Size: $([math]::Round($hexFileInfo.Length / 1KB, 2)) KB" -ForegroundColor White
}

if (Test-Path "build/zephyr/zephyr.elf") {
    $elfFile = Get-Item "build/zephyr/zephyr.elf"
    Write-Host "  ELF file: $($elfFile.FullName)" -ForegroundColor White
    Write-Host "  Size: $([math]::Round($elfFile.Length / 1KB, 2)) KB" -ForegroundColor White
}

Write-Host ""
Write-Host "TEST FIRMWARE FEATURES:" -ForegroundColor Yellow
Write-Host "  - Sends fixed RSSI = -55 dBm every 1 second when streaming" -ForegroundColor White
Write-Host "  - LED3 stays solid ON during streaming (not toggling)" -ForegroundColor White
Write-Host "  - Does NOT require Mipe connection" -ForegroundColor White
Write-Host ""
Write-Host "To flash the device, use:" -ForegroundColor Cyan
Write-Host "  nrfjprog --program $outputFile --chiperase --verify -r" -ForegroundColor White
Write-Host "Or use the nRF Connect Programmer app" -ForegroundColor White
Write-Host ""
