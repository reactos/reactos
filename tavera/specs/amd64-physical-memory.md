<!--
PROJECT:     Tavera OS
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/amd64-physical-memory.md
PURPOSE:     Specify amd64 physical memory through 6 TB for milestone 31
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# amd64 physical memory

## 1. Header and provenance

- **Component:** amd64 physical memory
- **Version:** not assigned. There is no product-version bump.
- **Author:** Ryan Wilson <ryan@taveraos.com>
- **Date:** 2026-11-18
- **Provenance:** Only allowed sources were used. They are the public document and in-tree files in section 2. The 6 TB ceiling is the Windows 10 x64 Enterprise and Pro for Workstations row on that Learn page. The 36-bit page-frame width is the width already written in `sdk/include/ndk/amd64/mmtypes.h`. The 1 GiB loader map is `MM_MAX_PAGE_LOADER` in this tree. The 128 GB nonpaged-pool ceiling is `MI_MAX_NONPAGED_POOL_SIZE` in this tree and the Learn nonpaged-pool row. Recalled Windows internals are not a source. A missing citation stops the edit.

This file is the spec the implementation follows. A later code change that disagrees with the spec updates the spec in the same change.

This spec is an implementation spec for milestone 31. It is not a raw-format probe spec. It does not interpret guest disk sectors, hive bytes, or other persisted Windows structures. Host signing tools stay staplers. They do not supply an offset, a prototype, or a layout.

Work that changes behavior under this spec follows `.cursor/rules/clean-room.mdc`, `.cursor/rules/clean-room-third-party-sources.mdc`, `.cursor/rules/clean-room-symbols-and-debuggers.mdc`, `.cursor/rules/clean-room-serial-capture.mdc`, `.cursor/rules/clean-room-probe-separation.mdc`, `.cursor/rules/clean-room-model-recall.mdc`, `.cursor/rules/clean-room-exposure.mdc`, and `.cursor/rules/clean-room-host-tools.mdc`. A forbidden input stops the component. The session logs that exposure in `contamination-log.md` outside `source/` and does not continue the component.

## 2. Sources

Every claim below names one of these entries. The date the Learn page was read was not recorded.

1. [Memory Limits for Windows and Windows Server Releases](https://learn.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases). Date read: not recorded. Windows 10 x64 Enterprise and Pro for Workstations are 6 TB. The 64-bit nonpaged-pool row is RAM or 128 GB, whichever is smaller.
2. `sdk/include/ndk/amd64/mmtypes.h` in this tree. `PageFrameNumber:36` and a commented `MM_MAX_PAGE` of 36 PFN bits. `HARDWARE_PTE`, `MMPTE_HARDWARE`, `MMPTE_TRANSITION`, and `MMPTE_HARDWARE_LARGEPAGE`.
3. `boot/freeldr/freeldr/include/mm.h` in this tree. amd64 `MM_MAX_PAGE` and `MM_MAX_PAGE_LOADER`.
4. `boot/freeldr/freeldr/ntldr/wlmemory.c` in this tree. `MempAddMemoryBlock`.
5. `boot/freeldr/freeldr/lib/mm/meminit.c` in this tree. The page lookup table and `MmGetAddressablePageCountIncludingHoles`.
6. `ntoskrnl/mm/amd64/init.c` in this tree. `MmNonPagedPoolStart` and the pool-size multiply.
7. `ntoskrnl/include/internal/amd64/mm.h` in this tree. `MI_MAX_NONPAGED_POOL_SIZE` is 128 GB.
8. `ntoskrnl/ex/init.c` in this tree. The serial banner.
9. A later boot log under `output/` is evidence only for what that log prints. No such log is cited yet.

No WDK or SDK headers, symbols, disassembly, or third-party struct dumps are sources. Geoff Chappell, Vergilius, syscall tables built from Microsoft binaries, and undocumented-API sites are not sources.

## 3. Purpose and scope

GitHub milestone 30 is [Milestone 31: amd64 physical memory](https://github.com/taveraos/taveraos/milestone/30). The project name is Tavera OS. This spec does not rename existing C file headers.

The component accepts amd64 physical memory through 6 TB, keeps the loader's own map at 1 GiB, widens the page-frame fields this `0x502` build compiles to the 36-bit width already in source 2, and sizes nonpaged pool without a 32-bit multiply wrap. The pool still ends before `MM_HAL_VA_START`.

In scope: `MM_MAX_PAGE`, forwarding firmware memory above the 1 GiB loader map, the compiled page-frame fields named in section 4, and the nonpaged-pool size arithmetic.

Left out on purpose:

- Logical processors (milestone 32) and the Vista page-table software bits (milestone 35).
- The root compile stays `_WIN32_WINNT=0x502`.
- The `NTDDI_LONGHORN` side of `MMPTE_HARDWARE` stays uncompiled.
- The i386 path that sizes paged pool as twice nonpaged pool.
- A guest larger than 6 TB, and any boot above 32 GiB, is not run on this machine.
- `MMPFN.u4.PteFrame` stays `MI_PTE_FRAME_BITS` 57 on `_WIN64`.

Work lands on branch `amd64-memory`, cut from `master` at `6b55bbf676a`. Unrelated dirty files stay unstaged and off this branch: `drivers/network/**/CMakeLists.txt`, `tools/pipeline/Build-Bootcd.ps1`, `tools/pipeline/Test-Boot.ps1`, and `tools/windows-trace/Collect-Traces.ps1`.

Each edited C file gains the Ryan Wilson copyright line. The comment under the header lists source 1 and source 2.

The kernel and FreeLoader commit includes `tavera/transparency/2026-11-18-amd64-physical-memory.json` with `question`, `allowedInputs`, and `"spdx": "GPL-2.0-or-later"`. Inputs are source 1, this spec, and the in-tree files in section 2. Add `harness` and `harness.logSha256` only for a guest log that is still on disk and contains the marker the note quotes. Update the milestone description in the same turn as the work: check off a finished task bullet, and replace only the `## Progress` section. Commit when asked. Do not push `upstream`.

## 4. Interfaces

Values already in this tree, then the value this spec requires. Widths are bit counts.

- **`MM_MAX_PAGE`** in source 3. Today `0x1FFFFF`. A commented definition is `0xFFFFFFFFF` (36 bits for the PFN). Required: `0x60000000` (6 TiB / 4 KiB). The comment beside that constant names source 1.
- **`MM_MAX_PAGE_LOADER`** in source 3. `0x3FFFF`. On x64, FreeLoader maps 1 GiB. This value stays.
- **`MempAddMemoryBlock`** in source 4. A run whose base is at or above `MM_MAX_PAGE` is omitted. A run that crosses `MM_MAX_PAGE` is shortened. Parameters and calling convention are the ones already in that file.
- **Page lookup table** in source 5. Today it is sized to the whole firmware map. Required: sized to the 1 GiB the loader maps. `MmGetAddressablePageCountIncludingHoles` stops the table at `MM_MAX_PAGE_LOADER`.
- **`WinLdrSetupMemoryLayout`.** Builds the low descriptors from that table. For every firmware descriptor that extends above the table, the part above 1 GiB goes through `MempAddMemoryBlock`. A descriptor that starts inside the 1 GiB table and continues past it is split.
- **`MAX_MAD_COUNT`.** Stays 200 unless a boot runs out of slots.
- **Loader identity map.** Stays at `0x40000000`. `MempBootPagePresent` and `PcUnmapReservedBootRanges` stay at that address.
- **`HARDWARE_PTE.PageFrameNumber`** in source 2. Today 28 bits, on every build. Required on the definitions this `0x502` build compiles: 36 bits. `reserved1` goes from 12 bits to 4 bits.
- **`MMPTE_HARDWARE`**, the `#else` side in source 2, which is the side this build compiles. `PageFrameNumber` 28 bits to 36. The matching reserved field 12 bits to 4. Write and Dirty stay on the lines they already occupy. The `NTDDI_LONGHORN` side is 36 bits and also moves Write and Dirty. That side stays uncompiled.
- **`MMPTE_TRANSITION`**, the `#else` side. `PageFrameNumber` 28 bits to 36. `Unused` 24 bits to 16. The fields before the page frame are already the same in both branches.
- **`MMPTE_HARDWARE_LARGEPAGE`**, the `#else` side. The page-frame width goes from 19 bits to 27. That 27-bit width is already in the other branch. The Write and Dirty bits above it do not move.
- **`MMPFN.u4.PteFrame`.** Already `MI_PTE_FRAME_BITS` 57 on `_WIN64`. Leave it.
- **`MmNonPagedPoolStart`** in source 6. The first byte after the PFN allocation: `MmPfnDatabase + MxPfnAllocation * PAGE_SIZE`, including the color tables that allocation already covers. The end stays before `MM_HAL_VA_START`.
- **`MI_MAX_INIT_NONPAGED_POOL_SIZE` and `MI_MAX_NONPAGED_POOL_SIZE`.** Both caps stay 128 GB. Source 7 is the 128 GB maximum. Source 1 is the same ceiling as the smaller of RAM and 128 GB.
- **Serial banner** in source 8. The text is `%u System Processor [%Iu MB Memory]`.

The compiled PTE stores a 28-bit page frame today, which names pages below 1 TB, because this tree's `NTDDI` is below `NTDDI_LONGHORN`.

## 5. Behavior

`MempAddMemoryBlock` drops or shortens any run at or above `MM_MAX_PAGE`. With today's `MM_MAX_PAGE` of `0x1FFFFF`, the kernel never sees a page at 8 GiB (source 4). The high-descriptor copy only forwards a BIOS run whose base is already above the full highest page, so it forwards nothing.

Required loader behavior:

- `MM_MAX_PAGE` is `0x60000000`. `MM_MAX_PAGE_LOADER` stays `0x3FFFF`.
- The page lookup table covers the 1 GiB the loader maps, so a large machine does not need an 18 GB table inside that map.
- Low descriptors still come from that table.
- Memory above 1 GiB is passed through `MempAddMemoryBlock`, which applies the 6 TB clip. A descriptor that crosses the 1 GiB boundary is split. The part inside the lookup table is marked there. The high tail is kept.
- The loader identity map stays at `0x40000000`.

Required page-frame behavior is the width changes in section 4, on the definitions this `0x502` build compiles. Write and Dirty do not move. The `NTDDI_LONGHORN` branch stays uncompiled.

Required nonpaged-pool behavior:

- The pool still starts at the first byte after the PFN allocation and still ends before `MM_HAL_VA_START`.
- Both caps stay 128 GB.
- The initial and maximum pool sizes are computed in `SIZE_T` before `MI_MAX_INIT_NONPAGED_POOL_SIZE` and `MI_MAX_NONPAGED_POOL_SIZE` are applied. Today's multiply is done in `ULONG`. `MmNumberOfPhysicalPages` is `PFN_COUNT`, a `ULONG`, times `MmMaxAdditionNonPagedPoolPerMb`. At 16 GiB that product wraps before the 128 GB cap is applied. The `SIZE_T` multiply keeps the 16 GiB and 32 GiB page counts from wrapping.
- amd64 paged pool does not use the i386 "twice nonpaged" path. That path stays as it is.

There is no edition split in this tree, so the physical ceiling is the 6 TB row from source 1.

Threading, IRQL, and memory ownership beyond the pool and loader addresses above are not stated by sources 1 through 8.

## 6. Observed behavior

No black-box test on Windows is recorded. No Windows build is named. The 6 TB and 128 GB figures are the published rows in source 1, not a measurement from a guest.

No boot log under `output/` is cited yet (source 9).

## 7. Compatibility notes

Source 1 lists Windows 10 x64 Enterprise and Pro for Workstations at 6 TB, and 64-bit nonpaged pool as RAM or 128 GB, whichever is smaller. This tree has no edition split, so the physical ceiling is 6 TB.

This tree's compiled page-frame field is 28 bits, which names pages below 1 TB. `HARDWARE_PTE` is 28 bits on every build in source 2. The uncompiled `NTDDI_LONGHORN` side is 36 bits and moves Write and Dirty. This spec widens the compiled fields to 36 bits and leaves Write and Dirty where they are.

No separate Windows 10 22H2 measurement is recorded against an older Windows version.

## 8. Test plan

These checks compare what the boot reports. They do not add a coding step beyond the values in sections 4 and 5.

Build with `tools/pipeline/Build-Bootcd.ps1`. Boot the live CD with `-accel tcg` and `-smp 4`. Do not pass `-accel whpx`. One QEMU at a time. Name the window, what ready looks like, and when to report back.

- Three live-CD boots on this machine: `-m 8192`, `-m 16384`, and `-m 32768`. 32 GiB is the top size given to QEMU here.
- The serial banner in source 8 is the reported RAM. It matches the guest aside from firmware holes. The desktop is the window, confirmed before the log is treated as success.
- The loader's `MM_MAX_PAGE` is `0x60000000`. A guest larger than 6 TB, and any boot above 32 GiB, is not run on this machine. Progress records that.

After those boots, rerun the milestone 8 syscall probe (`buildenv/differential-tests/syscall-diff`) so `NtCreateFile`, `NtWriteFile`, `NtReadFile`, `NtClose`, `NtDeviceIoControlFile`, `NtCreateThreadEx`, and `NtCreateUserProcess` stay on the statuses already recorded. Reuse the Windows log when `Test-WindowsProbe.ps1` prints `REUSE`.

## 9. Open questions

- No Windows build has been measured for these limits. Source 1 is the published table.
- Boots above 32 GiB, and any guest larger than 6 TB, are not run here. Whether `MAX_MAD_COUNT` of 200 is enough on a larger map is unchecked.
- Threading and IRQL for the loader clip and the pool sizing are not stated in sources 1 through 8.
- A boot log that would confirm the banner text against `-m 8192`, `-m 16384`, and `-m 32768` is not cited yet.

## 10. Review log

No review is recorded. Reorganized to this template on 2026-11-18. A reviewer still has to confirm this file has no implementation detail from a forbidden source.
