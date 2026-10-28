@echo off
setlocal
rem Builds build\WheelPlugin.asi with the MSVC x64 tools (Visual Studio / Build Tools with the C++ workload).
rem Needs the Script Hook V SDK in sdk\ (see sdk\README.md).

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Install Visual Studio or Build Tools with the "Desktop development with C++" workload.
    exit /b 1
)
set "VS="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if not defined VS (
    echo No Visual Studio installation with the C++ x64 tools was found.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

cd /d "%~dp0"
if not exist sdk\inc\main.h (
    echo Script Hook V SDK is missing. See sdk\README.md
    exit /b 1
)
if not exist build mkdir build

cl /nologo /LD /O2 /MT /EHsc /std:c++17 /utf-8 /W3 /I sdk /Fo:build\ src\main.cpp /Fe:build\WheelPlugin.asi /link /LIBPATH:sdk\lib ScriptHookV.lib dinput8.lib dxguid.lib user32.lib gdiplus.lib
if errorlevel 1 exit /b 1
echo.
echo Built build\WheelPlugin.asi
