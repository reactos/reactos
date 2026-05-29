# R3 - Gap Analysis User-mode API

Phân tích khoảng cách các DLL user-mode giữa ReactOS hiện tại và mục tiêu Windows 7.
Nguồn dữ liệu chính:
- `F:/reactos/media/doc/WINESYNC.txt` (trạng thái sync với Wine)
- Spec files trong `F:/reactos/dll/win32/*` và `F:/reactos/win32ss/{user,gdi}/*`
- Cấu trúc thư mục `dll/win32/*_vista` (tách layer NT6+)

---

## 1. Trạng thái Vista DLLs hiện tại (NT6+ tách lớp)

ReactOS dùng pattern `<dll>_vista.dll` (hoặc thư mục con) làm overlay forward cho các API NT6+, KHÔNG đụng vào spec NT 5.x gốc. Đây sẽ là nơi mở rộng cho Win7.

| Vista DLL | Vị trí | Số API exported | Trạng thái Win7 |
|-----------|--------|-----------------|-----------------|
| **kernel32_vista** | `dll/win32/kernel32/kernel32_vista/` | ~50 (spec 68 dòng) | Có SRW, CondVar, InitOnce, GetTickCount64, GetFileInformationByHandleEx, Locale Ex, GetThreadDescription/SetThreadDescription (gắn nhãn "Win 10"), TrySubmitThreadpoolCallback. **Thiếu**: phần lớn Thread Pool API (CreateThreadpool*, SubmitThreadpoolWork, WaitForThreadpool*), QueryUnbiasedInterruptTime, GetPhysicallyInstalledSystemMemory |
| **advapi32_vista** | `dll/win32/advapi32_vista/` | **6 API** (RegCopyTreeW, RegDeleteTreeA/W, RegSetKeyValueW, RegLoadMUIStringA/W) | Rất nghèo. Thiếu toàn bộ EventLog ETW v2 (EventRegister/Write/Unregister), CredProtect/Unprotect, LSA new, RegGetValueA/W, RegRenameKey, IsTextUnicode mới |
| **user32_vista** | `win32ss/user/user32_vista/` | **2 API** (GetDpiForSystem, GetDpiForWindow) — spec chỉ 2 dòng nhưng file `input.c`/`misc.c`/`win.c` đã sync Wine-10.0 | Code có nhưng chưa export Win7 APIs. Thiếu Touch (RegisterTouchWindow, GetTouchInputInfo…), Gesture (GetGestureInfo…), ChangeWindowMessageFilterEx, GetWindowDisplayAffinity |
| **gdi32_vista** | `win32ss/gdi/gdi32_vista/` | **2 API** (D3DKMTCreateDCFromMemory, D3DKMTDestroyDCFromMemory) | Hầu như trống. Thiếu phần lớn D3DKMT*, GetCharacterPlacementW v2, font fallback Win7 |
| **uxtheme_vista** | `dll/win32/uxtheme/uxtheme_vista.spec` | 1 API (OpenThemeDataForDpi) | Stub-only — uxtheme chính vẫn fork Wine cũ |
| **ws2_32_vista** | KHÔNG TỒN TẠI | — | Cần tạo cho WSK, RIO (Registered I/O — Win8 nhưng prefetch ở Win7 SP1) |
| **iphlpapi_vista** | KHÔNG TỒN TẠI | — | Cần tạo cho NotifyIpInterfaceChange, NotifyUnicastIpAddressChange, ConvertInterfaceLuidTo* (Vista+ thực ra) |
| **shell32_vista** | KHÔNG TỒN TẠI | — | Có thể cần cho IShellLibrary, ITaskbarList3/4, KnownFolders Win7 |
| **comctl32_vista** | KHÔNG TỒN TẠI | — | Cần cho v6.10 (Win7) — Ribbon helper, animation manager mới |

**Đánh giá**: Pattern "_vista" mới chỉ là khung mảnh dẻ. Spec coverage thực rất thấp — `advapi32_vista` chỉ 6 dòng, `user32_vista` 2 dòng, `gdi32_vista` 2 dòng. Phần lớn API NT6+ vẫn nằm ở stub trong DLL chính (xem cột "stub count" mục 2).

---

## 2. DLL hiện tại — mức độ sync với Wine

Tóm tắt từ `media/doc/WINESYNC.txt` (chỉ liệt kê các DLL Win7-relevant). Số stub đếm trong file `.spec` chính (`grep -c stub`).

| DLL | Wine version sync | Stub trong spec | Ghi chú |
|-----|-------------------|-----------------|---------|
| kernel32 | Forked + một số file lẻ sync Wine 3.3 | **191** | Threadpool toàn stub, nhiều API Win7 thiếu hẳn |
| user32 (win32ss) | Một số file Wine Staging 1.7.55 (cũ kỹ), nhiều file forked | 5 | Code đã forked sâu, sync khó |
| gdi32 (win32ss) | Forked; `linedda.c` sync 2009; bezier từ win32k sync 1.9.4 | 0 (spec) | Spec sạch nhưng cài đặt thiếu nhiều |
| advapi32 | wine/cred,crypt,des,lmhash sync 3.3; security.c **out of sync** | ít | sec/security là điểm nóng (LSA, audit Vista+) |
| shell32 | **Forked at Wine-20071011** (cực kỳ cũ ~14 năm) | 18 (visible) | Đây là điểm tắc lớn nhất cho Win7 shell |
| comctl32 | Hỗn hợp: button.c forked 3.3, datetime.c 6.0, phần lớn Wine 5.0 | 0 (spec) | TaskDialog có file nguồn `taskdialog.c` nhưng **không export** trong spec |
| uxtheme | **Forked** hoàn toàn | nhiều (spec dùng -stub) | Cần đại tu cho theme Aero Win7 |
| dwmapi | Synced Wine 8.14 | có | Có `DwmExtendFrameIntoClientArea`, `DwmEnableBlurBehindWindow`, `DwmSetWindowAttribute` — khá tốt cho Win7 baseline |
| gdiplus | Wine Staging 4.0 (theo WINESYNC) — *task brief nói "đã sync Wine 10" nhưng file không khớp* | — | Cần xác minh lại sync; nếu thật sự Wine 10 thì đầy đủ cho Win7 |
| propsys | **Synced Wine 10.0**, 186 exports | 0 | Tốt — sẵn sàng dùng |
| combase | Synced Wine 10.0 | — | Tốt cho COM modern |
| ole32 / oleaut32 | Wine 10.0 / 4.18 | — | OK |
| windowscodecs | Wine 10.0 | — | OK (WIC đầy đủ cho Win7) |
| rpcrt4 | Wine 10.0 | — | OK |
| msvcrt | Wine 10.0 | — | OK |
| msi | Wine 9.8 | — | OK cho Win7 MSI 5.0 baseline |
| winhttp | Wine 10.0 | — | OK |
| wintrust | Wine 10.0 | — | OK |
| uiautomationcore | Wine 10.0 | — | OK — UIA cho a11y Win7 |
| dbghelp | Wine Staging 5.16 | — | Tạm OK |
| crypt32 | Wine Staging 4.0 | — | Cần update cho CNG-related ở Win7 |
| bcrypt | Wine Staging 1.9.23 (RẤT cũ) | — | Cần update — Win7 CNG là mục tiêu chính |
| bcryptprimitives | Wine 10.0 | — | OK |

---

## 3. API/DLL Win7 còn THIẾU HOÀN TOÀN

DLL không tồn tại trong cây nguồn `dll/win32/`:

- **kernelbase.dll** — Vista trở đi, Win7 đẩy thêm nhiều API ra đây. ReactOS chưa có khung. Đây là blocker chiến lược cho api-set scheme Win7+.
- **api-ms-win-*.dll** (API Set schema) — Win7 đã có lớp api-set sơ khai (`api-ms-win-core-*-l1-1-0.dll`). Hoàn toàn chưa có.
- **sensorsapi.dll** + **sensor.h** stack — Win7 Sensor & Location Platform. Chưa có.
- **locationapi.dll** — Win7 Location API. Chưa có.
- **scenario.dll / pla.dll** (Performance Logs & Alerts) — Chưa có.
- **sapi.dll** (SAPI 5.4 Win7) — Chưa có. Chỉ có speech bridge cũ trong WINMM.
- **TaskScheduler 2.0 client** (`taskschd.dll`) — Chưa có. Schtasks cũ Wine.
- **dwrite.dll** — DirectWrite cho Win7. Chưa có.
- **d2d1.dll** (Direct2D) — Chưa có.
- **wer.dll / werconcpl.dll** — Windows Error Reporting v2. Chỉ có `faultrep` cũ.
- **winsta** — có thư mục nhưng winsta extension Win7 (Remote Desktop new APIs) chưa rõ.
- **ntshrui.dll**, **ngcsvc**, **fhsvc** — Win7 services UI. Chưa có.
- **wbio / winbio.dll** (Biometric Framework Win7) — Chưa có.
- **mfplat.dll / mf.dll / evr.dll** — Media Foundation Win7. Chưa có.
- **xpsprint.dll / xpsservices.dll** — XPS print pipeline Win7. Chưa có.

**Shell interfaces Win7 thiếu** (có thể nằm trong shell32 forked 2007):
- `IShellLibrary` (Libraries feature) — chưa thấy GUID/proxy
- `ITaskbarList3` / `ITaskbarList4` (jumplists, thumbnail toolbar, progress) — chưa thấy
- `IFileOperation` (thay SHFileOperation) — chưa thấy export
- `IApplicationDestinations`, `IApplicationDocumentLists`, `ICustomDestinationList` — chưa có (Jump Lists)
- `IUserNotification2`, `IObjectWithAppUserModelID` — chưa có
- `IFileDialog`/`IFileOpenDialog`/`IFileSaveDialog` modern Vista+ — shell32 spec không thấy export (vẫn dùng comdlg32 cổ điển)
- Ribbon Framework (`UIRibbon.dll`) — hoàn toàn chưa có

**comctl32 v6.10 (Win7) thiếu**:
- TaskDialog/TaskDialogIndirect — có code `taskdialog.c` nhưng KHÔNG xuất trong `comctl32.spec`. Cần thêm export.
- `LoadIconMetric`, `LoadIconWithScaleDown` — có 1 dòng `-version=0x600+` cho LoadIconWithScaleDown
- Animation manager / Direct UI mới — không có

---

## 4. API Win7 có 1 phần (đã có nền nhưng thiếu mảng)

| Khu vực | Có gì | Thiếu gì |
|---------|-------|----------|
| kernel32 SRW Locks | InitializeSRWLock, Acquire/Release Exclusive+Shared, TryAcquireSRWLockExclusive ở `kernel32_vista.spec` | TryAcquireSRWLockShared (Win7) |
| kernel32 Condition Variables | InitializeConditionVariable, SleepConditionVariableCS/SRW, WakeAll/Wake ở `kernel32_vista.spec` | Đủ cho Win7 baseline — OK |
| kernel32 Thread Pool | Trong spec NT5 chính: tất cả `CreateThreadpool*` đều `-stub -version=0x600+`. `kernel32_vista.spec` có TrySubmitThreadpoolCallback + FreeLibraryWhenCallbackReturns | Toàn bộ TpAlloc*, SubmitThreadpoolWork, WaitForThreadpoolWorkCallbacks, SetThreadpoolTimer, SetThreadpoolWait, CloseThreadpool*. Có `sdk/lib/rtl/threadpool.c` sync Wine 9.7 nhưng chưa nối export user-mode |
| kernel32 GetTick64 / Timing | GetTickCount64 đã có (`kernel32_vista`). GetSystemTimePreciseAsFileTime forward về GetSystemTimeAsFileTime (giả) | QueryUnbiasedInterruptTime, QueryInterruptTime, QueryInterruptTimePrecise |
| kernel32 Locale Ex | Toàn bộ EnumSystemLocalesEx, GetLocaleInfoEx, LCIDToLocaleName, LocaleNameToLCID, GetUserDefaultLocaleName, IsValidLocaleName đã có | LCMapStringEx, FindNLSStringEx, GetNLSVersionEx |
| kernel32 Symlink/MUI | CreateSymbolicLinkA/W, GetFileMUIInfo, GetFileMUIPath, GetUILanguageInfo, GetThread/UserPreferredUILanguages có | TransactedCreateSymbolicLink stubbed |
| advapi32 ETW | Có evntrace cũ trong `dll/win32/advapi32/misc/evntrace.c` | EventRegister/Write/Unregister/SetInformation (TraceLogging-era), API ETW v2 Win7 (EventEnabled, EventActivityIdControl) |
| advapi32 RegEx | RegCopyTree, RegDeleteTree, RegSetKeyValueW, RegLoadMUIString có (qua `_vista`) | RegGetValueA/W ở spec chính? Cần kiểm tra; RegSaveKeyEx, RegDisablePredefinedCacheEx |
| user32 DPI | GetDpiForSystem, GetDpiForWindow ở `user32_vista` | GetDpiForMonitor (shcore Win8 thực ra), SetProcessDpiAwarenessContext (Win10) — KHÔNG thuộc Win7 baseline, OK |
| user32 Touch/Gesture | Không có gì | Toàn bộ RegisterTouchWindow, GetTouchInputInfo, CloseTouchInputHandle, GetGestureInfo, GetGestureExtraArgs, SetGestureConfig — Win7 multi-touch |
| user32 Window Messages | UpdateLayeredWindowIndirect có | ChangeWindowMessageFilterEx (Win7 UIPI), GetWindowDisplayAffinity/SetWindowDisplayAffinity, IsTouchWindow |
| gdi32 GDI+ | gdiplus có (Wine Staging 4.0 theo WINESYNC) | Effects classes (Blur, Sharpen, Levels…) là Win7 — cần verify sync 10 |
| gdi32 Direct2D bridge | KHÔNG | D2D1CreateFactory, DWriteCreateFactory entry points |
| shell32 KnownFolder | SHGetKnownFolderPath, SHGetKnownFolderItem có trong spec | KnownFolderIDs Win7 (Libraries, Downloads, SearchHistory…) — cần audit guid table |
| shell32 ShellItem | SHCreateShellItem, SHCreateItemFromIDList, SHCreateItemFromParsingName, SHCreateItemInKnownFolder có | IShellItemArray, IShellItem2, IFileDialog vẫn thiếu |
| shell32 Forked 2007 | Đây là toàn bộ explorer/dialog/IShellFolder | Quá cũ — toàn bộ Win7 shell improvements nằm ở Wine 4-10 nhưng KHÔNG sync được do fork |
| comctl32 v6 | DrawShadowText, LoadIconWithScaleDown có | TaskDialog không export, EnableTheme*, ribbon, animation manager |
| uxtheme | Forked, có buffered paint, theme primitives | BeginPanningFeedback/EndPanningFeedback (Win7 scroll feedback), SetWindowThemeNonClientAttributes, OpenThemeDataForDpi (chỉ 1 dòng ở _vista spec) |
| dwmapi | DwmExtendFrameIntoClientArea, EnableBlurBehindWindow, SetWindowAttribute, GetColorizationColor có (Wine 8.14) | DwmpActivateLivePreview (private), DwmGetTransportAttributes; baseline đã đủ cho Aero Win7 |

---

## 5. Khuyến nghị: Port từ Wine những gì TRƯỚC?

Ưu tiên theo tác động/khả thi (P0 = blocker; P3 = nice-to-have):

### P0 — Phải làm sớm vì là phụ thuộc nền
1. **Thread Pool API đầy đủ trong kernel32_vista** — đã có `sdk/lib/rtl/threadpool.c` sync Wine 9.7; chỉ cần thêm exports (CreateThreadpool, SubmitThreadpoolWork, WaitForThreadpoolWorkCallbacks, CloseThreadpool*, SetThreadpoolTimer, SetThreadpoolWait, Tp* RTL forwarders). Rất nhiều phần mềm hiện đại đòi.
2. **Tạo khung kernelbase.dll** + api-set redirects. Không có kernelbase thì không thể chạy nhiều EXE Win7+ build với manifest mới.
3. **advapi32_vista mở rộng**: thêm EventRegister/EventWrite/EventUnregister, EventEnabled, EventActivityIdControl, EventSetInformation. Mã ETW Win7 có thể port từ Wine `advapi32/eventlog.c` mới.
4. **Export TaskDialog từ comctl32** — code đã có `taskdialog.c`; chỉ cần thêm `@ stdcall TaskDialog(...)` + `TaskDialogIndirect(...)` vào spec. Nhanh, tác động lớn (installer dùng nhiều).

### P1 — Tính năng UX Win7 chủ chốt
5. **ITaskbarList3/4 + Jump Lists** trong shell32 (hoặc shell32_vista mới). Port từ Wine `shell32/taskbar*.c`. Khó vì shell32 fork 2007 — có thể tạo overlay shell32_vista.
6. **IFileDialog / IFileOpenDialog / IFileSaveDialog** — Port từ Wine `comdlg32/itemdlg.c` (Vista+ common file dialog). Mở đường cho UX hiện đại.
7. **IShellLibrary** + Libraries feature — Port từ Wine `shell32/shelllink.c` + `shlfolder.c` modern.
8. **dwrite.dll + d2d1.dll**: Port từ Wine `dwrite/`, `d2d1/`. Cho phép app Win7 dùng GDI thay thế.
9. **Touch/Gesture trong user32_vista**: API stubs trả `ERROR_NOT_SUPPORTED` thành công thay vì crash. Sau đó cài đặt thực khi có driver.

### P2 — Mở rộng tương thích
10. **Re-sync comctl32 lên Wine 10** (hiện trộn 3.3/5.0/6.0). Lấy thêm Win7 v6.10 deltas.
11. **bcrypt sync lên Wine 10** (hiện Wine Staging 1.9.23 — rất cũ). Quan trọng cho TLS modern.
12. **mfplat / mf.dll khung** — Port Wine `mfplat/`, `mf/`. Win7 media stack.
13. **wer.dll** — Port Wine `wer/`.

### P3 — Sensor, Speech, Bio (ít app yêu cầu)
14. sensorsapi/locationapi — Wine có sơ khai.
15. sapi.dll mới — Wine partial.
16. winbio — Wine `winbio/` sơ khai.

### Lưu ý chiến lược
- **shell32 fork 2007 là nợ kỹ thuật lớn nhất**. Không thể "sync to Wine 10" trực tiếp — quá nhiều xung khắc. Giải pháp: tạo `shell32_vista.dll` overlay chứa toàn bộ interface Vista/Win7 mới (ITaskbarList3/4, IShellLibrary, IFileDialog, IKnownFolderManager mở rộng) port từ Wine, để shell32 chính chỉ giữ Win2000/XP behavior.
- **uxtheme cũng forked** — cần chiến lược tương tự: mở rộng `uxtheme_vista.spec` (hiện 1 dòng) port BeginPanningFeedback, SetWindowThemeNonClientAttributes.
- Mã `sdk/lib/rtl/threadpool.c` Wine 9.7 ĐÃ tồn tại — chỉ thiếu lớp export user-mode. Đây là quick win lớn nhất.
- **gdi32_vista** và **user32_vista** spec chỉ 2 dòng mỗi cái — nên dùng làm điểm tập kết cho TẤT CẢ API NT6+ mới (tránh chạm spec gốc XP).

---

## 6. Bảng tổng kết ưu tiên (tóm tắt 1 trang)

| Hạng mục | Effort | Impact | Pre-req |
|----------|--------|--------|---------|
| Export Thread Pool từ kernel32_vista | S | Rất cao | rtl/threadpool.c (đã có) |
| Tạo kernelbase + api-set | L | Cao (chiến lược) | — |
| Mở rộng advapi32_vista (ETW) | M | Cao | — |
| Export TaskDialog comctl32 | XS | Trung | taskdialog.c (đã có) |
| Tạo shell32_vista (ITaskbarList3/4, IFileDialog, IShellLibrary) | L | Rất cao | Port Wine modules |
| Port dwrite + d2d1 | L | Cao | DirectX, gdi32 mở rộng |
| user32_vista Touch/Gesture stubs | S | Trung | — |
| Resync bcrypt từ 1.9.23 lên 10 | M | Cao (TLS) | bcryptprimitives đã 10 |
| Mở rộng uxtheme_vista | M | Trung | uxtheme forked |
| Port mfplat/mf | XL | Trung | — |

