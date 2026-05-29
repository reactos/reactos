# R2 - Gap Analysis Kernel API (ntoskrnl + ntdll)

> Phạm vi: so sánh trạng thái kernel/native ReactOS hiện tại với Windows Vista (NT 6.0) và Windows 7 (NT 6.1).
> Cơ sở: ntoskrnl đang ở mức NT 5.2 (Server 2003 SP1). Đã có sẵn các shim module `*_vista`.

---

## 1. Trạng thái `ntdll_vista` / shim layer hiện tại

### 1.1 `dll/ntdll/nt_0600/ntdll_vista.spec` — đã có thật
- `LdrRegisterDllNotification`, `LdrUnregisterDllNotification` (forward sang `rtl_vista`)
- **SRW Locks** (`RtlInitializeSRWLock`, `Acquire/Release Shared/Exclusive`, `TryAcquireSRWLockExclusive`)
  → Mã thật ở `sdk/lib/rtl/srw.c`.
- **Condition Variables** (`RtlInitialize/Wake/WakeAll/SleepConditionVariableCS/SRW`)
  → Mã thật ở `sdk/lib/rtl/condvar.c`.
- **RunOnce** (`RtlRunOnceInitialize/BeginInitialize/Complete/ExecuteOnce`)
  → `sdk/lib/rtl/runonce.c`.
- **Thread Pool API (Vista)** — Tp* (toàn bộ 35 hàm): `TpAllocPool`, `TpAllocTimer`, `TpAllocWait`, `TpAllocWork`, `TpAllocIoCompletion`, `TpCallback*`, `TpReleaseCleanupGroup*`, `TpSetPoolMin/MaxThreads`, …
  → `sdk/lib/rtl/threadpool.c` (port từ Wine).
- **SM (Session Manager) RPC**: `RtlConnectToSm`, `RtlSendMsgToSm`.
- **Locale**: `RtlLcidToLocaleName`, `RtlLocaleNameToLcid`, `RtlCompareUnicodeStrings` (`sdk/lib/rtl/unicode_vista.c`).

### 1.2 `sdk/lib/drivers/ntoskrnl_vista/` — đã có shim driver-side
File / API đã có:

| File | API |
|------|-----|
| `ke.c` | `KeQueryActiveProcessorCount`, `KeQueryHighestNodeNumber` (return 0), `KeGetCurrentNodeNumber` (return 0), `KeSetCoalescableTimer` (chỉ wrap `KeSetTimerEx`, bỏ qua `TolerableDelay`) |
| `io.c` | `IoGetIrpExtraCreateParameter`, `IoQueueWorkItemEx`, `IoGetIoPriorityHint` (luôn Normal), `IoSetMasterIrpStatus`; `IoSet/GetDevicePropertyData`, `IoSetDeviceInterfacePropertyData` → `STATUS_NOT_IMPLEMENTED` |
| `po.c` | `PoRegisterPowerSettingCallback`, `PoUnregisterPowerSettingCallback`, `PoQueryWatchdogTime`, `PoSet/GetSystemWake` → stub/NOT_IMPLEMENTED |
| `pofx.c` | **Toàn bộ PoFx*** → `UNIMPLEMENTED` (Vista Power Framework v1) |
| `etw.c` | `EtwWrite`, `EtwRegister`, `EtwUnregister` → `STATUS_NOT_IMPLEMENTED` |
| `fsrtl.c` | `FsRtlRemoveDotsFromPath`, `FsRtlValidateReparsePointBuffer`, `FsRtlGetEcpListFromIrp`, `FsRtlGetNextExtraCreateParameter` (đã implement thật) |

> Nhận xét: `ntoskrnl_vista` là **static library cho driver**, không phải kernel thật. Driver Vista-class có thể link và chạy, nhưng feature thực sự (PoFx, ETW kernel, IO priority hint, KTM, ALPC) chưa hoạt động.

### 1.3 Các shim user-mode khác (sẵn có)
`advapi32_vista`, `kernel32_vista`, `gdi32_vista`, `user32_vista`, `uxtheme_vista`, `rtl_vista` — tồn tại đầy đủ thư mục và spec.

---

## 2. Feature Vista (NT 6.0) còn thiếu / chỉ là stub

| Feature | Module ReactOS | Trạng thái | Effort |
|---|---|---|---|
| **ALPC (Async LPC)** — 21 syscall `NtAlpc*` | `ntoskrnl/lpc/` (vẫn LPC cổ điển) | **STUB** trong `ntdll.spec` (`-version=0x600+`). Không có `ntoskrnl/alpc/`. Wine/Windows CSRSS, RPC over LPC, WMI đều dùng ALPC. | XL (3-6 tháng): cần thiết kế lại object type `AlpcPort`, completion list, message attributes, security context |
| **Worker Factory** (`NtCreateWorkerFactory`, `NtSetInformationWorkerFactory`, `NtWaitForWorkViaWorkerFactory`, `NtReleaseWorkerFactoryWorker`, `NtShutdownWorkerFactory`, `NtWorkerFactoryWorkerReady`, `NtQueryInformationWorkerFactory`) | Không tồn tại trong `ntoskrnl/ps/` | **STUB toàn bộ**. Là backend kernel cho `Tp*` Thread Pool — hiện `threadpool.c` ở user mode đang phải emulate. | L (1-3 tháng): object type WorkerFactory + integration `KQUEUE` |
| **NtCreateThreadEx / NtCreateUserProcess** | `ntoskrnl/ps/thread.c`, `process.c` | **STUB** (`-version=0x600+`). ReactOS chỉ có `NtCreateThread`/`NtCreateProcessEx` cổ điển. Wine và Vista+ kernel32 đều ưu tiên gọi 2 syscall này. | M (3-6 tuần) |
| **KTM (Kernel Transaction Manager)** — `NtCreateTransactionManager`, `NtCreateTransaction`, `NtCreateResourceManager`, `NtCreateEnlistment`, `NtCommit/Rollback/Prepare/Recover*`, `NtFreeze/ThawTransactions`, `NtRenameTransactionManager` | Không có `ntoskrnl/tm/`. Chỉ có header `sdk/include/psdk/ktmtypes.h`. | **STUB toàn bộ**. Tiền đề cho TxF (NTFS transaction) và TxR (registry). | XL (3-6 tháng) |
| **CLFS (Common Log File System)** | Không tồn tại | **THIẾU HOÀN TOÀN** — không có file `clfs*`. KTM phụ thuộc CLFS. | XL |
| **NtCreateKeyTransacted / NtOpenKeyTransacted** | `ntoskrnl/config/` | STUB | M (cần KTM trước) |
| **Private Namespaces** (`NtCreatePrivateNamespace`, `NtOpenPrivateNamespace`, `NtDeletePrivateNamespace`) | `ntoskrnl/ob/` | STUB. Cần cho COM/RPC isolation, AppContainer | M |
| **NtCancelIoFileEx / NtCancelSynchronousIoFile** | `ntoskrnl/io/iomgr` | STUB. Wine WriteFile sync cancel phụ thuộc | S (1-2 tuần) |
| **NtRemoveIoCompletionEx** (IO Completion v2 — bulk dequeue) | `ntoskrnl/io/iomgr` | STUB. Win Server 2008 perf path | S-M |
| **NtGetNextProcess / NtGetNextThread** | `ntoskrnl/ps` | STUB. Process Hacker, modern toolhelp dùng | S |
| **NtFlushProcessWriteBuffers** | `ntoskrnl/ex` | STUB. Lock-free algorithms, .NET GC | S (1 ngày — gọi KeFlushQueuedDpcs broadcast) |
| **NtTraceControl** | `ntoskrnl/wmi` | STUB. ETW control path | M |
| **NtQueryLicenseValue** | — | STUB. WinSAT, licensing | XS |
| **NtMapCMFModule** (CMF — Code Map Format, dùng cho MUI resources) | — | STUB | S |
| **NtInitializeNlsFiles / NtGetNlsSectionPtr** | `ntoskrnl/ex/locale.c` | STUB. Vista chuyển NLS sang sortdefault.nls | M |
| **NtAcquireCMFViewOwnership / NtReleaseCMFViewOwnership** | — | STUB | S |
| **PoFx (Power Framework v1)** — `PoFxRegisterDevice`, `PoFxActivateComponent`, `PoFxIdleComponent`, `PoFxSetDeviceIdleTimeout`, … | `ntoskrnl_vista/pofx.c` | **STUB toàn bộ**. Driver Vista WDF cần PoFx mới ra/vào D-states đúng | L |
| **PoRegisterPowerSettingCallback** | `ntoskrnl_vista/po.c` | STUB. Driver lắng nghe sleep/lid events | S |
| **PoQueryWatchdogTime, PoSet/GetSystemWake** | `ntoskrnl_vista/po.c` | STUB | S |
| **ETW kernel-mode** (`EtwRegister`, `EtwUnregister`, `EtwWrite`) | `ntoskrnl_vista/etw.c` | STUB. Vista yêu cầu driver event provider | L |
| **IoSetDevicePropertyData / IoGetDevicePropertyData / IoSetDeviceInterfacePropertyData** (DEVPROPKEY API) | `ntoskrnl_vista/io.c` | STUB. PnP Vista (devnode property store unified) | M |
| **IoGetIoPriorityHint / IoSetIoPriorityHint** | `ntoskrnl_vista/io.c` | Trả về Normal — không thực sự lưu hint | S |
| **KeQueryHighestNodeNumber / KeGetCurrentNodeNumber** (NUMA topology) | `ntoskrnl_vista/ke.c` | Hard-coded 0 (single node) | S (cần SAL/NUMA detection trong HAL) |
| **KeSetCoalescableTimer** (`TolerableDelay`) | `ntoskrnl_vista/ke.c` | Wrap `KeSetTimerEx`, bỏ qua tolerance → idle power impact | S |
| **Symlink filesystem** (`NtCreate*` với `IO_REPARSE_TAG_SYMLINK`) | `ntoskrnl/io/iomgr` | Có FsRtlValidateReparsePointBuffer nhưng IRP_MJ_CREATE chưa follow symlink mặc định | M |
| **Dynamic Hardware Partitioning** (`KeAddProcessor`, hot-add CPU) | `ntoskrnl/ke` | Thiếu | L (datacenter feature, low-priority cho Win7 client) |
| **Mandatory Integrity Control (MIC)** trong SeAccessCheck | `ntoskrnl/se` | Có SID nhưng AccessCheck chưa enforce integrity level | M |
| **UAC token split / Filtered Admin Token** (`NtFilterToken` với `LUA_TOKEN`) | `ntoskrnl/se/token.c` | NtFilterToken có sẵn nhưng Vista UAC flow chưa đầy đủ | M |

---

## 3. Feature Win7 (NT 6.1) thêm

| Feature | Module ReactOS | Trạng thái | Effort |
|---|---|---|---|
| **PoFx v2 / Runtime PM** — refinements `PoFxCompleteIdleState`, `PoFxReportDevicePoweredOn` | `ntoskrnl_vista/pofx.c` | STUB. Win7 USB autosuspend, GPU power gating | L |
| **User-Mode Scheduling (UMS)** — `NtUmsThreadYield`, `KiUserSchedulerRoutine` | Không có | THIẾU. x64-only, rất ít app dùng (chỉ SQL Server) | L — có thể skip |
| **TxF/NTFS Transactions trên driver fs** | `sdk/lib/drivers/ntoskrnl_vista` | Phụ thuộc KTM | XL |
| **Heap Vista LFH (Low-Fragmentation Heap) mặc định** | `sdk/lib/rtl/heap.c` | RTL có LFH nhưng không bật mặc định | S |
| **Process Mitigation Policies** (`ProcessMitigationPolicy` info class) | `ntoskrnl/ps/query.c` | Chưa | M |
| **ASLR Vista+** (mandatory image base randomization) | `ntoskrnl/mm/ARM3/section.c` | Có boot-time KASLR cho ntoskrnl nhưng user image ASLR còn cơ bản | M |
| **SMB2 / WSK (Winsock Kernel)** | `drivers/network` | Thiếu | XL — không phải scope kernel-API |
| **Win7 BCD store + winload.exe boot path** | `boot/freeldr` (chỉ có FreeLdr style NTLDR) | Không có `bcd*` file, không có `winload`. ReactOS dùng FreeLdr riêng. | L — workaround: tiếp tục dùng FreeLdr nhưng đọc được BCD hive |
| **PatchGuard (Kernel Patch Protection, x64)** | — | KHÔNG có, và **không nên port** (anti-feature làm khó dev) | — (Skip) |
| **SLAT / EPT / Hyper-V root partition** | — | THIẾU. Cần cho Hypervisor. | XL — skip cho mục tiêu Win7-client |
| **Boot Configuration Data (BCD) registry hive parser trong kernel** | `ntoskrnl/config` | Hive BCD file có (`bootdata/hivebcd.inf`) nhưng kernel không tiêu thụ — Vista winload truyền BCD options qua LOADER_PARAMETER_BLOCK | M |
| **WNF (Windows Notification Facility)** — `NtCreateWnfStateName`, `NtSubscribeWnfStateChange` | — | THIẾU (chính thức là Win8 nhưng đã prototype trong Win7 internal). Có thể bỏ qua | — |
| **DPC Stack riêng (per-CPU)** | `ntoskrnl/ke/dpc.c` | Hiện DPC chạy trên thread stack hiện tại; Vista+ có DPC stack riêng cho safety | M |
| **Process Reflection** (`NtCreateProcessReflection`) | — | THIẾU. Crash dump on running process | M |
| **NtCreateSectionEx** (image policy) | `ntoskrnl/mm/ARM3/section.c` | Có NtCreateSection cổ điển | S |
| **Affinity Group (>64 CPU)** | `ntoskrnl/ke` | KAFFINITY vẫn ULONG_PTR, không có group | L — không cần cho desktop |

---

## 4. Critical path để chạy được kernel "Vista-class"

Thứ tự ưu tiên (gate sequential — cái sau phụ thuộc cái trước):

1. **Bật `_WIN32_WINNT=0x600` cho ntoskrnl chính** + merge `ntoskrnl_vista` shims vào ntoskrnl khi đủ ổn. Hiện shim chỉ link cho **driver**, không cho kernel.
2. **NtCreateUserProcess + NtCreateThreadEx** (P0 — Wine, kernel32 Vista, csrss đều gọi)
3. **NtFlushProcessWriteBuffers** (P0 — quick win, vài giờ; .NET, lock-free C++)
4. **NtGetNextProcess / NtGetNextThread** (P0 — toolhelp/PH; trivial)
5. **NtCancelIoFileEx / NtCancelSynchronousIoFile** (P1 — Wine IO unblocks)
6. **NtRemoveIoCompletionEx** (P1 — IOCP-heavy apps: SQL/IIS/Node)
7. **Worker Factory** (P1 — giải phóng `Tp*` thread pool RTL khỏi việc tự emulate)
8. **ALPC** (P0 cho Vista subsystem) — cần thiết kế object mới, port từ Wine/ReactOS WIP nếu có. Đây là **gate** lớn nhất: csrss Vista, RPC over LPC mới, lsass mới, audio service đều phụ thuộc.
9. **Private Namespaces + NtCreate*KeyTransacted** (P2 — sau ALPC)
10. **IoGet/SetDevicePropertyData (DEVPROPKEY)** (P1 — driver Vista PnP)
11. **PoRegisterPowerSettingCallback + power setting GUIDs** (P1 — Vista power UI hoạt động)
12. **PoFx (Power Framework v1)** (P2 — driver Vista WDF mới chạy hết feature)
13. **ETW kernel** (`EtwRegister/Write/Unregister`) (P2 — driver verifier, perf trace)
14. **Mandatory Integrity Control + UAC token split** (P1 cho UX Vista đúng nghĩa)
15. **KTM + CLFS** (P3 — TxF/TxR; có thể defer sang giai đoạn Win7)
16. **NUMA topology thật (KeQueryHighestNodeNumber)** (P3 — server)

### Critical path tối thiểu để boot user-mode Vista
```
Bật _WIN32_WINNT=0x600 → NtCreateUserProcess + NtCreateThreadEx
  → ALPC (port hoặc skeleton-impl đủ cho CSRSS Vista)
  → Worker Factory (cho Tp* thread pool)
  → Private Namespace
  → IoGetIrpExtraCreateParameter (đã có) + DEVPROPKEY
  → MIC + UAC token split
```

### Quick wins (< 1 tuần mỗi cái)
- `NtFlushProcessWriteBuffers`
- `NtGetNextProcess` / `NtGetNextThread`
- `NtCancelIoFileEx`
- `KeSetCoalescableTimer` honoring `TolerableDelay`
- `IoGetIoPriorityHint` thật (đọc từ IRP extension)

---

## 5. Khuyến nghị

1. **Giai đoạn 1 (Vista-class kernel skeleton)** — focus 4 mục: NtCreateUserProcess/ThreadEx, Worker Factory, ALPC skeleton, Private Namespace. Đây là 4 mảnh chặn không user-mode Vista nào chạy được kể cả khi shim đầy đủ.
2. **ALPC là tâm điểm**: kéo dài 3-6 tháng nhưng không có lối tắt; CSRSS Vista+, RPC, audio, BITS đều cần. Cân nhắc port code ALPC từ:
   - WRK 1.2 (chỉ Server 2003 — KHÔNG có ALPC, vô ích)
   - Wine `ntdll/unix/server.c` (user-side reference)
   - Pattern Microsoft công bố qua symbol + papers (Alex Ionescu)
3. **Tận dụng `ntoskrnl_vista` đã có**: hiện là static lib chỉ cho driver. Có 2 cách:
   - Mở rộng nó thành kernel-module riêng load sau ntoskrnl (giống `mountmgr.sys`).
   - **Khuyên dùng**: dần merge các stub vào ntoskrnl chính và bật `_WIN32_WINNT=0x600` cho toàn bộ ntoskrnl, ngừng tách shim.
4. **`ntdll_vista` đã giải quyết được mảng RTL/user-mode** (SRW, CondVar, RunOnce, Tp*). Vấn đề còn lại nằm 90% ở **kernel syscalls** — nếu syscalls thật chỉ là stub thì user-mode shim cũng vô nghĩa với các app gọi trực tiếp Nt*.
5. **Skip cho phạm vi Win7-client**: PatchGuard, UMS, SLAT/Hyper-V, Affinity Group, Process Reflection. Defer KTM/CLFS sang phase cuối.
6. **Quick wins** (mục Critical path #3-5, #10-11) nên làm ngay tuần đầu — nâng compat ngay với nhiều ứng dụng Vista mà không cần refactor lớn.
7. **BCD parser**: không cần thiết kế lại boot — FreeLdr giữ nguyên. Chỉ cần ntoskrnl đọc được `LOADER_PARAMETER_BLOCK` mở rộng kiểu Vista (extension v2/v3) để các driver Vista không AV khi truy cập field mới.

---

*Tài liệu này không thay đổi mã. Mọi kết luận dựa trên grep/glob/đọc file tại snapshot hiện tại của `F:/reactos`.*
