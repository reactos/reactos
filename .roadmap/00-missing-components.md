# Danh sách các phần còn thiếu trong mã nguồn ReactOS

*Đối chiếu cấu trúc `F:/reactos` với Windows 7. Không phải tất cả đều bắt buộc - có cái Wine/Mesa lấp được.*

---

## A. KERNEL & HAL

### Thiếu hoàn toàn
- **dxgkrnl.sys** — DirectX Graphics Kernel (CRITICAL cho WDDM/Win7)
- **dxgmms1.sys / dxgmms2.sys** — Video Memory Manager + Scheduler
- **fltMgr.sys** — Filter Manager (bắt buộc cho AV/EFS/BitLocker hiện đại)
- **clfs.sys** — Common Log File System (KTM phụ thuộc)
- **tm.sys** — Kernel Transaction Manager (KTM)
- **bam.sys / dam.sys** — Background/Desktop Activity Moderator (Win8+)
- **pshed.dll** — Platform Specific Hardware Error Driver
- **ndis.sys 6.x** — hiện tại là NDIS 5
- **netio.sys** — Network I/O subsystem (đã có 1 phần ở DLL nhưng thiếu driver)
- **fwpkclnt.sys + wfplwfs.sys** — Windows Filtering Platform (rewrite firewall từ Vista)

### Đang stub/yếu
- **HAL** — chỉ `halx86`, `halarm` — thiếu HAL ACPI multiprocessor x64 full, HAL UEFI
- **ntoskrnl/po/** — Power Framework v2 (PoFx) thiếu
- **ntoskrnl/ke/** — ALPC chưa hoàn chỉnh, Worker Factory thiếu
- **ntoskrnl/cc/** — Cache Manager chưa hỗ trợ async paging mới của Vista+

---

## B. DRIVERS

### Storage (`drivers/storage/`) thiếu
- **stornvme.sys** — NVMe (modern SSD bắt buộc)
- **storahci.sys** — AHCI miniport
- **iaStor*** — Intel RST
- **vhdmp.sys** — VHD mounting (Win7 feature)
- **storvsp.sys / storvsc.sys** — Hyper-V storage
- **disk.sys 6.x rewrite**

### Network (`drivers/network/`) thiếu
- **bcrypt.sys** — Crypto kernel
- **mrxsmb20.sys / mrxsmb10.sys** — SMB2/3 client (chỉ có SMB1 ở mup)
- **srv2.sys** — SMB2 server
- **wfp***, **fwpkclnt.sys** — Windows Filtering Platform
- **netvsc.sys** — Hyper-V network
- **NDIS 6.x** stack
- **TCPIP rewrite Vista** (hiện ReactOS dùng LWIP-based, chưa phải Next Generation TCP/IP Stack)

### Multimedia (`drivers/multimedia/`) thiếu
- **portcls.sys 2.x** — Audio Port Class (WASAPI base)
- **drmk.sys** — DRM kernel
- **ks.sys** updated — Kernel Streaming
- **HDAudio class driver** (hdaudbus.sys) hoàn chỉnh
- **usbaudio2.sys**

### Display (`win32ss/drivers/`)
- Chỉ có XDDM-style: `displays`, `font`, `miniport`, `videoprt`
- **Thiếu toàn bộ WDDM stack** (dxgkrnl + UMD/KMD model)
- Không có `drivers/video/` (đã bỏ?) và `drivers/directx/`

### USB
- `drivers/usb/` có nhưng **USB 3.0/xhci** thiếu hoặc rất sơ khai
- **usbhub3.sys** — USB 3 hub

### Bluetooth
- `drivers/bluetooth/` có khung — **bthport.sys**, A2DP/HFP profiles thiếu nặng

### Khác
- **TPM driver** (tpm.sys) — không có → block BitLocker
- **Sensor framework** (SensorsCx) — không có
- **GPIO/SPB framework** — không có

---

## C. FILESYSTEMS

| FS | Trạng thái |
|----|-----------|
| FAT (fastfat, vfatfs) | OK |
| NTFS | đọc OK, ghi đang investigate (WIP) |
| CDFS, UDFS | OK |
| EXT2 (ext2) | có |
| BTRFS | experimental |
| NFS | có client |
| **exFAT** | **THIẾU** (Win7 cần) |
| **ReFS** | **THIẾU** (Win8+) |
| **CSV** (Cluster Shared Volumes) | THIẾU |
| **SMB2/3 client** | **THIẾU** (chỉ SMB1) |
| **WIM mount** | THIẾU (wimmount.sys) |

---

## D. DIRECTX & GRAPHICS

`F:/reactos/dll/directx/` chỉ có: **d3d8, d3d9, ddraw, dsound_new, ksproxy, ksuser, msdvbnp, msvidctl**

### Thiếu hoàn toàn (cốt lõi Win7)
- **dxgi.dll, dxgi1_2.dll** — DirectX Graphics Infrastructure
- **d3d10.dll, d3d10_1.dll, d3d10core.dll**
- **d3d11.dll** — Direct3D 11 (CRITICAL Win7)
- **d2d1.dll** — Direct2D (cần cho Aero & ribbon)
- **dwrite.dll** — DirectWrite text rendering
- **d3dcompiler_*.dll** — HLSL compiler
- **xaudio2_*.dll** — XAudio2 game audio
- **xinput1_4.dll** (đã có 1_0, 1_1, 1_2, 1_3)
- **dxva2.dll** — Video acceleration
- **mf*.dll** (Media Foundation) — xem mục E

### Có thể tận dụng
- Wine d3d11/dxgi (đã upstream)
- Mesa Gallium nine

---

## E. MEDIA FOUNDATION (toàn bộ thiếu)

ReactOS hiện chỉ có DirectShow legacy (quartz, etc). Win7 standard codec stack dựa trên Media Foundation:
- **mf.dll, mfplat.dll, mfreadwrite.dll, mfplay.dll**
- **mfcore.dll, mfsrcsnk.dll**
- **evr.dll** — Enhanced Video Renderer
- **wmadmod.dll, wmvdecod.dll** — codec MFTs
- **mediaengine.dll** — HTML5 video stack

---

## F. AUDIO MODERN STACK

Hiện có: `winmm`, `mmdrv`, `mmdevapi` (skeleton), `wdmaud.drv`, `dsound_new`

### Thiếu
- **audiodg.exe** — Audio Device Graph (isolation process)
- **audiosrv.dll** — đã có service nhưng chưa Vista-grade
- **audioses.dll** — WASAPI session
- **audioeng.dll** — audio engine
- **MMDevAPI** full COM interfaces (IMMDeviceEnumerator, IAudioClient, IAudioRenderClient)
- **AVRT.dll** đã có nhưng cần verify Vista compat
- **Sound mixer per-app** (Win7 feature)

---

## G. SHELL / EXPLORER WIN7-SPECIFIC

`base/shell/explorer` là explorer của ReactOS — chỉ có XP-style.

### Thiếu
- **Superbar / Taskbar Win7** (large icons + jump lists + pinning + previews)
- **ITaskbarList3/4** trong shell32 (jump lists API)
- **Aero Peek, Aero Snap, Aero Shake**
- **Libraries** (Documents/Music/Pictures/Videos = IShellLibrary)
- **HomeGroup**
- **Start Menu Win7** (search bar, jump lists)
- **Notification Area redesign** + customization dialog
- **Ribbon UI framework** (uiribbon.dll) — dùng cho WordPad, MSPaint, Explorer Win8
- **Action Center** thay Security Center
- **Aero Theme engine** + Glass blur
- **Window Snap/Shake** chrome
- **Search box trong Explorer** (Windows Search 4+ + ESE indexer)

---

## H. SECURITY & IDENTITY

### Thiếu
- **fvevol.sys + fveapi.dll + manage-bde** — BitLocker
- **vaultsvc.dll + vaultcli.dll** — Credential Manager (Win7)
- **appid.dll + appidsvc** — AppLocker
- **slc.dll + slsvc.exe + sppsvc.exe** — Software Licensing
- **ngc.dll** — không cần cho Win7
- **TPM Base Services** (tbssvc.dll)
- **EFS rewrite Vista** (đã có 1 phần)
- **UIPI** — User Interface Privilege Isolation
- **Mandatory Integrity Control (MIC)** — cần rewrite `ntoskrnl/se/`
- **DPAPI-NG** — không cần Win7
- **Smart Card auto-enroll**, **Certificate Lifecycle Manager**

---

## I. NETWORKING WIN7

### Thiếu
- **WFP user-mode** (fwpuclnt.dll) đầy đủ
- **HomeGroup** (provsvc.dll, hgcpl.dll)
- **DirectAccess** (iphlpapi extension, IKEEXT)
- **BranchCache** (peerdistsvc.dll, peerdist.sys)
- **WLAN AutoConfig** (wlansvc 6.x) — có service skeleton
- **Mobile Broadband** (WWAN framework)
- **Bluetooth full profile stack**
- **NPS** (Network Policy Server)
- **DNS Client cache rewrite Vista**

---

## J. APPLICATIONS WIN7 STOCK CÒN THIẾU

| App | Trạng thái |
|-----|-----------|
| Calculator (calc) | có cũ |
| Notepad | có |
| WordPad | có (cần Ribbon Win7) |
| MSPaint | có (cần Ribbon Win7) |
| Windows Photo Viewer | có shimgvw |
| **Snipping Tool** | THIẾU |
| **Sticky Notes** | THIẾU |
| **Windows Media Player 12** | THIẾU (cần MF) |
| **Windows Media Center** | THIẾU |
| **Windows DVD Maker** | THIẾU |
| **Problem Steps Recorder (psr)** | THIẾU |
| **PowerShell + PowerShell ISE** | **THIẾU** (lớn) |
| **Performance Monitor** | THIẾU |
| **Resource Monitor** | THIẾU |
| **System Configuration (msconfig Win7)** | có cũ |
| **Action Center** | THIẾU |
| **Windows Update Standalone Installer (wusa)** | có khung |
| **Backup and Restore** | THIẾU |
| **Windows Defender (MSE)** | THIẾU |
| **XPS Viewer** | THIẾU |
| **Tablet PC / Math Input Panel** | THIẾU |
| **On-Screen Keyboard hiện đại** | có cũ (osk) |

---

## K. SERVICES WIN7 CHƯA CÓ

`base/services/` hiện có ~22 service. Win7 ship hơn 130 services. Thiếu:

- **AudioEndpointBuilder** (audioendpointbuilder.dll)
- **AudioSrv 6.x** (rewrite)
- **Themes (uxsms / Themes)** — service quản theme
- **BFE** (Base Filtering Engine) — WFP service
- **DPS** (Diagnostic Policy Service)
- **EventSystem 6.x** (đã có 1 phần)
- **Group Policy Client (gpsvc)**
- **MMCSS** (Multimedia Class Scheduler)
- **Network Location Awareness (NLA 2.0)**
- **PlugPlay 6.x rewrite**
- **Power Manager (Power)**
- **Print Spooler Win7**
- **SearchIndexer** (Windows Search)
- **Sens** (System Event Notification Service Vista)
- **Superfetch (SysMain)**
- **TaskScheduler 2.0** (chỉ có 1.0)
- **Tcpip Stack Service**
- **Themes**
- **UmRdpService** (Remote Desktop)
- **WMP Network Sharing**
- **Workstation 6.x**
- **WSearch**
- **WinHTTP Auto Proxy**
- **Background Intelligent Transfer (BITS)** — qmgr có khung
- **WPDBusEnum** (Portable Devices)
- **Bluetooth Support Service**
- **Diagnostic Service Host**

---

## L. WMI v2 & EVENT TRACING

- **wbem/WMI** có khung
- **wevtsvc.dll** — Event Log Vista (rewrite, channel-based) THIẾU
- **wevtapi.dll** — Event Log API mới THIẾU
- **ETW** (Event Tracing for Windows) Vista improvements thiếu
- **WMIv2 namespace + classes** thiếu nhiều

---

## M. BOOT / FIRMWARE

`F:/reactos/boot/` có **freeldr**, **bcd folder**, **armllb** nhưng:

### Thiếu
- **bootmgr / bootmgfw.efi** (UEFI boot manager) — freeldr chỉ thay NTLDR
- **winload.exe / winload.efi** — Win7 chia tách kernel loader khỏi boot manager
- **winresume.exe / winresume.efi** — hibernation
- **measured boot / secure boot integration**
- **BCD full implementation** (chỉ có folder khung)

---

## N. API SET FRAMEWORK (Win7 mới)

Win7 giới thiệu **api-ms-win-*.dll** (API Set Schema). ReactOS chưa có cơ chế:
- **apisetschema.dll** — schema mapping
- **ApiSet redirection** trong loader
- **api-ms-win-core-*.dll** — hàng trăm API set DLL

→ Thiếu hệ thống này khiến nhiều ứng dụng Win7+ không load được.

---

## O. PRINTING WIN7

`win32ss/printing/` có. Thiếu:
- **XPSDrv** — XPS print driver model (Win7 default)
- **localspl.dll Win7** rewrite
- **prntvpt.dll** — Print Ticket
- **mxdwdrv.dll** — Microsoft XPS Document Writer

---

## P. ADDITIONAL DLL CHECKLIST (Win7 ship list)

Đối chiếu `dll/win32` (271 DLLs hiện có) với System32 Win7 (~3500 files):

### Top DLL/EXE thiếu đáng chú ý
```
aclayers.dll        Shim engine v2
acspecfc.dll        App Compat
audioeng.dll        Audio engine
audioses.dll        WASAPI
authui.dll          UAC credential UI
bcryptprimitives.dll (có nhưng cần verify Vista)
cabview.dll         (có nhưng shell ext)
certenroll.dll      Certificate enrollment
comuid.dll          COM+ UI
dbghelp.dll Win7    (có cũ)
DcompUiHelper.dll
diagperf.dll
diagtrack.dll       Telemetry (không nên port)
dnsrslvr.dll        (có service)
EhStorAPI.dll       Enhanced Storage
fhsvc.dll           File History (Win8)
gdi32.dll Win7      (có nhưng XP-era)
GroupPolicy.dll
imm32.dll Win7      (có cũ)
ksuser.dll          (có)
LegacyNetUX.dll
LocationApi.dll
LogonController.dll
mfc***.dll          MFC modern (có 42 cũ)
mscms.dll Win7      Color Mgmt v2
mscoreei.dll        .NET shim
msftedit.dll Win7   (có cũ)
NetworkExplorer.dll
ngc.dll             (Win10 only)
PortableDeviceApi.dll  WPD
PrintConfig.dll
PropsysWin7.dll
provsvc.dll         HomeGroup
samcli.dll
SearchFolder.dll
SecurityHealthAgent (Win8+)
SensorsApi.dll
shacct.dll
SharedRealitySvc
Smb20.dll
SrpUxNativeSnapIn   AppLocker UI
sti.dll Win7        (có cũ)
taskschd.dll        TaskScheduler 2
tbssvc.dll          TPM Base
themecpl.dll
TpmCoreProvisioning
uIRibbon.dll        Ribbon
ureg.dll
UserAccountControlSettings.dll
VaultCli.dll        Credential Vault
WFAPIGP.dll         WFP
WinBio*.dll         Biometric
WindowsCodecsRaw    (có codecs)
WMNetMgr.dll        WMP network
WSearch (search)
wsmsvc.dll          WinRM
WUDFPlatform.dll    UMDF
```

---

## TỔNG KẾT THEO MỨC ĐỘ KHẨN

### CRITICAL (chặn việc chạy app Win7)
1. **API Set framework** (apisetschema)
2. **dxgkrnl + WDDM stack**
3. **d3d10/d3d11 + dxgi + d2d + dwrite**
4. **Media Foundation core**
5. **WASAPI** (audiodg, audioses, audioeng)
6. **fltMgr.sys** + filter framework
7. **PowerShell host** + .NET 4 phụ thuộc

### HIGH (UX Win7)
8. Ribbon framework + uiribbon.dll
9. Superbar / ITaskbarList3/4 / Jump Lists / Aero
10. DWM compositor đầy đủ + Glass
11. Libraries + HomeGroup
12. Action Center
13. Themes service + Aero theme

### MEDIUM (modern hardware)
14. NVMe + AHCI + USB3 (xhci)
15. NDIS 6.x + WFP + TCPIP Vista
16. UEFI boot + bootmgr + winload
17. exFAT + SMB2/3 client
18. PortCls 2.x + HDAudio Win7

### LOW (apps & polish)
19. Stock apps thiếu (Snipping/Sticky Notes/PSR/WMP12)
20. Search Indexer + WSearch
21. BitLocker (cần TPM stack)
22. Backup & Restore

---

## NHỮNG GÌ ĐÃ CÓ MỪNG

- **Cấu trúc directory đầy đủ**: ntoskrnl, win32ss, hal, dll/win32, drivers, base
- **271 DLLs trong dll/win32** — coverage tốt cho XP-era
- **Vista compat DLLs** đã bắt đầu: `*_vista` (advapi32_vista, kernel32_vista, ntdll_vista, user32_vista)
- **FreeLoader + BCD folder** — nền tảng cho Win7 boot
- **win32ss** clean separation (gdi, user, drivers, printing, reactx)
- **Wine sync workflow** — nhiều DLL theo upstream Wine 10
- **Filesystem coverage** rộng (FAT, NTFS read, EXT2, BTRFS, NFS, UDFS, CDFS)
