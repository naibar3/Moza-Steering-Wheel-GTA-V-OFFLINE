<#
.SYNOPSIS
  Build WheelPlugin and write the two release zips next to the repository folder:
    WheelPlugin-v<version>-install.zip   for players (asi, ini, install / uninstall scripts, guides, notices)
    WheelPlugin-v<version>-source.zip    for developers (no Script Hook V SDK, no build output)
.PARAMETER Out
  Where to write the folders and zips. Default: the folder that contains this repository.
.PARAMETER NoBuild
  Use the existing build\WheelPlugin.asi.
.NOTES
  The version is WP_VERSION in src\main.cpp. The Script Hook V SDK (sdk\inc, sdk\lib) is never packaged.
#>
param(
    [string]$Out = (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent),
    [switch]$NoBuild
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

# zip with forward slashes in the entry names (Compress-Archive in Windows PowerShell 5.1 writes backslashes, which break other unzip tools)
function New-Zip([string]$Dir, [string]$Zip) {
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    if (Test-Path $Zip) { Remove-Item $Zip -Force }
    $base = (Resolve-Path $Dir).Path.TrimEnd('\')
    $fs = [IO.File]::Open($Zip, [IO.FileMode]::Create)
    $za = New-Object IO.Compression.ZipArchive($fs, [IO.Compression.ZipArchiveMode]::Create)
    try {
        Get-ChildItem $Dir -Recurse -File | ForEach-Object {
            $rel = $_.FullName.Substring($base.Length + 1).Replace('\', '/')
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($za, $_.FullName, $rel, [IO.Compression.CompressionLevel]::Optimal)
        }
    } finally { $za.Dispose(); $fs.Dispose() }
}
$ver = ([regex]::Match((Get-Content (Join-Path $root "src\main.cpp") -Raw), '#define WP_VERSION "([^"]+)"')).Groups[1].Value
if (-not $ver) { throw "WP_VERSION not found in src\main.cpp" }

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

# ---- install package
$inst = Join-Path $Out "WheelPlugin-v$ver-install"
if (Test-Path $inst) { Remove-Item $inst -Recurse -Force }
New-Item -ItemType Directory $inst | Out-Null
Copy-Item $asi $inst
Copy-Item (Join-Path $root "config\WheelPlugin.ini") $inst
Copy-Item (Join-Path $root "release\install.bat") $inst
Copy-Item (Join-Path $root "release\uninstall.bat") $inst
(Get-Content (Join-Path $root "release\README.txt") -Raw -Encoding UTF8).Replace("@VERSION@", "v$ver") | Set-Content (Join-Path $inst "README.txt") -Encoding UTF8
Copy-Item (Join-Path $root "docs\USER_GUIDE.md") $inst
Copy-Item (Join-Path $root "docs\USER_GUIDE.th.md") $inst
Copy-Item (Join-Path $root "LICENSE") (Join-Path $inst "LICENSE.txt")
Copy-Item (Join-Path $root "THIRD_PARTY_NOTICES.md") (Join-Path $inst "THIRD_PARTY_NOTICES.txt")
$instZip = "$inst.zip"
New-Zip $inst $instZip

# ---- source package (the Script Hook V SDK and build output are left out)
$srcDir = Join-Path $Out "WheelPlugin-v$ver-source"
if (Test-Path $srcDir) { Remove-Item $srcDir -Recurse -Force }
New-Item -ItemType Directory $srcDir | Out-Null
& robocopy $root $srcDir /E /XD build .git "$root\sdk\inc" "$root\sdk\lib" /XF *.asi *.obj *.exp *.lib *.pdb *.ilk WheelPlugin.log WheelPlugin_capture_*.csv /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }
if ((Test-Path (Join-Path $srcDir "sdk\inc")) -or (Test-Path (Join-Path $srcDir "sdk\lib"))) { throw "the Script Hook V SDK must not be packaged" }
$srcZip = "$srcDir.zip"
New-Zip $srcDir $srcZip

Write-Host ""
Write-Host "version:  $ver"
Write-Host "asi:      $((Get-Item $asi).LastWriteTime)  $((Get-Item $asi).Length) bytes"
Write-Host "install:  $instZip  ($([int]((Get-Item $instZip).Length / 1KB)) KB)"
Write-Host "source:   $srcZip  ($([int]((Get-Item $srcZip).Length / 1KB)) KB)"
