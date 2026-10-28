<#
.SYNOPSIS
  Build WheelPlugin and copy it into the GTA V folder.
.PARAMETER Game
  GTA V folder (the one with GTA5.exe). Can also be set once with the GTAV_DIR environment variable.
.PARAMETER NoBuild
  Skip the build and copy the existing build\WheelPlugin.asi.
.PARAMETER Launch
  Start the game afterwards with -nobattleye (mods do not work with BattlEye; single player only).
#>
param(
    [string]$Game = $env:GTAV_DIR,
    [switch]$NoBuild,
    [switch]$Launch
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

if (-not $Game -or -not (Test-Path (Join-Path $Game "GTA5.exe"))) {
    throw "GTA5.exe not found. Pass -Game <GTA V folder> or set GTAV_DIR."
}
if (-not $NoBuild) {
    # vcvars64.bat may print a harmless "vswhere.exe is not recognized" line to stderr: do not let that stop the script
    $ErrorActionPreference = "Continue"
    & cmd /c "`"$root\build.bat`"" 2>&1 | ForEach-Object { "$_" } | Where-Object { $_ -notmatch "vswhere" }
    $buildExit = $LASTEXITCODE
    $ErrorActionPreference = "Stop"
    if ($buildExit -ne 0) { throw "build failed" }
}
$asi = Join-Path $root "build\WheelPlugin.asi"
if (-not (Test-Path $asi)) { throw "build\WheelPlugin.asi not found" }

# a running game locks the plugin file
$running = Get-Process GTA5 -ErrorAction SilentlyContinue
if ($running) { throw "GTA5.exe is running (pid $($running.Id)). Close the game, the plugin file is locked." }

Copy-Item $asi (Join-Path $Game "WheelPlugin.asi") -Force
$built = (Get-Item $asi).LastWriteTime
$installed = (Get-Item (Join-Path $Game "WheelPlugin.asi")).LastWriteTime
Write-Host "built:     $built"
Write-Host "installed: $installed"
if ($built -ne $installed) { Write-Warning "timestamps differ: the copy may not be the build you just made" }

if ($Launch) {
    Start-Process -FilePath (Join-Path $Game "PlayGTAV.exe") -ArgumentList "-nobattleye" -WorkingDirectory $Game
    Write-Host "launched with -nobattleye"
}
