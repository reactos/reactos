/*
 * Copyright 2016 Mark Jansen
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

#if !defined(SDBWRITE_HOSTTOOL)
#define WIN32_NO_STATUS
#include "windows.h"
#include "sdbtypes.h"
#include "sdbpapi.h"
#else /* !defined(SDBWRITE_HOSTTOOL) */
#include <typedefs.h>
#include "sdbtypes.h"
#include "sdbpapi.h"
#endif /* !defined(SDBWRITE_HOSTTOOL) */

#include "sdbstringtable.h"

#define DEFAULT_TABLE_SIZE    0x100

typedef struct SdbHashEntry
{
    struct SdbHashEntry* Next;
    TAGID Tagid;
    WCHAR Name[1];
} SdbHashEntry;

struct SdbStringHashTable
{
    DWORD Size;
    struct SdbHashEntry** Entries;
};


static struct SdbStringHashTable* HashCreate(void)
{
    struct SdbStringHashTable* tab = SdbAlloc(sizeof(*tab));
    if (!tab)
    {
        SHIM_ERR("Failed to allocate 8 bytes.\r\n");
        return tab;
    }
    tab->Size = DEFAULT_TABLE_SIZE;
    tab->Entries = SdbAlloc(tab->Size * sizeof(*tab->Entries));
    return tab;
}


void SdbpTableDestroy(struct SdbStringHashTable** pTable)
{
    struct SdbStringHashTable* table = *pTable;
    struct SdbHashEntry* entry, *next;
    DWORD n, depth = 0, once = 1;

    *pTable = NULL;
    for (n = 0; n < table->Size; ++n)
    {
        depth = 0;
        entry = next = table->Entries[n];
        while (entry)
        {
            next = entry->Next;
            SdbFree(entry);
            entry = next;
            depth++;
        }
        if (once && depth > 3)
        {
            // warn
            once = 0;
        }
    }
    SdbFree(table->Entries);
    SdbFree(table);
}

/* Based on RtlHashUnicodeString */
static DWORD StringHash(const WCHAR* str)
{
    DWORD hash = 0;
    for (; *str; str++)
    {
        hash = ((65599 * hash) + (ULONG)(*str));
    }
    return hash;
}

static struct SdbHashEntry** TableFindPtr(struct SdbStringHashTable* table, const WCHAR* str)
{
    DWORD hash = StringHash(str);
    struct SdbHashEntry** entry = &table->Entries[hash % table->Size];
    while (*entry)
    {
        if (!wcscmp((*entry)->Name, str))
            return entry;
        entry = &(*entry)->Next;
    }
    return entry;
}

static BOOL HashAddString(struct SdbStringHashTable* table, struct SdbHashEntry** position, const WCHAR* str, TAGID tagid)
{
    struct SdbHashEntry* entry;
    SIZE_T size;

    if (!position)
        position = TableFindPtr(table, str);

    size = offsetof(struct SdbHashEntry, Name[SdbpStrlen(str) + 2]);
    entry = (*position) = SdbAlloc(size);
    if (!entry)
    {
        SHIM_ERR("Failed to allocate %u bytes.", size);
        return FALSE;
    }
    entry->Tagid = tagid;
    wcscpy(entry->Name, str);
    return TRUE;
}


BOOL SdbpAddStringToTable(struct SdbStringHashTable** table, const WCHAR* str, TAGID* tagid)
{
    struct SdbHashEntry** entry;

    if (!*table)
    {
        *table = HashCreate();
        if (!*table)
        {
            SHIM_ERR("Error creating hash table\n");
            return FALSE;
        }
    }

    entry = TableFindPtr(*table, str);
    if (*entry)
    {
        *tagid = (*entry)->Tagid;
        return FALSE;
    }
    return HashAddString(*table, entry, str, *tagid);
}

