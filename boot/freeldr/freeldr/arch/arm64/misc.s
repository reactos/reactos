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
// ULONG64
// Arm64GetHypervisorConfig(VOID);
//
// Returns HCR_EL2, only valid at EL2.
//
    LEAF_ENTRY Arm64GetHypervisorConfig
    mrs     x0, hcr_el2
    ret
    LEAF_END Arm64GetHypervisorConfig

//
// VOID
// Arm64SwitchTranslation(
//     _In_ ULONG64 Mair,
//     _In_ ULONG64 Tcr,
//     _In_ ULONG64 Ttbr);
//
// Loads the new tables into both halves. The caller runs on pages that are
// identity mapped in both the old and the new tables. At EL2 the EL1
// translation regime is set up with the MMU on and the call returns at EL1.
//
    LEAF_ENTRY Arm64SwitchTranslation
    mrs     x3, CurrentEL
    cmp     x3, #(2 << 2)
    b.eq    Arm64SwitchFromEl2

    /* The loader maps its code writable, with WXN from the firmware it could not run */
    mrs     x3, sctlr_el1
    bic     x3, x3, #(1 << 19)
    msr     sctlr_el1, x3
    isb

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

LABEL(Arm64SwitchFromEl2)
    dsb     ish
    msr     mair_el1, x0
    msr     tcr_el1, x1
    msr     ttbr0_el1, x2
    msr     ttbr1_el1, x2

    /* EL1 runs AArch64 without stage 2 and without traps to EL2 */
    mov     x3, #(1 << 31)
    msr     hcr_el2, x3
    msr     vttbr_el2, xzr
    msr     hstr_el2, xzr
    mov     x3, #0x33FF
    msr     cptr_el2, x3

    /* The counters and timers stay reachable from EL1 */
    mrs     x3, cnthctl_el2
    orr     x3, x3, #3
    msr     cnthctl_el2, x3
    msr     cntvoff_el2, xzr

    /* EL1 reads the real processor IDs */
    mrs     x3, midr_el1
    msr     vpidr_el2, x3
    mrs     x3, mpidr_el1
    msr     vmpidr_el2, x3

    /* With a GICv3 CPU interface, EL1 may use its system registers */
    mrs     x3, id_aa64pfr0_el1
    ubfx    x3, x3, #24, #4
    cbz     x3, Arm64SwitchNoGic
    mrs     x3, icc_sre_el2
    orr     x3, x3, #1
    orr     x3, x3, #8
    msr     icc_sre_el2, x3
    isb
    msr     ich_hcr_el2, xzr

LABEL(Arm64SwitchNoGic)
    /* RES1 bits, MMU, caches and stack alignment checks */
    ldr     x3, =0x30D0181D
    isb
    tlbi    vmalle1
    dsb     ish
    ic      iallu
    dsb     ish
    msr     sctlr_el1, x3

    /* Continue after the call at EL1h on the same stack, interrupts masked */
    mov     x3, sp
    msr     sp_el1, x3
    mov     x3, #0x3C5
    msr     spsr_el2, x3
    msr     elr_el2, x30
    isb
    eret
    LEAF_END Arm64SwitchTranslation

    END
