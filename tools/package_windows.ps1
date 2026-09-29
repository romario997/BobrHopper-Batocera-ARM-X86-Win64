# The Windows release package: out\package\BobrHopper-Windows-<version>.zip with one folder, BobrHopper\, holding
# BobrHopper.exe (GUI subsystem, icon - build/build_windows.sh), SDL2.dll, data\ and CZYTAJ.txt (port/windows/).
# Unzipped anywhere it runs as it is: data\ is found next to the exe, the settings go to conf\ next to it, or to
# %APPDATA%\BobrHopper when that folder cannot be written.
#   powershell -ExecutionPolicy Bypass -File tools\package_windows.ps1 [-Version v028] [-NoBuild]
param([string]$Version = "", [string]$Out = "", [switch]$NoBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
function Fail([string]$msg) { Write-Host "BLAD: $msg" -ForegroundColor Red; exit 1 }
# the version comes from the source (what the title screen shows), as in package_r36s.ps1
if (-not $Version) {
    $vh = Get-Content (Join-Path $repo 'src\ui\version_app.h') -Raw
    if ($vh -match 'kAppVersion\s*=\s*"(v\d+)"') { $Version = $Matches[1] }
    else { Fail 'no kAppVersion in src/ui/version_app.h' }
}
if (-not $Out) { $Out = Join-Path $repo "out\package\BobrHopper-Windows-$Version.zip" }

$exe = Join-Path $repo 'out\windows\BobrHopper.exe'
$dll = Join-Path $repo 'out\windows\SDL2.dll'
$data = Join-Path $repo 'data'
$readmeSrc = Join-Path $repo 'port\windows\CZYTAJ.txt'

if (-not $NoBuild) {
    # Git Bash runs the build scripts (pwd -W, cygpath)
    $sh = $null
    $cmd = Get-Command sh.exe -ErrorAction SilentlyContinue
    if ($cmd) { $sh = $cmd.Source }
    foreach ($c in @("$env:ProgramFiles\Git\bin\sh.exe", "${env:ProgramFiles(x86)}\Git\bin\sh.exe",
                     "$env:LOCALAPPDATA\Programs\Git\bin\sh.exe")) {
        if (-not $sh -and (Test-Path -LiteralPath $c)) { $sh = $c }
    }
    if (-not $sh) { Fail 'no Git Bash (sh.exe) - build with: sh build/build_windows.sh, then run this with -NoBuild' }
    Push-Location $repo
    try {
        & $sh build/build_windows.sh
        if ($LASTEXITCODE -ne 0) { Fail 'build/build_windows.sh failed' }
    } finally { Pop-Location }
}

foreach ($p in @($exe, $dll, $readmeSrc, (Join-Path $data 'manifest.txt'))) {
    if (-not (Test-Path -LiteralPath $p)) { Fail "missing $p" }
}
$newestSource = Get-ChildItem -LiteralPath (Join-Path $repo 'src'), (Join-Path $repo 'apps') -Recurse -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($newestSource.LastWriteTime -gt (Get-Item -LiteralPath $exe).LastWriteTime) {
    Fail "BobrHopper.exe is older than $($newestSource.Name) - rebuild: sh build/build_windows.sh"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
if (Test-Path -LiteralPath $Out) { Remove-Item -LiteralPath $Out -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
# CZYTAJ.txt: UTF-8 with a BOM and CRLF, so Notepad shows the Polish letters
$readme = [System.IO.File]::ReadAllText($readmeSrc, [System.Text.Encoding]::UTF8).Replace('{VERSION}', $Version)
$readme = $readme.Replace("`r`n", "`n").Replace("`n", "`r`n")

$root = 'BobrHopper/'
$zip = [System.IO.Compression.ZipFile]::Open($Out, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    $add = {
        param([string]$path, [string]$name)
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $path, $name,
            [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
    & $add $exe ($root + 'BobrHopper.exe')
    & $add $dll ($root + 'SDL2.dll')
    foreach ($f in Get-ChildItem -LiteralPath $data -Recurse -File) {
        $rel = $f.FullName.Substring($data.Length).TrimStart('\').Replace('\', '/')
        & $add $f.FullName ($root + 'data/' + $rel)
    }
    $entry = $zip.CreateEntry($root + 'CZYTAJ.txt')
    $writer = New-Object System.IO.StreamWriter($entry.Open(), (New-Object System.Text.UTF8Encoding($true)))
    $writer.Write($readme)
    $writer.Dispose()
} finally {
    $zip.Dispose()
}

# verify what was written
$check = [System.IO.Compression.ZipFile]::OpenRead($Out)
try {
    $names = @($check.Entries | ForEach-Object { $_.FullName })
    $dataFiles = (Get-ChildItem -LiteralPath $data -Recurse -File).Count
    if (@($names | Where-Object { $_.Contains('\') }).Count) { Fail 'entries with a backslash in the zip' }
    if (@($names | Where-Object { $_.StartsWith($root + 'data/') }).Count -ne $dataFiles) { Fail 'data incomplete in the zip' }
    if ($check.GetEntry($root + 'BobrHopper.exe').Length -ne (Get-Item -LiteralPath $exe).Length) { Fail 'exe size differs in the zip' }
    if (-not $check.GetEntry($root + 'SDL2.dll')) { Fail 'no SDL2.dll in the zip' }
} finally {
    $check.Dispose()
}
$size = [math]::Round((Get-Item -LiteralPath $Out).Length / 1MB, 1)
Write-Host "Paczka: $Out ($size MB, $($names.Count) plikow, $dataFiles plikow danych)"
exit 0
