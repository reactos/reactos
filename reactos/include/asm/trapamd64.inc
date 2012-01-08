
APIC_EOI = HEX(0FFFFFFFFFFFE00B0)

TF_VOLATILES            = HEX(01)
TF_NONVOLATILES         = HEX(02)
TF_XMM                  = HEX(04)
TF_SEGMENTS             = HEX(08)
TF_DEBUG                = HEX(10)
TF_IRQL                 = HEX(20)
TF_SAVE_ALL             = (TF_VOLATILES OR TF_NONVOLATILES OR TF_XMM OR TF_SEGMENTS)
TF_HAS_ERROR_CODE       = HEX(40)
TF_SEND_EOI             = HEX(80)
//TF_SYSTEMSERVICE        = (TRAPFLAG_VOLATILES or TRAPFLAG_DEBUG)

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
    LOCAL dont_swap

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
    .endprolog

    /* Save rbp and rax */
    mov [rsp + KTRAP_FRAME_Rbp], rbp
    mov [rsp + KTRAP_FRAME_Rax], rax

    /* Point rbp to the KTRAP_FRAME */
    lea rbp, [rsp]

    if (Flags AND TF_NONVOLATILES)
        /* Save non-volatile registers */
        mov [rbp + KTRAP_FRAME_Rbx], rbx
        mov [rbp + KTRAP_FRAME_Rdi], rdi
        mov [rbp + KTRAP_FRAME_Rsi], rsi
    endif

    if (Flags AND TF_VOLATILES)
        /* Save volatile registers */
        mov [rbp + KTRAP_FRAME_Rcx], rcx
        mov [rbp + KTRAP_FRAME_Rdx], rdx
        mov [rbp + KTRAP_FRAME_R8], r8
        mov [rbp + KTRAP_FRAME_R9], r9
        mov [rbp + KTRAP_FRAME_R10], r10
        mov [rbp + KTRAP_FRAME_R11], r11
    endif

    if (Flags AND TF_XMM)
        /* Save xmm registers */
        movdqa [rbp + KTRAP_FRAME_Xmm0], xmm0
        movdqa [rbp + KTRAP_FRAME_Xmm1], xmm1
        movdqa [rbp + KTRAP_FRAME_Xmm2], xmm2
        movdqa [rbp + KTRAP_FRAME_Xmm3], xmm3
        movdqa [rbp + KTRAP_FRAME_Xmm4], xmm4
        movdqa [rbp + KTRAP_FRAME_Xmm5], xmm5
    endif

    if (Flags AND TF_SEGMENTS)
        /* Save segment selectors */
        mov [rbp + KTRAP_FRAME_SegDs], ds
        mov [rbp + KTRAP_FRAME_SegEs], es
        mov [rbp + KTRAP_FRAME_SegFs], fs
        mov [rbp + KTRAP_FRAME_SegGs], gs
    endif

    /* Save previous mode and swap gs when it was UserMode */
    mov ax, [rbp + KTRAP_FRAME_SegCs]
    and al, 1
    mov [rbp + KTRAP_FRAME_PreviousMode], al
    jz dont_swap
    swapgs
dont_swap:

    if (Flags AND TF_IRQL)
        /* Save previous irql */
        mov rax, cr8
        mov [rbp + KTRAP_FRAME_PreviousIrql], al
    endif

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
    LOCAL dont_swap_back

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

    test byte ptr [rbp + KTRAP_FRAME_PreviousMode], 1
    jz dont_swap_back
    swapgs
dont_swap_back:

    if (Flags AND TF_NONVOLATILES)
        /* Restore non-volatile registers */
        mov rbx, [rbp + KTRAP_FRAME_Rbx]
        mov rdi, [rbp + KTRAP_FRAME_Rdi]
        mov rsi, [rbp + KTRAP_FRAME_Rsi]
    endif

    if (Flags AND TF_VOLATILES)
        /* Restore volatile registers */
        mov rax, [rbp + KTRAP_FRAME_Rax]
        mov rcx, [rbp + KTRAP_FRAME_Rcx]
        mov rdx, [rbp + KTRAP_FRAME_Rdx]
        mov r8, [rbp + KTRAP_FRAME_R8]
        mov r9, [rbp + KTRAP_FRAME_R9]
        mov r10, [rbp + KTRAP_FRAME_R10]
        mov r11, [rbp + KTRAP_FRAME_R11]
    endif

    if (Flags AND TF_XMM)
        /* Restore xmm registers */
        movdqa xmm0, [rbp + KTRAP_FRAME_Xmm0]
        movdqa xmm1, [rbp + KTRAP_FRAME_Xmm1]
        movdqa xmm2, [rbp + KTRAP_FRAME_Xmm2]
        movdqa xmm3, [rbp + KTRAP_FRAME_Xmm3]
        movdqa xmm4, [rbp + KTRAP_FRAME_Xmm4]
        movdqa xmm5, [rbp + KTRAP_FRAME_Xmm5]
    endif

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
    ENDFUNC &Trap
ENDM

