/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     SDB Debug heap functionality
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include "ntndk.h"


#if SDBAPI_DEBUG_ALLOC

#define TRACE_ALL_FREE_CALLS    1

typedef struct SHIM_ALLOC_ENTRY
{
    PVOID Address;
    SIZE_T Size;
    int Line;
    const char* File;
    PVOID Next;
    PVOID Prev;
} SHIM_ALLOC_ENTRY, *PSHIM_ALLOC_ENTRY;

static RTL_CRITICAL_SECTION g_SdbpAllocationLock;
static RTL_AVL_TABLE g_SdbpAllocationTable;
static HANDLE g_PrivAllocationHeap;

static RTL_GENERIC_COMPARE_RESULTS
NTAPI ShimAllocCompareRoutine(_In_ PRTL_AVL_TABLE Table, _In_ PVOID FirstStruct, _In_ PVOID SecondStruct)
{
    PVOID First = ((PSHIM_ALLOC_ENTRY)FirstStruct)->Address;
    PVOID Second = ((PSHIM_ALLOC_ENTRY)SecondStruct)->Address;

    if (First < Second)
        return GenericLessThan;
    else if (First == Second)
        return GenericEqual;
    return GenericGreaterThan;
}

static PVOID NTAPI ShimAllocAllocateRoutine(_In_ PRTL_AVL_TABLE Table, _In_ CLONG ByteSize)
{
    return RtlAllocateHeap(g_PrivAllocationHeap, HEAP_ZERO_MEMORY, ByteSize);
}

static VOID NTAPI ShimAllocFreeRoutine(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer)
{
    RtlFreeHeap(g_PrivAllocationHeap, 0, Buffer);
}

void SdbpInsertAllocation(PVOID address, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Entry = {0};

    Entry.Address = address;
    Entry.Size = size;
    Entry.Line = line;
    Entry.File = file;

    RtlEnterCriticalSection(&g_SdbpAllocationLock);
    RtlInsertElementGenericTableAvl(&g_SdbpAllocationTable, &Entry, sizeof(Entry), NULL);
    RtlLeaveCriticalSection(&g_SdbpAllocationLock);
}

void SdbpUpdateAllocation(PVOID address, PVOID newaddress, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;
    Lookup.Address = address;

    RtlEnterCriticalSection(&g_SdbpAllocationLock);
    Entry = RtlLookupElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup);

    if (address == newaddress)
    {
        Entry->Size = size;
    }
    else
    {
        Lookup.Address = newaddress;
        Lookup.Size = size;
        Lookup.Line = line;
        Lookup.File = file;
        Lookup.Prev = address;
        RtlInsertElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup, sizeof(Lookup), NULL);
        Entry->Next = newaddress;
    }
    RtlLeaveCriticalSection(&g_SdbpAllocationLock);
}

static void SdbpPrintSingleAllocation(PSHIM_ALLOC_ENTRY Entry)
{
    DbgPrint(" > %s(%d): %s%sAlloc( %d ) ==> %p\r\n", Entry->File, Entry->Line,
             Entry->Next ? "Invalidated " : "", Entry->Prev ? "Re" : "", Entry->Size, Entry->Address);

}

void SdbpRemoveAllocation(PVOID address, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;

#if TRACE_ALL_FREE_CALLS
    DbgPrint("\r\n===============\r\n%s(%d): SdbpFree called, tracing alloc:\r\n", file, line);
#endif

    Lookup.Address = address;
    RtlEnterCriticalSection(&g_SdbpAllocationLock);
    while (Lookup.Address)
    {
        Entry = RtlLookupElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup);
        if (Entry)
        {
            Lookup = *Entry;
            RtlDeleteElementGenericTableAvl(&g_SdbpAllocationTable, Entry);

#if TRACE_ALL_FREE_CALLS
            SdbpPrintSingleAllocation(&Lookup);
#endif
            Lookup.Address = Lookup.Prev;
        }
        else
        {
            Lookup.Address = NULL;
        }
    }
    RtlLeaveCriticalSection(&g_SdbpAllocationLock);
#if TRACE_ALL_FREE_CALLS
    DbgPrint("===============\r\n");
#endif
}

void SdbpDebugHeapInit(HANDLE privateHeapPtr)
{
    g_PrivAllocationHeap = privateHeapPtr;

    RtlInitializeCriticalSection(&g_SdbpAllocationLock);
    RtlInitializeGenericTableAvl(&g_SdbpAllocationTable, ShimAllocCompareRoutine,
        ShimAllocAllocateRoutine, ShimAllocFreeRoutine, NULL);
}

void SdbpDebugHeapDeinit(void)
{
    if (g_SdbpAllocationTable.NumberGenericTableElements != 0)
    {
        PSHIM_ALLOC_ENTRY Entry;

        DbgPrint("\r\n===============\r\n===============\r\nSdbpHeapDeinit: Dumping leaks\r\n");
        RtlEnterCriticalSection(&g_SdbpAllocationLock);
        Entry = RtlEnumerateGenericTableAvl(&g_SdbpAllocationTable, TRUE);

        while (Entry)
        {
            SdbpPrintSingleAllocation(Entry);
            Entry = RtlEnumerateGenericTableAvl(&g_SdbpAllocationTable, FALSE);
        }
        RtlLeaveCriticalSection(&g_SdbpAllocationLock);
        DbgPrint("===============\r\n===============\r\n");
    }
    /*__debugbreak();*/
    /*RtlDeleteCriticalSection(&g_SdbpAllocationLock);*/
}

#endif
