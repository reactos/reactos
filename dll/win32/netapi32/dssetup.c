/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/dssetup.c
 * PURPOSE:         Directory Service Setup interface code
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

#include <rpc.h>
#include "dssetup_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

static
RPC_STATUS
DsSetupBind(
    LPWSTR lpServerName,
    handle_t *hBinding)
{
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("DsSetupBind() called\n");

    *hBinding = NULL;

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      lpServerName,
                                      L"\\pipe\\lsarpc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return status;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return status;
}


static
void
DsSetupUnbind(
    handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("DsSetupUnbind()\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


VOID
WINAPI
DsRoleFreeMemory(
    _In_ PVOID Buffer)
{
    TRACE("DsRoleFreeMemory(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}


DWORD
WINAPI
DsRoleGetPrimaryDomainInformation(
    LPCWSTR lpServer,
    DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PBYTE* Buffer)
{
    handle_t hBinding = NULL;
    NET_API_STATUS status;

    TRACE("DsRoleGetPrimaryDomainInformation(%p, %d, %p)\n",
          lpServer, InfoLevel, Buffer);

    /* Check some input parameters */

    if (!Buffer)
        return ERROR_INVALID_PARAMETER;

    if ((InfoLevel < DsRolePrimaryDomainInfoBasic) || (InfoLevel > DsRoleOperationState))
        return ERROR_INVALID_PARAMETER;

    *Buffer = NULL;

    status = DsSetupBind((LPWSTR)lpServer, &hBinding);
    if (status)
    {
        TRACE("DsSetupBind() failed (Status %lu\n)\n", status);
        return status;
    }

    RpcTryExcept
    {
        status = DsRolerGetPrimaryDomainInformation(hBinding,
                                                    InfoLevel,
                                                    (PDSROLER_PRIMARY_DOMAIN_INFORMATION *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (hBinding != NULL)
        DsSetupUnbind(hBinding);

    return status;
}

/* EOF */
