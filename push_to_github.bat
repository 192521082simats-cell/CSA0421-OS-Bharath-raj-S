@echo off
title Push SysPilot to GitHub
color 0a
echo ======================================================================
echo          SysPilot: Push Project to GitHub Repository
echo ======================================================================
echo.
echo Target Remote: https://github.com/192521082simats-cell/CSA0421-OS-Bharath-raj-S.git
echo Active Branch: main
echo.
echo Attempting to push to remote main branch...
echo (If prompted, please complete the 1-click GitHub sign-in in your browser)
echo.

cd /d "%~dp0"
git push -u origin main

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ----------------------------------------------------------------------
    echo Notice: If the remote repository already has existing files (like README),
    echo would you like to overwrite it with your complete SysPilot Capstone project?
    echo ----------------------------------------------------------------------
    echo.
    set /p choice="Force push to overwrite remote with SysPilot? (y/n): "
    if /i "%choice%"=="y" (
        echo Pushing with force...
        git push -u origin main --force
    ) else (
        echo Pulling remote changes with merge...
        git pull origin main --allow-unrelated-histories --no-rebase -m "Merge remote repository into SysPilot"
        git push -u origin main
    )
)

echo.
echo ======================================================================
echo Done! Check your repository at:
echo https://github.com/192521082simats-cell/CSA0421-OS-Bharath-raj-S
echo ======================================================================
pause
