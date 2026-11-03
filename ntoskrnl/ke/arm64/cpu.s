/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 TLB and cache maintenance
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

    TEXTAREA

//
// VOID
// KeArm64FlushTbEntry(
//     _In_ PVOID Address);
//
// Drops the translations of one page for every ASID on every processor.
//
    LEAF_ENTRY KeArm64FlushTbEntry
    lsr     x0, x0, #12
    dsb     ishst
    tlbi    vaae1is, x0
    dsb     ish
    isb
    ret
    LEAF_END KeArm64FlushTbEntry

//
// VOID
// KeArm64FlushTbAll(VOID);
//
    LEAF_ENTRY KeArm64FlushTbAll
    dsb     ishst
    tlbi    vmalle1is
    dsb     ish
    isb
    ret
    LEAF_END KeArm64FlushTbAll

//
// VOID
// KeArm64SweepIcacheRange(
//     _In_ PVOID BaseAddress,
//     _In_ SIZE_T Length);
//
// Writes new code back to the point of unification and drops stale
// instructions, CTR_EL0.DminLine gives the data cache line size.
//
    LEAF_ENTRY KeArm64SweepIcacheRange
    mrs     x3, ctr_el0
    ubfx    x4, x3, #16, #4
    mov     x5, #4
    lsl     x5, x5, x4
    sub     x6, x5, #1
    add     x1, x0, x1
    bic     x2, x0, x6
LABEL(KeArm64SweepIcacheClean)
    cmp     x2, x1
    b.hs    KeArm64SweepIcacheInvalidate
    dc      cvau, x2
    add     x2, x2, x5
    b       KeArm64SweepIcacheClean
LABEL(KeArm64SweepIcacheInvalidate)
    dsb     ish
    ic      ialluis
    dsb     ish
    isb
    ret
    LEAF_END KeArm64SweepIcacheRange

//
// VOID
// KeArm64SweepDcacheRange(
//     _In_ PVOID BaseAddress,
//     _In_ SIZE_T Length);
//
// Cleans and invalidates to the point of coherency, for non coherent DMA.
//
    LEAF_ENTRY KeArm64SweepDcacheRange
    mrs     x3, ctr_el0
    ubfx    x4, x3, #16, #4
    mov     x5, #4
    lsl     x5, x5, x4
    sub     x6, x5, #1
    add     x1, x0, x1
    bic     x2, x0, x6
LABEL(KeArm64SweepDcacheLoop)
    cmp     x2, x1
    b.hs    KeArm64SweepDcacheDone
    dc      civac, x2
    add     x2, x2, x5
    b       KeArm64SweepDcacheLoop
LABEL(KeArm64SweepDcacheDone)
    dsb     sy
    ret
    LEAF_END KeArm64SweepDcacheRange

//
// VOID
// KeArm64WaitForInterrupt(VOID);
//
    LEAF_ENTRY KeArm64WaitForInterrupt
    dsb     sy
    wfi
    ret
    LEAF_END KeArm64WaitForInterrupt

//
// VOID
// KeArm64SetTranslationBase(
//     _In_ ULONG64 RootTable);
//
// Both halves share one root table, the new process gets a clean TLB.
//
    LEAF_ENTRY KeArm64SetTranslationBase
    dsb     ish
    msr     ttbr0_el1, x0
    msr     ttbr1_el1, x0
    isb
    tlbi    vmalle1
    dsb     nsh
    isb
    ret
    LEAF_END KeArm64SetTranslationBase

//
// VOID
// KeZeroPages(
//     _In_ PVOID Address,
//     _In_ ULONG Size);
//
// Size is a whole number of pages.
//
    LEAF_ENTRY KeZeroPages
    mov     w1, w1
LABEL(KeZeroPagesLoop)
    stp     xzr, xzr, [x0], #16
    stp     xzr, xzr, [x0], #16
    stp     xzr, xzr, [x0], #16
    stp     xzr, xzr, [x0], #16
    subs    x1, x1, #64
    b.hi    KeZeroPagesLoop
    ret
    LEAF_END KeZeroPages

    END
