/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/rtl/i386/stack.s
 * PURPOSE:         Stack Support for RTL
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

/* FUNCTIONS *****************************************************************/

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
