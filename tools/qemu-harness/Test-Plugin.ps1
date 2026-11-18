# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/qemu-harness/Test-Plugin.ps1
# PURPOSE:     Smoke-test the QEMU ABI plugin
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$Qemu = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $SourceRoot "tools\pipeline\Resolve-VerosPaths.ps1")
$here = $PSScriptRoot
$out = Join-Path $SourceRoot "output\plugin-test"
New-Item -ItemType Directory -Force -Path $out | Out-Null

if (-not $Qemu) {
    $localI386 = Resolve-VerosToolPath -SourceRoot $SourceRoot -RelativePath "tools\qemu\qemu-system-i386.exe"
    if (Test-Path -LiteralPath $localI386) {
        $Qemu = (Resolve-Path -LiteralPath $localI386).Path
    }
    else {
        foreach ($base in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
            if (-not $base) { continue }
            $fallback = Join-Path $base "qemu\qemu-system-i386.exe"
            if (Test-Path -LiteralPath $fallback) {
                $Qemu = (Resolve-Path -LiteralPath $fallback).Path
                break
            }
        }
    }
}
if (-not $Qemu -or -not (Test-Path $Qemu)) {
    Write-Error "QEMU i386 was not found under the buildenv tools\qemu folder."
}

function Find-Gcc64 {
    $names = @(
        "x86_64-w64-mingw32-gcc.exe",
        "gcc.exe"
    )
    $dirs = New-Object System.Collections.Generic.List[string]
    $rosbe = Resolve-VerosRosbeRoot -SourceRoot $SourceRoot
    if ($rosbe) {
        $dirs.Add((Join-Path $rosbe "amd64\bin")) | Out-Null
        $dirs.Add((Join-Path $rosbe "bin")) | Out-Null
    }
    if ($env:ProgramFiles) {
        $dirs.Add((Join-Path $env:ProgramFiles "Git\mingw64\bin")) | Out-Null
    }
    foreach ($dir in $dirs) {
        foreach ($name in $names) {
            $path = Join-Path $dir $name
            if (Test-Path $path) { return $path }
        }
    }
    $cmd = Get-Command gcc.exe, x86_64-w64-mingw32-gcc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cmd) { return $cmd.Source }
    return $null
}

$gcc = Find-Gcc64
if (-not $gcc) {
    Write-Error "No gcc was found to build abi.dll. Install RosBE or a 64-bit mingw-w64 gcc."
}
$gccDir = Split-Path $gcc -Parent
$rosbeBin = Join-Path (Split-Path (Split-Path $gccDir -Parent) -Parent) "bin"
if (Test-Path $rosbeBin) {
    $env:PATH = "$rosbeBin;$gccDir;" + $env:PATH
}

$dll = Join-Path $here "abi.dll"
& $gcc -shared -O2 -o $dll (Join-Path $here "abi.c")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$bytes = [byte[]]@(
    0xFA, 0x31, 0xC0, 0x8E, 0xD8, 0x8E, 0xC0,
    0xC7, 0x06, 0xB8, 0x00, 0x1B, 0x7C,
    0xC7, 0x06, 0xBA, 0x00, 0x00, 0x00,
    0xE8, 0x06, 0x00,
    0xCD, 0x2E,
    0x0F, 0x34,
    0xF4,
    0xCF,
    0xC3
)
$image = New-Object byte[] 512
[Array]::Copy($bytes, $image, $bytes.Length)
$image[510] = 0x55
$image[511] = 0xAA
$img = Join-Path $out "guest.img"
[IO.File]::WriteAllBytes($img, $image)
$map = Join-Path $out "exports.map"
Set-Content -Path $map -Value "ntoskrnl.exe!IoCallDriver 0x7C1C"
$log = Join-Path $out "abi.jsonl"
$err = Join-Path $out "qemu.err"
$dllFwd = $dll -replace '\\', '/'
$logFwd = $log -replace '\\', '/'
$mapFwd = $map -replace '\\', '/'
$bios = Join-Path (Split-Path $Qemu -Parent) "share"
$qemuArgs = "-accel tcg -L `"$bios`" -m 64 -fda `"$img`" -boot a -display none -no-reboot -plugin `"file=$dllFwd,log=$logFwd,map=$mapFwd`""
$proc = Start-Process -FilePath $Qemu -ArgumentList $qemuArgs -PassThru -RedirectStandardError $err -WindowStyle Hidden
if (-not $proc.WaitForExit(15000)) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}
if (-not (Test-Path $log)) {
    Write-Host (Get-Content $err -Raw -ErrorAction SilentlyContinue)
    Write-Error "Plugin log was not written."
}
$text = Get-Content $log -Raw
if ($text -notmatch '"kind":"syscall"') { Write-Error "Missing syscall event in $log" }
if ($text -notmatch 'IoCallDriver') { Write-Error "Missing ntoskrnl export event in $log" }
Write-Host "Plugin fixture passed."

$winBytes = [byte[]]@(
    0x66, 0x31, 0xC0, 0x8E, 0xD8, 0x8E, 0xC0, 0xFA,
    0xC7, 0x06, 0xB8, 0x00, 0x1D, 0x7C,
    0xC7, 0x06, 0xBA, 0x00, 0x00, 0x00,
    0x66, 0xB8, 0x55, 0x00, 0x00, 0x00,
    0xCD, 0x2E,
    0xF4,
    0x66, 0x31, 0xC0,
    0xCF
)
$winImage = New-Object byte[] 512
[Array]::Copy($winBytes, $winImage, $winBytes.Length)
$winImage[510] = 0x55
$winImage[511] = 0xAA
$winImg = Join-Path $out "win-guest.img"
[IO.File]::WriteAllBytes($winImg, $winImage)
$numbers = Join-Path $out "syscalls.txt"
Set-Content -Path $numbers -Value "85 NtCreateFile"
$winLog = Join-Path $out "windows.jsonl"
$winErr = Join-Path $out "windows.err"
$winLogFwd = $winLog -replace '\\', '/'
$numbersFwd = $numbers -replace '\\', '/'
$status = Join-Path $SourceRoot "tools\windows-trace\ntstatus.txt"
$statusFwd = $status -replace '\\', '/'
$winArgs = "-accel tcg -L `"$bios`" -m 64 -fda `"$winImg`" -boot a -display none -no-reboot -plugin `"file=$dllFwd,log=$winLogFwd,mode=windows,syscalls=$numbersFwd,status=$statusFwd`""
$win = Start-Process -FilePath $Qemu -ArgumentList $winArgs -PassThru -RedirectStandardError $winErr -WindowStyle Hidden
if (-not $win.WaitForExit(15000)) {
    Stop-Process -Id $win.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}
if (-not (Test-Path $winLog)) {
    Write-Host (Get-Content $winErr -Raw -ErrorAction SilentlyContinue)
    Write-Error "Windows-mode log was not written."
}
$winText = Get-Content $winLog -Raw
if ($winText -notmatch '"syscall":"NtCreateFile"') { Write-Error "Windows mode did not name NtCreateFile" }
if ($winText -notmatch '"status":"STATUS_SUCCESS"') { Write-Error "Windows mode did not record STATUS_SUCCESS" }
Write-Host "Windows-mode fixture passed."
Write-Host $log
