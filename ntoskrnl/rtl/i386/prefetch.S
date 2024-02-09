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
