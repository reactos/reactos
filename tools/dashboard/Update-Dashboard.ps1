# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/dashboard/Update-Dashboard.ps1
# PURPOSE:     Update the dashboard from the latest probes
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = "",
    [ValidateSet("All", "Syscalls", "KernelAbi", "Applications", "Drivers", "Regressions", "HeatMap", "Traces", "Tasks", "DriverAbi", "Graphics", "SdkTargets")]
    [string[]]$Section = @("All"),
    [string]$WebsitePath = "",
    [string]$DriverLog = "",
    [string]$AppCatalog = "",
    [switch]$SelfTest,
    [switch]$Repair
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
    $json = ConvertTo-PlainJson $Value
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $utf8)
}

function Get-SectionSet {
    param([string[]]$Names)
    if ($Names -contains "All") {
        return @("Syscalls", "KernelAbi", "Applications", "Drivers", "Regressions", "HeatMap", "Traces", "Tasks", "DriverAbi", "Graphics", "SdkTargets")
    }
    return @($Names | Select-Object -Unique)
}

function ConvertTo-SiteStatus {
    param([string]$Status)
    switch ($Status) {
        "works" { return "working" }
        "working" { return "working" }
        "partial" { return "partial" }
        "broken" { return "failing" }
        "failing" { return "failing" }
        default { return $null }
    }
}

function Get-StatusRank {
    param([string]$Status)
    switch ($Status) {
        "working" { return 3 }
        "partial" { return 2 }
        "failing" { return 1 }
        default { return 0 }
    }
}

function Get-Slug {
    param([string]$Name)
    $slug = $Name.ToLowerInvariant() -replace '[^a-z0-9]+', '-'
    return $slug.Trim('-')
}

function Format-NameList {
    param([string[]]$Names)
    if ($Names.Count -eq 0) { return "" }
    if ($Names.Count -eq 1) { return $Names[0] }
    $head = $Names[0..($Names.Count - 2)] -join ", "
    return "$head, and $($Names[-1])"
}

function Set-UnmeasuredNames {
    param([string]$Notes, [string[]]$Remove)
    $pattern = ' ([^.]*?) were not measured by this probe\.'
    $match = [regex]::Match($Notes, $pattern)
    if (-not $match.Success) { return $Notes }
    $names = @($match.Groups[1].Value -split ', ' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    if ($names.Count -gt 0) {
        $last = $names[$names.Count - 1]
        if ($last.StartsWith("and ")) { $names[$names.Count - 1] = $last.Substring(4) }
    }
    $kept = @($names | Where-Object { $Remove -notcontains $_ })
    $sentence = ""
    if ($kept.Count -gt 0) { $sentence = " " + (Format-NameList $kept) + " were not measured by this probe." }
    return [regex]::Replace($Notes, $pattern, $sentence, 1)
}

function Remove-CountSentence {
    param([string]$Notes, [string]$Field)
    if ($Field -eq "applicationsSummary") {
        return [regex]::Replace($Notes, " applicationsSummary counts .*?not a new amd64 launch\.", "")
    }
    return [regex]::Replace($Notes, " driversSummary counts .*?is not in this count\.", "")
}

function Add-CountSentence {
    param([string]$Notes, [string]$Sentence)
    $trimmed = $Notes.TrimEnd()
    if (-not $trimmed.EndsWith(".")) { $trimmed += "." }
    return $trimmed + " " + $Sentence
}

function Set-Gap {
    param($Gaps, [string]$SectionName, [string]$Reason, [bool]$Measured)
    $list = New-Object System.Collections.Generic.List[object]
    $found = $false
    foreach ($gap in @($Gaps)) {
        if ($null -eq $gap) { continue }
        if ([string]$gap.section -eq $SectionName) {
            $found = $true
            if (-not $Measured) { $list.Add($gap) | Out-Null }
            continue
        }
        $list.Add($gap) | Out-Null
    }
    if (-not $Measured -and -not $found -and $Reason) {
        $list.Add([ordered]@{ section = $SectionName; reason = $Reason }) | Out-Null
    }
    return ,$list.ToArray()
}

function Read-ApplicationRows {
    param([string]$CatalogPath)
    $parsed = [System.IO.File]::ReadAllText($CatalogPath) | ConvertFrom-Json
    $rows = @()
    foreach ($row in $parsed) { $rows += $row }
    $best = @{}
    foreach ($row in $rows) {
        $status = ConvertTo-SiteStatus ([string]$row.status)
        if (-not $status) { continue }
        $name = [string]$row.name
        $key = $name.ToLowerInvariant()
        $candidate = [pscustomobject]@{
            slug   = (Get-Slug $name)
            name   = $name
            status = $status
        }
        if (-not $best.ContainsKey($key) -or (Get-StatusRank $status) -gt (Get-StatusRank $best[$key].status)) {
            $best[$key] = $candidate
        }
    }
    return @($best.Values | Sort-Object name)
}

function Read-DriverRows {
    param([string]$LogPath)
    $stream = [System.IO.File]::Open($LogPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        $reader = New-Object System.IO.StreamReader($stream)
        $text = $reader.ReadToEnd()
        $reader.Dispose()
    }
    finally {
        $stream.Dispose()
    }
    $pattern = '''\\((?<kind>Driver|FileSystem))\\(?<name>[^'']+)'' DriverEntry returned (?<code>0x[0-9a-fA-F]+)'
    $matches = [regex]::Matches($text, $pattern)
    $best = @{}
    foreach ($match in $matches) {
        $kind = [string]$match.Groups["kind"].Value
        $name = [string]$match.Groups["name"].Value
        if ($name -eq "winnull") { continue }
        $code = [string]$match.Groups["code"].Value
        $status = "failing"
        if ($code -eq "0x00000000") { $status = "working" }
        $key = $kind + "|" + $name.ToLowerInvariant()
        $best[$key] = [pscustomobject]@{
            category = $(if ($kind -eq "FileSystem") { "File systems" } else { "Drivers" })
            name     = $name
            status   = $status
        }
    }
    return @($best.Values)
}

function Get-SummaryCounts {
    param($Rows)
    $counts = [ordered]@{ working = 0; partial = 0; failing = 0 }
    foreach ($row in @($Rows)) { $counts[$row.status]++ }
    return $counts
}

function Update-HostSections {
    param($Site, [bool]$Applications, [bool]$Drivers, [string]$CatalogPath, [string]$LogPath)
    $notes = [string]$Site.notes
    $gaps = $Site.unmeasured
    $remove = @()

    if ($Applications) {
        if (-not (Test-Path -LiteralPath $CatalogPath)) { throw "Application catalog was not found at $CatalogPath" }
        $rows = @(Read-ApplicationRows $CatalogPath)
        if ($rows.Count -eq 0) { throw "Application catalog has no tested rows at $CatalogPath" }
        $counts = Get-SummaryCounts $rows
        $Site.applicationsSummary = $counts
        $Site.applications = @($rows)
        $notes = Remove-CountSentence $notes "applicationsSummary"
        $notes = Add-CountSentence $notes ("applicationsSummary counts $($rows.Count) recorded runs in compat/catalog/apps.json (working $($counts.working), partial $($counts.partial), failing $($counts.failing)). Untested catalog rows stay out of the count. These runs are the recorded 32-bit results, not a new amd64 launch.")
        $remove += "applications"
        $gaps = Set-Gap $gaps "applicationsSummary" "" $true
        $gaps = Set-Gap $gaps "applications" "" $true
    }

    if ($Drivers) {
        if (-not (Test-Path -LiteralPath $LogPath)) { throw "Driver boot log was not found at $LogPath" }
        $rows = @(Read-DriverRows $LogPath)
        if ($rows.Count -eq 0) { throw "Driver boot log has no DriverEntry result at $LogPath" }
        $counts = Get-SummaryCounts $rows
        $Site.driversSummary = $counts
        $groups = @()
        foreach ($category in @("Drivers", "File systems")) {
            $members = @($rows | Where-Object { $_.category -eq $category } | Sort-Object name | ForEach-Object {
                [pscustomobject]@{ name = $_.name; status = $_.status }
            })
            if ($members.Count -gt 0) {
                $groups += [ordered]@{ category = $category; drivers = $members }
            }
        }
        $Site.driverGroups = $groups
        $relative = "output/driver/load.log"
        $notes = Remove-CountSentence $notes "driversSummary"
        $notes = Add-CountSentence $notes ("driversSummary counts $($rows.Count) in-box DriverEntry results in $relative (working $($counts.working), partial $($counts.partial), failing $($counts.failing)). A zero status is working. winnull is the separately loaded Windows null.sys and is not in this count.")
        $remove += "drivers"
        $gaps = Set-Gap $gaps "driversSummary" "" $true
        $gaps = Set-Gap $gaps "driverGroups" "" $true
    }

    if ($remove.Count -gt 0) { $notes = Set-UnmeasuredNames $notes $remove }
    $Site.notes = $notes
    $Site.unmeasured = @($gaps)
    return $Site
}

function Repair-WebsiteAfterKernelMerge {
    param([string]$WebsitePath)
    if (-not (Test-Path -LiteralPath $WebsitePath)) { return }
    $site = Get-Content -LiteralPath $WebsitePath -Raw | ConvertFrom-Json
    $driverValue = $site.driverAbi
    $driverIsList = $driverValue -is [System.Collections.IList]
    if (-not $driverIsList) { $site.driverAbi = $null }
    $notes = [string]$site.notes
    $notes = [regex]::Replace($notes, ' driverAbi is [0-9.]+ from the driver ABI category \([^)]*\)\. Untested rows stay out of that percent\.', '')
    $remove = @()
    if ($null -ne $site.applicationsSummary -and $null -ne $site.applicationsSummary.working) { $remove += "applications" }
    if ($null -ne $site.driversSummary -and $null -ne $site.driversSummary.working) { $remove += "drivers" }
    $notes = Set-UnmeasuredNames $notes $remove
    if ($notes -notmatch 'driverAbi' -and $notes -match 'and graphics were not measured by this probe\.') {
        $notes = $notes.Replace('and graphics were not measured by this probe.', 'driverAbi, and graphics were not measured by this probe.')
    }
    $site.notes = $notes
    $site.unmeasured = @(Set-Gap $site.unmeasured "driverAbi" "The driver ABI list is not filled from a category percent." $false)
    Write-JsonFile $WebsitePath $site
    Write-Host "Kernel ABI kept; driver ABI list left unmeasured."
}

function Get-Amd64PluginLog {
    param([string]$Root)
    $bootPath = Join-Path $Root "output\boot-latest.json"
    if (-not (Test-Path -LiteralPath $bootPath)) { return "" }
    $boot = Get-Content -LiteralPath $bootPath -Raw | ConvertFrom-Json
    $iso = [string]$boot.iso
    if ($iso -notmatch "build-amd64") { return "" }
    if ([string]$boot.result -ne "booted") { return "" }
    $log = [string]$boot.pluginLog
    if (-not $log -or -not (Test-Path -LiteralPath $log)) { return "" }
    return $log
}

function Invoke-DashboardSelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("dashboard-update-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $root | Out-Null
    try {
        $catalog = Join-Path $root "apps.json"
        $log = Join-Path $root "load.log"
        $website = Join-Path $root "latest.json"
        $catalogJson = @"
[
  { "name": "cmd", "status": "untested", "evidence": "tree", "notes": "" },
  { "name": "7-Zip", "status": "untested", "evidence": "tree", "notes": "" },
  { "name": "7-Zip", "status": "works", "evidence": "tree", "notes": "window stayed up" },
  { "name": "git", "status": "broken", "evidence": "tree", "notes": "" }
]
"@
        $utf8 = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($catalog, $catalogJson, $utf8)
        $logText = @"
'\Driver\acpi' DriverEntry returned 0x00000000
'\FileSystem\Cdfs' DriverEntry returned 0x00000000
'\Driver\BusLogic' DriverEntry returned 0xc00000c0
'\Driver\winnull' DriverEntry returned 0xc0000035
'\Driver\acpi' DriverEntry returned 0x00000000
"@
        [System.IO.File]::WriteAllText($log, $logText, $utf8)
        $site = [ordered]@{
            spdx = "GPL-2.0-or-later"
            sample = $false
            overall = 80.2
            syscallOverall = 80.2
            kernelAbi = $null
            applicationsSummary = [ordered]@{ working = $null; partial = $null; failing = $null }
            applications = $null
            driversSummary = [ordered]@{ working = $null; partial = $null; failing = $null }
            driverGroups = $null
            notes = "overall and syscallOverall are exact status matches from the amd64 probe: 1 of 1 names. kernelAbi, lastBuild, applications, drivers, and graphics were not measured by this probe."
            unmeasured = @(
                [ordered]@{ section = "applicationsSummary"; reason = "The amd64 probe does not launch applications." }
                [ordered]@{ section = "applications"; reason = "The amd64 probe does not launch applications." }
                [ordered]@{ section = "driversSummary"; reason = "The amd64 probe does not load drivers." }
                [ordered]@{ section = "driverGroups"; reason = "The amd64 probe does not load drivers." }
                [ordered]@{ section = "kernelAbi"; reason = "The amd64 probe does not call ntoskrnl, HAL, or KMDF exports." }
            )
        }
        Write-JsonFile $website $site
        $loaded = Get-Content -LiteralPath $website -Raw | ConvertFrom-Json
        $updated = Update-HostSections -Site $loaded -Applications $true -Drivers $true -CatalogPath $catalog -LogPath $log
        if ($updated.overall -ne 80.2 -or $updated.syscallOverall -ne 80.2) { throw "host sections changed the syscall score" }
        if ($updated.applicationsSummary.working -ne 1 -or $updated.applicationsSummary.failing -ne 1 -or $updated.applicationsSummary.partial -ne 0) {
            throw "application counts were $($updated.applicationsSummary.working)/$($updated.applicationsSummary.partial)/$($updated.applicationsSummary.failing)"
        }
        if (@($updated.applications).Count -ne 2) { throw "untested or duplicate applications were counted" }
        if ($updated.driversSummary.working -ne 2 -or $updated.driversSummary.failing -ne 1) {
            throw "driver counts were $($updated.driversSummary.working)/$($updated.driversSummary.failing)"
        }
        $names = @($updated.driverGroups | ForEach-Object { $_.drivers } | ForEach-Object { $_.name })
        if ($names -contains "winnull") { throw "winnull was counted" }
        if ($updated.notes -match "applications were not measured" -or $updated.notes -match "drivers were not measured") {
            throw "measured names stayed in the unmeasured sentence"
        }
        if ($updated.notes -notmatch "applicationsSummary counts 2") { throw "application sentence missing" }
        if ($updated.notes -notmatch "driversSummary counts 3") { throw "driver sentence missing" }
        $gapNames = @($updated.unmeasured | ForEach-Object { [string]$_.section })
        if ($gapNames -contains "applicationsSummary" -or $gapNames -contains "driverGroups") { throw "measured gaps remained" }
        if (-not ($gapNames -contains "kernelAbi")) { throw "kernel gap was dropped" }
        $again = Update-HostSections -Site $updated -Applications $true -Drivers $true -CatalogPath $catalog -LogPath $log
        $firstNotes = [regex]::Matches([string]$again.notes, "applicationsSummary counts").Count
        if ($firstNotes -ne 1) { throw "application sentence was duplicated" }
        Write-Host "Dashboard update fixture passed."
    }
    finally {
        if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
    }
}

if (-not $SourceRoot) {
    $SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

if ($SelfTest) {
    Invoke-DashboardSelfTest
    exit 0
}

if ($Repair) {
    if (-not $WebsitePath) { $WebsitePath = Join-Path $SourceRoot "veros\dashboard\latest.json" }
    Repair-WebsiteAfterKernelMerge -WebsitePath $WebsitePath
    exit 0
}

$sections = Get-SectionSet $Section
if (-not $WebsitePath) { $WebsitePath = Join-Path $SourceRoot "veros\dashboard\latest.json" }
if (-not $DriverLog) { $DriverLog = Join-Path $SourceRoot "output\driver\load.log" }
if (-not $AppCatalog) { $AppCatalog = Join-Path $SourceRoot "compat\catalog\apps.json" }

$differential = Join-Path (Split-Path -Parent $SourceRoot) "differential-tests"
if ($sections -contains "Syscalls") {
    & (Join-Path $differential "syscall-diff\Publish-Dashboard.ps1") -SourceRoot $SourceRoot -OutputPath $WebsitePath
}

if ($sections -contains "KernelAbi") {
    & (Join-Path $differential "kernel-abi\Compare-KernelAbi.ps1") -SourceRoot $SourceRoot -WebsitePath $WebsitePath
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$refreshApps = ($sections -contains "Applications") -or ($sections -contains "Syscalls")
$refreshDrivers = ($sections -contains "Drivers") -or ($sections -contains "Syscalls")
if ($refreshApps -or $refreshDrivers) {
    if (-not (Test-Path -LiteralPath $WebsitePath)) { throw "Website snapshot was not found at $WebsitePath" }
    $site = Get-Content -LiteralPath $WebsitePath -Raw | ConvertFrom-Json
    $site = Update-HostSections -Site $site -Applications $refreshApps -Drivers $refreshDrivers -CatalogPath $AppCatalog -LogPath $DriverLog
    . (Join-Path $PSScriptRoot "DashboardStore.ps1")
    $dashRoot = Split-Path -Parent $WebsitePath
    $captured = [string]$site.capturedAt
    if (-not $captured) { $captured = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ") }
    if ($refreshApps) { Write-DashboardSection -DashboardRoot $dashRoot -Name "applications" -CapturedAt $captured -Values $site }
    if ($refreshDrivers) { Write-DashboardSection -DashboardRoot $dashRoot -Name "drivers" -CapturedAt $captured -Values $site }
    Update-DashboardSection -DashboardRoot $dashRoot -Name "syscalls" -CapturedAt $captured -Values ([ordered]@{
        notes      = [string]$site.notes
        unmeasured = @($site.unmeasured)
    })
    $appText = "unchanged"
    $driverText = "unchanged"
    if ($refreshApps) { $appText = "$($site.applicationsSummary.working) working, $($site.applicationsSummary.partial) partial, $($site.applicationsSummary.failing) failing" }
    if ($refreshDrivers) { $driverText = "$($site.driversSummary.working) working, $($site.driversSummary.partial) partial, $($site.driversSummary.failing) failing" }
    Write-Host "Updated $WebsitePath applications: $appText; drivers: $driverText"
}

$tabSections = New-Object System.Collections.Generic.List[string]
if ($sections -contains "Syscalls") {
    foreach ($name in @("Regressions", "HeatMap", "Tasks")) { $tabSections.Add($name) | Out-Null }
}
foreach ($name in @("Regressions", "HeatMap", "Traces", "Tasks", "DriverAbi", "Graphics")) {
    if ($sections -contains $name) { $tabSections.Add($name) | Out-Null }
}
$tabUnique = @($tabSections | Select-Object -Unique)
if ($tabUnique.Count -gt 0) {
    & (Join-Path $PSScriptRoot "Publish-TabProbes.ps1") -SourceRoot $SourceRoot -WebsitePath $WebsitePath -Section $tabUnique
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($sections -contains "SdkTargets") {
    & (Join-Path $PSScriptRoot "Publish-SdkTargets.ps1") -SourceRoot $SourceRoot -WebsitePath $WebsitePath
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
