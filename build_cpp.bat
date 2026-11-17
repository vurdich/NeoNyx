@echo off
setlocal
title Build NeoNyx C++
where cl >nul 2>nul
if errorlevel 1 (
  echo Visual Studio C++ Build Tools are not available.
  echo Open "x64 Native Tools Command Prompt for VS" and run this file there.
  pause
  exit /b 1
)
where rc >nul 2>nul
if errorlevel 1 (
  echo Windows Resource Compiler (rc.exe) was not found.
  echo Run this from the x64 Native Tools Command Prompt for VS.
  pause
  exit /b 1
)
if not exist build mkdir build
rc /nologo /fo build\NeoNyx.res src\NeoNyx.rc
if errorlevel 1 (
  echo Resource compilation failed.
  pause
  exit /b 1
)
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN src\main.cpp build\NeoNyx.res /link /SUBSYSTEM:WINDOWS comctl32.lib comdlg32.lib shell32.lib shlwapi.lib crypt32.lib /OUT:build\NeoNyx.exe
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
echo.
echo Build complete: build\NeoNyx.exe
pause
