@echo off
setlocal
echo ==== WheelPlugin uninstaller ====
set "GAME=%~1"
if "%GAME%"=="" set /p "GAME=GTA V folder (drag it here or type the path): "
set "GAME=%GAME:"=%"
if not exist "%GAME%\GTA5.exe" (
    echo GTA5.exe not found in "%GAME%"
    pause
    exit /b 1
)
tasklist /fi "imagename eq GTA5.exe" 2>nul | find /i "GTA5.exe" >nul
if not errorlevel 1 (
    echo GTA5.exe is running. Close the game first, it locks the plugin file.
    pause
    exit /b 1
)
del /q "%GAME%\WheelPlugin.asi" 2>nul && echo Removed WheelPlugin.asi
del /q "%GAME%\WheelPlugin.log" 2>nul
del /q "%GAME%\WheelPlugin_capture_*.csv" 2>nul
del /q "%GAME%\WheelPlugin_pumps.txt" 2>nul
choice /m "Also delete your settings (WheelPlugin.ini)"
if errorlevel 2 goto done
del /q "%GAME%\WheelPlugin.ini" 2>nul && echo Removed WheelPlugin.ini
:done
echo.
echo Done. Script Hook V and the -nobattleye line in commandline.txt were left as they are.
pause
