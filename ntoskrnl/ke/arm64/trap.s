/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 exception vectors, trap entry and trap exit
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

/*
 * Every trap builds the same stack, from high to low addresses:
 *   KTRAP_FRAME, KARM64_VFP_STATE, KEXCEPTION_FRAME
 * A trap from user mode starts at the thread's initial stack, so the trap
 * frame ends up where KeGetTrapFrame expects it.
 */
#define TRAP_FRAME_OFFSET   (KEXCEPTION_FRAME_LENGTH + KARM64_VFP_STATE_LENGTH)

/* Trap types for KiArm64TrapDispatch */
#define KI_TRAP_SYNCHRONOUS 0
#define KI_TRAP_IRQ         1
#define KI_TRAP_FIQ         2
#define KI_TRAP_SERROR      3

/* ESR_EL1 for an SVC, the service number goes in the low bits */
#define ESR_SVC64_HIGH      0x5600

    IMPORT KiArm64TrapDispatch

//
// The vector table needs 2 KB alignment, every entry is 128 bytes.
//
#ifdef _MSC_VER
    AREA |.text$vec|,ALIGN=11,CODE,READONLY
#else
    .section .text$vec, "xr"
    .p2align 11
#endif

    LEAF_ENTRY KiArm64ExceptionVectors

    /* Current EL with SP_EL0, never used by the kernel */
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector

    /* Current EL with SP_EL1 */
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_SYNCHRONOUS
    b       KiArm64KernelTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_IRQ
    b       KiArm64KernelTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_FIQ
    b       KiArm64KernelTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_SERROR
    b       KiArm64KernelTrap

    /* Lower EL running AArch64 */
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_SYNCHRONOUS
    b       KiArm64UserTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_IRQ
    b       KiArm64UserTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_FIQ
    b       KiArm64UserTrap
    ALIGN   128
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    mov     w0, #KI_TRAP_SERROR
    b       KiArm64UserTrap

    /* Lower EL running AArch32, not supported */
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128
    b       KiArm64BadVector
    ALIGN   128

    LEAF_END KiArm64ExceptionVectors

    TEXTAREA

//
// Nothing sane can be saved from these vectors, park the processor.
//
    LEAF_ENTRY KiArm64BadVector
    msr     daifset, #0xF
LABEL(KiArm64BadVectorLoop)
    wfi
    b       KiArm64BadVectorLoop
    LEAF_END KiArm64BadVector

//
// Trap from EL1. x0 and x1 are saved, w0 holds the trap type.
//
    LEAF_ENTRY KiArm64KernelTrap
    stp     x2, x3, [sp, #TrX2]
    stp     x4, x5, [sp, #TrX4]
    stp     x6, x7, [sp, #TrX6]
    stp     x8, x9, [sp, #TrX8]
    stp     x10, x11, [sp, #TrX10]
    stp     x12, x13, [sp, #TrX12]
    stp     x14, x15, [sp, #TrX14]
    stp     x16, x17, [sp, #TrX16]
    str     x18, [sp, #TrX18]
    stp     x30, x29, [sp, #TrLr]

    add     x1, sp, #KTRAP_FRAME_LENGTH
    str     x1, [sp, #TrSp]

    /* KernelMode, the rest of the header starts clear */
    str     xzr, [sp]
    b       KiArm64TrapSystemRegisters
    LEAF_END KiArm64KernelTrap

//
// Trap from EL0. x0 and x1 are saved, w0 holds the trap type.
//
    LEAF_ENTRY KiArm64UserTrap
    stp     x2, x3, [sp, #TrX2]
    stp     x4, x5, [sp, #TrX4]
    stp     x6, x7, [sp, #TrX6]
    stp     x8, x9, [sp, #TrX8]
    stp     x10, x11, [sp, #TrX10]
    stp     x12, x13, [sp, #TrX12]
    stp     x14, x15, [sp, #TrX14]
    stp     x16, x17, [sp, #TrX16]
    str     x18, [sp, #TrX18]
    stp     x30, x29, [sp, #TrLr]

    mrs     x1, sp_el0
    str     x1, [sp, #TrSp]

    str     xzr, [sp]
    mov     w1, #1
    strb    w1, [sp, #TrPreviousMode]

    /* x18 holds the TEB in user mode and the PCR in kernel mode */
    mrs     x18, tpidr_el1
    b       KiArm64TrapSystemRegisters
    LEAF_END KiArm64UserTrap

    LEAF_ENTRY KiArm64TrapSystemRegisters
    mrs     x1, elr_el1
    str     x1, [sp, #TrPc]
    mrs     x1, spsr_el1
    str     w1, [sp, #TrSpsr]
    mrs     x1, esr_el1
    str     w1, [sp, #TrEsr]
    mrs     x1, far_el1
    str     x1, [sp, #TrFaultAddress]
    b       KiArm64TrapSave
    LEAF_END KiArm64TrapSystemRegisters

//
// Saves the floating point state and the nonvolatile registers, then calls
// the dispatcher. sp points to a complete trap frame and w0 is the trap type.
// The unwind codes describe this frame, so exceptions raised by the
// dispatcher unwind into the interrupted code.
//
    NESTED_ENTRY KiArm64TrapSave
    sub     sp, sp, #KARM64_VFP_STATE_LENGTH
    stp     q0, q1, [sp, #(VfV + 0x000)]
    stp     q2, q3, [sp, #(VfV + 0x020)]
    stp     q4, q5, [sp, #(VfV + 0x040)]
    stp     q6, q7, [sp, #(VfV + 0x060)]
    stp     q8, q9, [sp, #(VfV + 0x080)]
    stp     q10, q11, [sp, #(VfV + 0x0A0)]
    stp     q12, q13, [sp, #(VfV + 0x0C0)]
    stp     q14, q15, [sp, #(VfV + 0x0E0)]
    stp     q16, q17, [sp, #(VfV + 0x100)]
    stp     q18, q19, [sp, #(VfV + 0x120)]
    stp     q20, q21, [sp, #(VfV + 0x140)]
    stp     q22, q23, [sp, #(VfV + 0x160)]
    stp     q24, q25, [sp, #(VfV + 0x180)]
    stp     q26, q27, [sp, #(VfV + 0x1A0)]
    stp     q28, q29, [sp, #(VfV + 0x1C0)]
    stp     q30, q31, [sp, #(VfV + 0x1E0)]
    mrs     x1, fpcr
    str     w1, [sp, #VfFpcr]
    mrs     x1, fpsr
    str     w1, [sp, #VfFpsr]
    str     xzr, [sp]
    mov     x1, sp
    str     x1, [sp, #(KARM64_VFP_STATE_LENGTH + TrVfpState)]

    sub     sp, sp, #KEXCEPTION_FRAME_LENGTH
    stp     x19, x20, [sp, #ExX19]
    stp     x21, x22, [sp, #ExX21]
    stp     x23, x24, [sp, #ExX23]
    stp     x25, x26, [sp, #ExX25]
    stp     x27, x28, [sp, #ExX27]
    stp     x29, x30, [sp, #ExFp]

    mov     w2, w0
    add     x0, sp, #TRAP_FRAME_OFFSET
    mov     x1, sp
    bl      KiArm64TrapDispatch

    /* Keeps the return address inside the described range */
    nop
    b       KiArm64TrapExit
LABEL(KiArm64TrapSaveEnd)
    NESTED_END KiArm64TrapSave

//
// Restores everything a trap saved and returns to the interrupted code.
// sp points to the exception frame.
//
    LEAF_ENTRY KiArm64TrapExit
    msr     daifset, #3
    ldp     x19, x20, [sp, #ExX19]
    ldp     x21, x22, [sp, #ExX21]
    ldp     x23, x24, [sp, #ExX23]
    ldp     x25, x26, [sp, #ExX25]
    ldp     x27, x28, [sp, #ExX27]
    add     sp, sp, #KEXCEPTION_FRAME_LENGTH

    /* sp points to the floating point state */
    ALTERNATE_ENTRY KiArm64VfpTrapExit
    msr     daifset, #3
    ldr     w1, [sp, #VfFpcr]
    msr     fpcr, x1
    ldr     w1, [sp, #VfFpsr]
    msr     fpsr, x1
    ldp     q0, q1, [sp, #(VfV + 0x000)]
    ldp     q2, q3, [sp, #(VfV + 0x020)]
    ldp     q4, q5, [sp, #(VfV + 0x040)]
    ldp     q6, q7, [sp, #(VfV + 0x060)]
    ldp     q8, q9, [sp, #(VfV + 0x080)]
    ldp     q10, q11, [sp, #(VfV + 0x0A0)]
    ldp     q12, q13, [sp, #(VfV + 0x0C0)]
    ldp     q14, q15, [sp, #(VfV + 0x0E0)]
    ldp     q16, q17, [sp, #(VfV + 0x100)]
    ldp     q18, q19, [sp, #(VfV + 0x120)]
    ldp     q20, q21, [sp, #(VfV + 0x140)]
    ldp     q22, q23, [sp, #(VfV + 0x160)]
    ldp     q24, q25, [sp, #(VfV + 0x180)]
    ldp     q26, q27, [sp, #(VfV + 0x1A0)]
    ldp     q28, q29, [sp, #(VfV + 0x1C0)]
    ldp     q30, q31, [sp, #(VfV + 0x1E0)]
    add     sp, sp, #KARM64_VFP_STATE_LENGTH

    /* sp points to the trap frame */
    ldr     x1, [sp, #TrPc]
    msr     elr_el1, x1
    ldr     w1, [sp, #TrSpsr]
    msr     spsr_el1, x1
    tst     w1, #0xF
    b.ne    KiArm64ReturnToKernel

    ldr     x1, [sp, #TrSp]
    msr     sp_el0, x1
    ldp     x0, x1, [sp, #TrX0]
    ldp     x2, x3, [sp, #TrX2]
    ldp     x4, x5, [sp, #TrX4]
    ldp     x6, x7, [sp, #TrX6]
    ldp     x8, x9, [sp, #TrX8]
    ldp     x10, x11, [sp, #TrX10]
    ldp     x12, x13, [sp, #TrX12]
    ldp     x14, x15, [sp, #TrX14]
    ldp     x16, x17, [sp, #TrX16]
    ldr     x18, [sp, #TrX18]
    ldp     x30, x29, [sp, #TrLr]
    add     sp, sp, #KTRAP_FRAME_LENGTH
    eret

LABEL(KiArm64ReturnToKernel)
    /* A changed context can move the stack, take the frame along */
    ldr     x2, [sp, #TrSp]
    add     x3, sp, #KTRAP_FRAME_LENGTH
    cmp     x2, x3
    b.eq    KiArm64ReturnToKernelRestore

    sub     x2, x2, #KTRAP_FRAME_LENGTH
    mov     x3, sp
    mov     x4, #KTRAP_FRAME_LENGTH
    cmp     x2, x3
    b.hi    KiArm64MoveFrameDown

LABEL(KiArm64MoveFrameUp)
    ldr     x5, [x3], #8
    str     x5, [x2], #8
    subs    x4, x4, #8
    b.ne    KiArm64MoveFrameUp
    sub     x2, x2, #KTRAP_FRAME_LENGTH
    b       KiArm64MoveFrameDone

LABEL(KiArm64MoveFrameDown)
    add     x2, x2, x4
    add     x3, x3, x4
LABEL(KiArm64MoveFrameDownLoop)
    ldr     x5, [x3, #-8]!
    str     x5, [x2, #-8]!
    subs    x4, x4, #8
    b.ne    KiArm64MoveFrameDownLoop

LABEL(KiArm64MoveFrameDone)
    mov     sp, x2

LABEL(KiArm64ReturnToKernelRestore)
    ldp     x0, x1, [sp, #TrX0]
    ldp     x2, x3, [sp, #TrX2]
    ldp     x4, x5, [sp, #TrX4]
    ldp     x6, x7, [sp, #TrX6]
    ldp     x8, x9, [sp, #TrX8]
    ldp     x10, x11, [sp, #TrX10]
    ldp     x12, x13, [sp, #TrX12]
    ldp     x14, x15, [sp, #TrX14]
    ldp     x16, x17, [sp, #TrX16]
    ldr     x18, [sp, #TrX18]
    ldp     x30, x29, [sp, #TrLr]
    add     sp, sp, #KTRAP_FRAME_LENGTH
    eret
    LEAF_END KiArm64TrapExit

//
// DECLSPEC_NORETURN
// VOID
// KiExceptionExit(
//     _In_ PKTRAP_FRAME TrapFrame,
//     _In_ PKEXCEPTION_FRAME ExceptionFrame);
//
// The nonvolatile registers come from the given exception frame, which may
// live anywhere on the stack, the rest from the trap and floating point frames.
//
    LEAF_ENTRY KiExceptionExit
    msr     daifset, #3
    ldp     x19, x20, [x1, #ExX19]
    ldp     x21, x22, [x1, #ExX21]
    ldp     x23, x24, [x1, #ExX23]
    ldp     x25, x26, [x1, #ExX25]
    ldp     x27, x28, [x1, #ExX27]
    ldr     x2, [x0, #TrVfpState]
    mov     sp, x2
    b       KiArm64VfpTrapExit
    LEAF_END KiExceptionExit

//
// System service for kernel mode callers, the Zw stubs branch here with the
// service number in x16. It builds the frame an SVC would have built.
//
    LEAF_ENTRY KiZwSystemService
    sub     sp, sp, #KTRAP_FRAME_LENGTH
    stp     x0, x1, [sp, #TrX0]
    stp     x2, x3, [sp, #TrX2]
    stp     x4, x5, [sp, #TrX4]
    stp     x6, x7, [sp, #TrX6]
    stp     x8, x9, [sp, #TrX8]
    stp     x10, x11, [sp, #TrX10]
    stp     x12, x13, [sp, #TrX12]
    stp     x14, x15, [sp, #TrX14]
    stp     x16, x17, [sp, #TrX16]
    str     x18, [sp, #TrX18]
    stp     x30, x29, [sp, #TrLr]

    add     x1, sp, #KTRAP_FRAME_LENGTH
    str     x1, [sp, #TrSp]
    str     x30, [sp, #TrPc]
    str     xzr, [sp]
    str     xzr, [sp, #TrFaultAddress]

    /* Return to EL1h with the caller's interrupt state */
    mrs     x1, daif
    mov     x2, #5
    orr     x1, x1, x2
    str     w1, [sp, #TrSpsr]

    mov     w1, #ESR_SVC64_HIGH
    lsl     w1, w1, #16
    and     w2, w16, #0xFFFF
    orr     w1, w1, w2
    str     w1, [sp, #TrEsr]

    msr     daifset, #3
    mov     w0, #KI_TRAP_SYNCHRONOUS
    b       KiArm64TrapSave
    LEAF_END KiZwSystemService

    XDATAAREA
LABEL(KiArm64TrapSaveXdata)
    /* No epilog, four code words */
    dcd     ((KiArm64TrapSaveEnd - KiArm64TrapSave) / 4) + 0x20000000
    dcb     0xC8, 0x0C                  // save_regp x19, x20, ExX19
    dcb     0xC8, 0x8E                  // save_regp x21, x22, ExX21
    dcb     0xC9, 0x10                  // save_regp x23, x24, ExX23
    dcb     0xC9, 0x92                  // save_regp x25, x26, ExX25
    dcb     0xCA, 0x14                  // save_regp x27, x28, ExX27
    dcb     0xC0, 0x2D                  // alloc_m exception and floating point frames
    dcb     0xE8, 0xE4                  // trap frame, end
    dcb     0xE3, 0xE3                  // padding

    PDATAAREA
    DCRVA   KiArm64TrapSave
    DCRVA   KiArm64TrapSaveXdata

    END
