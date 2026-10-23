@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%~dp0setup_windows.py" %*
    goto finish
)

where python >nul 2>nul
if not errorlevel 1 (
    python "%~dp0setup_windows.py" %*
    goto finish
)

echo Python 3 is required only to download the optional Windows tools.
echo Install Python from https://www.python.org/downloads/windows/ and run this file again.
set "RESULT=1"
goto pause

:finish
set "RESULT=%ERRORLEVEL%"

:pause
echo.
pause
exit /b %RESULT%
