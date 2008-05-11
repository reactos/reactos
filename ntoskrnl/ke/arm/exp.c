/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/exp.c
 * PURPOSE:         Implements exception helper routines for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/


/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
    UNIMPLEMENTED;
    return;
}
