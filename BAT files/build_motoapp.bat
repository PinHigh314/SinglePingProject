@echo off
setlocal

echo ========================================
echo SinglePing MotoApp Build Script
echo ========================================
echo.

:: Navigate to MotoApp directory
cd /d "%~dp0..\MotoApp"
echo Working directory: %CD%
echo.

:: Build with Gradle
echo Building project with Gradle...
echo Running: gradlew clean assembleDebug signingReport --no-build-cache
call gradlew clean assembleDebug signingReport --no-build-cache

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

REM Ensure 2-digit format for all components
if "%YY%"=="" set "YY=25"
if "%MM%"=="" set "MM=12"
if "%DD%"=="" set "DD=31"
if "%HH%"=="" set "HH=23"
if "%Min%"=="" set "Min=59"

REM Ensure single digits are padded with leading zero
if "%YY:~1%"=="" set "YY=0%YY%"
if "%MM:~1%"=="" set "MM=0%MM%"
if "%DD:~1%"=="" set "DD=0%DD%"
if "%HH:~1%"=="" set "HH=0%HH%"
if "%Min:~1%"=="" set "Min=0%Min%"

REM Create timestamp (YYMMDD_HHMM)
set "timestamp=%YY%%MM%%DD%_%HH%%Min%"

:: Create output filename
set output_name=MotoApp_%timestamp%.apk

:: Copy apk file to compiled_code directory
echo Copying apk file to compiled_code directory...
if not exist "%~dp0..\compiled_code" mkdir "%~dp0..\compiled_code"
copy app\build\outputs\apk\debug\app-debug.apk "%~dp0..\compiled_code\%output_name%" >nul

if %errorlevel% equ 0 (
    echo Apk file copied to: compiled_code\%output_name%
    
    :: Update version log
    echo %date% %time% - Built: %output_name% >> "%~dp0..\compiled_code\version_log.md"
) else (
    echo Warning: Failed to copy apk file to compiled_code directory
)

:: Copy apk file to My Drive directory
echo Copying apk file to My Drive directory...
copy app\build\outputs\apk\debug\app-debug.apk "C:\Users\%USERNAME%\Google Drive\My Drive\%output_name%" >nul

if %errorlevel% equ 0 (
    echo Apk file copied to: C:\Users\%USERNAME%\Google Drive\My Drive\%output_name%
) else (
    echo Warning: Failed to copy apk file to My Drive directory
)

echo.
pause
exit
