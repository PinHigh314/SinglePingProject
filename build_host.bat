@echo off
echo ========================================
echo    SinglePing Project - Build Host Device
echo ========================================
echo.

echo Setting up Nordic Connect SDK environment...
echo.

REM Set environment variables for nRF Connect SDK v3.1.0
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk

REM Add Nordic toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo.

echo ========================================
echo Building HOST DEVICE...
echo ========================================
echo.

cd Host\host_device
echo Working directory: %CD%
echo.

REM Run Host device build
powershell -ExecutionPolicy Bypass -File "build_west.ps1"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Host device build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Host Device Build: SUCCESS
echo ========================================
echo.

echo Build artifacts created:
echo   - build/host_device/zephyr/zephyr.hex
echo   - build/host_device/zephyr/zephyr.elf
echo.

echo To flash the Host device:
echo   nrfjprog --program build\host_device\zephyr\zephyr.hex --chiperase --verify -r
echo.

pause
