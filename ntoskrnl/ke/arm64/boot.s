/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 boot stack and PCR setup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

/* KeInitializeThread builds the idle thread's frames right below its initial stack */
#define BOOT_STACK_RESERVE  (KTRAP_FRAME_LENGTH + KARM64_VFP_STATE_LENGTH + \
                             KEXCEPTION_FRAME_LENGTH + KSTART_FRAME_LENGTH + \
                             KSWITCH_FRAME_LENGTH)

    IMPORT KiSystemStartupBootStack

    TEXTAREA

//
// VOID
// KiArm64SetPcr(
//     _In_ PKIPCR Pcr);
//
// x18 is reserved for the platform, in kernel mode it holds the PCR.
// TPIDR_EL1 keeps a copy to reload x18 on entry from user mode.
//
    LEAF_ENTRY KiArm64SetPcr
    msr     tpidr_el1, x0
    mov     x18, x0
    ret
    LEAF_END KiArm64SetPcr

//
// DECLSPEC_NORETURN
// VOID
// KiSwitchToBootStack(
//     _In_ ULONG_PTR InitialStack);
//
    LEAF_ENTRY KiSwitchToBootStack
    sub     sp, x0, #BOOT_STACK_RESERVE
    mov     x29, #0
    mov     x30, #0
    b       KiSystemStartupBootStack
    LEAF_END KiSwitchToBootStack

    END
