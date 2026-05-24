/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/api.c
 * PURPOSE:          DHCP client api handlers
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

#define NDEBUG
#include <reactos/debug.h>

static CRITICAL_SECTION ApiCriticalSection;

extern HANDLE hAdapterStateChangedEvent;

VOID ApiInit() {
    InitializeCriticalSection( &ApiCriticalSection );
}

VOID ApiLock() {
    EnterCriticalSection( &ApiCriticalSection );
}

VOID ApiUnlock() {
    LeaveCriticalSection( &ApiCriticalSection );
}

VOID ApiFree() {
    DeleteCriticalSection( &ApiCriticalSection );
}

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\dhcpcsvc", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(Server_dhcpcsvc_v0_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}

HANDLE
InitRpc(VOID)
{
    return CreateThread( NULL, 0, RpcThreadRoutine, (LPVOID)NULL, 0, NULL);
}

VOID
ShutdownRpc(VOID)
{
    RpcMgmtStopServerListening(NULL);
}

/* This represents the service portion of the DHCP client API */

/* Function 0 */
DWORD
__stdcall
Server_EnableDhcp(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName,
    _In_ BOOL Enable)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT1("Server_EnableDhcp(%S %u)\n", AdapterName, Enable);

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
    if (Adapter == NULL)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT1("Adapter: %p\n", Adapter);

    if (Enable)
    {
        DPRINT1("Enable DHCP for Adapter: %p\n", Adapter);

        if (Adapter->DhclientState.state != S_STATIC)
        {
            DPRINT1("The Adapter is already enabled!\n");
            goto done;
        }

        add_protocol(Adapter->DhclientInfo.name,
                     Adapter->DhclientInfo.rfdesc, got_one,
                     &Adapter->DhclientInfo);

        Adapter->DhclientInfo.client->state = S_INIT;
        state_reboot(&Adapter->DhclientInfo);
    }
    else
    {
        DPRINT1("Disable DHCP for Adapter: %p\n", Adapter);

        if (Adapter->DhclientState.state == S_STATIC)
        {
            DPRINT1("The Adapter is already disabled!\n");
            goto done;
        }

        Adapter->DhclientState.state = S_STATIC;
        proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
        if (proto)
            remove_protocol(proto);
    }

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return ret;
}

/* Function 1 */
DWORD
__stdcall
Server_AcquireParameters(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_AcquireParameters(%S)\n", AdapterName);

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
    if (Adapter == NULL || Adapter->DhclientState.state == S_STATIC)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    add_protocol(Adapter->DhclientInfo.name,
                 Adapter->DhclientInfo.rfdesc, got_one,
                 &Adapter->DhclientInfo);

    Adapter->DhclientInfo.client->state = S_INIT;
    state_reboot(&Adapter->DhclientInfo);

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return ret;
}

/* Function 2 */
DWORD
__stdcall
Server_AcquireParametersByBroadcast(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    DPRINT1("Server_AcquireParametersByBroadcast(%S) is unimplemented!\n", AdapterName);
    return ERROR_SUCCESS;
}

/* Function 3 */
DWORD
__stdcall
Server_ReleaseParameters(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_ReleaseParameters(%S)\n", AdapterName);

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
    if (Adapter == NULL)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    state_release(&Adapter->DhclientInfo);

    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return ret;
}

/* Function 4 */
DWORD
__stdcall
Server_FallbackRefreshParams(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    PDHCP_ADAPTER Adapter;
    HKEY hAdapterKey;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_FallbackRefreshParams(%S)\n", AdapterName);

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
    if (Adapter == NULL)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    if (Adapter->AlternateConfiguration)
    {
        free(Adapter->AlternateConfiguration);
        Adapter->AlternateConfiguration = NULL;
    }

    hAdapterKey = FindAdapterKey(Adapter);
    if (hAdapterKey)
    {
        ret = LoadAlternateConfiguration(Adapter, hAdapterKey);
        RegCloseKey(hAdapterKey);
    }

done:
    ApiUnlock();

    return ret;
}


/* Function 5 */
DWORD
__stdcall
Server_StaticRefreshParams(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ DWORD AdapterIndex,
    _In_ DWORD Address,
    _In_ DWORD Netmask)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_StaticRefreshParams()\n");

    ApiLock();

    Adapter = AdapterFindIndex(AdapterIndex);
    if (Adapter == NULL)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    if (Adapter->NteContext)
    {
        DeleteIPAddress(Adapter->NteContext);
        Adapter->NteContext = 0;
    }
    if (Adapter->RouterMib.dwForwardNextHop)
    {
        DeleteIpForwardEntry(&Adapter->RouterMib);
        Adapter->RouterMib.dwForwardNextHop = 0;
    }

    Adapter->DhclientState.state = S_STATIC;
    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    ret = AddIPAddress(Address,
                       Netmask,
                       AdapterIndex,
                       &Adapter->NteContext,
                       &Adapter->NteInstance);

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return ret;
}

/* Function 6 */
DWORD
__stdcall
Server_RemoveDNSRegistrations(
    _In_ PDHCP_SERVER_NAME ServerName)
{
    DPRINT1("Server_RemoveDNSRegistrations()\n");
    /* FIXME: Call dnsapi.DnsRemoveRegistrations() */
    return ERROR_SUCCESS;
}

/* Function 7 */
DWORD
__stdcall
Server_RequestParams(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName,
    _In_ DHCPCAPI_CLASSID *ClassId,
    _In_ DHCPCAPI_PARAMS_ARRAY *SendParams,
    _In_ DWORD Unknown5,
    _In_ DWORD Unknown6)
{
    DPRINT1("Server_RequestParams(%S %p %p %lx %lx)\n",
            AdapterName, ClassId, SendParams, Unknown5, Unknown6);

    if (SendParams != NULL)
    {
        DPRINT1("SendParams nParams %lu  Params %p\n", SendParams->nParams, SendParams->Params);
    }

    return ERROR_SUCCESS;
}
