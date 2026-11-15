# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/compat/Merge-DashboardAbi.ps1
# PURPOSE:     Merge plugin ABI scores into the dashboard snapshot
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$WebsitePath = "",
    [string]$SnapshotPath = "",
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

function Format-NameList {
    param([string[]]$Names)
    if ($Names.Count -eq 1) { return $Names[0] }
    $head = $Names[0..($Names.Count - 2)] -join ", "
    return "$head, and $($Names[-1])"
}

function Format-Score {
    param($Number)
    $rounded = [math]::Round([double]$Number, 1)
    if ([math]::Abs($rounded - [math]::Truncate($rounded)) -lt 0.001) {
        return ([int64][math]::Truncate($rounded)).ToString([Globalization.CultureInfo]::InvariantCulture)
    }
    return $rounded.ToString("0.0", [Globalization.CultureInfo]::InvariantCulture)
}

function Get-PluginScoreCount {
    param($Items, [string]$Category)
    $count = 0
    foreach ($item in @($Items)) {
        if ([string]$item.category -ne $Category) { continue }
        if ([string]$item.source -ne "plugin") { continue }
        $status = [string]$item.status
        if ($status -eq "works" -or $status -eq "partial" -or $status -eq "broken") { $count++ }
    }
    return $count
}

function Get-CategoryPercent {
    param($Categories, [string]$Id)
    foreach ($cat in @($Categories)) {
        if ([string]$cat.id -eq $Id) { return $cat.percent }
    }
    return $null
}

function Update-ProbeNote {
    param([string]$Notes, [bool]$KernelMeasured, [bool]$DriverMeasured)
    $names = @()
    if (-not $KernelMeasured) { $names += "kernelAbi" }
    $names += @("lastBuild", "applications", "drivers", "subsystems", "traceCorpus", "task")
    if (-not $DriverMeasured) { $names += "driverAbi" }
    $names += "graphics"
    $sentence = " " + (Format-NameList $names) + " were not measured by this probe."
    $updated = [regex]::Replace($Notes, ' [^.]*were not measured by this probe\.', $sentence, 1)
    $updated = [regex]::Replace($updated, ' kernelAbi is [0-9.]+ from the kernel-exports category \(\d+ exports? returned or bugchecked; log [^)]+\)\. Untested rows stay out of that percent\.', '')
    $updated = [regex]::Replace($updated, ' driverAbi is [0-9.]+ from the driver ABI category \(\d+ exports? returned or bugchecked; log [^)]+\)\. Untested rows stay out of that percent\.', '')
    return $updated
}

function Add-MeasurementSentence {
    param([string]$Notes, [string]$Field, [string]$CategoryLabel, $Percent, [int]$Count, [string]$Log)
    $exportWord = "exports"
    if ($Count -eq 1) { $exportWord = "export" }
    $score = Format-Score $Percent
    return $Notes + " $Field is $score from the $CategoryLabel ($Count $exportWord returned or bugchecked; log $Log). Untested rows stay out of that percent."
}

function Sync-Gap {
    param($Gaps, [string]$Section, [string]$Reason, [bool]$Measured)
    $list = New-Object System.Collections.Generic.List[object]
    $found = $false
    foreach ($gap in @($Gaps)) {
        if ($null -eq $gap) { continue }
        if ([string]$gap.section -eq $Section) {
            $found = $true
            if (-not $Measured) { $list.Add($gap) | Out-Null }
            continue
        }
        $list.Add($gap) | Out-Null
    }
    if (-not $Measured -and -not $found) {
        $list.Add([ordered]@{ section = $Section; reason = $Reason }) | Out-Null
    }
    return ,$list.ToArray()
}

function Test-SameScore {
    param($Left, $Right)
    $leftMissing = $null -eq $Left
    $rightMissing = $null -eq $Right
    if ($leftMissing -and $rightMissing) { return $true }
    if ($leftMissing -or $rightMissing) { return $false }
    return [math]::Abs([double]$Left - [double]$Right) -lt 0.001
}

function Update-WebsiteAbi {
    param([string]$WebsiteFile, [string]$SnapshotFile)
    if (-not (Test-Path -LiteralPath $WebsiteFile)) {
        Write-Host "No website snapshot at $WebsiteFile"
        return $false
    }
    if (-not (Test-Path -LiteralPath $SnapshotFile)) {
        Write-Host "No compatibility snapshot at $SnapshotFile"
        return $false
    }

    $site = Get-Content -LiteralPath $WebsiteFile -Raw | ConvertFrom-Json
    $snap = Get-Content -LiteralPath $SnapshotFile -Raw | ConvertFrom-Json
    $log = [string]$snap.pluginLog
    $driverCount = 0
    $driverPercent = $null
    if ($log) {
        $driverCount = Get-PluginScoreCount -Items $snap.items -Category "driverAbi"
        if ($driverCount -gt 0) { $driverPercent = Get-CategoryPercent -Categories $snap.categories -Id "driverAbi" }
    }
    $kernelMeasured = $null -ne $site.kernelAbi
    $driverMeasured = $driverCount -gt 0 -and $null -ne $driverPercent

    $notes = Update-ProbeNote -Notes ([string]$site.notes) -KernelMeasured $kernelMeasured -DriverMeasured $driverMeasured
    if ($driverMeasured) {
        $notes = Add-MeasurementSentence -Notes $notes -Field "driverAbi" -CategoryLabel "driver ABI category" -Percent $driverPercent -Count $driverCount -Log $log
    }

    $gaps = $site.unmeasured
    $gaps = Sync-Gap -Gaps $gaps -Section "kernelAbi" -Reason "The Windows 10 22H2 kernel and the VerOS kernel have not both finished the kernel probe." -Measured $kernelMeasured
    $gaps = Sync-Gap -Gaps $gaps -Section "driverAbi" -Reason "The snapshot has no driver ABI export that returned or bugchecked in the plugin log." -Measured $driverMeasured

    $unchanged = (Test-SameScore $site.driverAbi $driverPercent) -and ([string]$site.notes -eq $notes)
    if ($unchanged) {
        $before = @($site.unmeasured | ForEach-Object { [string]$_.section + "|" + [string]$_.reason }) -join "`n"
        $after = @($gaps | ForEach-Object { [string]$_.section + "|" + [string]$_.reason }) -join "`n"
        if ($before -eq $after) {
            Write-Host "Website ABI fields already match $WebsiteFile"
            return $false
        }
    }

    $site.driverAbi = $(if ($driverMeasured) { [double]$driverPercent } else { $null })
    $site.notes = $notes
    $site.unmeasured = @($gaps)
    $dashRoot = Split-Path -Parent $WebsiteFile
    . (Join-Path $PSScriptRoot "..\dashboard\DashboardStore.ps1")
    $captured = [string]$site.capturedAt
    if (-not $captured) { $captured = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ") }
    Write-DashboardSection -DashboardRoot $dashRoot -Name "driver-abi" -CapturedAt $captured -Values ([ordered]@{
        driverAbi = $site.driverAbi
    })
    Update-DashboardSection -DashboardRoot $dashRoot -Name "syscalls" -CapturedAt $captured -Values ([ordered]@{
        notes      = [string]$site.notes
        unmeasured = @($site.unmeasured)
    })
    Write-Host "Updated $WebsiteFile kernelAbi=$($site.kernelAbi) driverAbi=$($site.driverAbi)"
    return $true
}

function New-WebsiteFixture {
    param([string]$Note)
    return [ordered]@{
        spdx           = "GPL-2.0-or-later"
        schemaVersion  = 1
        sample         = $false
        overall        = 78.2
        kernelAbi      = $null
        driverAbi      = $null
        syscallOverall = 78.2
        notes          = $Note
        unmeasured     = @(
            [ordered]@{ section = "kernelAbi"; reason = "The i386 probe does not call ntoskrnl, HAL, or KMDF exports." }
            [ordered]@{ section = "lastBuild"; reason = "The guest banner is a date and commit, not a monotonic build number." }
            [ordered]@{ section = "driverAbi"; reason = "The i386 probe does not exercise IRP, PnP, power, DMA, or KMDF." }
            [ordered]@{ section = "graphics"; reason = "The i386 probe does not exercise the display stack." }
        )
    }
}

function Invoke-MergeSelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("abi-merge-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $root | Out-Null
    try {
        $note = "overall and syscallOverall are exact status matches from the i386 probe: 1 of 1 names. kernelAbi, lastBuild, applications, drivers, subsystems, traceCorpus, task, driverAbi, and graphics were not measured by this probe. The formula weights are the website contract and were not applied."
        $website = Join-Path $root "latest.json"
        $snapshot = Join-Path $root "snapshot.json"
        $utf8 = New-Object System.Text.UTF8Encoding $false
        $original = ConvertTo-PlainJson (New-WebsiteFixture $note)
        [System.IO.File]::WriteAllText($website, $original + [Environment]::NewLine, $utf8)

        $idle = [ordered]@{
            pluginLog  = $null
            categories = @(
                [ordered]@{ id = "kernel"; percent = $null }
                [ordered]@{ id = "driverAbi"; percent = 0 }
            )
            items = @(
                [ordered]@{ name = "UMDF"; category = "driverAbi"; status = "missing"; source = "manual" }
            )
        }
        [System.IO.File]::WriteAllText($snapshot, (ConvertTo-PlainJson $idle) + [Environment]::NewLine, $utf8)
        if (Update-WebsiteAbi -WebsiteFile $website -SnapshotFile $snapshot) { throw "an unscored snapshot changed the website file" }
        $idleText = [System.IO.File]::ReadAllText($website)
        if ($idleText -ne ($original + [Environment]::NewLine)) { throw "an unscored snapshot rewrote the website file" }

        $scored = [ordered]@{
            pluginLog  = "output/plugin.jsonl"
            categories = @(
                [ordered]@{ id = "kernel"; percent = 100 }
                [ordered]@{ id = "driverAbi"; percent = 50 }
            )
            items = @(
                [ordered]@{ name = "ExAllocatePool"; category = "kernel"; status = "works"; source = "plugin" }
                [ordered]@{ name = "IoCallDriver"; category = "driverAbi"; status = "works"; source = "plugin" }
                [ordered]@{ name = "UMDF"; category = "driverAbi"; status = "missing"; source = "manual" }
                [ordered]@{ name = "KeWaitForSingleObject"; category = "kernel"; status = "untested"; source = "spec" }
            )
        }
        [System.IO.File]::WriteAllText($snapshot, (ConvertTo-PlainJson $scored) + [Environment]::NewLine, $utf8)
        if (-not (Update-WebsiteAbi -WebsiteFile $website -SnapshotFile $snapshot)) { throw "a plugin score did not update the website file" }
        $filled = Get-Content -LiteralPath $website -Raw | ConvertFrom-Json
        if ($filled.overall -ne 78.2 -or $filled.syscallOverall -ne 78.2) { throw "syscall scores changed" }
        if ($null -ne $filled.kernelAbi -or $filled.driverAbi -ne 50) { throw "kernel ABI was copied from the plugin, or driver ABI was not" }
        if ($filled.notes -match "from the kernel-exports category") { throw "plugin kernel note was copied" }
        if ($filled.notes -notmatch "driverAbi is 50 from the driver ABI category \(1 export returned or bugchecked; log output/plugin\.jsonl\)") { throw "driver note missing" }
        if ($filled.notes -notmatch "kernelAbi, lastBuild") { throw "unmeasured kernelAbi left the sentence" }
        $gapNames = @($filled.unmeasured | ForEach-Object { [string]$_.section })
        if (-not ($gapNames -contains "kernelAbi") -or $gapNames -contains "driverAbi") { throw "kernel gap missing or driver gap remained" }
        if (-not ($gapNames -contains "lastBuild" -and $gapNames -contains "graphics")) { throw "other unmeasured sections were dropped" }
        $once = [System.IO.File]::ReadAllText($website)
        if (Update-WebsiteAbi -WebsiteFile $website -SnapshotFile $snapshot) { throw "a second merge rewrote a matching website file" }
        if ([System.IO.File]::ReadAllText($website) -ne $once) { throw "a second merge changed the website file" }

        $cleared = [ordered]@{
            pluginLog  = "output/plugin.jsonl"
            categories = @(
                [ordered]@{ id = "kernel"; percent = $null }
                [ordered]@{ id = "driverAbi"; percent = $null }
            )
            items = @(
                [ordered]@{ name = "ExAllocatePool"; category = "kernel"; status = "untested"; source = "spec" }
            )
        }
        [System.IO.File]::WriteAllText($snapshot, (ConvertTo-PlainJson $cleared) + [Environment]::NewLine, $utf8)
        if (-not (Update-WebsiteAbi -WebsiteFile $website -SnapshotFile $snapshot)) { throw "a snapshot with no plugin score left the old percents" }
        $empty = Get-Content -LiteralPath $website -Raw | ConvertFrom-Json
        if ($null -ne $empty.kernelAbi -or $null -ne $empty.driverAbi) { throw "cleared ABI fields were still filled" }
        if ($empty.overall -ne 78.2) { throw "clearing ABI fields changed the syscall score" }
        $restored = @($empty.unmeasured | ForEach-Object { [string]$_.section })
        if (-not ($restored -contains "kernelAbi" -and $restored -contains "driverAbi")) { throw "cleared sections were not listed as unmeasured" }
        if ($empty.notes -match "from the kernel-exports category" -or $empty.notes -match "from the driver ABI category") { throw "cleared measurement sentences remained" }
        Write-Host "ABI merge fixture passed."
    }
    finally {
        if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
    }
}

if ($SelfTest) {
    Invoke-MergeSelfTest
    exit 0
}

if (-not $WebsitePath) { $WebsitePath = Join-Path $SourceRoot "veros\dashboard\latest.json" }
if (-not $SnapshotPath) { $SnapshotPath = Join-Path $SourceRoot "dashboard\data\snapshot.json" }
Update-WebsiteAbi -WebsiteFile $WebsitePath -SnapshotFile $SnapshotPath | Out-Null
