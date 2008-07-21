/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engsem.c
 * PURPOSE:         Semaphore Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
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
    UNIMPLEMENTED;
    return 0;
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
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngReleaseSemaphore(IN HSEMAPHORE hSem)
{
    UNIMPLEMENTED;
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
