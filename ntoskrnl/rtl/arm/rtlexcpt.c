/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/rtl/arm/rtlexcpt.c
 * PURPOSE:         ARM Exception Helper Routines for Stack Walking
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
