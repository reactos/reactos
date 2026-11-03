/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 system register helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

    TEXTAREA

//
// ULONG64
// Arm64GetPhysicalAddressRange(VOID);
//
// Returns the PARange field of ID_AA64MMFR0_EL1.
//
    LEAF_ENTRY Arm64GetPhysicalAddressRange
    mrs     x0, id_aa64mmfr0_el1
    and     x0, x0, #0xF
    ret
    LEAF_END Arm64GetPhysicalAddressRange

//
// ULONG
// Arm64GetCurrentEl(VOID);
//
    LEAF_ENTRY Arm64GetCurrentEl
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    and     x0, x0, #3
    ret
    LEAF_END Arm64GetCurrentEl

//
// VOID
// Arm64SwitchTranslation(
//     _In_ ULONG64 Mair,
//     _In_ ULONG64 Tcr,
//     _In_ ULONG64 Ttbr);
//
// Loads the new tables into both halves. The caller runs on pages that are
// identity mapped in both the old and the new tables.
//
    LEAF_ENTRY Arm64SwitchTranslation
    dsb     ish
    msr     mair_el1, x0
    msr     tcr_el1, x1
    msr     ttbr0_el1, x2
    msr     ttbr1_el1, x2
    isb
    tlbi    vmalle1
    dsb     ish
    ic      iallu
    dsb     ish
    isb
    ret
    LEAF_END Arm64SwitchTranslation

    END
