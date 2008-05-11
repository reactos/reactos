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

/* FUNCTIONS ****************************************************************/

.func LdrInitializeThunk@16
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
.endfunc

.func KiUserApcExceptionHandler
_KiUserApcExceptionHandler:

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_EXIT_UNWIND + EXCEPTION_UNWINDING
    jz .return

    /* Test alert the thread */
    call _NtTestAlert@0

.return:
    /* We'll execute handler */
    mov eax, EXCEPTION_EXECUTE_HANDLER
    ret 16
.endfunc

.func KiUserApcDispatcher@16
.globl _KiUserApcDispatcher@16
_KiUserApcDispatcher@16:

    /* Setup SEH stack */
    lea eax, [esp+CONTEXT_ALIGNED_SIZE+16]
    mov ecx, fs:[TEB_EXCEPTION_LIST]
    mov edx, offset _KiUserApcExceptionHandler
    mov [eax], ecx
    mov [eax+4], edx

    /* Enable SEH */
    mov fs:[TEB_EXCEPTION_LIST], eax

    /* Put the Context in EDI */
    pop eax
    lea edi, [esp+12]

    /* Call the APC Routine */
    call eax

    /* Restore exception list */
    mov ecx, [edi+CONTEXT_ALIGNED_SIZE]
    mov fs:[TEB_EXCEPTION_LIST], ecx

    /* Switch back to the context */
    push 1
    push edi
    call _ZwContinue@8

    /* Save callback return value */
    mov esi, eax

    /* Raise status */
StatusRaiseApc:
    push esi
    call _RtlRaiseStatus@4
    jmp StatusRaiseApc
    ret 16
.endfunc

.func KiUserCallbackExceptionHandler
_KiUserCallbackExceptionHandler:

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_EXIT_UNWIND + EXCEPTION_UNWINDING
    jz return

    /* Tell the kernel to invalidate the stack */
    push STATUS_CALLBACK_POP_STACK
    push 0
    push 0
    call _ZwCallbackReturn@12

return:
    /* We'll execute the handler */
    mov eax, EXCEPTION_EXECUTE_HANDLER
    ret 16
.endfunc

.func KiUserCallbackDispatcher@12
.globl _KiUserCallbackDispatcher@12
_KiUserCallbackDispatcher@12:

    /* Setup SEH stack */
    mov ecx, fs:[TEB_EXCEPTION_LIST]
    mov edx, offset _KiUserCallbackExceptionHandler
    lea eax, [esp+16]
    mov [esp+16], ecx
    mov [esp+20], edx

    /* Enable SEH */
    mov fs:[TEB_EXCEPTION_LIST], eax

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

    /* Save callback return value */
    mov esi, eax

    /* Raise status */
StatusRaise:
    push esi
    call _RtlRaiseStatus@4
    jmp StatusRaise
    ret 12
.endfunc

.func KiRaiseUserExceptionDispatcher@0
.globl _KiRaiseUserExceptionDispatcher@0
_KiRaiseUserExceptionDispatcher@0:

    /* Setup stack for EXCEPTION_RECORD */
    push ebp
    mov ebp, esp
    sub esp, EXCEPTION_RECORD_LENGTH

    /* Fill out the record */
    mov [esp+EXCEPTION_RECORD_EXCEPTION_ADDRESS], eax
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
.endfunc

.func KiUserExceptionDispatcher@8
.globl _KiUserExceptionDispatcher@8
_KiUserExceptionDispatcher@8:

    /* clear the direct flag 
     * text from bug 2279
     * if it not clear it means that if an exception occurs while
     * the direction flag is set (typically inside memmove), the 
     * exception handlers will be called with the direction flag still
     * set.  The Windows x86-32 and x86-64 ABI requires that the 
     * direction flag be Calling memset() with a compile-time constant 
     * size on both GCC and MSVC will result in inlining a "rep stosd" 
     * instruction.  Because of the ABI, they will assume that the 
     * direction flag is clear and not emit a "cld" instruction. 
     * Using memset() in an exception handler therefore will 
     * corrupt memory if the exception occurred during a reverse copy 
     * such as a forward overlapping memmove().
     *
     * For reliability and ease of debugging, please add "cld" to the beginning of
     * KiUserExceptionDispatcher.  Note that the same will be true of x86-64 whenever
     * that happens.  This does not affect continuing execution; the CONTEXT of the
     * exception has the direction flag set and will be restored upon NtContinue. 
     * KiUserApcDispatcher and KiUserCallbackDispatcher need to be evaluated for this
     * issue.
     */
     
    cld

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
.endfunc

.func KiIntSystemCall@0
.globl _KiIntSystemCall@0
_KiIntSystemCall@0:

    /* Set stack in EDX and do the interrupt */
    lea edx, [esp+8]
    int 0x2E

    /* Return to caller */
    ret
.endfunc

.func KiFastSystemCall@0
.globl _KiFastSystemCall@0
_KiFastSystemCall@0:

    /* Put ESP in EDX and do the SYSENTER */
    mov edx, esp
    sysenter
.endfunc

.func KiFastSystemCallRet@0
.globl _KiFastSystemCallRet@0
_KiFastSystemCallRet@0:

    /* Just return to caller */
    ret
.endfunc

.func RtlpGetStackLimits@8
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
.endfunc
