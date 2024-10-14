/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ReactOS AMD64 user mode callback helper
 * COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <ksamd64.inc>
#include <trapamd64.inc>

EXTERN KiInitiateUserApc:PROC
EXTERN KeUserCallbackDispatcher:QWORD

/*
 * NTSTATUS
 * KiUserModeCallout (
 *     _Inout_ PKCALLOUT_FRAME CalloutFrame);
 */
EXTERN KiUserModeCallout:PROC

.code64

/*
 * NTSTATUS
 * KiCallUserMode (
 *     _In_ PVOID *OutputBuffer@<rcx>,
 *     _In_ PULONG OutputLength@<rdx>);
 */
PUBLIC KiCallUserMode
.PROC KiCallUserMode

    /* Generate a KEXCEPTION_FRAME on the stack */
    /* This is identical to a KCALLOUT_FRAME */
    GENERATE_EXCEPTION_FRAME

    /* Save OutputBuffer and OutputLength */
    mov [rsp + ExOutputBuffer], rcx
    mov [rsp + ExOutputLength], rdx

    /* Call the C function */
    mov rcx, rsp
    call KiUserModeCallout

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret

.ENDP


/*!
 * \brief Exits to user mode, restores only rsp and rip, zeroes the rest.
 *
 * \todo Merge this with KiCallUserMode + KiUserModeCallout and skip the trap frame.
 *
 * DECLSPEC_NORETURN
 * VOID
 * KiUserCallbackExit(
 *     _In_ PKTRAP_FRAME TrapFrame@<rcx>);
 */
 PUBLIC KiUserCallbackExit
KiUserCallbackExit:

    /* Point rsp to the trap frame */
    mov rsp, rcx

    /* Zero non-volatile registers for KiUserCallbackDispatcher.
       This must be done before dispatching a pending user APC. */
    xor rbx, rbx
    xor rdi, rdi
    xor rsi, rsi
    xor rbp, rbp
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    pxor xmm6, xmm6
    pxor xmm7, xmm7
    pxor xmm8, xmm8
    pxor xmm9, xmm9
    pxor xmm10, xmm10
    pxor xmm11, xmm11
    pxor xmm12, xmm12
    pxor xmm13, xmm13
    pxor xmm14, xmm14
    pxor xmm15, xmm15

    /* Check for pending user APC */
    mov rdx, gs:[PcCurrentThread]
    HANDLE_USER_APCS rdx, rsp

    /* Zero volatile registers */
    xor rax, rax
    xor rdx, rdx
    xor r8, r8
    xor r9, r9
    xor r10, r10
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5

    /* Disable interrupts for return */
    cli

    /* Prepare user mode return address (rcx) and eflags (r11) for sysret */
    mov rcx, qword ptr KeUserCallbackDispatcher[rip]
    mov r11, EFLAGS_IF_MASK

    /* Load user mode stack (It was copied to the trap frame) */
    mov rsp, [rsp + KTRAP_FRAME_Rsp]

    /* Swap gs back to user */
    swapgs

    /* return to user mode */
    sysretq


/*
 * DECLSPEC_NORETURN
 * VOID
 * KiCallbackReturn (
 *     _In_ PVOID Stack,
 *     _In_ NTSTATUS Status);
 */
PUBLIC KiCallbackReturn
.PROC KiCallbackReturn

    .ENDPROLOG

    /* Restore the stack */
    mov rsp, rcx

    /* Set return status */
    mov eax, edx

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret

.ENDP


END
