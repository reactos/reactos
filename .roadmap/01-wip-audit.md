# R1 - Audit WIP/TODO ReactOS

> Phạm vi: kiểm kê TODO/FIXME/HACK/STUB/UNIMPLEMENTED trên cây nguồn F:/reactos để định hướng lộ trình nâng cấp từ NT 5.2 (Server 2003) sang Vista/Windows 7.
> Ngày: 2026-05-29. Branch: `main` (clone từ itcthienkhiem/reactos).

## Tóm tắt

Phương pháp: dùng ripgrep (qua công cụ Grep) đếm các từ khóa `TODO|FIXME|HACK|STUB|UNIMPLEMENTED` (không phân biệt hoa thường ở từ khóa C, nhưng giữ chữ in vì code base dùng macro `UNIMPLEMENTED`/`FIXME`).

Số lượng phân theo nhóm module (tổng số occurrence, số file có khớp):

| Module gốc | Occurrences | Files |
|---|---:|---:|
| `dll/win32/*` (tất cả Win32 user-mode DLL) | ~16 000+ (ước tính, head_limit 400 đã chạm 4143) | 400+ |
| `win32ss/` (gdi32/user32/ntuser/winsrv/printing) | 1 808 | 315 |
| `drivers/` (bus, net, usb, fs, audio, hid, sac…) | 2 600+ (ước tính, head_limit 300 đạt 1352, cây con `bus`, `usb`, `network`, `filesystems` chưa được liệt hết) | 300+ |
| `ntoskrnl/` | 1 026 | 209 |
| `boot/` (freeldr, bootdata) | 272 | 87 |
| `hal/` (halx86, halarm) | 168 | 48 |
| `base/services/` | 287 | 36 |
| `base/system/` (smss, winlogon, lsass, services, diskpart, ...) | 135 | 28 |
| `base/shell/` (explorer, cmd, progman) | 138 | 41 |
| `sdk/` (atl, fast486, widl, strmbase, evtlib, …) | 316+ | 100+ |
| `dll/ntdll/` | 82 | 12 |

Tổng quan: **>23 000 marker** TODO/FIXME/HACK/STUB/UNIMPLEMENTED đang trải rộng trên >1 500 file. Phần lớn ở user-mode DLL nhập từ Wine (oleaut32, combase, kernelbase, ieframe, bcrypt, wmvcore, dbgeng…). Khoảng 1 000 marker tập trung trong nhân ntoskrnl, đáng chú ý là ARM3, PnP manager, security, fstub và ARM port.

Lưu ý git log: clone hiện tại chỉ có 3 entry trong `.git/logs/HEAD` (fast-forward + 1 "first commit"), nghĩa là không có lịch sử commit WIP riêng của fork — phần "Commit WIP đang dở" bên dưới được rút từ thông điệp `// WIP`/`HACK` trong mã, không phải từ commit log.

## Top 25 hotspot file theo số TODO

| # | File | Count | Mô tả vấn đề chính |
|---:|---|---:|---|
| 1 | `dll/win32/dbgeng/dbgeng.c` | 358 | Toàn bộ engine debugger (IDebugClient, IDebugControl…) gần như stub trả `E_NOTIMPL`. Khối DLL cốt lõi cho WinDbg/CDB/Win7. |
| 2 | `dll/win32/wmvcore/reader.c` | 203 | Windows Media reader: tất cả method stub. Không phải critical Win7 base nhưng cần cho media. |
| 3 | `dll/win32/oleaut32/typelib.c` | 108 | TypeLib loader Wine – nhiều FIXME về kích thước/định dạng. Cần cho COM Vista+. |
| 4 | `dll/win32/dnsapi/stubs.c` | 105 | Loạt API DNS Vista+ chưa cài (DnsQueryEx, DnsServiceXxx…). |
| 5 | `ntoskrnl/ex/sysinfo.c` | 103 | `NtQuerySystemInformation` thiếu nhánh + bộ đếm hiệu năng đặt cứng = 0 (PageFaultCount, CopyOnWriteCount…). Cảnh quan giám sát Win7. |
| 6 | `win32ss/gdi/eng/stubs.c` | 89 | Eng/DDI stub (Engine drawing). Ảnh hưởng driver display. |
| 7 | `dll/win32/oleaut32/usrmarshal.c` | 78 | Marshal stub – COM proxy. |
| 8 | `dll/win32/iehtmlwnd.c` (ieframe) | 74 | IE host window, không critical. |
| 9 | `dll/win32/comdlg32/printdlg.c` | 34 + `itemdlg.c` 32 | Common dialog Vista+ (Items dialog mới). |
| 10 | `win32ss/user/ntuser/ntstubs.c` | 56 | Nt syscall stubs Win32k thiếu (NtUserNotifyProcessCreate, NtUserCalcMenuBar…). |
| 11 | `dll/win32/ieframe/shellbrowser.c` | 54 | IE shell browser. |
| 12 | `drivers/ksfilter/ks/pin.c` | 53 | Kernel streaming pin – âm thanh/multimedia. |
| 13 | `dll/win32/wtsapi32/wtsapi32.c` | 52 | Terminal Services – Vista+ Remote Desktop API stub. |
| 14 | `dll/win32/ieframe/ie.c` | 52 | IE COM. |
| 15 | `win32ss/printing/base/winspool/printers.c` | 50 | Print spooler Vista+. |
| 16 | `dll/win32/oleaut32/olepicture.c` | 48 | OLE picture / GDI+. |
| 17 | `dll/win32/atl/atl_ax.c` | 48 | ATL ActiveX host. |
| 18 | `dll/win32/lsasrv/lsarpc.c` | 47 | **LSA RPC server** – security backbone. Win7 yêu cầu nhiều RPC mới (LsarOpenPolicy2…). |
| 19 | `base/services/netlogon/rpcserver.c` | 47 | Netlogon RPC – AD/domain logon. |
| 20 | `ntoskrnl/fstub/disksup.c` | 47 | Phân hoạch đĩa, có 3 "REACTOS HACK" cho xHalIoAssignDriveLetters – ảnh hưởng boot/install. |
| 21 | `drivers/bus/pcix/enum.c` | 45 | PCI bus enumerator (port mới). |
| 22 | `drivers/bus/acpi_new/uacpiosl.c` | 44 | OS layer cho uACPI – ACPI driver mới đang port (toàn bộ là `UNIMPLEMENTED_DBGBREAK`). |
| 23 | `base/services/wlansvc/rpcserver.c` | 41 + `dll/win32/user32_vista/wine/misc.c` 41 | Wlansvc RPC (Vista+ WLAN API), user32_vista shim. |
| 24 | `drivers/crypto/ksecdd/stubs.c` | 39 | KSecDD stubs – CNG/BCRYPT chain. |
| 25 | `dll/win32/combase/combase.c` | 39 | COM apartment runtime. |

Các hotspot bổ sung đáng chú ý: `win32ss/user/ntuser/winpos.c` (27), `dll/win32/atl/atl_ax.c` (48), `dll/win32/comctl32/toolbar.c` (38), `boot/freeldr/freeldr/disk/scsiport.c` (42), `dll/win32/mciqtz32/mciqtz.c` (35), `ntoskrnl/io/pnpmgr/devaction.c` (16), `ntoskrnl/mm/ARM3/sysldr.c` (14), `ntoskrnl/mm/ARM3/section.c` (14).

## Phân loại theo Phase

### Phase 0 – Stabilize NT 5.2 (cần xử lý TRƯỚC khi nâng version)

Đây là các nhóm vi phạm tính chính xác/độ ổn định của bản hiện tại; nếu nâng Win7 mà không vá thì sẽ amplify lỗi.

1. **ntoskrnl/mm/ARM3** (ARM3 manager): `virtual.c`, `section.c`, `sysldr.c`, `mdlsup.c`, `procsup.c`, `expool.c`, `pfnlist.c`, `mmsup.c` – tổng ~90 marker, gồm `UNIMPLEMENTED` ở MM core, FIXME về WS/TLB invalidation. Chưa hoàn thiện => crash dưới tải, blocker khi enable WoW64/Win7 patterns.
2. **ntoskrnl/io/pnpmgr** – `devaction.c`, `pnpmgr.c`, `plugplay.c`, `pnpres.c`, `pnpinit.c`: ~40 marker, gồm "Cleanup device", thiếu device properties – PnP rebuild là điều kiện tiên quyết cho Vista+ device class.
3. **ntoskrnl/fstub/disksup.c** – 47 marker, 3 `REACTOS HACK` cho drive letters. Phải dọn trước khi adopt BCD/EFI boot mới.
4. **ntoskrnl/se** (`audit.c` 23, `token.c` 4, `srm.c`, `acl.c`, `accesschk.c`) – security manager chưa đầy đủ audit subsystem; là điều kiện để bật MIC/UAC Vista.
5. **ntoskrnl/ex/sysinfo.c** – `NtQuerySystemInformation` thiếu nhiều class + hard-code counters = 0. Vista/Win7 dùng nhiều class mới (SystemPerformanceInformation v2, SystemProcessInformationEx…).
6. **win32ss/user/ntuser/winpos.c, window.c, defwnd.c, sysparams.c, ntstubs.c** – các "MinMax pos", "DPI aware rcWorkDPI" đã FIXME sẵn. Win32k là blocker cho mọi GUI nâng cấp.
7. **win32ss/gdi/eng/stubs.c + gdi/ntgdi/freetype.c (21)** – DDI eng còn 89 stub; Win7 GDI dùng nhiều entry mới (DirectComposition prep).
8. **drivers/bus/pcix (cây mới)** – `enum.c` 45, `pdo.c` 16, `fdo.c` 14, `init.c` 22, `utils.c` 12, `cardbus.c` 8: port đang triển khai dở của Win2003/Vista PCI driver.
9. **drivers/bus/acpi_new** – `uacpiosl.c` 44 + `uacpi.c` 7 + `interpreter.c` 7: ACPI mới (uACPI) gần như chưa nối. Win7 đòi ACPI 4.0/5.0 + S-states.
10. **base/system/winlogon** – `security.c` 27, `wlx.c` 18, `sas.c` 11: SAS/WLX hiện thiếu nhiều, Win7 dùng Credential Provider chứ không phải GINA → cần lát đường.
11. **base/system/services + rpcserver.c** – SCM còn 26 + 7 + 4 marker. Win7 services có protected services / trigger-start mới.
12. **boot/freeldr** – `winldr.c` 13, `setupldr.c` 3, `wlregistry.c` 6, `i386trap.S` 21, `scsiport.c` 42, `ntoskrnl.c` 7: freeldr cần tải nhân lớn hơn (loader block Vista+). Hiện vẫn FIXME loader API.

### Phase 1 – Vista compat (NT 6.0)

1. **dll/win32/kernelbase** – Wine 'main', 'file', 'locale', 'memory', 'registry', 'thread'… (~150 marker tổng). KernelBase tách khỏi kernel32 là dấu mốc Vista. Cần bù `K32GetModuleInformation` family, `GetCurrentProcessorNumberEx`, …
2. **dll/win32/advapi32/wine/cred.c (20), security.c (13), crypt.c (17)** – CredUI/Credential Manager, các SE function Vista.
3. **dll/win32/dnsapi/stubs.c** – 105 stub Vista API (`DnsQueryEx`, `DnsServiceBrowse`, multicast DNS, IDN).
4. **dll/win32/bcrypt/bcrypt_main.c** – 58 marker. Vista CNG (BCrypt/NCrypt) là dependency của TLS 1.2, BitLocker stack, EFS Vista+.
5. **dll/win32/lsasrv** – `lsarpc.c` 47, `authpackage.c` 8, `policy.c` 7: LSA Vista đặt nền cho MSV1_0 / Kerberos extension, audit policy mới.
6. **dll/win32/wtsapi32** – 52 marker. Terminal Services Vista+ (Remote Apps).
7. **win32ss/user/user32_vista/wine/{misc.c, input.c, win.c, sysparams.c, dpi.c}** – đây là DLL `user32_vista` đang gom các API Vista (`SetProcessDPIAware`, `IsWindowsVistaOrGreater`, raw input v2). Tổng ~88 marker.
8. **dll/win32/combase** – `combase.c` 39, `marshal.c` 20, `rpc.c` 22, `roapi.c` 17: combase tách khỏi ole32 là mốc Vista, là điều kiện cho WinRT sau này.
9. **dll/win32/comdlg32/itemdlg.c (32)** – IFileDialog Vista.
10. **dll/win32/xolehlp (19), dll/win32/dwmapi (22)** – DWM API Vista (compositor).
11. **drivers/ksfilter, drivers/wdm/audio** – KS pin/filter (53 + 19 trong filter.c) + WaveRT-style endpoint chuẩn Vista WASAPI.

### Phase 2 – Win7 features (NT 6.1)

1. **dll/win32/ntmarta/ntmarta.c** – đã có comment "Vista+ API" 2 chỗ. Sẽ là bước tiếp.
2. **dll/win32/shell32 / browseui / explorer (base/shell/explorer/traywnd.cpp 21, taskswnd.cpp 12, syspager.cpp 8)** – Taskbar/Jump list/Aero peek Win7; comment "Vista+ flags" đã có ở `syspager.cpp:626`.
3. **DPI awareness** (`user32_vista/dpi.c` 8) – cần per-monitor DPI Win7+.
4. **Direct2D/DirectWrite/DirectComposition stack** – chưa có module tương đương; phụ thuộc phase 1 DWM.
5. **bcrypt/ncrypt cipher suites & TLS 1.2** – sau khi xong CNG Phase 1.
6. **WMI v2 / event tracing** (`ntoskrnl/wmi/wmi.c` 24, `dll/ntdll/etw/trace.c` 16) – ETW xstack Win7 dùng cho perf counter, Resource Monitor.
7. **dll/win32/bluetoothapis** – chỉ 6 marker, gần như rỗng. Win7 yêu cầu Bluetooth stack.
8. **boot BCD support** – `base/setup/lib/utils/bldrsup.c:9` ghi rõ `TODO: Add support for NT 6.x family! (detection + BCD manipulation).` – là cửa ngõ install Win7-style.

## Commit WIP đang dở

Lịch sử git của clone chỉ có 3 entry (clone + ff + "first commit") nên không có thông điệp WIP riêng. Các "WIP marker" trong mã đáng chú ý:

- `boot/freeldr/freeldr/ntldr/winldr.c` – 13 FIXME loader block, chưa support Vista LOADER_PARAMETER_EXTENSION mới.
- `drivers/bus/acpi_new/*` – cả cây mới uACPI đang dở (`UNIMPLEMENTED_DBGBREAK` xuyên suốt OSL layer).
- `drivers/bus/pcix/*` – PCI driver port theo style mới của Windows, các file enum/init/pdo/fdo còn nhiều FIXME.
- `win32ss/user/user32_vista/*` – DLL shim cho API Vista đang xây.
- `dll/win32/bcrypt/bcrypt_main.c` – semi-stub có ghi `// semi-stub`, comment chỉ rõ "called with unsupported parameters".
- `dll/win32/dbgeng/dbgeng.c` – 358 FIXME, có thể coi như stub-only module.
- `ntoskrnl/fstub/disksup.c` – 3 chỗ `REACTOS HACK -- Needed for xHalIoAssignDriveLetters()` – nợ kỹ thuật rõ ràng cần refactor.
- `dll/win32/ntmarta/ntmarta.c:36,54` – có FIXME đánh dấu trực tiếp "Vista+ API".

Nếu cần lịch sử commit dạng `WIP/[STUB]/TODO` đầy đủ, cần fetch upstream `reactos/reactos` (clone hiện chỉ là fork shallow).

## Khuyến nghị ưu tiên cho team 2–5 người

1. **Tạo Phase 0 freeze trước**: chỉ định 1 người (hoặc cặp) thuần dọn ntoskrnl (ARM3 + PnP + sysinfo + se/audit) trong 4–6 tuần. Không bắt đầu Vista compat khi MM/PnP còn UNIMPLEMENTED – sẽ nhân đôi lỗi. Mục tiêu: zero `UNIMPLEMENTED` trong `mm/ARM3/*.c` và `io/pnpmgr/*.c`.

2. **Tách "Vista shim DLL" thành đường đi rõ**: hoàn thiện `user32_vista`, `kernelbase`, `combase`, `bcrypt`, `dnsapi`, `lsasrv` theo thứ tự dependency (kernelbase → combase → bcrypt → lsasrv → dnsapi → user32_vista). 1 dev có thể đẩy 1 DLL/2 tuần với test harness Wine đã có sẵn trong `modules/rostests/winetests/`.

3. **Đầu tư test trước khi vá**: hotspot `dll/win32/oleaut32`, `dll/win32/comctl32`, `dll/win32/combase` đã có winetest. Bật chúng vào CI và xem tỉ lệ pass hiện tại → dùng làm baseline đo tiến độ Phase 1, tránh "vá xong vẫn vỡ".

4. **PCI/ACPI là blocker cứng cho HW hiện đại**: bố trí 1 người chuyên driver bus theo dõi đồng thời `drivers/bus/pcix` (45 marker chỉ ở enum.c) và `drivers/bus/acpi_new` (44 marker ở OSL). Nếu không có ACPI 4.0+ thì không boot được nhiều laptop Vista/Win7-class. Có thể tận dụng uACPI upstream (chỉ cần hoàn thiện OSL).

5. **Lập "WIP debt board"**: tự động hóa script chạy ripgrep mỗi tuần để theo dõi `count(UNIMPLEMENTED) + count(FIXME)` theo module. Mục tiêu Phase 0: giảm 30% trong 3 tháng ở 4 module ntoskrnl, win32k, freeldr, hal. Đặt SLA: không merge PR Vista/Win7 nếu tăng marker net của module được sửa.

6. **Bổ sung sớm**: vá `boot/freeldr/freeldr/disk/scsiport.c` (42 marker) – freeldr ScsiPort là blocker khi cài bộ kernel mới (Vista/Win7 cần loader block lớn hơn + SCSI/AHCI ổn định). Đây là công việc nhỏ gọn, hợp cho dev mới onboard.

## Phụ lục: file/loại đáng đọc kỹ trước khi lập kế hoạch

- `F:/reactos/apistatus.lst` – chỉ là chỉ mục module cho rgenstat (xem ở `sdk/tools/rgenstat/`); không phải báo cáo trạng thái chi tiết.
- `F:/reactos/sdk/tools/rgenstat/rgenstat.c` – công cụ tự động đếm impl/stub có sẵn; có thể tái sử dụng cho dashboard.
- `F:/reactos/win32ss/user/ntuser/ntstubs.c` – 56 stub Nt syscall, là danh mục thẳng các Win32k API cần.
- `F:/reactos/dll/win32/dnsapi/stubs.c` – danh mục thẳng các API DNS Vista+.
- `F:/reactos/dll/win32/dbgeng/dbgeng.c` – nếu coi debug stack là không ưu tiên, có thể defer toàn bộ.
- `F:/reactos/drivers/bus/acpi_new/uacpi/` – check uACPI upstream version để biết còn thiếu gì ở OSL.
