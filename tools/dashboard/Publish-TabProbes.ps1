# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/dashboard/Publish-TabProbes.ps1
# PURPOSE:     Publish dashboard tab probes
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = "",
    [ValidateSet("All", "Regressions", "HeatMap", "Traces", "Tasks", "DriverAbi", "Graphics")]
    [string[]]$Section = @("All"),
    [string]$WebsitePath = "",
    [string]$HistoryPath = "",
    [string]$SubsystemMap = "",
    [string]$DriverGroups = "",
    [string]$PluginLog = "",
    [string]$TraceDir = "",
    [string]$NameList = "",
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

function Set-SiteField {
    param($Site, [string]$Name, $Value)
    if ($Site.PSObject.Properties.Name -contains $Name) {
        $Site.$Name = $Value
        return
    }
    $Site | Add-Member -NotePropertyName $Name -NotePropertyValue $Value
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

function Get-SectionSet {
    param([string[]]$Names)
    $all = @("Regressions", "HeatMap", "Traces", "Tasks", "DriverAbi", "Graphics")
    if ($Names -contains "All") { return $all }
    return @($Names | Select-Object -Unique)
}

function Get-RoundScore {
    param($Number)
    return [math]::Round([double]$Number, 1)
}

function Read-NameSet {
    param([string]$Path)
    $set = @{}
    foreach ($line in (Get-Content -LiteralPath $Path)) {
        $name = $line.Trim()
        if (-not $name -or $name.StartsWith("#")) { continue }
        $set[$name] = $true
    }
    return $set
}

function Read-PluginLabels {
    param([string]$Path)
    $results = @{}
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) { return $results }
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        $reader = New-Object System.IO.StreamReader($stream)
        while ($null -ne ($line = $reader.ReadLine())) {
            if (-not $line.StartsWith("{")) { continue }
            if ($line -notmatch '"symbol":"([^"]+)"') { continue }
            $symbol = $Matches[1]
            $name = $symbol
            if ($symbol -match "!(.+)$") { $name = $Matches[1] }
            $rank = 0
            $label = $null
            if ($results.ContainsKey($name)) {
                $rank = $results[$name].Rank
                $label = $results[$name].Label
            }
            if ($line -match '"result":"bugcheck"' -or $line -match '"phase":"bugcheck"') {
                $rank = 3; $label = "bugcheck"
            }
            elseif ($line -match '"phase":"return"' -and $line -match '"result":"error"' -and $rank -lt 2) {
                $rank = 2; $label = "error"
            }
            elseif ($line -match '"phase":"return"' -and $line -match '"result":"success"' -and $rank -lt 1) {
                $rank = 1; $label = "success"
            }
            if ($label) { $results[$name] = [pscustomobject]@{ Rank = $rank; Label = $label } }
        }
        $reader.Dispose()
    }
    finally {
        $stream.Dispose()
    }
    return $results
}

function Get-LabelScore {
    param([string]$Label)
    switch ($Label) {
        "success" { return 100.0 }
        "error" { return 50.0 }
        "bugcheck" { return 0.0 }
        default { return $null }
    }
}

function Read-TraceCounts {
    param([string]$Directory, $Allowed)
    $workloads = @()
    $named = @{}
    $namedTotal = 0
    if (-not (Test-Path -LiteralPath $Directory)) { return $null }
    $files = @(Get-ChildItem -LiteralPath $Directory -Filter "*.jsonl" -File | Sort-Object Name)
    if ($files.Count -eq 0) { return $null }
    foreach ($file in $files) {
        $records = 0
        $stream = [System.IO.File]::Open($file.FullName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        try {
            $reader = New-Object System.IO.StreamReader($stream)
            while ($null -ne ($line = $reader.ReadLine())) {
                if ($line -notmatch '"syscall":"([^"]+)"') { continue }
                $records++
                $token = $Matches[1]
                if ($token -match '^[A-Za-z]') {
                    if ($Allowed.ContainsKey($token)) {
                        if (-not $named.ContainsKey($token)) { $named[$token] = 0 }
                        $named[$token]++
                        $namedTotal++
                    }
                }
            }
            $reader.Dispose()
        }
        finally {
            $stream.Dispose()
        }
        if ($records -gt 0) {
            $workloads += [ordered]@{ name = $file.BaseName; records = $records }
        }
    }
    if ($workloads.Count -eq 0) { return $null }
    $corpus = $null
    if ($namedTotal -gt 0) {
        $corpus = @()
        foreach ($name in ($named.Keys | Sort-Object { $named[$_] } -Descending)) {
            $share = Get-RoundScore (100.0 * $named[$name] / $namedTotal)
            $corpus += [ordered]@{ syscall = $name; share = $share }
        }
    }
    return [pscustomobject]@{ Workloads = $workloads; Corpus = $corpus; Named = $namedTotal }
}

function Get-ContractStatus {
    param($Report, [string[]]$Names)
    $rows = @($Report.calls | Where-Object { $Names -contains $_.name })
    if ($rows.Count -eq 0) { return "untested" }
    $done = @($rows | Where-Object { $_.windowsCompleted -eq $true })
    if ($done.Count -eq 0) { return "untested" }
    $matched = @($done | Where-Object { $_.capabilityMatch -eq $true })
    if ($matched.Count -eq $done.Count) { return "works" }
    $present = @($rows | Where-Object { $_.reactos -ne "MISSING" -and $_.reactos -ne "missing" })
    if ($present.Count -eq 0) { return "missing" }
    return "partial"
}

function Get-GraphicsReport {
    param([string]$Root)
    $dir = Join-Path $Root "output\graphics-diff"
    $reportPath = Join-Path $dir "report.json"
    $winLog = Join-Path $dir "windows.log"
    if (-not (Test-Path -LiteralPath $reportPath)) { return $null }
    if (-not (Test-Path -LiteralPath $winLog)) { return $null }
    if (-not (Select-String -LiteralPath $winLog -Pattern "DIFF END OK" -Quiet)) { return $null }
    return (Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json)
}

function Get-GraphicsRows {
    param([string]$Root)
    $rows = @(
        @{ Name = "Software framebuffer"; Rel = "win32ss"; Present = "partial"; Absent = "missing" },
        @{ Name = "DRM"; Rel = "drivers/gpu/drm"; Present = "partial"; Absent = "missing" },
        @{ Name = "Compositor"; Rel = "win32ss/user/ntuser/dwm"; Present = "partial"; Absent = "missing" },
        @{ Name = "DXVK"; Rel = "dll/directx/dxvk"; Present = "partial"; Absent = "missing" },
        @{ Name = "D3D11"; Rel = "dll/directx/wine/d3d11"; Present = "untested"; Absent = "missing" },
        @{ Name = "D3D12"; Rel = "dll/directx/wine/d3d12"; Present = "untested"; Absent = "missing" },
        @{ Name = "WDDM"; Rel = "win32ss/gdi/ntgdi/d3dkmt.c"; Present = "partial"; Absent = "missing" }
    )
    $measured = Get-GraphicsReport $Root
    $drawNames = @("Gdi.Fill", "Gdi.Rect", "Gdi.Line", "Gdi.Blit", "Gdi.Stretch", "Gdi.Alpha", "Gdi.Pixel")
    $built = @()
    foreach ($row in $rows) {
        $exists = Test-Path -LiteralPath (Join-Path $Root ($row.Rel -replace '/', '\'))
        $status = $(if ($exists) { $row.Present } else { $row.Absent })
        if ($measured -and $row.Name -eq "Software framebuffer") {
            $status = Get-ContractStatus $measured $drawNames
        }
        elseif ($measured -and $row.Name -eq "D3D11") {
            $status = Get-ContractStatus $measured @("D3d11.Clear")
        }
        $value = switch ($status) {
            "works" { "Working" }
            "partial" { "Partial" }
            "missing" { "Missing" }
            default { "Untested" }
        }
        $tone = switch ($status) {
            "works" { "ok" }
            "missing" { "bad" }
            default { "warn" }
        }
        $built += [ordered]@{ label = $row.Name; value = $value; tone = $tone }
    }
    return $built
}

function Get-SubsystemScores {
    param($Syscalls, [string]$MapPath)
    $map = Get-Content -LiteralPath $MapPath -Raw | ConvertFrom-Json
    $groups = [ordered]@{}
    foreach ($prop in $map.PSObject.Properties) {
        $groups[$prop.Name] = @($prop.Value)
    }
    $owner = @{}
    foreach ($name in $groups.Keys) {
        foreach ($syscall in $groups[$name]) { $owner[$syscall] = $name }
    }
    $bucket = @{}
    foreach ($name in $groups.Keys) { $bucket[$name] = [pscustomobject]@{ Measured = 0; Matches = 0; Misses = 0 } }
    $bucket["Other"] = [pscustomobject]@{ Measured = 0; Matches = 0; Misses = 0 }
    foreach ($row in @($Syscalls)) {
        if ($null -eq $row.compatibility) { continue }
        $group = "Other"
        if ($owner.ContainsKey([string]$row.name)) { $group = $owner[[string]$row.name] }
        if (-not $bucket.ContainsKey($group)) {
            $bucket[$group] = [pscustomobject]@{ Measured = 0; Matches = 0; Misses = 0 }
        }
        $bucket[$group].Measured++
        if ([double]$row.compatibility -ge 100) { $bucket[$group].Matches++ }
        else { $bucket[$group].Misses++ }
    }
    $scores = @()
    $order = @($groups.Keys) + @("Other")
    foreach ($name in $order) {
        $item = $bucket[$name]
        if ($item.Measured -eq 0) { continue }
        $scores += [pscustomobject]@{
            Name    = $name
            Score   = (Get-RoundScore (100.0 * $item.Matches / $item.Measured))
            Misses  = $item.Misses
            Measured = $item.Measured
        }
    }
    return $scores
}

function Get-HistoryPoints {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return @() }
    $parsed = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    return @($parsed)
}

function ConvertTo-ScoreMap {
    param($Syscalls)
    $map = @{}
    foreach ($row in @($Syscalls)) {
        if ($null -eq $row.compatibility) { continue }
        $map[[string]$row.name] = [double]$row.compatibility
    }
    return $map
}

function Update-Regression {
    param($Site, [string]$Path)
    $current = ConvertTo-ScoreMap $Site.syscalls
    $points = @(Get-HistoryPoints $Path)
    $previous = $null
    if ($points.Count -gt 0) { $previous = $points[$points.Count - 1] }
    $previousOverall = $null
    $change = $null
    $improvements = @()
    $regressions = @()
    if ($previous -and $previous.syscalls) {
        $older = @{}
        foreach ($row in @($previous.syscalls)) { $older[[string]$row.name] = $row.compatibility }
        foreach ($name in $current.Keys) {
            if (-not $older.ContainsKey($name) -or $null -eq $older[$name]) { continue }
            $delta = Get-RoundScore ($current[$name] - [double]$older[$name])
            if ($delta -gt 0) { $improvements += [ordered]@{ name = $name; delta = $delta } }
            elseif ($delta -lt 0) { $regressions += [ordered]@{ name = $name; delta = $delta } }
        }
        if ($null -ne $previous.overall -and $null -ne $Site.overall) {
            $previousOverall = Get-RoundScore $previous.overall
            $change = Get-RoundScore ([double]$Site.overall - $previousOverall)
        }
    }
    $label = "No earlier snapshot"
    if ($null -ne $previousOverall) { $label = "Previous snapshot" }
    $regression = [ordered]@{
        previousLabel   = $label
        previousOverall = $previousOverall
        currentBuild    = $Site.lastBuild
        currentOverall  = $Site.overall
        change          = $change
        improvements    = @($improvements)
        regressions     = @($regressions)
    }
    Set-SiteField $Site "regression" $regression
    return $Site
}

function Update-HeatAndTask {
    param($Site, [string]$MapPath, [bool]$Heat, [bool]$IncludeTask)
    $scores = @(Get-SubsystemScores -Syscalls $Site.syscalls -MapPath $MapPath)
    if ($scores.Count -eq 0) { return $Site }
    if ($Heat) {
        Set-SiteField $Site "subsystems" @($scores | ForEach-Object { [ordered]@{ name = $_.Name; score = $_.Score } })
        $worst = $scores | Sort-Object @{ Expression = "Score"; Ascending = $true }, @{ Expression = "Misses"; Ascending = $false } | Select-Object -First 1
        Set-SiteField $Site "highestImpact" ([string]$worst.Name)
    }
    if ($IncludeTask) {
        $target = $Site.highestImpact
        if (-not $target) {
            $worst = $scores | Sort-Object @{ Expression = "Score"; Ascending = $true }, @{ Expression = "Misses"; Ascending = $false } | Select-Object -First 1
            $target = [string]$worst.Name
        }
        $owner = @{}
        $map = Get-Content -LiteralPath $MapPath -Raw | ConvertFrom-Json
        foreach ($prop in $map.PSObject.Properties) {
            foreach ($syscall in @($prop.Value)) { $owner[$syscall] = $prop.Name }
        }
        $chosen = $null
        foreach ($row in @($Site.syscalls)) {
            if ($null -eq $row.compatibility -or [double]$row.compatibility -ge 100) { continue }
            $group = "Other"
            if ($owner.ContainsKey([string]$row.name)) { $group = $owner[[string]$row.name] }
            if ($group -eq $target) { $chosen = $row; break }
        }
        if (-not $chosen) {
            $chosen = @($Site.syscalls | Where-Object { $null -ne $_.compatibility -and [double]$_.compatibility -lt 100 } | Select-Object -First 1)[0]
        }
        if ($chosen) {
            $measured = @($Site.syscalls | Where-Object { $null -ne $_.compatibility }).Count
            $gain = 0.0
            if ($measured -gt 0) { $gain = Get-RoundScore (100.0 / $measured) }
            $windows = ""
            $veros = ""
            foreach ($item in @($Site.divergences)) {
                if ([string]$item.syscall -eq [string]$chosen.name) {
                    $windows = [string]$item.windows
                    $veros = [string]$item.veros
                    break
                }
            }
            $body = "Compare Windows 10 22H2 and this boot for $($chosen.name)."
            if ($windows -and $veros) { $body = "$($chosen.name) returned $windows on Windows 10 22H2 and $veros on this boot." }
            $task = [ordered]@{
                id                    = [string]$chosen.name
                subsystem             = $(if ($target) { $target } else { "Syscall probe" })
                "functionName"        = [string]$chosen.name
                mismatchCount         = 1
                affectedApplications  = @("amd64 syscall probe")
                estimatedGain         = $gain
                body                  = $body
            }
            Set-SiteField $Site "task" $task
        }
    }
    return $Site
}

function Update-DriverAbi {
    param($Site, [string]$GroupPath, [string]$LogPath)
    $labels = Read-PluginLabels $LogPath
    if ($labels.Count -eq 0) { return $false }
    $groups = Get-Content -LiteralPath $GroupPath -Raw | ConvertFrom-Json
    $order = @("IRP", "PnP", "Power", "DMA", "KMDF")
    $rows = @()
    foreach ($name in $order) {
        $exports = @($groups.$name)
        $sum = 0.0
        $count = 0
        foreach ($export in $exports) {
            if (-not $labels.ContainsKey([string]$export)) { continue }
            $score = Get-LabelScore $labels[[string]$export].Label
            if ($null -eq $score) { continue }
            $sum += $score
            $count++
        }
        if ($count -gt 0) {
            $rows += [ordered]@{ name = $name; score = (Get-RoundScore ($sum / $count)) }
        }
    }
    if ($rows.Count -eq 0) { return $false }
    Set-SiteField $Site "driverAbi" $rows
    return $true
}

function Set-TabNote {
    param($Site, [string]$Sentence)
    $notes = [string]$Site.notes
    $notes = [regex]::Replace($notes, ' Tab probes:.*$', '')
    Set-SiteField $Site "notes" ($notes.TrimEnd() + " Tab probes: " + $Sentence)
    return $Site
}

function Invoke-TabSelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("tab-probes-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $root | Out-Null
    try {
        $map = Join-Path $root "subsystems.json"
        $groups = Join-Path $root "groups.json"
        $plugin = Join-Path $root "plugin.jsonl"
        $trace = Join-Path $root "traces"
        $names = Join-Path $root "names.txt"
        $sitePath = Join-Path $root "latest.json"
        $history = Join-Path $root "history.json"
        New-Item -ItemType Directory -Force -Path $trace | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $root "win32ss\gdi\ntgdi") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $root "dll\directx\wine\d3d11") | Out-Null
        Set-Content -LiteralPath (Join-Path $root "win32ss\gdi\ntgdi\d3dkmt.c") -Value "/* stub */" -Encoding ASCII
        $utf8 = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($map, '{"Filesystem":["NtCreateFile"],"Objects":["NtClose"]}', $utf8)
        [System.IO.File]::WriteAllText($groups, '{"IRP":["IoCallDriver"],"PnP":["IoReportDetectedDevice"],"Power":[],"DMA":[],"KMDF":[]}', $utf8)
        [System.IO.File]::WriteAllText($plugin, '{"kind":"export","phase":"return","symbol":"ntoskrnl.exe!IoCallDriver","result":"success"}' + "`n", $utf8)
        [System.IO.File]::WriteAllText((Join-Path $trace "notepad.jsonl"), '{"syscall":"NtClose","status":"STATUS_SUCCESS"}' + "`n" + '{"syscall":"49","status":"STATUS_SUCCESS","n":49}' + "`n", $utf8)
        [System.IO.File]::WriteAllText($names, "NtClose`r`nNtCreateFile`r`n", $utf8)
        $site = [ordered]@{
            spdx = "GPL-2.0-or-later"
            sample = $false
            capturedAt = "2026-11-07T00:00:00Z"
            overall = 50
            syscallOverall = 50
            lastBuild = $null
            syscalls = @(
                [ordered]@{ name = "NtClose"; compatibility = 100; windowsCalls = 1; matches = 1; mismatches = 0 },
                [ordered]@{ name = "NtCreateFile"; compatibility = 0; windowsCalls = 1; matches = 0; mismatches = 1 }
            )
            divergences = @([ordered]@{ syscall = "NtCreateFile"; windows = "STATUS_SUCCESS"; veros = "STATUS_ACCESS_DENIED"; count = 1 })
            regression = $null
            subsystems = $null
            highestImpact = $null
            traceCorpus = $null
            task = $null
            driverAbi = $null
            graphics = $null
            notes = "kernelAbi, subsystems, traceCorpus, task, driverAbi, and graphics were not measured by this probe."
        }
        Write-JsonFile $sitePath $site
        & $PSCommandPath -SourceRoot $root -Section All -WebsitePath $sitePath -HistoryPath $history -SubsystemMap $map -DriverGroups $groups -PluginLog $plugin -TraceDir $trace -NameList $names
        $filled = Get-Content -LiteralPath $sitePath -Raw | ConvertFrom-Json
        if ($filled.highestImpact -ne "Filesystem") { throw "highest impact was $($filled.highestImpact)" }
        if (@($filled.subsystems).Count -ne 2) { throw "subsystem count" }
        if ($filled.task.functionName -ne "NtCreateFile") { throw "task picked $($filled.task.functionName)" }
        if ($filled.task.estimatedGain -ne 50) { throw "gain was $($filled.task.estimatedGain)" }
        $irp = @($filled.driverAbi | Where-Object { $_.name -eq "IRP" })[0]
        if ($irp.score -ne 100) { throw "IRP score was $($irp.score)" }
        if (@($filled.traceCorpus).Count -ne 1 -or $filled.traceCorpus[0].syscall -ne "NtClose") { throw "named trace was not kept" }
        if ($filled.traceWorkloads[0].records -ne 2) { throw "numeric trace line was dropped from the workload count" }
        $fb = @($filled.graphics | Where-Object { $_.label -eq "Software framebuffer" })[0]
        $drm = @($filled.graphics | Where-Object { $_.label -eq "DRM" })[0]
        $d3d = @($filled.graphics | Where-Object { $_.label -eq "D3D11" })[0]
        if ($fb.value -ne "Partial" -or $drm.value -ne "Missing" -or $d3d.value -ne "Untested") {
            throw "graphics was $($fb.value)/$($drm.value)/$($d3d.value)"
        }
        $gfx = Join-Path $root "output\graphics-diff"
        New-Item -ItemType Directory -Force -Path $gfx | Out-Null
        [System.IO.File]::WriteAllText((Join-Path $gfx "windows.log"), "DIFF END OK`r`n", $utf8)
        $gfxReport = '{"calls":[' +
            '{"name":"Gdi.Fill","windowsCompleted":true,"capabilityMatch":true,"reactos":"0x11111111"},' +
            '{"name":"Gdi.Rect","windowsCompleted":true,"capabilityMatch":false,"reactos":"0x22222222"},' +
            '{"name":"D3d11.Clear","windowsCompleted":true,"capabilityMatch":true,"reactos":"0x88888888"}' +
            ']}'
        [System.IO.File]::WriteAllText((Join-Path $gfx "report.json"), $gfxReport, $utf8)
        & $PSCommandPath -SourceRoot $root -Section Graphics -WebsitePath $sitePath
        $scored = Get-Content -LiteralPath $sitePath -Raw | ConvertFrom-Json
        $fb2 = @($scored.graphics | Where-Object { $_.label -eq "Software framebuffer" })[0]
        $drm2 = @($scored.graphics | Where-Object { $_.label -eq "DRM" })[0]
        $d3d2 = @($scored.graphics | Where-Object { $_.label -eq "D3D11" })[0]
        $wddm = @($scored.graphics | Where-Object { $_.label -eq "WDDM" })[0]
        if ($fb2.value -ne "Partial" -or $d3d2.value -ne "Working" -or $drm2.value -ne "Missing") {
            throw "measured graphics was $($fb2.value)/$($d3d2.value)/$($drm2.value)"
        }
        if ($wddm.value -ne "Partial") { throw "WDDM left the catalog and became $($wddm.value)" }
        $site2 = $filled
        $site2.syscalls[1].compatibility = 100
        $site2.overall = 100
        $site2.capturedAt = "2026-11-08T00:00:00Z"
        Write-JsonFile $sitePath $site2
        & $PSCommandPath -SourceRoot $root -Section Regressions -WebsitePath $sitePath -HistoryPath $history -SubsystemMap $map
        $again = Get-Content -LiteralPath $sitePath -Raw | ConvertFrom-Json
        if ($again.regression.previousOverall -ne 50 -or $again.regression.change -ne 50) {
            throw "regression was $($again.regression.previousOverall) -> $($again.regression.change)"
        }
        $improved = @($again.regression.improvements | Where-Object { $_.name -eq "NtCreateFile" })
        if ($improved.Count -ne 1 -or $improved[0].delta -ne 100) { throw "NtCreateFile delta missing" }
        Write-Host "Tab probe fixture passed."
    }
    finally {
        if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
    }
}

if (-not $SourceRoot) {
    $SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

if ($SelfTest) {
    Invoke-TabSelfTest
    exit 0
}

$sections = Get-SectionSet $Section
if (-not $WebsitePath) { $WebsitePath = Join-Path $SourceRoot "veros\dashboard\latest.json" }
if (-not $HistoryPath) { $HistoryPath = Join-Path $SourceRoot "veros\dashboard\history.json" }
if (-not $SubsystemMap) { $SubsystemMap = Join-Path $SourceRoot "compat\catalog\syscall-subsystems.json" }
if (-not $DriverGroups) { $DriverGroups = Join-Path $SourceRoot "compat\catalog\driver-abi-groups.json" }
if (-not $PluginLog) { $PluginLog = Join-Path $SourceRoot "output\abi-boot\abi.jsonl" }
if (-not $TraceDir) { $TraceDir = Join-Path $SourceRoot "output\windows-trace" }
if (-not $NameList) { $NameList = Join-Path $SourceRoot "compat\reference\win10-22h2-syscalls.txt" }

if (-not (Test-Path -LiteralPath $WebsitePath)) { throw "Website snapshot was not found at $WebsitePath" }
$site = Get-Content -LiteralPath $WebsitePath -Raw | ConvertFrom-Json
$filled = @()

if ($sections -contains "Regressions") {
    $site = Update-Regression -Site $site -Path $HistoryPath
    $filled += "regression"
}
if (($sections -contains "HeatMap") -or ($sections -contains "Tasks")) {
    $site = Update-HeatAndTask -Site $site -MapPath $SubsystemMap -Heat ($sections -contains "HeatMap") -IncludeTask ($sections -contains "Tasks")
    if ($sections -contains "HeatMap" -and $site.subsystems) { $filled += "subsystems" }
    if ($sections -contains "Tasks" -and $site.task) { $filled += "task" }
}
if ($sections -contains "Traces") {
    $allowed = Read-NameSet $NameList
    $trace = Read-TraceCounts -Directory $TraceDir -Allowed $allowed
    if ($trace) {
        Set-SiteField $site "traceWorkloads" @($trace.Workloads)
        Set-SiteField $site "traceCorpus" $trace.Corpus
        $filled += "traceWorkloads"
        if ($trace.Corpus) { $filled += "traceCorpus" }
    }
}
if ($sections -contains "DriverAbi") {
    if (Update-DriverAbi -Site $site -GroupPath $DriverGroups -LogPath $PluginLog) { $filled += "driverAbi" }
}
if ($sections -contains "Graphics") {
    Set-SiteField $site "graphics" @(Get-GraphicsRows $SourceRoot)
    $filled += "graphics"
}

$sentence = "Filled $($filled -join ', ') from the amd64 probe, the plugin log, the Windows trace counts, and the tree."
if ($sections -contains "Traces" -and -not ($filled -contains "traceCorpus")) {
    $sentence += " Trace records were counted. A numeric syscall id was not given a name."
}
$site = Set-TabNote -Site $site -Sentence $sentence
. (Join-Path $PSScriptRoot "DashboardStore.ps1")
$dashRoot = Split-Path -Parent $WebsitePath
$captured = [string]$site.capturedAt
if (-not $captured) { $captured = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ") }
if ($filled -contains "regression") {
    Write-DashboardSection -DashboardRoot $dashRoot -Name "regression" -CapturedAt $captured -Values $site
}
if (($filled -contains "subsystems") -or ($filled -contains "task")) {
    Write-DashboardSection -DashboardRoot $dashRoot -Name "areas" -CapturedAt $captured -Values $site
}
if (($filled -contains "traceWorkloads") -or ($filled -contains "traceCorpus")) {
    Write-DashboardSection -DashboardRoot $dashRoot -Name "traces" -CapturedAt $captured -Values $site
}
if ($filled -contains "driverAbi") {
    Write-DashboardSection -DashboardRoot $dashRoot -Name "driver-abi" -CapturedAt $captured -Values $site
}
if ($filled -contains "graphics") {
    Write-DashboardSection -DashboardRoot $dashRoot -Name "graphics" -CapturedAt $captured -Values $site
}
if ($site.notes -or $site.unmeasured) {
    Update-DashboardSection -DashboardRoot $dashRoot -Name "syscalls" -CapturedAt $captured -Values ([ordered]@{
        notes      = [string]$site.notes
        unmeasured = @($site.unmeasured)
    })
}
Write-Host ("Updated $WebsitePath " + ($filled -join ", "))
exit 0
