# Build script for SinglePing Host Device
# Based on BuildExperience.md lessons

Write-Host "===== SinglePing Host Device Build Script =====" -ForegroundColor Cyan
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

# Check if clean build is requested
$cleanBuild = $false
if ($args -contains "--clean" -or $args -contains "-c") {
    $cleanBuild = $true
    Write-Host "Clean build requested..." -ForegroundColor Yellow
}

# Navigate to the script directory (host_device folder)
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Cyan
Write-Host ""

# Clean build directory if requested or if it doesn't exist properly
if ($cleanBuild -or !(Test-Path "build/CMakeCache.txt")) {
    if (Test-Path "build") {
        Write-Host "Removing existing build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build"
    }
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

# Display build artifacts
if (Test-Path "build/zephyr/zephyr.hex") {
    $hexFile = Get-Item "build/zephyr/zephyr.hex"
    Write-Host "Build artifacts:" -ForegroundColor Cyan
    Write-Host "  HEX file: $($hexFile.FullName)" -ForegroundColor White
    Write-Host "  Size: $([math]::Round($hexFile.Length / 1KB, 2)) KB" -ForegroundColor White
}

if (Test-Path "build/zephyr/zephyr.elf") {
    $elfFile = Get-Item "build/zephyr/zephyr.elf"
    Write-Host "  ELF file: $($elfFile.FullName)" -ForegroundColor White
    Write-Host "  Size: $([math]::Round($elfFile.Length / 1KB, 2)) KB" -ForegroundColor White
}

Write-Host ""
Write-Host "Memory usage report should be available above in the build output." -ForegroundColor Yellow
Write-Host ""
Write-Host "To flash the device, use:" -ForegroundColor Cyan
Write-Host "  nrfjprog --program build/zephyr/zephyr.hex --chiperase --verify -r" -ForegroundColor White
Write-Host "Or use the nRF Connect Programmer app" -ForegroundColor White
Write-Host ""
