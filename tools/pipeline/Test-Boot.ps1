# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/pipeline/Test-Boot.ps1
# PURPOSE:     Boot the bootcd and wait for the desktop
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$Iso = "",
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [int]$TimeoutSec = 1200,
    [string]$Qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe",
    [string]$Plugin = "",
    [string]$Map = "",
    [string]$OutputDir = "",
    [string]$AppsDir = "",
    [switch]$ShowWindow
)

$ErrorActionPreference = "Stop"

if (-not $OutputDir) {
    $OutputDir = Join-Path $SourceRoot "output"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

if (-not $Iso) {
    $Iso = Join-Path $SourceRoot "build-amd64\bootcd.iso"
}
if (-not $PSBoundParameters.ContainsKey("Qemu")) {
    $localQemu = Join-Path (Split-Path $SourceRoot -Parent) "tools\qemu\qemu-system-x86_64.exe"
    if (Test-Path $localQemu) {
        $Qemu = $localQemu
    }
}
if (-not (Test-Path $Qemu)) {
    Write-Error "QEMU was not found at $Qemu"
}
if (-not (Test-Path $Iso)) {
    Write-Error "ISO was not found at $Iso. Run Build-Bootcd.ps1 first."
}

if (-not $Plugin) {
    $Candidate = Join-Path $SourceRoot "tools\qemu-harness\abi.dll"
    if (Test-Path $Candidate) {
        $Plugin = $Candidate
    }
}

$stamp = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
$serial = Join-Path $OutputDir "boot-$stamp.log"
$jsonl = Join-Path $OutputDir "abi-$stamp.jsonl"
$qemuLog = Join-Path $OutputDir "qemu-$stamp.log"

$bios = Join-Path (Split-Path $Qemu -Parent) "share"
$argLine = "-accel tcg -L `"$bios`" -m 8192 -smp 4 -cdrom `"$Iso`" -boot d -no-reboot -serial `"file:$serial`""
if ($ShowWindow) {
    $argLine += " -display gtk -name `"VerOS NT`""
} else {
    $argLine += " -display none"
}
if ($AppsDir) {
    if (-not (Test-Path $AppsDir)) {
        Write-Error "Apps directory was not found at $AppsDir"
    }
    $appsFwd = ($AppsDir -replace '\\', '/')
    $argLine += " -drive file=fat:rw:${appsFwd},format=raw,if=ide,index=0,media=disk"
}
if ($Plugin) {
    $pluginFwd = ($Plugin -replace '\\', '/')
    $jsonlFwd = ($jsonl -replace '\\', '/')
    $pluginArg = "file=$pluginFwd,log=$jsonlFwd"
    if ($Map) {
        $pluginArg += ",map=" + ($Map -replace '\\', '/')
    }
    $argLine += " -plugin `"$pluginArg`""
}

Write-Host "QEMU: $Qemu"
Write-Host "accelerator=tcg"
$start = @{
    FilePath = $Qemu
    ArgumentList = $argLine
    PassThru = $true
    RedirectStandardError = $qemuLog
}
if (-not $ShowWindow) {
    $start.WindowStyle = "Hidden"
}
$proc = Start-Process @start
$finished = $proc.WaitForExit($TimeoutSec * 1000)
if (-not $finished) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    $result = "hang"
}
elseif ($proc.ExitCode -eq 0) {
    $result = "exited"
}
else {
    $result = "exited"
}

$summary = [ordered]@{
    accelerator = "tcg"
    iso         = $Iso
    serialLog   = $serial
    pluginLog   = $jsonl
    qemuLog     = $qemuLog
    result      = $result
    timeoutSec  = $TimeoutSec
}
$summary | ConvertTo-Json | Set-Content -Path (Join-Path $OutputDir "boot-latest.json")
Write-Host "Serial log: $serial"
if ($result -eq "hang") {
    exit 2
}

$installedDisk = Join-Path (Split-Path $SourceRoot -Parent) "virtual-machines\veros-ntfs\disk.qcow2"
if (-not (Test-Path $installedDisk)) {
    Write-Host "Installed disk was not found at $installedDisk"
    exit 0
}

$installedStamp = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
$installedSerial = Join-Path $OutputDir "installed-$installedStamp.log"
$installedQemuLog = Join-Path $OutputDir "qemu-installed-$installedStamp.log"
$installedArgs = "-accel tcg -L `"$bios`" -m 8192 -smp 4 -drive `"file=$installedDisk,format=qcow2,if=ide,index=0,media=disk`" -boot c -no-reboot -serial `"file:$installedSerial`""
if ($ShowWindow) {
    $installedArgs += " -display gtk -name `"VerOS NT`""
} else {
    $installedArgs += " -display none"
}
Write-Host "Installed disk: $installedDisk"
$installedStart = @{
    FilePath = $Qemu
    ArgumentList = $installedArgs
    PassThru = $true
    RedirectStandardError = $installedQemuLog
}
if (-not $ShowWindow) {
    $installedStart.WindowStyle = "Hidden"
}
$installedProc = Start-Process @installedStart
$installedFinished = $installedProc.WaitForExit($TimeoutSec * 1000)
if (-not $installedFinished) {
    Stop-Process -Id $installedProc.Id -Force -ErrorAction SilentlyContinue
    $installedResult = "hang"
} else {
    $installedResult = "exited"
}
$installedSummary = [ordered]@{
    accelerator = "tcg"
    disk        = $installedDisk
    serialLog   = $installedSerial
    qemuLog     = $installedQemuLog
    result      = $installedResult
    timeoutSec  = $TimeoutSec
}
$installedSummary | ConvertTo-Json | Set-Content -Path (Join-Path $OutputDir "installed-boot-latest.json")
Write-Host "Installed serial log: $installedSerial"
if ($installedResult -eq "hang") {
    exit 2
}
exit 0
