# SinglePing Mipe Device Build Script
# PowerShell script for building Mipe firmware

Write-Host "===== SinglePing Mipe Device Build Script =====" -ForegroundColor Cyan
Write-Host ""

# Set environment variables
Write-Host "Setting up environment variables..." -ForegroundColor Yellow
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:PATH = "C:/ncs/toolchains/b8b84efebd/opt/bin;C:/ncs/toolchains/b8b84efebd/opt/bin/Scripts;" + $env:PATH

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE"
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR"
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT"
Write-Host ""

Write-Host "Working directory: $PWD"
Write-Host ""

# Check for clean build parameter
$cleanBuild = $false
if ($args -contains "--clean" -or $args -contains "-c") {
    $cleanBuild = $true
}

# Clean build directory if requested
if ($cleanBuild) {
    Write-Host "Clean build requested..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Write-Host "Removing existing build directory..." -ForegroundColor Yellow
        Remove-Item -Path "build" -Recurse -Force
    }
}

# Configure with CMake
Write-Host "Configuring project with CMake..." -ForegroundColor Yellow
Write-Host "Board: nrf54l15dk/nrf54l15/cpuapp"
Write-Host ""

$cmakeCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Write-Host "Running: $cmakeCmd" -ForegroundColor Gray
& cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# Build with Ninja
Write-Host "Building project with Ninja..." -ForegroundColor Yellow
Write-Host "Running: ninja -C build" -ForegroundColor Gray
& ninja -C build

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "===== Build Completed Successfully! =====" -ForegroundColor Green
Write-Host ""

# Display build artifacts
if (Test-Path "build/zephyr/zephyr.hex") {
    $hexFile = Get-Item "build/zephyr/zephyr.hex"
    Write-Host "Build artifacts:" -ForegroundColor Cyan
    Write-Host "  HEX file: $($hexFile.FullName)"
    Write-Host "  Size: $([math]::Round($hexFile.Length / 1KB, 1)) KB"
}

if (Test-Path "build/zephyr/zephyr.elf") {
    $elfFile = Get-Item "build/zephyr/zephyr.elf"
    Write-Host "  ELF file: $($elfFile.FullName)"
    Write-Host "  Size: $([math]::Round($elfFile.Length / 1KB, 1)) KB"
}

Write-Host ""
Write-Host "Memory usage report should be available above in the build output."
Write-Host ""
Write-Host "To flash the device, use:"
Write-Host "  nrfjprog --program build/zephyr/zephyr.hex --chiperase --verify -r"
Write-Host "Or use the nRF Connect Programmer app"
Write-Host ""
