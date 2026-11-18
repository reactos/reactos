# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/windows-trace/Install-Windows.ps1
# PURPOSE:     Install the Windows 10 22H2 trace guest
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$Iso = "",
    [string]$OfficeInstaller = "",
    [string]$BrowserInstaller = "",
    [string]$VirtioIso = "",
    [string]$Qemu = "",
    [int]$ImageIndex = 0,
    [int]$TimeoutSec = 9000,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
. (Join-Path $SourceRoot "tools\pipeline\Resolve-VerosPaths.ps1")
$BuildenvRoot = Resolve-VerosBuildenvRoot -SourceRoot $SourceRoot
if (-not $Iso) { $Iso = Join-Path $BuildenvRoot "iso\Windows_10_22H2_English_x64.iso" }
if (-not $VirtioIso) { $VirtioIso = Join-Path $BuildenvRoot "downloads\virtio-win-0.1.271.iso" }
if (-not $Qemu) { $Qemu = Resolve-VerosQemuExe -SourceRoot $SourceRoot }
$out = Join-Path $SourceRoot "output\windows"
$payload = Join-Path $out "payload"
$disk = Join-Path $out "win10.qcow2"
$marker = Join-Path $out "install-ok.txt"
New-Item -ItemType Directory -Force -Path $payload | Out-Null

if ((Test-Path $marker) -and -not $Force) {
    Write-Host "Install already recorded at $marker"
    exit 0
}
foreach ($need in @($Iso, $OfficeInstaller, $BrowserInstaller, $VirtioIso, $Qemu)) {
    if (-not $need -or -not (Test-Path $need)) { Write-Error "Missing $need" }
}

function Find-ImageIndex {
    param([string]$Path, [int]$Preferred)
    if ($Preferred -gt 0) { return $Preferred }
    $mounted = Mount-DiskImage -ImagePath $Path -PassThru
    try {
        $letter = ($mounted | Get-Volume).DriveLetter
        $wim = "${letter}:\sources\install.wim"
        if (-not (Test-Path $wim)) { $wim = "${letter}:\sources\install.esd" }
        try {
            $images = @(Get-WindowsImage -ImagePath $wim)
        }
        catch {
            Write-Host "Image list needs elevation. Using Windows 10 Pro index 6."
            return 6
        }
        $pro = $images | Where-Object { $_.ImageName -eq "Windows 10 Pro" } | Select-Object -First 1
        if ($pro) { return [int]$pro.ImageIndex }
        return [int]$images[0].ImageIndex
    }
    finally {
        Dismount-DiskImage -ImagePath $Path -ErrorAction SilentlyContinue | Out-Null
    }
}

$index = Find-ImageIndex -Path $Iso -Preferred $ImageIndex
Write-Host "Installing image index $index"

$answer = @"
<?xml version="1.0" encoding="utf-8"?>
<unattend xmlns="urn:schemas-microsoft-com:unattend" xmlns:wcm="http://schemas.microsoft.com/WMIConfig/2002/State">
  <settings pass="windowsPE">
    <component name="Microsoft-Windows-International-Core-WinPE" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
      <SetupUILanguage><UILanguage>en-US</UILanguage></SetupUILanguage>
      <InputLocale>en-US</InputLocale>
      <SystemLocale>en-US</SystemLocale>
      <UILanguage>en-US</UILanguage>
      <UserLocale>en-US</UserLocale>
    </component>
    <component name="Microsoft-Windows-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
      <DiskConfiguration>
        <Disk wcm:action="add">
          <DiskID>0</DiskID>
          <WillWipeDisk>true</WillWipeDisk>
          <CreatePartitions>
            <CreatePartition wcm:action="add">
              <Order>1</Order>
              <Type>Primary</Type>
              <Size>100</Size>
            </CreatePartition>
            <CreatePartition wcm:action="add">
              <Order>2</Order>
              <Type>Primary</Type>
              <Extend>true</Extend>
            </CreatePartition>
          </CreatePartitions>
          <ModifyPartitions>
            <ModifyPartition wcm:action="add">
              <Order>1</Order>
              <PartitionID>1</PartitionID>
              <Label>System</Label>
              <Format>NTFS</Format>
              <Active>true</Active>
            </ModifyPartition>
            <ModifyPartition wcm:action="add">
              <Order>2</Order>
              <PartitionID>2</PartitionID>
              <Label>Windows</Label>
              <Letter>C</Letter>
              <Format>NTFS</Format>
            </ModifyPartition>
          </ModifyPartitions>
        </Disk>
      </DiskConfiguration>
      <ImageInstall>
        <OSImage>
          <InstallFrom>
            <MetaData wcm:action="add">
              <Key>/IMAGE/INDEX</Key>
              <Value>$index</Value>
            </MetaData>
          </InstallFrom>
          <InstallTo>
            <DiskID>0</DiskID>
            <PartitionID>2</PartitionID>
          </InstallTo>
        </OSImage>
      </ImageInstall>
      <UserData>
        <AcceptEula>true</AcceptEula>
        <FullName>Trace</FullName>
        <Organization>Trace</Organization>
        <ProductKey>
          <Key>W269N-WFGWX-YVC9B-4J6C9-T83GX</Key>
          <WillShowUI>Never</WillShowUI>
        </ProductKey>
      </UserData>
    </component>
  </settings>
  <settings pass="specialize">
    <component name="Microsoft-Windows-Shell-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
      <ComputerName>wintrace</ComputerName>
      <TimeZone>UTC</TimeZone>
    </component>
  </settings>
  <settings pass="oobeSystem">
    <component name="Microsoft-Windows-Shell-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
      <OOBE>
        <HideEULAPage>true</HideEULAPage>
        <HideOEMRegistrationScreen>true</HideOEMRegistrationScreen>
        <HideOnlineAccountScreens>true</HideOnlineAccountScreens>
        <HideWirelessSetupInOOBE>true</HideWirelessSetupInOOBE>
        <ProtectYourPC>3</ProtectYourPC>
      </OOBE>
      <UserAccounts>
        <LocalAccounts>
          <LocalAccount wcm:action="add">
            <Name>trace</Name>
            <Group>Administrators</Group>
            <Password>
              <Value>TraceGuest1</Value>
              <PlainText>true</PlainText>
            </Password>
          </LocalAccount>
        </LocalAccounts>
      </UserAccounts>
      <AutoLogon>
        <Enabled>true</Enabled>
        <Username>trace</Username>
        <Password>
          <Value>TraceGuest1</Value>
          <PlainText>true</PlainText>
        </Password>
      </AutoLogon>
      <FirstLogonCommands>
        <SynchronousCommand wcm:action="add">
          <Order>1</Order>
          <CommandLine>cmd.exe /c for %D in (D E F G H I J) do if exist %D:\install-apps.cmd call %D:\install-apps.cmd</CommandLine>
        </SynchronousCommand>
      </FirstLogonCommands>
    </component>
  </settings>
</unattend>
"@
$utf8 = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText((Join-Path $payload "autounattend.xml"), $answer, $utf8)

@'
@echo off
set SRC=
for %%D in (D E F G H I J) do if exist %%D:\install-apps.cmd set SRC=%%D:
if "%SRC%"=="" exit /b 1
pnputil /add-driver %SRC%\vioser\vioser.inf /install
msiexec /i %SRC%\qemu-ga-x86_64.msi /qn /norestart
msiexec /i %SRC%\LibreOffice.msi /qn /norestart ALLUSERS=1
if exist %SRC%\FirefoxInstaller.exe start /wait "" %SRC%\FirefoxInstaller.exe /S
mkdir "%ProgramData%\Microsoft\Windows\Start Menu\Programs\StartUp" 2>nul
copy /Y %SRC%\trace-workload.cmd "%ProgramData%\Microsoft\Windows\Start Menu\Programs\StartUp\trace-workload.cmd"
echo ready> C:\trace-ready.txt
shutdown /s /t 15
'@ | Set-Content -Path (Join-Path $payload "install-apps.cmd") -Encoding ASCII

@'
@echo off
set SRC=
for %%D in (D E F G H I J) do if exist %%D:\workload.txt set SRC=%%D:
if "%SRC%"=="" exit /b 0
set /p W=<"%SRC%\workload.txt"
if /I "%W%"=="notepad" start "" notepad.exe
if /I "%W%"=="explorer" start "" explorer.exe C:\
if /I "%W%"=="office" start "" "C:\Program Files\LibreOffice\program\soffice.exe" --writer
if /I "%W%"=="browser" start "" "C:\Program Files\Mozilla Firefox\firefox.exe" about:blank
'@ | Set-Content -Path (Join-Path $payload "trace-workload.cmd") -Encoding ASCII

$seven = Join-Path $BuildenvRoot "tools\7zip\7z.exe"
if (-not (Test-Path $seven)) { $seven = Join-Path $env:ProgramFiles "7-Zip\7z.exe" }
if (-not (Test-Path $seven)) { $seven = "7z" }
& $seven x $VirtioIso "-o$out\virtio" "guest-agent\qemu-ga-x86_64.msi" "vioserial\w10\amd64\*" -y | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $payload "vioser") | Out-Null
Copy-Item (Join-Path $out "virtio\guest-agent\qemu-ga-x86_64.msi") (Join-Path $payload "qemu-ga-x86_64.msi") -Force
Copy-Item (Join-Path $out "virtio\vioserial\w10\amd64\*") (Join-Path $payload "vioser") -Force
Copy-Item $OfficeInstaller (Join-Path $payload "LibreOffice.msi") -Force
Copy-Item $BrowserInstaller (Join-Path $payload "FirefoxInstaller.exe") -Force

$mkisofs = Join-Path $SourceRoot "build\host-tools\bin\mkisofs.exe"
if (-not (Test-Path $mkisofs)) { Write-Error "mkisofs was not found at $mkisofs" }
$answerIso = Join-Path $out "answer.iso"
& $mkisofs -o $answerIso -J -r -V TRACE $payload
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$img = Join-Path (Split-Path $Qemu) "qemu-img.exe"
if ($Force -or -not (Test-Path $disk)) {
    if (Test-Path $disk) { Remove-Item $disk -Force }
    & $img create -f qcow2 $disk 64G
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$serial = Join-Path $out "install-serial.log"
$err = Join-Path $out "install.err"
$arg = "-accel tcg -machine pc -cpu qemu64 -m 8192 -smp 4 -display none -vga std " +
    "-boot order=c,once=d -monitor tcp:127.0.0.1:4445,server,nowait " +
    "-drive `"file=$disk,format=qcow2,if=ide,index=0,media=disk`" " +
    "-drive `"file=$Iso,format=raw,if=ide,index=1,media=cdrom`" " +
    "-drive `"file=$answerIso,format=raw,if=ide,index=2,media=cdrom`" " +
    "-netdev user,id=n0 -device e1000,netdev=n0 " +
    "-chardev socket,id=qga,host=127.0.0.1,port=4711,server=on,wait=off " +
    "-device virtio-serial-pci -device virtserialport,chardev=qga,name=org.qemu.guest_agent.0 " +
    "-serial `"file:$serial`""
Write-Host "QEMU install started"
$proc = Start-Process -FilePath $Qemu -ArgumentList $arg -PassThru -RedirectStandardError $err -WindowStyle Hidden
$pressUntil = (Get-Date).AddSeconds(20)
while (-not $proc.HasExited -and (Get-Date) -lt $pressUntil) {
    try {
        $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", 4445)
        $writer = New-Object System.IO.StreamWriter($client.GetStream())
        $writer.NewLine = "`n"
        $writer.AutoFlush = $true
        Start-Sleep -Milliseconds 200
        $writer.WriteLine("sendkey spc")
        $writer.WriteLine("sendkey ret")
        $writer.Close()
        $client.Close()
    } catch {}
    Start-Sleep -Seconds 2
}
if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    Write-Error "Windows install timed out after $TimeoutSec seconds. See $err"
}
$length = (Get-Item $disk).Length
if ($length -lt 2000000000) {
    Write-Error "Windows did not write the disk (size $length). See $err and $serial"
}
Set-Content -Path $marker -Value "index=$index qemuExit=$($proc.ExitCode) bytes=$length"
Write-Host "Install finished: $marker"
