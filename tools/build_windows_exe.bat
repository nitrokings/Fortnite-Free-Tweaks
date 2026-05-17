@echo off
setlocal
cd /d "%~dp0\.."
py -3 -m pip install --upgrade pyinstaller
py -3 -m PyInstaller --onefile --windowed --name "NTRO TWEAKS FREE" src\ntro_tweaks_free.py
if errorlevel 1 (
  echo.
  echo Build failed. Make sure Python 3 and pip are installed, then run this file again.
  pause
  exit /b 1
)
echo.
echo Built dist\NTRO TWEAKS FREE.exe
pause
