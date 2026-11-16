# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/qemu-harness/New-ExportMap.ps1
# PURPOSE:     Write the ntoskrnl, hal, and wdf export map
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Convert-RvaToOffset {
    param([byte[]]$Bytes, [int]$Pe, [uint32]$Rva)
    $count = [BitConverter]::ToUInt16($Bytes, $Pe + 6)
    $optional = [BitConverter]::ToUInt16($Bytes, $Pe + 20)
    $section = $Pe + 24 + $optional
    for ($i = 0; $i -lt $count; $i++) {
        $off = $section + (40 * $i)
        $virtual = [BitConverter]::ToUInt32($Bytes, $off + 12)
        $rawSize = [BitConverter]::ToUInt32($Bytes, $off + 16)
        $raw = [BitConverter]::ToUInt32($Bytes, $off + 20)
        if ($Rva -ge $virtual -and $Rva -lt ($virtual + $rawSize)) {
            return [int]($raw + ($Rva - $virtual))
        }
    }
    return $null
}

function Read-CString {
    param([byte[]]$Bytes, [int]$Offset)
    $end = $Offset
    while ($end -lt $Bytes.Length -and $Bytes[$end] -ne 0) { $end++ }
    return [Text.Encoding]::ASCII.GetString($Bytes, $Offset, $end - $Offset)
}

function Get-ExportMap {
    param([string]$Path, [string]$Module, [hashtable]$Wanted)
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    $magic = [BitConverter]::ToUInt16($bytes, $pe + 24)
    if ($magic -ne 0x20B) { throw "$Path is not a PE32+ image" }
    $imageBase = [BitConverter]::ToUInt64($bytes, $pe + 48)
    $imageSize = [BitConverter]::ToUInt32($bytes, $pe + 80)
    $exportRva = [BitConverter]::ToUInt32($bytes, $pe + 136)
    $exportSize = [BitConverter]::ToUInt32($bytes, $pe + 140)
    $exportOff = Convert-RvaToOffset $bytes $pe $exportRva
    if ($null -eq $exportOff) { throw "$Module export directory is not in a section" }
    $names = [BitConverter]::ToUInt32($bytes, $exportOff + 24)
    $funcsRva = [BitConverter]::ToUInt32($bytes, $exportOff + 28)
    $namesRva = [BitConverter]::ToUInt32($bytes, $exportOff + 32)
    $ordsRva = [BitConverter]::ToUInt32($bytes, $exportOff + 36)
    $funcsOff = Convert-RvaToOffset $bytes $pe $funcsRva
    $namesOff = Convert-RvaToOffset $bytes $pe $namesRva
    $ordsOff = Convert-RvaToOffset $bytes $pe $ordsRva
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add(("# image {0} 0x{1:x} 0x{2:x}" -f $Module, $imageBase, $imageSize)) | Out-Null
    for ($i = 0; $i -lt $names; $i++) {
        $nameRva = [BitConverter]::ToUInt32($bytes, $namesOff + (4 * $i))
        $nameOff = Convert-RvaToOffset $bytes $pe $nameRva
        $name = Read-CString $bytes $nameOff
        if (-not $Wanted.ContainsKey($name)) { continue }
        $ordinal = [BitConverter]::ToUInt16($bytes, $ordsOff + (2 * $i))
        $funcRva = [BitConverter]::ToUInt32($bytes, $funcsOff + (4 * $ordinal))
        $forwarded = $funcRva -ge $exportRva -and $funcRva -lt ($exportRva + $exportSize)
        if ($forwarded) { continue }
        $lines.Add(("{0}!{1} 0x{2:x}" -f $Module, $name, ($imageBase + $funcRva))) | Out-Null
        $Wanted[$name] = $true
    }
    return $lines
}

function Read-Wanted {
    param([string]$Path)
    $wanted = @{}
    foreach ($line in [IO.File]::ReadAllLines($Path)) {
        $name = $line.Trim()
        if (-not $name -or $name.StartsWith("#")) { continue }
        $wanted[$name] = $false
    }
    return $wanted
}

if (-not $OutputPath) {
    $OutputPath = Join-Path $SourceRoot "output\abi-boot\exports.map"
}
$kernelWanted = Read-Wanted (Join-Path $SourceRoot "compat\reference\win10-22h2-ntoskrnl.txt")
$halWanted = Read-Wanted (Join-Path $SourceRoot "compat\reference\win10-22h2-hal.txt")
$kernel = Get-ExportMap (Join-Path $SourceRoot "build-amd64\ntoskrnl\ntoskrnl.exe") "ntoskrnl.exe" $kernelWanted
$hal = Get-ExportMap (Join-Path $SourceRoot "build-amd64\hal\halx86\hal.dll") "hal.dll" $halWanted
$dir = Split-Path $OutputPath -Parent
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$all = @()
$all += $kernel
$all += $hal
$utf8 = New-Object System.Text.UTF8Encoding $false
[IO.File]::WriteAllLines($OutputPath, $all, $utf8)
$found = @($all | Where-Object { $_ -notlike "#*" }).Count
Write-Host "Wrote $found exports to $OutputPath"
$missing = @()
foreach ($pair in $kernelWanted.GetEnumerator()) { if (-not $pair.Value) { $missing += $pair.Key } }
foreach ($pair in $halWanted.GetEnumerator()) { if (-not $pair.Value) { $missing += $pair.Key } }
if ($missing.Count -gt 0) {
    Write-Host ("Not exported by this build: " + ($missing -join ", "))
}
