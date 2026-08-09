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
    _In_ DHCPCAPI_PARAMS_ARRAY *RecdParams,
    _Inout_ LPDHCPCAPI_RESULT_ARRAY *RecdResults)
{
    PDHCP_ADAPTER Adapter;
    DHCPCAPI_RESULT_ARRAY *Results = NULL;
    DWORD i, dwReturnCount, dwReturnLength;
    DWORD OptionId, DataSize, Offset, Index;
	PBYTE Data;
    DWORD ret = ERROR_SUCCESS;

    DPRINT("Server_RequestParams(%S %p %p %p %p)\n",
           AdapterName, ClassId, SendParams, RecdParams, RecdResults);

#if 0
    if (SendParams != NULL)
    {
        DPRINT1("SendParams nParams %lu  Params %p\n", SendParams->nParams, SendParams->Params);
        for (i = 0; i < SendParams->nParams; i++)
        {
            DPRINT1("SendParam %lu: Option %lu  Vendor %u\n", i, SendParams->Params[i].OptionId, SendParams->Params[i].IsVendor);
        }
    }

    if (RecdParams != NULL)
    {
        DPRINT1("RecdParams nParams %lu  Params %p\n", RecdParams->nParams, RecdParams->Params);
        for (i = 0; i < RecdParams->nParams; i++)
        {
            DPRINT1("RecdParam %lu: Option %lu  Vendor %u\n", i, RecdParams->Params[i].OptionId, RecdParams->Params[i].IsVendor);
        }
    }
#endif

    ApiLock();

    Adapter = AdapterFindName(AdapterName);
    if (Adapter == NULL)
    {
        DPRINT1("Adapter not found\n");
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT("Adapter: %p\n", Adapter);

    if (Adapter->DhclientState.state != S_BOUND)
    {
        DPRINT1("Adapter is not in S_BOUND state!\n");
        ret = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    dwReturnCount = 0;
    dwReturnLength = 0;

    DPRINT("ActiveLease: %p \n", Adapter->DhclientState.active);
    if (Adapter->DhclientState.active)
    {
        for (i = 0; i < RecdParams->nParams; i++)
        {
            if (RecdParams->Params[i].IsVendor == FALSE)
            {
                OptionId = RecdParams->Params[i].OptionId;
                DPRINT("Option %u: Length %d \n", OptionId, Adapter->DhclientState.active->options[OptionId].len);
                if (Adapter->DhclientState.active->options[OptionId].len != 0)
                {
                    dwReturnLength += Adapter->DhclientState.active->options[OptionId].len;
                    dwReturnCount++;
                }
            }
        }

        DPRINT("Return count: %lu  Return length: %lu\n", dwReturnCount, dwReturnLength);

        Results = MIDL_user_allocate(sizeof(DHCPCAPI_RESULT_ARRAY));
        if (Results == NULL)
        {
            DPRINT1("Result allocation failed!\n");
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        Results->ResultsCount = dwReturnCount;
        Results->Results = MIDL_user_allocate(dwReturnCount * sizeof(DHCPCAPI_RESULTS));
        if (Results->Results == NULL)
        {
            DPRINT1("Results allocation failed!\n");
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        Results->DataSize = dwReturnLength;
        Results->Data = MIDL_user_allocate(dwReturnLength);
        if (Results->Data == NULL)
        {
            DPRINT1("Data allocation failed!\n");
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        Offset = 0;
        Index = 0;
        for (i = 0; i < RecdParams->nParams; i++)
        {
            if (RecdParams->Params[i].IsVendor == FALSE)
            {
                OptionId = RecdParams->Params[i].OptionId;
                DataSize = Adapter->DhclientState.active->options[OptionId].len;
                Data = Adapter->DhclientState.active->options[OptionId].data;

                DPRINT("Option %u: Length %d \n", OptionId, DataSize);

                if (DataSize != 0)
                {
                    Results->Results[Index].OptionId = OptionId;
                    Results->Results[Index].IsVendor = RecdParams->Params[i].IsVendor;
                    Results->Results[Index].DataSize = DataSize;
                    Results->Results[Index].DataOffset = Offset;

                    CopyMemory(&Results->Data[Offset], Data, DataSize);

                    Index++;
                    Offset += DataSize;
                }
            }
        }

        *RecdResults = Results;
    }

done:


    ApiUnlock();

    return ret;
}
