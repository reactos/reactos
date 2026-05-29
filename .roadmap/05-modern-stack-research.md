# R5 - Modern Subsystems Research (Win7 Upgrade)

Phạm vi: 4 subsystem hiện đại của Windows 7 cần thiết để ReactOS đạt tương thích Win7.
Repo: `F:/reactos`. Đây là tài liệu research, KHÔNG thay đổi mã.

---

## 1. DWM (Desktop Window Manager) + Aero

### Current state
- `dwmapi.dll` tồn tại ở `F:/reactos/dll/win32/dwmapi/`:
  - File chính `dwmapi_main.c`: hầu hết API là **stub** trả về `S_OK`/`E_NOTIMPL`.
  - `DwmIsCompositionEnabled` đã có nhưng luôn trả `FALSE` (vì `RtlGetVersion` báo version < 6.3 trong ReactOS hiện tại).
  - `DwmExtendFrameIntoClientArea`, `DwmEnableBlurBehindWindow`, `DwmRegisterThumbnail`, `DwmSetWindowAttribute`... đều `FIXME stub`.
  - `DwmGetCompositionTimingInfo` có implement giả (chỉ dùng QPC).
- `dwmapi.spec` liệt kê đầy đủ ordinal Win7/Vista (`DwmpGetColorizationParameters`, `DwmpDxBindSwapChain`, `DwmpDxgiEnableRedirection`,...) nhưng phần lớn là `stub -noname`.
- `uxtheme.dll` (`F:/reactos/dll/win32/uxtheme/`) đã có khá đầy đủ:
  - `themehooks.c` (USERAPIHOOK), `draw.c`, `nonclient.c`, `msstyles.c`, `pngsup.cpp`, `stylemap.c`.
  - File phụ `uxtheme_vista.spec` cho thấy có chuẩn bị một số API Vista/7.
  - Theme engine hiện tại = **classic msstyles parser**, vẽ trực tiếp qua GDI (không qua compositor).
- **Không có** `dwm.exe` (process compositor); **không có** `d2d1.dll`, `dwrite.dll`, `dcomp.dll`. Chỉ có `dxgiformat.idl` mức header.

### Gap (so với Win7)
1. Không có DWM process: cần tạo `dwm.exe` chạy per-session, hook win32k để intercept tất cả render of top-level windows → vẽ vào off-screen surface, rồi compose lại.
2. Không có per-window redirection surface trong win32k (`NtUserGetCompositionRedirectionWindow` v.v).
3. Không có Direct2D, DirectComposition (DCOMP), DWrite. Phải có ít nhất D2D 1.0 hoặc GDI compositor giả lập.
4. DXGI chỉ có header — chưa có swapchain, không có flip-model presentation.
5. Aero glass (blur-behind, extend frame, transparency): cần shader hoặc software-blur, plus compositor.
6. Thumbnail / Live Preview / Flip3D / Taskbar peek: stubs.
7. Color/Composition events: `DwmGetTransportAttributes`, MMCSS scheduler integration.
8. Theme `aero.msstyles` (PNG-based, có animation): hiện uxtheme đã hỗ trợ PNG basic, nhưng chưa hỗ trợ animation timeline và state transitions của Aero.

### Phụ thuộc
- **DXGI 1.1+** (swapchain, adapter enumeration). Hiện chỉ có IDL.
- **Direct2D + DWrite**: cần port từ Wine (Wine có `d2d1` partial).
- **DirectComposition (dcomp.dll)** – Win8+ thực ra, Win7 dùng MIL (Media Integration Layer) qua `dwmcore.dll` + `milcore.dll`.
- **WDDM driver model** ở video stack (hiện ReactOS dùng XDDM/legacy). Đây là cản trở lớn nhất.
- MMCSS service (multimedia class scheduler) – chưa có.
- `dwmapi` → `dwm.exe` IPC (LPC port `DwmApiPort`).

### Effort estimate
- **Massive**. Ước tính 18–30 tháng/người-năm cho phiên bản tối thiểu chấp nhận được:
  - WDDM-lite + DXGI: 6–9 tháng.
  - D2D/DWrite port từ Wine + glue: 4–6 tháng.
  - `dwm.exe` compositor + win32k redirection: 6–9 tháng.
  - Aero theme + animation + glass: 2–4 tháng.
- Có thể đi đường tắt: **disable compositor**, chỉ implement `dwmapi` API trả về giá trị hợp lý để app không crash, giữ classic theme. Effort: 1–2 tháng (đây là pragmatic path).

---

## 2. WASAPI Audio

### Current state
- `mmdevapi.dll` (`F:/reactos/dll/win32/mmdevapi/`) đã có khung:
  - `devenum.c`: implement `IMMDeviceEnumerator` (đọc registry `HKLM\Software\Microsoft\Windows\CurrentVersion\MMDevices\Audio`).
  - `audiovolume.c`: endpoint volume (partial).
  - `mmdevapi.spec`: chỉ export `DllGetClassObject` + 14 stub ordinal.
  - **Không có** `IAudioClient`, `IAudioRenderClient`, `IAudioCaptureClient`, `IAudioSessionManager` implementation. (header `audioclient.idl` đã có).
- `audiosrv` (`F:/reactos/base/services/audiosrv/`): service legacy, dựa trên `KSCATEGORY_AUDIO`, mục đích chỉ enumerate device → cấp cho WinMM/wdmaud.drv. Note trong `audiosrv.txt`: AudioSrv ROS thiết kế để chạy song song với AudioSrv Windows.
- Stack hiện tại = **MMSYSTEM legacy**:
  - User: `winmm.dll` → `wdmaud.drv` (`F:/reactos/drivers/wdm/audio/legacy/wdmaud/`).
  - Kernel: `sysaudio.sys` (`F:/reactos/drivers/wdm/audio/sysaudio/`) → `portcls.sys` (`F:/reactos/drivers/wdm/audio/backpln/portcls/`) → KS miniport.
- `mmsys.cpl` (`F:/reactos/dll/cpl/mmsys/`): Sound control panel kiểu XP (không phải sndvol per-app slider của Win7).
- **Không có** `audiodg.exe` (Audio Device Graph isolation process).
- **Không có** mixer per-session, không có ducking, không có spatial.

### Gap (so với Win7)
1. **`IAudioClient` interface**: chưa implement (shared-mode + exclusive-mode, event-driven + push). Đây là API trung tâm của WASAPI.
2. **`audiodg.exe`**: process chạy riêng (Local Service, protected) hosting audio engine, mix per-session streams → render device. Bao gồm APO (Audio Processing Objects).
3. **Session manager**: `IAudioSessionManager2` + per-app volume / mute / ducking, hiển thị trong sndvol.
4. **MMCSS** ("Pro Audio", "Audio" classes): scheduler ưu tiên thread audio. ReactOS chưa có.
5. **Endpoint detection** dựa Pin Category KSPROPSETID_Jack — portcls đã có một phần nhưng UI/UX endpoint của Win7 chưa.
6. **Format conversion / resampler** trong kernel mode (KS Audio resampler MFT) – hiện chưa.
7. WaveRT (Wave Realtime) port driver: code có (`port_wavert.cpp` filter), nhưng cần test với HD Audio bus driver mới.
8. `sndvol.exe` mới (per-app slider) — hiện ReactOS dùng sndvol32 XP style.

### Phụ thuộc
- **HD Audio bus driver** (`hdaudbus`): đã có ở `F:/reactos/drivers/wdm/audio/hdaudbus/`. Cần kiểm tra tương thích Win7 INF.
- COM apartment + LPC: `mmdevapi.dll` ↔ `audiodg.exe` qua RPC LRPC.
- Avrt.dll (MMCSS user-mode API) — chưa có.
- ksuser.dll: có sẵn.
- WMF (Media Foundation): không bắt buộc cho WASAPI cốt lõi, nhưng nhiều APO phụ thuộc.

### Effort estimate
- **Medium-large**. 9–15 tháng/người-năm:
  - `IAudioClient`/`IAudioRenderClient` + Render path: 3–4 tháng.
  - `audiodg.exe` + IPC: 3–4 tháng.
  - Session manager + sndvol mới: 2–3 tháng.
  - MMCSS service + Avrt: 1–2 tháng.
- Reference tốt: code Wine `mmdevapi`/`mmdevapi.dll` đã có `IAudioClient` (PulseAudio/ALSA backend). Có thể port logic, thay backend bằng KS/portcls.

---

## 3. BCD Boot (Boot Configuration Data)

### Current state
- ReactOS dùng **FreeLoader** ở `F:/reactos/boot/freeldr/`:
  - Đọc `freeldr.ini` (text file) để liệt kê OS entries — `lib/inifile/inifile.c`, `ini_init.c`, `parse.c`.
  - `bootmgr.c`: loading methods cho `WindowsNT40`, `Windows`, `Windows2003`, `WindowsVista`, `ReactOSSetup`.
  - `ntldr/winldr.c`: tương thích NTLDR/osloader (Vista-style).
  - Hỗ trợ UEFI: `arch/uefi/uefildr.c` (có boot UEFI cơ bản, không phải bootmgfw chuẩn Win).
- Thư mục `F:/reactos/boot/bcd/` **trống** (chỉ có `.keep`) — chưa khởi tạo bất kỳ code/registry hive BCD nào.
- `boot/environ/` **không tồn tại** (đây là EFI Boot Manager subsystem trong source Win10 leak — ReactOS chưa có).
- Grep `BCD` trong `boot/freeldr`: 0 hit liên quan logic (chỉ random match trong RTC code).
- `hivebcd.inf` ở `boot/bootdata/`: chỉ là placeholder cho registry hive (chưa kiểm tra nội dung — likely là skeleton).

### Gap (so với Win7)
1. **bootmgr.exe**: Windows 7 boot manager (EFI + BIOS), đọc `\Boot\BCD` (registry hive) để liệt kê entries. ReactOS chưa có.
2. **winload.exe / winload.efi**: OS loader thực sự (kế thừa ntldr nhưng có signed kernel verification, kernel address ASLR, BCD-driven). FreeLoader's `winldr.c` chỉ tương thích NTLDR style command-line, không tương thích BCD options.
3. **BCD store reader**: parse registry hive offline (NT hive format) để đọc `{bootmgr}`, `{default}`, `{globalsettings}`, `{current}` GUID entries. Chưa có code.
4. **bcdedit.exe**: utility CLI để sửa BCD store. Chưa có (ReactOS dùng edit `freeldr.ini` qua `bootcfg`/boot CPL).
5. **bootmgfw.efi** (UEFI Secure Boot signed boot manager): không có. ReactOS uefildr trực tiếp là bootloader.
6. **Recovery Environment (WinRE)** entries trong BCD: chưa.
7. **PCAT/EFI dual support** với cùng BCD store: chưa.
8. **MeasuredBoot / TPM PCRs**: chưa.

### Phụ thuộc
- Hive parser: ReactOS đã có `cmlib`/`hivelib` (registry kernel + freeldr `wlregistry.c`). Tái sử dụng được để parse BCD hive.
- GUID lib + RPC: có.
- UEFI runtime services (`SetVariable` cho BootOrder): freeldr UEFI đã có chạm tới, cần mở rộng.
- Signed loader: cần code signing infrastructure + cert chain (gần như chính trị, không kỹ thuật).

### Effort estimate
- **Medium**. 4–8 tháng/người-năm:
  - BCD hive reader/writer + bcdedit CLI: 2–3 tháng.
  - bootmgr.exe (BIOS + UEFI): 3–4 tháng (có thể fork freeldr).
  - winload.exe tương thích bootlib API mà ntoskrnl Win7 expect (LOADER_PARAMETER_BLOCK extensions): 2–3 tháng.
- Khả thi vì freeldr đã làm phần lớn việc loader; chủ yếu thay nguồn cấu hình `freeldr.ini` → BCD hive và bổ sung extended LPB fields.

---

## 4. UAC + Mandatory Integrity Control (MIC)

### Current state
- **MIC SIDs có sẵn** ở `dll/win32/advapi32/wine/security.c`:
  - `WinLowLabelSid`, `WinMediumLabelSid`, `WinHighLabelSid`, `WinSystemLabelSid` với `SECURITY_MANDATORY_LABEL_AUTHORITY`.
  - Header `sdk/include/xdk/setypes.h`, `ndk/setypes.h` định nghĩa `SE_GROUP_INTEGRITY`, `TokenIntegrityLevel`, các RID `SECURITY_MANDATORY_*`.
  - Test ở `modules/rostests/winetests/advapi32/security.c` — testset có nhưng impl còn thiếu.
- **Không có** `consent.exe` (UAC consent UI prompt).
- **Không có** `appinfo.dll` service (Application Information service — handles `CreateProcess` elevation).
- `base/services/umpnpmgr/`: PnP manager, không liên quan UAC consent (đã check).
- `userinit.exe`, `winlogon.exe`, `lsass.exe`, `logonui.exe` đều tồn tại nhưng theo model XP (single-token, không split-token cho admin).
- Whoami (`base/applications/cmdutils/whoami/whoami.c`) có hiển thị integrity level → token APIs đọc được ở user-mode.
- Kernel-side: SRM (Security Reference Monitor) cần check Mandatory Label trên ACL (SACL có `SYSTEM_MANDATORY_LABEL_ACE`). Cần xác minh ntoskrnl/se có enforce label policy chưa — grep cho thấy có ở `setypes.h` mà chưa thấy logic enforcement trong se* code.
- UIPI (User Interface Privilege Isolation): win32k chưa filter window messages (`SendMessage`, `SetWindowsHookEx`, `journal hook`) theo integrity level.

### Gap (so với Win7)
1. **Split token logon**: LSA cần tạo 2 token cho admin user (filtered + linked admin) khi logon. Hiện LSA chỉ tạo 1 token.
2. **`appinfo.dll` service** (RPC server) implement `RAiLaunchAdminProcess`. Là core của UAC. Hoàn toàn chưa có.
3. **`consent.exe`**: chạy trong Secure Desktop (separate WinSta), hiển thị prompt, ký consent → app info service. Chưa có.
4. **Manifest parsing for `requestedExecutionLevel`** trong `kernel32!CreateProcessInternal` + shell32 ShellExecute → chuyển sang appinfo nếu cần elevate. Hiện CreateProcess không inspect manifest cho UAC.
5. **Token Linked**: `TokenLinkedToken`, `SeFilterToken` để tạo filtered admin token. Set partial trong ndk nhưng cần check enforce.
6. **MIC enforcement trong SRM**:
   - `SeAccessCheck` phải compare requester's integrity level vs object label trước khi check DACL.
   - "No Write Up" (default) + optional "No Read Up", "No Execute Up".
   - Chưa thấy code path này trong `ntoskrnl/se/*` (chưa grep sâu — cần task riêng).
7. **UIPI** trong win32k: `NtUserBuildHwndList`, `SendMessage`, hooks phải reject calls từ low → high IL. Chưa có.
8. **Virtualization** (LUAFV file/registry redirection cho legacy app): cần `luafv.sys` filesystem filter — chưa có ở ReactOS.
9. **Secure Desktop**: tạo WinSta riêng cho consent prompt + switch desktop. Code cơ bản win32k có CreateDesktop, nhưng workflow secure desktop chưa hook.
10. **AdminApprovalMode + EnableLUA** registry policy + group policy plumbing.

### Phụ thuộc
- LSA + Netlogon refactor để hỗ trợ split-token.
- Kernel SRM patch.
- win32k message filter (UIPI).
- Shell32 + kernel32 manifest reading.
- RPC infrastructure (đã có).
- Group Policy engine: ReactOS có gpsvc partial.

### Effort estimate
- **Large**. 12–18 tháng/người-năm:
  - SRM MIC enforcement (kernel): 3–4 tháng.
  - UIPI (win32k): 2–3 tháng.
  - LSA split-token + linked token: 2–3 tháng.
  - appinfo.dll + consent.exe + Secure Desktop UX: 3–4 tháng.
  - Manifest plumbing + ShellExecute runas integration: 1–2 tháng.
  - LUAFV (file/registry virtualization): 2–3 tháng (có thể skip ban đầu).

---

## Tổng kết Critical Path

```
            (independent)              (independent)
              BCD Boot                  WASAPI Audio
                 |                          |
                 v                          v
            (foundation)               (foundation)
                                        
                          UAC + MIC
                       (kernel SRM + LSA)
                              |
                              v
                            DWM
                  (đòi UAC vì dwm.exe chạy
                   Local Service tách session,
                   plus cần WDDM driver model)
```

### Quan hệ phụ thuộc thực sự
1. **BCD**: hoàn toàn độc lập với 3 stack còn lại. Có thể làm song song. Block chỉ chuyện "boot tương thích Win7 ISO/dual-boot".
2. **WASAPI Audio**: độc lập về kernel/SRM, nhưng `audiodg.exe` chạy như Local Service với integrity SYSTEM → tốt hơn có MIC trước để tận dụng isolation đúng cách. Tuy nhiên có thể implement WASAPI **trước** MIC, chỉ chấp nhận audiodg không isolation.
3. **UAC + MIC**: là **prerequisite** cho:
   - DWM (vì compositor cần chạy ở session-scope với integrity boundary).
   - Audio APO sandbox.
   - Bất kỳ Win7 app nào dùng `requestedExecutionLevel="requireAdministrator"`.
   - Modern Explorer (libraries, public folders dùng IL).
4. **DWM** là stack **đắt nhất** và phụ thuộc WDDM driver model (không nằm trong R5 — sẽ là một research task riêng). Realistically: triển khai theo 2 pha:
   - Pha 1: stub `dwmapi` trả giá trị hợp lý + giữ classic theme (effort thấp).
   - Pha 2: full compositor (sau khi có WDDM, D2D, DXGI).

### Đề xuất thứ tự thực thi
1. **BCD** (vì độc lập, tăng tương thích cài đặt, effort vừa phải).
2. **UAC + MIC** kernel-side foundation (SRM + LSA split token) — đặt nền cho mọi thứ.
3. **WASAPI** song song với UAC (team khác).
4. **UAC user-mode** (appinfo + consent) sau khi kernel sẵn.
5. **DWM stub layer** ngay (để app Win7 chạy không crash).
6. **WDDM + D2D + Full DWM** thành milestone dài hạn, gắn với research riêng.

### Effort tổng (tối thiểu functional)
| Stack | Effort | Critical? |
|---|---|---|
| BCD | 4–8 person-months | Medium (UX cài đặt) |
| WASAPI | 9–15 person-months | High (mọi app modern) |
| UAC + MIC | 12–18 person-months | High (foundation) |
| DWM stub | 1–2 person-months | Low |
| DWM full | 18–30 person-months | Medium (graphics-heavy apps) |
| **Tổng tối thiểu** | **~28–55 PM** | (không tính DWM full) |

### Risks
- **WDDM**: là dependency ngầm cho DWM. Toàn bộ video stack ReactOS hiện là XDDM. Đây là project riêng quy mô tương đương cả R5 này.
- **Driver signing / Secure Boot**: cản trở dual-boot với Win7+ trên UEFI nếu không có cert.
- **Registry hive format BCD**: định dạng được document một phần (Geoff Chappell), không hoàn toàn ổn định giữa các SP — cần test với Win7 RTM/SP1.
- **Wine/MS API surface**: Wine đã có mmdevapi+IAudioClient nhưng dùng PulseAudio backend — phải viết lại backend KS-based.

---

*Tài liệu nghiên cứu. Không sửa code trong commit này.*
