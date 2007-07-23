/*
 * Full Pointer Translation Routines
 *
 * Copyright 2006 Robert Shearman
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

PFULL_PTR_XLAT_TABLES WINAPI NdrFullPointerXlatInit(ULONG NumberOfPointers,
                                                    XLAT_SIDE XlatSide)
{
    ULONG NumberOfBuckets;
    PFULL_PTR_XLAT_TABLES pXlatTables = HeapAlloc(GetProcessHeap(), 0, sizeof(*pXlatTables));

    TRACE("(%d, %d)\n", NumberOfPointers, XlatSide);

    if (!NumberOfPointers) NumberOfPointers = 512;
    NumberOfBuckets = ((NumberOfPointers + 3) & ~3) - 1;

    pXlatTables->RefIdToPointer.XlatTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(void *) * NumberOfPointers);
    pXlatTables->RefIdToPointer.StateTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(unsigned char) * NumberOfPointers);
    pXlatTables->RefIdToPointer.NumberOfEntries = NumberOfPointers;

    TRACE("NumberOfBuckets = %d\n", NumberOfBuckets);
    pXlatTables->PointerToRefId.XlatTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(PFULL_PTR_TO_REFID_ELEMENT) * NumberOfBuckets);
    pXlatTables->PointerToRefId.NumberOfBuckets = NumberOfBuckets;
    pXlatTables->PointerToRefId.HashMask = NumberOfBuckets - 1;

    pXlatTables->NextRefId = 1;
    pXlatTables->XlatSide = XlatSide;

    return pXlatTables;
}

void WINAPI NdrFullPointerXlatFree(PFULL_PTR_XLAT_TABLES pXlatTables)
{
    TRACE("(%p)\n", pXlatTables);

    HeapFree(GetProcessHeap(), 0, pXlatTables->RefIdToPointer.XlatTable);
    HeapFree(GetProcessHeap(), 0, pXlatTables->RefIdToPointer.StateTable);
    HeapFree(GetProcessHeap(), 0, pXlatTables->PointerToRefId.XlatTable);

    HeapFree(GetProcessHeap(), 0, pXlatTables);
}

static void expand_pointer_table_if_necessary(PFULL_PTR_XLAT_TABLES pXlatTables, ULONG RefId)
{
    if (RefId >= pXlatTables->RefIdToPointer.NumberOfEntries)
    {
        pXlatTables->RefIdToPointer.NumberOfEntries = RefId * 2;
        pXlatTables->RefIdToPointer.XlatTable =
            HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                pXlatTables->RefIdToPointer.XlatTable,
                sizeof(void *) * pXlatTables->RefIdToPointer.NumberOfEntries);
        pXlatTables->RefIdToPointer.StateTable =
            HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                pXlatTables->RefIdToPointer.StateTable,
                sizeof(unsigned char) * pXlatTables->RefIdToPointer.NumberOfEntries);

        if (!pXlatTables->RefIdToPointer.XlatTable || !pXlatTables->RefIdToPointer.StateTable)
            pXlatTables->RefIdToPointer.NumberOfEntries = 0;
    }
}

int WINAPI NdrFullPointerQueryPointer(PFULL_PTR_XLAT_TABLES pXlatTables,
                                      void *pPointer, unsigned char QueryType,
                                      ULONG *pRefId )
{
    ULONG Hash = 0;
    int i;
    PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;

    TRACE("(%p, %p, %d, %p)\n", pXlatTables, pPointer, QueryType, pRefId);

    if (!pPointer)
    {
        *pRefId = 0;
        return 1;
    }

    /* simple hashing algorithm, don't know whether it matches native */
    for (i = 0; i < sizeof(pPointer); i++)
        Hash = (Hash * 3) ^ ((unsigned char *)&pPointer)[i];

    XlatTableEntry = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    for (; XlatTableEntry; XlatTableEntry = XlatTableEntry->Next)
        if (pPointer == XlatTableEntry->Pointer)
        {
            *pRefId = XlatTableEntry->RefId;
            if (XlatTableEntry->State & QueryType)
                return 1;
            XlatTableEntry->State |= QueryType;
            return 0;
        }

    XlatTableEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(*XlatTableEntry));
    XlatTableEntry->Next = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    XlatTableEntry->Pointer = pPointer;
    XlatTableEntry->RefId = *pRefId = pXlatTables->NextRefId++;
    XlatTableEntry->State = QueryType;
    pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask] = XlatTableEntry;

    /* insert pointer into mapping table */
    expand_pointer_table_if_necessary(pXlatTables, XlatTableEntry->RefId);
    if (pXlatTables->RefIdToPointer.NumberOfEntries > XlatTableEntry->RefId)
    {
        pXlatTables->RefIdToPointer.XlatTable[XlatTableEntry->RefId] = pPointer;
        pXlatTables->RefIdToPointer.StateTable[XlatTableEntry->RefId] = QueryType;
    }

    return 0;
}

int WINAPI NdrFullPointerQueryRefId(PFULL_PTR_XLAT_TABLES pXlatTables,
                                    ULONG RefId, unsigned char QueryType,
                                    void **ppPointer)
{
    TRACE("(%p, 0x%x, %d, %p)\n", pXlatTables, RefId, QueryType, ppPointer);

    expand_pointer_table_if_necessary(pXlatTables, RefId);

    pXlatTables->NextRefId = max(RefId + 1, pXlatTables->NextRefId);

    if (pXlatTables->RefIdToPointer.NumberOfEntries > RefId)
    {
        *ppPointer = pXlatTables->RefIdToPointer.XlatTable[RefId];
        if (QueryType)
        {
            if (pXlatTables->RefIdToPointer.StateTable[RefId] & QueryType)
                return 1;
            pXlatTables->RefIdToPointer.StateTable[RefId] |= QueryType;
            return 0;
        }
        else
            return 0;
    }
    *ppPointer = NULL;
    return 0;
}

void WINAPI NdrFullPointerInsertRefId(PFULL_PTR_XLAT_TABLES pXlatTables,
                                      ULONG RefId, void *pPointer)
{
    ULONG Hash = 0;
    int i;
    PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;

    TRACE("(%p, 0x%x, %p)\n", pXlatTables, RefId, pPointer);

    /* simple hashing algorithm, don't know whether it matches native */
    for (i = 0; i < sizeof(pPointer); i++)
        Hash = (Hash * 3) ^ ((unsigned char *)&pPointer)[i];

    XlatTableEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(*XlatTableEntry));
    XlatTableEntry->Next = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    XlatTableEntry->Pointer = pPointer;
    XlatTableEntry->RefId = RefId;
    XlatTableEntry->State = 0;
    pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask] = XlatTableEntry;

    /* insert pointer into mapping table */
    expand_pointer_table_if_necessary(pXlatTables, RefId);
    if (pXlatTables->RefIdToPointer.NumberOfEntries > RefId)
        pXlatTables->RefIdToPointer.XlatTable[XlatTableEntry->RefId] = pPointer;
}

int WINAPI NdrFullPointerFree(PFULL_PTR_XLAT_TABLES pXlatTables, void *Pointer)
{
    ULONG Hash = 0;
    int i;
    PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;
    ULONG RefId = 0;

    TRACE("(%p, %p)\n", pXlatTables, Pointer);

    if (!Pointer)
        return 1;

    /* simple hashing algorithm, don't know whether it matches native */
    for (i = 0; i < sizeof(Pointer); i++)
        Hash = (Hash * 3) ^ ((unsigned char *)&Pointer)[i];

    XlatTableEntry = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    for (; XlatTableEntry; XlatTableEntry = XlatTableEntry->Next)
        if (Pointer == XlatTableEntry->Pointer)
        {
            if (XlatTableEntry->State & 0x20)
                return 0;
            XlatTableEntry->State |= 0x20;
            RefId = XlatTableEntry->RefId;
            break;
        }

    if (!XlatTableEntry)
        return 0;

    if (pXlatTables->RefIdToPointer.NumberOfEntries > RefId)
    {
        pXlatTables->RefIdToPointer.StateTable[RefId] |= 0x20;
        return 1;
    }

    return 0;
}
