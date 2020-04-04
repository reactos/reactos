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


/* Function: 0x00 */
DWORD
__stdcall
CRrReadCache(
    _In_ DNSRSLVR_HANDLE pwszServerName,
    _Out_ DNS_CACHE_ENTRY **ppCacheEntries)
{
    DPRINT("CRrReadCache(%S %p)\n",
           pwszServerName, ppCacheEntries);

    return DnsIntCacheGetEntries(ppCacheEntries);
}


/* Function: 0x04 */
DWORD
__stdcall
R_ResolverFlushCache(
    _In_ DNSRSLVR_HANDLE pwszServerName)
{
    DPRINT("R_ResolverFlushCache(%S)\n",
           pwszServerName);

    return DnsIntCacheFlush(CACHE_FLUSH_NON_HOSTS_FILE_ENTRIES);
}


/* Function: 0x07 */
DWORD
__stdcall
R_ResolverQuery(
    _In_ DNSRSLVR_HANDLE pszServerName,
    _In_ LPCWSTR pszName,
    _In_ WORD wType,
    _In_ DWORD dwFlags,
    _Inout_ DWORD *dwRecords,
    _Out_ DNS_RECORDW **ppResultRecords)
{
    PDNS_RECORDW Record;
    DNS_STATUS Status = ERROR_SUCCESS;

    DPRINT("R_ResolverQuery(%S %S %x %lx %p %p)\n",
           pszServerName, pszName, wType, dwFlags, dwRecords, ppResultRecords);

    if (pszName == NULL || wType == 0 || ppResultRecords == NULL)
        return ERROR_INVALID_PARAMETER;

    if ((dwFlags & DNS_QUERY_WIRE_ONLY) != 0 && (dwFlags & DNS_QUERY_NO_WIRE_QUERY) != 0)
        return ERROR_INVALID_PARAMETER;

    if (dwFlags & DNS_QUERY_WIRE_ONLY)
    {
        DPRINT("DNS query!\n");
        Status = Query_Main(pszName,
                            wType,
                            dwFlags,
                            ppResultRecords);
    }
    else if (dwFlags & DNS_QUERY_NO_WIRE_QUERY)
    {
        DPRINT("DNS cache query!\n");
        Status = DnsIntCacheGetEntryByName(pszName,
                                           wType,
                                           dwFlags,
                                           ppResultRecords);
    }
    else
    {
        DPRINT("DNS cache query!\n");
        Status = DnsIntCacheGetEntryByName(pszName,
                                           wType,
                                           dwFlags,
                                           ppResultRecords);
        if (Status == DNS_INFO_NO_RECORDS)
        {
            DPRINT("DNS query!\n");
            Status = Query_Main(pszName,
                                wType,
                                dwFlags,
                                ppResultRecords);
            if (Status == ERROR_SUCCESS)
            {
                DPRINT("DNS query successful!\n");
                DnsIntCacheAddEntry(*ppResultRecords, FALSE);
            }
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

void __RPC_USER DNSRSLVR_RPC_HANDLE_rundown(DNSRSLVR_HANDLE hClientHandle)
{
}
