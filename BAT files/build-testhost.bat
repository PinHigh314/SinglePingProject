@echo off
setlocal

echo ========================================
echo SinglePing TestHost Device Build Script
echo Using stable cmake/ninja build process
echo ========================================
echo.
echo Usage: build-testhost.bat [options] [description]
echo   Options:
echo     --no-clean or -n : Skip clean build (incremental build)
echo   Description:
echo     Optional description for the output hex file
echo     Example: build-testhost.bat v1_initial
echo     Example: build-testhost.bat --no-clean fast_build
echo.

:: Set environment variables for nRF Connect SDK v3.1.0
echo Setting up environment variables...
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr

:: Add toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo.

:: Check for clean build flag - default to clean build for reliability
set clean_build=1
if "%1"=="--no-clean" set clean_build=0
if "%1"=="-n" set clean_build=0

:: Navigate to testhost_device directory (from BAT files folder)
cd /d "%~dp0..\TestHost\testhost_device"
echo Working directory: %CD%
echo.

:: Check if CMakeLists.txt exists
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in %CD%
    echo Please ensure you're in the correct directory
    pause
    exit /b 1
)

:: Clean build directory if requested
if %clean_build%==1 (
    echo Clean build requested...
    if exist build (
        echo Removing existing build directory...
        rmdir /s /q build
    )
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

echo Running: cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

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
echo Running: ninja -C build
ninja -C build

if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Completed Successfully!
echo ========================================
echo.

:: Display build artifacts
if exist build\zephyr\zephyr.hex (
    echo Build artifacts:
    echo   HEX file: %CD%\build\zephyr\zephyr.hex
    for %%F in (build\zephyr\zephyr.hex) do echo   Size: %%~zF bytes
)

if exist build\zephyr\zephyr.elf (
    echo   ELF file: %CD%\build\zephyr\zephyr.elf
    for %%F in (build\zephyr\zephyr.elf) do echo   Size: %%~zF bytes
)

echo.
echo To flash the device, use:
echo   nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r
echo   (This performs a complete chip erase before programming)
echo Or use the nRF Connect Programmer app
echo.

:: Get description from command line argument or use default
set description=%2
if "%description%"=="" set description=build

:: Get current date and time for versioning with proper formatting
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value 2^>nul') do set "dt=%%a"
if "%dt%"=="" (
    REM Fallback to PowerShell if wmic not available
    for /f "tokens=1-5 delims=: " %%a in ('powershell -Command "Get-Date -Format 'yy MM dd HH mm'"') do (
        set "YY=%%a"
        set "MM=%%b"
        set "DD=%%c"
        set "HH=%%d"
        set "Min=%%e"
    )
) else (
    set "YY=%dt:~2,2%"
    set "MM=%dt:~4,2%"
    set "DD=%dt:~6,2%"
    set "HH=%dt:~8,2%"
    set "Min=%dt:~10,2%"
)

REM Ensure 2-digit minutes
if "%Min%"=="" set "Min=00"
if "%Min:~1,1%"=="" set "Min=0%Min%"

REM Create timestamp (YYMMDD_HHMM)
set "timestamp=%YY%%MM%%DD%_%HH%%Min%"

:: Create output filename
set output_name=TestHost_%timestamp%.hex

:: Copy hex file to compiled_code directory
echo Copying hex file to compiled_code directory...
if not exist "%~dp0..\compiled_code" mkdir "%~dp0..\compiled_code"
copy build\zephyr\zephyr.hex "%~dp0..\compiled_code\%output_name%" >nul

if %errorlevel% equ 0 (
    echo Hex file copied to: ..\compiled_code\%output_name%
    
    :: Update version log
    echo %date% %time% - Built: %output_name% >> "%~dp0..\compiled_code\version_log.md"
) else (
    echo Warning: Failed to copy hex file to compiled_code directory
)

echo.
pause
