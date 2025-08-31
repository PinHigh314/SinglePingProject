@echo off
setlocal

echo ========================================
echo SinglePing Zephyr Environment Setup
echo ========================================
echo.

:: Set Zephyr SDK location (external to project)
set ZEPHYR_BASE=C:\zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr

:: Add toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;%PATH%

:: Set additional useful environment variables
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk

echo Environment variables set:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo.
echo Toolchain added to PATH
echo.
echo To make these permanent, run this script before building,
echo or add these variables to your system environment.
echo.
echo Ready for Zephyr development!
echo.
pause
