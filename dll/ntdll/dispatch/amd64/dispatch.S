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
EXTERN ZwCallbackReturn:PROC
EXTERN RtlRaiseStatus:PROC

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

    /* The stack is set up with a UCALLOUT_FRAME */
    /* The frame ends with a MACHINE_FRAME. */
    .PUSHFRAME

    /* This is for the Home space, Buffer, Length and ApiNumber */
    .ALLOCSTACK (6 * 8)
    .ENDPROLOG

#if DBG
    /* We enter the function with a fully setup stack, so it must be aligned! */
    test rsp, 15
    jz AlignmentOk
    int HEX(2C)
AlignmentOk:
#endif

    /* Get the parameters from the callout frame */
    mov rcx, [rsp + CkBuffer]
    mov edx, [rsp + CkLength]
    mov r8d, [rsp + CkApiNumber]

    /* Get the callback table */
    mov rax, gs:[TePeb]
    mov r9, [rax + PeKernelCallbackTable]

    /* Call the routine */
    call qword ptr [r9 + r8 * 8]

    /* Return from callback */
    xor ecx, ecx
    xor edx, edx
    mov r8d, eax
    call ZwCallbackReturn

    /* Save callback return value */
    mov esi, eax

    /* Raise status */
StatusRaise:
    mov ecx, esi
    call RtlRaiseStatus
    jmp StatusRaise

.ENDP

/*
    BOOLEAN
    NTAPI
    RtlDispatchException(
        _In_ PEXCEPTION_RECORD ExceptionRecord,
        _In_ PCONTEXT ContextRecord);
 */
EXTERN RtlDispatchException:PROC

/*
    NTSTATUS
    NTAPI
    ZwContinue(
        _In_ PCONTEXT Context,
        _In_ BOOLEAN TestAlert);
 */
EXTERN ZwContinue:PROC

/*
    NTSTATUS
    NTAPI
    ZwRaiseException(
        _In_ PEXCEPTION_RECORD ExceptionRecord,
        _In_ PCONTEXT Context,
        _In_ BOOLEAN SearchFrames);
 */
EXTERN ZwRaiseException:PROC

/*
    VOID
    NTAPI
    RtlRaiseStatus(
        _In_ PEXCEPTION_RECORD ExceptionRecord);
 */
EXTERN RtlRaiseException:PROC

/*
    VOID
    KiUserExceptionDispatcher(
        CONTEXT ContextRecord<rcx>,
        PEXCEPTION_RECORD ExceptionRecord<rdx>);

    This function is called with the following stack layout:
        CONTEXT ContextRecord  <- RSP, RCX
        EXCEPTION_RECORD ExceptionRecord <- RDX
        ULONG64 Alignment
        MACHINE_FRAME MachineFrame
 */
PUBLIC KiUserExceptionDispatcher
.PROC KiUserExceptionDispatcher

    /* The stack is set up with a KUSER_EXCEPTION_STACK */
    /* The frame ends with a MACHINE_FRAME. */
    .PUSHFRAME

    /* This is for the alignment, EXCEPTION_RECORD and CONTEXT */
    .ALLOCSTACK 8 + EXCEPTION_RECORD_LENGTH + CONTEXT_FRAME_LENGTH
    .ENDPROLOG

    /* Clear direction flag */
    cld

    /* Dispatch the exception */
    call RtlDispatchException

    /* Check for success */
    or al, al
    jz RaiseException

    /* We're fine, continue execution */
    lea rcx, [rsp] /* ContextRecord */
    mov dl, 0 /* TestAlert */
    call ZwContinue

    /* Exit */
    jmp Exit

RaiseException:

    /* Raise the exception */
    lea rcx, [rsp + CONTEXT_FRAME_LENGTH] /* ExceptionRecord */
    lea rdx, [rsp] /* ContextRecord */
    xor r8, r8
    call ZwRaiseException

Exit:
    lea rcx, [rsp + CONTEXT_FRAME_LENGTH] /* ExceptionRecord */
    mov rdx, rax
    call KiUserExceptionDispatcherNested
    ret

.ENDP

/*
    VOID
    KiUserExceptionDispatcherNested(
        _In_ ExceptionRecord<rcx>,
        _In_ Status<edx>
    )
 */
.PROC KiUserExceptionDispatcherNested
    /* Allocate space for the nested exception record */
    sub rsp, EXCEPTION_RECORD_LENGTH
    .ALLOCSTACK EXCEPTION_RECORD_LENGTH
    .ENDPROLOG

    /* Set it up */
    mov dword ptr [rsp + ErNumberParameters], 0
    mov dword ptr [rsp + ErExceptionFlags], EXCEPTION_NONCONTINUABLE
    mov [rsp + ErExceptionRecord], rcx
    mov [rsp + ErExceptionCode], edx

    /* Raise the exception */
    mov rcx, rsp
    call RtlRaiseException

    /* Cleanup stack and return */
    add rsp, EXCEPTION_RECORD_LENGTH
    ret
.ENDP

END

