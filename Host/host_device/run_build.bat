@echo off
echo Running Host Device Build with PowerShell...
echo.

REM Run PowerShell with execution policy bypass
powershell -ExecutionPolicy Bypass -File "build.ps1"

echo.
echo Build script completed.
pause

