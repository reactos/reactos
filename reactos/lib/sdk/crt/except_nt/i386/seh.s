/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/crt/misc/i386/seh.S
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

#define DISPOSITION_DISMISS         0
#define DISPOSITION_CONTINUE_SEARCH 1
#define DISPOSITION_COLLIDED_UNWIND 3

/* GLOBALS *******************************************************************/

.globl __global_unwind2
.globl __local_unwind2
.globl __abnormal_termination
.globl __except_handler2
.globl __except_handler3

/* FUNCTIONS *****************************************************************/

.func unwind_handler
_unwind_handler:

    /* Check if we were unwinding and continue search if not */
    mov ecx, [esp+4]
    test dword ptr [ecx+4], EXCEPTION_EXIT_UNWIND + EXCEPTION_UNWINDING
    mov eax, DISPOSITION_CONTINUE_SEARCH
    jz unwind_handler_return

    /* We have a collision, do a local unwind */
    mov eax, [esp+20]
    push ebp
    mov ebp, [eax+16]
    mov edx, [eax+40]
    push edx
    mov edx, [eax+36]
    push edx
    call __local_unwind2
    add esp, 8
    pop ebp

    /* Set new try level */
    mov eax, [esp+8]
    mov edx, [esp+16]
    mov [edx], eax

    /* Return collided unwind */
    mov eax, DISPOSITION_COLLIDED_UNWIND

unwind_handler_return:
    ret
.endfunc

.func _global_unwind2
__global_unwind2:

    /* Create stack and save all registers */
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    push ebp

    /* Call unwind */
    push 0
    push 0
    push glu_return
    push [ebp+8]
    call _RtlUnwind@16

glu_return:
    /* Restore registers and return */
    pop ebp
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret
.endfunc

.func _abnormal_termination
__abnormal_termination:

    /* Assume false */
    xor eax, eax

    /* Check if the handler is the unwind handler */
    mov ecx, fs:0
    cmp dword ptr [ecx+4], offset _unwind_handler
    jne short ab_return

    /* Get the try level */
    mov edx, [ecx+12]
    mov edx, [edx+12]

    /* Compare it */
    cmp [ecx+8], edx
    jne ab_return

    /* Return true */
    mov eax, 1

    /* Return */
ab_return:
    ret
.endfunc

.func _local_unwind2
__local_unwind2:

    /* Save volatiles */
    push ebx
    push esi
    push edi

    /* Get the exception registration */
    mov eax, [esp+16]

    /* Setup SEH to protect the unwind */
    push ebp
    push eax
    push -2
    push offset _unwind_handler
    push fs:0
    mov fs:0, esp

unwind_loop:
    /* Get the exception registration and try level */
    mov eax, [esp+36]
    mov ebx, [eax+8]
    mov esi, [eax+12]

    /* Validate the unwind */
    cmp esi, -1
    je unwind_return
    cmp dword ptr [esp+40], -1
    je unwind_ok
    cmp esi, [esp+40]
    jbe unwind_return

unwind_ok:
    /* Get the new enclosing level and save it */
    lea esi, [esi+esi*2]
    mov ecx, [ebx+esi*4]
    mov [esp+8], ecx
    mov [eax+12], ecx

    /* Check the filter type */
    cmp dword ptr [ebx+esi*4+4], 0
    jnz __NLG_Return2

    /* FIXME: NLG Notification */

    /* Call the handler */
    call dword ptr [ebx+esi*4+8]

__NLG_Return2:
    /* Unwind again */
    jmp unwind_loop

unwind_return:
    /* Cleanup SEH */
    pop fs:0
    add esp, 16
    pop edi
    pop esi
    pop ebx
    ret
.endfunc

.func _except_handler2
__except_handler2:

    /* Setup stack and save volatiles */
    push ebp
    mov ebp, esp
    sub esp, 8
    push ebx
    push esi
    push edi
    push ebp

    /* Clear direction flag */
    cld

    /* Get exception registration and record */
    mov ebx, [ebp+12]
    mov eax, [ebp+8]

    /* Check if this is an unwind */
    test dword ptr [eax+4], EXCEPTION_EXIT_UNWIND + EXCEPTION_UNWINDING
    jnz except_unwind2

    /* Save exception pointers structure */
    mov [ebp-8], eax
    mov eax, [ebp+16]
    mov [ebp-4], eax
    lea eax, [ebp-8]
    mov [ebx+20], eax

    /* Get the try level and scope table */
    mov esi, [ebx+12]
    mov esi, [ebx+8]

except_loop2:
    /* Validate try level */
    cmp esi, -1
    je except_search2

    /* Check if this is the termination handler */
    lea ecx, [esi+esi*2]
    cmp dword ptr [edi+ecx*4+4], 0
    jz except_continue2

    /* Save registers and call filter, then restore them */
    push esi
    push ebp
    mov ebp, [ebx+16]
    call dword ptr [edi+ecx*4+4]
    pop ebp
    pop esi

    /* Restore ebx and check the result */
    mov ebx, [ebp+12]
    or eax, eax
    jz except_continue2
    js except_dismiss2

    /* So this is an accept, call the termination handlers */
    mov edi, [ebx+8]
    push ebx
    call __global_unwind2
    add esp, 4

    /* Restore ebp */
    mov ebp, [ebx+16]

    /* Do local unwind */
    push esi
    push ebx
    call __local_unwind2
    add esp, 8

    /* Set new try level */
    lea ecx, [esi+esi*2]
    mov eax, [edi+ecx*4]
    mov [ebx+12], eax

    /* Call except handler */
    call [edi+ecx*4+8]

except_continue2:
    /* Reload try level and except again */
    mov edi, [ebx+8]
    lea ecx, [esi+esi*2]
    mov esi, [edi+ecx*4]
    jmp except_loop2

except_dismiss2:
    /* Dismiss it */
    mov eax, DISPOSITION_DISMISS
    jmp except_return2

except_search2:
    /* Continue searching */
    mov eax, DISPOSITION_CONTINUE_SEARCH
    jmp except_return2

    /* Do local unwind */
except_unwind2:
    push ebp
    mov ebp, [ebx+16]
    push -1
    push ebx
    call __local_unwind2
    add esp, 8

    /* Retore EBP and set return disposition */
    pop ebp
    mov eax, DISPOSITION_CONTINUE_SEARCH

except_return2:
    /* Restore registers and stack */
    pop ebp
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret
.endfunc

.func _except_handler3
__except_handler3:

    /* Setup stack and save volatiles */
    push ebp
    mov ebp, esp
    sub esp, 8
    push ebx
    push esi
    push edi
    push ebp

    /* Clear direction flag */
    cld

    /* Get exception registration and record */
    mov ebx, [ebp+12]
    mov eax, [ebp+8]

    /* Check if this is an unwind */
    test dword ptr [eax+4], EXCEPTION_EXIT_UNWIND + EXCEPTION_UNWINDING
    jnz except_unwind3

    /* Save exception pointers structure */
    mov [ebp-8], eax
    mov eax, [ebp+16]
    mov [ebp-4], eax
    lea eax, [ebp-8]
    mov [ebx-4], eax

    /* Get the try level and scope table */
    mov esi, [ebx+12]
    mov esi, [ebx+8]

    /* FIXME: Validate the SEH exception */

except_loop3:
    /* Validate try level */
    cmp esi, -1
    je except_search3

    /* Check if this is the termination handler */
    lea ecx, [esi+esi*2]
    mov eax, [edi+ecx*4+4]
    or eax, eax
    jz except_continue3

    /* Save registers clear them all */
    push esi
    push ebp
    lea ebp, [ebx+16]
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi

    /* Call the filter and restore our registers */
    call eax
    pop ebp
    pop esi

    /* Restore ebx and check the result */
    mov ebx, [ebp+12]
    or eax, eax
    jz except_continue3
    js except_dismiss3

    /* So this is an accept, call the termination handlers */
    mov edi, [ebx+8]
    push ebx
    call __global_unwind2
    add esp, 4

    /* Restore ebp */
    lea ebp, [ebx+16]

    /* Do local unwind */
    push esi
    push ebx
    call __local_unwind2
    add esp, 8

    /* FIXME: Do NLG Notification */

    /* Set new try level */
    lea ecx, [esi+esi*2]
    mov eax, [edi+ecx*4]
    mov [ebx+12], eax

    /* Clear registers and call except handler */
    mov eax, [edi+ecx*4+8]
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi
    call eax

except_continue3:
    /* Reload try level and except again */
    mov edi, [ebx+8]
    lea ecx, [esi+esi*2]
    mov esi, [edi+ecx*4]
    jmp except_loop3

except_dismiss3:
    /* Dismiss it */
    mov eax, DISPOSITION_DISMISS
    jmp except_return3

except_search3:
    /* Continue searching */
    mov eax, DISPOSITION_CONTINUE_SEARCH
    jmp except_return3

    /* Do local unwind */
except_unwind3:
    push ebp
    mov ebp, [ebx+16]
    push -1
    push ebx
    call __local_unwind2
    add esp, 8

    /* Retore EBP and set return disposition */
    pop ebp
    mov eax, DISPOSITION_CONTINUE_SEARCH

except_return3:
    /* Restore registers and stack */
    pop ebp
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret
.endfunc

//
//
// REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME
//
//
.func RtlpGetStackLimits@8
.globl _RtlpGetStackLimits@8
_RtlpGetStackLimits@8:

    /* Get the current thread */
    mov eax, [fs:KPCR_CURRENT_THREAD]

    /* Get the stack limits */
    mov ecx, [eax+KTHREAD_STACK_LIMIT]
    mov edx, [eax+KTHREAD_INITIAL_STACK]
    sub edx, SIZEOF_FX_SAVE_AREA

    /* Return them */
    mov eax, [esp+4]
    mov [eax], ecx

    mov eax, [esp+8]
    mov [eax], edx

    /* return */
    ret 8
.endfunc
