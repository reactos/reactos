
#include <asm.inc>

#include <ksamd64.inc>

.code64

EXTERN KdbEnterDebuggerException : PROC

PUBLIC KdbEnter
KdbEnter:

    /* save flags */
    pushfq
//  .pushreg ?

    /* Make room for a KTRAP_FRAME */
    sub rsp, KTRAP_FRAME_LENGTH
//    .allocstack KTRAP_FRAME_LENGTH

    /* Save rbp */
    mov [rsp + KTRAP_FRAME_Rbp], rbp

    /* Save non-volatile registers */
    mov [rsp + KTRAP_FRAME_Rbx], rbx
    mov [rsp + KTRAP_FRAME_Rdi], rdi
    mov [rsp + KTRAP_FRAME_Rsi], rsi

    /* Save volatile registers */
    mov [rsp + KTRAP_FRAME_Rax], rax
    mov [rsp + KTRAP_FRAME_Rcx], rcx
    mov [rsp + KTRAP_FRAME_Rdx], rdx
    mov [rsp + KTRAP_FRAME_R8], r8
    mov [rsp + KTRAP_FRAME_R9], r9
    mov [rsp + KTRAP_FRAME_R10], r10
    mov [rsp + KTRAP_FRAME_R11], r11

    /* Save xmm registers */
    movdqa [rsp + KTRAP_FRAME_Xmm0], xmm0
    movdqa [rsp + KTRAP_FRAME_Xmm1], xmm1
    movdqa [rsp + KTRAP_FRAME_Xmm2], xmm2
    movdqa [rsp + KTRAP_FRAME_Xmm3], xmm3
    movdqa [rsp + KTRAP_FRAME_Xmm4], xmm4
    movdqa [rsp + KTRAP_FRAME_Xmm5], xmm5

    /* Save cs and previous mode */
    mov ax, cs
    mov [rsp + KTRAP_FRAME_SegCs], ax
    and ax, 1
    mov [rsp + KTRAP_FRAME_PreviousMode], al

    /* Save segment selectors */
    mov ax, ds
    mov [rsp + KTRAP_FRAME_SegDs], ax
    mov ax, es
    mov [rsp + KTRAP_FRAME_SegEs], ax
    mov ax, fs
    mov [rsp + KTRAP_FRAME_SegFs], ax
    mov ax, gs
    mov [rsp + KTRAP_FRAME_SegGs], ax

    /* Save previous irql */
    mov rax, cr8
    mov [rsp + KTRAP_FRAME_PreviousIrql], al

    /* Save debug registers */
    mov rax, dr0
    mov [rsp + KTRAP_FRAME_Dr0], rax
    mov rax, dr1
    mov [rsp + KTRAP_FRAME_Dr1], rax
    mov rax, dr2
    mov [rsp + KTRAP_FRAME_Dr2], rax
    mov rax, dr3
    mov [rsp + KTRAP_FRAME_Dr3], rax
    mov rax, dr6
    mov [rsp + KTRAP_FRAME_Dr6], rax
    mov rax, dr7
    mov [rsp + KTRAP_FRAME_Dr7], rax

    /* Point rbp, where rsp was before */
    lea rbp, [rsp + KTRAP_FRAME_LENGTH]
    mov [rsp + KTRAP_FRAME_Rsp], rbp

    /* Store the EFLAGS we previously pushed on the stack */
    mov rax, [rbp + 8]
    mov [rsp + KTRAP_FRAME_EFlags], rax

    /* Get RIP from the stack */
    mov rax, [rbp + 16]
    mov [rsp + KTRAP_FRAME_Rip], rax

    /* Make sure the direction flag is cleared */
    cld

    /* Clear all breakpoint enables in dr7. */
    mov rax, dr7
    and rax, HEX(0FFFF0000)
    mov dr7, rax

    /* Call KDB */
    mov byte ptr [rsp + KTRAP_FRAME_P5], 1 /* FirstChance */
    mov r9, rsp                /* Pointer to the trap frame */
    mov r8, 0                  /* Context */
    mov dl, 0                  /* PreviousMode (KernelMode) */
    mov rcx, 0                 /* ExceptionRecord */
    call KdbEnterDebuggerException

    /* Restore segment selectors */
    mov ax, [rsp + KTRAP_FRAME_SegDs]
    mov ds, ax
    mov ax, [rsp + KTRAP_FRAME_SegEs]
    mov es, ax
    mov ax, [rsp + KTRAP_FRAME_SegFs]
    mov fs, ax

    /* Restore non-volatile registers */
    mov rbx, [rsp + KTRAP_FRAME_Rbx]
    mov rdi, [rsp + KTRAP_FRAME_Rdi]
    mov rsi, [rsp + KTRAP_FRAME_Rsi]

    /* Restore volatile registers */
    mov rax, [rsp + KTRAP_FRAME_Rax]
    mov rcx, [rsp + KTRAP_FRAME_Rcx]
    mov rdx, [rsp + KTRAP_FRAME_Rdx]
    mov r8, [rsp + KTRAP_FRAME_R8]
    mov r9, [rsp + KTRAP_FRAME_R9]
    mov r10, [rsp + KTRAP_FRAME_R10]
    mov r11, [rsp + KTRAP_FRAME_R11]

    /* Restore RSP */
    mov rsp, [rsp + KTRAP_FRAME_Rsp]

    /* Restore EFLAGS */
    popfq

    ret

PUBLIC KdbpStackSwitchAndCall
KdbpStackSwitchAndCall:

    /* Save old stack */
    mov rax, rsp

    /* Set new stack */
    mov rsp, rcx

    /* Save old stack on new stack */
    push rax

    /* Call function */
    call rdx

    /* Restire old stack */
    pop rax
    mov rsp, rax

    /* Return */
    ret

END