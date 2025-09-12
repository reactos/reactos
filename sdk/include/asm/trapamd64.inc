/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ASM macros for x64 trap handling
 * COPYRIGHT:   Copyright 2011-2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

MACRO(ASSERT_TRAP_FRAME_INTS_ENABLED, Register)
#if DBG
    LOCAL IntsAreEnabled
    test dword ptr [Register + KTRAP_FRAME_EFlags], HEX(200)
    jnz IntsAreEnabled
    int HEX(2C)
IntsAreEnabled:
#endif
ENDM

MACRO(ASSERT_TRAP_FRAME_IRQL_VALID, Register)
#if DBG
    LOCAL IrqlIsValid
    mov rax, cr8
    cmp byte ptr [Register + KTRAP_FRAME_PreviousIrql], al
    je IrqlIsValid
    int HEX(2C)
IrqlIsValid:
#endif
ENDM

MACRO(ASSERT_IRQL_PASSIVE)
#if DBG
    LOCAL IrqlIsPassive
    mov rax, cr8
    test rax, rax
    jz IrqlIsPassive
    int HEX(2C)
IrqlIsPassive:
#endif
ENDM

// Checks for user APCs and delivers them if necessary.
// Clobbers all volatile registers except rax.
MACRO(HANDLE_USER_APCS, ThreadReg, TrapFrame)
    LOCAL NoUserApcPending

    /* Check for pending user APC */
    cmp byte ptr [ThreadReg + ThApcState + AsUserApcPending], 0
    jz NoUserApcPending
    lea rcx, [TrapFrame]
    call KiInitiateUserApc
NoUserApcPending:
ENDM

APIC_EOI = HEX(0FFFFFFFFFFFE00B0)

TF_SEGMENTS             = HEX(08)
TF_DEBUG                = HEX(10)
TF_IRQL                 = HEX(20)
TF_SAVE_ALL             = (TF_SEGMENTS)
TF_HAS_ERROR_CODE       = HEX(40)
TF_SEND_EOI             = HEX(80)
//TF_SYSTEMSERVICE        = (TRAPFLAG_VOLATILES or TRAPFLAG_DEBUG)
TF_CHECKUSERAPC         = HEX(100)

/*
 * Stack Layout:
 * |-------------------|
 * | KTRAP_FRAME       |
 * |-------------------| <- rbp
 * | EXCEPTION_RECORD  |
 * |-------------------|
 * | KEXCEPTION_FRAME  |
 * |-------------------| <- rsp
 *
 */

/*
 * EnterTrap - Allocate KTRAP_FRAME_LENGTH and save registers to it
 */
MACRO(EnterTrap, Flags)
    LOCAL kernel_mode_entry

    /* Save the trap flags for this trap */
    CurrentTrapFlags = VAL(Flags)

    /* Size of hardware trap frame */
    if (Flags AND TF_HAS_ERROR_CODE)
        .pushframe code
        SIZE_INITIAL_FRAME = 6 * 8
    else
        .pushframe
        SIZE_INITIAL_FRAME = 5 * 8
    endif

    /* Make room for a KTRAP_FRAME */
    sub rsp, (KTRAP_FRAME_LENGTH - SIZE_INITIAL_FRAME)
    .allocstack (KTRAP_FRAME_LENGTH - SIZE_INITIAL_FRAME)

    /* Save rbp */
    mov [rsp + KTRAP_FRAME_Rbp], rbp
    .savereg rbp, KTRAP_FRAME_Rbp

    /* Point rbp to the KTRAP_FRAME */
    lea rbp, [rsp]
    .setframe rbp, 0

    .endprolog

    /* Save volatile registers */
    mov [rbp + KTRAP_FRAME_Rax], rax
    mov [rbp + KTRAP_FRAME_Rcx], rcx
    mov [rbp + KTRAP_FRAME_Rdx], rdx
    mov [rbp + KTRAP_FRAME_R8], r8
    mov [rbp + KTRAP_FRAME_R9], r9
    mov [rbp + KTRAP_FRAME_R10], r10
    mov [rbp + KTRAP_FRAME_R11], r11

    /* Save volatile xmm registers */
    movaps [rbp + KTRAP_FRAME_Xmm0], xmm0
    movaps [rbp + KTRAP_FRAME_Xmm1], xmm1
    movaps [rbp + KTRAP_FRAME_Xmm2], xmm2
    movaps [rbp + KTRAP_FRAME_Xmm3], xmm3
    movaps [rbp + KTRAP_FRAME_Xmm4], xmm4
    movaps [rbp + KTRAP_FRAME_Xmm5], xmm5

    if (Flags AND TF_SEGMENTS)
        /* Save segment selectors */
        mov [rbp + KTRAP_FRAME_SegDs], ds
        mov [rbp + KTRAP_FRAME_SegEs], es
        mov [rbp + KTRAP_FRAME_SegFs], fs
        mov [rbp + KTRAP_FRAME_SegGs], gs
    endif

    /* Save MCXSR */
    stmxcsr [rbp + KTRAP_FRAME_MxCsr]

#if DBG
    mov ecx, MSR_GS_BASE
    rdmsr
    mov [rbp + KTRAP_FRAME_GsBase], eax
    mov [rbp + KTRAP_FRAME_GsBase + 4], edx
#endif

    /* Save previous mode and check if it was user mode */
    mov ax, [rbp + KTRAP_FRAME_SegCs]
    and al, 1
    mov [rbp + KTRAP_FRAME_PreviousMode], al
    jz kernel_mode_entry

    /* Set sane segments */
    mov ax, (KGDT64_R3_DATA or RPL_MASK)
    mov ds, ax
    mov es, ax
    swapgs

    /* Load kernel MXCSR */
    ldmxcsr gs:[PcMxCsr]

    ASSERT_IRQL_PASSIVE

kernel_mode_entry:

//    if (Flags AND TF_IRQL)
        /* Save previous irql */
        mov rax, cr8
        mov [rbp + KTRAP_FRAME_PreviousIrql], al
//    endif

    if (Flags AND TF_DEBUG)
        /* Save debug registers */
        mov rax, dr0
        mov [rbp + KTRAP_FRAME_Dr0], rax
        mov rax, dr1
        mov [rbp + KTRAP_FRAME_Dr1], rax
        mov rax, dr2
        mov [rbp + KTRAP_FRAME_Dr2], rax
        mov rax, dr3
        mov [rbp + KTRAP_FRAME_Dr3], rax
        mov rax, dr6
        mov [rbp + KTRAP_FRAME_Dr6], rax
        mov rax, dr7
        mov [rbp + KTRAP_FRAME_Dr7], rax
    endif

    /* Make sure the direction flag is cleared */
    cld
ENDM

/*
 *  ExitTrap - Restore registers and free stack space
 */
MACRO(ExitTrap, Flags)
    LOCAL kernel_mode_return

    ASSERT_TRAP_FRAME_IRQL_VALID rbp

    if (Flags AND TF_SEGMENTS)
        /* Restore segment selectors */
        mov ds, [rbp + KTRAP_FRAME_SegDs]
        mov es, [rbp + KTRAP_FRAME_SegEs]
        mov fs, [rbp + KTRAP_FRAME_SegFs]
    endif

    if (Flags AND TF_IRQL)
        /* Restore previous irql */
        movzx rax, byte ptr [rbp + KTRAP_FRAME_PreviousIrql]
        mov cr8, rax
    endif

    /* Check if we came from user mode */
    test byte ptr [rbp + KTRAP_FRAME_SegCs], 1
    jz kernel_mode_return

    if (Flags AND TF_CHECKUSERAPC)
        mov r10, gs:[PcCurrentThread]
        HANDLE_USER_APCS r10, rbp
    endif

    ASSERT_TRAP_FRAME_INTS_ENABLED rbp
    ASSERT_IRQL_PASSIVE

    cli

    /* Swap gs to user mode */
    swapgs

kernel_mode_return:

    /* Restore volatile registers */
    mov rax, [rbp + KTRAP_FRAME_Rax]
    mov rcx, [rbp + KTRAP_FRAME_Rcx]
    mov rdx, [rbp + KTRAP_FRAME_Rdx]
    mov r8, [rbp + KTRAP_FRAME_R8]
    mov r9, [rbp + KTRAP_FRAME_R9]
    mov r10, [rbp + KTRAP_FRAME_R10]
    mov r11, [rbp + KTRAP_FRAME_R11]

    /* Restore xmm registers */
    movaps xmm0, [rbp + KTRAP_FRAME_Xmm0]
    movaps xmm1, [rbp + KTRAP_FRAME_Xmm1]
    movaps xmm2, [rbp + KTRAP_FRAME_Xmm2]
    movaps xmm3, [rbp + KTRAP_FRAME_Xmm3]
    movaps xmm4, [rbp + KTRAP_FRAME_Xmm4]
    movaps xmm5, [rbp + KTRAP_FRAME_Xmm5]

    /* Restore MCXSR */
    ldmxcsr [rbp + KTRAP_FRAME_MxCsr]

    /* Restore rbp */
    mov rbp, [rbp + KTRAP_FRAME_Rbp]

    /* Adjust stack pointer */
    add rsp, KTRAP_FRAME_Rip

    if (Flags AND TF_SEND_EOI)
        /* Write 0 to the local APIC EOI register */
        mov dword ptr [APIC_EOI], 0
    endif

    /* Return from the trap */
    iretq
ENDM


MACRO(TRAP_ENTRY, Trap, Flags)
    EXTERN Trap&Handler :PROC
    PUBLIC &Trap
    FUNC &Trap
        /* Common code to create the trap frame */
        EnterTrap Flags

        /* Call the C handler */
        mov rcx, rbp
        call Trap&Handler

        /* Leave */
        ExitTrap Flags
    ENDFUNC
ENDM

