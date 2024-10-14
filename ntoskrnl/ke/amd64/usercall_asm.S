/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ReactOS AMD64 user mode callback helper
 * COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <ksamd64.inc>

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
