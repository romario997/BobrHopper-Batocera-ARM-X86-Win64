# The Batocera / Knulli package (RG35XX H): out\package\BobrHopper-Batocera-<ver>\ and the same as a .zip:
#   WGRAJ_NA_KONSOLE.bat + install.ps1     the installer (card or network, see port/batocera/install.ps1)
#   gamelist-entry.xml                     the menu entry with the artwork, merged into roms/ports/gamelist.xml
#   CZYTAJ.txt
#   ports/BobrHopper.sh                    port/batocera/BobrHopper.sh
#   ports/bobrhopper/bobrhopper.aarch64    out/batocera (sh build/build_batocera.sh) - 64-bit Batocera (v43+)
#   ports/bobrhopper/bobrhopper.armhf      the same, 32-bit (community Batocera v40 on the RG35XX H)
#   ports/bobrhopper/data/...              data/ (sh build/bake_all.sh)
#   ports/images/BobrHopper-*.png          tools/make_batocera_media.py
# Refuses a stale binary and a CRLF launcher, like package_r36s.ps1.
param([string]$Version = "")
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Version) {
    $vh = Get-Content (Join-Path $repo 'src\ui\version_app.h') -Raw
    if ($vh -match 'kAppVersion\s*=\s*"(v\d+)"') { $Version = $Matches[1] }
    else { throw 'package_batocera: no kAppVersion in src/ui/version_app.h' }
}
function Fail([string]$msg) { Write-Host "BLAD: $msg" -ForegroundColor Red; exit 1 }

$bin = Join-Path $repo 'out\batocera\bobrhopper.aarch64'
$bin32 = Join-Path $repo 'out\batocera\bobrhopper.armhf'
$pb = Join-Path $repo 'port\batocera'
$launcher = Join-Path $pb 'BobrHopper.sh'
$data = Join-Path $repo 'data'
$media = Join-Path $repo 'out\batocera\media'
$mediaFiles = @('BobrHopper-image.png', 'BobrHopper-thumb.png', 'BobrHopper-marquee.png')
foreach ($p in @($bin, $bin32, $launcher, (Join-Path $pb 'install.ps1'), (Join-Path $pb 'WGRAJ_NA_KONSOLE.bat'),
                 (Join-Path $pb 'gamelist-entry.xml'), (Join-Path $data 'manifest.txt'))) {
    if (-not (Test-Path -LiteralPath $p)) { Fail "brak $p" }
}
foreach ($m in $mediaFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $media $m))) { Fail "brak $media\$m - uruchom: python tools/make_batocera_media.py" }
}
if ([System.IO.File]::ReadAllBytes($launcher) -contains 13) { Fail "port\batocera\BobrHopper.sh ma CRLF" }
$newestSource = Get-ChildItem -LiteralPath (Join-Path $repo 'src'), (Join-Path $repo 'apps') -Recurse -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($newestSource.LastWriteTime -gt (Get-Item -LiteralPath $bin).LastWriteTime -or $newestSource.LastWriteTime -gt (Get-Item -LiteralPath $bin32).LastWriteTime) {
    Fail "binarka starsza niz $($newestSource.Name) - przebuduj: sh build/build_batocera.sh"
}
[xml](Get-Content -LiteralPath (Join-Path $pb 'gamelist-entry.xml') -Raw -Encoding UTF8) | Out-Null

$name = "BobrHopper-Batocera-$Version"
$outDir = Join-Path $repo "out\package\$name"
$zipPath = "$outDir.zip"
if (Test-Path -LiteralPath $outDir) { Remove-Item -LiteralPath $outDir -Recurse -Force }
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
$ports = Join-Path $outDir 'ports'
New-Item -ItemType Directory -Force -Path (Join-Path $ports 'bobrhopper'), (Join-Path $ports 'images') | Out-Null

Copy-Item -LiteralPath $launcher -Destination (Join-Path $ports 'BobrHopper.sh')
Copy-Item -LiteralPath $bin -Destination (Join-Path $ports 'bobrhopper\bobrhopper.aarch64')
Copy-Item -LiteralPath $bin32 -Destination (Join-Path $ports 'bobrhopper\bobrhopper.armhf')
Copy-Item -LiteralPath $data -Destination (Join-Path $ports 'bobrhopper\data') -Recurse
foreach ($m in $mediaFiles) { Copy-Item -LiteralPath (Join-Path $media $m) -Destination (Join-Path $ports "images\$m") }
foreach ($f in @('install.ps1', 'WGRAJ_NA_KONSOLE.bat', 'gamelist-entry.xml')) {
    Copy-Item -LiteralPath (Join-Path $pb $f) -Destination (Join-Path $outDir $f)
}

$readme = @"
Bobr Hopper dla Batocery / Knulli (Anbernic RG35XX H i inne konsole ARM) - $Version
================================================================================

INSTALACJA AUTOMATYCZNA
1. Rozpakuj ten ZIP w dowolne miejsce na komputerze.
2. Kliknij dwa razy WGRAJ_NA_KONSOLE.bat.
3. Instalator sam znajdzie konsole i ZAPYTA, zanim cokolwiek zapisze:
   - karte SD wlozona do komputera (Batocera v40 na RG35XX H i Knulli: karta widoczna w Windows, FAT32/exFAT),
   - albo konsole w sieci Wi-Fi pod adresem \\BATOCERA\share.
   Nowa Batocera (v43+) trzyma gry na partycji ext4, ktorej Windows nie widzi na karcie - wtedy wlacz konsole,
   polacz ja z Wi-Fi (ta sama siec co komputer) i uruchom instalator. Jesli jej nie znajdzie, zapyta o adres IP
   (START > USTAWIENIA SIECI / NETWORK SETTINGS na konsoli).
   Sciezke mozna tez podac recznie w wierszu polecen:  WGRAJ_NA_KONSOLE.bat E:\
4. Na konsoli: START > USTAWIENIA GIER > AKTUALIZUJ LISTY GIER (GAME SETTINGS > UPDATE GAMELISTS)
   albo restart konsoli. Gra jest w PORTS > Bobr Hopper, z obrazkiem.

INSTALACJA RECZNA
  Skopiuj zawartosc folderu "ports" z paczki do /userdata/roms/ports na konsoli (w sieci: \\BATOCERA\share\roms\ports),
  a wpis z gamelist-entry.xml dopisz do roms/ports/gamelist.xml (bez niego gra tez dziala, tylko bez obrazka).

STEROWANIE
  D-pad         skok (wcisniecie = przysiad, puszczenie = skok)
  A             skok do przodu / start / zagraj ponownie
  Start         pauza
  Select        ustawienia (dzwieki, muzyka, cienie, widok, jezyk, postac, gracze)
  Select+Start  wyjscie z gry  (dziala tez skrot systemowy HOTKEY+START)
  Select+L      licznik FPS

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
    if ($check.GetEntry("$name/ports/bobrhopper/bobrhopper.aarch64").Length -ne (Get-Item -LiteralPath $bin).Length) { Fail "binarka aarch64 w zipie ma zly rozmiar" }
    if ($check.GetEntry("$name/ports/bobrhopper/bobrhopper.armhf").Length -ne (Get-Item -LiteralPath $bin32).Length) { Fail "binarka armhf w zipie ma zly rozmiar" }
    foreach ($m in $mediaFiles) { if (-not $check.GetEntry("$name/ports/images/$m")) { Fail "brak $m w zipie" } }
} finally { $check.Dispose() }
$size = [math]::Round((Get-Item -LiteralPath $zipPath).Length / 1MB, 1)
Write-Host "Paczka: $zipPath ($size MB, $($names.Count) plikow)"
Write-Host "Folder: $outDir"
exit 0
