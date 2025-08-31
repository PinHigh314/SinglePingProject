@echo off
setlocal

echo ========================================
echo SinglePing Host Device Build Script
echo Using proven Direct CMake + Ninja method
echo ========================================
echo.
echo Usage: build_host_new.bat [options] [description]
echo   Options:
echo     --no-clean : Skip cleaning build directory
echo   Description:
echo     Optional description for the output hex file
echo     Example: build_host_new.bat tmt1_rssi_collection
echo     Example: build_host_new.bat --no-clean ble_dual_role_test
echo.

:: Set environment variables for nRF Connect SDK v3.1.0
echo Setting up environment variables...
set ZEPHYR_BASE=C:/ncs/v3.1.0/zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr

:: Add toolchain to PATH
set PATH=C:/ncs/toolchains/b8b84efebd/opt/bin;C:/ncs/toolchains/b8b84efebd/opt/bin/Scripts;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo.

:: Check for no-clean flag (clean is now default)
set clean_build=1
if "%1"=="--no-clean" set clean_build=0

:: Navigate to Host device directory (from BAT files folder)
cd /d "%~dp0..\Host\host_device"
echo Working directory: %CD%
echo.

:: Check if CMakeLists.txt exists
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in %CD%
    echo Please ensure you're in the correct directory
    pause
    exit /b 1
)

:: Clean build directory by default (unless --no-clean specified)
if %clean_build%==1 (
    echo Clean build enabled by default...
    if exist build (
        echo Removing existing build directory...
        rmdir /s /q build
    )
) else (
    echo Clean build disabled (--no-clean specified)
)

:: Check if build directory needs to be created
if not exist build\CMakeCache.txt (
    if exist build (
        echo Build directory exists but no CMakeCache.txt found, cleaning...
        rmdir /s /q build
    )
)

:: Configure with CMake
echo Configuring project with CMake...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo.

echo Running: cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DZEPHYR_BASE="%ZEPHYR_BASE%"
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DZEPHYR_BASE="%ZEPHYR_BASE%"

if %errorlevel% neq 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

:: Build with Ninja
echo Building project with Ninja...
echo Running: cmake --build build
cmake --build build

if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Output files:
echo   - ELF: build/zephyr/zephyr.elf
echo   - HEX: build/zephyr/zephyr.hex
echo   - BIN: build/zephyr/zephyr.bin
echo.

:: Copy hex file to compiled_code directory with timestamp
set timestamp=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set timestamp=%timestamp: =0%

if exist "%~dp0..\compiled_code" (
    set output_name=host_device_tmt1_%timestamp%.hex
    echo Copying hex file to compiled_code as: %output_name%
    copy "build\zephyr\zephyr.hex" "%~dp0..\compiled_code\%output_name%"
    
    if %errorlevel%==0 (
        echo Successfully copied to: %output_name%
    ) else (
        echo Failed to copy hex file
    )
)

echo.
echo Build completed successfully!
echo Press any key to continue...
pause > nul
