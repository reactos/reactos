/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Call an exception filter funclet with the frame pointer set up
 * COPYRIGHT:   Copyright 2026 Yausen <yuyi2439@qq.com>
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/

.code64

/*
 * LONG
 * __C_specific_handler_call_filter(
 *     _In_ PVOID Filter,             // rcx
 *     _In_ PVOID Argument,           // rdx
 *     _In_ PVOID EstablisherFrame);  // r8
 *
 * PSEH2 filter funclets are labels inside the guarded function, so they expect
 * RBP to hold the frame pointer of that function (the establisher frame). GCC
 * may emit frame relative spills at the filter entry, which would write to a
 * random address when RBP does not point to the function's frame. Set RBP up
 * for the duration of the call and restore the caller's value afterwards.
 */
PUBLIC __C_specific_handler_call_filter
.PROC __C_specific_handler_call_filter

    /* Save the caller's frame pointer */
    push rbp
    .pushreg rbp

    /* Reserve the home space for the callee */
    sub rsp, 32
    .allocstack 32
    .endprolog

    /* Move the filter address out of the way */
    mov r9, rcx

    /* Set up the arguments for the filter: (Argument, EstablisherFrame) */
    mov rcx, rdx
    mov rdx, r8

    /* The filter funclet expects RBP to be the establisher frame */
    mov rbp, r8

    /* Call the filter */
    call r9

    /* Restore the frame pointer and return */
    add rsp, 32
    pop rbp
    ret

.ENDP

END
