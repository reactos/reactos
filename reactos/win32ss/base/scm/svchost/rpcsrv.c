/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/rpcsrv.c
 * PURPOSE:     RPC Service Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

/* GLOBALS *******************************************************************/

LONG RpcpNumInstances;
CRITICAL_SECTION RpcpCriticalSection;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
RpcpInitRpcServer (
    VOID
    )
{
    /* Clear the reference count and initialize the critical section */
    RpcpNumInstances = 0;
    return RtlInitializeCriticalSection((PVOID)&RpcpCriticalSection);
}

NTSTATUS
NTAPI
RpcpStopRpcServer (
    _In_ RPC_IF_HANDLE IfSpec
    )
{
    RPC_STATUS rpcStatus;

    /* Unregister the interface */
    rpcStatus = RpcServerUnregisterIf(IfSpec, NULL, TRUE);

    /* Acquire the lock while we dereference the RPC services */
    EnterCriticalSection(&RpcpCriticalSection);
    if (--RpcpNumInstances == 0)
    {
        /* All RPC services stopped, rundown the server */
        RpcMgmtStopServerListening(NULL);
        RpcMgmtWaitServerListen();
    }

    /* Release the lock and return the unregister result */
    LeaveCriticalSection(&RpcpCriticalSection);
    return I_RpcMapWin32Status(rpcStatus);
}

NTSTATUS
NTAPI
RpcpStopRpcServerEx (
    _In_ RPC_IF_HANDLE IfSpec
    )
{
    RPC_STATUS rpcStatus;

    /* Unregister the interface */
    rpcStatus = RpcServerUnregisterIfEx(IfSpec, NULL, TRUE);

    /* Acquire the lock while we dereference the RPC services */
    EnterCriticalSection(&RpcpCriticalSection);
    if (--RpcpNumInstances == 0)
    {
        /* All RPC services stopped, rundown the server */
        RpcMgmtStopServerListening(NULL);
        RpcMgmtWaitServerListen();
    }

    /* Release the lock and return the unregister result */
    LeaveCriticalSection(&RpcpCriticalSection);
    return I_RpcMapWin32Status(rpcStatus);
}

NTSTATUS
NTAPI
RpcpAddInterface (
    _In_ LPCWSTR IfName,
    _In_ RPC_IF_HANDLE IfSpec
    )
{
    PWCHAR endpointName;
    NTSTATUS ntStatus;
    RPC_STATUS rpcStatus;

    /* Allocate space for the interface name and the \\PIPE\\ prefix */
    endpointName = LocalAlloc(0, sizeof(WCHAR) * wcslen(IfName) + 16);
    if (endpointName)
    {
        /* Copy the prefix, and then the interface name */
        wcscpy(endpointName, L"\\PIPE\\");
        wcscat(endpointName, IfName);

        /* Create a named pipe endpoint with this name */
        rpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", 10, endpointName, NULL);
        if ((rpcStatus != RPC_S_OK) && (rpcStatus != RPC_S_DUPLICATE_ENDPOINT))
        {
            /* We couldn't create it, or it already existed... */
            DbgPrint("RpcServerUseProtseqW failed! rpcstatus = %u\n", rpcStatus);
        }
        else
        {
            /* It worked, register an interface on this endpoint now*/
            rpcStatus = RpcServerRegisterIf(IfSpec, 0, 0);
        }

        /* In both success and failure, free the name, and convert the status */
        LocalFree(endpointName);
        ntStatus = I_RpcMapWin32Status(rpcStatus);
    }
    else
    {
        /* No memory, bail out */
        ntStatus = STATUS_NO_MEMORY;
    }

    /* Return back to the caller */
    return ntStatus;
}

NTSTATUS
NTAPI
RpcpStartRpcServer (
    _In_ LPCWSTR IfName,
    _In_ RPC_IF_HANDLE IfSpec
    )
{
    NTSTATUS ntStatus;

    /* Acquire the lock while we instantiate a new interface */
    EnterCriticalSection(&RpcpCriticalSection);

    /* Add this interface to the service */
    ntStatus = RpcpAddInterface(IfName, IfSpec);
    if (!ntStatus)
    {
        /* Increment the reference count to see if this was the first interface */
        if (++RpcpNumInstances == 1)
        {
            /* It was, so put the server into listening mode now */
            ntStatus = RpcServerListen(1, 12345, TRUE);
            if (ntStatus == RPC_S_ALREADY_LISTENING) ntStatus = STATUS_SUCCESS;
        }
    }

    /* Release the lock and return back the result to the caller */
    LeaveCriticalSection(&RpcpCriticalSection);
    return I_RpcMapWin32Status(ntStatus);
}

