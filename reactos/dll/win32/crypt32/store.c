/*
 * Copyright 2002 Mike McCormack for CodeWeavers
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
 *
 * FIXME:
 * - The concept of physical stores and locations isn't implemented.  (This
 *   doesn't mean registry stores et al aren't implemented.  See the PSDK for
 *   registering and enumerating physical stores and locations.)
 * - Many flags, options and whatnot are unimplemented.
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "excpt.h"
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define WINE_CRYPTCERTSTORE_MAGIC 0x74726563

static const WINE_CONTEXT_INTERFACE gCertInterface = {
    (CreateContextFunc)CertCreateCertificateContext,
    (AddContextToStoreFunc)CertAddCertificateContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCertificateToStore,
    (DuplicateContextFunc)CertDuplicateCertificateContext,
    (EnumContextsInStoreFunc)CertEnumCertificatesInStore,
    (EnumPropertiesFunc)CertEnumCertificateContextProperties,
    (GetContextPropertyFunc)CertGetCertificateContextProperty,
    (SetContextPropertyFunc)CertSetCertificateContextProperty,
    (SerializeElementFunc)CertSerializeCertificateStoreElement,
    (FreeContextFunc)CertFreeCertificateContext,
    (DeleteContextFunc)CertDeleteCertificateFromStore,
};
PCWINE_CONTEXT_INTERFACE pCertInterface = &gCertInterface;

static const WINE_CONTEXT_INTERFACE gCRLInterface = {
    (CreateContextFunc)CertCreateCRLContext,
    (AddContextToStoreFunc)CertAddCRLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCRLToStore,
    (DuplicateContextFunc)CertDuplicateCRLContext,
    (EnumContextsInStoreFunc)CertEnumCRLsInStore,
    (EnumPropertiesFunc)CertEnumCRLContextProperties,
    (GetContextPropertyFunc)CertGetCRLContextProperty,
    (SetContextPropertyFunc)CertSetCRLContextProperty,
    (SerializeElementFunc)CertSerializeCRLStoreElement,
    (FreeContextFunc)CertFreeCRLContext,
    (DeleteContextFunc)CertDeleteCRLFromStore,
};
PCWINE_CONTEXT_INTERFACE pCRLInterface = &gCRLInterface;

static const WINE_CONTEXT_INTERFACE gCTLInterface = {
    (CreateContextFunc)CertCreateCTLContext,
    (AddContextToStoreFunc)CertAddCTLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCTLToStore,
    (DuplicateContextFunc)CertDuplicateCTLContext,
    (EnumContextsInStoreFunc)CertEnumCTLsInStore,
    (EnumPropertiesFunc)CertEnumCTLContextProperties,
    (GetContextPropertyFunc)CertGetCTLContextProperty,
    (SetContextPropertyFunc)CertSetCTLContextProperty,
    (SerializeElementFunc)CertSerializeCTLStoreElement,
    (FreeContextFunc)CertFreeCTLContext,
    (DeleteContextFunc)CertDeleteCTLFromStore,
};
PCWINE_CONTEXT_INTERFACE pCTLInterface = &gCTLInterface;

struct WINE_CRYPTCERTSTORE;

typedef struct WINE_CRYPTCERTSTORE * (*StoreOpenFunc)(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);

/* Called to enumerate the next context in a store. */
typedef void * (*EnumFunc)(struct WINE_CRYPTCERTSTORE *store, void *pPrev);

/* Called to add a context to a store.  If toReplace is not NULL,
 * context replaces toReplace in the store, and access checks should not be
 * performed.  Otherwise context is a new context, and it should only be
 * added if the store allows it.  If ppStoreContext is not NULL, the added
 * context should be returned in *ppStoreContext.
 */
typedef BOOL (*AddFunc)(struct WINE_CRYPTCERTSTORE *store, void *context,
 void *toReplace, const void **ppStoreContext);

typedef BOOL (*DeleteFunc)(struct WINE_CRYPTCERTSTORE *store, void *context);

typedef struct _CONTEXT_STORE
{
    AddFunc    addContext;
    EnumFunc   enumContext;
    DeleteFunc deleteContext;
} CONTEXT_STORE, *PCONTEXT_STORE;

typedef enum _CertStoreType {
    StoreTypeMem,
    StoreTypeCollection,
    StoreTypeProvider,
} CertStoreType;

/* A cert store is polymorphic through the use of function pointers.  A type
 * is still needed to distinguish collection stores from other types.
 * On the function pointers:
 * - closeStore is called when the store's ref count becomes 0
 * - control is optional, but should be implemented by any store that supports
 *   persistence
 */
typedef struct WINE_CRYPTCERTSTORE
{
    DWORD                       dwMagic;
    LONG                        ref;
    DWORD                       dwOpenFlags;
    HCRYPTPROV                  cryptProv;
    CertStoreType               type;
    PFN_CERT_STORE_PROV_CLOSE   closeStore;
    CONTEXT_STORE               certs;
    CONTEXT_STORE               crls;
    PFN_CERT_STORE_PROV_CONTROL control; /* optional */
} WINECRYPT_CERTSTORE, *PWINECRYPT_CERTSTORE;

typedef struct _WINE_MEMSTORE
{
    WINECRYPT_CERTSTORE hdr;
    struct ContextList *certs;
    struct ContextList *crls;
} WINE_MEMSTORE, *PWINE_MEMSTORE;

typedef struct _WINE_HASH_TO_DELETE
{
    BYTE        hash[20];
    struct list entry;
} WINE_HASH_TO_DELETE, *PWINE_HASH_TO_DELETE;

typedef struct _WINE_REGSTOREINFO
{
    DWORD                dwOpenFlags;
    HCRYPTPROV           cryptProv;
    PWINECRYPT_CERTSTORE memStore;
    HKEY                 key;
    BOOL                 dirty;
    CRITICAL_SECTION     cs;
    struct list          certsToDelete;
    struct list          crlsToDelete;
} WINE_REGSTOREINFO, *PWINE_REGSTOREINFO;

typedef struct _WINE_FILESTOREINFO
{
    DWORD                dwOpenFlags;
    HCRYPTPROV           cryptProv;
    PWINECRYPT_CERTSTORE memStore;
    HANDLE               file;
    BOOL                 dirty;
} WINE_FILESTOREINFO, *PWINE_FILESTOREINFO;

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

typedef struct _WINE_PROVIDERSTORE
{
    WINECRYPT_CERTSTORE             hdr;
    DWORD                           dwStoreProvFlags;
    PWINECRYPT_CERTSTORE            memStore;
    HCERTSTOREPROV                  hStoreProv;
    PFN_CERT_STORE_PROV_CLOSE       provCloseStore;
    PFN_CERT_STORE_PROV_WRITE_CERT  provWriteCert;
    PFN_CERT_STORE_PROV_DELETE_CERT provDeleteCert;
    PFN_CERT_STORE_PROV_WRITE_CRL   provWriteCrl;
    PFN_CERT_STORE_PROV_DELETE_CRL  provDeleteCrl;
    PFN_CERT_STORE_PROV_CONTROL     provControl;
} WINE_PROVIDERSTORE, *PWINE_PROVIDERSTORE;

static void CRYPT_InitStore(WINECRYPT_CERTSTORE *store, HCRYPTPROV hCryptProv,
 DWORD dwFlags, CertStoreType type)
{
    store->ref = 1;
    store->dwMagic = WINE_CRYPTCERTSTORE_MAGIC;
    store->type = type;
    if (!hCryptProv)
    {
        hCryptProv = CRYPT_GetDefaultProvider();
        dwFlags |= CERT_STORE_NO_CRYPT_RELEASE_FLAG;
    }
    store->cryptProv = hCryptProv;
    store->dwOpenFlags = dwFlags;
}

static BOOL CRYPT_MemAddCert(PWINECRYPT_CERTSTORE store, void *cert,
 void *toReplace, const void **ppStoreContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    PCERT_CONTEXT context;

    TRACE("(%p, %p, %p, %p)\n", store, cert, toReplace, ppStoreContext);

    context = (PCERT_CONTEXT)ContextList_Add(ms->certs, cert, toReplace);
    if (context)
    {
        context->hCertStore = store;
        if (ppStoreContext)
            *ppStoreContext = CertDuplicateCertificateContext(context);
    }
    return context ? TRUE : FALSE;
}

static void *CRYPT_MemEnumCert(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    ret = ContextList_Enum(ms->certs, pPrev);
    if (!ret)
        SetLastError(CRYPT_E_NOT_FOUND);

    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_MemDeleteCert(PWINECRYPT_CERTSTORE store, void *pCertContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    ContextList_Delete(ms->certs, pCertContext);
    return TRUE;
}

static BOOL CRYPT_MemAddCrl(PWINECRYPT_CERTSTORE store, void *crl,
 void *toReplace, const void **ppStoreContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    PCRL_CONTEXT context;

    TRACE("(%p, %p, %p, %p)\n", store, crl, toReplace, ppStoreContext);

    context = (PCRL_CONTEXT)ContextList_Add(ms->crls, crl, toReplace);
    if (context)
    {
        context->hCertStore = store;
        if (ppStoreContext)
            *ppStoreContext = CertDuplicateCRLContext(context);
    }
    return context ? TRUE : FALSE;
}

static void *CRYPT_MemEnumCrl(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    ret = ContextList_Enum(ms->crls, pPrev);
    if (!ret)
        SetLastError(CRYPT_E_NOT_FOUND);

    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_MemDeleteCrl(PWINECRYPT_CERTSTORE store, void *pCrlContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    ContextList_Delete(ms->crls, pCrlContext);
    return TRUE;
}

static void CRYPT_MemEmptyStore(PWINE_MEMSTORE store)
{
    ContextList_Empty(store->certs);
    ContextList_Empty(store->crls);
}

static void WINAPI CRYPT_MemCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINE_MEMSTORE *store = (WINE_MEMSTORE *)hCertStore;

    TRACE("(%p, %08x)\n", store, dwFlags);
    if (dwFlags)
        FIXME("Unimplemented flags: %08x\n", dwFlags);

    ContextList_Free(store->certs);
    ContextList_Free(store->crls);
    CryptMemFree(store);
}

static WINECRYPT_CERTSTORE *CRYPT_MemOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINE_MEMSTORE store;

    TRACE("(%ld, %08x, %p)\n", hCryptProv, dwFlags, pvPara);

    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        store = NULL;
    }
    else
    {
        store = CryptMemAlloc(sizeof(WINE_MEMSTORE));
        if (store)
        {
            memset(store, 0, sizeof(WINE_MEMSTORE));
            CRYPT_InitStore(&store->hdr, hCryptProv, dwFlags, StoreTypeMem);
            store->hdr.closeStore          = CRYPT_MemCloseStore;
            store->hdr.certs.addContext    = CRYPT_MemAddCert;
            store->hdr.certs.enumContext   = CRYPT_MemEnumCert;
            store->hdr.certs.deleteContext = CRYPT_MemDeleteCert;
            store->hdr.crls.addContext     = CRYPT_MemAddCrl;
            store->hdr.crls.enumContext    = CRYPT_MemEnumCrl;
            store->hdr.crls.deleteContext  = CRYPT_MemDeleteCrl;
            store->hdr.control             = NULL;
            store->certs = ContextList_Create(pCertInterface,
             sizeof(CERT_CONTEXT));
            store->crls = ContextList_Create(pCRLInterface,
             sizeof(CRL_CONTEXT));
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static void WINAPI CRYPT_CollectionCloseStore(HCERTSTORE store, DWORD dwFlags)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    PWINE_STORE_LIST_ENTRY entry, next;

    TRACE("(%p, %08x)\n", store, dwFlags);

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &cs->stores, WINE_STORE_LIST_ENTRY,
     entry)
    {
        TRACE("closing %p\n", entry);
        CertCloseStore((HCERTSTORE)entry->store, dwFlags);
        CryptMemFree(entry);
    }
    cs->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&cs->cs);
    CryptMemFree(cs);
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
 unsigned int contextStoreOffset, void *context, void *toReplace, unsigned int contextSize,
 void **pChildContext)
{
    BOOL ret;
    void *childContext = NULL;
    PWINE_STORE_LIST_ENTRY storeEntry = NULL;

    TRACE("(%p, %d, %p, %p, %d)\n", store, contextStoreOffset, context,
     toReplace, contextSize);

    ret = FALSE;
    if (toReplace)
    {
        void *existingLinked = Context_GetLinkedContext(toReplace, contextSize);
        PCONTEXT_STORE contextStore;

        storeEntry = *(PWINE_STORE_LIST_ENTRY *)Context_GetExtra(toReplace,
         contextSize);
        contextStore = (PCONTEXT_STORE)((LPBYTE)storeEntry->store +
         contextStoreOffset);
        ret = contextStore->addContext(storeEntry->store, context,
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
                PCONTEXT_STORE contextStore = (PCONTEXT_STORE)(
                 (LPBYTE)entry->store + contextStoreOffset);

                storeEntry = entry;
                ret = contextStore->addContext(entry->store, context, NULL,
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
 PWINE_STORE_LIST_ENTRY storeEntry, size_t contextStoreOffset,
 PCWINE_CONTEXT_INTERFACE contextInterface, void *pPrev, size_t contextSize)
{
    void *ret, *child;
    struct list *storeNext = list_next(&store->stores, &storeEntry->entry);
    PCONTEXT_STORE contextStore = (PCONTEXT_STORE)((LPBYTE)storeEntry->store +
     contextStoreOffset);

    TRACE("(%p, %p, %p)\n", store, storeEntry, pPrev);

    if (pPrev)
    {
        /* Ref-counting funny business: "duplicate" (addref) the child, because
         * the free(pPrev) below can cause the ref count to become negative.
         */
        child = Context_GetLinkedContext(pPrev, contextSize);
        contextInterface->duplicate(child);
        child = contextStore->enumContext(storeEntry->store, child);
        contextInterface->free(pPrev);
        pPrev = NULL;
    }
    else
        child = storeEntry->store->certs.enumContext(storeEntry->store, NULL);
    if (child)
        ret = CRYPT_CollectionCreateContextFromChild(store, storeEntry, child,
         contextSize, FALSE);
    else
    {
        if (storeNext)
            ret = CRYPT_CollectionAdvanceEnum(store, LIST_ENTRY(storeNext,
             WINE_STORE_LIST_ENTRY, entry), contextStoreOffset,
             contextInterface, NULL, contextSize);
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
    CertFreeCertificateContext((PCCERT_CONTEXT)childContext);
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
         offsetof(WINECRYPT_CERTSTORE, certs), pCertInterface, pPrev,
         sizeof(CERT_CONTEXT));
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             offsetof(WINECRYPT_CERTSTORE, certs), pCertInterface, NULL,
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

    ret = CertDeleteCertificateFromStore((PCCERT_CONTEXT)
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
    CertFreeCRLContext((PCCRL_CONTEXT)childContext);
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
         offsetof(WINECRYPT_CERTSTORE, crls), pCRLInterface, pPrev,
         sizeof(CRL_CONTEXT));
    }
    else
    {
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry = LIST_ENTRY(cs->stores.next,
             WINE_STORE_LIST_ENTRY, entry);

            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry,
             offsetof(WINECRYPT_CERTSTORE, crls), pCRLInterface, NULL,
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

    ret = CertDeleteCRLFromStore((PCCRL_CONTEXT)
     Context_GetLinkedContext(pCrlContext, sizeof(CRL_CONTEXT)));
    return ret;
}

static WINECRYPT_CERTSTORE *CRYPT_CollectionOpenStore(HCRYPTPROV hCryptProv,
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
            CRYPT_InitStore(&store->hdr, hCryptProv, dwFlags,
             StoreTypeCollection);
            store->hdr.closeStore          = CRYPT_CollectionCloseStore;
            store->hdr.certs.addContext    = CRYPT_CollectionAddCert;
            store->hdr.certs.enumContext   = CRYPT_CollectionEnumCert;
            store->hdr.certs.deleteContext = CRYPT_CollectionDeleteCert;
            store->hdr.crls.addContext     = CRYPT_CollectionAddCRL;
            store->hdr.crls.enumContext    = CRYPT_CollectionEnumCRL;
            store->hdr.crls.deleteContext  = CRYPT_CollectionDeleteCRL;
            InitializeCriticalSection(&store->cs);
            store->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PWINE_COLLECTIONSTORE->cs");
            list_init(&store->stores);
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static void WINAPI CRYPT_ProvCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    PWINE_PROVIDERSTORE store = (PWINE_PROVIDERSTORE)hCertStore;

    TRACE("(%p, %08x)\n", store, dwFlags);

    if (store->provCloseStore)
        store->provCloseStore(store->hStoreProv, dwFlags);
    if (!(store->dwStoreProvFlags & CERT_STORE_PROV_EXTERNAL_FLAG))
        CertCloseStore(store->memStore, dwFlags);
    CryptMemFree(store);
}

static BOOL CRYPT_ProvAddCert(PWINECRYPT_CERTSTORE store, void *cert,
 void *toReplace, const void **ppStoreContext)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    BOOL ret;

    TRACE("(%p, %p, %p, %p)\n", store, cert, toReplace, ppStoreContext);

    if (toReplace)
        ret = ps->memStore->certs.addContext(ps->memStore, cert, toReplace,
         (const void **)ppStoreContext);
    else
    {
        ret = TRUE;
        if (ps->provWriteCert)
            ret = ps->provWriteCert(ps->hStoreProv, (PCCERT_CONTEXT)cert,
             CERT_STORE_PROV_WRITE_ADD_FLAG);
        if (ret)
            ret = ps->memStore->certs.addContext(ps->memStore, cert, NULL,
             (const void **)ppStoreContext);
    }
    /* dirty trick: replace the returned context's hCertStore with
     * store.
     */
    if (ppStoreContext)
        (*(PCERT_CONTEXT *)ppStoreContext)->hCertStore = store;
    return ret;
}

static void *CRYPT_ProvEnumCert(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    void *ret;

    ret = ps->memStore->certs.enumContext(ps->memStore, pPrev);
    if (ret)
    {
        /* same dirty trick: replace the returned context's hCertStore with
         * store.
         */
        ((PCERT_CONTEXT)ret)->hCertStore = store;
    }
    return ret;
}

static BOOL CRYPT_ProvDeleteCert(PWINECRYPT_CERTSTORE store, void *cert)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    BOOL ret = TRUE;

    TRACE("(%p, %p)\n", store, cert);

    if (ps->provDeleteCert)
        ret = ps->provDeleteCert(ps->hStoreProv, cert, 0);
    if (ret)
        ret = ps->memStore->certs.deleteContext(ps->memStore, cert);
    return ret;
}

static BOOL CRYPT_ProvAddCRL(PWINECRYPT_CERTSTORE store, void *crl,
 void *toReplace, const void **ppStoreContext)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    BOOL ret;

    TRACE("(%p, %p, %p, %p)\n", store, crl, toReplace, ppStoreContext);

    if (toReplace)
        ret = ps->memStore->crls.addContext(ps->memStore, crl, toReplace,
         (const void **)ppStoreContext);
    else
    {
        if (ps->hdr.dwOpenFlags & CERT_STORE_READONLY_FLAG)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
            if (ps->provWriteCrl)
                ret = ps->provWriteCrl(ps->hStoreProv, (PCCRL_CONTEXT)crl,
                 CERT_STORE_PROV_WRITE_ADD_FLAG);
            if (ret)
                ret = ps->memStore->crls.addContext(ps->memStore, crl, NULL,
                 (const void **)ppStoreContext);
        }
    }
    /* dirty trick: replace the returned context's hCertStore with
     * store.
     */
    if (ppStoreContext)
        (*(PCRL_CONTEXT *)ppStoreContext)->hCertStore = store;
    return ret;
}

static void *CRYPT_ProvEnumCRL(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    void *ret;

    ret = ps->memStore->crls.enumContext(ps->memStore, pPrev);
    if (ret)
    {
        /* same dirty trick: replace the returned context's hCertStore with
         * store.
         */
        ((PCERT_CONTEXT)ret)->hCertStore = store;
    }
    return ret;
}

static BOOL CRYPT_ProvDeleteCRL(PWINECRYPT_CERTSTORE store, void *crl)
{
    PWINE_PROVIDERSTORE ps = (PWINE_PROVIDERSTORE)store;
    BOOL ret = TRUE;

    TRACE("(%p, %p)\n", store, crl);

    if (ps->provDeleteCrl)
        ret = ps->provDeleteCrl(ps->hStoreProv, crl, 0);
    if (ret)
        ret = ps->memStore->crls.deleteContext(ps->memStore, crl);
    return ret;
}

static BOOL WINAPI CRYPT_ProvControl(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    PWINE_PROVIDERSTORE store = (PWINE_PROVIDERSTORE)hCertStore;
    BOOL ret = TRUE;

    TRACE("(%p, %08x, %d, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    if (store->provControl)
        ret = store->provControl(store->hStoreProv, dwFlags, dwCtrlType,
         pvCtrlPara);
    return ret;
}

static PWINECRYPT_CERTSTORE CRYPT_ProvCreateStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, PWINECRYPT_CERTSTORE memStore, const CERT_STORE_PROV_INFO *pProvInfo)
{
    PWINE_PROVIDERSTORE ret = (PWINE_PROVIDERSTORE)CryptMemAlloc(
     sizeof(WINE_PROVIDERSTORE));

    if (ret)
    {
        CRYPT_InitStore(&ret->hdr, hCryptProv, dwFlags,
         StoreTypeProvider);
        ret->dwStoreProvFlags = pProvInfo->dwStoreProvFlags;
        if (ret->dwStoreProvFlags & CERT_STORE_PROV_EXTERNAL_FLAG)
        {
            CertCloseStore(memStore, 0);
            ret->memStore = NULL;
        }
        else
            ret->memStore = memStore;
        ret->hStoreProv = pProvInfo->hStoreProv;
        ret->hdr.closeStore = CRYPT_ProvCloseStore;
        ret->hdr.certs.addContext = CRYPT_ProvAddCert;
        ret->hdr.certs.enumContext = CRYPT_ProvEnumCert;
        ret->hdr.certs.deleteContext = CRYPT_ProvDeleteCert;
        ret->hdr.crls.addContext = CRYPT_ProvAddCRL;
        ret->hdr.crls.enumContext = CRYPT_ProvEnumCRL;
        ret->hdr.crls.deleteContext = CRYPT_ProvDeleteCRL;
        ret->hdr.control = CRYPT_ProvControl;
        if (pProvInfo->cStoreProvFunc > CERT_STORE_PROV_CLOSE_FUNC)
            ret->provCloseStore =
             pProvInfo->rgpvStoreProvFunc[CERT_STORE_PROV_CLOSE_FUNC];
        else
            ret->provCloseStore = NULL;
        if (pProvInfo->cStoreProvFunc >
         CERT_STORE_PROV_WRITE_CERT_FUNC)
            ret->provWriteCert = pProvInfo->rgpvStoreProvFunc[
             CERT_STORE_PROV_WRITE_CERT_FUNC];
        else
            ret->provWriteCert = NULL;
        if (pProvInfo->cStoreProvFunc >
         CERT_STORE_PROV_DELETE_CERT_FUNC)
            ret->provDeleteCert = pProvInfo->rgpvStoreProvFunc[
             CERT_STORE_PROV_DELETE_CERT_FUNC];
        else
            ret->provDeleteCert = NULL;
        if (pProvInfo->cStoreProvFunc >
         CERT_STORE_PROV_WRITE_CRL_FUNC)
            ret->provWriteCrl = pProvInfo->rgpvStoreProvFunc[
             CERT_STORE_PROV_WRITE_CRL_FUNC];
        else
            ret->provWriteCert = NULL;
        if (pProvInfo->cStoreProvFunc >
         CERT_STORE_PROV_DELETE_CRL_FUNC)
            ret->provDeleteCrl = pProvInfo->rgpvStoreProvFunc[
             CERT_STORE_PROV_DELETE_CRL_FUNC];
        else
            ret->provDeleteCert = NULL;
        if (pProvInfo->cStoreProvFunc >
         CERT_STORE_PROV_CONTROL_FUNC)
            ret->provControl = pProvInfo->rgpvStoreProvFunc[
             CERT_STORE_PROV_CONTROL_FUNC];
        else
            ret->provControl = NULL;
    }
    return (PWINECRYPT_CERTSTORE)ret;
}

static PWINECRYPT_CERTSTORE CRYPT_ProvOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara)
{
    static HCRYPTOIDFUNCSET set = NULL;
    PFN_CERT_DLL_OPEN_STORE_PROV_FUNC provOpenFunc;
    HCRYPTOIDFUNCADDR hFunc;
    PWINECRYPT_CERTSTORE ret = NULL;

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_OPEN_STORE_PROV_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwEncodingType, lpszStoreProvider, 0,
     (void **)&provOpenFunc, &hFunc);
    if (provOpenFunc)
    {
        CERT_STORE_PROV_INFO provInfo = { 0 };

        provInfo.cbSize = sizeof(provInfo);
        if (dwFlags & CERT_STORE_DELETE_FLAG)
            provOpenFunc(lpszStoreProvider, dwEncodingType, hCryptProv,
             dwFlags, pvPara, NULL, &provInfo);
        else
        {
            HCERTSTORE memStore;

            memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);
            if (memStore)
            {
                if (provOpenFunc(lpszStoreProvider, dwEncodingType, hCryptProv,
                 dwFlags, pvPara, memStore, &provInfo))
                    ret = CRYPT_ProvCreateStore(hCryptProv, dwFlags, memStore,
                     &provInfo);
                else
                    CertCloseStore(memStore, 0);
            }
        }
        CryptFreeOIDFunctionAddress(hFunc, 0);
    }
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    return ret;
}

static void CRYPT_HashToStr(const BYTE *hash, LPWSTR asciiHash)
{
    static const WCHAR fmt[] = { '%','0','2','X',0 };
    DWORD i;

    assert(hash);
    assert(asciiHash);

    for (i = 0; i < 20; i++)
        wsprintfW(asciiHash + i * 2, fmt, hash[i]);
}

static const WCHAR CertsW[] = { 'C','e','r','t','i','f','i','c','a','t','e','s',
 0 };
static const WCHAR CRLsW[] = { 'C','R','L','s',0 };
static const WCHAR CTLsW[] = { 'C','T','L','s',0 };
static const WCHAR BlobW[] = { 'B','l','o','b',0 };

static void CRYPT_RegReadSerializedFromReg(const WINE_REGSTOREINFO *store, HKEY key,
 DWORD contextType)
{
    LONG rc;
    DWORD index = 0;
    WCHAR subKeyName[MAX_PATH];

    do {
        DWORD size = sizeof(subKeyName) / sizeof(WCHAR);

        rc = RegEnumKeyExW(key, index++, subKeyName, &size, NULL, NULL, NULL,
         NULL);
        if (!rc)
        {
            HKEY subKey;

            rc = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
            if (!rc)
            {
                LPBYTE buf = NULL;

                size = 0;
                rc = RegQueryValueExW(subKey, BlobW, NULL, NULL, NULL, &size);
                if (!rc)
                    buf = CryptMemAlloc(size);
                if (buf)
                {
                    rc = RegQueryValueExW(subKey, BlobW, NULL, NULL, buf,
                     &size);
                    if (!rc)
                    {
                        const void *context;
                        DWORD addedType;

                        TRACE("Adding cert with hash %s\n",
                         debugstr_w(subKeyName));
                        context = CRYPT_ReadSerializedElement(buf, size,
                         contextType, &addedType);
                        if (context)
                        {
                            const WINE_CONTEXT_INTERFACE *contextInterface;
                            BYTE hash[20];

                            switch (addedType)
                            {
                            case CERT_STORE_CERTIFICATE_CONTEXT:
                                contextInterface = &gCertInterface;
                                break;
                            case CERT_STORE_CRL_CONTEXT:
                                contextInterface = &gCRLInterface;
                                break;
                            case CERT_STORE_CTL_CONTEXT:
                                contextInterface = &gCTLInterface;
                                break;
                            default:
                                contextInterface = NULL;
                            }
                            if (contextInterface)
                            {
                                size = sizeof(hash);
                                if (contextInterface->getProp(context,
                                 CERT_HASH_PROP_ID, hash, &size))
                                {
                                    WCHAR asciiHash[20 * 2 + 1];

                                    CRYPT_HashToStr(hash, asciiHash);
                                    TRACE("comparing %s\n",
                                     debugstr_w(asciiHash));
                                    TRACE("with %s\n", debugstr_w(subKeyName));
                                    if (!lstrcmpW(asciiHash, subKeyName))
                                    {
                                        TRACE("hash matches, adding\n");
                                        contextInterface->addContextToStore(
                                         store->memStore, context,
                                         CERT_STORE_ADD_REPLACE_EXISTING, NULL);
                                    }
                                    else
                                        TRACE("hash doesn't match, ignoring\n");
                                }
                                contextInterface->free(context);
                            }
                        }
                    }
                    CryptMemFree(buf);
                }
                RegCloseKey(subKey);
            }
            /* Ignore intermediate errors, continue enumerating */
            rc = ERROR_SUCCESS;
        }
    } while (!rc);
}

static void CRYPT_RegReadFromReg(const WINE_REGSTOREINFO *store)
{
    static const WCHAR * const subKeys[] = { CertsW, CRLsW, CTLsW };
    static const DWORD contextFlags[] = { CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
     CERT_STORE_CRL_CONTEXT_FLAG, CERT_STORE_CTL_CONTEXT_FLAG };
    DWORD i;

    for (i = 0; i < sizeof(subKeys) / sizeof(subKeys[0]); i++)
    {
        HKEY key;
        LONG rc;

        rc = RegCreateKeyExW(store->key, subKeys[i], 0, NULL, 0, KEY_READ, NULL,
         &key, NULL);
        if (!rc)
        {
            CRYPT_RegReadSerializedFromReg(store, key, contextFlags[i]);
            RegCloseKey(key);
        }
    }
}

/* Hash is assumed to be 20 bytes in length (a SHA-1 hash) */
static BOOL CRYPT_WriteSerializedToReg(HKEY key, const BYTE *hash, const BYTE *buf,
 DWORD len)
{
    WCHAR asciiHash[20 * 2 + 1];
    LONG rc;
    HKEY subKey;
    BOOL ret;

    CRYPT_HashToStr(hash, asciiHash);
    rc = RegCreateKeyExW(key, asciiHash, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
     &subKey, NULL);
    if (!rc)
    {
        rc = RegSetValueExW(subKey, BlobW, 0, REG_BINARY, buf, len);
        RegCloseKey(subKey);
    }
    if (!rc)
        ret = TRUE;
    else
    {
        SetLastError(rc);
        ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_SerializeContextsToReg(HKEY key,
 const WINE_CONTEXT_INTERFACE *contextInterface, HCERTSTORE memStore)
{
    const void *context = NULL;
    BOOL ret;

    do {
        context = contextInterface->enumContextsInStore(memStore, context);
        if (context)
        {
            BYTE hash[20];
            DWORD hashSize = sizeof(hash);

            ret = contextInterface->getProp(context, CERT_HASH_PROP_ID, hash,
             &hashSize);
            if (ret)
            {
                DWORD size = 0;
                LPBYTE buf = NULL;

                ret = contextInterface->serialize(context, 0, NULL, &size);
                if (size)
                    buf = CryptMemAlloc(size);
                if (buf)
                {
                    ret = contextInterface->serialize(context, 0, buf, &size);
                    if (ret)
                        ret = CRYPT_WriteSerializedToReg(key, hash, buf, size);
                }
                CryptMemFree(buf);
            }
        }
        else
            ret = TRUE;
    } while (ret && context != NULL);
    if (context)
        contextInterface->free(context);
    return ret;
}

static BOOL CRYPT_RegWriteToReg(PWINE_REGSTOREINFO store)
{
    static const WCHAR * const subKeys[] = { CertsW, CRLsW, CTLsW };
    static const WINE_CONTEXT_INTERFACE * const interfaces[] = { &gCertInterface,
     &gCRLInterface, &gCTLInterface };
    struct list *listToDelete[] = { &store->certsToDelete, &store->crlsToDelete,
     NULL };
    BOOL ret = TRUE;
    DWORD i;

    for (i = 0; ret && i < sizeof(subKeys) / sizeof(subKeys[0]); i++)
    {
        HKEY key;
        LONG rc = RegCreateKeyExW(store->key, subKeys[i], 0, NULL, 0,
         KEY_ALL_ACCESS, NULL, &key, NULL);

        if (!rc)
        {
            if (listToDelete[i])
            {
                PWINE_HASH_TO_DELETE toDelete, next;
                WCHAR asciiHash[20 * 2 + 1];

                EnterCriticalSection(&store->cs);
                LIST_FOR_EACH_ENTRY_SAFE(toDelete, next, listToDelete[i],
                 WINE_HASH_TO_DELETE, entry)
                {
                    LONG rc;

                    CRYPT_HashToStr(toDelete->hash, asciiHash);
                    TRACE("Removing %s\n", debugstr_w(asciiHash));
                    rc = RegDeleteKeyW(key, asciiHash);
                    if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND)
                    {
                        SetLastError(rc);
                        ret = FALSE;
                    }
                    list_remove(&toDelete->entry);
                    CryptMemFree(toDelete);
                }
                LeaveCriticalSection(&store->cs);
            }
            ret = CRYPT_SerializeContextsToReg(key, interfaces[i],
             store->memStore);
            RegCloseKey(key);
        }
        else
        {
            SetLastError(rc);
            ret = FALSE;
        }
    }
    return ret;
}

/* If force is true or the registry store is dirty, writes the contents of the
 * store to the registry.
 */
static BOOL CRYPT_RegFlushStore(PWINE_REGSTOREINFO store, BOOL force)
{
    BOOL ret;

    TRACE("(%p, %d)\n", store, force);

    if (store->dirty || force)
        ret = CRYPT_RegWriteToReg(store);
    else
        ret = TRUE;
    return ret;
}

static void WINAPI CRYPT_RegCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;

    TRACE("(%p, %08x)\n", store, dwFlags);
    if (dwFlags)
        FIXME("Unimplemented flags: %08x\n", dwFlags);

    CRYPT_RegFlushStore(store, FALSE);
    RegCloseKey(store->key);
    store->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&store->cs);
    CryptMemFree(store);
}

static BOOL WINAPI CRYPT_RegWriteContext(PWINE_REGSTOREINFO store,
 const void *context, DWORD dwFlags)
{
    BOOL ret;

    if (dwFlags & CERT_STORE_PROV_WRITE_ADD_FLAG)
    {
        store->dirty = TRUE;
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL CRYPT_RegDeleteContext(PWINE_REGSTOREINFO store,
 struct list *deleteList, const void *context,
 PCWINE_CONTEXT_INTERFACE contextInterface)
{
    BOOL ret;

    if (store->dwOpenFlags & CERT_STORE_READONLY_FLAG)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        ret = FALSE;
    }
    else
    {
        PWINE_HASH_TO_DELETE toDelete =
         CryptMemAlloc(sizeof(WINE_HASH_TO_DELETE));

        if (toDelete)
        {
            DWORD size = sizeof(toDelete->hash);

            ret = contextInterface->getProp(context, CERT_HASH_PROP_ID,
             toDelete->hash, &size);
            if (ret)
            {
                EnterCriticalSection(&store->cs);
                list_add_tail(deleteList, &toDelete->entry);
                LeaveCriticalSection(&store->cs);
            }
            else
            {
                CryptMemFree(toDelete);
                ret = FALSE;
            }
        }
        else
            ret = FALSE;
        if (ret)
            store->dirty = TRUE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_RegWriteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT cert, DWORD dwFlags)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;

    TRACE("(%p, %p, %d)\n", hCertStore, cert, dwFlags);

    return CRYPT_RegWriteContext(store, cert, dwFlags);
}

static BOOL WINAPI CRYPT_RegDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;

    TRACE("(%p, %p, %08x)\n", store, pCertContext, dwFlags);

    return CRYPT_RegDeleteContext(store, &store->certsToDelete, pCertContext,
     pCertInterface);
}

static BOOL WINAPI CRYPT_RegWriteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT crl, DWORD dwFlags)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;

    TRACE("(%p, %p, %d)\n", hCertStore, crl, dwFlags);

    return CRYPT_RegWriteContext(store, crl, dwFlags);
}

static BOOL WINAPI CRYPT_RegDeleteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;

    TRACE("(%p, %p, %08x)\n", store, pCrlContext, dwFlags);

    return CRYPT_RegDeleteContext(store, &store->crlsToDelete, pCrlContext,
     pCRLInterface);
}

static BOOL WINAPI CRYPT_RegControl(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    PWINE_REGSTOREINFO store = (PWINE_REGSTOREINFO)hCertStore;
    BOOL ret;

    TRACE("(%p, %08x, %d, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    switch (dwCtrlType)
    {
    case CERT_STORE_CTRL_RESYNC:
        CRYPT_RegFlushStore(store, FALSE);
        CRYPT_MemEmptyStore((PWINE_MEMSTORE)store->memStore);
        CRYPT_RegReadFromReg(store);
        ret = TRUE;
        break;
    case CERT_STORE_CTRL_COMMIT:
        ret = CRYPT_RegFlushStore(store,
         dwFlags & CERT_STORE_CTRL_COMMIT_FORCE_FLAG);
        break;
    default:
        FIXME("%d: stub\n", dwCtrlType);
        ret = FALSE;
    }
    return ret;
}

/* Copied from shlwapi's SHDeleteKeyW, and reformatted to match this file. */
static DWORD CRYPT_RecurseDeleteKey(HKEY hKey, LPCWSTR lpszSubKey)
{
    DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = 0;

    TRACE("(hkey=%p,%s)\n", hKey, debugstr_w(lpszSubKey));

    dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
    if (!dwRet)
    {
        /* Find how many subkeys there are */
        dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
         &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
        if (!dwRet)
        {
            dwMaxSubkeyLen++;
            if (dwMaxSubkeyLen > sizeof(szNameBuf)/sizeof(WCHAR))
            {
                /* Name too big: alloc a buffer for it */
                lpszName = CryptMemAlloc(dwMaxSubkeyLen*sizeof(WCHAR));
            }

            if (!lpszName)
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                /* Recursively delete all the subkeys */
                for (i = 0; i < dwKeyCount && !dwRet; i++)
                {
                    dwSize = dwMaxSubkeyLen;
                    dwRet = RegEnumKeyExW(hSubKey, i, lpszName, &dwSize, NULL,
                     NULL, NULL, NULL);
                    if (!dwRet)
                        dwRet = CRYPT_RecurseDeleteKey(hSubKey, lpszName);
                }

                if (lpszName != szNameBuf)
                {
                    /* Free buffer if allocated */
                    CryptMemFree(lpszName);
                }
            }
        }

        RegCloseKey(hSubKey);
        if (!dwRet)
            dwRet = RegDeleteKeyW(hKey, lpszSubKey);
    }
    return dwRet;
}

static void *regProvFuncs[] = {
    CRYPT_RegCloseStore,
    NULL, /* CERT_STORE_PROV_READ_CERT_FUNC */
    CRYPT_RegWriteCert,
    CRYPT_RegDeleteCert,
    NULL, /* CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CRL_FUNC */
    CRYPT_RegWriteCRL,
    CRYPT_RegDeleteCRL,
    NULL, /* CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_WRITE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_DELETE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC */
    CRYPT_RegControl,
};

static WINECRYPT_CERTSTORE *CRYPT_RegOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINECRYPT_CERTSTORE store = NULL;

    TRACE("(%ld, %08x, %p)\n", hCryptProv, dwFlags, pvPara);

    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        DWORD rc = CRYPT_RecurseDeleteKey((HKEY)pvPara, CertsW);

        if (rc == ERROR_SUCCESS || rc == ERROR_NO_MORE_ITEMS)
            rc = CRYPT_RecurseDeleteKey((HKEY)pvPara, CRLsW);
        if (rc == ERROR_SUCCESS || rc == ERROR_NO_MORE_ITEMS)
            rc = CRYPT_RecurseDeleteKey((HKEY)pvPara, CTLsW);
        if (rc == ERROR_NO_MORE_ITEMS)
            rc = ERROR_SUCCESS;
        SetLastError(rc);
    }
    else
    {
        HKEY key;

        if (DuplicateHandle(GetCurrentProcess(), (HANDLE)pvPara,
         GetCurrentProcess(), (LPHANDLE)&key,
         dwFlags & CERT_STORE_READONLY_FLAG ? KEY_READ : KEY_ALL_ACCESS,
         TRUE, 0))
        {
            PWINECRYPT_CERTSTORE memStore;

            memStore = CRYPT_MemOpenStore(hCryptProv, dwFlags, NULL);
            if (memStore)
            {
                PWINE_REGSTOREINFO regInfo = CryptMemAlloc(
                 sizeof(WINE_REGSTOREINFO));

                if (regInfo)
                {
                    CERT_STORE_PROV_INFO provInfo = { 0 };

                    regInfo->dwOpenFlags = dwFlags;
                    regInfo->cryptProv = hCryptProv;
                    regInfo->memStore = memStore;
                    regInfo->key = key;
                    InitializeCriticalSection(&regInfo->cs);
                    regInfo->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PWINE_REGSTOREINFO->cs");
                    list_init(&regInfo->certsToDelete);
                    list_init(&regInfo->crlsToDelete);
                    CRYPT_RegReadFromReg(regInfo);
                    regInfo->dirty = FALSE;
                    provInfo.cbSize = sizeof(provInfo);
                    provInfo.cStoreProvFunc = sizeof(regProvFuncs) /
                     sizeof(regProvFuncs[0]);
                    provInfo.rgpvStoreProvFunc = regProvFuncs;
                    provInfo.hStoreProv = regInfo;
                    store = CRYPT_ProvCreateStore(hCryptProv, dwFlags, memStore,
                     &provInfo);
                }
            }
        }
    }
    TRACE("returning %p\n", store);
    return store;
}

/* FIXME: this isn't complete for the Root store, in which the top-level
 * self-signed CA certs reside.  Adding a cert to the Root store should present
 * the user with a dialog indicating the consequences of doing so, and asking
 * the user to confirm whether the cert should be added.
 */
static PWINECRYPT_CERTSTORE CRYPT_SysRegOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    static const WCHAR fmt[] = { '%','s','\\','%','s',0 };
    LPCWSTR storeName = (LPCWSTR)pvPara;
    LPWSTR storePath;
    PWINECRYPT_CERTSTORE store = NULL;
    HKEY root;
    LPCWSTR base;
    BOOL ret;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_w((LPCWSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    ret = TRUE;
    switch (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK)
    {
    case CERT_SYSTEM_STORE_LOCAL_MACHINE:
        root = HKEY_LOCAL_MACHINE;
        base = CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH;
        break;
    case CERT_SYSTEM_STORE_CURRENT_USER:
        root = HKEY_CURRENT_USER;
        base = CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH;
        break;
    case CERT_SYSTEM_STORE_CURRENT_SERVICE:
        /* hklm\Software\Microsoft\Cryptography\Services\servicename\
         * SystemCertificates
         */
        FIXME("CERT_SYSTEM_STORE_CURRENT_SERVICE, %s: stub\n",
         debugstr_w(storeName));
        return NULL;
    case CERT_SYSTEM_STORE_SERVICES:
        /* hklm\Software\Microsoft\Cryptography\Services\servicename\
         * SystemCertificates
         */
        FIXME("CERT_SYSTEM_STORE_SERVICES, %s: stub\n",
         debugstr_w(storeName));
        return NULL;
    case CERT_SYSTEM_STORE_USERS:
        /* hku\user sid\Software\Microsoft\SystemCertificates */
        FIXME("CERT_SYSTEM_STORE_USERS, %s: stub\n",
         debugstr_w(storeName));
        return NULL;
    case CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY:
        root = HKEY_CURRENT_USER;
        base = CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH;
        break;
    case CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY:
        root = HKEY_LOCAL_MACHINE;
        base = CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH;
        break;
    case CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE:
        /* hklm\Software\Microsoft\EnterpriseCertificates */
        FIXME("CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, %s: stub\n",
         debugstr_w(storeName));
        return NULL;
    default:
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    storePath = CryptMemAlloc((lstrlenW(base) + lstrlenW(storeName) + 2) *
     sizeof(WCHAR));
    if (storePath)
    {
        LONG rc;
        HKEY key;
        REGSAM sam = dwFlags & CERT_STORE_READONLY_FLAG ? KEY_READ :
            KEY_ALL_ACCESS;

        wsprintfW(storePath, fmt, base, storeName);
        if (dwFlags & CERT_STORE_OPEN_EXISTING_FLAG)
            rc = RegOpenKeyExW(root, storePath, 0, sam, &key);
        else
        {
            DWORD disp;

            rc = RegCreateKeyExW(root, storePath, 0, NULL, 0, sam, NULL,
                                 &key, &disp);
            if (!rc && dwFlags & CERT_STORE_CREATE_NEW_FLAG &&
                disp == REG_OPENED_EXISTING_KEY)
            {
                RegCloseKey(key);
                rc = ERROR_FILE_EXISTS;
            }
        }
        if (!rc)
        {
            store = CRYPT_RegOpenStore(hCryptProv, dwFlags, key);
            RegCloseKey(key);
        }
        else
            SetLastError(rc);
        CryptMemFree(storePath);
    }
    return store;
}

static PWINECRYPT_CERTSTORE CRYPT_SysRegOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    int len;
    PWINECRYPT_CERTSTORE ret = NULL;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_a((LPCSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, NULL, 0);
    if (len)
    {
        LPWSTR storeName = CryptMemAlloc(len * sizeof(WCHAR));

        if (storeName)
        {
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, storeName, len);
            ret = CRYPT_SysRegOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}

static PWINECRYPT_CERTSTORE CRYPT_SysOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCERTSTORE store = 0;
    BOOL ret;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_w((LPCWSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    /* This returns a different error than system registry stores if the
     * location is invalid.
     */
    switch (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK)
    {
    case CERT_SYSTEM_STORE_LOCAL_MACHINE:
    case CERT_SYSTEM_STORE_CURRENT_USER:
    case CERT_SYSTEM_STORE_CURRENT_SERVICE:
    case CERT_SYSTEM_STORE_SERVICES:
    case CERT_SYSTEM_STORE_USERS:
    case CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY:
    case CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY:
    case CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE:
        ret = TRUE;
        break;
    default:
        SetLastError(ERROR_FILE_NOT_FOUND);
        ret = FALSE;
    }
    if (ret)
    {
        HCERTSTORE regStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,
         0, hCryptProv, dwFlags, pvPara);

        if (regStore)
        {
            store = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);
            CertAddStoreToCollection(store, regStore,
             dwFlags & CERT_STORE_READONLY_FLAG ? 0 :
             CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
            CertCloseStore(regStore, 0);
            /* CERT_SYSTEM_STORE_CURRENT_USER returns both the HKCU and HKLM
             * stores.
             */
            if ((dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) ==
             CERT_SYSTEM_STORE_CURRENT_USER)
            {
                dwFlags &= ~CERT_SYSTEM_STORE_CURRENT_USER;
                dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
                regStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0,
                 hCryptProv, dwFlags, pvPara);
                if (regStore)
                {
                    CertAddStoreToCollection(store, regStore,
                     dwFlags & CERT_STORE_READONLY_FLAG ? 0 :
                     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
                    CertCloseStore(regStore, 0);
                }
            }
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static PWINECRYPT_CERTSTORE CRYPT_SysOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    int len;
    PWINECRYPT_CERTSTORE ret = NULL;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_a((LPCSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, NULL, 0);
    if (len)
    {
        LPWSTR storeName = CryptMemAlloc(len * sizeof(WCHAR));

        if (storeName)
        {
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, storeName, len);
            ret = CRYPT_SysOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}

static void WINAPI CRYPT_FileCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;

    TRACE("(%p, %08x)\n", store, dwFlags);
    if (store->dirty)
        CRYPT_WriteSerializedFile(store->file, store->memStore);
    CertCloseStore(store->memStore, dwFlags);
    CloseHandle(store->file);
    CryptMemFree(store);
}

static BOOL WINAPI CRYPT_FileWriteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT cert, DWORD dwFlags)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;

    TRACE("(%p, %p, %d)\n", hCertStore, cert, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;

    TRACE("(%p, %p, %08x)\n", hCertStore, pCertContext, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileWriteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT crl, DWORD dwFlags)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;

    TRACE("(%p, %p, %d)\n", hCertStore, crl, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileDeleteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;

    TRACE("(%p, %p, %08x)\n", hCertStore, pCrlContext, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileControl(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    PWINE_FILESTOREINFO store = (PWINE_FILESTOREINFO)hCertStore;
    BOOL ret;

    TRACE("(%p, %08x, %d, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    switch (dwCtrlType)
    {
    case CERT_STORE_CTRL_RESYNC:
        CRYPT_MemEmptyStore((PWINE_MEMSTORE)store->memStore);
        CRYPT_ReadSerializedFile(store->file, store);
        ret = TRUE;
        break;
    case CERT_STORE_CTRL_COMMIT:
        if (!(store->dwOpenFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG))
        {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            ret = FALSE;
        }
        else if (store->dirty)
            ret = CRYPT_WriteSerializedFile(store->file, store->memStore);
        else
            ret = TRUE;
        break;
    default:
        FIXME("%d: stub\n", dwCtrlType);
        ret = FALSE;
    }
    return ret;
}

static void *fileProvFuncs[] = {
    CRYPT_FileCloseStore,
    NULL, /* CERT_STORE_PROV_READ_CERT_FUNC */
    CRYPT_FileWriteCert,
    CRYPT_FileDeleteCert,
    NULL, /* CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CRL_FUNC */
    CRYPT_FileWriteCRL,
    CRYPT_FileDeleteCRL,
    NULL, /* CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_WRITE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_DELETE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC */
    CRYPT_FileControl,
};

static PWINECRYPT_CERTSTORE CRYPT_FileOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINECRYPT_CERTSTORE store = NULL;
    HANDLE file = (HANDLE)pvPara;

    TRACE("(%ld, %08x, %p)\n", hCryptProv, dwFlags, pvPara);

    if (!pvPara)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if ((dwFlags & CERT_STORE_READONLY_FLAG) &&
     (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG))
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    if (DuplicateHandle(GetCurrentProcess(), (HANDLE)pvPara,
     GetCurrentProcess(), &file, dwFlags & CERT_STORE_READONLY_FLAG ?
     GENERIC_READ : GENERIC_READ | GENERIC_WRITE, TRUE, 0))
    {
        PWINECRYPT_CERTSTORE memStore;

        memStore = CRYPT_MemOpenStore(hCryptProv, dwFlags, NULL);
        if (memStore)
        {
            if (CRYPT_ReadSerializedFile(file, memStore))
            {
                PWINE_FILESTOREINFO info = CryptMemAlloc(
                 sizeof(WINE_FILESTOREINFO));

                if (info)
                {
                    CERT_STORE_PROV_INFO provInfo = { 0 };

                    info->dwOpenFlags = dwFlags;
                    info->cryptProv = hCryptProv;
                    info->memStore = memStore;
                    info->file = file;
                    info->dirty = FALSE;
                    provInfo.cbSize = sizeof(provInfo);
                    provInfo.cStoreProvFunc = sizeof(fileProvFuncs) /
                     sizeof(fileProvFuncs[0]);
                    provInfo.rgpvStoreProvFunc = fileProvFuncs;
                    provInfo.hStoreProv = info;
                    store = CRYPT_ProvCreateStore(hCryptProv, dwFlags, memStore,
                     &provInfo);
                }
            }
        }
    }
    TRACE("returning %p\n", store);
    return store;
}

static PWINECRYPT_CERTSTORE CRYPT_FileNameOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCERTSTORE store = 0;
    LPCWSTR fileName = (LPCWSTR)pvPara;
    DWORD access, create;
    HANDLE file;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags, debugstr_w(fileName));

    if (!fileName)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return NULL;
    }
    if (!(dwFlags & (CERT_FILE_STORE_COMMIT_ENABLE_FLAG |
     CERT_STORE_READONLY_FLAG)))
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }

    access = GENERIC_READ;
    if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG)
        access |= GENERIC_WRITE;
    if (dwFlags & CERT_STORE_CREATE_NEW_FLAG)
        create = CREATE_NEW;
    else if (dwFlags & CERT_STORE_OPEN_EXISTING_FLAG)
        create = OPEN_EXISTING;
    else
        create = OPEN_ALWAYS;
    file = CreateFileW(fileName, access, FILE_SHARE_READ, NULL, create,
     FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        /* FIXME: need to check whether it's a serialized store; if not, fall
         * back to a PKCS#7 signed message, then to a single serialized cert.
         */
        store = CertOpenStore(CERT_STORE_PROV_FILE, 0, hCryptProv, dwFlags,
         file);
        CloseHandle(file);
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static PWINECRYPT_CERTSTORE CRYPT_FileNameOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    int len;
    PWINECRYPT_CERTSTORE ret = NULL;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_a((LPCSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, NULL, 0);
    if (len)
    {
        LPWSTR storeName = CryptMemAlloc(len * sizeof(WCHAR));

        if (storeName)
        {
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvPara, -1, storeName, len);
            ret = CRYPT_FileNameOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}

static PWINECRYPT_CERTSTORE CRYPT_PhysOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
        FIXME("(%ld, %08x, %p): stub\n", hCryptProv, dwFlags, pvPara);
    else
        FIXME("(%ld, %08x, %s): stub\n", hCryptProv, dwFlags,
         debugstr_w((LPCWSTR)pvPara));
    return NULL;
}

HCERTSTORE WINAPI CertOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwMsgAndCertEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void* pvPara)
{
    WINECRYPT_CERTSTORE *hcs;
    StoreOpenFunc openFunc = NULL;

    TRACE("(%s, %08x, %08lx, %08x, %p)\n", debugstr_a(lpszStoreProvider),
          dwMsgAndCertEncodingType, hCryptProv, dwFlags, pvPara);

    if (!HIWORD(lpszStoreProvider))
    {
        switch (LOWORD(lpszStoreProvider))
        {
        case (int)CERT_STORE_PROV_MEMORY:
            openFunc = CRYPT_MemOpenStore;
            break;
        case (int)CERT_STORE_PROV_FILE:
            openFunc = CRYPT_FileOpenStore;
            break;
        case (int)CERT_STORE_PROV_REG:
            openFunc = CRYPT_RegOpenStore;
            break;
        case (int)CERT_STORE_PROV_FILENAME_A:
            openFunc = CRYPT_FileNameOpenStoreA;
            break;
        case (int)CERT_STORE_PROV_FILENAME_W:
            openFunc = CRYPT_FileNameOpenStoreW;
            break;
        case (int)CERT_STORE_PROV_COLLECTION:
            openFunc = CRYPT_CollectionOpenStore;
            break;
        case (int)CERT_STORE_PROV_SYSTEM_A:
            openFunc = CRYPT_SysOpenStoreA;
            break;
        case (int)CERT_STORE_PROV_SYSTEM_W:
            openFunc = CRYPT_SysOpenStoreW;
            break;
        case (int)CERT_STORE_PROV_SYSTEM_REGISTRY_A:
            openFunc = CRYPT_SysRegOpenStoreA;
            break;
        case (int)CERT_STORE_PROV_SYSTEM_REGISTRY_W:
            openFunc = CRYPT_SysRegOpenStoreW;
            break;
        case (int)CERT_STORE_PROV_PHYSICAL_W:
            openFunc = CRYPT_PhysOpenStoreW;
            break;
        default:
            if (LOWORD(lpszStoreProvider))
                FIXME("unimplemented type %d\n", LOWORD(lpszStoreProvider));
        }
    }
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_MEMORY))
        openFunc = CRYPT_MemOpenStore;
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_FILENAME_W))
        openFunc = CRYPT_FileOpenStore;
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_SYSTEM))
        openFunc = CRYPT_SysOpenStoreW;
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_COLLECTION))
        openFunc = CRYPT_CollectionOpenStore;
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_SYSTEM_REGISTRY))
        openFunc = CRYPT_SysRegOpenStoreW;
    else
    {
        FIXME("unimplemented type %s\n", lpszStoreProvider);
        openFunc = NULL;
    }

    if (!openFunc)
        hcs = CRYPT_ProvOpenStore(lpszStoreProvider, dwMsgAndCertEncodingType,
         hCryptProv, dwFlags, pvPara);
    else
        hcs = openFunc(hCryptProv, dwFlags, pvPara);
    return (HCERTSTORE)hcs;
}

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV hProv,
 LPCSTR szSubSystemProtocol)
{
    if (!szSubSystemProtocol)
    {
        SetLastError(E_INVALIDARG);
        return 0;
    }
    return CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, hProv,
     CERT_SYSTEM_STORE_CURRENT_USER, szSubSystemProtocol);
}

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV hProv,
 LPCWSTR szSubSystemProtocol)
{
    if (!szSubSystemProtocol)
    {
        SetLastError(E_INVALIDARG);
        return 0;
    }
    return CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, hProv,
     CERT_SYSTEM_STORE_CURRENT_USER, szSubSystemProtocol);
}

BOOL WINAPI CertSaveStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType,
             DWORD dwSaveAs, DWORD dwSaveTo, void* pvSaveToPara, DWORD dwFlags)
{
    FIXME("(%p,%d,%d,%d,%p,%08x) stub!\n", hCertStore, 
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);
    return TRUE;
}

DWORD CertStore_GetAccessState(HCERTSTORE hCertStore)
{
    DWORD state = 0;

    if (hCertStore)
    {
        PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;

        if (store->type != StoreTypeMem &&
         !(store->dwOpenFlags & CERT_STORE_READONLY_FLAG))
            state |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
    }
    return state;
}

#define CertContext_CopyProperties(to, from) \
 Context_CopyProperties((to), (from), sizeof(CERT_CONTEXT))

BOOL WINAPI CertAddCertificateContextToStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition,
 PCCERT_CONTEXT *ppStoreContext)
{
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    BOOL ret = TRUE;
    PCCERT_CONTEXT toAdd = NULL, existing = NULL;

    TRACE("(%p, %p, %08x, %p)\n", hCertStore, pCertContext,
     dwAddDisposition, ppStoreContext);

    /* Weird case to pass a test */
    if (dwAddDisposition == 0)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        return FALSE;
    }
    if (dwAddDisposition != CERT_STORE_ADD_ALWAYS)
    {
        BYTE hashToAdd[20];
        DWORD size = sizeof(hashToAdd);

        ret = CertGetCertificateContextProperty(pCertContext, CERT_HASH_PROP_ID,
         hashToAdd, &size);
        if (ret)
        {
            CRYPT_HASH_BLOB blob = { sizeof(hashToAdd), hashToAdd };

            existing = CertFindCertificateInStore(hCertStore,
             pCertContext->dwCertEncodingType, 0, CERT_FIND_SHA1_HASH, &blob,
             NULL);
        }
    }

    switch (dwAddDisposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        toAdd = CertDuplicateCertificateContext(pCertContext);
        break;
    case CERT_STORE_ADD_NEW:
        if (existing)
        {
            TRACE("found matching certificate, not adding\n");
            SetLastError(CRYPT_E_EXISTS);
            ret = FALSE;
        }
        else
            toAdd = CertDuplicateCertificateContext(pCertContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING:
        toAdd = CertDuplicateCertificateContext(pCertContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
        toAdd = CertDuplicateCertificateContext(pCertContext);
        if (existing)
            CertContext_CopyProperties(toAdd, existing);
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if (existing)
            CertContext_CopyProperties(existing, pCertContext);
        break;
    default:
        FIXME("Unimplemented add disposition %d\n", dwAddDisposition);
        ret = FALSE;
    }

    if (toAdd)
    {
        if (store)
            ret = store->certs.addContext(store, (void *)toAdd,
             (void *)existing, (const void **)ppStoreContext);
        else if (ppStoreContext)
            *ppStoreContext = CertDuplicateCertificateContext(toAdd);
        CertFreeCertificateContext(toAdd);
    }
    CertFreeCertificateContext(existing);

    TRACE("returning %d\n", ret);
    return ret;
}

PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pPrev)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    PCCERT_CONTEXT ret;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (PCCERT_CONTEXT)hcs->certs.enumContext(hcs, (void *)pPrev);
    return ret;
}

BOOL WINAPI CertDeleteCertificateFromStore(PCCERT_CONTEXT pCertContext)
{
    BOOL ret;

    TRACE("(%p)\n", pCertContext);

    if (!pCertContext)
        ret = TRUE;
    else if (!pCertContext->hCertStore)
    {
        ret = TRUE;
        CertFreeCertificateContext(pCertContext);
    }
    else
    {
        PWINECRYPT_CERTSTORE hcs =
         (PWINECRYPT_CERTSTORE)pCertContext->hCertStore;

        if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
            ret = FALSE;
        else
            ret = hcs->certs.deleteContext(hcs, (void *)pCertContext);
        CertFreeCertificateContext(pCertContext);
    }
    return ret;
}

#define CrlContext_CopyProperties(to, from) \
 Context_CopyProperties((to), (from), sizeof(CRL_CONTEXT))

BOOL WINAPI CertAddCRLContextToStore(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pCrlContext, DWORD dwAddDisposition,
 PCCRL_CONTEXT* ppStoreContext)
{
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    BOOL ret = TRUE;
    PCCRL_CONTEXT toAdd = NULL, existing = NULL;

    TRACE("(%p, %p, %08x, %p)\n", hCertStore, pCrlContext,
     dwAddDisposition, ppStoreContext);

    /* Weird case to pass a test */
    if (dwAddDisposition == 0)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        return FALSE;
    }
    if (dwAddDisposition != CERT_STORE_ADD_ALWAYS)
    {
        existing = CertFindCRLInStore(hCertStore, 0, 0, CRL_FIND_EXISTING,
         pCrlContext, NULL);
    }

    switch (dwAddDisposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        toAdd = CertDuplicateCRLContext(pCrlContext);
        break;
    case CERT_STORE_ADD_NEW:
        if (existing)
        {
            TRACE("found matching CRL, not adding\n");
            SetLastError(CRYPT_E_EXISTS);
            ret = FALSE;
        }
        else
            toAdd = CertDuplicateCRLContext(pCrlContext);
        break;
    case CERT_STORE_ADD_NEWER:
        if (existing)
        {
            LONG newer = CompareFileTime(&existing->pCrlInfo->ThisUpdate,
             &pCrlContext->pCrlInfo->ThisUpdate);

            if (newer < 0)
                toAdd = CertDuplicateCRLContext(pCrlContext);
            else
            {
                TRACE("existing CRL is newer, not adding\n");
                SetLastError(CRYPT_E_EXISTS);
                ret = FALSE;
            }
        }
        else
            toAdd = CertDuplicateCRLContext(pCrlContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING:
        toAdd = CertDuplicateCRLContext(pCrlContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
        toAdd = CertDuplicateCRLContext(pCrlContext);
        if (existing)
            CrlContext_CopyProperties(toAdd, existing);
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if (existing)
            CrlContext_CopyProperties(existing, pCrlContext);
        break;
    default:
        FIXME("Unimplemented add disposition %d\n", dwAddDisposition);
        ret = FALSE;
    }

    if (toAdd)
    {
        if (store)
            ret = store->crls.addContext(store, (void *)toAdd,
             (void *)existing, (const void **)ppStoreContext);
        else if (ppStoreContext)
            *ppStoreContext = CertDuplicateCRLContext(toAdd);
        CertFreeCRLContext(toAdd);
    }
    CertFreeCRLContext(existing);

    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertDeleteCRLFromStore(PCCRL_CONTEXT pCrlContext)
{
    BOOL ret;

    TRACE("(%p)\n", pCrlContext);

    if (!pCrlContext)
        ret = TRUE;
    else if (!pCrlContext->hCertStore)
    {
        ret = TRUE;
        CertFreeCRLContext(pCrlContext);
    }
    else
    {
        PWINECRYPT_CERTSTORE hcs =
         (PWINECRYPT_CERTSTORE)pCrlContext->hCertStore;

        if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
            ret = FALSE;
        else
            ret = hcs->crls.deleteContext(hcs, (void *)pCrlContext);
        CertFreeCRLContext(pCrlContext);
    }
    return ret;
}

PCCRL_CONTEXT WINAPI CertEnumCRLsInStore(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pPrev)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    PCCRL_CONTEXT ret;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (PCCRL_CONTEXT)hcs->crls.enumContext(hcs, (void *)pPrev);
    return ret;
}

PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwCertEncodingType,
  const BYTE* pbCtlEncoded, DWORD cbCtlEncoded)
{
    FIXME("(%08x, %p, %08x): stub\n", dwCertEncodingType, pbCtlEncoded,
     cbCtlEncoded);
    return NULL;
}

BOOL WINAPI CertAddEncodedCTLToStore(HCERTSTORE hCertStore,
 DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded,
 DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext)
{
    FIXME("(%p, %08x, %p, %d, %08x, %p): stub\n", hCertStore,
     dwMsgAndCertEncodingType, pbCtlEncoded, cbCtlEncoded, dwAddDisposition,
     ppCtlContext);
    return FALSE;
}

BOOL WINAPI CertAddCTLContextToStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pCtlContext, DWORD dwAddDisposition,
 PCCTL_CONTEXT* ppStoreContext)
{
    FIXME("(%p, %p, %08x, %p): stub\n", hCertStore, pCtlContext,
     dwAddDisposition, ppStoreContext);
    return TRUE;
}

PCCTL_CONTEXT WINAPI CertDuplicateCTLContext(PCCTL_CONTEXT pCtlContext)
{
    FIXME("(%p): stub\n", pCtlContext );
    return pCtlContext;
}

BOOL WINAPI CertFreeCTLContext(PCCTL_CONTEXT pCtlContext)
{
    FIXME("(%p): stub\n", pCtlContext );
    return TRUE;
}

BOOL WINAPI CertDeleteCTLFromStore(PCCTL_CONTEXT pCtlContext)
{
    FIXME("(%p): stub\n", pCtlContext);
    return TRUE;
}

PCCTL_CONTEXT WINAPI CertEnumCTLsInStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pPrev)
{
    FIXME("(%p, %p): stub\n", hCertStore, pPrev);
    return NULL;
}

HCERTSTORE WINAPI CertDuplicateStore(HCERTSTORE hCertStore)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;

    TRACE("(%p)\n", hCertStore);

    if (hcs && hcs->dwMagic == WINE_CRYPTCERTSTORE_MAGIC)
        InterlockedIncrement(&hcs->ref);
    return hCertStore;
}

BOOL WINAPI CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *) hCertStore;

    TRACE("(%p, %08x)\n", hCertStore, dwFlags);

    if( ! hCertStore )
        return TRUE;

    if ( hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC )
        return FALSE;

    if (InterlockedDecrement(&hcs->ref) == 0)
    {
        TRACE("%p's ref count is 0, freeing\n", hcs);
        hcs->dwMagic = 0;
        if (!(hcs->dwOpenFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
            CryptReleaseContext(hcs->cryptProv, 0);
        hcs->closeStore(hcs, dwFlags);
    }
    else
        TRACE("%p's ref count is %d\n", hcs, hcs->ref);
    return TRUE;
}

BOOL WINAPI CertControlStore(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    BOOL ret;

    TRACE("(%p, %08x, %d, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    if (!hcs)
        ret = FALSE;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = FALSE;
    else
    {
        if (hcs->control)
            ret = hcs->control(hCertStore, dwFlags, dwCtrlType, pvCtrlPara);
        else
            ret = TRUE;
    }
    return ret;
}

DWORD WINAPI CertEnumCTLContextProperties(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId)
{
    FIXME("(%p, %d): stub\n", pCTLContext, dwPropId);
    return 0;
}

BOOL WINAPI CertGetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    FIXME("(%p, %d, %p, %p): stub\n", pCTLContext, dwPropId, pvData, pcbData);
    return FALSE;
}

BOOL WINAPI CertSetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    FIXME("(%p, %d, %08x, %p): stub\n", pCTLContext, dwPropId, dwFlags,
     pvData);
    return FALSE;
}

BOOL WINAPI CertAddStoreToCollection(HCERTSTORE hCollectionStore,
 HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
    PWINE_COLLECTIONSTORE collection = (PWINE_COLLECTIONSTORE)hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = (WINECRYPT_CERTSTORE *)hSiblingStore;
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
    PWINE_COLLECTIONSTORE collection = (PWINE_COLLECTIONSTORE)hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = (WINECRYPT_CERTSTORE *)hSiblingStore;
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
