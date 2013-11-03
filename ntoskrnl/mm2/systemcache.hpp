/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/systemcache.hpp
 * PURPOSE:         System cache header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

class SYSTEM_CACHE
{
public:
    NTSTATUS Initialize();

    ULONG GetWsMin() { return SystemCacheWsMin; }

private:
    WORKING_SET_LIST *SystemCacheWorkingSetList;
    WS_LIST_ENTRY *SystemCacheWsle;
    WORKING_SET SystemCacheWs;
    PVOID SystemCacheStart;
    PVOID SystemCacheEnd;
    ULONG SizeOfSystemCacheInPages;
    ULONG LargeSystemCache;
    ULONG SystemCacheWsMax;
    ULONG SystemCacheWsMin;
    ULONG FirstFreeSystemCache;
    ULONG LastFreeSystemCache;
    PTENTRY *SystemCachePtes;

    NTSTATUS InitializeInternal();
};
