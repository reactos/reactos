# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/compat/Build-Snapshot.ps1
# PURPOSE:     Build a compatibility snapshot from probe logs
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$PluginLog = "",
    [string]$SerialLog = "",
    [string]$CrashJson = "",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function Read-NameList {
    param([string]$Path)
    $names = @()
    foreach ($line in (Get-Content $Path)) {
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith("#")) { continue }
        $parts = $trim -split "\s+"
        $names += [pscustomobject]@{ Name = $parts[0]; Ordinal = $(if ($parts.Count -gt 1) { $parts[1] } else { $null }) }
    }
    return $names
}

function Read-SpecMap {
    param([string[]]$Files)
    $map = @{}
    foreach ($file in $Files) {
        if (-not (Test-Path $file)) { continue }
        foreach ($line in (Get-Content $file)) {
            if ($line -notmatch "^\s*@\s+\S+") { continue }
            $stub = $line -match "(^|\s)-stub(\s|$)"
            if ($line -match "([A-Za-z_][A-Za-z0-9_]*)\s*\(") {
                $name = $Matches[1]
                if (-not $map.ContainsKey($name)) {
                    $map[$name] = [pscustomobject]@{ Stub = $stub; File = $file }
                }
                elseif (-not $stub) {
                    $map[$name].Stub = $false
                }
            }
        }
    }
    return $map
}

function Read-PluginResults {
    param([string]$Path, [string]$CrashPath)
    $results = @{}
    if ($Path -and (Test-Path $Path)) {
        foreach ($line in (Get-Content $Path)) {
            if (-not $line.Trim().StartsWith("{")) { continue }
            $event = $line | ConvertFrom-Json
            $symbol = [string]$event.symbol
            $name = $symbol
            if ($symbol -match "!(.+)$") { $name = $Matches[1] }
            if (-not $name) { continue }
            $rank = 0
            if ($results.ContainsKey($name)) { $rank = $results[$name].Rank }
            $next = $rank
            $label = $results[$name].Label
            if ($event.result -eq "bugcheck" -or $event.phase -eq "bugcheck") {
                $next = 3; $label = "bugcheck"
            }
            elseif ($event.phase -eq "return" -and $event.result -eq "error" -and $next -lt 2) {
                $next = 2; $label = "error"
            }
            elseif ($event.phase -eq "return" -and $event.result -eq "success" -and $next -lt 1) {
                $next = 1; $label = "success"
            }
            $results[$name] = [pscustomobject]@{ Rank = $next; Label = $label }
        }
    }
    if ($CrashPath -and (Test-Path $CrashPath)) {
        $crash = Get-Content $CrashPath -Raw | ConvertFrom-Json
        if ($crash.kind -eq "bugcheck" -and $crash.inFlightCall -and $crash.inFlightCall.symbol) {
            $symbol = [string]$crash.inFlightCall.symbol
            $name = $symbol
            if ($symbol -match "!(.+)$") { $name = $Matches[1] }
            $results[$name] = [pscustomobject]@{ Rank = 3; Label = "bugcheck" }
        }
    }
    return $results
}

function Get-ItemStatus {
    param($Spec, $PluginLabel)
    if ($PluginLabel -eq "bugcheck") { return "broken" }
    if ($PluginLabel -eq "success") { return "works" }
    if ($PluginLabel -eq "error") { return "partial" }
    if ($Spec -and $Spec.Stub) { return "partial" }
    if (-not $Spec) { return "missing" }
    return "untested"
}

function Get-Score {
    param([string]$Status)
    switch ($Status) {
        "works" { return 1.0 }
        "partial" { return 0.5 }
        "broken" { return 0.0 }
        "missing" { return 0.0 }
        default { return $null }
    }
}

function Get-Counts {
    param($Items)
    $counts = [ordered]@{ works = 0; partial = 0; broken = 0; missing = 0; untested = 0 }
    foreach ($item in $Items) { $counts[$item.status]++ }
    return $counts
}

function Get-Percent {
    param($Items)
    $sum = 0.0
    $n = 0
    foreach ($item in $Items) {
        $score = Get-Score $item.status
        if ($null -ne $score) {
            $sum += $score
            $n++
        }
    }
    if ($n -eq 0) { return $null }
    return [math]::Round(100.0 * $sum / $n, 1)
}

function New-ExportItem {
    param($Name, $Category, $Group, $SpecMap, $Plugin, $Evidence)
    $spec = $null
    if ($SpecMap.ContainsKey($Name)) { $spec = $SpecMap[$Name] }
    $label = $null
    if ($Plugin.ContainsKey($Name)) { $label = $Plugin[$Name].Label }
    $status = Get-ItemStatus -Spec $spec -PluginLabel $label
    $present = [bool]($spec -and -not $spec.Stub)
    return [ordered]@{
        id       = "$Category-$Name"
        name     = $Name
        category = $Category
        group    = $Group
        status   = $status
        present  = $present
        source   = $(if ($label) { "plugin" } elseif ($spec) { "spec" } else { "reference" })
        evidence = $(if ($spec) { $spec.File } else { $Evidence })
        notes    = ""
        numberMatch = "untested"
    }
}

function Get-BootStatus {
    param([string]$CrashPath, [string]$BootJson)
    $status = "untested"
    $accelerator = $null
    if ($CrashPath -and (Test-Path $CrashPath)) {
        $crash = Get-Content $CrashPath -Raw | ConvertFrom-Json
        if ($crash.kind -eq "booted") { $status = "works" }
        elseif ($crash.kind -eq "bugcheck") { $status = "broken" }
        elseif ($crash.kind -eq "hang") { $status = "hang" }
    }
    if ($BootJson -and (Test-Path $BootJson)) {
        $boot = Get-Content $BootJson -Raw | ConvertFrom-Json
        if ($boot.accelerator) { $accelerator = [string]$boot.accelerator }
    }
    return [ordered]@{ status = $status; accelerator = $accelerator }
}

function Test-LogNamesModule {
    param([string]$SerialText, [string]$Name, [string]$Extension)
    if (-not $SerialText -or -not $Name) { return $false }
    return [regex]::IsMatch($SerialText, [regex]::Escape($Name) + "\." + $Extension, "IgnoreCase")
}

function Apply-SyscallDiff {
    param($Items, [string]$Path)
    if (-not $Path -or -not (Test-Path $Path)) { return }
    $report = Get-Content $Path -Raw | ConvertFrom-Json
    foreach ($call in @($report.calls)) {
        if ($call.status -eq "untested") { continue }
        foreach ($item in $Items) {
            if ($item.category -eq "syscalls" -and $item.name -eq $call.name) {
                $item.status = [string]$call.status
                $item.notes = "Windows: $($call.windows); ReactOS: $($call.reactos)"
                $item.source = "diff"
                $item.evidence = "output/syscall-diff/report.json"
            }
        }
    }
}

function Get-RepoRelativePath {
    param([string]$Root, [string]$Path)
    $full = [System.IO.Path]::GetFullPath($Path)
    $rootFull = [System.IO.Path]::GetFullPath($Root)
    $sep = [System.IO.Path]::DirectorySeparatorChar
    if (-not $rootFull.EndsWith($sep)) { $rootFull += $sep }
    if ($full.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        return ($full.Substring($rootFull.Length) -replace '\\', '/')
    }
    return ($full -replace '\\', '/')
}

function Build-SnapshotObject {
    param($Root, $SpecMap, $Plugin, $SerialText, $Boot, [string]$DiffPath = "", [string]$PluginLog = "")
    $ref = Join-Path $Root "compat\reference"
    $cat = Join-Path $Root "compat\catalog"
    $items = @()

    $groups = Get-Content (Join-Path $cat "driver-abi-groups.json") -Raw | ConvertFrom-Json
    foreach ($prop in $groups.PSObject.Properties) {
        foreach ($name in $prop.Value) {
            $items += New-ExportItem -Name $name -Category "driverAbi" -Group $prop.Name -SpecMap $SpecMap -Plugin $Plugin -Evidence "compat/catalog/driver-abi-groups.json"
        }
    }
    foreach ($row in (Get-Content (Join-Path $cat "structures.json") -Raw | ConvertFrom-Json)) {
        $items += [ordered]@{
            id = "driverAbi-$($row.name)"; name = $row.name; category = "driverAbi"; group = "layout"
            status = $row.status; present = $false; source = "manual"; evidence = $row.evidence; notes = $row.notes; numberMatch = "untested"
        }
    }

    foreach ($row in (Read-NameList (Join-Path $ref "win10-22h2-ntoskrnl.txt"))) {
        $items += New-ExportItem -Name $row.Name -Category "kernel" -Group "ntoskrnl" -SpecMap $SpecMap -Plugin $Plugin -Evidence "compat/reference/win10-22h2-ntoskrnl.txt"
    }
    foreach ($row in (Read-NameList (Join-Path $ref "win10-22h2-hal.txt"))) {
        $items += New-ExportItem -Name $row.Name -Category "kernel" -Group "hal" -SpecMap $SpecMap -Plugin $Plugin -Evidence "compat/reference/win10-22h2-hal.txt"
    }

    $top = @("NtCreateFile", "NtReadFile", "NtWriteFile", "NtDeviceIoControlFile", "NtCreateUserProcess", "NtCreateThreadEx", "NtAllocateVirtualMemory", "NtMapViewOfSection", "NtQueryInformationProcess", "NtClose")
    foreach ($row in (Read-NameList (Join-Path $ref "win10-22h2-syscalls.txt"))) {
        $item = New-ExportItem -Name $row.Name -Category "syscalls" -Group $(if ($top -contains $row.Name) { "top" } else { "all" }) -SpecMap $SpecMap -Plugin $Plugin -Evidence "compat/reference/win10-22h2-syscalls.txt"
        if ($row.Ordinal) { $item.numberMatch = "recorded" } else { $item.numberMatch = "untested" }
        $items += $item
    }

    foreach ($row in (Get-Content (Join-Path $cat "drivers.json") -Raw | ConvertFrom-Json)) {
        $status = $row.status
        if ($status -eq "untested" -and (Test-LogNamesModule $SerialText $row.name "sys")) {
            $status = "partial"
        }
        $items += [ordered]@{
            id = "drivers-$($row.name)"; name = $row.name; category = "drivers"; group = "inbox"
            status = $status; present = $true; source = "manual"; evidence = $row.evidence; notes = $row.notes; numberMatch = "untested"
        }
    }
    foreach ($row in (Get-Content (Join-Path $cat "graphics.json") -Raw | ConvertFrom-Json)) {
        $items += [ordered]@{
            id = "graphics-$($row.name)"; name = $row.name; category = "graphics"; group = "graphics"
            status = $row.status; present = ($row.status -ne "missing"); source = "manual"; evidence = $row.evidence; notes = $row.notes; numberMatch = "untested"
        }
    }
    foreach ($row in (Get-Content (Join-Path $cat "apps.json") -Raw | ConvertFrom-Json)) {
        $status = $row.status
        if ($status -eq "untested" -and (Test-LogNamesModule $SerialText $row.name "exe")) {
            $status = "partial"
        }
        $items += [ordered]@{
            id = "applications-$($row.name)"; name = $row.name; category = "applications"; group = "apps"
            status = $status; present = $false; source = "manual"; evidence = $row.evidence; notes = $row.notes; numberMatch = "untested"
        }
    }

    Apply-SyscallDiff -Items $items -Path $DiffPath

    $weights = [ordered]@{ driverAbi = 40; kernel = 25; syscalls = 15; drivers = 10; graphics = 5; applications = 5 }
    $titles = [ordered]@{ driverAbi = "Driver ABI"; kernel = "Kernel exports"; syscalls = "Syscalls"; drivers = "Drivers"; graphics = "Graphics"; applications = "Applications" }
    $categories = @()
    $used = @{}
    foreach ($key in @($weights.Keys)) {
        $subset = @($items | Where-Object { $_.category -eq $key })
        $percent = Get-Percent $subset
        $categories += [ordered]@{
            id = $key; title = $titles[$key]; percent = $percent; weight = $weights[$key]; counts = (Get-Counts $subset)
        }
        if ($null -ne $percent) { $used[$key] = $weights[$key] }
    }
    $weightSum = 0
    foreach ($value in $used.Values) { $weightSum += $value }
    $overall = $null
    $weightsUsed = [ordered]@{}
    if ($weightSum -gt 0) {
        $acc = 0.0
        foreach ($key in $used.Keys) {
            $cat = $categories | Where-Object { $_.id -eq $key }
            $share = $used[$key] / $weightSum
            $weightsUsed[$key] = [math]::Round(100.0 * $share, 1)
            $acc += $cat.percent * $share
        }
        $overall = [math]::Round($acc, 1)
    }
    $headline = $categories | Where-Object { $_.id -eq "driverAbi" } | Select-Object -First 1

    $version = "unknown"
    $cmake = Join-Path $Root "sdk\include\reactos\version.cmake"
    if (Test-Path $cmake) {
        $text = Get-Content $cmake -Raw
        $major = [regex]::Match($text, 'KERNEL_VERSION_MAJOR "([^"]+)"').Groups[1].Value
        $minor = [regex]::Match($text, 'KERNEL_VERSION_MINOR "([^"]+)"').Groups[1].Value
        $patch = [regex]::Match($text, 'KERNEL_VERSION_PATCH_LEVEL "([^"]+)"').Groups[1].Value
        $type = [regex]::Match($text, 'KERNEL_VERSION_BUILD_TYPE "([^"]+)"').Groups[1].Value
        $version = "$major.$minor.$patch"
        if ($type) { $version = "$version-$type" }
    }
    $git = "unknown"
    try { $git = (& git -C $Root describe --always --dirty).Trim() } catch { }
    $pluginRecord = $null
    if ($PluginLog -and (Test-Path -LiteralPath $PluginLog)) {
        $pluginRecord = Get-RepoRelativePath -Root $Root -Path $PluginLog
    }

    return [ordered]@{
        build      = [ordered]@{ version = $version; git = $git }
        boot       = $(if ($Boot) { $Boot } else { [ordered]@{ status = "untested"; accelerator = $null } })
        pluginLog  = $pluginRecord
        weights    = $weights
        weightsUsed = $weightsUsed
        headline   = $headline
        overall    = [ordered]@{ percent = $overall }
        categories = $categories
        topSyscalls = $top
        numberMatch = "untested"
        items      = $items
    }
}

function Invoke-SnapshotSelfTest {
    $fixtureRoot = Join-Path $PSScriptRoot "fixtures"
    $spec = Read-SpecMap -Files @((Join-Path $fixtureRoot "sample.spec"))
    $plugin = Read-PluginResults -Path (Join-Path $fixtureRoot "plugin.jsonl") -CrashPath (Join-Path $fixtureRoot "crash.json")
    $io = Get-ItemStatus -Spec $spec["IoCallDriver"] -PluginLabel $plugin["IoCallDriver"].Label
    $stub = Get-ItemStatus -Spec $spec["NtCreateUserProcess"] -PluginLabel $null
    $broken = Get-ItemStatus -Spec $spec["KeWaitForSingleObject"] -PluginLabel $plugin["KeWaitForSingleObject"].Label
    if ($io -ne "works") { throw "IoCallDriver should be works, got $io" }
    if ($stub -ne "partial") { throw "NtCreateUserProcess should be partial, got $stub" }
    if ($broken -ne "broken") { throw "KeWaitForSingleObject should be broken, got $broken" }
    $snap = Build-SnapshotObject -Root (Join-Path $PSScriptRoot "..\..") -SpecMap $spec -Plugin $plugin -SerialText ""
    $found = @($snap.items | Where-Object { $_.name -eq "IoCallDriver" -and $_.status -eq "works" })
    if ($found.Count -lt 1) { throw "snapshot did not upgrade IoCallDriver" }
    if ($null -ne $snap.pluginLog) { throw "snapshot recorded a plugin log that was not passed" }
    Write-Host "Snapshot fixture passed."
}

if ($SelfTest) {
    Invoke-SnapshotSelfTest
    exit 0
}

$specFiles = @(
    (Join-Path $SourceRoot "dll\ntdll\def\ntdll.spec"),
    (Join-Path $SourceRoot "ntoskrnl\ntoskrnl.spec"),
    (Join-Path $SourceRoot "hal\hal.spec"),
    (Join-Path $SourceRoot "sdk\lib\drivers\wdf\wdfldr\wdfldr.spec")
)
$spec = Read-SpecMap -Files $specFiles
$plugin = Read-PluginResults -Path $PluginLog -CrashPath $CrashJson
$serialText = ""
if ($SerialLog -and (Test-Path $SerialLog)) { $serialText = Get-Content $SerialLog -Raw }
$boot = Get-BootStatus -CrashPath $CrashJson -BootJson (Join-Path $SourceRoot "output\boot-latest.json")
$diff = Join-Path $SourceRoot "output\syscall-diff\report.json"
$snapshot = Build-SnapshotObject -Root $SourceRoot -SpecMap $spec -Plugin $plugin -SerialText $serialText -Boot $boot -DiffPath $diff -PluginLog $PluginLog

$outDir = Join-Path $SourceRoot "dashboard\data"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$json = $snapshot | ConvertTo-Json -Depth 8
Set-Content -Path (Join-Path $outDir "snapshot.json") -Value $json
Set-Content -Path (Join-Path $outDir "snapshot.js") -Value ("window.SNAPSHOT = " + $json + ";")
Write-Host "Wrote dashboard/data/snapshot.json"
