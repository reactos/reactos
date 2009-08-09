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
#include "wine/list.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef struct _WINE_STORE_LIST_ENTRY
{
    PWINECRYPT_CERTSTORE store;
    DWORD                dwUpdateFlags;
    DWORD                dwPriority;
    struct list          entry;
} WINE_STORE_LIST_ENTRY, *PWINE_STORE_LIST_ENTRY;

typedef struct _WINE_COLLECTIONSTORE
{
    WINECRYPT_CERTSTORE hdr;
    CRITICAL_SECTION    cs;
    struct list         stores;
} WINE_COLLECTIONSTORE, *PWINE_COLLECTIONSTORE;

static void WINAPI CRYPT_CollectionCloseStore(HCERTSTORE store, DWORD dwFlags)
{
    PWINE_COLLECTIONSTORE cs = store;
    PWINE_STORE_LIST_ENTRY entry, next;

    TRACE("(%p, %08x)\n", store, dwFlags);

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &cs->stores, WINE_STORE_LIST_ENTRY,
     entry)
    {
        TRACE("closing %p\n", entry);
        CertCloseStore(entry->store, dwFlags);
        CryptMemFree(entry);
    }
    cs->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&cs->cs);
    CRYPT_FreeStore(store);
}

static void *CRYPT_CollectionCreateContextFromChild(PWINE_COLLECTIONSTORE store,
 PWINE_STORE_LIST_ENTRY storeEntry, void *child, size_t contextSize,
 BOOL addRef)
{
    void *ret = Context_CreateLinkContext(contextSize, child,
     sizeof(PWINE_STORE_LIST_ENTRY), addRef);

    if (ret)
        *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(ret, contextSize)
         = storeEntry;

    return ret;
}

static BOOL CRYPT_CollectionAddContext(PWINE_COLLECTIONSTORE store,
 unsigned int contextFuncsOffset, void *context, void *toReplace, unsigned int contextSize,
 void **pChildContext)
{
    BOOL ret;
    void *childContext = NULL;
    PWINE_STORE_LIST_ENTRY storeEntry = NULL;

    TRACE("(%p, %d, %p, %p, %d)\n", store, contextFuncsOffset, context,
     toReplace, contextSize);

    ret = FALSE;
    if (toReplace)
    {
        void *existingLinked = Context_GetLinkedContext(toReplace, contextSize);
        PCONTEXT_FUNCS contextFuncs;

        storeEntry = *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(toReplace,
         contextSize);
        contextFuncs = (PCONTEXT_FUNCS)((LPBYTE)storeEntry->store +
         contextFuncsOffset);
        ret = contextFuncs->addContext(storeEntry->store, context,
         existingLinked, (const void **)&childContext);
    }
    else
    {
        PWINE_STORE_LIST_ENTRY entry, next;

        EnterCriticalSection(&store->cs);
        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &store->stores,
         WINE_STORE_LIST_ENTRY, entry)
        {
            if (entry->dwUpdateFlags & CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG)
            {
                PCONTEXT_FUNCS contextFuncs = (PCONTEXT_FUNCS)(
                 (LPBYTE)entry->store + contextFuncsOffset);

                storeEntry = entry;
                ret = contextFuncs->addContext(entry->store, context, NULL,
                 (const void **)&childContext);
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
static void *CRYPT_CollectionAdvanceEnum(PWINE_COLLECTIONSTORE store,
 PWINE_STORE_LIST_ENTRY storeEntry, PCONTEXT_FUNCS contextFuncs,
 PCWINE_CONTEXT_INTERFACE contextInterface, void *pPrev, size_t contextSize)
{
    void *ret, *child;
    struct list *storeNext = list_next(&store->stores, &storeEntry->entry);

    TRACE("(%p, %p, %p)\n", store, storeEntry, pPrev);

    if (pPrev)
    {
        /* Ref-counting funny business: "duplicate" (addref) the child, because
         * the free(pPrev) below can cause the ref count to become negative.
         */
        child = Context_GetLinkedContext(pPrev, contextSize);
        contextInterface->duplicate(child);
        child = contextFuncs->enumContext(storeEntry->store, child);
        contextInterface->free(pPrev);
        pPrev = NULL;
    }
    else
        child = contextFuncs->enumContext(storeEntry->store, NULL);
    if (child)
        ret = CRYPT_CollectionCreateContextFromChild(store, storeEntry, child,
         contextSize, FALSE);
    else
    {
        if (storeNext)
        {
            /* We always want the same function pointers (from certs, crls)
             * in the next store, so use the same offset into the next store.
             */
            size_t offset = (LPBYTE)contextFuncs - (LPBYTE)storeEntry->store;
            PWINE_STORE_LIST_ENTRY storeNextEntry =
             LIST_ENTRY(storeNext, WINE_STORE_LIST_ENTRY, entry);
            PCONTEXT_FUNCS storeNextContexts =
             (PCONTEXT_FUNCS)((LPBYTE)storeNextEntry->store + offset);

            ret = CRYPT_CollectionAdvanceEnum(store, storeNextEntry,
             storeNextContexts, contextInterface, NULL, contextSize);
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

static BOOL CRYPT_CollectionAddCert(PWINECRYPT_CERTSTORE store, void *cert,
 void *toReplace, const void **ppStoreContext)
{
    BOOL ret;
    void *childContext = NULL;
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(WINECRYPT_CERTSTORE, certs),
     cert, toReplace, sizeof(CERT_CONTEXT), &childContext);
    if (ppStoreContext && childContext)
    {
        PWINE_STORE_LIST_ENTRY storeEntry = *(PWINE_STORE_LIST_ENTRY *)
         Context_GetExtra(childContext, sizeof(CERT_CONTEXT));
        PCERT_CONTEXT context =
         CRYPT_CollectionCreateContextFromChild(cs, storeEntry, childContext,
         sizeof(CERT_CONTEXT), TRUE);

        if (context)
            context->hCertStore = store;
        *ppStoreContext = context;
    }
    CertFreeCertificateContext(childContext);
    return ret;
}

static void *CRYPT_CollectionEnumCert(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    EnterCriticalSection(&cs->cs);
    if (pPrev)
    {
        PWINE_STORE_LIST_ENTRY storeEntry =
         *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(pPrev,
         sizeof(CERT_CONTEXT));

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->certs, pCertInterface, pPrev,
         sizeof(CERT_CONTEXT));
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->certs, pCertInterface, NULL,
             sizeof(CERT_CONTEXT));
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    if (ret)
        ((PCERT_CONTEXT)ret)->hCertStore = store;
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_CollectionDeleteCert(PWINECRYPT_CERTSTORE store,
 void *pCertContext)
{
    BOOL ret;

    TRACE("(%p, %p)\n", store, pCertContext);

    ret = CertDeleteCertificateFromStore(
     Context_GetLinkedContext(pCertContext, sizeof(CERT_CONTEXT)));
    return ret;
}

static BOOL CRYPT_CollectionAddCRL(PWINECRYPT_CERTSTORE store, void *crl,
 void *toReplace, const void **ppStoreContext)
{
    BOOL ret;
    void *childContext = NULL;
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(WINECRYPT_CERTSTORE, crls),
     crl, toReplace, sizeof(CRL_CONTEXT), &childContext);
    if (ppStoreContext && childContext)
    {
        PWINE_STORE_LIST_ENTRY storeEntry = *(PWINE_STORE_LIST_ENTRY *)
         Context_GetExtra(childContext, sizeof(CRL_CONTEXT));
        PCRL_CONTEXT context =
         CRYPT_CollectionCreateContextFromChild(cs, storeEntry, childContext,
         sizeof(CRL_CONTEXT), TRUE);

        if (context)
            context->hCertStore = store;
        *ppStoreContext = context;
    }
    CertFreeCRLContext(childContext);
    return ret;
}

static void *CRYPT_CollectionEnumCRL(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    EnterCriticalSection(&cs->cs);
    if (pPrev)
    {
        PWINE_STORE_LIST_ENTRY storeEntry =
         *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(pPrev,
         sizeof(CRL_CONTEXT));

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->crls, pCRLInterface, pPrev, sizeof(CRL_CONTEXT));
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->crls, pCRLInterface, NULL,
             sizeof(CRL_CONTEXT));
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    if (ret)
        ((PCRL_CONTEXT)ret)->hCertStore = store;
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_CollectionDeleteCRL(PWINECRYPT_CERTSTORE store,
 void *pCrlContext)
{
    BOOL ret;

    TRACE("(%p, %p)\n", store, pCrlContext);

    ret = CertDeleteCRLFromStore(
     Context_GetLinkedContext(pCrlContext, sizeof(CRL_CONTEXT)));
    return ret;
}

static BOOL CRYPT_CollectionAddCTL(PWINECRYPT_CERTSTORE store, void *ctl,
 void *toReplace, const void **ppStoreContext)
{
    BOOL ret;
    void *childContext = NULL;
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;

    ret = CRYPT_CollectionAddContext(cs, offsetof(WINECRYPT_CERTSTORE, ctls),
     ctl, toReplace, sizeof(CTL_CONTEXT), &childContext);
    if (ppStoreContext && childContext)
    {
        PWINE_STORE_LIST_ENTRY storeEntry = *(PWINE_STORE_LIST_ENTRY *)
         Context_GetExtra(childContext, sizeof(CTL_CONTEXT));
        PCTL_CONTEXT context =
         CRYPT_CollectionCreateContextFromChild(cs, storeEntry, childContext,
         sizeof(CTL_CONTEXT), TRUE);

        if (context)
            context->hCertStore = store;
        *ppStoreContext = context;
    }
    CertFreeCTLContext(childContext);
    return ret;
}

static void *CRYPT_CollectionEnumCTL(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    EnterCriticalSection(&cs->cs);
    if (pPrev)
    {
        PWINE_STORE_LIST_ENTRY storeEntry =
         *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(pPrev,
         sizeof(CTL_CONTEXT));

        ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
         &storeEntry->store->ctls, pCTLInterface, pPrev, sizeof(CTL_CONTEXT));
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             &storeEntry->store->ctls, pCTLInterface, NULL,
             sizeof(CTL_CONTEXT));
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
    }
    LeaveCriticalSection(&cs->cs);
    if (ret)
        ((PCTL_CONTEXT)ret)->hCertStore = store;
    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_CollectionDeleteCTL(PWINECRYPT_CERTSTORE store,
 void *pCtlContext)
{
    BOOL ret;

    TRACE("(%p, %p)\n", store, pCtlContext);

    ret = CertDeleteCTLFromStore(
     Context_GetLinkedContext(pCtlContext, sizeof(CTL_CONTEXT)));
    return ret;
}

PWINECRYPT_CERTSTORE CRYPT_CollectionOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINE_COLLECTIONSTORE store;

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
            CRYPT_InitStore(&store->hdr, dwFlags, StoreTypeCollection);
            store->hdr.closeStore          = CRYPT_CollectionCloseStore;
            store->hdr.certs.addContext    = CRYPT_CollectionAddCert;
            store->hdr.certs.enumContext   = CRYPT_CollectionEnumCert;
            store->hdr.certs.deleteContext = CRYPT_CollectionDeleteCert;
            store->hdr.crls.addContext     = CRYPT_CollectionAddCRL;
            store->hdr.crls.enumContext    = CRYPT_CollectionEnumCRL;
            store->hdr.crls.deleteContext  = CRYPT_CollectionDeleteCRL;
            store->hdr.ctls.addContext     = CRYPT_CollectionAddCTL;
            store->hdr.ctls.enumContext    = CRYPT_CollectionEnumCTL;
            store->hdr.ctls.deleteContext  = CRYPT_CollectionDeleteCTL;
            InitializeCriticalSection(&store->cs);
            store->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PWINE_COLLECTIONSTORE->cs");
            list_init(&store->stores);
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

BOOL WINAPI CertAddStoreToCollection(HCERTSTORE hCollectionStore,
 HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
    PWINE_COLLECTIONSTORE collection = hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = hSiblingStore;
    PWINE_STORE_LIST_ENTRY entry;
    BOOL ret;

    TRACE("(%p, %p, %08x, %d)\n", hCollectionStore, hSiblingStore,
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
        TRACE("sibling %p's ref count is %d\n", sibling, sibling->ref);
        entry->store = sibling;
        entry->dwUpdateFlags = dwUpdateFlags;
        entry->dwPriority = dwPriority;
        list_init(&entry->entry);
        TRACE("%p: adding %p, priority %d\n", collection, entry, dwPriority);
        EnterCriticalSection(&collection->cs);
        if (dwPriority)
        {
            PWINE_STORE_LIST_ENTRY cursor;
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
    PWINE_COLLECTIONSTORE collection = hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = hSiblingStore;
    PWINE_STORE_LIST_ENTRY store, next;

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
    LIST_FOR_EACH_ENTRY_SAFE(store, next, &collection->stores,
     WINE_STORE_LIST_ENTRY, entry)
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
