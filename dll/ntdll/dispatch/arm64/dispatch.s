/*
 * PROJECT:     ReactOS NT Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 user mode entry points
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

/* APC frame the kernel builds at sp before entering KiUserApcDispatcher */
#define ApNormalRoutine     0x00
#define ApNormalContext     0x08
#define ApSystemArgument1   0x10
#define ApSystemArgument2   0x18
#define ApTestAlert         0x20
#define ApContext           0x30

    IMPORT LdrpInit
    IMPORT ZwContinue
    IMPORT RtlRaiseStatus

    TEXTAREA

//
// VOID
// LdrInitializeThunk(
//     _In_ PVOID NormalContext,
//     _In_ PVOID SystemArgument1,
//     _In_ PVOID SystemArgument2,
//     _In_ PCONTEXT Context);
//
// Every thread starts with this APC, KiUserApcDispatcher continues with
// the thread's context once the loader returns.
//
    LEAF_ENTRY LdrInitializeThunk
    mov     x0, x3
    b       LdrpInit
    LEAF_END LdrInitializeThunk

//
// The unwind data covers the nops in front of the entry point, so the
// entry counts as the body: unwinding drops the APC header and restores
// the interrupted context stored after it.
//
    NESTED_ENTRY KiUserApcDispatcherUnwind
    nop
    nop
    nop
    nop

    ALTERNATE_ENTRY KiUserApcDispatcher
    ldr     x16, [sp, #ApNormalRoutine]
    ldr     x0, [sp, #ApNormalContext]
    ldr     x1, [sp, #ApSystemArgument1]
    ldr     x2, [sp, #ApSystemArgument2]
    add     x3, sp, #ApContext
    blr     x16

    add     x0, sp, #ApContext
    ldr     w1, [sp, #ApTestAlert]
    and     w1, w1, #1
    bl      ZwContinue

    /* Only a failure comes back */
    bl      RtlRaiseStatus
    brk     #0xF000
LABEL(KiUserApcDispatcherEnd)
    NESTED_END KiUserApcDispatcherUnwind

    XDATAAREA
LABEL(KiUserApcDispatcherXdata)
    /* Two code words, no epilog */
    dcd     ((KiUserApcDispatcherEnd - KiUserApcDispatcherUnwind) / 4) + 0x10000000
    dcb     0xE3, 0xE3, 0xE3            // nop
    dcb     0x03                        // alloc_s 0x30
    dcb     0xEA                        // context
    dcb     0xE4                        // end
    dcb     0xE3, 0xE3                  // padding

    PDATAAREA
    DCRVA   KiUserApcDispatcherUnwind
    DCRVA   KiUserApcDispatcherXdata

    END
