# Instalator Bobr Hopper na Batocere / Knulli (RG35XX H). Uruchamiany przez WGRAJ_NA_KONSOLE.bat.
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-Target <E:\ | \\BATOCERA\share | \\192.168.1.50\share>]
# Kopiuje ports\ z tej paczki do <SHARE>\roms\ports (nic nie kasuje: ustawienia i rekord w bobrhopper\conf zostaja)
# i dopisuje gre do roms\ports\gamelist.xml (z kopia zapasowa), zeby w menu byla miniaturka.
# Batocera na H700 ma partycje SHARE w ext4 - Windows jej na karcie nie widzi, wtedy wgrywamy przez siec (Wi-Fi).
param([string]$Target = "", [switch]$Yes)   # -Yes: no confirmation question (tests)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$srcPorts = Join-Path $here 'ports'
$entryFile = Join-Path $here 'gamelist-entry.xml'

function Info([string]$m) { Write-Host $m }
function Ok([string]$m) { Write-Host $m -ForegroundColor Green }
function Warn([string]$m) { Write-Host $m -ForegroundColor Yellow }
function Fail([string]$m) { Write-Host ""; Write-Host "BLAD: $m" -ForegroundColor Red; exit 1 }

if (-not (Test-Path -LiteralPath (Join-Path $srcPorts 'BobrHopper.sh'))) { Fail "brak plikow gry obok instalatora ($srcPorts). Rozpakuj caly ZIP." }

# Test-Path na nieistniejacym adresie sieciowym potrafi wisiec dlugo - sprawdzamy w tle z limitem czasu
function Test-PathTimeout([string]$path, [int]$seconds = 20) {
    $job = Start-Job -ScriptBlock { param($p) Test-Path -LiteralPath $p } -ArgumentList $path
    if (Wait-Job $job -Timeout $seconds) { $r = Receive-Job $job; Remove-Job $job -Force; return [bool]$r }
    Remove-Job $job -Force
    return $false
}

# z tego, co podal uzytkownik (litera, SHARE, roms albo ports), robi folder roms
function Resolve-Roms([string]$t) {
    $t = $t.Trim().Trim('"').TrimEnd('\', '/')
    if ($t -match '^[A-Za-z]:?$') { $t = $t.Substring(0, 1) + ':' }
    if ($t -match '^\d{1,3}(\.\d{1,3}){3}$' -or $t -match '^[A-Za-z][\w-]*$' -and $t.Length -gt 1) { $t = "\\$t\share" }
    foreach ($c in @("$t\roms", $t, (Split-Path -Parent $t))) {
        if (-not $c) { continue }
        if ((Split-Path -Leaf $c) -eq 'roms' -and (Test-PathTimeout $c)) { return $c }
    }
    return $null
}

Info "=============================================="
Info " Bobr Hopper - wgrywanie na Batocere (RG35XX H)"
Info "=============================================="
Info ""

$roms = $null
if ($Target) {
    $roms = Resolve-Roms $Target
    if (-not $roms) { Fail "pod '$Target' nie ma folderu roms." }
} else {
    Info "Szukam karty z Batocera/Knulli w komputerze..."
    $bootOnly = @()
    foreach ($d in Get-PSDrive -PSProvider FileSystem) {
        $root = $d.Root.TrimEnd('\')
        if ($root -eq $env:SystemDrive) { continue }
        try {
            if (Test-Path -LiteralPath "$root\roms\ports" -ErrorAction Stop) { $roms = "$root\roms"; break }
            if ((Test-Path -LiteralPath "$root\batocera-boot.conf") -or (Test-Path -LiteralPath "$root\boot\batocera")) { $bootOnly += $root }
        } catch { }
    }
    if (-not $roms -and $bootOnly.Count) {
        Warn "Widze tylko partycje startowa Batocery ($($bootOnly -join ', ')). Gry sa na partycji SHARE (ext4),"
        Warn "ktorej Windows nie widzi - wgram przez siec."
    }
    if (-not $roms) {
        Info "Szukam konsoli w sieci (\\BATOCERA\share)... do 20 sekund"
        if (Test-PathTimeout '\\BATOCERA\share\roms') { $roms = '\\BATOCERA\share\roms' }
    }
    while (-not $roms) {
        Info ""
        Warn "Nie znalazlem konsoli automatycznie."
        Info "Wlacz konsole, polacz ja z Wi-Fi (START > USTAWIENIA SIECI / NETWORK SETTINGS) i odczytaj tam jej adres IP."
        $ans = Read-Host "Wpisz adres IP konsoli (np. 192.168.1.50) albo litere karty (np. E), puste = wyjscie"
        if (-not $ans) { Fail "przerwano." }
        $roms = Resolve-Roms $ans
        if (-not $roms) { Warn "Pod '$ans' nie ma folderu roms." }
    }
}

$ports = Join-Path $roms 'ports'
$network = $roms.StartsWith('\\')
Info ""
Ok "Cel: $ports"
# -Yes skips the question only for an explicit -Target: an automatically found card is always confirmed
$answer = if ($Yes -and $Target) { "T" } else { Read-Host "Wgrac tutaj gre? [T/n]" }
if ($answer -and $answer -notmatch '^[TtYy]') { Fail "przerwano." }

# --- kopiowanie (robocopy bez /PURGE: niczego nie kasuje, conf\ z ustawieniami zostaje) ------------------------
if (-not (Test-Path -LiteralPath $ports)) { New-Item -ItemType Directory -Path $ports | Out-Null }
Info "Kopiuje pliki gry..."
& robocopy.exe $srcPorts $ports /E /R:2 /W:2 /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { Fail "kopiowanie nie powiodlo sie (robocopy kod $LASTEXITCODE)." }

# --- sprawdzenie ----------------------------------------------------------------------------------------------
foreach ($b in @('bobrhopper.aarch64', 'bobrhopper.armhf')) {
    $srcBin = Join-Path $srcPorts "bobrhopper\$b"
    $dstBin = Join-Path $ports "bobrhopper\$b"
    if ((Get-Item -LiteralPath $dstBin).Length -ne (Get-Item -LiteralPath $srcBin).Length) { Fail "plik gry $b na konsoli ma zly rozmiar - sprobuj jeszcze raz." }
}
if ([System.IO.File]::ReadAllBytes((Join-Path $ports 'BobrHopper.sh')) -contains 13) { Fail "BobrHopper.sh na konsoli ma konce linii Windows." }
$srcCount = (Get-ChildItem -LiteralPath (Join-Path $srcPorts 'bobrhopper\data') -Recurse -File).Count
$dstCount = (Get-ChildItem -LiteralPath (Join-Path $ports 'bobrhopper\data') -Recurse -File).Count
if ($dstCount -lt $srcCount) { Fail "na konsoli brakuje plikow danych ($dstCount z $srcCount)." }
Ok "Pliki gry skopiowane i sprawdzone."

# --- gamelist.xml: wpis z miniaturka --------------------------------------------------------------------------
$gl = Join-Path $ports 'gamelist.xml'
$entry = New-Object System.Xml.XmlDocument
$entry.Load($entryFile)
$newGame = $entry.SelectSingleNode('/gameList/game')
$utf8 = New-Object System.Text.UTF8Encoding($false)
try {
    $doc = New-Object System.Xml.XmlDocument
    $doc.PreserveWhitespace = $false
    if (Test-Path -LiteralPath $gl) {
        $backup = "$gl.przed-bobrhopper"
        if (-not (Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $gl -Destination $backup }
        $doc.LoadXml([System.IO.File]::ReadAllText($gl, [System.Text.Encoding]::UTF8))
        if (-not $doc.DocumentElement -or $doc.DocumentElement.Name -ne 'gameList') { throw "to nie jest gamelist.xml" }
    } else {
        $doc.AppendChild($doc.CreateXmlDeclaration('1.0', 'UTF-8', $null)) | Out-Null
        $doc.AppendChild($doc.CreateElement('gameList')) | Out-Null
    }
    foreach ($g in @($doc.DocumentElement.SelectNodes('game'))) {
        $p = $g.SelectSingleNode('path')
        if ($p -and ($p.InnerText -eq './BobrHopper.sh' -or $p.InnerText -eq 'BobrHopper.sh' -or $p.InnerText -like '*/ports/BobrHopper.sh')) {
            # czas gry i liczba uruchomien zostaja z poprzedniego wpisu
            foreach ($keepTag in @('playcount', 'lastplayed', 'gametime', 'favorite')) {
                $old = $g.SelectSingleNode($keepTag)
                if ($old -and -not $newGame.SelectSingleNode($keepTag)) { $newGame.AppendChild($entry.ImportNode($old, $true)) | Out-Null }
            }
            $doc.DocumentElement.RemoveChild($g) | Out-Null
        }
    }
    $doc.DocumentElement.AppendChild($doc.ImportNode($newGame, $true)) | Out-Null
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Indent = $true
    $settings.Encoding = $utf8
    $settings.NewLineChars = "`n"
    $ms = New-Object System.IO.MemoryStream
    $w = [System.Xml.XmlWriter]::Create($ms, $settings)
    $doc.Save($w); $w.Dispose()
    [System.IO.File]::WriteAllBytes($gl, $ms.ToArray())
    Ok "Dopisano gre do listy (gamelist.xml) z miniaturka."
} catch {
    Warn "Nie udalo sie dopisac gry do gamelist.xml ($($_.Exception.Message))."
    Warn "Gra i tak bedzie w menu Ports, tylko bez obrazka. Plik gamelist.xml nie zostal zmieniony."
}

Info ""
Ok "GOTOWE."
if ($network) {
    Info "Na konsoli: START > USTAWIENIA GIER > AKTUALIZUJ LISTY GIER (GAME SETTINGS > UPDATE GAMELISTS),"
    Info "albo po prostu uruchom konsole ponownie."
} else {
    Info "Bezpiecznie wysun karte, wloz ja do konsoli i wlacz konsole."
}
Info "Gra bedzie w menu: PORTS > Bobr Hopper."
Info "Jesli sie nie uruchomi: przynies plik roms\ports\bobrhopper\bobrhopper-launcher.log"
exit 0
