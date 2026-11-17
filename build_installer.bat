@echo off
setlocal
title Build NeoNyx Setup
if not exist build\NeoNyx.exe (
  echo First build the application with build_cpp.bat
  pause
  exit /b 1
)
where ISCC.exe >nul 2>nul
if errorlevel 1 (
  echo Inno Setup 6 compiler (ISCC.exe) was not found.
  echo Install Inno Setup 6, then run this file again.
  pause
  exit /b 1
)
ISCC.exe installer\NeoNyx.iss
if errorlevel 1 (
  echo Installer build failed.
  pause
  exit /b 1
)
echo.
echo Ready: dist\NeoNyx_Setup.exe
pause
