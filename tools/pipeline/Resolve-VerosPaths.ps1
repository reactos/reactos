# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/pipeline/Resolve-VerosPaths.ps1
# PURPOSE:     Resolve buildenv tool paths relative to the source tree
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
# PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>

# Dot-source from source/tools scripts. No drive letters. Discovery walks
# from the source root looking for env.cmd + tools/ in the parent or a
# sibling buildenv/ folder.
#
# Layouts:
#   <ws>\source  +  <ws>\buildenv\tools   (side by side)
#   <ws>\source  +  <ws>\tools            (source nested in buildenv)

function Test-VerosBuildenvRoot {
    param([string]$Path)
    if (-not $Path) { return $false }
    return (Test-Path -LiteralPath (Join-Path $Path "env.cmd")) -and
        (Test-Path -LiteralPath (Join-Path $Path "tools"))
}

function Resolve-VerosBuildenvRoot {
    param([string]$SourceRoot)

    $seen = @{}
    $candidates = New-Object System.Collections.Generic.List[string]
    function Add-Candidate([string]$Path) {
        if (-not $Path) { return }
        try { $full = [IO.Path]::GetFullPath($Path) } catch { return }
        if ($seen.ContainsKey($full)) { return }
        $seen[$full] = $true
        $candidates.Add($full) | Out-Null
    }

    $cursor = $SourceRoot
    for ($i = 0; $i -lt 6; $i++) {
        $parent = Split-Path -Parent $cursor
        if (-not $parent -or $parent -eq $cursor) { break }
        Add-Candidate $parent
        Add-Candidate (Join-Path $parent "buildenv")
        $cursor = $parent
    }

    foreach ($candidate in $candidates) {
        if (Test-VerosBuildenvRoot $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Buildenv root was not found beside $SourceRoot. Expected env.cmd and tools/."
}

function Resolve-VerosToolPath {
    param(
        [string]$SourceRoot,
        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $buildenv = Resolve-VerosBuildenvRoot -SourceRoot $SourceRoot
    return (Join-Path $buildenv $RelativePath)
}

function Resolve-VerosRosbeRoot {
    param([string]$SourceRoot)

    $local = Resolve-VerosToolPath -SourceRoot $SourceRoot -RelativePath "tools\rosbe"
    if (Test-Path -LiteralPath (Join-Path $local "rosbe-gcc-env.cmd")) {
        return (Resolve-Path -LiteralPath $local).Path
    }
    return $null
}

function Resolve-VerosQemuExe {
    param([string]$SourceRoot)

    $local = Resolve-VerosToolPath -SourceRoot $SourceRoot -RelativePath "tools\qemu\qemu-system-x86_64.exe"
    if (Test-Path -LiteralPath $local) {
        return (Resolve-Path -LiteralPath $local).Path
    }
    foreach ($base in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
        if (-not $base) { continue }
        $fallback = Join-Path $base "qemu\qemu-system-x86_64.exe"
        if (Test-Path -LiteralPath $fallback) {
            return (Resolve-Path -LiteralPath $fallback).Path
        }
    }
    return $null
}

function Resolve-VerosClangBin {
    param([string]$SourceRoot)

    $local = Resolve-VerosToolPath -SourceRoot $SourceRoot -RelativePath "tools\clang\bin\clang.exe"
    if (Test-Path -LiteralPath $local) {
        return (Split-Path -Parent (Resolve-Path -LiteralPath $local).Path)
    }
    if ($env:ProgramFiles) {
        $fallback = Join-Path $env:ProgramFiles "LLVM\bin\clang.exe"
        if (Test-Path -LiteralPath $fallback) {
            return (Split-Path -Parent (Resolve-Path -LiteralPath $fallback).Path)
        }
    }
    return $null
}
