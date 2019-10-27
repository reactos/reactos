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
R_ResolverFlushCache(
    DNSRSLVR_HANDLE pwszServerName)
{
    // FIXME Should store (and flush) entries by server handle
    DnsIntCacheFlush();
    return 0;
}

DWORD
R_ResolverQuery(
    DNSRSLVR_HANDLE pwszServerName,
    LPCWSTR pwsName,
    WORD wType,
    DWORD Flags,
    DWORD *dwRecords,
    DNS_RECORDW **ppResultRecords)
{
#if 0
    DNS_QUERY_REQUEST  QueryRequest = { 0 };
    DNS_QUERY_RESULT   QueryResults = { 0 };
#endif
    DNS_STATUS         Status;
    PDNS_RECORDW       Record;

    DPRINT1("R_ResolverQuery %p %p %x %lx %p %p\n",
            pwszServerName, pwsName, wType, Flags, dwRecords, ppResultRecords);

    if (!pwszServerName || !pwsName || !wType || !ppResultRecords)
        return ERROR_INVALID_PARAMETER;

    // FIXME Should lookup entries by server handle
    if (DnsIntCacheGetEntryFromName(pwsName, ppResultRecords))
    {
        Status = ERROR_SUCCESS;
    }
    else
    {
#if 0
        QueryRequest.Version = DNS_QUERY_REQUEST_VERSION1;
        QueryRequest.QueryType = wType;
        QueryRequest.QueryName = pwsName;
        QueryRequest.QueryOptions = Flags;
        QueryResults.Version = DNS_QUERY_REQUEST_VERSION1;

        Status = DnsQueryEx(&QueryRequest, &QueryResults, NULL);
        if (Status == ERROR_SUCCESS)
        {
            // FIXME Should store (and flush) entries by server handle
            DnsIntCacheAddEntry(QueryResults.pQueryRecords);
            *ppResultRecords = QueryResults.pQueryRecords;
        }
#endif
    }

    if (dwRecords)
        *dwRecords = 0;

    if (Status == ERROR_SUCCESS)
    {
        Record = *ppResultRecords;
        while (Record)
        {
            if (dwRecords)
                (*dwRecords)++;
            Record = Record->pNext;
        }
    }

    DPRINT1("R_ResolverQuery result %ld %ld\n", Status, *dwRecords);

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
