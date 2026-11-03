/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 context capture and restore
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

    TEXTAREA

//
// VOID
// RtlCaptureContext(
//     _Out_ PCONTEXT ContextRecord);
//
// The context describes the caller right after the call: Pc is the
// return address, x0 and lr hold nothing useful and are stored as 0.
//
    LEAF_ENTRY RtlCaptureContext

    str     xzr, [x0, #CxX0]
    stp     x1, x2, [x0, #CxX1]
    stp     x3, x4, [x0, #CxX3]
    stp     x5, x6, [x0, #CxX5]
    stp     x7, x8, [x0, #CxX7]
    stp     x9, x10, [x0, #CxX9]
    stp     x11, x12, [x0, #CxX11]
    stp     x13, x14, [x0, #CxX13]
    stp     x15, x16, [x0, #CxX15]
    stp     x17, x18, [x0, #CxX17]
    stp     x19, x20, [x0, #CxX19]
    stp     x21, x22, [x0, #CxX21]
    stp     x23, x24, [x0, #CxX23]
    stp     x25, x26, [x0, #CxX25]
    stp     x27, x28, [x0, #CxX27]
    stp     x29, xzr, [x0, #CxFp]
    mov     x16, sp
    stp     x16, x30, [x0, #CxSp]

    mrs     x16, nzcv
    str     w16, [x0, #CxCpsr]
    mrs     x16, fpcr
    mrs     x17, fpsr
    str     w16, [x0, #CxFpcr]
    str     w17, [x0, #CxFpsr]

    add     x16, x0, #CxV
    stp     q0, q1, [x16, #0x000]
    stp     q2, q3, [x16, #0x020]
    stp     q4, q5, [x16, #0x040]
    stp     q6, q7, [x16, #0x060]
    stp     q8, q9, [x16, #0x080]
    stp     q10, q11, [x16, #0x0A0]
    stp     q12, q13, [x16, #0x0C0]
    stp     q14, q15, [x16, #0x0E0]
    stp     q16, q17, [x16, #0x100]
    stp     q18, q19, [x16, #0x120]
    stp     q20, q21, [x16, #0x140]
    stp     q22, q23, [x16, #0x160]
    stp     q24, q25, [x16, #0x180]
    stp     q26, q27, [x16, #0x1A0]
    stp     q28, q29, [x16, #0x1C0]
    stp     q30, q31, [x16, #0x1E0]

    ldr     w16, =CONTEXT_FULL
    str     w16, [x0, #CxContextFlags]
    ret

    LEAF_END RtlCaptureContext

//
// DECLSPEC_NORETURN
// VOID
// RtlpRestoreContextInternal(
//     _In_ PCONTEXT ContextRecord);
//
// Everything is loaded before sp moves, so the context may live below the
// new stack pointer. x16 and x17 carry the new pc and sp, which only call
// sites can afford, and x18 keeps pointing at the current TEB or PCR.
//
    LEAF_ENTRY RtlpRestoreContextInternal

    add     x16, x0, #CxV
    ldp     q0, q1, [x16, #0x000]
    ldp     q2, q3, [x16, #0x020]
    ldp     q4, q5, [x16, #0x040]
    ldp     q6, q7, [x16, #0x060]
    ldp     q8, q9, [x16, #0x080]
    ldp     q10, q11, [x16, #0x0A0]
    ldp     q12, q13, [x16, #0x0C0]
    ldp     q14, q15, [x16, #0x0E0]
    ldp     q16, q17, [x16, #0x100]
    ldp     q18, q19, [x16, #0x120]
    ldp     q20, q21, [x16, #0x140]
    ldp     q22, q23, [x16, #0x160]
    ldp     q24, q25, [x16, #0x180]
    ldp     q26, q27, [x16, #0x1A0]
    ldp     q28, q29, [x16, #0x1C0]
    ldp     q30, q31, [x16, #0x1E0]

    ldr     w16, [x0, #CxFpcr]
    msr     fpcr, x16
    ldr     w16, [x0, #CxFpsr]
    msr     fpsr, x16
    ldr     w16, [x0, #CxCpsr]
    msr     nzcv, x16

    ldp     x2, x3, [x0, #CxX2]
    ldp     x4, x5, [x0, #CxX4]
    ldp     x6, x7, [x0, #CxX6]
    ldp     x8, x9, [x0, #CxX8]
    ldp     x10, x11, [x0, #CxX10]
    ldp     x12, x13, [x0, #CxX12]
    ldp     x14, x15, [x0, #CxX14]
    ldp     x19, x20, [x0, #CxX19]
    ldp     x21, x22, [x0, #CxX21]
    ldp     x23, x24, [x0, #CxX23]
    ldp     x25, x26, [x0, #CxX25]
    ldp     x27, x28, [x0, #CxX27]
    ldp     x29, x30, [x0, #CxFp]

    ldr     x16, [x0, #CxPc]
    ldr     x17, [x0, #CxSp]
    ldp     x0, x1, [x0, #CxX0]
    mov     sp, x17
    br      x16

    LEAF_END RtlpRestoreContextInternal

    END
