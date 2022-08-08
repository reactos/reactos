/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shim database string table builder
 * COPYRIGHT:   Copyright 2016-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#if !defined(SDBWRITE_HOSTTOOL)
#define WIN32_NO_STATUS
#include "windows.h"
#include <appcompat/sdbtypes.h>
#include "sdbpapi.h"
#else /* !defined(SDBWRITE_HOSTTOOL) */
#include <typedefs.h>
#include <guiddef.h>
#include "sdbtypes.h"
#include "sdbpapi.h"
#endif /* !defined(SDBWRITE_HOSTTOOL) */

#include "sdbstringtable.h"

#if !defined(offsetof)
#if defined(__GNUC__)
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t)&(((TYPE *)0)->MEMBER))
#endif
#endif // !defined(offsetof)

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

int Sdbwcscmp(const WCHAR* s1, const WCHAR* s2)
{
    while (*s1 == *s2)
    {
        if (*s1 == 0)
            return 0;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


// implementation taken from reactos/sdk/lib/crt/string/wcs.c
INT Sdbwcscpy(WCHAR* wcDest, size_t numElement, const WCHAR *wcSrc)
{
    size_t size = 0;
    if(!wcDest || !numElement)
        return 22;  /* EINVAL */

    wcDest[0] = 0;

    if(!wcSrc)
        return 22;  /* EINVAL */

    size = SdbpStrlen(wcSrc) + 1;

    if(size > numElement)
        return 34;  /* ERANGE */

    memcpy(wcDest, wcSrc, size * sizeof(WCHAR));

    return 0;
}

static struct SdbHashEntry** TableFindPtr(struct SdbStringHashTable* table, const WCHAR* str)
{
    DWORD hash = StringHash(str);
    struct SdbHashEntry** entry = &table->Entries[hash % table->Size];
    while (*entry)
    {
        if (!Sdbwcscmp((*entry)->Name, str))
            return entry;
        entry = &(*entry)->Next;
    }
    return entry;
}

static BOOL HashAddString(struct SdbStringHashTable* table, struct SdbHashEntry** position, const WCHAR* str, TAGID tagid)
{
    struct SdbHashEntry* entry;
    SIZE_T size, len;

    if (!position)
        position = TableFindPtr(table, str);

    len = SdbpStrlen(str) + 1;
    size = offsetof(struct SdbHashEntry, Name[len]);
    entry = (*position) = SdbAlloc(size);
    if (!entry)
    {
        SHIM_ERR("Failed to allocate %u bytes.\n", size);
        return FALSE;
    }
    entry->Tagid = tagid;
    Sdbwcscpy(entry->Name, len, str);
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

