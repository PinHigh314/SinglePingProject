@echo off
setlocal

echo ========================================
echo SinglePing Zephyr SDK Migration Script
echo ========================================
echo.
echo This script will help you move the Zephyr SDK from:
echo   C:\Development\SinglePingProject\sdk-zephyr
echo to:
echo   C:\zephyr
echo.

:: Check if source exists
if not exist "%~dp0sdk-zephyr" (
    echo ERROR: sdk-zephyr directory not found in current project
    echo Please run this script from the project root directory
    pause
    exit /b 1
)

:: Check if destination already exists
if exist "C:\zephyr" (
    echo WARNING: C:\zephyr already exists!
    echo.
    set /p confirm="Do you want to overwrite it? (y/N): "
    if /i not "%confirm%"=="y" (
        echo Migration cancelled.
        pause
        exit /b 0
    )
    echo Removing existing C:\zephyr...
    rmdir /s /q "C:\zephyr"
)

echo.
echo Starting migration...
echo Source: %~dp0sdk-zephyr
echo Destination: C:\zephyr
echo.

:: Create destination directory
if not exist "C:\" (
    echo ERROR: Cannot access C:\ drive
    pause
    exit /b 1
)

:: Move the directory
echo Moving Zephyr SDK...
robocopy "%~dp0sdk-zephyr" "C:\zephyr" /E /MOVE /R:3 /W:10

if %errorlevel% geq 8 (
    echo ERROR: Migration failed with error code %errorlevel%
    echo Please check permissions and try again
    pause
    exit /b 1
)

echo.
echo Migration completed successfully!
echo.

:: Verify the move
if exist "C:\zephyr" (
    echo ✓ Zephyr SDK found at C:\zephyr
    if exist "C:\zephyr\CMakeLists.txt" (
        echo ✓ CMakeLists.txt found - SDK appears complete
    ) else (
        echo ⚠ Warning: CMakeLists.txt not found - SDK may be incomplete
    )
) else (
    echo ERROR: Migration verification failed
    pause
    exit /b 1
)

:: Check if source was removed
if not exist "%~dp0sdk-zephyr" (
    echo ✓ Source directory removed successfully
) else (
    echo ⚠ Warning: Source directory still exists - manual cleanup may be needed
)

echo.
echo Next steps:
echo 1. Run setup-zephyr-env.bat to configure environment variables
echo 2. Test a build with one of your build scripts
echo 3. Consider adding C:\zephyr to your system PATH permanently
echo.
echo Migration complete!
pause
