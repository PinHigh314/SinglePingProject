@echo off
echo ========================================
echo    SinglePing Project - Build Mipe Device
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
echo Building MIPE DEVICE...
echo ========================================
echo.

cd Mipe\mipe_device
echo Working directory: %CD%
echo.

REM Run Mipe device build
powershell -ExecutionPolicy Bypass -File "build.ps1"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Mipe device build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Mipe Device Build: SUCCESS
echo ========================================
echo.

echo Build artifacts created:
echo   - build/zephyr/zephyr.hex
echo   - build/zephyr/zephyr.elf
echo.

echo To flash the Mipe device:
echo   nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r
echo.

pause
