/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/lock.c
 * PURPOSE:     User-side Services Start Serialization Lock functions
 * COPYRIGHT:   Copyright 2012 Hermès Bélusca
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#include <time.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* The unique user service start lock of the SCM */
static PSTART_LOCK pServiceStartLock = NULL;


/* FUNCTIONS *****************************************************************/

/*
 * NOTE: IsServiceController is TRUE if locked by the
 * Service Control Manager, and FALSE otherwise.
 */
DWORD
ScmAcquireServiceStartLock(IN BOOL IsServiceController,
                           OUT LPSC_RPC_LOCK lpLock)
{
    DWORD dwRequiredSize;
    DWORD dwError = ERROR_SUCCESS;

    *lpLock = NULL;

    /* Lock the service database exclusively */
    ScmLockDatabaseExclusive();

    if (pServiceStartLock != NULL)
    {
        dwError = ERROR_SERVICE_DATABASE_LOCKED;
        goto done;
    }

    /* Allocate a new lock for the database */
    dwRequiredSize = sizeof(START_LOCK);

    if (!IsServiceController)
    {
        /* FIXME: dwRequiredSize += RtlLengthSid(UserSid <-- to be retrieved); */
    }

    pServiceStartLock = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  dwRequiredSize);
    if (pServiceStartLock == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    pServiceStartLock->Tag = LOCK_TAG;
    pServiceStartLock->TimeWhenLocked = (DWORD)time(NULL);

    /* FIXME: Retrieve the owner SID. Use IsServiceController. */
    pServiceStartLock->LockOwnerSid   = (PSID)NULL;

    *lpLock = (LPSC_RPC_LOCK)pServiceStartLock;

done:
    /* Unlock the service database */
    ScmUnlockDatabase();

    return dwError;
}


DWORD
ScmReleaseServiceStartLock(IN OUT LPSC_RPC_LOCK lpLock)
{
    PSTART_LOCK pStartLock;
    DWORD dwError = ERROR_SUCCESS;

    if (lpLock == NULL)
        return ERROR_INVALID_SERVICE_LOCK;

    pStartLock = (PSTART_LOCK)*lpLock;

    if (pStartLock->Tag != LOCK_TAG)
        return ERROR_INVALID_SERVICE_LOCK;

    /* Lock the service database exclusively */
    ScmLockDatabaseExclusive();

    /* Release the lock handle */
    if ((pStartLock == pServiceStartLock) &&
        (pServiceStartLock != NULL))
    {
        HeapFree(GetProcessHeap(), 0, pServiceStartLock);
        pServiceStartLock = NULL;
        *lpLock = NULL;

        dwError = ERROR_SUCCESS;
    }
    else
    {
        dwError = ERROR_INVALID_SERVICE_LOCK;
    }

    /* Unlock the service database */
    ScmUnlockDatabase();

    return dwError;
}


/*
 * Helper functions for RQueryServiceLockStatusW() and
 * RQueryServiceLockStatusA().
 * We suppose that lpLockStatus points to a valid
 * well-sized buffer.
 */
VOID
ScmQueryServiceLockStatusW(OUT LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus)
{
    /* Lock the service database shared */
    ScmLockDatabaseShared();

    if (pServiceStartLock != NULL)
    {
        lpLockStatus->fIsLocked = TRUE;

        /* FIXME: Retrieve the owner name. */
        lpLockStatus->lpLockOwner = NULL;

        lpLockStatus->dwLockDuration = (DWORD)time(NULL) - pServiceStartLock->TimeWhenLocked;
    }
    else
    {
        lpLockStatus->fIsLocked = FALSE;

        wcscpy((LPWSTR)(lpLockStatus + 1), L"");
        lpLockStatus->lpLockOwner = (LPWSTR)(ULONG_PTR)sizeof(QUERY_SERVICE_LOCK_STATUSW);

        lpLockStatus->dwLockDuration = 0;
    }

    /* Unlock the service database */
    ScmUnlockDatabase();

    return;
}


VOID
ScmQueryServiceLockStatusA(OUT LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus)
{
    /* Lock the service database shared */
    ScmLockDatabaseShared();

    if (pServiceStartLock != NULL)
    {
        lpLockStatus->fIsLocked = TRUE;

        /* FIXME: Retrieve the owner name. */
        lpLockStatus->lpLockOwner = NULL;

        lpLockStatus->dwLockDuration = (DWORD)time(NULL) - pServiceStartLock->TimeWhenLocked;
    }
    else
    {
        lpLockStatus->fIsLocked = FALSE;

        strcpy((LPSTR)(lpLockStatus + 1), "");
        lpLockStatus->lpLockOwner = (LPSTR)(ULONG_PTR)sizeof(QUERY_SERVICE_LOCK_STATUSA);

        lpLockStatus->dwLockDuration = 0;
    }

    /* Unlock the service database */
    ScmUnlockDatabase();

    return;
}

/* EOF */
