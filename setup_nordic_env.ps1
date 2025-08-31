# Nordic Connect SDK Environment Setup Script
# Run this script before building your Zephyr projects

Write-Host "Setting up Nordic Connect SDK environment variables..." -ForegroundColor Green

# Set Zephyr environment variables
$env:ZEPHYR_BASE = "C:\ncs\v3.1.0\zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk"

# Add Nordic toolchain to PATH
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;" + $env:PATH

# Verify the setup
Write-Host ""
Write-Host "Environment variables set:" -ForegroundColor Yellow
Write-Host "ZEPHYR_BASE: $env:ZEPHYR_BASE"
Write-Host "ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT"
Write-Host "ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR"
Write-Host ""
Write-Host "Nordic toolchain added to PATH" -ForegroundColor Green
Write-Host ""
Write-Host "You can now build your Zephyr projects!" -ForegroundColor Green
Write-Host ""
Write-Host "To make these permanent, add them to your system environment variables or run this script before each build session." -ForegroundColor Cyan

