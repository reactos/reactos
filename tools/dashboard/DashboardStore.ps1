# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/dashboard/DashboardStore.ps1
# PURPOSE:     Store each dashboard probe in its own file
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = "",
    [switch]$Convert,
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function ConvertTo-PlainJson {
    param($Value, [int]$Indent = 0)
    $pad = "  " * $Indent
    $next = "  " * ($Indent + 1)
    if ($null -eq $Value) { return "null" }
    if ($Value -is [bool]) {
        if ($Value) { return "true" }
        return "false"
    }
    if ($Value -is [string]) {
        $text = $Value.Replace("\", "\\").Replace('"', '\"').Replace("`r", "\r").Replace("`n", "\n")
        return '"' + $text + '"'
    }
    $isWhole = $Value -is [int] -or $Value -is [long] -or $Value -is [decimal]
    $isFloat = $Value -is [double] -or $Value -is [float] -or $Value -is [single]
    if ($isWhole -or $isFloat) {
        $number = [double]$Value
        $rounded = [math]::Round($number, 1)
        if ([math]::Abs($rounded - [math]::Truncate($rounded)) -lt 0.001) {
            return ([int64][math]::Truncate($rounded)).ToString([Globalization.CultureInfo]::InvariantCulture)
        }
        return $rounded.ToString("0.0", [Globalization.CultureInfo]::InvariantCulture)
    }
    if ($Value -is [System.Collections.IList]) {
        if ($Value.Count -eq 0) { return "[]" }
        $lines = @()
        foreach ($item in $Value) {
            $lines += ($next + (ConvertTo-PlainJson -Value $item -Indent ($Indent + 1)))
        }
        return "[" + [Environment]::NewLine + ($lines -join ("," + [Environment]::NewLine)) + [Environment]::NewLine + $pad + "]"
    }
    $map = $Value
    if ($Value -is [pscustomobject]) {
        $map = [ordered]@{}
        foreach ($prop in $Value.PSObject.Properties) { $map[$prop.Name] = $prop.Value }
    }
    if ($map -is [System.Collections.IDictionary]) {
        if ($map.Count -eq 0) { return "{}" }
        $lines = @()
        foreach ($key in @($map.Keys)) {
            $encoded = ConvertTo-PlainJson -Value $map[$key] -Indent ($Indent + 1)
            $lines += ($next + '"' + $key + '": ' + $encoded)
        }
        return "{" + [Environment]::NewLine + ($lines -join ("," + [Environment]::NewLine)) + [Environment]::NewLine + $pad + "}"
    }
    throw "Unsupported JSON value: $($Value.GetType().FullName)"
}

function Write-JsonFile {
    param([string]$Path, $Value)
    $directory = Split-Path -Parent $Path
    if ($directory -and -not (Test-Path -LiteralPath $directory)) {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
    }
    $json = ConvertTo-PlainJson $Value
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $utf8)
}

function Get-DashboardCatalog {
    return @(
        [pscustomobject]@{ Name = "syscalls"; Keys = @("spdx", "schemaVersion", "sample", "target", "overall", "syscallOverall", "syscalls", "divergences", "northStar", "formula", "lastHarnessRun", "notes", "unmeasured", "lastBuild"); History = @("overall", "syscallOverall", "syscalls") },
        [pscustomobject]@{ Name = "kernel-abi"; Keys = @("kernelAbi", "kernelAbiDetail"); History = @("kernelAbi") },
        [pscustomobject]@{ Name = "applications"; Keys = @("applicationsSummary", "applications"); History = @("applicationsSummary") },
        [pscustomobject]@{ Name = "drivers"; Keys = @("driversSummary", "driverGroups"); History = @("driversSummary") },
        [pscustomobject]@{ Name = "graphics"; Keys = @("graphics"); History = @("graphics") },
        [pscustomobject]@{ Name = "sdk-targets"; Keys = @("sdkTargets", "sdkTargetsSummary", "sdkTargetsNote", "sdkLevel"); History = @("sdkLevel", "sdkTargetsSummary", "sdkTargetsNote") },
        [pscustomobject]@{ Name = "driver-abi"; Keys = @("driverAbi"); History = @("driverAbi") },
        [pscustomobject]@{ Name = "traces"; Keys = @("traceCorpus", "traceWorkloads"); History = @() },
        [pscustomobject]@{ Name = "areas"; Keys = @("subsystems", "highestImpact", "task"); History = @() },
        [pscustomobject]@{ Name = "regression"; Keys = @("regression"); History = @() }
    )
}

function Get-DashboardPaths {
    param([string]$SourceRoot, [string]$DashboardRoot)
    if (-not $DashboardRoot) {
        if (-not $SourceRoot) { throw "A dashboard root is required" }
        $DashboardRoot = Join-Path $SourceRoot "tavera\dashboard"
    }
    return [pscustomobject]@{
        Root     = $DashboardRoot
        Sections = (Join-Path $DashboardRoot "sections")
        History  = (Join-Path $DashboardRoot "history")
        Bundle   = (Join-Path $DashboardRoot "latest.json")
        Index    = (Join-Path $DashboardRoot "history\index.json")
        Legacy   = (Join-Path $DashboardRoot "history.json")
    }
}

function Get-DashValue {
    param($Object, [string]$Name)
    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $prop = $Object.PSObject.Properties[$Name]
    if ($prop) { return $prop.Value }
    return $null
}

function Test-DashPresent {
    param($Value)
    if ($null -eq $Value) { return $false }
    if ($Value -is [string]) { return $Value.Length -gt 0 }
    if ($Value -is [System.Collections.IList]) { return $Value.Count -gt 0 }
    if ($Value -is [System.Collections.IDictionary]) {
        foreach ($item in @($Value.Values)) {
            if (Test-DashPresent $item) { return $true }
        }
        return $false
    }
    if ($Value -is [pscustomobject]) {
        foreach ($prop in @($Value.PSObject.Properties)) {
            if (Test-DashPresent $prop.Value) { return $true }
        }
        return $false
    }
    return $true
}

function Get-Stamp {
    param([string]$CapturedAt)
    $styles = [Globalization.DateTimeStyles]::RoundtripKind
    $culture = [Globalization.CultureInfo]::InvariantCulture
    $when = [datetime]::MinValue
    if (-not [datetime]::TryParse($CapturedAt, $culture, $styles, [ref]$when)) {
        throw "Unparsed dashboard time: [$CapturedAt]"
    }
    $utc = $when.ToUniversalTime()
    return [pscustomobject]@{
        At   = $utc.ToString("yyyy-MM-ddTHH:mm:ssZ")
        Day  = $utc.ToString("yyyy-MM-dd")
        File = $utc.ToString("yyyy-MM-ddTHHmmssZ")
    }
}

function Get-GraphicsPercent {
    param($Rows)
    $scores = @()
    foreach ($row in @($Rows)) {
        if ($null -eq $row) { continue }
        $text = ([string]$row.value).Trim().ToLowerInvariant()
        if ([string]$row.tone -eq "score" -or $text.EndsWith("%")) {
            $clean = $text.TrimEnd("%")
            $number = 0.0
            if ([double]::TryParse($clean, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$number)) {
                $scores += $number
            }
            continue
        }
        if ($text -eq "untested" -or $text -eq "unknown" -or $text -eq "") { continue }
        if ($text -eq "working" -or $text -eq "ready" -or $text -eq "ok") { $scores += 100.0 }
        elseif ($text -eq "partial") { $scores += 50.0 }
        elseif ($text -eq "missing" -or $text -eq "failing") { $scores += 0.0 }
    }
    if ($scores.Count -eq 0) { return $null }
    return [math]::Round(($scores | Measure-Object -Average).Average, 1)
}

function Read-JsonFile {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    return (Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json)
}

function Get-OwnedValues {
    param([string]$Name, $Source, [switch]$SkipEmpty)
    $spec = @(Get-DashboardCatalog | Where-Object { $_.Name -eq $Name })[0]
    if (-not $spec) { throw "Unknown dashboard section $Name" }
    $values = [ordered]@{}
    $any = $false
    foreach ($key in @($spec.Keys)) {
        $has = $false
        if ($Source -is [System.Collections.IDictionary]) { $has = $Source.Contains($key) }
        elseif ($Source.PSObject.Properties[$key]) { $has = $true }
        if (-not $has) { continue }
        $value = Get-DashValue $Source $key
        if ($SkipEmpty -and -not (Test-DashPresent $value)) { continue }
        $values[$key] = $value
        if (Test-DashPresent $value) { $any = $true }
    }
    return [pscustomobject]@{ Values = $values; Any = $any; History = @($spec.History) }
}

function Save-ChosenSections {
    param($Chosen, $Paths)
    foreach ($name in @($Chosen.Keys)) {
        $item = $Chosen[$name]
        $section = [ordered]@{ capturedAt = (Get-Stamp $item.At).At; section = $name }
        foreach ($key in @($item.Values.Keys)) { $section[$key] = $item.Values[$key] }
        Write-JsonFile (Join-Path $Paths.Sections ($name + ".json")) $section
    }
}

function Import-BundleIfEmpty {
    param($Paths)
    if (Test-Path -LiteralPath $Paths.Sections) {
        $existing = @(Get-ChildItem -LiteralPath $Paths.Sections -Filter "*.json" -ErrorAction SilentlyContinue)
        if ($existing.Count -gt 0) { return }
    }
    $bundle = Read-JsonFile $Paths.Bundle
    if (-not $bundle) { return }
    if (-not (Get-DashValue $bundle "capturedAt")) {
        $bundle | Add-Member -NotePropertyName capturedAt -NotePropertyValue "1970-01-01T00:00:00Z" -Force
    }
    $chosen = @{}
    Import-DashboardSnapshot -Snapshot $bundle -Paths $Paths -Chosen $chosen
    Save-ChosenSections -Chosen $chosen -Paths $Paths
}

function Write-DashboardSection {
    param(
        [string]$SourceRoot = "",
        [string]$DashboardRoot = "",
        [string]$Name,
        [string]$CapturedAt,
        $Values
    )
    $paths = Get-DashboardPaths -SourceRoot $SourceRoot -DashboardRoot $DashboardRoot
    Import-BundleIfEmpty $paths
    $stamp = Get-Stamp $CapturedAt
    $owned = Get-OwnedValues -Name $Name -Source $Values
    $section = [ordered]@{ capturedAt = $stamp.At; section = $Name }
    foreach ($key in @($owned.Values.Keys)) { $section[$key] = $owned.Values[$key] }
    Write-JsonFile (Join-Path $paths.Sections ($Name + ".json")) $section

    if ($owned.History.Count -gt 0) {
        $archive = [ordered]@{ capturedAt = $stamp.At; section = $Name }
        foreach ($key in $owned.History) {
            if ($owned.Values.Contains($key)) { $archive[$key] = $owned.Values[$key] }
        }
        Write-JsonFile (Join-Path $paths.History ($stamp.File + "-" + $Name + ".json")) $archive
    }
    Update-TrendIndex -Paths $paths -Name $Name -Stamp $stamp -Values $owned.Values
    Update-DashboardBundle -DashboardRoot $paths.Root
}

function Update-DashboardSection {
    param(
        [string]$SourceRoot = "",
        [string]$DashboardRoot = "",
        [string]$Name,
        [string]$CapturedAt,
        $Values
    )
    $paths = Get-DashboardPaths -SourceRoot $SourceRoot -DashboardRoot $DashboardRoot
    $current = Read-JsonFile (Join-Path $paths.Sections ($Name + ".json"))
    $merged = [ordered]@{}
    if ($current) {
        foreach ($prop in $current.PSObject.Properties) {
            if ($prop.Name -eq "capturedAt" -or $prop.Name -eq "section") { continue }
            $merged[$prop.Name] = $prop.Value
        }
    }
    $overlay = Get-OwnedValues -Name $Name -Source $Values
    foreach ($key in @($overlay.Values.Keys)) { $merged[$key] = $overlay.Values[$key] }
    if (-not $CapturedAt -and $current) { $CapturedAt = [string]$current.capturedAt }
    Write-DashboardSection -DashboardRoot $paths.Root -Name $Name -CapturedAt $CapturedAt -Values $merged
}

function Update-TrendIndex {
    param($Paths, [string]$Name, $Stamp, $Values)
    $score = $null
    $field = $null
    switch ($Name) {
        "syscalls" {
            $field = "syscall"
            $score = Get-DashValue $Values "syscallOverall"
            if ($null -eq $score) { $score = Get-DashValue $Values "overall" }
        }
        "kernel-abi" { $field = "kernelAbi"; $score = Get-DashValue $Values "kernelAbi" }
        "graphics" { $field = "graphics"; $score = Get-GraphicsPercent (Get-DashValue $Values "graphics") }
        "sdk-targets" { $field = "sdkLevel"; $score = Get-DashValue $Values "sdkLevel" }
    }
    if (-not $field -or $null -eq $score) { return }
    $rows = @()
    $parsed = Read-JsonFile $Paths.Index
    if ($parsed) { $rows = @($parsed) }
    $found = $false
    $next = @()
    foreach ($row in $rows) {
        $day = (Get-Stamp ([string]$row.at)).Day
        if ($day -eq $Stamp.Day) {
            $found = $true
            $row | Add-Member -NotePropertyName $field -NotePropertyValue ([math]::Round([double]$score, 1)) -Force
            if ([string]$row.at -lt $Stamp.At) { $row.at = $Stamp.At }
        }
        $next += $row
    }
    if (-not $found) {
        $point = [ordered]@{ at = $Stamp.At; kernelAbi = $null; syscall = $null; graphics = $null; sdkLevel = $null }
        $point[$field] = [math]::Round([double]$score, 1)
        $next += [pscustomobject]$point
    }
    $ordered = @($next | Sort-Object { [string]$_.at })
    Write-JsonFile $Paths.Index $ordered
}

function Update-DashboardBundle {
    param([string]$SourceRoot = "", [string]$DashboardRoot = "")
    $paths = Get-DashboardPaths -SourceRoot $SourceRoot -DashboardRoot $DashboardRoot
    $bundle = [ordered]@{
        spdx          = "GPL-2.0-or-later"
        schemaVersion = 1
        sample        = $false
        capturedAt    = $null
        target        = "Win10 22H2"
    }
    $newest = ""
    foreach ($spec in @(Get-DashboardCatalog)) {
        $section = Read-JsonFile (Join-Path $paths.Sections ($spec.Name + ".json"))
        if (-not $section) { continue }
        foreach ($key in @($spec.Keys)) {
            $bundle[$key] = Get-DashValue $section $key
        }
        $at = [string]$section.capturedAt
        if ($at -and $at -gt $newest) { $newest = $at }
    }
    if ($newest) { $bundle.capturedAt = $newest }
    $trend = Read-JsonFile $Paths.Index
    if ($trend) { $bundle.compatibilityTrend = @($trend) }
    Write-JsonFile $Paths.Bundle $bundle
    Write-SyscallHistoryRollup $paths
}

function Write-SyscallHistoryRollup {
    param($Paths)
    if (-not (Test-Path -LiteralPath $Paths.History)) { return }
    $files = @(Get-ChildItem -LiteralPath $Paths.History -Filter "*-syscalls.json" | Sort-Object Name)
    $points = @()
    foreach ($file in $files) {
        $row = Read-JsonFile $file.FullName
        if ($row) { $points += $row }
    }
    if ($points.Count -gt 0) { Write-JsonFile $Paths.Legacy $points }
}

function Import-DashboardSnapshot {
    param($Snapshot, $Paths, $Chosen)
    $at = [string](Get-DashValue $Snapshot "capturedAt")
    if (-not $at) { return }
    foreach ($spec in @(Get-DashboardCatalog)) {
        $owned = Get-OwnedValues -Name $spec.Name -Source $Snapshot -SkipEmpty
        if (-not $owned.Any) { continue }
        $values = [ordered]@{}
        if ($Chosen.ContainsKey($spec.Name)) {
            foreach ($key in @($Chosen[$spec.Name].Values.Keys)) {
                $values[$key] = $Chosen[$spec.Name].Values[$key]
            }
        }
        foreach ($key in @($owned.Values.Keys)) { $values[$key] = $owned.Values[$key] }
        $Chosen[$spec.Name] = [pscustomobject]@{ At = $at; Values = $values; History = $owned.History }
        if ($owned.History.Count -eq 0) { continue }
        $stamp = Get-Stamp $at
        $archive = [ordered]@{ capturedAt = $stamp.At; section = $spec.Name }
        foreach ($key in $owned.History) {
            if ($owned.Values.Contains($key)) { $archive[$key] = $owned.Values[$key] }
        }
        Write-JsonFile (Join-Path $paths.History ($stamp.File + "-" + $spec.Name + ".json")) $archive
        Update-TrendIndex -Paths $paths -Name $spec.Name -Stamp $stamp -Values $owned.Values
    }
}

function Convert-DashboardStore {
    param([string]$SourceRoot)
    $paths = Get-DashboardPaths -SourceRoot $SourceRoot
    $snaps = New-Object System.Collections.Generic.List[object]
    $legacy = Read-JsonFile $paths.Legacy
    if ($legacy) {
        foreach ($row in @($legacy)) { $snaps.Add($row) | Out-Null }
    }
    Push-Location $SourceRoot
    try { $shas = @(& git log --format=%H -- "tavera/dashboard/latest.json") }
    finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { throw "git log failed while converting the dashboard" }
    [array]::Reverse($shas)
    foreach ($sha in @($shas | Where-Object { $_ })) {
        Push-Location $SourceRoot
        try { $raw = & git show ($sha.Trim() + ":tavera/dashboard/latest.json") }
        finally { Pop-Location }
        if ($LASTEXITCODE -ne 0 -or -not $raw) { continue }
        $snaps.Add(($raw | Out-String | ConvertFrom-Json)) | Out-Null
    }
    $chosen = @{}
    foreach ($snap in $snaps) { Import-DashboardSnapshot -Snapshot $snap -Paths $paths -Chosen $chosen }
    Save-ChosenSections -Chosen $chosen -Paths $paths
    Update-DashboardBundle -DashboardRoot $paths.Root
    Write-Host ("Converted " + $chosen.Count + " dashboard sections into " + $paths.Sections)
}

function Invoke-StoreSelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("dash-store-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $root | Out-Null
    try {
        Write-DashboardSection -DashboardRoot $root -Name "syscalls" -CapturedAt "2026-11-08T00:21:28Z" -Values ([ordered]@{
            overall = 80.0; syscallOverall = 80.0; notes = "kept"; syscalls = @([ordered]@{ name = "NtClose"; compatibility = 100 })
        })
        Write-DashboardSection -DashboardRoot $root -Name "graphics" -CapturedAt "2026-11-09T02:07:21Z" -Values ([ordered]@{
            graphics = @([ordered]@{ label = "Frame"; value = "working"; tone = "ok" })
        })
        Write-DashboardSection -DashboardRoot $root -Name "syscalls" -CapturedAt "2026-11-11T14:16:26Z" -Values ([ordered]@{
            overall = 87.1; syscallOverall = 87.1; notes = "kept"
        })
        $bundle = Read-JsonFile (Join-Path $root "latest.json")
        if ($bundle.notes -ne "kept") { throw "syscall notes were dropped" }
        if ($bundle.graphics[0].value -ne "working") { throw "graphics were dropped by the later syscall write" }
        if ($bundle.syscallOverall -ne 87.1) { throw "syscall score was not the newest section" }
        $graphics = Read-JsonFile (Join-Path $root "sections\graphics.json")
        if ($graphics.capturedAt -ne "2026-11-09T02:07:21Z") { throw "graphics kept the syscall timestamp" }
        $index = @(Read-JsonFile (Join-Path $root "history\index.json"))
        if ($index.Count -lt 2) { throw "trend index did not keep both days" }
        Write-Host "Dashboard store fixture passed."
    }
    finally {
        if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
    }
}

if ($MyInvocation.InvocationName -eq ".") { return }

if (-not $SourceRoot) {
    $SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}
if ($SelfTest) {
    Invoke-StoreSelfTest
    exit 0
}
if ($Convert) {
    Convert-DashboardStore -SourceRoot $SourceRoot
    exit 0
}
