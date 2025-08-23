@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Git Commit Helper for SinglePingProject
echo ========================================
echo.

:: Show current branch
echo Current branch:
git branch --show-current
echo.

:: Show current status
echo Current status:
git status --short
echo.

:: Show recent commits
echo Recent commits:
git log --oneline -5
echo.

:: Ask for commit message
set /p "commit_msg=Enter commit message (or 'exit' to cancel): "
if /i "%commit_msg%"=="exit" (
    echo Commit cancelled.
    pause
    exit /b
)

:: Check if message is empty
if "%commit_msg%"=="" (
    echo Error: Commit message cannot be empty!
    pause
    exit /b
)

:: Stage all changes
echo.
echo Staging all changes...
git add -A

:: Show what will be committed
echo.
echo Changes to be committed:
git status --short
echo.

:: Confirm commit
set /p "confirm=Proceed with commit? (y/n): "
if /i not "%confirm%"=="y" (
    echo Commit cancelled.
    git reset
    pause
    exit /b
)

:: Commit changes
echo.
echo Committing changes...
git commit -m "%commit_msg%"

if %errorlevel% neq 0 (
    echo Error: Commit failed!
    pause
    exit /b
)

echo.
echo Commit successful!
echo.

:: Ask about push
set /p "push=Push to remote? (y/n): "
if /i "%push%"=="y" (
    echo Pushing to remote...
    git push
    if %errorlevel% neq 0 (
        echo Warning: Push failed. You may need to pull first or resolve conflicts.
    ) else (
        echo Push successful!
    )
)

echo.

:: Ask about tag
set /p "tag=Create a tag for this commit? (y/n): "
if /i "%tag%"=="y" (
    set /p "tag_name=Enter tag name (e.g., v1.0.0): "
    if not "!tag_name!"=="" (
        set /p "tag_msg=Enter tag message (optional): "
        if "!tag_msg!"=="" (
            git tag !tag_name!
        ) else (
            git tag -a !tag_name! -m "!tag_msg!"
        )
        echo Tag created: !tag_name!
        
        set /p "push_tag=Push tag to remote? (y/n): "
        if /i "!push_tag!"=="y" (
            git push origin !tag_name!
            echo Tag pushed to remote!
        )
    )
)

echo.
echo ========================================
echo Git operations completed!
echo ========================================
echo.
pause
