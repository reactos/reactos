/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/rtl/i386/stack.S
 * PURPOSE:         Stack Support for RTL
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

EXTERN _KePrefetchNTAGranularity:DWORD

/* FUNCTIONS *****************************************************************/
.code

PUBLIC _RtlpGetStackLimits@8
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


PUBLIC @RtlPrefetchMemoryNonTemporal@8
@RtlPrefetchMemoryNonTemporal@8:

    /*
     * Kernel will overwrite this to 'nop' during init
     * if prefetchnta is available. Slight optimization
     * as compared to checking KeI386XMMIPresent for every call.
     */
    ret

    /* Get granularity */
    mov eax, [_KePrefetchNTAGranularity]

    /* Prefetch this line */
FetchLine:
    prefetchnta byte ptr [ecx]

    /* Update address and count */
    add ecx, eax
    sub edx, eax

    /* Keep looping for the next line, or return if done */
    ja FetchLine
    ret

END
