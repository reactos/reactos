/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/ccmutex.c
 * PURPOSE:         Implements the Broken Mutex Implementation for the broken Cc
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
CcAcquireBrokenMutexUnsafe(PFAST_MUTEX FastMutex)
{
    ASSERT(KeGetCurrentThread() == NULL ||
           FastMutex->Owner != KeGetCurrentThread());
    ASSERT(KeGetCurrentIrql() == APC_LEVEL || 
    KeGetCurrentThread() == NULL || 
    KeGetCurrentThread()->KernelApcDisable);
  
    InterlockedIncrementUL(&FastMutex->Contention);
    while (InterlockedExchange(&FastMutex->Count, 0) == 0)
    {
        KeWaitForSingleObject(&FastMutex->Gate,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    InterlockedDecrementUL(&FastMutex->Contention);
    FastMutex->Owner = KeGetCurrentThread();
}

VOID
FASTCALL
CcReleaseBrokenMutexUnsafe(PFAST_MUTEX FastMutex)
{
    ASSERT(KeGetCurrentThread() == NULL ||
           FastMutex->Owner == KeGetCurrentThread());
    ASSERT(KeGetCurrentIrql() == APC_LEVEL || 
    KeGetCurrentThread() == NULL || 
    KeGetCurrentThread()->KernelApcDisable);
  
    FastMutex->Owner = NULL;
    InterlockedExchange(&FastMutex->Count, 1);
    if (FastMutex->Contention > 0)
    {
        KeSetEvent(&FastMutex->Gate, 0, FALSE);
    }
}

VOID 
FASTCALL
CcAcquireBrokenMutex(PFAST_MUTEX FastMutex)
{
    KeEnterCriticalRegion();
    CcAcquireBrokenMutexUnsafe(FastMutex);
}


VOID
FASTCALL
CcReleaseBrokenMutex(PFAST_MUTEX FastMutex)
{
    CcReleaseBrokenMutexUnsafe(FastMutex);
    KeLeaveCriticalRegion();
}

BOOLEAN
FASTCALL
CcTryToAcquireBrokenMutex(PFAST_MUTEX FastMutex)
{
    KeEnterCriticalRegion();
    if (InterlockedExchange(&FastMutex->Count, 0) == 1)
    {
        FastMutex->Owner = KeGetCurrentThread();
        return(TRUE);
    }
    else
    {
        KeLeaveCriticalRegion();
        return(FALSE);
    }
}
