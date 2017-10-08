/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library (RTL)
 * FILE:            lib/rtl/i386/except_asm.s
 * PURPOSE:         User-mode exception support for IA-32
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

EXTERN _RtlpCheckForActiveDebugger@0:PROC
EXTERN _RtlDispatchException@8:PROC
EXTERN _ZwContinue@8:PROC
EXTERN _ZwRaiseException@12:PROC

#define ExceptionContinueSearch     1
#define ExceptionNestedException    2
#define ExceptionCollidedUnwind     3

/* FUNCTIONS *****************************************************************/

.code

PUBLIC _RtlpGetExceptionList@0
_RtlpGetExceptionList@0:

    /* Return the exception list */
    mov eax, fs:[TEB_EXCEPTION_LIST]
    ret


PUBLIC _RtlpSetExceptionList@4
_RtlpSetExceptionList@4:

    /* Get the new list */
    mov ecx, [esp+4]
    mov ecx, [ecx]

    /* Write it */
    mov fs:[TEB_EXCEPTION_LIST], ecx

    /* Return */
    ret 4


PUBLIC _RtlCaptureContext@4
_RtlCaptureContext@4:

    /* Preserve EBX and put the context in it */
    push ebx
    mov ebx, [esp+8]

    /* Save the basic register context */
    mov [ebx+CONTEXT_EAX], eax
    mov [ebx+CONTEXT_ECX], ecx
    mov [ebx+CONTEXT_EDX], edx
    mov eax, [esp]
    mov [ebx+CONTEXT_EBX], eax
    mov [ebx+CONTEXT_ESI], esi
    mov [ebx+CONTEXT_EDI], edi

    /* Capture the other regs */
    jmp CaptureRest


PUBLIC _RtlpCaptureContext@4
_RtlpCaptureContext@4:

    /* Preserve EBX and put the context in it */
    push ebx
    mov ebx, [esp+8]

    /* Clear the basic register context */
    mov dword ptr [ebx+CONTEXT_EAX], 0
    mov dword ptr [ebx+CONTEXT_ECX], 0
    mov dword ptr [ebx+CONTEXT_EDX], 0
    mov dword ptr [ebx+CONTEXT_EBX], 0
    mov dword ptr [ebx+CONTEXT_ESI], 0
    mov dword ptr [ebx+CONTEXT_EDI], 0

CaptureRest:
    /* Capture the segment registers */
    mov [ebx+CONTEXT_SEGCS], cs
    mov [ebx+CONTEXT_SEGDS], ds
    mov [ebx+CONTEXT_SEGES], es
    mov [ebx+CONTEXT_SEGFS], fs
    mov [ebx+CONTEXT_SEGGS], gs
    mov [ebx+CONTEXT_SEGSS], ss

    /* Capture flags */
    pushfd
    pop [ebx+CONTEXT_EFLAGS]

    /* The return address should be in [ebp+4] */
    mov eax, [ebp+4]
    mov [ebx+CONTEXT_EIP], eax

    /* Get EBP */
    mov eax, [ebp+0]
    mov [ebx+CONTEXT_EBP], eax

    /* And get ESP */
    lea eax, [ebp+8]
    mov [ebx+CONTEXT_ESP], eax

    /* Return to the caller */
    pop ebx
    ret 4


PUBLIC _RtlpExecuteHandlerForException@20
_RtlpExecuteHandlerForException@20:

    /* Copy the routine in EDX */
    mov edx, offset _RtlpExceptionProtector

    /* Jump to common routine */
    jmp _RtlpExecuteHandler@20


PUBLIC _RtlpExecuteHandlerForUnwind@20
_RtlpExecuteHandlerForUnwind@20:
    /* Copy the routine in EDX */
    mov edx, offset _RtlpUnwindProtector


_RtlpExecuteHandler@20:

    /* Save non-volatile */
    push ebx
    push esi
    push edi

    /* Clear registers */
    xor eax, eax
    xor ebx, ebx
    xor esi, esi
    xor edi, edi

    /* Call the 2nd-stage executer */
    push [esp+32]
    push [esp+32]
    push [esp+32]
    push [esp+32]
    push [esp+32]
    call _RtlpExecuteHandler2@20

    /* Restore non-volatile */
    pop edi
    pop esi
    pop ebx
    ret 20


PUBLIC _RtlpExecuteHandler2@20
_RtlpExecuteHandler2@20:

    /* Set up stack frame */
    push ebp
    mov ebp, esp

    /* Save the Frame */
    push [ebp+12]

    /* Push handler address */
    push edx

    /* Push the exception list */
    push [fs:TEB_EXCEPTION_LIST]

    /* Link us to it */
    mov [fs:TEB_EXCEPTION_LIST], esp

    /* Call the handler */
    push [ebp+20]
    push [ebp+16]
    push [ebp+12]
    push [ebp+8]
    mov ecx, [ebp+24]
    call ecx

    /* Unlink us */
    mov esp, [fs:TEB_EXCEPTION_LIST]

    /* Restore it */
    pop [fs:TEB_EXCEPTION_LIST]

    /* Undo stack frame and return */
    mov esp, ebp
    pop ebp
    ret 20


_RtlpExceptionProtector:

    /* Assume we'll continue */
    mov eax, ExceptionContinueSearch

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_UNWINDING + EXCEPTION_EXIT_UNWIND
    jnz return

    /* Save the frame in ECX and Context in EDX */
    mov ecx, [esp+8]
    mov edx, [esp+16]

    /* Get the nested frame */
    mov eax, [ecx+8]

    /* Set it as the dispatcher context */
    mov [edx], eax

    /* Return nested exception */
    mov eax, ExceptionNestedException

return:
    ret 16


_RtlpUnwindProtector:

    /* Assume we'll continue */
    mov eax, ExceptionContinueSearch

    /* Put the exception record in ECX and check the Flags */
    mov ecx, [esp+4]
    test dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_UNWINDING + EXCEPTION_EXIT_UNWIND
    jz .return

    /* Save the frame in ECX and Context in EDX */
    mov ecx, [esp+8]
    mov edx, [esp+16]

    /* Get the nested frame */
    mov eax, [ecx+8]

    /* Set it as the dispatcher context */
    mov [edx], eax

    /* Return collided unwind */
    mov eax, ExceptionCollidedUnwind

.return:
    ret 16


PUBLIC _RtlRaiseException@4
_RtlRaiseException@4:

    /* Set up stack frame */
    push ebp
    mov ebp, esp

    /*
     * Save the context while preserving everything but ESP and EBP.
     * This is vital because the caller will be restored with this context
     * in case the execution is continued, which means we must not clobber
     * the non-volatiles. We preserve the volatiles too because the context
     * could get passed to a debugger.
     */
    lea esp, [esp-CONTEXT_FRAME_LENGTH]
    push esp
    call _RtlCaptureContext@4

    /* Adjust ESP to account for the argument that was passed */
    add dword ptr [esp+CONTEXT_ESP], 4

    /* Save the exception address */
    mov edx, [ebp+4]
    mov eax, [ebp+8]
    mov [eax+EXCEPTION_RECORD_EXCEPTION_ADDRESS], edx

    /* Write the context flag */
    mov dword ptr [esp+CONTEXT_FLAGS], CONTEXT_FULL

    /* Check if user mode debugger is active */
    call _RtlpCheckForActiveDebugger@0
    test al, al
    jnz DebuggerActive1

    /* Dispatch the exception */
    push esp
    push [ebp+8]
    call _RtlDispatchException@8
    test al, al
    jz RaiseException

    /* Continue, go back to previous context */
    mov ecx, esp
    push 0
    push ecx
    call  _ZwContinue@8
    jmp RaiseStatus1

DebuggerActive1:

    /* Raise an exception immediately */
    mov ecx, esp
    push 1
    push ecx
    push [ebp+8]
    call _ZwRaiseException@12
    jmp RaiseStatus1

RaiseException:

    /* Raise the exception */
    mov ecx, esp
    push 0
    push ecx
    push [ebp+8]
    call _ZwRaiseException@12

RaiseStatus1:

    /* If we returned, raise a status */
    push eax
    call _RtlRaiseStatus@4


PUBLIC _RtlRaiseStatus@4
_RtlRaiseStatus@4:

    /* Set up stack frame */
    push ebp
    mov ebp, esp

    /*
     * Save the context while preserving everything but ESP and EBP.
     * This is vital because the caller will be restored with this context
     * in case the execution is continued, which means we must not clobber
     * the non-volatiles. We preserve the volatiles too because the context
     * could get passed to a debugger.
     */
    lea esp, [esp-CONTEXT_FRAME_LENGTH-EXCEPTION_RECORD_LENGTH]
    push esp
    call _RtlCaptureContext@4

    /* Adjust ESP to account for the argument that was passed */
    add dword ptr [esp+CONTEXT_ESP], 4

    /* Set up the exception record */
    lea ecx, [esp+CONTEXT_FRAME_LENGTH]
    mov eax, [ebp+8]
    mov dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_CODE], eax
    mov dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_FLAGS], EXCEPTION_NONCONTINUABLE
    and dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_RECORD], 0
    mov eax, [ebp+4]
    mov dword ptr [ecx+EXCEPTION_RECORD_EXCEPTION_ADDRESS], eax
    and dword ptr [ecx+EXCEPTION_RECORD_NUMBER_PARAMETERS], 0

    /* Write the context flag */
    mov dword ptr [esp+CONTEXT_FLAGS], CONTEXT_FULL

    /* Check if user mode debugger is active */
    call _RtlpCheckForActiveDebugger@0

    /* Restore ECX and jump if debugger is active */
    lea ecx, [esp+CONTEXT_FRAME_LENGTH]
    test al, al
    jnz DebuggerActive2

    /* Dispatch the exception */
    push esp
    push ecx
    call _RtlDispatchException@8

    /* Raise exception if we got here */
    lea ecx, [esp+CONTEXT_FRAME_LENGTH]
    mov edx, esp
    push 0
    push edx
    push ecx
    call _ZwRaiseException@12
    jmp RaiseStatus2

DebuggerActive2:

    /* Raise an exception immediately */
    mov edx, esp
    push 1
    push edx
    push ecx
    call _ZwRaiseException@12

RaiseStatus2:

    /* If we returned, raise a status */
    push eax
    call _RtlRaiseStatus@4

END
