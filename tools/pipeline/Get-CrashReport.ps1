# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/pipeline/Get-CrashReport.ps1
# PURPOSE:     Write a crash report from a serial log
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SerialLog = "",
    [string]$PluginLog = "",
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$OutPath = "",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function Get-VersionFromCmake {
    param([string]$Root)
    $path = Join-Path $Root "sdk\include\reactos\version.cmake"
    $text = Get-Content $path -Raw
    $major = [regex]::Match($text, 'KERNEL_VERSION_MAJOR "([^"]+)"').Groups[1].Value
    $minor = [regex]::Match($text, 'KERNEL_VERSION_MINOR "([^"]+)"').Groups[1].Value
    $patch = [regex]::Match($text, 'KERNEL_VERSION_PATCH_LEVEL "([^"]+)"').Groups[1].Value
    $type = [regex]::Match($text, 'KERNEL_VERSION_BUILD_TYPE "([^"]+)"').Groups[1].Value
    $version = "$major.$minor.$patch"
    if ($type) { $version = "$version-$type" }
    return $version
}

function Get-GitSha {
    param([string]$Root)
    try {
        return (& git -C $Root rev-parse --short HEAD).Trim()
    }
    catch {
        return "unknown"
    }
}

function Get-ReactOSCrashReport {
    param(
        [string]$SerialText,
        [string]$PluginText,
        [string]$Version,
        [string]$GitSha
    )

    $bug = [regex]::Match($SerialText, "Fatal System Error:\s*(0x[0-9A-Fa-f]+)")
    $params = [regex]::Match($SerialText, "Fatal System Error:\s*0x[0-9A-Fa-f]+\s*\r?\n\s*(\([^\)]*\))")
    $asserts = [regex]::Matches($SerialText, "Assertion '([^']*)'") | ForEach-Object { $_.Groups[1].Value }
    $frames = [regex]::Matches($SerialText, "<[^>\r\n]*:[^>\r\n]*>") | ForEach-Object { $_.Value }
    $lines = $SerialText -split "\r?\n"
    $tail = @()
    if ($lines.Count -gt 40) {
        $tail = $lines[($lines.Count - 40)..($lines.Count - 1)]
    }
    else {
        $tail = $lines
    }

    $inFlight = $null
    if ($PluginText) {
        $events = @()
        foreach ($line in ($PluginText -split "\r?\n")) {
            if ($line.Trim().StartsWith("{")) {
                $events += ($line | ConvertFrom-Json)
            }
        }
        $open = @{}
        foreach ($event in $events) {
            $name = [string]$event.symbol
            if (-not $name) { continue }
            if ($event.phase -eq "enter") { $open[$name] = $event }
            elseif ($event.phase -eq "return" -and $open.ContainsKey($name)) { $open.Remove($name) }
        }
        if ($open.Count -gt 0) {
            $inFlight = @($open.Values)[-1]
        }
    }

    $code = $null
    if ($bug.Success) { $code = $bug.Groups[1].Value }
    $kind = "ok"
    if ($code) { $kind = "bugcheck" }
    elseif ($SerialText -match "ReactOS") { $kind = "booted" }
    else { $kind = "hang" }
    if ($kind -ne "bugcheck") { $inFlight = $null }

    return [ordered]@{
        kind          = $kind
        bugcheck      = $code
        parameters    = $(if ($params.Success) { $params.Groups[1].Value } else { $null })
        assertions    = @($asserts)
        frames        = @($frames)
        tail          = @($tail)
        inFlightCall  = $inFlight
        version       = $Version
        git           = $GitSha
    }
}

function Invoke-CrashSelfTest {
    $fixture = Join-Path $PSScriptRoot "fixtures\bugcheck.log"
    $plugin = Join-Path $PSScriptRoot "fixtures\inflight.jsonl"
    $serial = Get-Content $fixture -Raw
    $pluginText = Get-Content $plugin -Raw
    $report = Get-ReactOSCrashReport -SerialText $serial -PluginText $pluginText -Version "0.4.17-dev" -GitSha "fixture"
    if ($report.bugcheck -ne "0x0000001E") { throw "bugcheck code mismatch: $($report.bugcheck)" }
    if ($report.parameters -ne "(0xC0000005,0x00000000,0x00000000,0x00000000)") { throw "parameters mismatch" }
    if ($report.frames.Count -lt 1) { throw "missing frames" }
    if ($report.assertions.Count -lt 1) { throw "missing assertion" }
    if ($report.inFlightCall.symbol -ne "ntoskrnl.exe!IoCallDriver") { throw "in-flight call was not attached" }
    Write-Host "Crash report fixture passed."
}

if ($SelfTest) {
    Invoke-CrashSelfTest
    exit 0
}

if (-not $SerialLog) {
    Write-Error "Pass -SerialLog or -SelfTest."
}
$serialText = ""
if (Test-Path $SerialLog) { $serialText = Get-Content $SerialLog -Raw }
$pluginText = ""
if ($PluginLog -and (Test-Path $PluginLog)) { $pluginText = Get-Content $PluginLog -Raw }
$report = Get-ReactOSCrashReport -SerialText $serialText -PluginText $pluginText -Version (Get-VersionFromCmake $SourceRoot) -GitSha (Get-GitSha $SourceRoot)
if (-not $OutPath) {
    $dir = Join-Path $SourceRoot "output\crashes"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $stamp = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
    $OutPath = Join-Path $dir "$stamp-$($report.git).json"
}
$report | ConvertTo-Json -Depth 6 | Set-Content -Path $OutPath
Write-Host "Crash report: $OutPath"
if ($report.kind -eq "bugcheck" -or $report.kind -eq "hang") { exit 1 }
exit 0
