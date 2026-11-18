# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/windows-trace/Collect-Traces.ps1
# PURPOSE:     Record numeric syscall lines from the Windows guest
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$Disk = "",
    [string]$Qemu = "",
    [string]$SyscallMap = "",
    [int]$TimeoutSec = 5400,
    [int]$QgaPort = 4711,
    [int]$MonitorPort = 4445
)

$ErrorActionPreference = "Stop"
. (Join-Path $SourceRoot "tools\pipeline\Resolve-VerosPaths.ps1")
$BuildenvRoot = Resolve-VerosBuildenvRoot -SourceRoot $SourceRoot
if (-not $Disk) {
    $Disk = Join-Path $BuildenvRoot "virtual-machines\win10-22h2\disk.qcow2"
}
if (-not $Qemu) {
    $Qemu = Resolve-VerosQemuExe -SourceRoot $SourceRoot
}
if (-not $Qemu) {
    Write-Error "QEMU was not found under the buildenv tools\qemu folder."
}
$QemuShare = Join-Path (Split-Path $Qemu) "share"
$out = Join-Path $SourceRoot "output\windows"
$trace = Join-Path $SourceRoot "output\windows-trace"
$dll = Join-Path $SourceRoot "tools\qemu-harness\abi.dll"
$status = Join-Path $PSScriptRoot "ntstatus.txt"
$mkisofs = Join-Path $SourceRoot "build\host-tools\bin\mkisofs.exe"
if (-not (Test-Path $Disk)) { Write-Error "No installed disk at $Disk" }
if (-not (Test-Path $dll)) { Write-Error "abi.dll is missing. Run tools\qemu-harness\Test-Plugin.ps1 first." }
New-Item -ItemType Directory -Force -Path $trace | Out-Null

function New-WorkloadIso {
    param([string]$Name)
    $dir = Join-Path $out "workload-$Name"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Set-Content -Path (Join-Path $dir "workload.txt") -Value $Name -Encoding ASCII
    $iso = Join-Path $out "workload-$Name.iso"
    & $mkisofs -o $iso -J -r -V WORK $dir | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "mkisofs failed for $Name" }
    return $iso
}

function Send-Monitor {
    param([string]$Command)
    $mon = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $MonitorPort)
    $writer = New-Object System.IO.StreamWriter($mon.GetStream())
    $writer.NewLine = "`n"
    $writer.AutoFlush = $true
    Start-Sleep -Milliseconds 80
    $writer.WriteLine($Command)
    Start-Sleep -Milliseconds 40
    $writer.Close()
    $mon.Close()
}

function Send-GuestText {
    param([string]$Text)
    foreach ($ch in $Text.ToCharArray()) {
        $key = switch ($ch) {
            ' ' { 'spc' }
            '\' { 'backslash' }
            ':' { 'shift-semicolon' }
            '.' { 'dot' }
            '"' { 'shift-apostrophe' }
            '-' { 'minus' }
            default {
                if ([string]$ch -cmatch '[A-Z]') { "shift-" + ([string]$ch).ToLower() }
                else { ([string]$ch).ToLower() }
            }
        }
        Send-Monitor "sendkey $key"
    }
}

function Send-Qga {
    param($Stream, [string]$Json)
    $bytes = [Text.Encoding]::UTF8.GetBytes($Json + "`n")
    $Stream.Write($bytes, 0, $bytes.Length)
    $Stream.Flush()
}

function Read-Qga {
    param($Stream)
    $buffer = New-Object byte[] 4096
    $text = ""
    $until = (Get-Date).AddSeconds(8)
    while ((Get-Date) -lt $until) {
        if ($Stream.DataAvailable) {
            $n = $Stream.Read($buffer, 0, $buffer.Length)
            if ($n -gt 0) { $text += [Text.Encoding]::UTF8.GetString($buffer, 0, $n) }
            if ($text.Contains("return")) { break }
        }
        else { Start-Sleep -Milliseconds 200 }
    }
    return $text
}

$apps = [ordered]@{
    notepad  = "C:\Windows\System32\notepad.exe"
    explorer = "C:\Windows\explorer.exe"
    office   = "C:\Program Files\LibreOffice\program\soffice.exe"
    browser  = "C:\Program Files\Mozilla Firefox\firefox.exe"
}
$search = @{
    notepad  = "notepad"
    explorer = "explorer"
    office   = "writer"
    browser  = "firefox"
}
$extra = @{
    notepad  = @()
    explorer = @("C:\")
    office   = @("--writer")
    browser  = @("about:blank")
}

foreach ($name in $apps.Keys) {
    $log = Join-Path $trace "$name.jsonl"
    $iso = New-WorkloadIso $name
    $dllFwd = ($dll -replace '\\', '/')
    $logFwd = ($log -replace '\\', '/')
    $statusFwd = ($status -replace '\\', '/')
    $mark = Join-Path $trace "$name.workload"
    if (Test-Path $mark) { Remove-Item $mark -Force }
    $markFwd = $mark -replace '\\', '/'
    $plugin = "file=$dllFwd,log=$logFwd,mode=windows,status=$statusFwd,mark=$markFwd"
    if ($SyscallMap -and (Test-Path $SyscallMap)) {
        $plugin += ",syscalls=" + ($SyscallMap -replace '\\', '/')
    }
    $codeFd = Join-Path $QemuShare "edk2-x86_64-code.fd"
    $varsFd = Join-Path $out "ovmf-vars.fd"
    Copy-Item (Join-Path $QemuShare "edk2-i386-vars.fd") $varsFd -Force
    $exe = $apps[$name]
    if ($exe -match ' ') { $exe = '"' + $exe + '"' }
    $launch = $exe
    foreach ($item in $extra[$name]) { $launch += " " + $item }
    $arg = "-accel tcg -snapshot -machine q35 -cpu qemu64,hv_relaxed,hv_spinlocks=0x1fff,hv_vapic,hv_time -m 8192 -smp 4 -display none -vga qxl " +
        "-device qemu-xhci -device usb-tablet " +
        "-monitor tcp:127.0.0.1:${MonitorPort},server,nowait " +
        "-drive if=pflash,format=raw,readonly=on,file=`"$codeFd`" " +
        "-drive if=pflash,format=raw,file=`"$varsFd`" " +
        "-drive `"file=$Disk,format=qcow2,if=none,id=hd`" -device virtio-blk-pci,drive=hd,bootindex=1 " +
        "-drive `"file=$iso,format=raw,if=none,id=cd`" -device ich9-ahci,id=ahci -device ide-cd,bus=ahci.0,drive=cd,bootindex=2 " +
        "-netdev user,id=n0 -device virtio-net-pci,netdev=n0 " +
        "-chardev socket,id=qga,host=127.0.0.1,port=$QgaPort,server=on,wait=off " +
        "-device virtio-serial-pci -device virtserialport,chardev=qga,name=org.qemu.guest_agent.0 " +
        "-plugin `"$plugin`""
    Write-Host "Tracing $name"
    $err = Join-Path $trace "$name.err"
    $proc = Start-Process -FilePath $Qemu -ArgumentList $arg -PassThru -RedirectStandardError $err -WindowStyle Hidden
    $client = $null
    $deadline = (Get-Date).AddSeconds(240)
    $started = $false
    while ((Get-Date) -lt $deadline -and -not $proc.HasExited) {
        try {
            $client = New-Object System.Net.Sockets.TcpClient
            $client.Connect("127.0.0.1", $QgaPort)
            $stream = $client.GetStream()
            Send-Qga $stream '{"execute":"guest-ping"}'
            $pong = Read-Qga $stream
            if ($pong -match "return") {
                $argList = @()
                foreach ($item in $extra[$name]) { $argList += $item }
                $body = @{ execute = "guest-exec"; arguments = @{ path = $apps[$name]; arg = $argList } }
                Send-Qga $stream (($body | ConvertTo-Json -Compress -Depth 5))
                Read-Qga $stream | Out-Null
                Set-Content -Path $mark -Value $name -Encoding ASCII
                $started = $true
                Set-Content -Path (Join-Path $trace "$name.started") -Value "guest-exec" -Encoding ASCII
                Start-Sleep -Seconds 180
                Send-Qga $stream '{"execute":"guest-shutdown"}'
                break
            }
        }
        catch { $client = $null }
        Start-Sleep -Seconds 10
    }
    if (-not $started -and -not $proc.HasExited) {
        Set-Content -Path $mark -Value $name -Encoding ASCII
        Send-Monitor "sendkey ctrl-esc"
        Start-Sleep -Seconds 2
        Send-GuestText $search[$name]
        Send-Monitor "sendkey ret"
        $started = $true
        Set-Content -Path (Join-Path $trace "$name.started") -Value "keyboard" -Encoding ASCII
        Start-Sleep -Seconds 180
        try { Send-Monitor "quit" } catch {}
    }
    if (-not $proc.WaitForExit(180000)) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
    if (-not $started) {
        Set-Content -Path (Join-Path $trace "$name.started") -Value "startup-script" -Encoding ASCII
    }
    Write-Host "Wrote $log"
}
