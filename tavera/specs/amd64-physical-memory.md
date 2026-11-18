<!--
PROJECT:     Tavera OS
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/amd64-physical-memory.md
PURPOSE:     Specify amd64 physical memory through 6 TB for milestone 31
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# amd64 physical memory

GitHub milestone 30 is [Milestone 31: amd64 physical memory](https://github.com/taveraos/taveraos/milestone/30). The project name is Tavera OS. This spec does not rename existing C file headers.

Work lands on branch `amd64-memory`, cut from `master` at `6b55bbf676a`. Unrelated dirty files stay unstaged and off this branch: `drivers/network/**/CMakeLists.txt`, `tools/pipeline/Build-Bootcd.ps1`, `tools/pipeline/Test-Boot.ps1`, and `tools/windows-trace/Collect-Traces.ps1`.

The root compile stays `_WIN32_WINNT=0x502`. Logical processors (milestone 32) and the Vista page-table software bits (milestone 35) stay untouched. There is no product-version bump.

This file is the spec the implementation follows. A later code change that disagrees with the spec updates the spec in the same change.

## Clean room

These rules are strict for this work:

- `.cursor/rules/clean-room.mdc`
- `.cursor/rules/clean-room-third-party-sources.mdc`
- `.cursor/rules/clean-room-symbols-and-debuggers.mdc`
- `.cursor/rules/clean-room-serial-capture.mdc`
- `.cursor/rules/clean-room-probe-separation.mdc`
- `.cursor/rules/clean-room-model-recall.mdc`
- `.cursor/rules/clean-room-exposure.mdc`
- `.cursor/rules/clean-room-host-tools.mdc`

Behavior, names, sizes, and layouts come from a public document with a title and URL, a file already in this tree, or a guest log from a boot of this tree. The 6 TB ceiling is the Windows 10 x64 Enterprise and Pro for Workstations row on the Learn page below. The 36-bit page-frame width is the width already written in `sdk/include/ndk/amd64/mmtypes.h`. The 1 GiB loader map is `MM_MAX_PAGE_LOADER` in this tree. The 128 GB nonpaged-pool ceiling is `MI_MAX_NONPAGED_POOL_SIZE` in this tree and the Learn nonpaged-pool row.

Recalled Windows internals are not a source. A missing citation stops the edit. A forbidden input stops the component: leaked NT source, the Windows Research Kernel, a decompiler listing, a disassembly, Microsoft symbols, a debugger attached to Windows, or a WDK or SDK header. Geoff Chappell, Vergilius, syscall tables built from Microsoft binaries, and undocumented-API sites are not sources. The session logs the exposure in `contamination-log.md` outside `source/` and does not continue that component.

This spec is an implementation spec for milestone 31. It is not a raw-format probe spec. It does not interpret guest disk sectors, hive bytes, or other persisted Windows structures. Host signing tools stay staplers. They do not supply an offset, a prototype, or a layout.

## Allowed sources

- [Memory Limits for Windows and Windows Server Releases](https://learn.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases): Windows 10 x64 Enterprise and Pro for Workstations are 6 TB. The same page's 64-bit nonpaged-pool row is RAM or 128 GB, whichever is smaller. There is no edition split in this tree, so the physical ceiling is 6 TB.
- In-tree layouts only: `sdk/include/ndk/amd64/mmtypes.h` already has `PageFrameNumber:36` and a commented `MM_MAX_PAGE` of 36 PFN bits. `boot/freeldr/freeldr/include/mm.h` already sets `MM_MAX_PAGE_LOADER` to the 1 GiB the loader maps (`0x3FFFF`). `ntoskrnl/mm/amd64/init.c` already sets `MmNonPagedPoolStart` to the byte after the PFN allocation and asserts that end is before `MM_HAL_VA_START`. `MI_MAX_NONPAGED_POOL_SIZE` in `ntoskrnl/include/internal/amd64/mm.h` is already 128 GB.
- A later boot log under `output/` is evidence only for what that log prints. No WDK or SDK headers, symbols, disassembly, or third-party struct dumps.

## What is capped today

`boot/freeldr/freeldr/include/mm.h` defines the amd64 limits as:

```c
//HACK: ReactOS AMD64 can't handle the full memory range yet CORE-20265
//#define MM_MAX_PAGE        0xFFFFFFFFF /* 36 bits for the PFN */
#define MM_MAX_PAGE        0x1FFFFF
#define MM_MAX_PAGE_LOADER 0x3FFFF /* on x64 freeldr only maps 1 GB */
```

`MempAddMemoryBlock` in `boot/freeldr/freeldr/ntldr/wlmemory.c` drops or shortens any run at or above `MM_MAX_PAGE`, so the kernel never sees a page at 8 GiB. The lookup table in `boot/freeldr/freeldr/lib/mm/meminit.c` is sized to the whole firmware map. The high-descriptor copy only forwards a BIOS run whose base is already above that full highest page, so it forwards nothing.

The compiled PTE (this tree's `NTDDI` is below `NTDDI_LONGHORN`) stores a 28-bit page frame, which names pages below 1 TB. `HARDWARE_PTE` is 28 bits on every build. The `NTDDI_LONGHORN` side of `MMPTE_HARDWARE` is 36 bits, and it also moves Write and Dirty. That side stays uncompiled.

`ntoskrnl/mm/amd64/init.c` already places nonpaged pool after the PFN database:

```c
/* Non paged pool starts after the PFN database */
MmNonPagedPoolStart = MmPfnDatabase + MxPfnAllocation * PAGE_SIZE;
...
ASSERT(MmNonPagedPoolEnd < (PVOID)MM_HAL_VA_START);
```

The maximum-pool multiply is still done in `ULONG` (`MmNumberOfPhysicalPages` is `PFN_COUNT`, a `ULONG`, times `MmMaxAdditionNonPagedPoolPerMb`). At 16 GiB that product wraps before the 128 GB cap is applied.

## Loader

In `boot/freeldr/freeldr/include/mm.h`, set amd64 `MM_MAX_PAGE` to `0x60000000` (6 TiB / 4 KiB). Leave `MM_MAX_PAGE_LOADER` at `0x3FFFF`. The comment beside the new constant names the Learn page. A run whose base is at or above that page is omitted; a run that crosses it is shortened. That is the existing clip in `MempAddMemoryBlock`.

Size the page lookup table to the 1 GiB the loader maps. `MmGetAddressablePageCountIncludingHoles` stops the table at `MM_MAX_PAGE_LOADER` so a large machine does not need an 18 GB table inside the 1 GiB map.

In `WinLdrSetupMemoryLayout`, keep building the low descriptors from that table. For every firmware descriptor that extends above the table, pass the part above 1 GiB through `MempAddMemoryBlock`, which applies the 6 TB clip. A descriptor that starts inside the 1 GiB table and continues past it is split, so the high tail is not dropped. `MAX_MAD_COUNT` stays 200 unless a boot actually runs out of slots.

The loader identity map stays at `0x40000000`. `MempBootPagePresent` and `PcUnmapReservedBootRanges` stay at that address.

## Page-frame fields

Edit only the definitions this `0x502` build compiles, in `sdk/include/ndk/amd64/mmtypes.h`:

- `HARDWARE_PTE`: `PageFrameNumber` 28 to 36, `reserved1` 12 to 4.
- `MMPTE_HARDWARE` `#else`: same 28 to 36 and 12 to 4. Write and Dirty stay on the lines they already occupy. Do not compile the `NTDDI_LONGHORN` branch.
- `MMPTE_TRANSITION` `#else`: 28 to 36 and `Unused` 24 to 16. The fields before the page frame are already the same in both branches.
- `MMPTE_HARDWARE_LARGEPAGE` `#else`: 19 to 27. That 27-bit width is already in the other branch, and the Write and Dirty bits above it do not move.

`MMPFN.u4.PteFrame` is already `MI_PTE_FRAME_BITS` 57 on `_WIN64`. Leave it. Add the Ryan Wilson copyright line on each edited C file. The comment under the header lists the Learn page and this header.

## Nonpaged pool

Keep `MmNonPagedPoolStart` at the first byte after the PFN allocation (`MmPfnDatabase + MxPfnAllocation * PAGE_SIZE`), including the color tables that allocation already covers. Keep both caps at 128 GB.

In `ntoskrnl/mm/amd64/init.c`, compute the initial and maximum pool sizes in `SIZE_T` before applying `MI_MAX_INIT_NONPAGED_POOL_SIZE` and `MI_MAX_NONPAGED_POOL_SIZE`, so the 16 GiB and 32 GiB page counts cannot wrap the multiply. The pool must still end before `MM_HAL_VA_START`. amd64 paged pool is not on the i386 "twice nonpaged" path; leave that path alone.

## Boot and regression

Build with `tools/pipeline/Build-Bootcd.ps1`. Boot the live CD with `-accel tcg` and `-smp 4`. Do not pass `-accel whpx`. One QEMU at a time. Name the window, what ready looks like, and when to report back.

- Three live-CD boots on this machine: `-m 8192`, `-m 16384`, and `-m 32768`. 32 GiB is the top size given to QEMU here.
- The serial banner in `ntoskrnl/ex/init.c` (`%u System Processor [%Iu MB Memory]`) is the reported RAM. It matches the guest aside from firmware holes. The desktop is the window, confirmed before the log is treated as success.
- The 6 TB clip stays in the loader as `MM_MAX_PAGE` `0x60000000`. A guest larger than 6 TB, and any boot above 32 GiB, is not run on this machine. Progress records that.

After those boots, rerun the milestone 8 syscall probe (`buildenv/differential-tests/syscall-diff`) so `NtCreateFile`, `NtWriteFile`, `NtReadFile`, `NtClose`, `NtDeviceIoControlFile`, `NtCreateThreadEx`, and `NtCreateUserProcess` stay on the statuses already recorded. Reuse the Windows log when `Test-WindowsProbe.ps1` prints `REUSE`.

## Record

The kernel and FreeLoader commit includes `tavera/transparency/2026-11-18-amd64-physical-memory.json` with `question`, `allowedInputs`, and `"spdx": "GPL-2.0-or-later"`. Inputs are the Learn URL, this spec, and the in-tree files above. Add `harness` and `harness.logSha256` only for a guest log that is still on disk and contains the marker the note quotes. Update the milestone description in the same turn as the work: check off a finished task bullet, and replace only the `## Progress` section. Commit when asked. Do not push `upstream`.
