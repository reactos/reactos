/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library (RTL)
 * FILE:            lib/rtl/i386/except_asm.S
 * PURPOSE:         User-mode exception support for IA-32
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

#define ExceptionContinueSearch     1
#define ExceptionNestedException    2
#define ExceptionCollidedUnwind     3

/* FUNCTIONS *****************************************************************/

.func RtlpGetExceptionList@0
.globl _RtlpGetExceptionList@0
_RtlpGetExceptionList@0:

    /* Return the exception list */
    mov eax, fs:[TEB_EXCEPTION_LIST]
    ret
.endfunc

.func RtlpSetExceptionList@4
.globl _RtlpSetExceptionList@4
_RtlpSetExceptionList@4:

    /* Get the new list */
    mov ecx, [esp+4]
    mov ecx, [ecx]

    /* Write it */
    mov fs:[TEB_EXCEPTION_LIST], ecx

    /* Return */
    ret 4
.endfunc

.func RtlpGetExceptionAddress@0
.globl _RtlpGetExceptionAddress@0
_RtlpGetExceptionAddress@0:

    /* Return the address from the stack */
    mov eax, [ebp+4]

    /* Return */
    ret
.endfunc

.func RtlCaptureContext@4
.globl _RtlCaptureContext@4
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
.endfunc

.func RtlpCaptureContext@4
.globl _RtlpCaptureContext@4
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
.endfunc

.func RtlpExecuteHandlerForException@20
.globl _RtlpExecuteHandlerForException@20
_RtlpExecuteHandlerForException@20:

    /* Copy the routine in EDX */
    mov edx, offset _RtlpExceptionProtector

    /* Jump to common routine */
    jmp _RtlpExecuteHandler@20
.endfunc

.func RtlpExecuteHandlerForUnwind@20
.globl _RtlpExecuteHandlerForUnwind@20
_RtlpExecuteHandlerForUnwind@20:
    /* Copy the routine in EDX */
    mov edx, offset _RtlpUnwindProtector
.endfunc

.func RtlpExecuteHandler@20
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
    push [esp+0x20]
    push [esp+0x20]
    push [esp+0x20]
    push [esp+0x20]
    push [esp+0x20]
    call _RtlpExecuteHandler2@20

    /* Restore non-volatile */
    pop edi
    pop esi
    pop ebx
    ret 0x14
.endfunc

.func RtlpExecuteHandler2@20
.globl _RtlpExecuteHandler2@20
_RtlpExecuteHandler2@20:

    /* Set up stack frame */
    push ebp
    mov ebp, esp

    /* Save the Frame */
    push [ebp+0xC]

    /* Push handler address */
    push edx

    /* Push the exception list */
    push [fs:TEB_EXCEPTION_LIST]

    /* Link us to it */
    mov [fs:TEB_EXCEPTION_LIST], esp

    /* Call the handler */
    push [ebp+0x14]
    push [ebp+0x10]
    push [ebp+0xC]
    push [ebp+8]
    mov ecx, [ebp+0x18]
    call ecx

    /* Unlink us */
    mov esp, [fs:TEB_EXCEPTION_LIST]

    /* Restore it */
    pop [fs:TEB_EXCEPTION_LIST]

    /* Undo stack frame and return */
    mov esp, ebp
    pop ebp
    ret 0x14
.endfunc

.func RtlpExceptionProtector
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
.endfunc

.func RtlpUnwindProtector
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
.endfunc

