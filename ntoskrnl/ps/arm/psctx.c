/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ps/arm/psctx.c
 * PURPOSE:         Process Manager: Set/Get Context for ARM
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PspGetContext(IN PKTRAP_FRAME TrapFrame,
              IN PVOID NonVolatileContext,
              IN OUT PCONTEXT Context)
{   
    PAGED_CODE();
}

VOID
NTAPI
PspSetContext(OUT PKTRAP_FRAME TrapFrame,
              OUT PVOID NonVolatileContext,
              IN PCONTEXT Context,
              IN KPROCESSOR_MODE Mode)
{   
    PAGED_CODE();
}

VOID
NTAPI
PspGetOrSetContextKernelRoutine(IN PKAPC Apc,
                                IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                                IN OUT PVOID* NormalContext,
                                IN OUT PVOID* SystemArgument1,
                                IN OUT PVOID* SystemArgument2)
{
    UNIMPLEMENTED;
}

/* EOF */
