/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT Library
 * FILE:            lib/ntdll/main/i386/dispatch.S
 * PURPOSE:         User-Mode NT Dispatchers
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

#define EXCEPTION_NONCONTINUABLE    1
#define EXCEPTION_UNWINDING         2
#define EXCEPTION_EXIT_UNWIND       4
#define EXCEPTION_UNWIND            (EXCEPTION_UNWINDING + EXCEPTION_EXIT_UNWIND)

#define STATUS_CALLBACK_POP_STACK   0xC0000423

#define ExceptionContinueSearch     1

/* FUNCTIONS ****************************************************************/

.globl _LdrInitializeThunk@16
_LdrInitializeThunk@16:

    /* Get the APC Context */
    lea eax, [esp+16]

    /* Send it as the first parameter */
    mov [esp+4], eax

    /* Terminate the frame list */
    xor ebp, ebp

    /* Jump into the C initialization routine */
    jmp _LdrpInit@12

.globl _KiUserExceptionApcHandler@16
_KiUserApcExceptionHandler@16:

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_UNWIND
    jz .return

    /* Test alert the thread */
    call _NtTestAlert@0

.return:
    /* We'll continue */
    mov eax, ExceptionContinueSearch
    ret 16
    
.globl _KiUserApcDispatcher@16
_KiUserApcDispatcher@16:

    /* Put the Context in EDI */
    lea edi, [esp+16]

    /* Get the ApcRoutine and call it */
    pop eax
    call eax

    /* Switch back to the context */
    push 1
    push edi
    call _ZwContinue@8

.globl _KiUserCallbackExceptionHandler@16
_KiUserCallbackExceptionHandler@16:

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_UNWIND
    jz return

    /* Tell the kernel to invalidate the stack */
    push STATUS_CALLBACK_POP_STACK
    push 0
    push 0
    call _ZwCallbackReturn@12

return:
    /* We'll continue */
    mov eax, ExceptionContinueSearch
    ret 16

.globl _KiUserCallbackDispatcher@12
_KiUserCallbackDispatcher@12:
    
    /* Get the callback Index */
    add esp, 4
    pop edx

    /* Get the callback table */
    mov eax, [fs:TEB_PEB]
    mov eax, [eax+PEB_KERNEL_CALLBACK_TABLE]

    /* Call the routine */
    call [eax+edx*4]

    /* Return from callback */
    push eax
    push 0
    push 0
    call _ZwCallbackReturn@12

.globl _KiRaiseUserExceptionDispatcher@0
_KiRaiseUserExceptionDispatcher@0:

    /* Setup stack for EXCEPTION_RECORD */
    push ebp
    mov ebp, esp
    sub esp, SIZEOF_EXCEPTION_RECORD

    /* Fill out the record */
    mov eax, [fs:KPCR_TEB]
    mov eax, [eax+TEB_EXCEPTION_CODE]
    mov [esp+EXCEPTION_RECORD_EXCEPTION_CODE], eax
    mov dword ptr [esp+EXCEPTION_RECORD_EXCEPTION_FLAGS], 0
    mov dword ptr [esp+EXCEPTION_RECORD_EXCEPTION_RECORD], 0
    mov dword ptr [esp+EXCEPTION_RECORD_NUMBER_PARAMETERS], 0

    /* Raise the exception */
    push esp
    call _RtlRaiseException@4

    /* Return exception code */
    mov eax, [esp+EXCEPTION_RECORD_EXCEPTION_CODE]
    mov esp, ebp
    pop ebp
    ret

.globl _KiUserExceptionDispatcher@8
_KiUserExceptionDispatcher@8:

    /* Save the Context and Exception Records */
    mov ecx, [esp+4]
    mov ebx, [esp]

    /* Call the vectored exception handler */
    push ecx
    push ebx
    call _RtlpExecuteVectoredExceptionHandlers@8

    /* Check for success */
    or al, al
    jnz ContinueExecution

    /* Dispatch the exception */
    sub esp, 8
    call _RtlDispatchException@8

    /* Check for success */
    or al, al
    jz RaiseException

ContinueExecution:
    /* Pop off the records */
    pop ebx
    pop ecx

    /* We're fine, continue execution */
    push 0
    push ecx
    call _ZwContinue@8

    /* Exit */
    jmp Exit

RaiseException:
    /* Pop off the records */
    pop ebx
    pop ecx

    /* Raise the exception */
    push 0
    push ecx
    push ebx
    call _ZwRaiseException@12

Exit:
    /* Allocate space for the nested exception record */
    add esp, -SIZEOF_EXCEPTION_RECORD

    /* Set it up */
    mov [esp+EXCEPTION_RECORD_EXCEPTION_CODE], eax
    mov dword ptr [esp+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_NONCONTINUABLE
    mov [esp+EXCEPTION_RECORD_EXCEPTION_RECORD], ebx
    mov dword ptr [esp+EXCEPTION_RECORD_NUMBER_PARAMETERS], 0

    /* Raise the exception */
    push esp
    call _RtlRaiseException@4
    ret 8

.globl _RtlpGetStackLimits@8
_RtlpGetStackLimits@8:

    /* Get the stack limits */
    mov eax, [fs:TEB_STACK_LIMIT]
    mov ecx, [fs:TEB_STACK_BASE]

    /* Return them */
    mov edx, [esp+4]
    mov [edx], eax
    mov edx, [esp+8]
    mov [edx], ecx

    /* return */
    ret 8

