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
Server_AcquireParameters(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_AcquireParameters()\n");

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


/* Function 1 */
DWORD
__stdcall
Server_ReleaseParameters(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ LPWSTR AdapterName)
{
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_ReleaseParameters()\n");

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
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

    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    Adapter->DhclientInfo.client->active = NULL;
    Adapter->DhclientInfo.client->state = S_INIT;

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return ret;
}

/* Function 2 */
DWORD
__stdcall
Server_QueryHWInfo(
    _In_ PDHCP_SERVER_NAME ServerName,
    _In_ DWORD AdapterIndex,
    _Out_ PDWORD MediaType,
    _Out_ PDWORD Mtu,
    _Out_ PDWORD Speed)
{
    PDHCP_ADAPTER Adapter;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_QueryHWInfo()\n");

    ApiLock();

    Adapter = AdapterFindIndex(AdapterIndex);
    if (Adapter == NULL)
    {
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    *MediaType = Adapter->IfMib.dwType;
    *Mtu = Adapter->IfMib.dwMtu;
    *Speed = Adapter->IfMib.dwSpeed;

done:
    ApiUnlock();

    return ret;
}

/* Function 3 */
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
