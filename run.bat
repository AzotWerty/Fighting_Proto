@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1"
if errorlevel 1 (
    echo.
    echo Не удалось запустить игру. Прочитайте сообщение выше.
    pause
)
