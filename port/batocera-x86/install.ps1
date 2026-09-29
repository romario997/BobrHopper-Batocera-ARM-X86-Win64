# Instalator Bobr Hopper na Batocere PC (x86_64). Uruchamiany przez WGRAJ_NA_BATOCERE_PC.bat.
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-Target <H:\ | H:\roms | \\BATOCERA\share | 192.168.1.50>] [-Yes]
# Kopiuje ports\ z tej paczki do <SHARE>\roms\ports (nic nie kasuje: ustawienia i rekord w bobrhopper\conf zostaja)
# i dopisuje gre do roms\ports\gamelist.xml (z kopia zapasowa), zeby w menu byla miniaturka.
# Batocera PC na pendrivie/dysku: partycja SHARE bywa exFAT (widoczna w Windows jako osobna litera, np. "ROMS") -
# wtedy wgrywamy wprost na nia; SHARE w ext4/btrfs Windows nie widzi - wtedy przez siec (\\BATOCERA\share).
# ZANIM cokolwiek zapisze, instalator pokazuje cel i pyta o zgode. -Yes pomija pytanie TYLKO razem z -Target (testy).
param([string]$Target = "", [switch]$Yes)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$srcPorts = Join-Path $here 'ports'
$entryFile = Join-Path $here 'gamelist-entry.xml'
$binName = 'bobrhopper.x86_64'

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

# z tego, co podal uzytkownik (litera, SHARE, roms albo ports, IP albo nazwa w sieci), robi folder roms
function Resolve-Roms([string]$t) {
    $t = $t.Trim().Trim('"').TrimEnd('\', '/')
    if ($t -match '^[A-Za-z]:?$') { $t = $t.Substring(0, 1) + ':' }
    if ($t -match '^\d{1,3}(\.\d{1,3}){3}$' -or $t -match '^[A-Za-z][\w-]*$' -and $t.Length -gt 1) { $t = "\\$t\share" }
    foreach ($c in @("$t\roms", $t, $(try { Split-Path -Parent $t } catch { $null }))) {   # Split-Path throws on a bare "E:"
        if (-not $c) { continue }
        if ((Split-Path -Leaf $c) -eq 'roms' -and (Test-PathTimeout $c)) { return $c }
    }
    return $null
}

# Batocera na tym samym dysku co litera (partycja startowa z boot\batocera.board): "x86_64", "rk3326", ... albo ""
function Get-BoardOnSameDisk([string]$letter) {
    try {
        $disk = (Get-Partition -DriveLetter $letter -ErrorAction Stop).DiskNumber
        foreach ($p in Get-Partition -DiskNumber $disk -ErrorAction Stop) {
            if (-not $p.DriveLetter) { continue }
            $board = "$($p.DriveLetter):\boot\batocera.board"
            if (Test-Path -LiteralPath $board) { return ((Get-Content -LiteralPath $board -TotalCount 1) -as [string]).Trim() }
        }
    } catch { }
    return ""
}

Info "=================================================="
Info " Bobr Hopper - wgrywanie na Batocere PC (x86_64)"
Info "=================================================="
Info ""

$roms = $null
if ($Target) {
    $roms = Resolve-Roms $Target
    if (-not $roms) { Fail "pod '$Target' nie ma folderu roms." }
} else {
    Info "Szukam partycji SHARE Batocery w komputerze (tylko odczyt)..."
    $found = @()
    foreach ($d in Get-PSDrive -PSProvider FileSystem) {
        $root = $d.Root.TrimEnd('\')
        if ($root -eq $env:SystemDrive -or $root -notmatch '^[A-Za-z]:$') { continue }
        try {
            if ((Test-Path -LiteralPath "$root\roms" -ErrorAction Stop) -and (Test-Path -LiteralPath "$root\system\batocera.conf")) {
                $version = ""
                try { $version = ((Get-Content -LiteralPath "$root\system\data.version" -TotalCount 1) -as [string]).Trim() } catch { }
                $found += [pscustomobject]@{ Roms = "$root\roms"; Version = $version; Board = (Get-BoardOnSameDisk $root.Substring(0, 1)) }
            }
        } catch { }
    }
    if ($found.Count) {
        Info ""
        Info "Znalezione partycje SHARE Batocery:"
        for ($i = 0; $i -lt $found.Count; $i++) {
            $f = $found[$i]
            $note = if ($f.Board -and $f.Board -ne 'x86_64') { "  <- UWAGA: to Batocera $($f.Board), nie PC - ta paczka jest dla x86_64" } else { "" }
            Info ("  [{0}] {1}   (Batocera {2}{3}){4}" -f ($i + 1), $f.Roms, $(if ($f.Version) { $f.Version } else { '?' }),
                $(if ($f.Board) { ", $($f.Board)" } else { "" }), $note)
        }
        $pick = if ($found.Count -eq 1) { "1" } else { Read-Host "Ktora wybrac? (numer, puste = zadna)" }
        if ($pick -match '^\d+$' -and [int]$pick -ge 1 -and [int]$pick -le $found.Count) {
            $f = $found[[int]$pick - 1]
            if ($f.Board -and $f.Board -ne 'x86_64') {
                Warn "Wybrana partycja nalezy do Batocery $($f.Board) - gra z tej paczki tam nie ruszy."
            } else { $roms = $f.Roms }
        }
    }
    if (-not $roms) {
        Info "Szukam Batocery w sieci (\\BATOCERA\share)... do 20 sekund"
        if (Test-PathTimeout '\\BATOCERA\share\roms') { $roms = '\\BATOCERA\share\roms' }
    }
    while (-not $roms) {
        Info ""
        Warn "Nie znalazlem Batocery automatycznie."
        Info "Podaj litere dysku z partycja SHARE (np. H) albo adres IP komputera z Batocera w sieci"
        Info "(w Batocerze: MENU > USTAWIENIA SIECI / NETWORK SETTINGS)."
        $ans = Read-Host "Litera albo IP, puste = wyjscie"
        if (-not $ans) { Fail "przerwano." }
        $roms = Resolve-Roms $ans
        if (-not $roms) { Warn "Pod '$ans' nie ma folderu roms." }
    }
}

$ports = Join-Path $roms 'ports'
$network = $roms.StartsWith('\\')
Info ""
Ok "Cel: $ports"
Info "Zostana tam zapisane: BobrHopper.sh, folder bobrhopper\, obrazki w images\ i wpis w gamelist.xml (z kopia zapasowa)."
$answer = if ($Yes -and $Target) { "T" } else { Read-Host "Wgrac tutaj gre? [T/n]" }
if ($answer -and $answer -notmatch '^[TtYy]') { Fail "przerwano - nic nie zostalo zapisane." }

# --- kopiowanie (robocopy bez /PURGE: niczego nie kasuje, conf\ z ustawieniami zostaje) ------------------------
if (-not (Test-Path -LiteralPath $ports)) { New-Item -ItemType Directory -Path $ports | Out-Null }
Info "Kopiuje pliki gry..."
& robocopy.exe $srcPorts $ports /E /R:2 /W:2 /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { Fail "kopiowanie nie powiodlo sie (robocopy kod $LASTEXITCODE)." }

# --- sprawdzenie ----------------------------------------------------------------------------------------------
$srcBin = Join-Path $srcPorts "bobrhopper\$binName"
$dstBin = Join-Path $ports "bobrhopper\$binName"
if ((Get-Item -LiteralPath $dstBin).Length -ne (Get-Item -LiteralPath $srcBin).Length) { Fail "plik gry $binName ma zly rozmiar - sprobuj jeszcze raz." }
if ([System.IO.File]::ReadAllBytes((Join-Path $ports 'BobrHopper.sh')) -contains 13) { Fail "BobrHopper.sh na Batocerze ma konce linii Windows." }
$srcCount = (Get-ChildItem -LiteralPath (Join-Path $srcPorts 'bobrhopper\data') -Recurse -File).Count
$dstCount = (Get-ChildItem -LiteralPath (Join-Path $ports 'bobrhopper\data') -Recurse -File).Count
if ($dstCount -lt $srcCount) { Fail "brakuje plikow danych ($dstCount z $srcCount)." }
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
    Info "W Batocerze: MENU > USTAWIENIA GIER > AKTUALIZUJ LISTY GIER (GAME SETTINGS > UPDATE GAMELISTS),"
    Info "albo po prostu uruchom Batocere ponownie."
} else {
    Info "Bezpiecznie odlacz dysk/pendrive (Wysun) i uruchom z niego Batocere."
}
Info "Gra bedzie w menu: PORTS > Bobr Hopper."
Info "Jesli sie nie uruchomi: przynies plik roms\ports\bobrhopper\bobrhopper-launcher.log"
exit 0
