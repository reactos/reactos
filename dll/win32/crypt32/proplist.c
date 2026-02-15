/*
 * Copyright 2004-2006 Juan Lang
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
#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

struct _CONTEXT_PROPERTY_LIST
{
    CRITICAL_SECTION cs;
    struct list      properties;
};

typedef struct _CONTEXT_PROPERTY
{
    DWORD       propID;
    DWORD       cbData;
    LPBYTE      pbData;
    struct list entry;
} CONTEXT_PROPERTY;

CONTEXT_PROPERTY_LIST *ContextPropertyList_Create(void)
{
    CONTEXT_PROPERTY_LIST *list = CryptMemAlloc(sizeof(CONTEXT_PROPERTY_LIST));

    if (list)
    {
        InitializeCriticalSectionEx(&list->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
        list->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PCONTEXT_PROPERTY_LIST->cs");
        list_init(&list->properties);
    }
    return list;
}

void ContextPropertyList_Free(CONTEXT_PROPERTY_LIST *list)
{
    CONTEXT_PROPERTY *prop, *next;

    LIST_FOR_EACH_ENTRY_SAFE(prop, next, &list->properties, CONTEXT_PROPERTY,
     entry)
    {
        list_remove(&prop->entry);
        CryptMemFree(prop->pbData);
        CryptMemFree(prop);
    }
    list->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&list->cs);
    CryptMemFree(list);
}

BOOL ContextPropertyList_FindProperty(CONTEXT_PROPERTY_LIST *list, DWORD id,
 PCRYPT_DATA_BLOB blob)
{
    CONTEXT_PROPERTY *prop;
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %p)\n", list, id, blob);

    EnterCriticalSection(&list->cs);
    LIST_FOR_EACH_ENTRY(prop, &list->properties, CONTEXT_PROPERTY, entry)
    {
        if (prop->propID == id)
        {
            blob->cbData = prop->cbData;
            blob->pbData = prop->pbData;
            ret = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&list->cs);
    return ret;
}

BOOL ContextPropertyList_SetProperty(CONTEXT_PROPERTY_LIST *list, DWORD id,
 const BYTE *pbData, size_t cbData)
{
    LPBYTE data;
    BOOL ret = FALSE;

    if (cbData)
    {
        data = CryptMemAlloc(cbData);
        if (data)
            memcpy(data, pbData, cbData);
    }
    else
        data = NULL;
    if (!cbData || data)
    {
        CONTEXT_PROPERTY *prop;
        BOOL found = FALSE;

        EnterCriticalSection(&list->cs);
        LIST_FOR_EACH_ENTRY(prop, &list->properties, CONTEXT_PROPERTY, entry)
        {
            if (prop->propID == id)
            {
                found = TRUE;
                break;
            }
        }
        if (found)
        {
            CryptMemFree(prop->pbData);
            prop->cbData = cbData;
            prop->pbData = data;
            ret = TRUE;
        }
        else
        {
            prop = CryptMemAlloc(sizeof(CONTEXT_PROPERTY));
            if (prop)
            {
                prop->propID = id;
                prop->cbData = cbData;
                prop->pbData = data;
                list_add_tail(&list->properties, &prop->entry);
                ret = TRUE;
            }
            else
                CryptMemFree(data);
        }
        LeaveCriticalSection(&list->cs);
    }
    return ret;
}

void ContextPropertyList_RemoveProperty(CONTEXT_PROPERTY_LIST *list, DWORD id)
{
    CONTEXT_PROPERTY *prop;

    EnterCriticalSection(&list->cs);
    LIST_FOR_EACH_ENTRY(prop, &list->properties, CONTEXT_PROPERTY, entry)
    {
        if (prop->propID == id)
        {
            list_remove(&prop->entry);
            CryptMemFree(prop->pbData);
            CryptMemFree(prop);
            break;
        }
    }
    LeaveCriticalSection(&list->cs);
}

/* Since the properties are stored in a list, this is a tad inefficient
 * (O(n^2)) since I have to find the previous position every time.
 */
DWORD ContextPropertyList_EnumPropIDs(CONTEXT_PROPERTY_LIST *list, DWORD id)
{
    DWORD ret;

    EnterCriticalSection(&list->cs);
    if (id)
    {
        CONTEXT_PROPERTY *cursor = NULL, *prop;

        LIST_FOR_EACH_ENTRY(prop, &list->properties, CONTEXT_PROPERTY, entry)
        {
            if (prop->propID == id)
            {
                cursor = prop;
                break;
            }
        }
        if (cursor)
        {
            if (cursor->entry.next != &list->properties)
                ret = LIST_ENTRY(cursor->entry.next, CONTEXT_PROPERTY,
                 entry)->propID;
            else
                ret = 0;
        }
        else
            ret = 0;
    }
    else if (!list_empty(&list->properties))
        ret = LIST_ENTRY(list->properties.next, CONTEXT_PROPERTY,
         entry)->propID;
    else
        ret = 0;
    LeaveCriticalSection(&list->cs);
    return ret;
}

void ContextPropertyList_Copy(CONTEXT_PROPERTY_LIST *to, CONTEXT_PROPERTY_LIST *from)
{
    CONTEXT_PROPERTY *prop;

    EnterCriticalSection(&from->cs);
    LIST_FOR_EACH_ENTRY(prop, &from->properties, CONTEXT_PROPERTY, entry)
    {
        ContextPropertyList_SetProperty(to, prop->propID, prop->pbData,
         prop->cbData);
    }
    LeaveCriticalSection(&from->cs);
}
