/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/lock.c
 * PURPOSE:     User-side Services Start Serialization Lock functions
 * COPYRIGHT:   Copyright 2012 Hermès Bélusca
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#include <winnls.h>

#include <time.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* The unique user service start lock of the SCM */
static PSTART_LOCK pServiceStartLock = NULL;


/* FUNCTIONS *****************************************************************/

/*
 * Retrieves the user on whose behalf the current RPC call is made.
 * The returned buffer must be freed with HeapFree().
 */
static
DWORD
ScmGetRpcClientUser(
    _Out_ PTOKEN_USER *ppTokenUser)
{
    PTOKEN_USER pTokenUser;
    HANDLE hToken;
    DWORD dwSize = 0;
    DWORD dwError;

    *ppTokenUser = NULL;

    dwError = RpcImpersonateClient(NULL);
    if (dwError != RPC_S_OK)
        return dwError;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        dwError = GetLastError();

    RpcRevertToSelf();

    if (dwError != ERROR_SUCCESS)
        return dwError;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize == 0)
    {
        dwError = GetLastError();
        goto done;
    }

    pTokenUser = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (pTokenUser == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
    {
        dwError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pTokenUser);
        goto done;
    }

    *ppTokenUser = pTokenUser;

done:
    CloseHandle(hToken);

    return dwError;
}


/*
 * Returns the name of the given account as "DOMAIN\User",
 * or NULL on failure. The returned string must be freed with HeapFree().
 */
static
LPWSTR
ScmGetAccountName(
    _In_ PSID pSid)
{
    LPWSTR pszName;
    DWORD cchName = 0;
    DWORD cchDomain = 0;
    DWORD cchDomainBuffer;
    DWORD dwNameOffset = 0;
    SID_NAME_USE Use;

    /* Get the needed sizes, including the terminating NULLs */
    if (LookupAccountSidW(NULL, pSid, NULL, &cchName, NULL, &cchDomain, &Use) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return NULL;
    }

    /*
     * The domain name goes at the beginning of the buffer and the user name
     * after it. They are joined below, the backslash replacing the domain's
     * NULL terminator.
     */
    cchDomainBuffer = cchDomain;
    pszName = HeapAlloc(GetProcessHeap(), 0, (cchDomain + cchName) * sizeof(WCHAR));
    if (pszName == NULL)
        return NULL;

    if (!LookupAccountSidW(NULL, pSid, pszName + cchDomainBuffer, &cchName,
                           pszName, &cchDomain, &Use))
    {
        HeapFree(GetProcessHeap(), 0, pszName);
        return NULL;
    }

    /* On success, cchName and cchDomain do not include the NULL terminators */
    if (cchDomain != 0)
    {
        pszName[cchDomain] = L'\\';
        dwNameOffset = cchDomain + 1;
    }

    MoveMemory(pszName + dwNameOffset,
               pszName + cchDomainBuffer,
               (cchName + 1) * sizeof(WCHAR));

    return pszName;
}


/*
 * NOTE: IsServiceController is TRUE if locked by the
 * Service Control Manager, and FALSE otherwise.
 */
DWORD
ScmAcquireServiceStartLock(IN BOOL IsServiceController,
                           OUT LPSC_RPC_LOCK lpLock)
{
    PTOKEN_USER pTokenUser = NULL;
    DWORD dwRequiredSize;
    DWORD dwError = ERROR_SUCCESS;

    *lpLock = NULL;

    if (!IsServiceController)
    {
        /* Remember the user who acquires the lock. Not knowing it is not fatal. */
        dwError = ScmGetRpcClientUser(&pTokenUser);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("ScmGetRpcClientUser() failed (Error %lu)\n", dwError);
            dwError = ERROR_SUCCESS;
        }
    }

    /* Lock the service database exclusively */
    ScmLockDatabaseExclusive();

    if (pServiceStartLock != NULL)
    {
        dwError = ERROR_SERVICE_DATABASE_LOCKED;
        goto done;
    }

    /* Allocate a new lock for the database, the owner SID being stored after it */
    dwRequiredSize = sizeof(START_LOCK);

    if (pTokenUser != NULL)
        dwRequiredSize += GetLengthSid(pTokenUser->User.Sid);

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

    /* The owner SID stays NULL if the SCM acquired the lock */
    if (pTokenUser != NULL)
    {
        pServiceStartLock->LockOwnerSid = (PSID)(pServiceStartLock + 1);
        CopySid(dwRequiredSize - sizeof(START_LOCK),
                pServiceStartLock->LockOwnerSid,
                pTokenUser->User.Sid);
    }

    *lpLock = (LPSC_RPC_LOCK)pServiceStartLock;

done:
    /* Unlock the service database */
    ScmUnlockDatabase();

    if (pTokenUser != NULL)
        HeapFree(GetProcessHeap(), 0, pTokenUser);

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
 * Takes a snapshot of the lock status. The owner name is NULL if the
 * database is not locked, if the SCM locked it, or if the name could not
 * be retrieved. It must be freed with HeapFree().
 */
static
DWORD
ScmGetServiceLockStatus(
    _Out_ PBOOL pbIsLocked,
    _Out_ PDWORD pdwLockDuration,
    _Out_ LPWSTR *ppszLockOwner)
{
    PSID pOwnerSid = NULL;
    DWORD dwSidLength;
    DWORD dwError = ERROR_SUCCESS;

    *pbIsLocked = FALSE;
    *pdwLockDuration = 0;
    *ppszLockOwner = NULL;

    /* Lock the service database shared */
    ScmLockDatabaseShared();

    if (pServiceStartLock != NULL)
    {
        *pbIsLocked = TRUE;
        *pdwLockDuration = (DWORD)time(NULL) - pServiceStartLock->TimeWhenLocked;

        /* Copy the owner SID, as the lock may be released once we unlock the database */
        if (pServiceStartLock->LockOwnerSid != NULL)
        {
            dwSidLength = GetLengthSid(pServiceStartLock->LockOwnerSid);
            pOwnerSid = HeapAlloc(GetProcessHeap(), 0, dwSidLength);
            if (pOwnerSid != NULL)
                CopySid(dwSidLength, pOwnerSid, pServiceStartLock->LockOwnerSid);
            else
                dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    /* Unlock the service database */
    ScmUnlockDatabase();

    /* Look up the name only now, as it calls into the LSA */
    if (pOwnerSid != NULL)
    {
        *ppszLockOwner = ScmGetAccountName(pOwnerSid);
        HeapFree(GetProcessHeap(), 0, pOwnerSid);
    }

    return dwError;
}


/*
 * Helper functions for RQueryServiceLockStatusW() and
 * RQueryServiceLockStatusA(). The owner name is stored right
 * after the structure, and lpLockOwner receives its offset.
 */
DWORD
ScmQueryServiceLockStatusW(
    _Out_writes_bytes_(cbBufSize) LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
    _In_ DWORD cbBufSize,
    _Out_ LPDWORD pcbBytesNeeded)
{
    BOOL bIsLocked;
    DWORD dwLockDuration;
    LPWSTR pszLockOwner;
    DWORD dwRequiredSize;
    DWORD dwError;

    dwError = ScmGetServiceLockStatus(&bIsLocked, &dwLockDuration, &pszLockOwner);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwRequiredSize = sizeof(QUERY_SERVICE_LOCK_STATUSW) + sizeof(WCHAR);
    if (pszLockOwner != NULL)
        dwRequiredSize += (DWORD)wcslen(pszLockOwner) * sizeof(WCHAR);

    *pcbBytesNeeded = dwRequiredSize;

    if (cbBufSize < dwRequiredSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto done;
    }

    lpLockStatus->fIsLocked = bIsLocked;
    lpLockStatus->dwLockDuration = dwLockDuration;

    if (bIsLocked && pszLockOwner == NULL)
    {
        /* No owner name, e.g. the SCM holds the lock */
        lpLockStatus->lpLockOwner = NULL;
    }
    else
    {
        wcscpy((LPWSTR)(lpLockStatus + 1), pszLockOwner ? pszLockOwner : L"");
        lpLockStatus->lpLockOwner = (LPWSTR)(ULONG_PTR)sizeof(QUERY_SERVICE_LOCK_STATUSW);
    }

done:
    if (pszLockOwner != NULL)
        HeapFree(GetProcessHeap(), 0, pszLockOwner);

    return dwError;
}


DWORD
ScmQueryServiceLockStatusA(
    _Out_writes_bytes_(cbBufSize) LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
    _In_ DWORD cbBufSize,
    _Out_ LPDWORD pcbBytesNeeded)
{
    BOOL bIsLocked;
    DWORD dwLockDuration;
    LPWSTR pszLockOwner;
    INT cbLockOwner = 0;
    DWORD dwRequiredSize;
    DWORD dwError;

    dwError = ScmGetServiceLockStatus(&bIsLocked, &dwLockDuration, &pszLockOwner);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Size of the ANSI owner name, including the NULL terminator */
    if (pszLockOwner != NULL)
    {
        cbLockOwner = WideCharToMultiByte(CP_ACP, 0, pszLockOwner, -1,
                                          NULL, 0, NULL, NULL);
    }

    dwRequiredSize = sizeof(QUERY_SERVICE_LOCK_STATUSA) + max(cbLockOwner, 1);

    *pcbBytesNeeded = dwRequiredSize;

    if (cbBufSize < dwRequiredSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto done;
    }

    lpLockStatus->fIsLocked = bIsLocked;
    lpLockStatus->dwLockDuration = dwLockDuration;

    if (bIsLocked && cbLockOwner == 0)
    {
        /* No owner name, e.g. the SCM holds the lock */
        lpLockStatus->lpLockOwner = NULL;
    }
    else
    {
        if (cbLockOwner != 0)
        {
            WideCharToMultiByte(CP_ACP, 0, pszLockOwner, -1,
                                (LPSTR)(lpLockStatus + 1), cbLockOwner, NULL, NULL);
        }
        else
        {
            *(LPSTR)(lpLockStatus + 1) = '\0';
        }
        lpLockStatus->lpLockOwner = (LPSTR)(ULONG_PTR)sizeof(QUERY_SERVICE_LOCK_STATUSA);
    }

done:
    if (pszLockOwner != NULL)
        HeapFree(GetProcessHeap(), 0, pszLockOwner);

    return dwError;
}

/* EOF */
