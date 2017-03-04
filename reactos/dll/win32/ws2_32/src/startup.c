/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/startup.c
 * PURPOSE:     Startup/Cleanup Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

PWS_SOCK_POST_ROUTINE WsSockPostRoutine = NULL;
CRITICAL_SECTION WsStartupLock;

#define WsStartupLock()     EnterCriticalSection(&WsStartupLock)
#define WsStartupUnlock()   LeaveCriticalSection(&WsStartupLock)

/* FUNCTIONS *****************************************************************/

VOID
WSAAPI
WsCreateStartupSynchronization(VOID)
{
    /* Initialize the startup lock */
    InitializeCriticalSection(&WsStartupLock);
}

VOID
WSAAPI
WsDestroyStartupSynchronization(VOID)
{
    /* Destroy the startup lock */
    DeleteCriticalSection(&WsStartupLock);
}

/*
 * @implemented
 */
BOOL
WSAAPI
WSApSetPostRoutine(PVOID Routine)
{
    /* Set the post routine */
    DPRINT("WSApSetPostRoutine: %p\n", Routine);
    WsSockPostRoutine = (PWS_SOCK_POST_ROUTINE)Routine;
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
INT
WSAAPI
WSACleanup(VOID)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    LONG RefCount;
    DPRINT("WSACleanup\n");

    /* Enter startup lock */
    WsStartupLock();

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Decrement process reference count and check if it's zero */
        if (!(RefCount = InterlockedDecrement(&Process->RefCount)))
        {
            /* It's zero, destroy the process structure */
            WsProcDelete(Process);
        }
        else if (RefCount == 1 && WsAsyncThreadInitialized)
        {
            /* Kill async thread */
            WsAsyncTerminateThread();
        }

        DPRINT("WSACleanup RefCount = %ld\n", RefCount);
        /* Return success */
        ErrorCode = ERROR_SUCCESS;

        /* Clear last error */
        SetLastError(ERROR_SUCCESS);
    }
    else
    {
        DPRINT("WSACleanup uninitialized\n");
        /* Weren't initialized */
        SetLastError(ErrorCode);
        ErrorCode = SOCKET_ERROR;
    }

    /* Release startup lock */
    WsStartupUnlock();

    /* Done */
    return ErrorCode;
}

/*
 * @implemented
 */
INT
WINAPI
WSAStartup(IN WORD wVersionRequested,
           OUT LPWSADATA lpWSAData)
{
    WORD VersionReturned = 0;
    DWORD ErrorCode = ERROR_SUCCESS;
    PWSPROCESS CurrentProcess;
    DPRINT("WSAStartup: %wx %d.%d\n", wVersionRequested, LOBYTE(wVersionRequested), HIBYTE(wVersionRequested));

    /* Make sure that we went through DLL Init */
    if (!WsDllHandle) return WSASYSNOTREADY;

    /* Check which version is being requested */
    switch (LOBYTE(wVersionRequested))
    {
        case 0:

            /* We don't support this unknown version */
            ErrorCode = WSAVERNOTSUPPORTED;
            VersionReturned = MAKEWORD(2, 2);
            break;

        case 1:
            /* We support only 1.0 and 1.1 */
            if (HIBYTE(wVersionRequested) <= 1)
            {
                /* Caller wants 1.0, return it */
                VersionReturned = MAKEWORD(1, HIBYTE(wVersionRequested));
            }
            else
            {
                /* The only other version we support is 1.1 */
                VersionReturned = MAKEWORD(1, 1);
            }
            break;

        case 2:
            /* We support only 2.0, 2.1 and 2.2 */
            if (HIBYTE(wVersionRequested) <= 2)
            {
                /* Caller wants 2.0-2.2, return it */
                VersionReturned = MAKEWORD(2, HIBYTE(wVersionRequested));
            }
            else
            {
                /* The highest version we support is 2.2 */
                VersionReturned = MAKEWORD(2, 2);
            }
            break;

        default:

            /* Return 2.2 */
            VersionReturned = MAKEWORD(2, 2);
            break;
    }

    if (lpWSAData == NULL)
    {
        SetLastError(WSANOTINITIALISED);
        return ErrorCode == ERROR_SUCCESS ? WSAEFAULT : ErrorCode;
    }

    /* Return the Version Requested, unless error */
    lpWSAData->wVersion = VersionReturned;

    /* We support Winsock 2.2 */
    lpWSAData->wHighVersion = MAKEWORD(2,2);
    lstrcpy(lpWSAData->szDescription, "WinSock 2.0");
    lstrcpy(lpWSAData->szSystemStatus, "Running");

    /*
     * On Winsock 1, the following values are returned.
     * Taken straight from a Winsock Test app on Windows.
     */
    if (LOBYTE(wVersionRequested) == 1)
    {
        lpWSAData->iMaxSockets = 32767;
        lpWSAData->iMaxUdpDg = 65467;
    }
    else
    {
        lpWSAData->iMaxSockets = 0;
        lpWSAData->iMaxUdpDg = 0;
    }

    /* Requested invalid version (0) */
    if (ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(WSANOTINITIALISED);
        return ErrorCode;
    }

    /* Enter the startup synchronization lock */
    WsStartupLock();

    /* Now setup all our objects */
    while (TRUE)
    {
        /* Make sure we don't already have a process */
        CurrentProcess = WsGetProcess();
        if (CurrentProcess) break;

        /* Setup the process object support */
        ErrorCode = WsProcStartup();
        if (ErrorCode != ERROR_SUCCESS) break;

        /* Setup the process object support */
        ErrorCode = WsSockStartup();
        if (ErrorCode != ERROR_SUCCESS) break;

        /* Setup the process object support */
        ErrorCode = WsThreadStartup();
        if (ErrorCode != ERROR_SUCCESS) break;

        /* Try getting the process now */
        CurrentProcess = WsGetProcess();
        if (!CurrentProcess)
        {
            /* Something is weird... */
            ErrorCode = WSASYSNOTREADY;
            break;
        }
    }

    /* Check if all worked */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Set the requested version */
        WsProcSetVersion(CurrentProcess, wVersionRequested);

        /* Increase the reference count */
        InterlockedIncrement(&CurrentProcess->RefCount);
        DPRINT("WSAStartup RefCount = %ld\n", CurrentProcess->RefCount);

        /* Clear last error */
        SetLastError(ERROR_SUCCESS);
    }
    else
    {
        SetLastError(WSANOTINITIALISED);
    }

    /* Leave the startup lock */
    WsStartupUnlock();

    /* Return any Error */
    return ErrorCode;
}
