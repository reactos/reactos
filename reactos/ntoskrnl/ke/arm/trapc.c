/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/trapc.c
 * PURPOSE:         Implements the various trap handlers for ARM exceptions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
KiDataAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    DPRINT1("Data Abort (%p) @ %p\n", TrapFrame, TrapFrame->Lr - 8);
    
    while (TRUE);
    return STATUS_SUCCESS;
}

