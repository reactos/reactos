# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/pipeline/Invoke-InstallBoot.ps1
# PURPOSE:     Boot a live, installed, or application image
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Live", "Installed", "App")]
    [string]$Mode,
    [string]$SourceRoot = "",
    [string]$Iso = "",
    [string]$Disk = "",
    [string]$Qemu = "",
    [string]$QemuImg = "",
    [string]$OutputDir = "",
    [string]$AppsDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $SourceRoot) {
    $SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}
. (Join-Path $PSScriptRoot "Resolve-VerosPaths.ps1")

if (-not $OutputDir) {
    $OutputDir = Join-Path $SourceRoot "output\install"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

if (-not $Disk) {
    $Disk = Join-Path $OutputDir "veros.qcow2"
}
if (-not $Iso) {
    $Iso = Join-Path $SourceRoot "build-amd64\bootcd.iso"
}
if (-not $Qemu) {
    $Qemu = Resolve-VerosQemuExe -SourceRoot $SourceRoot
}
if (-not $Qemu -or -not (Test-Path $Qemu)) {
    Write-Error "QEMU was not found under the buildenv tools\qemu folder."
}
if (-not $QemuImg) {
    $QemuImg = Join-Path (Split-Path $Qemu -Parent) "qemu-img.exe"
}
if (-not (Test-Path $QemuImg)) {
    Write-Error "qemu-img was not found at $QemuImg"
}

$running = Get-Process qemu-system-x86_64 -ErrorAction SilentlyContinue
if ($running) {
    Write-Error "QEMU is already running (pid $($running.Id -join ', ')). Stop it before opening this disk."
}

if (-not (Test-Path $Disk)) {
    & $QemuImg create -f qcow2 $Disk 16G
    if ($LASTEXITCODE -ne 0) {
        Write-Error "qemu-img could not create $Disk"
    }
}

if ($Mode -eq "Live" -and -not (Test-Path $Iso)) {
    Write-Error "ISO was not found at $Iso. Run Build-Bootcd.ps1 first."
}
if ($Mode -eq "App") {
    if (-not $AppsDir -or -not (Test-Path $AppsDir)) {
        Write-Error "App mode needs an existing -AppsDir."
    }
}

$stamp = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
$serial = Join-Path $OutputDir "serial-$Mode-$stamp.log"
$qemuLog = Join-Path $OutputDir "qemu-$Mode-$stamp.log"
$serialFwd = ($serial -replace '\\', '/')
$bios = Join-Path (Split-Path $Qemu -Parent) "share"
$diskArg = "-drive `"file=$Disk,format=qcow2,if=ide,index=0,media=disk`""

$argLine = "-accel tcg -L `"$bios`" -m 8192 -smp 4 -no-reboot -display gtk -name `"VerOS NT`" -serial `"file:$serialFwd`" $diskArg"
switch ($Mode) {
    "Live" {
        $argLine += " -cdrom `"$Iso`" -boot d"
    }
    "Installed" {
        $argLine += " -boot c"
    }
    "App" {
        $appsFwd = ($AppsDir -replace '\\', '/')
        $argLine += " -boot c -drive file=fat:rw:${appsFwd},format=raw,if=ide,index=1,media=disk"
    }
}

Write-Host "QEMU: $Qemu"
Write-Host "accelerator=tcg"
Write-Host "mode=$Mode"
Write-Host "disk=$Disk"

$proc = Start-Process -FilePath $Qemu -ArgumentList $argLine -PassThru -RedirectStandardError $qemuLog
$summary = [ordered]@{
    accelerator = "tcg"
    mode        = $Mode
    disk        = $Disk
    iso         = $(if ($Mode -eq "Live") { $Iso } else { $null })
    appsDir     = $(if ($Mode -eq "App") { $AppsDir } else { $null })
    serialLog   = $serial
    qemuLog     = $qemuLog
    pid         = $proc.Id
}
$summary | ConvertTo-Json | Set-Content -Path (Join-Path $OutputDir "boot-latest.json")
Write-Host "Serial log: $serial"
Write-Host "PID: $($proc.Id)"
exit 0
