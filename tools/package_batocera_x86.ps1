# The Batocera PC (x86_64) package: out\package\BobrHopper-BatoceraPC-<ver>\ and the same as a .zip:
#   WGRAJ_NA_BATOCERE_PC.bat + install.ps1 the installer (SHARE drive or network; asks before writing anything)
#   gamelist-entry.xml                     the menu entry with the artwork, merged into roms/ports/gamelist.xml
#   CZYTAJ.txt
#   ports/BobrHopper.sh                    port/batocera-x86/BobrHopper.sh
#   ports/bobrhopper/bobrhopper.x86_64     out/batocera-x86 (sh build/build_batocera_x86.sh)
#   ports/bobrhopper/data/...              data/ (sh build/bake_all.sh - includes the big-screen fonts)
#   ports/images/BobrHopper-*.png          tools/make_batocera_x86_media.py (1920x1080)
# Refuses a stale binary and a CRLF launcher, like package_r36s.ps1. Adapted from the ARM port's package_batocera.ps1.
param([string]$Version = "")
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Version) {
    $vh = Get-Content (Join-Path $repo 'src\ui\version_app.h') -Raw
    if ($vh -match 'kAppVersion\s*=\s*"(v\d+)"') { $Version = $Matches[1] }
    else { throw 'package_batocera_x86: no kAppVersion in src/ui/version_app.h' }
}
function Fail([string]$msg) { Write-Host "BLAD: $msg" -ForegroundColor Red; exit 1 }

$bin = Join-Path $repo 'out\batocera-x86\bobrhopper.x86_64'
$pb = Join-Path $repo 'port\batocera-x86'
$launcher = Join-Path $pb 'BobrHopper.sh'
$data = Join-Path $repo 'data'
$media = Join-Path $repo 'out\batocera-x86\media'
$mediaFiles = @('BobrHopper-image.png', 'BobrHopper-thumb.png', 'BobrHopper-marquee.png')
foreach ($p in @($bin, $launcher, (Join-Path $pb 'install.ps1'), (Join-Path $pb 'WGRAJ_NA_BATOCERE_PC.bat'),
                 (Join-Path $pb 'gamelist-entry.xml'), (Join-Path $data 'manifest.txt'), (Join-Path $data 'fonts\retro_108.fnt'))) {
    if (-not (Test-Path -LiteralPath $p)) { Fail "brak $p" }
}
foreach ($m in $mediaFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $media $m))) { Fail "brak $media\$m - uruchom: python tools/make_batocera_x86_media.py" }
}
if ([System.IO.File]::ReadAllBytes($launcher) -contains 13) { Fail "port\batocera-x86\BobrHopper.sh ma CRLF" }
$newestSource = Get-ChildItem -LiteralPath (Join-Path $repo 'src'), (Join-Path $repo 'apps') -Recurse -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($newestSource.LastWriteTime -gt (Get-Item -LiteralPath $bin).LastWriteTime) {
    Fail "binarka starsza niz $($newestSource.Name) - przebuduj: sh build/build_batocera_x86.sh"
}
[xml](Get-Content -LiteralPath (Join-Path $pb 'gamelist-entry.xml') -Raw -Encoding UTF8) | Out-Null

$name = "BobrHopper-BatoceraPC-$Version"
$outDir = Join-Path $repo "out\package\$name"
$zipPath = "$outDir.zip"
if (Test-Path -LiteralPath $outDir) { Remove-Item -LiteralPath $outDir -Recurse -Force }
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
$ports = Join-Path $outDir 'ports'
New-Item -ItemType Directory -Force -Path (Join-Path $ports 'bobrhopper'), (Join-Path $ports 'images') | Out-Null

Copy-Item -LiteralPath $launcher -Destination (Join-Path $ports 'BobrHopper.sh')
Copy-Item -LiteralPath $bin -Destination (Join-Path $ports 'bobrhopper\bobrhopper.x86_64')
Copy-Item -LiteralPath $data -Destination (Join-Path $ports 'bobrhopper\data') -Recurse
foreach ($m in $mediaFiles) { Copy-Item -LiteralPath (Join-Path $media $m) -Destination (Join-Path $ports "images\$m") }
foreach ($f in @('install.ps1', 'WGRAJ_NA_BATOCERE_PC.bat', 'gamelist-entry.xml')) {
    Copy-Item -LiteralPath (Join-Path $pb $f) -Destination (Join-Path $outDir $f)
}

$readme = @"
Bobr Hopper dla Batocery PC (x86_64: komputer z Batocera na pendrivie / dysku) - $Version
========================================================================================

Gra dziala na pelnym ekranie w rozdzielczosci pulpitu Batocery (np. 1920x1080). Menu, napisy i ich
obrysy sa przeskalowane proporcjonalnie (przy 1080p 2,25x), obraz 16:9 bez rozciagania.

INSTALACJA AUTOMATYCZNA
1. Rozpakuj ten ZIP w dowolne miejsce na komputerze z Windows.
2. Podlacz pendrive / dysk z Batocera (albo wlacz Batocere w tej samej sieci).
3. Kliknij dwa razy WGRAJ_NA_BATOCERE_PC.bat.
4. Instalator poszuka partycji SHARE Batocery (tej z folderami roms, system, bios... - czesto exFAT,
   np. z etykieta SHARE albo ROMS), pokaze co znalazl i ZAPYTA, zanim cokolwiek zapisze.
   Jesli SHARE jest w ext4/btrfs (Windows jej nie widzi), wgra gre przez siec: \\BATOCERA\share.
   Sciezke mozna tez podac recznie w wierszu polecen:  WGRAJ_NA_BATOCERE_PC.bat H:\
5. Uruchom Batocere. Gra jest w PORTS > Bobr Hopper, z obrazkiem
   (jesli jej nie widac: MENU > USTAWIENIA GIER > AKTUALIZUJ LISTY GIER / UPDATE GAMELISTS).

INSTALACJA RECZNA
  Skopiuj zawartosc folderu "ports" z paczki do /userdata/roms/ports (na partycji SHARE: roms\ports),
  a wpis z gamelist-entry.xml dopisz do roms/ports/gamelist.xml (bez niego gra tez dziala, tylko bez obrazka).

STEROWANIE
  Pad:        D-pad skok (wcisniecie = przysiad, puszczenie = skok), A skok do przodu / start,
              Start pauza, Select ustawienia, Select+Start wyjscie (dziala tez HOTKEY+START), Select+L licznik FPS
  Klawiatura: strzalki + Spacja/Enter (drugi gracz: WSAD), Esc pauza

Jesli gra sie nie uruchomi: roms/ports/bobrhopper/bobrhopper-launcher.log - przynies go do komputera.
Ustawienia i rekord: roms/ports/bobrhopper/conf/crossy.cfg (ponowna instalacja ich nie kasuje).
"@
[System.IO.File]::WriteAllText((Join-Path $outDir 'CZYTAJ.txt'), $readme.Replace("`r`n", "`n").Replace("`n", "`r`n"),
    (New-Object System.Text.UTF8Encoding($true)))

# the zip: '/' in entry names (Compress-Archive in PowerShell 5.1 writes '\')
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::Open($zipPath, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($f in Get-ChildItem -LiteralPath $outDir -Recurse -File) {
        $rel = $f.FullName.Substring($outDir.Length).TrimStart('\').Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $f.FullName, "$name/$rel",
            [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $zip.Dispose() }

# verify
$check = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $names = @($check.Entries | ForEach-Object { $_.FullName })
    $dataFiles = (Get-ChildItem -LiteralPath $data -Recurse -File).Count
    if (@($names | Where-Object { $_.StartsWith("$name/ports/bobrhopper/data/") }).Count -ne $dataFiles) { Fail "niepelne dane w zipie" }
    $ms = New-Object System.IO.MemoryStream
    $s = $check.GetEntry("$name/ports/BobrHopper.sh").Open(); $s.CopyTo($ms); $s.Dispose()
    if ($ms.ToArray() -contains 13) { Fail "launcher w zipie ma CR" }
    if ($check.GetEntry("$name/ports/bobrhopper/bobrhopper.x86_64").Length -ne (Get-Item -LiteralPath $bin).Length) { Fail "binarka x86_64 w zipie ma zly rozmiar" }
    foreach ($m in $mediaFiles) { if (-not $check.GetEntry("$name/ports/images/$m")) { Fail "brak $m w zipie" } }
} finally { $check.Dispose() }
$size = [math]::Round((Get-Item -LiteralPath $zipPath).Length / 1MB, 1)
Write-Host "Paczka: $zipPath ($size MB, $($names.Count) plikow)"
Write-Host "Folder: $outDir"
exit 0
