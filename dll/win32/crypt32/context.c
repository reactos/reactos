/*
 * Copyright 2006 Juan Lang
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

typedef enum _ContextType {
    ContextTypeData,
    ContextTypeLink,
} ContextType;

typedef struct _BASE_CONTEXT
{
    LONG        ref;
    ContextType type;
} BASE_CONTEXT, *PBASE_CONTEXT;

typedef struct _DATA_CONTEXT
{
    LONG                   ref;
    ContextType            type; /* always ContextTypeData */
    PCONTEXT_PROPERTY_LIST properties;
} DATA_CONTEXT, *PDATA_CONTEXT;

typedef struct _LINK_CONTEXT
{
    LONG          ref;
    ContextType   type; /* always ContextTypeLink */
    PBASE_CONTEXT linked;
} LINK_CONTEXT, *PLINK_CONTEXT;

#define CONTEXT_FROM_BASE_CONTEXT(p, s) ((LPBYTE)(p) - (s))
#define BASE_CONTEXT_FROM_CONTEXT(p, s) (PBASE_CONTEXT)((LPBYTE)(p) + (s))

void *Context_CreateDataContext(size_t contextSize)
{
    void *ret = CryptMemAlloc(contextSize + sizeof(DATA_CONTEXT));

    if (ret)
    {
        PDATA_CONTEXT context = (PDATA_CONTEXT)((LPBYTE)ret + contextSize);

        context->ref = 1;
        context->type = ContextTypeData;
        context->properties = ContextPropertyList_Create();
        if (!context->properties)
        {
            CryptMemFree(ret);
            ret = NULL;
        }
    }
    return ret;
}

void *Context_CreateLinkContext(unsigned int contextSize, void *linked, unsigned int extra,
 BOOL addRef)
{
    void *context = CryptMemAlloc(contextSize + sizeof(LINK_CONTEXT) + extra);

    TRACE("(%d, %p, %d)\n", contextSize, linked, extra);

    if (context)
    {
        PLINK_CONTEXT linkContext = (PLINK_CONTEXT)BASE_CONTEXT_FROM_CONTEXT(
         context, contextSize);
        PBASE_CONTEXT linkedBase = BASE_CONTEXT_FROM_CONTEXT(linked,
         contextSize);

        memcpy(context, linked, contextSize);
        linkContext->ref = 1;
        linkContext->type = ContextTypeLink;
        linkContext->linked = linkedBase;
        if (addRef)
            InterlockedIncrement(&linkedBase->ref);
        TRACE("%p's ref count is %d\n", context, linkContext->ref);
    }
    return context;
}

void Context_AddRef(void *context, size_t contextSize)
{
    PBASE_CONTEXT baseContext = BASE_CONTEXT_FROM_CONTEXT(context, contextSize);

    InterlockedIncrement(&baseContext->ref);
}

void *Context_GetExtra(const void *context, size_t contextSize)
{
    PBASE_CONTEXT baseContext = BASE_CONTEXT_FROM_CONTEXT(context, contextSize);

    assert(baseContext->type == ContextTypeLink);
    return (LPBYTE)baseContext + sizeof(LINK_CONTEXT);
}

void *Context_GetLinkedContext(void *context, size_t contextSize)
{
    PBASE_CONTEXT baseContext = BASE_CONTEXT_FROM_CONTEXT(context, contextSize);

    assert(baseContext->type == ContextTypeLink);
    return CONTEXT_FROM_BASE_CONTEXT(((PLINK_CONTEXT)baseContext)->linked,
     contextSize);
}

PCONTEXT_PROPERTY_LIST Context_GetProperties(void *context, size_t contextSize)
{
    PBASE_CONTEXT ptr = BASE_CONTEXT_FROM_CONTEXT(context, contextSize);

    while (ptr && ptr->type == ContextTypeLink)
        ptr = ((PLINK_CONTEXT)ptr)->linked;
    return (ptr && ptr->type == ContextTypeData) ?
     ((PDATA_CONTEXT)ptr)->properties : NULL;
}

void Context_Release(void *context, size_t contextSize,
 ContextFreeFunc dataContextFree)
{
    PBASE_CONTEXT base = BASE_CONTEXT_FROM_CONTEXT(context, contextSize);

    if (InterlockedDecrement(&base->ref) == 0)
    {
        TRACE("freeing %p\n", context);
        switch (base->type)
        {
        case ContextTypeData:
            ContextPropertyList_Free(((PDATA_CONTEXT)base)->properties);
            dataContextFree(context);
            break;
        case ContextTypeLink:
            /* The linked context is of the same type as this, so release
             * it as well, using the same offset and data free function.
             */
            Context_Release(CONTEXT_FROM_BASE_CONTEXT(
             ((PLINK_CONTEXT)base)->linked, contextSize), contextSize,
             dataContextFree);
            break;
        default:
            assert(0);
        }
        CryptMemFree(context);
    }
    else
        TRACE("%p's ref count is %d\n", context, base->ref);
}

void Context_CopyProperties(const void *to, const void *from,
 size_t contextSize)
{
    PCONTEXT_PROPERTY_LIST toProperties, fromProperties;

    toProperties = Context_GetProperties((void *)to, contextSize);
    fromProperties = Context_GetProperties((void *)from, contextSize);
    ContextPropertyList_Copy(toProperties, fromProperties);
}

struct ContextList
{
    PCWINE_CONTEXT_INTERFACE contextInterface;
    size_t contextSize;
    CRITICAL_SECTION cs;
    struct list contexts;
};

struct ContextList *ContextList_Create(
 PCWINE_CONTEXT_INTERFACE contextInterface, size_t contextSize)
{
    struct ContextList *list = CryptMemAlloc(sizeof(struct ContextList));

    if (list)
    {
        list->contextInterface = contextInterface;
        list->contextSize = contextSize;
        InitializeCriticalSection(&list->cs);
        list->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ContextList.cs");
        list_init(&list->contexts);
    }
    return list;
}

static inline struct list *ContextList_ContextToEntry(struct ContextList *list,
 const void *context)
{
    struct list *ret;

    if (context)
        ret = (struct list *)Context_GetExtra(context, list->contextSize);
    else
        ret = NULL;
    return ret;
}

static inline void *ContextList_EntryToContext(struct ContextList *list,
 struct list *entry)
{
    return (LPBYTE)entry - sizeof(LINK_CONTEXT) - list->contextSize;
}

void *ContextList_Add(struct ContextList *list, void *toLink, void *toReplace)
{
    void *context;

    TRACE("(%p, %p, %p)\n", list, toLink, toReplace);

    context = Context_CreateLinkContext(list->contextSize, toLink,
     sizeof(struct list), TRUE);
    if (context)
    {
        struct list *entry = ContextList_ContextToEntry(list, context);

        TRACE("adding %p\n", context);
        EnterCriticalSection(&list->cs);
        if (toReplace)
        {
            struct list *existing = ContextList_ContextToEntry(list, toReplace);

            entry->prev = existing->prev;
            entry->next = existing->next;
            entry->prev->next = entry;
            entry->next->prev = entry;
            existing->prev = existing->next = existing;
            list->contextInterface->free(toReplace);
        }
        else
            list_add_tail(&list->contexts, entry);
        LeaveCriticalSection(&list->cs);
    }
    return context;
}

void *ContextList_Enum(struct ContextList *list, void *pPrev)
{
    struct list *listNext;
    void *ret;

    EnterCriticalSection(&list->cs);
    if (pPrev)
    {
        struct list *prevEntry = ContextList_ContextToEntry(list, pPrev);

        listNext = list_next(&list->contexts, prevEntry);
        list->contextInterface->free(pPrev);
    }
    else
        listNext = list_next(&list->contexts, &list->contexts);
    LeaveCriticalSection(&list->cs);

    if (listNext)
    {
        ret = ContextList_EntryToContext(list, listNext);
        list->contextInterface->duplicate(ret);
    }
    else
        ret = NULL;
    return ret;
}

void ContextList_Delete(struct ContextList *list, void *context)
{
    struct list *entry = ContextList_ContextToEntry(list, context);

    EnterCriticalSection(&list->cs);
    list_remove(entry);
    LeaveCriticalSection(&list->cs);
    list->contextInterface->free(context);
}

void ContextList_Empty(struct ContextList *list)
{
    struct list *entry, *next;

    EnterCriticalSection(&list->cs);
    LIST_FOR_EACH_SAFE(entry, next, &list->contexts)
    {
        const void *context = ContextList_EntryToContext(list, entry);

        TRACE("removing %p\n", context);
        list_remove(entry);
        list->contextInterface->free(context);
    }
    LeaveCriticalSection(&list->cs);
}

void ContextList_Free(struct ContextList *list)
{
    ContextList_Empty(list);
    list->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&list->cs);
    CryptMemFree(list);
}
