@echo off
setlocal
rem Exercise the native volume control in the actual player, with all theme dependencies.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0smoke-panel.ps1"
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0smoke-panel.ps1" -SingleTrack
exit /b %errorlevel%
