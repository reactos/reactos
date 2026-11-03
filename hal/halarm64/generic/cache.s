/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     TLB and barrier helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

    TEXTAREA

//
// VOID
// HalpFlushTbEntry(
//     _In_ PVOID VirtualAddress);
//
    LEAF_ENTRY HalpFlushTbEntry
    lsr     x0, x0, #12
    dsb     ishst
    tlbi    vaae1is, x0
    dsb     ish
    isb
    ret
    LEAF_END HalpFlushTbEntry

//
// VOID
// HalpPublishPteWrites(VOID);
//
    LEAF_ENTRY HalpPublishPteWrites
    dsb     ishst
    isb
    ret
    LEAF_END HalpPublishPteWrites

//
// VOID
// HalpWaitForInterrupt(VOID);
//
    LEAF_ENTRY HalpWaitForInterrupt
    dsb     sy
    wfi
    ret
    LEAF_END HalpWaitForInterrupt

//
// ULONG64
// HalpCallFirmware(
//     _In_ ULONG64 Function,
//     _In_ ULONG64 Argument1,
//     _In_ ULONG64 Argument2,
//     _In_ ULONG64 Argument3,
//     _In_ BOOLEAN UseHvc);
//
// PSCI call, the conduit comes from the FADT.
//
    LEAF_ENTRY HalpCallFirmware
    cbz     w4, HalpCallFirmwareSmc
    hvc     #0
    ret
LABEL(HalpCallFirmwareSmc)
    smc     #0
    ret
    LEAF_END HalpCallFirmware

    END
