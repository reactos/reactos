/*
 * PROJECT:     ReactOS DNS Resolver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/dnsrslvr/rpcserver.c
 * PURPOSE:     RPC server interface
 * COPYRIGHT:   Copyright 2016 Christoph von Wittich
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

DWORD
WINAPI
RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncalrpc", 20, L"DNSResolver", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(DnsResolver_v2_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    DPRINT("RpcServerListen finished\n");
    return 0;
}

DWORD
__stdcall
R_ResolverFlushCache(
    DNSRSLVR_HANDLE pwszServerName)
{
    DPRINT("R_ResolverFlushCache()\n");

    // FIXME Should store (and flush) entries by server handle
    DnsIntCacheFlush();
    return 0;
}

DWORD
__stdcall
R_ResolverQuery(
    DNSRSLVR_HANDLE pszServerName,
    LPCWSTR pszName,
    WORD wType,
    DWORD dwFlags,
    DWORD *dwRecords,
    DNS_RECORDW **ppResultRecords)
{
    PDNS_RECORDW Record;
    DNS_STATUS Status;

    DPRINT("R_ResolverQuery(%S %S %x %lx %p %p)\n",
           pszServerName, pszName, wType, dwFlags, dwRecords, ppResultRecords);

    if (pszName == NULL || wType == 0 || ppResultRecords == NULL)
        return ERROR_INVALID_PARAMETER;

    if ((dwFlags & DNS_QUERY_WIRE_ONLY) != 0 && (dwFlags & DNS_QUERY_NO_WIRE_QUERY) != 0)
        return ERROR_INVALID_PARAMETER;

    if (DnsIntCacheGetEntryFromName(pszName, ppResultRecords))
    {
        DPRINT("DNS cache query successful!\n");
        Status = ERROR_SUCCESS;
    }
    else
    {
        DPRINT("DNS query!\n");
        Status = Query_Main(pszName,
                            wType,
                            dwFlags,
                            ppResultRecords);
        if (Status == ERROR_SUCCESS)
        {
            DPRINT("DNS query successful!\n");
            DnsIntCacheAddEntry(*ppResultRecords);
        }
    }

    if (dwRecords)
    {
        *dwRecords = 0;

        if (Status == ERROR_SUCCESS)
        {
            Record = *ppResultRecords;
            while (Record)
            {
                DPRINT("Record: %S\n", Record->pName);
                (*dwRecords)++;
                Record = Record->pNext;
            }
        }
    }

    DPRINT("R_ResolverQuery result %ld %ld\n", Status, *dwRecords);

    return Status;
}

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

void __RPC_USER WLANSVC_RPC_HANDLE_rundown(DNSRSLVR_HANDLE hClientHandle)
{
}
