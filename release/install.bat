@echo off
setlocal
echo ==== WheelPlugin installer ====
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
if not exist "%GAME%\ScriptHookV.dll" echo WARNING: ScriptHookV.dll not found in the game folder. Install Script Hook V first.
if not exist "%GAME%\dinput8.dll" echo WARNING: dinput8.dll ^(ASI loader from Script Hook V^) not found in the game folder.

copy /y "%~dp0WheelPlugin.asi" "%GAME%\" >nul && echo Copied WheelPlugin.asi

if exist "%GAME%\WheelPlugin.ini" (
    echo Your WheelPlugin.ini is kept: new settings use their defaults.
    choice /m "Overwrite it with the default settings instead"
    if errorlevel 2 goto skipini
)
copy /y "%~dp0WheelPlugin.ini" "%GAME%\" >nul && echo Copied WheelPlugin.ini
:skipini

choice /m "Add -nobattleye to commandline.txt (mods do not work with BattlEye; story mode only)"
if errorlevel 2 goto done
findstr /c:"-nobattleye" "%GAME%\commandline.txt" >nul 2>&1
if errorlevel 1 (
    >>"%GAME%\commandline.txt" echo.
    >>"%GAME%\commandline.txt" echo -nobattleye
    echo Added -nobattleye
) else (
    echo -nobattleye already present
)

:done
echo.
echo Done. Start the game, load story mode and press F11. The guides (USER_GUIDE.md / USER_GUIDE.th.md) explain the first start.
pause
