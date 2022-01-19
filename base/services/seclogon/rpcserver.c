/*
 * PROJECT:     ReactOS Secondary Logon Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Secondary Logon service RPC server
 * COPYRIGHT:   Eric Kohl 2022 <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <seclogon_s.h>

WINE_DEFAULT_DEBUG_CHANNEL(seclogon);

/* FUNCTIONS *****************************************************************/


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


DWORD
StartRpcServer(VOID)
{
    ULONG Status;

    Status = ((LPSTART_RPC_SERVER)lpServiceGlobals->RpcpStartRpcServer)(L"seclogon", ISeclogon_v1_0_s_ifspec);
    TRACE("RpcpStartRpcServer returned 0x%08lx\n", Status);

    return RtlNtStatusToDosError(Status);
}


DWORD
StopRpcServer(VOID)
{
    ULONG Status;

    Status = ((LPSTOP_RPC_SERVER)lpServiceGlobals->RpcpStopRpcServer)(ISeclogon_v1_0_s_ifspec);
    TRACE("RpcpStopRpcServer returned 0x%08lx\n", Status);

    return RtlNtStatusToDosError(Status);
}


VOID
__stdcall
SeclCreateProcessWithLogonW(
    _In_ handle_t hBinding,
    _In_ SECL_REQUEST *pRequest,
    _Out_ SECL_RESPONSE *pResponse)
{
    TRACE("SeclCreateProcessWithLogonW(%p %p %p)\n", hBinding, pRequest, pResponse);

    if (pRequest != NULL)
    {
        TRACE("Username: '%S'\n", pRequest->Username);
        TRACE("Domain: '%S'\n", pRequest->Domain);
        TRACE("Password: '%S'\n", pRequest->Password);
        TRACE("ApplicationName: '%S'\n", pRequest->ApplicationName);
        TRACE("CommandLine: '%S'\n", pRequest->CommandLine);
        TRACE("CurrentDirectory: '%S'\n", pRequest->CurrentDirectory);
    }

    /* FIXME: Logon */

    /* FIXME: Create Process */

    if (pResponse != NULL)
        pResponse->ulError = 4;
}
