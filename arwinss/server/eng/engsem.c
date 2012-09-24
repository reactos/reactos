/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engsem.c
 * PURPOSE:         Semaphore Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HSEMAPHORE
APIENTRY
EngCreateSemaphore(VOID)
{
    PERESOURCE Semaphore;
    NTSTATUS Status;

    /* Allocate memory for a resource (must be non paged) */
    Semaphore = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), TAG_GSEM);

    /* Fail in case of error */
    if (!Semaphore) return NULL;

    /* Initialize the resource */
    Status = ExInitializeResourceLite(Semaphore);

    /* Free memory and exit in case of failure */
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Semaphore, TAG_GSEM);
        return NULL;
    }

    /* Return "handle" to the newly created resource */
    return (HSEMAPHORE)Semaphore;
}

VOID
APIENTRY
EngDeleteSemaphore(IN HSEMAPHORE hSem)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngInitializeSafeSemaphore(OUT ENGSAFESEMAPHORE* Semaphore)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
EngDeleteSafeSemaphore(IN OUT ENGSAFESEMAPHORE* Semaphore)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngAcquireSemaphore(IN HSEMAPHORE hSem)
{
    /* Enter critical region and acquire the resource */
    ExEnterCriticalRegionAndAcquireResourceExclusive((PERESOURCE)hSem);
}

VOID
APIENTRY
EngReleaseSemaphore(IN HSEMAPHORE hSem)
{
    /* Release resource and leave critical region */
    ExReleaseResourceAndLeaveCriticalRegion((PERESOURCE)hSem);
}

BOOL
APIENTRY
EngIsSemaphoreOwned(IN HSEMAPHORE hSem)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngIsSemaphoreOwnedByCurrentThread(IN HSEMAPHORE hSem)
{
    UNIMPLEMENTED;
    return FALSE;
}
