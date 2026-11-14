# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/pipeline/Build-Bootcd.ps1
# PURPOSE:     Build the amd64 bootcd
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$BuildDir = "",
    [string]$RosBE = "",
    [string]$Arch = "amd64",
    [switch]$Publish,
    [switch]$ClientOnly,
    [switch]$RepackOnly,
    [ValidateSet("dev", "beta", "stable")]
    [string]$Track = "dev",
    [string]$BootstrapUrl = "",
    [string]$UpdateUrl = "",
    [string]$PublishUrl = ""
)

$ErrorActionPreference = "Stop"

function Resolve-UpdateServer {
    $checkout = Split-Path $SourceRoot -Parent
    $configPath = Join-Path $checkout "veros-update.json"
    $resolved = @{
        BootstrapUrl = "https://bootstrap.invalid"
        UpdateUrl = "https://update.invalid"
        PublishUrl = "http://127.0.0.1:9080"
    }
    if (Test-Path $configPath) {
        $cfg = Get-Content $configPath -Raw | ConvertFrom-Json
        if ($cfg.bootstrapUrl) { $resolved.BootstrapUrl = [string]$cfg.bootstrapUrl }
        if ($cfg.updateUrl) { $resolved.UpdateUrl = [string]$cfg.updateUrl }
        if ($cfg.publishUrl) { $resolved.PublishUrl = [string]$cfg.publishUrl }
    }
    if ($BootstrapUrl) { $resolved.BootstrapUrl = $BootstrapUrl }
    if ($UpdateUrl) { $resolved.UpdateUrl = $UpdateUrl }
    if ($PublishUrl) { $resolved.PublishUrl = $PublishUrl }
    return $resolved
}

$UpdateServer = Resolve-UpdateServer
$env:VEROS_BOOTSTRAP_URL = $UpdateServer.BootstrapUrl
$env:VEROS_UPDATE_URL = $UpdateServer.UpdateUrl
$env:VEROS_PUBLISH_URL = $UpdateServer.PublishUrl
$env:UPDATE_PUBLISH_URL = $UpdateServer.PublishUrl
$env:VEROS_UPDATE_TRACK = $Track

function Find-RosBE {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint "rosbe-gcc-env.cmd"))) {
        return (Resolve-Path $Hint).Path
    }
    $local = Join-Path (Split-Path $SourceRoot -Parent) "tools\rosbe"
    foreach ($candidate in @($local, "C:\RosBE", "D:\RosBE", "$env:SystemDrive\RosBE")) {
        if (Test-Path (Join-Path $candidate "rosbe-gcc-env.cmd")) {
            return $candidate
        }
    }
    return $null
}

if ($Arch -ne "amd64") {
    Write-Error "ARCH=amd64 is the only supported OS build. A native i386 boot is not supported. 32-bit applications run on the amd64 boot."
}
if (-not $BuildDir) {
    $BuildDir = Join-Path $SourceRoot "build-amd64"
}
$RosBERoot = Find-RosBE -Hint $RosBE
if (-not $RosBERoot) {
    $logDir = Join-Path $SourceRoot "output"
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    $message = "RosBE was not found. Install RosBE 2.2.1 for Windows and re-run. See docs/LOCAL_BUILD.md."
    Set-Content -Path (Join-Path $logDir "build-error.txt") -Value $message
    Write-Error $message
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
if ($ClientOnly -or $RepackOnly) {
    Write-Host "Skipping the OS compile."
} else {
$buildArgs = "--build `"$BuildDir`" --target bootcd"
$runner = Join-Path $BuildDir "build-bootcd.cmd"
$clang = $null
$localClang = Join-Path (Split-Path $SourceRoot -Parent) "tools\clang\bin\clang.exe"
foreach ($candidate in @(
    $localClang,
    "D:\LLVM-18\clang+llvm-18.1.8-x86_64-pc-windows-msvc\bin\clang.exe",
    "C:\Program Files\LLVM\bin\clang.exe"
)) {
    if (Test-Path $candidate) { $clang = $candidate; break }
}
if (-not $clang) {
    Write-Error "amd64 needs Clang. RosBE's amd64 GCC was built without plugin support, so it cannot compile this tree."
}
$clangBin = Split-Path $clang -Parent
$toolchain = Join-Path $SourceRoot "toolchain-clang.cmake"
$cmakeArgs = "-S `"$SourceRoot`" -B `"$BuildDir`" -G Ninja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=`"$toolchain`" -DARCH:STRING=amd64 -DCMAKE_BUILD_TYPE=Debug -DENABLE_CCACHE:BOOL=0"
$lines = @(
    "@echo off",
    "set ROS_ARCH=amd64",
    "set _ROSBE_ROSSCRIPTDIR=$RosBERoot",
    "set BISON_PKGDATADIR=$RosBERoot\share\bison",
    "set M4=$RosBERoot\bin\m4.exe",
    "set PATH=$clangBin;$RosBERoot\amd64\bin;$RosBERoot\bin;%PATH%",
    "set VEROS_BOOTSTRAP_URL=$($UpdateServer.BootstrapUrl)",
    "set VEROS_UPDATE_URL=$($UpdateServer.UpdateUrl)",
    "set VEROS_PUBLISH_URL=$($UpdateServer.PublishUrl)",
    "set UPDATE_PUBLISH_URL=$($UpdateServer.PublishUrl)",
    "set VEROS_UPDATE_TRACK=$Track"
)
$lines += @(
    "cmake $cmakeArgs",
    "if errorlevel 1 exit /b %errorlevel%",
    "set TRIES=0",
    ":linkretry",
    "cmake $buildArgs -- -j 8",
    "if not errorlevel 1 goto :buildok",
    "set /a TRIES+=1",
    "if %TRIES% lss 8 goto :linkretry",
    "exit /b 1",
    ":buildok"
)
$lines | Set-Content -Path $runner -Encoding ASCII
Write-Host "Building $Arch Debug bootcd with $RosBERoot"
& cmd.exe /d /c "`"$runner`""
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
}

function Get-UpdaterInputHash {
    param([string]$ClientRoot)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $files = Get-ChildItem $ClientRoot -Recurse -File | Where-Object {
        $relative = $_.FullName.Substring($ClientRoot.Length).TrimStart('\')
        $relative -notmatch '^(target|dist)\\' -and $_.Extension -match '^\.(rs|toml|json|crt|sig)$'
    } | Sort-Object FullName
    foreach ($file in $files) {
        $name = [Text.Encoding]::UTF8.GetBytes($file.FullName.Substring($ClientRoot.Length))
        $sha.TransformBlock($name, 0, $name.Length, $null, 0) | Out-Null
        $bytes = [IO.File]::ReadAllBytes($file.FullName)
        $sha.TransformBlock($bytes, 0, $bytes.Length, $null, 0) | Out-Null
    }
    $sha.TransformFinalBlock(@(), 0, 0) | Out-Null
    return ([BitConverter]::ToString($sha.Hash)).Replace("-", "").ToLower()
}

function Sync-UpdaterClient {
    param([string]$ClientRoot, [string]$Track, [string]$PublishUrl)
    $dist = Join-Path $ClientRoot "dist"
    New-Item -ItemType Directory -Force -Path $dist | Out-Null
    $hash = Get-UpdaterInputHash -ClientRoot $ClientRoot
    $stampPath = Join-Path $dist "stamp.json"
    if (Test-Path $stampPath) {
        $stamp = Get-Content $stampPath -Raw | ConvertFrom-Json
        if ($stamp.hash -eq $hash -and (Test-Path (Join-Path $dist "verosupd-setup.exe"))) {
            Write-Host "Updater inputs are unchanged. Reusing dist."
            return
        }
    }
    $checkout = Split-Path (Split-Path $ClientRoot -Parent) -Parent
    $env:RUSTUP_HOME = Join-Path $checkout "tools\rust\rustup"
    $env:CARGO_HOME = Join-Path $checkout "tools\rust\cargo"
    $env:CARGO_TARGET_DIR = Join-Path $ClientRoot "target"
    $env:RUSTUP_TOOLCHAIN = "1.99.1-x86_64-pc-windows-gnu"
    $clang = Join-Path $checkout "tools\clang\bin"
    $shim = Join-Path $ClientRoot "dist\toolchain"
    New-Item -ItemType Directory -Force -Path $shim | Out-Null
    Copy-Item (Join-Path $clang "llvm-dlltool.exe") (Join-Path $shim "dlltool.exe") -Force
    $mingw = Join-Path $checkout "tools\rosbe\amd64\bin"
    $env:PATH = "$shim;$mingw;$env:PATH"
    $gcc = Join-Path $mingw "x86_64-w64-mingw32-gcc.exe"
    $env:CC = $gcc
    $env:CC_x86_64_pc_windows_gnu = $gcc
    $rustGcc = Join-Path $env:RUSTUP_HOME "toolchains\1.99.1-x86_64-pc-windows-gnu\lib\rustlib\x86_64-pc-windows-gnu\bin\self-contained\x86_64-w64-mingw32-gcc.exe"
    $env:CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER = $rustGcc
    $env:RUSTFLAGS = "-C link-self-contained=yes"
    & cargo build --release --target x86_64-pc-windows-gnu --manifest-path (Join-Path $ClientRoot "Cargo.toml")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $built = Join-Path $ClientRoot "target\x86_64-pc-windows-gnu\release"
    foreach ($name in @("verosupd-setup.exe", "uninstall.exe", "recover.exe", "verosupdsvc.exe", "verosperfsvc.exe", "verosupd.exe", "veroshook.dll")) {
        Copy-Item (Join-Path $built $name) (Join-Path $dist $name) -Force
    }
    Copy-Item (Join-Path $built "verosupd_cpl.dll") (Join-Path $dist "VerOSUpdate.cpl") -Force
    foreach ($name in @("server.json", "server.json.sig", "ca.crt")) {
        $source = Join-Path $ClientRoot $name
        if (-not (Test-Path $source)) { Write-Error "The updater input $name is missing." }
        Copy-Item $source (Join-Path $dist $name) -Force
    }
    $payload = [System.Security.Cryptography.SHA256]::Create()
    foreach ($name in @("verosupdsvc.exe", "verosperfsvc.exe", "verosupd.exe", "uninstall.exe", "server.json", "server.json.sig", "ca.crt")) {
        $bytes = [IO.File]::ReadAllBytes((Join-Path $dist $name))
        $payload.TransformBlock($bytes, 0, $bytes.Length, $null, 0) | Out-Null
    }
    $payload.TransformFinalBlock(@(), 0, 0) | Out-Null
    $digest = ([BitConverter]::ToString($payload.Hash)).Replace("-", "").ToLower()
    $key = $env:PUBLISH_API_KEY
    if (-not $key) {
        $publish = ""
        $license = ""
        $envFile = Join-Path (Split-Path $ClientRoot -Parent) ".env"
        foreach ($line in Get-Content $envFile) {
            if ($line -like "PUBLISH_API_KEY=*") { $publish = $line.Substring(16).Trim() }
            if ($line -like "LICENSE_API_KEY=*") { $license = $line.Substring(16).Trim() }
        }
        if ($publish) { $key = $publish } else { $key = $license }
    }
    if (-not $key) {
        Write-Warning "The updater payload was built but not uploaded. No publish API key is set."
        return
    }
    $body = @{ channel = $Track; version = 0; sha256 = $digest; enabled = $true } | ConvertTo-Json
    try {
        $response = Invoke-RestMethod -Method Post -Uri "$PublishUrl/v1/updater" -Headers @{ "x-api-key" = $key } -ContentType "application/json" -Body $body
        if ($null -eq $response -or $null -eq $response.version) {
            throw "The updater publish returned no version."
        }
        @{ hash = $hash; version = [int]$response.version } | ConvertTo-Json | Set-Content $stampPath -Encoding ascii
    } catch {
        Write-Warning "The updater payload was built but not uploaded. The boot CD will still include it. $($_.Exception.Message)"
    }
}

function Install-UpdaterClient {
    param([string]$ClientRoot, [string]$Reactos)
    $setup = Join-Path $ClientRoot "dist\verosupd-setup.exe"
    & $setup /image $Reactos
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Export-OsImage {
    param([string]$List, [string]$Image)
    if (-not (Test-Path $List)) { Write-Error "The boot CD file list is missing at $List." }
    if (Test-Path $Image) { Remove-Item $Image -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $Image | Out-Null
    $copied = 0
    foreach ($line in Get-Content $List) {
        if ($line -notlike "VerOS/*=*") { continue }
        $split = $line.IndexOf("=")
        if ($split -lt 7) { continue }
        $rel = $line.Substring(6, $split - 6) -replace "/", "\"
        $src = $line.Substring($split + 1)
        if (-not (Test-Path -LiteralPath $src)) { throw "Image file is missing: $src" }
        $item = Get-Item -LiteralPath $src
        $dest = Join-Path $Image $rel
        if ($item.PSIsContainer) {
            New-Item -ItemType Directory -Force -Path $dest | Out-Null
            continue
        }
        $parent = Split-Path $dest -Parent
        if ($parent -and -not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
        New-Item -ItemType HardLink -Path $dest -Target $src | Out-Null
        $copied++
    }
    if ($copied -lt 100) { throw "The image layout only had $copied files." }
    return $copied
}

function Publish-OsTree {
    param([string]$Checkout, [string]$BuildDir, [string]$ClientRoot, [string]$Track, [string]$PublishUrl)
    $env:UPDATE_PUBLISH_URL = $PublishUrl
    $bin = Join-Path $Checkout "update-server\target\release\veros-publish.exe"
    if (-not (Test-Path $bin)) {
        $bin = Join-Path $Checkout "update-server\target\debug\veros-publish.exe"
    }
    $cache = Join-Path $Checkout "update-server\cache"
    $source = Join-Path $Checkout "source"
    $revision = ""
    if (Get-Command git -ErrorAction SilentlyContinue) {
        $revision = (& git -C $source rev-parse HEAD 2>$null)
        if ($LASTEXITCODE -ne 0) { $revision = "" }
    }
    $env:VEROS_REVISION = "$revision".Trim()
    $image = Join-Path $BuildDir "image"
    $copied = Export-OsImage -List (Join-Path $BuildDir "boot\bootcd.Debug.lst") -Image $image
    Write-Host "Image layout has $copied files"
    $dist = Join-Path $ClientRoot "dist"
    $id = & $bin $image $Track $cache $dist
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    return ($id | Select-Object -Last 1)
}

function Repack-Bootcd {
    param([string]$BuildDir, [string]$Reactos)
    $list = Join-Path $BuildDir "boot\bootcd.Debug.lst"
    if (-not (Test-Path $list)) { Write-Error "The boot CD file list is missing at $list." }
    $kept = Get-Content $list | Where-Object { $_ -notmatch 'VerOS Update|VerOSUpdate\.cpl|veros\.build' }
    $added = @()
    $clientFiles = Get-ChildItem $Reactos -Recurse -File | Where-Object {
        $relative = $_.FullName.Substring($Reactos.Length).TrimStart('\')
        $relative -like "Program Files\VerOS Update\*" -or $relative -eq "system32\VerOSUpdate.cpl" -or $relative -eq "veros.build"
    }
    foreach ($file in $clientFiles) {
        $graft = "VerOS/" + $file.FullName.Substring($Reactos.Length).TrimStart('\').Replace('\', '/')
        $added += "$graft=$($file.FullName)"
    }
    $systemHive = (Join-Path $Reactos "system32\config\SYSTEM").Replace('\', '/')
    $softwareHive = (Join-Path $Reactos "system32\config\SOFTWARE").Replace('\', '/')
    $kept = $kept | ForEach-Object {
        if ($_ -like "VerOS/system32/config/system=*") { "VerOS/system32/config/system=$systemHive" }
        elseif ($_ -like "VerOS/system32/config/software=*") { "VerOS/system32/config/software=$softwareHive" }
        else { $_ }
    }
    @($kept + $added) | Set-Content $list -Encoding ascii
    $mkisofs = Get-ChildItem $BuildDir -Filter mkisofs.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    $isohybrid = Get-ChildItem $BuildDir -Filter isohybrid.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $mkisofs -or -not $isohybrid) { Write-Error "mkisofs or isohybrid was not built." }
    $iso = Join-Path $BuildDir "bootcd.iso"
    $sort = Join-Path $BuildDir "boot\bootfiles.sort"
    & $mkisofs.FullName -quiet -o $iso -iso-level 4 -publisher "ReactOS Project" -preparer "ReactOS Project" -volid ReactOS -volset ReactOS -eltorito-platform x86 -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog -sort $sort -duplicates-once -no-cache-inodes -graft-points -path-list $list
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $mbr = Join-Path $BuildDir "boot\freeldr\bootsect\isombr.bin"
    & $isohybrid.FullName -b $mbr -t 0x96 $iso
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$Checkout = Split-Path $SourceRoot -Parent
$ClientRoot = Join-Path $Checkout "update-server\client"
$Reactos = Join-Path $BuildDir "reactos"
if (-not $RepackOnly) {
    $hiveDir = Join-Path $Reactos "system32\config"
    New-Item -ItemType Directory -Force -Path $hiveDir | Out-Null
    Copy-Item (Join-Path $BuildDir "boot\bootdata\system") (Join-Path $hiveDir "SYSTEM") -Force
    Copy-Item (Join-Path $BuildDir "boot\bootdata\software") (Join-Path $hiveDir "SOFTWARE") -Force
    Sync-UpdaterClient -ClientRoot $ClientRoot -Track $Track -PublishUrl $UpdateServer.PublishUrl
    Install-UpdaterClient -ClientRoot $ClientRoot -Reactos $Reactos
}
Repack-Bootcd -BuildDir $BuildDir -Reactos $Reactos
if ($Publish) {
    $published = Publish-OsTree -Checkout $Checkout -BuildDir $BuildDir -ClientRoot $ClientRoot -Track $Track -PublishUrl $UpdateServer.PublishUrl
    Set-Content -Path (Join-Path $Reactos "veros.build") -Value $published -Encoding ascii
    Repack-Bootcd -BuildDir $BuildDir -Reactos $Reactos
}

$iso = Join-Path $BuildDir "bootcd.iso"
if (-not (Test-Path $iso)) {
    Write-Error "Build finished but $iso is missing."
}
Write-Host "ISO: $iso"
