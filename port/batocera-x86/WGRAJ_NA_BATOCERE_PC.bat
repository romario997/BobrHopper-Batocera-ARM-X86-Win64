@echo off
chcp 65001 >nul
title Bobr Hopper - wgrywanie na Batocere PC
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
echo.
pause
