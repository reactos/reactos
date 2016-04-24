/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#include "strsafe.h"
#include "apphelp.h"

#include "wine/unicode.h"


static HANDLE SdbpHeap(void);

#if SDBAPI_DEBUG_ALLOC

typedef struct SHIM_ALLOC_ENTRY
{
    PVOID Address;
    SIZE_T Size;
    int Line;
    const char* File;
    PVOID Next;
    PVOID Prev;
} SHIM_ALLOC_ENTRY, *PSHIM_ALLOC_ENTRY;


static RTL_AVL_TABLE g_SdbpAllocationTable;


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
    return HeapAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, ByteSize);
}

static VOID NTAPI ShimAllocFreeRoutine(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer)
{
    HeapFree(SdbpHeap(), 0, Buffer);
}

static void SdbpInsertAllocation(PVOID address, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Entry = {0};

    Entry.Address = address;
    Entry.Size = size;
    Entry.Line = line;
    Entry.File = file;
    RtlInsertElementGenericTableAvl(&g_SdbpAllocationTable, &Entry, sizeof(Entry), NULL);
}

static void SdbpUpdateAllocation(PVOID address, PVOID newaddress, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;
    Lookup.Address = address;
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
}

static void SdbpRemoveAllocation(PVOID address, int line, const char* file)
{
    char buf[512];
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;

    sprintf(buf, "\r\n===============\r\n%s(%d): SdbpFree called, tracing alloc:\r\n", file, line);
    OutputDebugStringA(buf);

    Lookup.Address = address;
    while (Lookup.Address)
    {
        Entry = RtlLookupElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup);
        if (Entry)
        {
            Lookup = *Entry;
            RtlDeleteElementGenericTableAvl(&g_SdbpAllocationTable, Entry);

            sprintf(buf, " > %s(%d): %s%sAlloc( %d ) ==> %p\r\n", Lookup.File, Lookup.Line,
                Lookup.Next ? "Invalidated " : "", Lookup.Prev ? "Re" : "", Lookup.Size, Lookup.Address);
            OutputDebugStringA(buf);
            Lookup.Address = Lookup.Prev;
        }
        else
        {
            Lookup.Address = NULL;
        }
    }
    sprintf(buf, "\r\n===============\r\n");
    OutputDebugStringA(buf);
}

#endif

static HANDLE g_Heap;
void SdbpHeapInit(void)
{
#if SDBAPI_DEBUG_ALLOC
    RtlInitializeGenericTableAvl(&g_SdbpAllocationTable, ShimAllocCompareRoutine,
        ShimAllocAllocateRoutine, ShimAllocFreeRoutine, NULL);
#endif
    g_Heap = HeapCreate(0, 0x10000, 0);
}

void SdbpHeapDeinit(void)
{
#if SDBAPI_DEBUG_ALLOC
    if (g_SdbpAllocationTable.NumberGenericTableElements != 0)
        __debugbreak();
#endif
    HeapDestroy(g_Heap);
}

DWORD SdbpStrlen(PCWSTR string)
{
    return (lstrlenW(string) + 1) * sizeof(WCHAR);
}

static HANDLE SdbpHeap(void)
{
    return g_Heap;
}

LPVOID SdbpAlloc(SIZE_T size
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
    LPVOID mem = HeapAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, size);
#if SDBAPI_DEBUG_ALLOC
    SdbpInsertAllocation(mem, size, line, file);
#endif
    return mem;
}

LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
    LPVOID newmem = HeapReAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, mem, size);
#if SDBAPI_DEBUG_ALLOC
    SdbpUpdateAllocation(mem, newmem, size, line, file);
#endif
    return newmem;
}

void SdbpFree(LPVOID mem
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
#if SDBAPI_DEBUG_ALLOC
    SdbpRemoveAllocation(mem, line, file);
#endif
    HeapFree(SdbpHeap(), 0, mem);
}
