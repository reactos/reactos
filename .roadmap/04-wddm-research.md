# R4 - WDDM Migration Research

> **Scope**: Lộ trình chuyển hệ thống đồ họa ReactOS từ XDDM (XP Display Driver
> Model) sang WDDM 1.1 (yêu cầu tối thiểu của Windows 7).
>
> **Đây là một trong những thay đổi kiến trúc lớn nhất và rủi ro nhất của toàn
> dự án Win7 upgrade.**

---

## 1. Architecture hiện tại (XDDM)

### 1.1 Sơ đồ thành phần

```
+--------------------------------------------------------------+
| User mode                                                    |
|                                                              |
|  Application (GDI/DirectDraw/D3D8/D3D9)                      |
|     |                                                        |
|     v                                                        |
|  gdi32.dll  ddraw.dll  d3d8.dll  d3d9.dll  opengl32.dll      |
|     |          |          |         |          |             |
|     |          v          v         v          |             |
|     |       d3d8thk      (uses ddraw)         mesa.dll       |
|     |                                          (sw 2.6)      |
+-----|----------------------------------------------------|---+
      |  ( NtGdi* / Win32K syscalls )                       |
      v                                                     v
+--------------------------------------------------------------+
| Kernel mode (win32k.sys)                                     |
|                                                              |
|  win32ss/gdi/ntgdi  (GDI kernel side)                        |
|  win32ss/gdi/eng    (GDI DDI / "Eng*" engine functions)      |
|                                                              |
|         +---------------------+                              |
|         |  reactx/dxg.sys     | <-- DirectDraw/D3D kernel   |
|         |  reactx/dxgthk.sys  |     side (XDDM DX kernel)    |
|         |  reactx/dxapi.sys   |                              |
|         |  reactx/ntddraw     |                              |
|         +---------------------+                              |
|                                                              |
|  Display Miniport Driver (XDDM "DrvXxx"):                    |
|    win32ss/drivers/displays/{framebuf, vga, vga_new}         |
|       - DrvEnablePDEV, DrvEnableSurface, DrvSetPalette,      |
|         DrvSetPointerShape, DrvEnableDirectDraw, ...         |
|                                                              |
|  Video Miniport Driver (NT4-style):                          |
|    win32ss/drivers/miniport/{bochs, vbe, vga, vga_new,       |
|                              vmx_svga, xboxvmp, pc98vid}     |
|  win32ss/drivers/videoprt.sys  (video port library)          |
|  win32ss/drivers/watchdog                                    |
+--------------------------------------------------------------+
```

### 1.2 Đặc tính XDDM (current ReactOS)

- **Display driver = một DLL** (`framebuf.dll`, `vga.dll`...) chứa
  `DrvEnablePDEV`/`DrvEnableSurface`/... — chạy trong context `win32k.sys`.
- **Video miniport** là `*.sys` chạy kernel (driver class video). Quản lý
  HW thông qua `videoprt.sys`.
- DirectDraw/D3D đi qua **`dxg.sys` + `dxgthk.sys`** (port nửa vời từ XP của
  Magnus Olsen, 2007). Đây là XDDM DX runtime — không có VidMm/VidSch.
- Không có khái niệm GPU memory paging, không có preemption, không có DWM.
- Mesa software OpenGL 2.6 (`dll/opengl/mesa`) — chỉ thay thế `opengl32.dll`
  của Windows XP (OpenGL 1.1 SW). Không có Gallium trong tree.

### 1.3 Hạt giống WDDM đã có trong tree (rất quan trọng)

Justin Miller (ReactOS dev, 2023–2024) đã đặt nền móng nhưng **chưa có
implementation**:

| File | Trạng thái |
|------|-----------|
| `sdk/include/ddk/d3dkmddi.h` | Header WDDM DDI (kernel-mode display driver interface) — copy từ MS public DDK |
| `sdk/include/ddk/d3dkmthk.h` | Header WDDM thunk (user→kernel) |
| `sdk/include/psdk/d3dkmdt.h` | WDDM data types (D3DKMT_*) |
| `sdk/include/reactos/rddm/rxgkinterface.h` | Định nghĩa `REACTOS_WIN32K_DXGKRNL_INTERFACE` — bảng callback dự kiến giữa win32k và dxgkrnl |
| `win32ss/gdi/gdi32_vista/d3dkmt.c` | User-mode thunks D3DKMT* — **tất cả trả `STATUS_PROCEDURE_NOT_FOUND`** |
| `win32ss/gdi/ntgdi/d3dkmt.c` | Kernel side stubs (`NtGdiDdDDI*`) — tất cả stub, có TODO trỏ tới [CORE-20027](https://jira.reactos.org/browse/CORE-20027) |
| `win32ss/reactx/ntdxvista/` | Vista DX vỏ rỗng — chỉ có `dxprivate.h` |

Không có `dxgkrnl.sys`, không có `dxgi.dll`, không có `d3d10/11.dll`. Mesa
Gallium **không có trong tree** (đề cập trong `Mesa_for_ReactOS.txt` chỉ là
package RAPPS riêng).

---

## 2. Architecture mục tiêu (WDDM 1.1 — Windows 7)

### 2.1 Sơ đồ thành phần

```
+--------------------------------------------------------------+
| User mode                                                    |
|                                                              |
|  Application                                                 |
|     |                                                        |
|     +--> d3d11.dll --> d3d11_1.dll \                         |
|     +--> d3d10.dll                  >--> dxgi.dll            |
|     +--> d3d9.dll  (D3D9Ex)        /                         |
|     +--> gdi32.dll (D3DKMT thunks via NtGdi)                 |
|                          |                                   |
|                          v                                   |
|                  +-----------------+                         |
|                  | UMD (vendor)    |  user-mode display      |
|                  | <vendor>um.dll  |  driver (.dll)          |
|                  +-----------------+                         |
+----------------------|---------------------------------------+
                       | syscalls (NtGdiDdDDI*)
+----------------------v---------------------------------------+
| Kernel mode                                                  |
|                                                              |
|  win32k.sys  --(internal IOCTL/callback bridge)-->  dxgkrnl  |
|                                                              |
|  +------------------- dxgkrnl.sys ------------------------+   |
|  |  - DXGK_INTERFACE (callbacks expose tới KMD)           |   |
|  |  - VidMm   : GPU virtual memory manager, paging,       |   |
|  |              residency, eviction                       |   |
|  |  - VidSch  : GPU scheduler (DMA queue, packet prep,    |   |
|  |              preemption granularity)                   |   |
|  |  - VidPN   : Video Present Network (display topology)  |   |
|  |  - GMM/handle manager, sync objects, fences            |   |
|  |  - GDI hardware acceleration adapter (CDD)             |   |
|  +--------------------------------------------------------+   |
|                       |                                      |
|                       v                                      |
|  +--------- Display Miniport Driver (KMD/DMD) -----------+   |
|  | Vendor .sys exporting DriverEntry that registers      |   |
|  | DXGKDDI_* entry points (DxgkDdiAddDevice,             |   |
|  | DxgkDdiStartDevice, DxgkDdiBuildPagingBuffer,         |   |
|  | DxgkDdiPresent, DxgkDdiSubmitCommand, ...)            |   |
|  +-------------------------------------------------------+   |
|                       |                                      |
|                       v                                      |
|  videoprt.sys (vẫn tồn tại, vai trò thu hẹp lại)             |
|  watchdog.sys (TDR — Timeout Detection & Recovery)           |
+--------------------------------------------------------------+
```

### 2.2 Yêu cầu WDDM 1.1 cụ thể

- GPU virtual memory + page faulting hỗ trợ ở mức **per-DMA-buffer**.
- DMA buffer scheduling, fence-based sync.
- Cursor/overlay/gamma do dxgkrnl trung gian (không gọi thẳng KMD).
- **DWM** (Desktop Window Manager) bật mặc định → phụ thuộc shared surfaces
  cross-process, redirected bitmaps.
- TDR (watchdog) reset GPU khi treo.
- **WHQL signed driver** (về kỹ thuật — ReactOS có thể nới lỏng test signing).

---

## 3. Khoảng cách (Gap analysis)

### 3.1 Phải viết mới gần như từ đầu

| Thành phần | Quy mô (LOC tham khảo Wine/Win) | Ghi chú |
|------------|--------------------------------|---------|
| **dxgkrnl.sys** | 200k–400k LOC | Core. Hiện hoàn toàn chưa có. Microsoft không công bố source. Cần reverse hoặc tái thiết từ DDK header. |
| **VidMm** (trong dxgkrnl) | 30k–60k | GPU VA space, paging, residency lists, segment allocator |
| **VidSch** (trong dxgkrnl) | 20k–40k | DMA packet queue, preemption (coarse @WDDM1.0, mid @1.1), TDR |
| **VidPN** | 10k–20k | Topology graph display sources↔targets |
| **dxgi.dll** | 30k | Adapter enumeration, swapchain, output, factory. Wine có port partial — có thể tận dụng. |
| **d3d10.dll / d3d10core.dll** | 50k+ | Wine có. Cần WDDM bên dưới để chạy thật. |
| **d3d11.dll** | 80k+ | Wine có (d3d11). Cần WDDM. |
| **DWM (dwm.exe + dwmapi.dll + uDWM)** | rất lớn | Win7 phụ thuộc DWM nặng (Aero). Có thể bỏ qua tạm thời. |
| **WDDM display miniport mẫu** (CPU/sw render) | 10k–20k | Cần ít nhất 1 KMD reference để bring-up (giống `BasicRender`/`BasicDisplay` của MS). |
| **Watchdog/TDR mở rộng** | 5k | `watchdog.sys` đã có khung, nhưng chưa hook DXGK. |

### 3.2 Cần thay đổi (không phải viết mới)

- **win32k.sys**: phải tách path GDI → đi qua **CDD** (Canonical Display
  Driver) thay vì gọi trực tiếp `Drv*` của display driver. CDD là một WDDM-aware
  display driver phần mềm của Microsoft — ReactOS phải có tương đương.
- **GDI engine** (`win32ss/gdi/eng`): các `Eng*` callouts thay đổi (vd
  `EngLockSurface`/`EngBitBlt` vẫn còn nhưng đường đi khác).
- **gdi32.dll**: `gdi32_vista` đã có khung nhưng tất cả stub. Phải nối thật
  vào `NtGdiDdDDI*` → IOCTL tới dxgkrnl.
- **videoprt.sys**: giữ lại nhưng vai trò chỉ là PnP + interrupt routing cho
  KMD. Phần mode-set/EDID di chuyển sang DXGK.
- **ntoskrnl/PnP**: bus driver cho display class đổi, INF schema mới
  (`Class = Display` với `DXGK_*` keys).

### 3.3 Có thể tận dụng / port

| Nguồn | Dùng cho | Mức độ |
|-------|----------|--------|
| **Wine** `dlls/d3d8,d3d9,d3d10,d3d11,dxgi,wined3d` | User-mode D3D runtime | Cao — đã port d3d8/d3d9 vào tree |
| **Wine** `dlls/winevulkan` | Vulkan ICD loader | Trung bình — Win7 không bắt buộc Vulkan |
| **Mesa Gallium + LLVMpipe** | Software 3D renderer làm UMD | Cao — đã có precedent, nhưng cần tích hợp với WDDM UMD interface (giống MS WARP) |
| **VirtualBox VBoxVideoWddm** (MIT/GPL) | KMD tham khảo cho VM | **Rất cao** — open-source WDDM 1.x KMD chạy thật, có thể fork làm reference |
| **QEMU virtio-gpu Linux DRM** | Ý tưởng VidMm/VidSch | Tham khảo kiến trúc |
| **Haiku app_server** | Không liên quan trực tiếp | — |

> Lưu ý: Microsoft `BasicRender`/`BasicDisplay` driver source **KHÔNG** public.
> Phải xây tương đương từ đầu hoặc dựa trên VBoxVideoWddm.

---

## 4. Lộ trình đề xuất (5–7 năm, realistic)

> Ước tính dưới giả định 2–3 dev full-time có kinh nghiệm graphics kernel.
> Với tốc độ thực tế của ReactOS (volunteer, bán thời gian), nhân 2–3 lần.

### Phase 0 — Foundation (đã làm một phần, 6–12 tháng nữa)

- [x] Import header WDDM (`d3dkmddi.h`, `d3dkmthk.h`, `d3dkmdt.h`).
- [x] Tạo `gdi32_vista` skeleton + `NtGdiDdDDI*` stubs.
- [x] Định nghĩa `REACTOS_WIN32K_DXGKRNL_INTERFACE`.
- [ ] Hoàn thiện `watchdog.sys` API mà DXGK sẽ gọi.
- [ ] Tách logical XDDM path trong `win32k.sys` (chuẩn bị cho dual-mode).
- [ ] CMake/build infra cho `dxgkrnl.sys` (target rỗng để link/test).

### Phase 1 — Stub dxgkrnl + smoke test (12–18 tháng)

- Tạo `drivers/directx/dxgkrnl/` với:
  - `DriverEntry`, registration with PnP
  - `DXGKRNL_INTERFACE` callback table (stub trả NOT_IMPLEMENTED)
  - IOCTL bridge win32k ↔ dxgkrnl (theo `rxgkinterface.h`)
- `NtGdiDdDDIOpenAdapterFromGdiDisplayName` trả về handle thật.
- Mục tiêu: `dxdiag.exe` enumerate được "adapter ảo" mà không crash.
- Vẫn chạy XDDM display driver song song (fallback).

### Phase 2 — Software-only WDDM stack hoàn chỉnh (18–24 tháng)

- Implement **VidMm tối thiểu**: system memory only (chưa GPU VA thật).
- Implement **VidSch tối thiểu**: round-robin, không preemption.
- Implement **VidPN**: 1 source × 1 target (single display).
- Viết KMD reference **`BasicDisplay`-equivalent** (CPU memcpy framebuffer).
- Viết UMD reference dùng llvmpipe/swrast cho D3D9.
- Tích hợp **CDD** (Canonical Display Driver) cho GDI rendering qua DXGK.
- Mục tiêu: chạy được Aero-disabled desktop Win7-style trên SW renderer.

### Phase 3 — Real GPU support qua VirtualBox/QEMU (12–24 tháng)

- Port **VBoxVideoWddm** vào tree (cần audit license).
- Port **QXL/virtio-gpu KMD** WDDM (chưa có sẵn — phải viết).
- Verify VidMm/VidSch chịu được DMA submit thật.
- Bật **TDR** thực sự.
- Mục tiêu: chạy WDDM bên trong VM với accelerated guest.

### Phase 4 — D3D10/11 runtime + DWM (24–36 tháng)

- Port `d3d10/d3d11/dxgi` từ Wine, nối vào DXGK thật.
- Implement DWM tối thiểu (compositor + thumbnail), bật Aero Basic.
- Implement shared surface, cross-process resource handles.
- Mục tiêu: Aero (cơ bản) chạy, ứng dụng D3D11 chạy được.

### Phase 5 — Real hardware vendor drivers (open-ended)

- Cần WHQL test workaround hoặc test-signing.
- Hỗ trợ Intel HD Graphics (gen 7–9) — open driver hiếm, chỉ có Linux.
- AMD/NVIDIA — gần như không khả thi nếu không có driver open.
- Mục tiêu thực tế: chạy được trên VM + adapter giả lập, không phải bare metal
  với GPU hiện đại.

---

## 5. Risk & Showstopper

### 5.1 Showstopper kỹ thuật

1. **Microsoft không công bố source dxgkrnl** — phải reverse hoặc tái thiết.
   Bất kỳ struct/IOCTL nội bộ nào cũng có thể sai → driver vendor không load.
   *Mitigation*: chấp nhận "ReactOS-specific" DXGK ABI, không nhằm tương thích
   binary với KMD của Windows. Driver vendor cũ vẫn không chạy được.
2. **Vendor KMD binary của Windows KHÔNG load được** trên ReactOS DXGK dù
   header giống — vì cấu trúc nội bộ của `DXGKRNL_PRIVATE_*`, escape codes,
   PnP enumeration order khác. Người dùng cuối kỳ vọng "chạy driver NVIDIA"
   → **không thực hiện được**.
3. **DWM phụ thuộc shared D3D surfaces** mà lại yêu cầu DXGI cross-process
   sharing, KMT handles, GDI redirection — toàn bộ chuỗi này phải đúng *cùng
   lúc* mới render được desktop. Khó debug từng bước.
4. **GDI accelerated path** trong Win7 đã đổi sang đi qua CDD/DXGK; nếu chỉ
   port WDDM mà giữ GDI cũ thì các app dùng GDI sẽ chậm hoặc artifact.

### 5.2 Risk IP / pháp lý

- Reverse-engineer struct nội bộ dxgkrnl: ReactOS đã có policy clean-room.
  WDDM phức tạp hơn nên cần discipline cao, có khả năng vi phạm nếu
  contributor nhìn vào leaked source (vd. Windows Research Kernel, leaks).
- VBoxVideoWddm có MIT license — an toàn để fork.
- Mesa: MIT — an toàn.

### 5.3 Risk dự án

- **Bandwidth**: ReactOS hiện không có team graphics riêng. Justin Miller mới
  là người duy nhất đang đẩy WDDM (theo commit history). Single point of
  failure.
- **API churn**: WDDM 1.1 → 1.2 → 1.3 → 2.0 thay đổi liên tục. Nếu nhắm
  Win7 (WDDM 1.1) thì ổn, nhưng các runtime user-mode mới (DXGI 1.2+) cần
  tính năng 1.2 → "Win7" cuối cùng sẽ phải vá tới 1.2 để chạy phần mềm.
- **Test surface khổng lồ**: mỗi GPU vendor, mỗi driver version cần test.
  ReactOS không có lab.

### 5.4 Cảnh báo lớn nhất

> **WDDM migration là dự án 5–10 năm, không phải 1–2 năm.**
>
> Đây là tasks lớn nhất, dài nhất, và rủi ro nhất trong toàn bộ Win7 upgrade.
> Nếu muốn ship "Win7 ReactOS" trong thời gian thực tế, **phải chấp nhận**:
>
> - Không tương thích binary với driver vendor Windows.
> - Hardware acceleration chỉ chạy trên VM + adapter giả lập trong giai đoạn
>   đầu.
> - Aero/DWM xếp sau cùng — desktop Win7 ban đầu trông giống Basic theme.
> - Có khả năng dự án dừng lại ở "software WDDM" mãi mãi nếu không có vendor
>   nào đóng góp KMD.
>
> **Khuyến nghị**: chia thành sub-roadmap riêng (WDDM-only), với milestone
> độc lập, KHÔNG block các phần khác của Win7 upgrade (kernel, user-mode,
> shell có thể tiến lên mà không cần WDDM hoàn chỉnh — chỉ cần một fallback
> XDDM-on-WDDM-shim hoặc giữ XDDM song song cho đến khi WDDM stable).

---

## 6. Files đã survey

- `F:/reactos/win32ss/drivers/displays/{framebuf,vga,vga_new}` — XDDM display drivers
- `F:/reactos/win32ss/drivers/miniport/{bochs,vbe,vga,vga_new,vmx_svga,xboxvmp,pc98vid}` — video miniport
- `F:/reactos/win32ss/drivers/videoprt/` — videoprt.sys (giữ lại trong WDDM)
- `F:/reactos/win32ss/drivers/watchdog/` — sẽ dùng cho TDR
- `F:/reactos/win32ss/reactx/{dxg,dxgthk,dxapi,ntddraw,ntdxvista}` — XDDM DX kernel
- `F:/reactos/win32ss/gdi/{eng,ntgdi,gdi32,gdi32_vista}` — GDI engine và thunks
- `F:/reactos/win32ss/gdi/gdi32_vista/d3dkmt.c` — user-mode D3DKMT stubs
- `F:/reactos/win32ss/gdi/ntgdi/d3dkmt.c` — kernel D3DKMT stubs + TODO CORE-20027
- `F:/reactos/sdk/include/ddk/d3dkmddi.h` — WDDM KMD DDI header
- `F:/reactos/sdk/include/ddk/d3dkmthk.h` — WDDM user thunk header
- `F:/reactos/sdk/include/psdk/d3dkmdt.h` — WDDM data types
- `F:/reactos/sdk/include/reactos/rddm/rxgkinterface.h` — win32k↔dxgkrnl interface
- `F:/reactos/dll/directx/{d3d8,d3d9,ddraw,wine/*}` — user-mode DX runtimes
- `F:/reactos/dll/opengl/{mesa,opengl32,glu32}` — software GL (không có Gallium)
- Jira ticket: [CORE-20027](https://jira.reactos.org/browse/CORE-20027)

---

*Generated: 2026-05-29 — R4 research, do not modify code yet.*
