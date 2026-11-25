@echo off
setlocal
cd /d "%~dp0\.."
py -3 src\ntro_tweaks_free.py
if errorlevel 1 (
  echo.
  echo Python 3 is required. Install it from https://www.python.org/downloads/windows/
  pause
)
