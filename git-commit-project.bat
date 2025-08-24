@echo off
echo ========================================
echo SinglePing Project Git Commit Script
echo ========================================

REM Check if we're in a git repository
git status >nul 2>&1
if errorlevel 1 (
    echo Error: Not a git repository or git not installed
    echo Please initialize git first: git init
    pause
    exit /b 1
)

REM Get commit message from user if not provided
set /p "commit_msg=Enter commit message (or press Enter for default): "
if "%commit_msg%"=="" (
    set "commit_msg=Project update - %date% %time%"
)

REM Add all changes (including deletions and new files)
echo Adding all changes to git...
git add --all

REM Commit with message
echo Committing changes...
git commit -m "%commit_msg%"

REM Show status after commit
echo.
echo Commit completed. Current status:
git status --short

echo.
echo To push to remote repository, run: git push origin main
pause
