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
#include <stdlib.h>

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
    FULL_PTR_XLAT_TABLES *pXlatTables = malloc(sizeof(*pXlatTables));

    TRACE("(%ld, %d)\n", NumberOfPointers, XlatSide);

    if (!NumberOfPointers) NumberOfPointers = 512;
    NumberOfBuckets = ((NumberOfPointers + 3) & ~3) - 1;

    pXlatTables->RefIdToPointer.XlatTable = calloc(NumberOfPointers, sizeof(void *));
    pXlatTables->RefIdToPointer.StateTable = calloc(NumberOfPointers, sizeof(unsigned char));
    pXlatTables->RefIdToPointer.NumberOfEntries = NumberOfPointers;

    TRACE("NumberOfBuckets = %ld\n", NumberOfBuckets);
    pXlatTables->PointerToRefId.XlatTable = calloc(NumberOfBuckets, sizeof(FULL_PTR_TO_REFID_ELEMENT *));
    pXlatTables->PointerToRefId.NumberOfBuckets = NumberOfBuckets;
    pXlatTables->PointerToRefId.HashMask = NumberOfBuckets - 1;

    pXlatTables->NextRefId = 1;
    pXlatTables->XlatSide = XlatSide;

    return pXlatTables;
}

void WINAPI NdrFullPointerXlatFree(PFULL_PTR_XLAT_TABLES pXlatTables)
{
    ULONG i;

    TRACE("(%p)\n", pXlatTables);

    /* free the entries in the table */
    for (i = 0; i < pXlatTables->PointerToRefId.NumberOfBuckets; i++)
    {
        PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;
        for (XlatTableEntry = pXlatTables->PointerToRefId.XlatTable[i];
            XlatTableEntry; )
        {
            PFULL_PTR_TO_REFID_ELEMENT Next = XlatTableEntry->Next;
            free(XlatTableEntry);
            XlatTableEntry = Next;
        }
    }

    free(pXlatTables->RefIdToPointer.XlatTable);
    free(pXlatTables->RefIdToPointer.StateTable);
    free(pXlatTables->PointerToRefId.XlatTable);

    free(pXlatTables);
}

static void expand_pointer_table_if_necessary(PFULL_PTR_XLAT_TABLES pXlatTables, ULONG RefId)
{
    if (RefId >= pXlatTables->RefIdToPointer.NumberOfEntries)
    {
        pXlatTables->RefIdToPointer.XlatTable =
            realloc(pXlatTables->RefIdToPointer.XlatTable, sizeof(void *) * RefId * 2);
        pXlatTables->RefIdToPointer.StateTable =
            realloc(pXlatTables->RefIdToPointer.StateTable, RefId * 2);
        if (!pXlatTables->RefIdToPointer.XlatTable || !pXlatTables->RefIdToPointer.StateTable)
        {
            pXlatTables->RefIdToPointer.NumberOfEntries = 0;
            return;
        }
        memset(pXlatTables->RefIdToPointer.XlatTable + pXlatTables->RefIdToPointer.NumberOfEntries, 0,
            (RefId * 2 - pXlatTables->RefIdToPointer.NumberOfEntries) * sizeof(void *));
        memset(pXlatTables->RefIdToPointer.StateTable + pXlatTables->RefIdToPointer.NumberOfEntries, 0,
            RefId * 2 - pXlatTables->RefIdToPointer.NumberOfEntries);
        pXlatTables->RefIdToPointer.NumberOfEntries = RefId * 2;
    }
}

int WINAPI NdrFullPointerQueryPointer(PFULL_PTR_XLAT_TABLES pXlatTables,
                                      void *pPointer, unsigned char QueryType,
                                      ULONG *pRefId )
{
    ULONG Hash = 0;
    unsigned int i;
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

    XlatTableEntry = malloc(sizeof(*XlatTableEntry));
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
    TRACE("(%p, 0x%lx, %d, %p)\n", pXlatTables, RefId, QueryType, ppPointer);

    if (!RefId)
        return 1;

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
    unsigned int i;
    PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;

    TRACE("(%p, 0x%lx, %p)\n", pXlatTables, RefId, pPointer);

    /* simple hashing algorithm, don't know whether it matches native */
    for (i = 0; i < sizeof(pPointer); i++)
        Hash = (Hash * 3) ^ ((unsigned char *)&pPointer)[i];

    XlatTableEntry = malloc(sizeof(*XlatTableEntry));
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
    unsigned int i;
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
