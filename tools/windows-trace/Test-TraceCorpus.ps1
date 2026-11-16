# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/windows-trace/Test-TraceCorpus.ps1
# PURPOSE:     Check that a Windows trace corpus is large enough
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [int]$Minimum = 1000000
)

$ErrorActionPreference = "Stop"
$trace = Join-Path $SourceRoot "output\windows-trace"
$names = @("notepad", "explorer", "office", "browser")
$total = 0
foreach ($name in $names) {
    $path = Join-Path $trace "$name.jsonl"
    $mark = Join-Path $trace "$name.started"
    if (-not (Test-Path $path)) { Write-Error "Missing $path" }
    if (-not (Test-Path $mark)) { Write-Error "Missing workload marker $mark" }
    $count = 0
    $sawStatus = $false
    Get-Content -Path $path -ReadCount 4000 | ForEach-Object {
        $count += $_.Count
        if (-not $sawStatus) {
            foreach ($line in $_) {
                if ($line -match '"syscall":' -and $line -match '"status":') { $sawStatus = $true; break }
            }
        }
    }
    if (-not $sawStatus) { Write-Error "$name has no syscall status records" }
    if ($count -lt 1) { Write-Error "$name is empty" }
    $total += $count
    Write-Host "$name $count"
}
if ($total -lt $Minimum) { Write-Error "Corpus has $total records, need $Minimum" }
Write-Host "Corpus $total"
