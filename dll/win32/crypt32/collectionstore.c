/*
 * Copyright 2004-2007 Juan Lang
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
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef struct _WINE_STORE_LIST_ENTRY
{
    WINECRYPT_CERTSTORE *store;
    DWORD                dwUpdateFlags;
    DWORD                dwPriority;
    struct list          entry;
} WINE_STORE_LIST_ENTRY;

typedef struct _WINE_COLLECTIONSTORE
{
    WINECRYPT_CERTSTORE hdr;
    CRITICAL_SECTION    cs;
    struct list         stores;
} WINE_COLLECTIONSTORE;

static void Collection_addref(WINECRYPT_CERTSTORE *store)
{
    LONG ref = InterlockedIncrement(&store->ref);
    TRACE("ref = %ld\n", ref);
}

static DWORD Collection_release(WINECRYPT_CERTSTORE *store, DWORD flags)
{
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;
    WINE_STORE_LIST_ENTRY *entry, *next;
    LONG ref;

    if(flags)
        FIXME("Unimplemented flags %lx\n", flags);

    ref = InterlockedDecrement(&cs->hdr.ref);
    TRACE("(%p) ref=%ld\n", store, ref);
    if(ref)
        return ERROR_SUCCESS;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &cs->stores, WINE_STORE_LIST_ENTRY, entry)
    {
        TRACE("closing %p\n", entry);
        entry->store->vtbl->release(entry->store, flags);
        CryptMemFree(entry);
    }
    cs->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&cs->cs);
    CRYPT_FreeStore(store);
    return ERROR_SUCCESS;
}

static void Collection_releaseContext(WINECRYPT_CERTSTORE *store, context_t *context)
{
    /* We don't cache context links, so just free them. */
    Context_Free(context);
}

static context_t *CRYPT_CollectionCreateContextFromChild(WINE_COLLECTIONSTORE *store,
 WINE_STORE_LIST_ENTRY *storeEntry, context_t *child)
{
    context_t *ret;

    ret = child->vtbl->clone(child, &store->hdr, TRUE);
    if (!ret)
        return NULL;

    ret->u.ptr = storeEntry;
    return ret;
}

static BOOL CRYPT_CollectionAddContext(WINE_COLLECTIONSTORE *store,
 unsigned int contextFuncsOffset, context_t *context, context_t *toReplace,
 context_t **pChildContext)
{
    BOOL ret;
    context_t *childContext = NULL;
    WINE_STORE_LIST_ENTRY *storeEntry = NULL;

    TRACE("(%p, %d, %p, %p)\n", store, contextFuncsOffset, context, toReplace);

    ret = FALSE;
    if (toReplace)
    {
        context_t *existingLinked = toReplace->linked;
        CONTEXT_FUNCS *contextFuncs;

        storeEntry = toReplace->u.ptr;
        contextFuncs = (CONTEXT_FUNCS*)((LPBYTE)storeEntry->store->vtbl +
         contextFuncsOffset);
        ret = contextFuncs->addContext(storeEntry->store, context,
         existingLinked, &childContext, TRUE);
    }
    else
    {
        WINE_STORE_LIST_ENTRY *entry;

        EnterCriticalSection(&store->cs);
        LIST_FOR_EACH_ENTRY(entry, &store->stores, WINE_STORE_LIST_ENTRY, entry)
        {
            if (entry->dwUpdateFlags & CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG)
            {
                CONTEXT_FUNCS *contextFuncs = (CONTEXT_FUNCS*)(
                 (LPBYTE)entry->store->vtbl + contextFuncsOffset);

                storeEntry = entry;
                ret = contextFuncs->addContext(entry->store, context, NULL, &childContext, TRUE);
                break;
            }
        }
        LeaveCriticalSection(&store->cs);
        if (!storeEntry)
            SetLastError(E_ACCESSDENIED);
    }
    *pChildContext = childContext;
    return ret;
}

/* Advances a collection enumeration by one context, if possible, where
 * advancing means:
 * - calling the current store's enumeration function once, and returning
 *   the enumerated context if one is returned
 * - moving to the next store if the current store has no more items, and
 *   recursively calling itself to get the next item.
 * Returns NULL if the collection contains no more items or on error.
 * Assumes the collection store's lock is held.
 */
static context_t *CRYPT_CollectionAdvanceEnum(WINE_COLLECTIONSTORE *store,
 WINE_STORE_LIST_ENTRY *storeEntry, const CONTEXT_FUNCS *contextFuncs,
 context_t *prev)
{
    context_t *child, *ret;
    struct list *storeNext = list_next(&store->stores, &storeEntry->entry);

    TRACE("(%p, %p, %p)\n", store, storeEntry, prev);

    if (prev)
    {
        /* Ref-counting funny business: "duplicate" (addref) the child, because
         * the free(pPrev) below can cause the ref count to become negative.
         */
        child = prev->linked;
        Context_AddRef(child);
        child = contextFuncs->enumContext(storeEntry->store, child);
        Context_Release(prev);
    }
    else
    {
        child = contextFuncs->enumContext(storeEntry->store, NULL);
    }
    if (child) {
        ret = CRYPT_CollectionCreateContextFromChild(store, storeEntry, child);
        Context_Release(child);
    }
    else
    {
        if (storeNext)
        {
            /* We always want the same function pointers (from certs, crls)
             * in the next store, so use the same offset into the next store.
             */
            size_t offset = (const BYTE *)contextFuncs - (LPBYTE)storeEntry->store->vtbl;
            WINE_STORE_LIST_ENTRY *storeNextEntry =
             LIST_ENTRY(storeNext, WINE_STORE_LIST_ENTRY, entry);
            CONTEXT_FUNCS *storeNextContexts =
             (CONTEXT_FUNCS*)((LPBYTE)storeNextEntry->store->vtbl + offset);

            ret = CRYPT_CollectionAdvanceEnum(store, storeNextEntry,
             storeNextContexts, NULL);
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL Collection_addCert(WINECRYPT_CERTSTORE *store, context_t *cert,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    BOOL ret;
    context_t *childContext = NULL;
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(store_vtbl_t, certs),
     cert, toReplace, &childContext);
    if (ppStoreContext && childContext)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = childContext->u.ptr;
        cert_t *context = (cert_t*)CRYPT_CollectionCreateContextFromChild(cs, storeEntry,
         childContext);

        *ppStoreContext = &context->base;
    }
    if (childContext)
        Context_Release(childContext);
    return ret;
}

static context_t *Collection_enumCert(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;
    context_t *ret;

    TRACE("(%p, %p)\n", store, prev);

    EnterCriticalSection(&cs->cs);
    if (prev)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = prev->u.ptr;

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->vtbl->certs, prev);
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            WINE_STORE_LIST_ENTRY *storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->vtbl->certs, NULL);
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL Collection_deleteCert(WINECRYPT_CERTSTORE *store, context_t *context)
{
    cert_t *cert = (cert_t*)context;
    cert_t *linked;

    TRACE("(%p, %p)\n", store, cert);

    linked = (cert_t*)context->linked;
    return CertDeleteCertificateFromStore(&linked->ctx);
}

static BOOL Collection_addCRL(WINECRYPT_CERTSTORE *store, context_t *crl,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    BOOL ret;
    context_t *childContext = NULL;
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(store_vtbl_t, crls),
     crl, toReplace, &childContext);
    if (ppStoreContext && childContext)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = childContext->u.ptr;
        crl_t *context = (crl_t*)CRYPT_CollectionCreateContextFromChild(cs, storeEntry,
         childContext);

        *ppStoreContext = &context->base;
    }
    if (childContext)
        Context_Release(childContext);
    return ret;
}

static context_t *Collection_enumCRL(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;
    context_t *ret;

    TRACE("(%p, %p)\n", store, prev);

    EnterCriticalSection(&cs->cs);
    if (prev)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = prev->u.ptr;

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->vtbl->crls, prev);
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            WINE_STORE_LIST_ENTRY *storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->vtbl->crls, NULL);
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL Collection_deleteCRL(WINECRYPT_CERTSTORE *store, context_t *context)
{
    crl_t *crl = (crl_t*)context, *linked;

    TRACE("(%p, %p)\n", store, crl);

    linked = (crl_t*)context->linked;
    return CertDeleteCRLFromStore(&linked->ctx);
}

static BOOL Collection_addCTL(WINECRYPT_CERTSTORE *store, context_t *ctl,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    BOOL ret;
    context_t *childContext = NULL;
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(store_vtbl_t, ctls),
     ctl, toReplace, &childContext);
    if (ppStoreContext && childContext)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = childContext->u.ptr;
        ctl_t *context = (ctl_t*)CRYPT_CollectionCreateContextFromChild(cs, storeEntry,
         childContext);

        *ppStoreContext = &context->base;
    }
    if (childContext)
        Context_Release(childContext);
    return ret;
}

static context_t *Collection_enumCTL(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_COLLECTIONSTORE *cs = (WINE_COLLECTIONSTORE*)store;
    void *ret;

    TRACE("(%p, %p)\n", store, prev);

    EnterCriticalSection(&cs->cs);
    if (prev)
    {
        WINE_STORE_LIST_ENTRY *storeEntry = prev->u.ptr;

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->vtbl->ctls, prev);
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            WINE_STORE_LIST_ENTRY *storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->vtbl->ctls, NULL);
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL Collection_deleteCTL(WINECRYPT_CERTSTORE *store, context_t *context)
{
    ctl_t *ctl = (ctl_t*)context, *linked;

    TRACE("(%p, %p)\n", store, ctl);

    linked = (ctl_t*)context->linked;
    return CertDeleteCTLFromStore(&linked->ctx);
}

static BOOL Collection_control(WINECRYPT_CERTSTORE *cert_store, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    BOOL ret;
    WINE_COLLECTIONSTORE *store = (WINE_COLLECTIONSTORE*)cert_store;
    WINE_STORE_LIST_ENTRY *entry;

    TRACE("(%p, %08lx, %ld, %p)\n", cert_store, dwFlags, dwCtrlType, pvCtrlPara);

    if (!store)
        return TRUE;
    if (store->hdr.dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (store->hdr.type != StoreTypeCollection)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    ret = TRUE;
    EnterCriticalSection(&store->cs);
    LIST_FOR_EACH_ENTRY(entry, &store->stores, WINE_STORE_LIST_ENTRY, entry)
    {
        if (entry->store->vtbl->control)
        {
            ret = entry->store->vtbl->control(entry->store, dwFlags, dwCtrlType, pvCtrlPara);
            if (!ret)
                break;
        }
    }
    LeaveCriticalSection(&store->cs);
    return ret;
}

static const store_vtbl_t CollectionStoreVtbl = {
    Collection_addref,
    Collection_release,
    Collection_releaseContext,
    Collection_control,
    {
        Collection_addCert,
        Collection_enumCert,
        Collection_deleteCert
    }, {
        Collection_addCRL,
        Collection_enumCRL,
        Collection_deleteCRL
    }, {
        Collection_addCTL,
        Collection_enumCTL,
        Collection_deleteCTL
    }
};

WINECRYPT_CERTSTORE *CRYPT_CollectionOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    WINE_COLLECTIONSTORE *store;

    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        store = NULL;
    }
    else
    {
        store = CryptMemAlloc(sizeof(WINE_COLLECTIONSTORE));
        if (store)
        {
            memset(store, 0, sizeof(WINE_COLLECTIONSTORE));
            CRYPT_InitStore(&store->hdr, dwFlags, StoreTypeCollection, &CollectionStoreVtbl);
            InitializeCriticalSectionEx(&store->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
            store->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PWINE_COLLECTIONSTORE->cs");
            list_init(&store->stores);
        }
    }
    return (WINECRYPT_CERTSTORE*)store;
}

BOOL WINAPI CertAddStoreToCollection(HCERTSTORE hCollectionStore,
 HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
    WINE_COLLECTIONSTORE *collection = hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = hSiblingStore;
    WINE_STORE_LIST_ENTRY *entry;
    BOOL ret;

    TRACE("(%p, %p, %08lx, %ld)\n", hCollectionStore, hSiblingStore,
     dwUpdateFlags, dwPriority);

    if (!collection || !sibling)
        return TRUE;
    if (collection->hdr.dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (collection->hdr.type != StoreTypeCollection)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (sibling->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    entry = CryptMemAlloc(sizeof(WINE_STORE_LIST_ENTRY));
    if (entry)
    {
        InterlockedIncrement(&sibling->ref);
        TRACE("sibling %p's ref count is %ld\n", sibling, sibling->ref);
        entry->store = sibling;
        entry->dwUpdateFlags = dwUpdateFlags;
        entry->dwPriority = dwPriority;
        TRACE("%p: adding %p, priority %ld\n", collection, entry, dwPriority);
        EnterCriticalSection(&collection->cs);
        if (dwPriority)
        {
            WINE_STORE_LIST_ENTRY *cursor;
            BOOL added = FALSE;

            LIST_FOR_EACH_ENTRY(cursor, &collection->stores,
             WINE_STORE_LIST_ENTRY, entry)
            {
                if (cursor->dwPriority < dwPriority)
                {
                    list_add_before(&cursor->entry, &entry->entry);
                    added = TRUE;
                    break;
                }
            }
            if (!added)
                list_add_tail(&collection->stores, &entry->entry);
        }
        else
            list_add_tail(&collection->stores, &entry->entry);
        LeaveCriticalSection(&collection->cs);
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
}

void WINAPI CertRemoveStoreFromCollection(HCERTSTORE hCollectionStore,
 HCERTSTORE hSiblingStore)
{
    WINE_COLLECTIONSTORE *collection = hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = hSiblingStore;
    WINE_STORE_LIST_ENTRY *store;

    TRACE("(%p, %p)\n", hCollectionStore, hSiblingStore);

    if (!collection || !sibling)
        return;
    if (collection->hdr.dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(E_INVALIDARG);
        return;
    }
    if (collection->hdr.type != StoreTypeCollection)
        return;
    if (sibling->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(E_INVALIDARG);
        return;
    }
    EnterCriticalSection(&collection->cs);
    LIST_FOR_EACH_ENTRY(store, &collection->stores, WINE_STORE_LIST_ENTRY, entry)
    {
        if (store->store == sibling)
        {
            list_remove(&store->entry);
            CertCloseStore(store->store, 0);
            CryptMemFree(store);
            break;
        }
    }
    LeaveCriticalSection(&collection->cs);
}
