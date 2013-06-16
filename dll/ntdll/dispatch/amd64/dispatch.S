/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntdll/dispatch/amd64/dispatch.S
 * PURPOSE:         Usermode dispatcher stubs
 *
 * PROGRAMMER:      Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

EXTERN NtContinue:PROC
EXTERN LdrpInit:PROC

.code

PUBLIC LdrInitializeThunk
.PROC LdrInitializeThunk
    mov rbp, 0
    .setframe rbp, 0
    .endprolog

    /* First parameter is the APC context */
    mov rcx, r9
    jmp LdrpInit

.ENDP

PUBLIC KiUserApcDispatcher
.PROC KiUserApcDispatcher
    .endprolog
    /* We enter with a 16 byte aligned stack */

    mov rcx, [rsp + CONTEXT_P1Home] /* NormalContext */
    mov rdx, [rsp + CONTEXT_P2Home] /* SystemArgument1 */
    mov r8, [rsp + CONTEXT_P3Home]  /* SystemArgument2 */
    lea r9, [rsp]                   /* Context */
    call qword ptr [rsp + CONTEXT_P4Home] /* NormalRoutine */

    /* NtContinue(Context, TRUE); */
    lea rcx, [rsp]
    mov dl, 1
    call NtContinue

    nop
    int 3
.ENDP


PUBLIC KiRaiseUserExceptionDispatcher
.PROC KiRaiseUserExceptionDispatcher
    .endprolog
    int 3

.ENDP

PUBLIC KiUserCallbackDispatcher
.PROC KiUserCallbackDispatcher
    .endprolog
    int 3

.ENDP

PUBLIC KiUserExceptionDispatcher
.PROC KiUserExceptionDispatcher
    .endprolog
    int 3

.ENDP

END

