/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2004,2005 Juan Lang
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME:
 * - As you can see in the stubs below, support for CRLs and CTLs is missing.
 *   Mostly this should be copy-paste work, and some code (e.g. extended
 *   properties) could be shared between them.
 * - Opening a cert store provider should be morphed to support loading
 *   external DLLs.
 * - The concept of physical stores and locations isn't implemented.  (This
 *   doesn't mean registry stores et al aren't implemented.  See the PSDK for
 *   registering and enumerating physical stores and locations.)
 * - Many flags, options and whatnot are unimplemented.
 */
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
/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

/* Some typedefs that make it easier to abstract which type of context we're
 * working with.
 */
typedef const void *(WINAPI *CreateContextFunc)(DWORD dwCertEncodingType,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded);
typedef BOOL (WINAPI *AddContextToStoreFunc)(HCERTSTORE hCertStore,
 const void *context, DWORD dwAddDisposition, const void **ppStoreContext);
typedef BOOL (WINAPI *AddEncodedContextToStoreFunc)(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwAddDisposition, const void **ppContext);
typedef const void *(WINAPI *EnumContextsInStoreFunc)(HCERTSTORE hCertStore,
 const void *pPrevContext);
typedef BOOL (WINAPI *GetContextPropertyFunc)(const void *context,
 DWORD dwPropID, void *pvData, DWORD *pcbData);
typedef BOOL (WINAPI *SetContextPropertyFunc)(const void *context,
 DWORD dwPropID, DWORD dwFlags, const void *pvData);
typedef BOOL (WINAPI *SerializeElementFunc)(const void *context, DWORD dwFlags,
 BYTE *pbElement, DWORD *pcbElement);
typedef BOOL (WINAPI *FreeContextFunc)(const void *context);
typedef BOOL (WINAPI *DeleteContextFunc)(const void *context);

/* An abstract context (certificate, CRL, or CTL) interface */
typedef struct _WINE_CONTEXT_INTERFACE
{
    CreateContextFunc            create;
    AddContextToStoreFunc        addContextToStore;
    AddEncodedContextToStoreFunc addEncodedToStore;
    EnumContextsInStoreFunc      enumContextsInStore;
    GetContextPropertyFunc       getProp;
    SetContextPropertyFunc       setProp;
    SerializeElementFunc         serialize;
    FreeContextFunc              free;
    DeleteContextFunc            deleteFromStore;
} WINE_CONTEXT_INTERFACE, *PWINE_CONTEXT_INTERFACE;

static const WINE_CONTEXT_INTERFACE gCertInterface = {
    (CreateContextFunc)CertCreateCertificateContext,
    (AddContextToStoreFunc)CertAddCertificateContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCertificateToStore,
    (EnumContextsInStoreFunc)CertEnumCertificatesInStore,
    (GetContextPropertyFunc)CertGetCertificateContextProperty,
    (SetContextPropertyFunc)CertSetCertificateContextProperty,
    (SerializeElementFunc)CertSerializeCertificateStoreElement,
    (FreeContextFunc)CertFreeCertificateContext,
    (DeleteContextFunc)CertDeleteCertificateFromStore,
};

static const WINE_CONTEXT_INTERFACE gCRLInterface = {
    (CreateContextFunc)CertCreateCRLContext,
    (AddContextToStoreFunc)CertAddCRLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCRLToStore,
    (EnumContextsInStoreFunc)CertEnumCRLsInStore,
    (GetContextPropertyFunc)CertGetCRLContextProperty,
    (SetContextPropertyFunc)CertSetCRLContextProperty,
    (SerializeElementFunc)CertSerializeCRLStoreElement,
    (FreeContextFunc)CertFreeCRLContext,
    (DeleteContextFunc)CertDeleteCRLFromStore,
};

static const WINE_CONTEXT_INTERFACE gCTLInterface = {
    (CreateContextFunc)CertCreateCTLContext,
    (AddContextToStoreFunc)CertAddCTLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCTLToStore,
    (EnumContextsInStoreFunc)CertEnumCTLsInStore,
    (GetContextPropertyFunc)CertGetCTLContextProperty,
    (SetContextPropertyFunc)CertSetCTLContextProperty,
    (SerializeElementFunc)CertSerializeCTLStoreElement,
    (FreeContextFunc)CertFreeCTLContext,
    (DeleteContextFunc)CertDeleteCTLFromStore,
};

struct WINE_CRYPTCERTSTORE;

typedef struct WINE_CRYPTCERTSTORE * (*StoreOpenFunc)(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);

struct _WINE_CERT_CONTEXT_REF;

/* Called to enumerate the next certificate in a store.  The returned pointer
 * must be newly allocated (via CryptMemAlloc):  CertFreeCertificateContext
 * frees it.
 */
typedef struct _WINE_CERT_CONTEXT_REF * (*EnumCertFunc)
 (struct WINE_CRYPTCERTSTORE *store, struct _WINE_CERT_CONTEXT_REF *pPrev);

struct _WINE_CERT_CONTEXT;

/* Called to create a new reference to an existing cert context.  Should call
 * CRYPT_InitCertRef to make sure the reference count is properly updated.
 * If the store does not provide any additional allocated data (that is, does
 * not need to implement a FreeCertFunc), it may use CRYPT_CreateCertRef for
 * this.
 */
typedef struct _WINE_CERT_CONTEXT_REF * (*CreateRefFunc)
 (struct _WINE_CERT_CONTEXT *context, HCERTSTORE store);

/* Optional, called when a cert context reference is being freed.  Don't free
 * the ref pointer itself, CertFreeCertificateContext does that.
 */
typedef void (*FreeCertFunc)(struct _WINE_CERT_CONTEXT_REF *ref);

typedef enum _CertStoreType {
    StoreTypeMem,
    StoreTypeCollection,
    StoreTypeReg,
    StoreTypeDummy,
} CertStoreType;

/* A cert store is polymorphic through the use of function pointers.  A type
 * is still needed to distinguish collection stores from other types.
 * On the function pointers:
 * - closeStore is called when the store's ref count becomes 0
 * - addCert is called with a PWINE_CERT_CONTEXT as the second parameter
 * - control is optional, but should be implemented by any store that supports
 *   persistence
 */
typedef struct WINE_CRYPTCERTSTORE
{
    DWORD                           dwMagic;
    LONG                            ref;
    DWORD                           dwOpenFlags;
    HCRYPTPROV                      cryptProv;
    CertStoreType                   type;
    PFN_CERT_STORE_PROV_CLOSE       closeStore;
    PFN_CERT_STORE_PROV_WRITE_CERT  addCert;
    CreateRefFunc                   createCertRef;
    EnumCertFunc                    enumCert;
    PFN_CERT_STORE_PROV_DELETE_CERT deleteCert;
    FreeCertFunc                    freeCert;   /* optional */
    PFN_CERT_STORE_PROV_CONTROL     control;    /* optional */
} WINECRYPT_CERTSTORE, *PWINECRYPT_CERTSTORE;

/* A certificate context has pointers to data that are owned by this module,
 * so rather than duplicate the data every time a certificate context is
 * copied, I keep a reference count to the data.  Thus I have two data
 * structures, the "true" certificate context (that has the reference count)
 * and a reference certificate context, that has a pointer to the true context.
 * Each one can be cast to a PCERT_CONTEXT, though you'll usually be dealing
 * with the reference version.
 */
typedef struct _WINE_CERT_CONTEXT
{
    CERT_CONTEXT     cert;
    LONG             ref;
    CRITICAL_SECTION cs;
    struct list      extendedProperties;
} WINE_CERT_CONTEXT, *PWINE_CERT_CONTEXT;

typedef struct _WINE_CERT_CONTEXT_REF
{
    CERT_CONTEXT cert;
    WINE_CERT_CONTEXT *context;
} WINE_CERT_CONTEXT_REF, *PWINE_CERT_CONTEXT_REF;

/* An extended certificate property in serialized form is prefixed by this
 * header.
 */
typedef struct _WINE_CERT_PROP_HEADER
{
    DWORD propID;
    DWORD unknown; /* always 1 */
    DWORD cb;
} WINE_CERT_PROP_HEADER, *PWINE_CERT_PROP_HEADER;

/* Stores an extended property in a cert. */
typedef struct _WINE_CERT_PROPERTY
{
    WINE_CERT_PROP_HEADER hdr;
    LPBYTE                pbData;
    struct list           entry;
} WINE_CERT_PROPERTY, *PWINE_CERT_PROPERTY;

/* A mem store has a list of these.  They're also returned by the mem store
 * during enumeration.
 */
typedef struct _WINE_CERT_LIST_ENTRY
{
    WINE_CERT_CONTEXT_REF cert;
    struct list entry;
} WINE_CERT_LIST_ENTRY, *PWINE_CERT_LIST_ENTRY;

typedef struct _WINE_MEMSTORE
{
    WINECRYPT_CERTSTORE hdr;
    CRITICAL_SECTION    cs;
    struct list         certs;
} WINE_MEMSTORE, *PWINE_MEMSTORE;

typedef struct _WINE_HASH_TO_DELETE
{
    BYTE        hash[20];
    struct list entry;
} WINE_HASH_TO_DELETE, *PWINE_HASH_TO_DELETE;

/* Returned by a reg store during enumeration. */
typedef struct _WINE_REG_CERT_CONTEXT
{
    WINE_CERT_CONTEXT_REF  cert;
    PWINE_CERT_CONTEXT_REF childContext;
} WINE_REG_CERT_CONTEXT, *PWINE_REG_CERT_CONTEXT;

typedef struct _WINE_REGSTORE
{
    WINECRYPT_CERTSTORE  hdr;
    PWINECRYPT_CERTSTORE memStore;
    HKEY                 key;
    BOOL                 dirty;
    CRITICAL_SECTION     cs;
    struct list          certsToDelete;
} WINE_REGSTORE, *PWINE_REGSTORE;

typedef struct _WINE_STORE_LIST_ENTRY
{
    PWINECRYPT_CERTSTORE store;
    DWORD                dwUpdateFlags;
    DWORD                dwPriority;
    struct list          entry;
} WINE_STORE_LIST_ENTRY, *PWINE_STORE_LIST_ENTRY;

/* Returned by a collection store during enumeration.
 * Note: relies on the list entry being valid after use, which a number of
 * conditions might make untrue (reentrancy, closing a collection store before
 * continuing an enumeration on it, ...).  The tests seem to indicate this
 * sort of unsafety is okay, since Windows isn't well-behaved in these
 * scenarios either.
 */
typedef struct _WINE_COLLECTION_CERT_CONTEXT
{
    WINE_CERT_CONTEXT_REF  cert;
    PWINE_STORE_LIST_ENTRY entry;
    PWINE_CERT_CONTEXT_REF childContext;
} WINE_COLLECTION_CERT_CONTEXT, *PWINE_COLLECTION_CERT_CONTEXT;

typedef struct _WINE_COLLECTIONSTORE
{
    WINECRYPT_CERTSTORE hdr;
    CRITICAL_SECTION    cs;
    struct list         stores;
} WINE_COLLECTIONSTORE, *PWINE_COLLECTIONSTORE;

/* Like CertGetCertificateContextProperty, but operates directly on the
 * WINE_CERT_CONTEXT.  Doesn't support special-case properties, since they
 * are handled by CertGetCertificateContextProperty, and are particular to the
 * store in which the property exists (which is separate from the context.)
 */
static BOOL WINAPI CRYPT_GetCertificateContextProperty(
 PWINE_CERT_CONTEXT context, DWORD dwPropId, void *pvData, DWORD *pcbData);

/* Like CertSetCertificateContextProperty, but operates directly on the
 * WINE_CERT_CONTEXT.  Doesn't handle special cases, since they're handled by
 * CertSetCertificateContextProperty anyway.
 */
static BOOL WINAPI CRYPT_SetCertificateContextProperty(
 PWINE_CERT_CONTEXT context, DWORD dwPropId, DWORD dwFlags, const void *pvData);

/* Helper function for store reading functions and
 * CertAddSerializedElementToStore.  Returns a context of the appropriate type
 * if it can, or NULL otherwise.  Doesn't validate any of the properties in
 * the serialized context (for example, bad hashes are retained.)
 * *pdwContentType is set to the type of the returned context.
 */
static const void * WINAPI CRYPT_ReadSerializedElement(const BYTE *pbElement,
 DWORD cbElement, DWORD dwContextTypeFlags, DWORD *pdwContentType);

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

/* Initializes the reference ref to point to pCertContext, which is assumed to
 * be a PWINE_CERT_CONTEXT, and increments pCertContext's reference count.
 * Also sets the hCertStore member of the reference to store.
 */
static void CRYPT_InitCertRef(PWINE_CERT_CONTEXT_REF ref,
 PWINE_CERT_CONTEXT context, HCERTSTORE store)
{
    TRACE("(%p, %p)\n", ref, context);
    memcpy(&ref->cert, context, sizeof(ref->cert));
    ref->context = context;
    InterlockedIncrement(&context->ref);
    TRACE("%p's ref count is %ld\n", context, context->ref);
    ref->cert.hCertStore = store;
}

static PWINE_CERT_CONTEXT_REF CRYPT_CreateCertRef(PWINE_CERT_CONTEXT context,
 HCERTSTORE store)
{
    PWINE_CERT_CONTEXT_REF pCertRef = CryptMemAlloc(
     sizeof(WINE_CERT_CONTEXT_REF));

    TRACE("(%p, %p)\n", context, store);
    if (pCertRef)
        CRYPT_InitCertRef(pCertRef, context, store);
    return pCertRef;
}

static BOOL WINAPI CRYPT_MemAddCert(HCERTSTORE store, PCCERT_CONTEXT pCert,
 DWORD dwAddDisposition)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    BOOL add = FALSE, ret;

    TRACE("(%p, %p, %ld)\n", store, pCert, dwAddDisposition);

    switch (dwAddDisposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        add = TRUE;
        break;
    case CERT_STORE_ADD_NEW:
    {
        BYTE hashToAdd[20], hash[20];
        DWORD size = sizeof(hashToAdd);

        ret = CRYPT_GetCertificateContextProperty((PWINE_CERT_CONTEXT)pCert,
         CERT_HASH_PROP_ID, hashToAdd, &size);
        if (ret)
        {
            PWINE_CERT_LIST_ENTRY cursor;

            /* Add if no cert with the same hash is found. */
            add = TRUE;
            EnterCriticalSection(&ms->cs);
            LIST_FOR_EACH_ENTRY(cursor, &ms->certs, WINE_CERT_LIST_ENTRY, entry)
            {
                size = sizeof(hash);
                ret = CertGetCertificateContextProperty(&cursor->cert.cert,
                 CERT_HASH_PROP_ID, hash, &size);
                if (ret && !memcmp(hashToAdd, hash, size))
                {
                    TRACE("found matching certificate, not adding\n");
                    SetLastError(CRYPT_E_EXISTS);
                    add = FALSE;
                    break;
                }
            }
            LeaveCriticalSection(&ms->cs);
        }
        break;
    }
    case CERT_STORE_ADD_REPLACE_EXISTING:
    {
        BYTE hashToAdd[20], hash[20];
        DWORD size = sizeof(hashToAdd);

        add = TRUE;
        ret = CRYPT_GetCertificateContextProperty((PWINE_CERT_CONTEXT)pCert,
         CERT_HASH_PROP_ID, hashToAdd, &size);
        if (ret)
        {
            PWINE_CERT_LIST_ENTRY cursor, next;

            /* Look for existing cert to delete */
            EnterCriticalSection(&ms->cs);
            LIST_FOR_EACH_ENTRY_SAFE(cursor, next, &ms->certs,
             WINE_CERT_LIST_ENTRY, entry)
            {
                size = sizeof(hash);
                ret = CertGetCertificateContextProperty(&cursor->cert.cert,
                 CERT_HASH_PROP_ID, hash, &size);
                if (ret && !memcmp(hashToAdd, hash, size))
                {
                    TRACE("found matching certificate, replacing\n");
                    list_remove(&cursor->entry);
                    CertFreeCertificateContext((PCCERT_CONTEXT)cursor);
                    break;
                }
            }
            LeaveCriticalSection(&ms->cs);
        }
        break;
    }
    default:
        FIXME("Unimplemented add disposition %ld\n", dwAddDisposition);
        add = FALSE;
    }
    if (add)
    {
        PWINE_CERT_LIST_ENTRY entry = CryptMemAlloc(
         sizeof(WINE_CERT_LIST_ENTRY));

        if (entry)
        {
            TRACE("adding %p\n", entry);
            CRYPT_InitCertRef(&entry->cert, (PWINE_CERT_CONTEXT)pCert, store);
            list_init(&entry->entry);
            EnterCriticalSection(&ms->cs);
            list_add_tail(&ms->certs, &entry->entry);
            LeaveCriticalSection(&ms->cs);
            ret = TRUE;
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

static PWINE_CERT_CONTEXT_REF CRYPT_MemEnumCert(PWINECRYPT_CERTSTORE store,
 PWINE_CERT_CONTEXT_REF pPrev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    PWINE_CERT_LIST_ENTRY prevEntry = (PWINE_CERT_LIST_ENTRY)pPrev, ret;
    struct list *listNext;

    TRACE("(%p, %p)\n", store, pPrev);
    EnterCriticalSection(&ms->cs);
    if (prevEntry)
    {
        listNext = list_next(&ms->certs, &prevEntry->entry);
        CertFreeCertificateContext((PCCERT_CONTEXT)pPrev);
    }
    else
        listNext = list_next(&ms->certs, &ms->certs);
    if (listNext)
    {
        ret = CryptMemAlloc(sizeof(WINE_CERT_LIST_ENTRY));
        if (ret)
        {
            memcpy(ret, LIST_ENTRY(listNext, WINE_CERT_LIST_ENTRY, entry),
             sizeof(WINE_CERT_LIST_ENTRY));
            InterlockedIncrement(&ret->cert.context->ref);
        }
    }
    else
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        ret = NULL;
    }
    LeaveCriticalSection(&ms->cs);

    TRACE("returning %p\n", ret);
    return (PWINE_CERT_CONTEXT_REF)ret;
}

static void CRYPT_UnrefCertificateContext(PWINE_CERT_CONTEXT_REF ref);

static BOOL WINAPI CRYPT_MemDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    WINE_MEMSTORE *store = (WINE_MEMSTORE *)hCertStore;
    WINE_CERT_CONTEXT_REF *ref = (WINE_CERT_CONTEXT_REF *)pCertContext;
    PWINE_CERT_LIST_ENTRY cert, next;
    BOOL ret = TRUE;

    /* Find the entry associated with the passed-in context, since the
     * passed-in context may not be a list entry itself (e.g. if it came from
     * CertDuplicateCertificateContext.)  Pointing to the same context is
     * a sufficient test of equality.
     */
    EnterCriticalSection(&store->cs);
    LIST_FOR_EACH_ENTRY_SAFE(cert, next, &store->certs, WINE_CERT_LIST_ENTRY,
     entry)
    {
        if (cert->cert.context == ref->context)
        {
            TRACE("removing %p\n", cert);
            /* FIXME: this isn't entirely thread-safe, the entry itself isn't
             * protected.
             */
            list_remove(&cert->entry);
            /* FIXME: generally I should do the following, otherwise there is
             * a memory leak.  But doing so when called by
             * CertDeleteCertificateFromStore results in a double free, so
             * leaving commented for now.
            ret = CertFreeCertificateContext((PCCERT_CONTEXT)cert);
             */
            cert->entry.prev = cert->entry.next = &store->certs;
            break;
        }
    }
    LeaveCriticalSection(&store->cs);
    return ret;
}

static void WINAPI CRYPT_MemCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINE_MEMSTORE *store = (WINE_MEMSTORE *)hCertStore;
    PWINE_CERT_LIST_ENTRY cert, next;

    TRACE("(%p, %08lx)\n", store, dwFlags);
    if (dwFlags)
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    /* Note that CertFreeCertificateContext calls HeapFree on the passed-in
     * pointer if its ref-count reaches zero.  That's okay here because there
     * aren't any allocated data outside of the WINE_CERT_CONTEXT_REF portion
     * of the CertListEntry.
     */
    LIST_FOR_EACH_ENTRY_SAFE(cert, next, &store->certs, WINE_CERT_LIST_ENTRY,
     entry)
    {
        TRACE("removing %p\n", cert);
        list_remove(&cert->entry);
        CertFreeCertificateContext((PCCERT_CONTEXT)cert);
    }
    DeleteCriticalSection(&store->cs);
    CryptMemFree(store);
}

static WINECRYPT_CERTSTORE *CRYPT_MemOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINE_MEMSTORE store;

    TRACE("(%ld, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

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
            store->hdr.closeStore    = CRYPT_MemCloseStore;
            store->hdr.addCert       = CRYPT_MemAddCert;
            store->hdr.createCertRef = CRYPT_CreateCertRef;
            store->hdr.enumCert      = CRYPT_MemEnumCert;
            store->hdr.deleteCert    = CRYPT_MemDeleteCert;
            store->hdr.freeCert      = NULL;
            InitializeCriticalSection(&store->cs);
            list_init(&store->certs);
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static BOOL WINAPI CRYPT_CollectionAddCert(HCERTSTORE store,
 PCCERT_CONTEXT pCert, DWORD dwAddDisposition)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    PWINE_STORE_LIST_ENTRY entry, next;
    BOOL ret;

    TRACE("(%p, %p, %ld)\n", store, pCert, dwAddDisposition);

    ret = FALSE;
    EnterCriticalSection(&cs->cs);
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &cs->stores, WINE_STORE_LIST_ENTRY,
     entry)
    {
        if (entry->dwUpdateFlags & CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG)
        {
            ret = entry->store->addCert(entry->store, pCert, dwAddDisposition);
            break;
        }
    }
    LeaveCriticalSection(&cs->cs);
    SetLastError(ret ? ERROR_SUCCESS : HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
    return ret;
}

static PWINE_CERT_CONTEXT_REF CRYPT_CollectionCreateCertRef(
 PWINE_CERT_CONTEXT context, HCERTSTORE store)
{
    PWINE_COLLECTION_CERT_CONTEXT ret = CryptMemAlloc(
     sizeof(WINE_COLLECTION_CERT_CONTEXT));

    if (ret)
    {
        /* Initialize to empty for now, just make sure the size is right */
        CRYPT_InitCertRef((PWINE_CERT_CONTEXT_REF)ret, context, store);
        ret->entry = NULL;
        ret->childContext = NULL;
    }
    return (PWINE_CERT_CONTEXT_REF)ret;
}

static void WINAPI CRYPT_CollectionCloseStore(HCERTSTORE store, DWORD dwFlags)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    PWINE_STORE_LIST_ENTRY entry, next;

    TRACE("(%p, %08lx)\n", store, dwFlags);

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &cs->stores, WINE_STORE_LIST_ENTRY,
     entry)
    {
        TRACE("closing %p\n", entry);
        CertCloseStore((HCERTSTORE)entry->store, dwFlags);
        CryptMemFree(entry);
    }
    DeleteCriticalSection(&cs->cs);
    CryptMemFree(cs);
}

/* Advances a collection enumeration by one cert, if possible, where advancing
 * means:
 * - calling the current store's enumeration function once, and returning
 *   the enumerated cert if one is returned
 * - moving to the next store if the current store has no more items, and
 *   recursively calling itself to get the next item.
 * Returns NULL if the collection contains no more items or on error.
 * Assumes the collection store's lock is held.
 */
static PWINE_COLLECTION_CERT_CONTEXT CRYPT_CollectionAdvanceEnum(
 PWINE_COLLECTIONSTORE store, PWINE_STORE_LIST_ENTRY storeEntry,
 PWINE_COLLECTION_CERT_CONTEXT pPrev)
{
    PWINE_COLLECTION_CERT_CONTEXT ret;
    PWINE_CERT_CONTEXT_REF child;
    struct list *storeNext = list_next(&store->stores, &storeEntry->entry);

    TRACE("(%p, %p, %p)\n", store, storeEntry, pPrev);

    child = storeEntry->store->enumCert((HCERTSTORE)storeEntry->store,
     pPrev ? pPrev->childContext : NULL);
    if (pPrev)
    {
        pPrev->childContext = NULL;
        CertFreeCertificateContext((PCCERT_CONTEXT)pPrev);
        pPrev = NULL;
    }
    if (child)
    {
        ret = (PWINE_COLLECTION_CERT_CONTEXT)CRYPT_CollectionCreateCertRef(
         child->context, store);
        if (ret)
        {
            ret->entry = storeEntry;
            ret->childContext = child;
        }
        else
            CertFreeCertificateContext((PCCERT_CONTEXT)child);
    }
    else
    {
        if (storeNext)
        {
            storeEntry = LIST_ENTRY(storeNext, WINE_STORE_LIST_ENTRY, entry);
            ret = CRYPT_CollectionAdvanceEnum(store, storeEntry, NULL);
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

static PWINE_CERT_CONTEXT_REF CRYPT_CollectionEnumCert(
 PWINECRYPT_CERTSTORE store, PWINE_CERT_CONTEXT_REF pPrev)
{
    PWINE_COLLECTIONSTORE cs = (PWINE_COLLECTIONSTORE)store;
    PWINE_COLLECTION_CERT_CONTEXT prevEntry =
     (PWINE_COLLECTION_CERT_CONTEXT)pPrev, ret;

    TRACE("(%p, %p)\n", store, pPrev);

    if (prevEntry)
    {
        EnterCriticalSection(&cs->cs);
        ret = CRYPT_CollectionAdvanceEnum(cs, prevEntry->entry, prevEntry);
        LeaveCriticalSection(&cs->cs);
    }
    else
    {
        EnterCriticalSection(&cs->cs);
        if (!list_empty(&cs->stores))
        {
            PWINE_STORE_LIST_ENTRY storeEntry;

            storeEntry = LIST_ENTRY(cs->stores.next, WINE_STORE_LIST_ENTRY,
             entry);
            ret = CRYPT_CollectionAdvanceEnum(cs, storeEntry, prevEntry);
        }
        else
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = NULL;
        }
        LeaveCriticalSection(&cs->cs);
    }
    TRACE("returning %p\n", ret);
    return (PWINE_CERT_CONTEXT_REF)ret;
}

static BOOL WINAPI CRYPT_CollectionDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    PWINE_COLLECTION_CERT_CONTEXT context =
     (PWINE_COLLECTION_CERT_CONTEXT)pCertContext;
    BOOL ret;

    TRACE("(%p, %p, %08lx)\n", hCertStore, pCertContext, dwFlags);

    ret = CertDeleteCertificateFromStore((PCCERT_CONTEXT)context->childContext);
    if (ret)
        context->childContext = NULL;
    return ret;
}

static void CRYPT_CollectionFreeCert(PWINE_CERT_CONTEXT_REF ref)
{
    PWINE_COLLECTION_CERT_CONTEXT context = (PWINE_COLLECTION_CERT_CONTEXT)ref;

    TRACE("(%p)\n", ref);

    if (context->childContext)
        CertFreeCertificateContext((PCCERT_CONTEXT)context->childContext);
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
            store->hdr.closeStore    = CRYPT_CollectionCloseStore;
            store->hdr.addCert       = CRYPT_CollectionAddCert;
            store->hdr.createCertRef = CRYPT_CollectionCreateCertRef;
            store->hdr.enumCert      = CRYPT_CollectionEnumCert;
            store->hdr.deleteCert    = CRYPT_CollectionDeleteCert;
            store->hdr.freeCert      = CRYPT_CollectionFreeCert;
            InitializeCriticalSection(&store->cs);
            list_init(&store->stores);
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static void CRYPT_HashToStr(LPBYTE hash, LPWSTR asciiHash)
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

static void CRYPT_RegReadSerializedFromReg(PWINE_REGSTORE store, HKEY key,
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
                                         store, context,
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

static void CRYPT_RegReadFromReg(PWINE_REGSTORE store)
{
    static const WCHAR *subKeys[] = { CertsW, CRLsW, CTLsW };
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
static BOOL CRYPT_WriteSerializedToReg(HKEY key, LPBYTE hash, LPBYTE buf,
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

static BOOL CRYPT_RegWriteToReg(PWINE_REGSTORE store)
{
    static const WCHAR *subKeys[] = { CertsW, CRLsW, CTLsW };
    static const WINE_CONTEXT_INTERFACE *interfaces[] = { &gCertInterface,
     &gCRLInterface, &gCTLInterface };
    struct list *listToDelete[] = { &store->certsToDelete, NULL, NULL };
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
static BOOL CRYPT_RegFlushStore(PWINE_REGSTORE store, BOOL force)
{
    BOOL ret;

    if (store->dirty || force)
        ret = CRYPT_RegWriteToReg(store);
    else
        ret = TRUE;
    return ret;
}

static void WINAPI CRYPT_RegCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    PWINE_REGSTORE store = (PWINE_REGSTORE)hCertStore;

    TRACE("(%p, %08lx)\n", store, dwFlags);
    if (dwFlags)
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    CRYPT_RegFlushStore(store, FALSE);
    /* certsToDelete should already be cleared by this point */
    store->memStore->closeStore(store->memStore, 0);
    RegCloseKey(store->key);
    DeleteCriticalSection(&store->cs);
    CryptMemFree(store);
}

static BOOL WINAPI CRYPT_RegAddCert(HCERTSTORE hCertStore, PCCERT_CONTEXT cert,
 DWORD dwAddDisposition)
{
    PWINE_REGSTORE store = (PWINE_REGSTORE)hCertStore;
    BOOL ret;

    TRACE("(%p, %p, %ld)\n", hCertStore, cert, dwAddDisposition);

    if (store->hdr.dwOpenFlags & CERT_STORE_READONLY_FLAG)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        ret = FALSE;
    }
    else
    {
        ret = store->memStore->addCert(store->memStore, cert, dwAddDisposition);
        if (ret)
            store->dirty = TRUE;
    }
    return ret;
}

static PWINE_CERT_CONTEXT_REF CRYPT_RegCreateCertRef(
 PWINE_CERT_CONTEXT context, HCERTSTORE store)
{
    PWINE_REG_CERT_CONTEXT ret = CryptMemAlloc(sizeof(WINE_REG_CERT_CONTEXT));

    if (ret)
    {
        CRYPT_InitCertRef((PWINE_CERT_CONTEXT_REF)ret, context, store);
        ret->childContext = NULL;
    }
    return (PWINE_CERT_CONTEXT_REF)ret;
}

static PWINE_CERT_CONTEXT_REF CRYPT_RegEnumCert(PWINECRYPT_CERTSTORE store,
 PWINE_CERT_CONTEXT_REF pPrev)
{
    PWINE_REGSTORE rs = (PWINE_REGSTORE)store;
    PWINE_CERT_CONTEXT_REF child;
    PWINE_REG_CERT_CONTEXT prev = (PWINE_REG_CERT_CONTEXT)pPrev, ret = NULL;

    TRACE("(%p, %p)\n", store, pPrev);

    child = rs->memStore->enumCert(rs->memStore, prev ? prev->childContext
     : NULL);
    if (prev)
    {
        prev->childContext = NULL;
        CertFreeCertificateContext((PCCERT_CONTEXT)prev);
        prev = NULL;
    }
    if (child)
    {
        ret = (PWINE_REG_CERT_CONTEXT)CRYPT_RegCreateCertRef(child->context,
         store);
        if (ret)
            ret->childContext = child;
        else
            CertFreeCertificateContext((PCCERT_CONTEXT)child);
    }
    return (PWINE_CERT_CONTEXT_REF)ret;
}

static BOOL WINAPI CRYPT_RegDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    PWINE_REGSTORE store = (PWINE_REGSTORE)hCertStore;
    BOOL ret;

    TRACE("(%p, %p, %08lx)\n", store, pCertContext, dwFlags);

    if (store->hdr.dwOpenFlags & CERT_STORE_READONLY_FLAG)
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

            ret = CertGetCertificateContextProperty(pCertContext,
             CERT_HASH_PROP_ID, toDelete->hash, &size);
            if (ret)
            {
                list_init(&toDelete->entry);
                EnterCriticalSection(&store->cs);
                list_add_tail(&store->certsToDelete, &toDelete->entry);
                LeaveCriticalSection(&store->cs);
                ret = store->memStore->deleteCert(store->memStore, pCertContext,
                 dwFlags);
            }
            else
                CryptMemFree(toDelete);
        }
        else
            ret = FALSE;
        if (ret)
            store->dirty = TRUE;
    }
    return ret;
}

static void CRYPT_RegFreeCert(PWINE_CERT_CONTEXT_REF ref)
{
    PWINE_REG_CERT_CONTEXT context = (PWINE_REG_CERT_CONTEXT)ref;

    TRACE("(%p)\n", ref);

    if (context->childContext)
        CertFreeCertificateContext((PCCERT_CONTEXT)context->childContext);
}

static BOOL WINAPI CRYPT_RegControl(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    PWINE_REGSTORE store = (PWINE_REGSTORE)hCertStore;
    BOOL ret;

    switch (dwCtrlType)
    {
    case CERT_STORE_CTRL_RESYNC:
        CRYPT_RegFlushStore(store, FALSE);
        store->memStore->closeStore(store->memStore, 0);
        store->memStore = CRYPT_MemOpenStore(store->hdr.cryptProv,
         store->hdr.dwOpenFlags, NULL);
        if (store->memStore)
        {
            CRYPT_RegReadFromReg(store);
            ret = TRUE;
        }
        else
            ret = FALSE;
        break;
    case CERT_STORE_CTRL_COMMIT:
        ret = CRYPT_RegFlushStore(store,
         dwFlags & CERT_STORE_CTRL_COMMIT_FORCE_FLAG);
        break;
    default:
        FIXME("%ld: stub\n", dwCtrlType);
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

static WINECRYPT_CERTSTORE *CRYPT_RegOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINE_REGSTORE store = NULL;

    TRACE("(%ld, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

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
                store = CryptMemAlloc(sizeof(WINE_REGSTORE));
                if (store)
                {
                    memset(store, 0, sizeof(WINE_REGSTORE));
                    CRYPT_InitStore(&store->hdr, hCryptProv, dwFlags,
                     StoreTypeReg);
                    store->hdr.closeStore    = CRYPT_RegCloseStore;
                    store->hdr.addCert       = CRYPT_RegAddCert;
                    store->hdr.createCertRef = CRYPT_RegCreateCertRef;
                    store->hdr.enumCert      = CRYPT_RegEnumCert;
                    store->hdr.deleteCert    = CRYPT_RegDeleteCert;
                    store->hdr.freeCert      = CRYPT_RegFreeCert;
                    store->hdr.control       = CRYPT_RegControl;
                    store->memStore          = memStore;
                    store->key               = key;
                    InitializeCriticalSection(&store->cs);
                    list_init(&store->certsToDelete);
                    CRYPT_RegReadFromReg(store);
                    store->dirty = FALSE;
                }
            }
        }
    }
    TRACE("returning %p\n", store);
    return (WINECRYPT_CERTSTORE *)store;
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

    TRACE("(%ld, %08lx, %s)\n", hCryptProv, dwFlags,
     debugstr_w((LPCWSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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

    TRACE("(%ld, %08lx, %s)\n", hCryptProv, dwFlags,
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

    TRACE("(%ld, %08lx, %s)\n", hCryptProv, dwFlags,
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
            if (store)
            {
                CertAddStoreToCollection(store, regStore,
                 dwFlags & CERT_STORE_READONLY_FLAG ? 0 :
                 CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
                CertCloseStore(regStore, 0);
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

    TRACE("(%ld, %08lx, %s)\n", hCryptProv, dwFlags,
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

HCERTSTORE WINAPI CertOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwMsgAndCertEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void* pvPara)
{
    WINECRYPT_CERTSTORE *hcs;
    StoreOpenFunc openFunc = NULL;

    TRACE("(%s, %08lx, %08lx, %08lx, %p)\n", debugstr_a(lpszStoreProvider),
          dwMsgAndCertEncodingType, hCryptProv, dwFlags, pvPara);

    if (!HIWORD(lpszStoreProvider))
    {
        switch (LOWORD(lpszStoreProvider))
        {
        case (int)CERT_STORE_PROV_MEMORY:
            openFunc = CRYPT_MemOpenStore;
            break;
        case (int)CERT_STORE_PROV_REG:
            openFunc = CRYPT_RegOpenStore;
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
        default:
            if (LOWORD(lpszStoreProvider))
                FIXME("unimplemented type %d\n", LOWORD(lpszStoreProvider));
        }
    }
    else if (!strcasecmp(lpszStoreProvider, sz_CERT_STORE_PROV_MEMORY))
        openFunc = CRYPT_MemOpenStore;
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
    {
        /* FIXME: need to look for an installed provider for this type */
        SetLastError(ERROR_FILE_NOT_FOUND);
        hcs = NULL;
    }
    else
        hcs = openFunc(hCryptProv, dwFlags, pvPara);
    return (HCERTSTORE)hcs;
}

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV hProv,
 LPCSTR szSubSystemProtocol)
{
    HCERTSTORE ret = 0;

    if (szSubSystemProtocol)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, szSubSystemProtocol, -1, NULL,
         0);
        LPWSTR param = CryptMemAlloc(len * sizeof(WCHAR));

        if (param)
        {
            MultiByteToWideChar(CP_ACP, 0, szSubSystemProtocol, -1, param, len);
            ret = CertOpenSystemStoreW(hProv, param);
            CryptMemFree(param);
        }
    }
    else
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    return ret;
}

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV hProv,
 LPCWSTR szSubSystemProtocol)
{
    HCERTSTORE ret;

    if (!szSubSystemProtocol)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return 0;
    }

    /* FIXME: needs some tests. It seems to open both HKEY_LOCAL_MACHINE and
     * HKEY_CURRENT_USER stores, but I'm not sure under what conditions, if any,
     * it fails.
     */
    ret = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, hProv,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (ret)
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,
         0, hProv, CERT_SYSTEM_STORE_LOCAL_MACHINE, szSubSystemProtocol);

        if (store)
        {
            CertAddStoreToCollection(ret, store,
             CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
            CertCloseStore(store, 0);
        }
        store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,
         0, hProv, CERT_SYSTEM_STORE_CURRENT_USER, szSubSystemProtocol);
        if (store)
        {
            CertAddStoreToCollection(ret, store,
             CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
            CertCloseStore(store, 0);
        }
    }
    return ret;
}

BOOL WINAPI CertSaveStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType,
             DWORD dwSaveAs, DWORD dwSaveTo, void* pvSaveToPara, DWORD dwFlags)
{
    FIXME("(%p,%ld,%ld,%ld,%p,%08lx) stub!\n", hCertStore, 
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);
    return TRUE;
}

PCCRL_CONTEXT WINAPI CertCreateCRLContext( DWORD dwCertEncodingType,
  const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    PCRL_CONTEXT pcrl;
    BYTE* data;

    TRACE("%08lx %p %08lx\n", dwCertEncodingType, pbCrlEncoded, cbCrlEncoded);

    /* FIXME: semi-stub, need to use CryptDecodeObjectEx to decode the CRL. */
    pcrl = CryptMemAlloc( sizeof (CRL_CONTEXT) );
    if( !pcrl )
        return NULL;

    data = CryptMemAlloc( cbCrlEncoded );
    if( !data )
    {
        CryptMemFree( pcrl );
        return NULL;
    }

    pcrl->dwCertEncodingType = dwCertEncodingType;
    pcrl->pbCrlEncoded       = data;
    pcrl->cbCrlEncoded       = cbCrlEncoded;
    pcrl->pCrlInfo           = NULL;
    pcrl->hCertStore         = 0;

    return pcrl;
}

/* Decodes the encoded certificate and creates the certificate context for it.
 * The reference count is initially zero, so you must create a reference to it
 * to avoid leaking memory.
 */
static PWINE_CERT_CONTEXT CRYPT_CreateCertificateContext(
 DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded)
{
    PWINE_CERT_CONTEXT cert = NULL;
    BOOL ret;
    PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
    PCERT_INFO certInfo = NULL;
    DWORD size = 0;

    TRACE("(%08lx, %p, %ld)\n", dwCertEncodingType, pbCertEncoded,
     cbCertEncoded);

    /* First try to decode it as a signed cert. */
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CERT, pbCertEncoded,
     cbCertEncoded, CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
     (BYTE *)&signedCert, &size);
    if (ret)
    {
        size = 0;
        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
         signedCert->ToBeSigned.pbData, signedCert->ToBeSigned.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&certInfo, &size);
        LocalFree(signedCert);
    }
    /* Failing that, try it as an unsigned cert */
    if (!ret)
    {
        size = 0;
        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
         pbCertEncoded, cbCertEncoded,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&certInfo, &size);
    }
    if (ret)
    {
        BYTE *data = NULL;

        cert = CryptMemAlloc(sizeof(WINE_CERT_CONTEXT));
        if (!cert)
            goto end;
        data = CryptMemAlloc(cbCertEncoded);
        if (!data)
        {
            CryptMemFree(cert);
            cert = NULL;
            goto end;
        }
        memcpy(data, pbCertEncoded, cbCertEncoded);
        cert->cert.dwCertEncodingType = dwCertEncodingType;
        cert->cert.pbCertEncoded      = data;
        cert->cert.cbCertEncoded      = cbCertEncoded;
        cert->cert.pCertInfo          = certInfo;
        cert->cert.hCertStore         = 0;
        cert->ref = 0;
        InitializeCriticalSection(&cert->cs);
        list_init(&cert->extendedProperties);
    }

end:
    return cert;
}

static void CRYPT_FreeCert(PWINE_CERT_CONTEXT context)
{
    PWINE_CERT_PROPERTY prop, next;

    CryptMemFree(context->cert.pbCertEncoded);
    LocalFree(context->cert.pCertInfo);
    DeleteCriticalSection(&context->cs);
    LIST_FOR_EACH_ENTRY_SAFE(prop, next, &context->extendedProperties,
     WINE_CERT_PROPERTY, entry)
    {
        list_remove(&prop->entry);
        CryptMemFree(prop->pbData);
        CryptMemFree(prop);
    }
    CryptMemFree(context);
}

PCCERT_CONTEXT WINAPI CertCreateCertificateContext(DWORD dwCertEncodingType,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded)
{
    PWINE_CERT_CONTEXT cert;
    PWINE_CERT_CONTEXT_REF ret = NULL;

    TRACE("(%08lx, %p, %ld)\n", dwCertEncodingType, pbCertEncoded,
     cbCertEncoded);

    cert = CRYPT_CreateCertificateContext(dwCertEncodingType, pbCertEncoded,
     cbCertEncoded);
    if (cert)
        ret = CRYPT_CreateCertRef(cert, 0);
    return (PCCERT_CONTEXT)ret;
}

/* Since the properties are stored in a list, this is a tad inefficient
 * (O(n^2)) since I have to find the previous position every time.
 */
DWORD WINAPI CertEnumCertificateContextProperties(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId)
{
    PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
    DWORD ret;

    TRACE("(%p, %ld)\n", pCertContext, dwPropId);

    EnterCriticalSection(&ref->context->cs);
    if (dwPropId)
    {
        PWINE_CERT_PROPERTY cursor = NULL;

        LIST_FOR_EACH_ENTRY(cursor, &ref->context->extendedProperties,
         WINE_CERT_PROPERTY, entry)
        {
            if (cursor->hdr.propID == dwPropId)
                break;
        }
        if (cursor)
        {
            if (cursor->entry.next != &ref->context->extendedProperties)
                ret = LIST_ENTRY(cursor->entry.next, WINE_CERT_PROPERTY,
                 entry)->hdr.propID;
            else
                ret = 0;
        }
        else
            ret = 0;
    }
    else if (!list_empty(&ref->context->extendedProperties))
        ret = LIST_ENTRY(ref->context->extendedProperties.next,
         WINE_CERT_PROPERTY, entry)->hdr.propID;
    else
        ret = 0;
    LeaveCriticalSection(&ref->context->cs);
    return ret;
}

static BOOL CRYPT_GetCertHashProp(PWINE_CERT_CONTEXT context, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CRYPT_SetCertificateContextProperty(context, dwPropId,
         0, &blob);
    }
    return ret;
}

static BOOL WINAPI CRYPT_GetCertificateContextProperty(
 PWINE_CERT_CONTEXT context, DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    PWINE_CERT_PROPERTY prop;
    BOOL ret, found;

    TRACE("(%p, %ld, %p, %p)\n", context, dwPropId, pvData, pcbData);

    EnterCriticalSection(&context->cs);
    ret = FALSE;
    found = FALSE;
    LIST_FOR_EACH_ENTRY(prop, &context->extendedProperties,
     WINE_CERT_PROPERTY, entry)
    {
        if (prop->hdr.propID == dwPropId)
        {
            if (!pvData)
            {
                *pcbData = prop->hdr.cb;
                ret = TRUE;
            }
            else if (*pcbData < prop->hdr.cb)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbData = prop->hdr.cb;
            }
            else
            {
                memcpy(pvData, prop->pbData, prop->hdr.cb);
                *pcbData = prop->hdr.cb;
                ret = TRUE;
            }
            found = TRUE;
        }
        break;
    }
    if (!found)
    {
        /* Implicit properties */
        switch (dwPropId)
        {
        case CERT_SHA1_HASH_PROP_ID:
            ret = CRYPT_GetCertHashProp(context, dwPropId, CALG_SHA1,
             context->cert.pbCertEncoded, context->cert.cbCertEncoded, pvData,
             pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CRYPT_GetCertHashProp(context, dwPropId, CALG_MD5,
             context->cert.pbCertEncoded, context->cert.cbCertEncoded, pvData,
             pcbData);
            break;
        case CERT_SUBJECT_NAME_MD5_HASH_PROP_ID:
            ret = CRYPT_GetCertHashProp(context, dwPropId, CALG_MD5,
             context->cert.pCertInfo->Subject.pbData,
             context->cert.pCertInfo->Subject.cbData,
             pvData, pcbData);
            break;
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
            ret = CRYPT_GetCertHashProp(context, dwPropId, CALG_MD5,
             context->cert.pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
             context->cert.pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
             pvData, pcbData);
            break;
        case CERT_SIGNATURE_HASH_PROP_ID:
        case CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID:
            FIXME("implicit property %ld\n", dwPropId);
            SetLastError(CRYPT_E_NOT_FOUND);
            break;
        default:
            SetLastError(CRYPT_E_NOT_FOUND);
        }
    }
    LeaveCriticalSection(&context->cs);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
    BOOL ret;

    TRACE("(%p, %ld, %p, %p)\n", pCertContext, dwPropId, pvData, pcbData);

    /* Special cases for invalid/special prop IDs.
     */
    switch (dwPropId)
    {
    case 0:
    case CERT_CERT_PROP_ID:
    case CERT_CRL_PROP_ID:
    case CERT_CTL_PROP_ID:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    case CERT_ACCESS_STATE_PROP_ID:
        if (!pvData)
        {
            *pcbData = sizeof(DWORD);
            return TRUE;
        }
        else if (*pcbData < sizeof(DWORD))
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbData = sizeof(DWORD);
            return FALSE;
        }
        else
        {
            DWORD state = 0;

            if (pCertContext->hCertStore)
            {
                PWINECRYPT_CERTSTORE store =
                 (PWINECRYPT_CERTSTORE)pCertContext->hCertStore;

                /* Take advantage of knowledge of the stores to answer the
                 * access state question
                 */
                if (store->type != StoreTypeReg ||
                 !(store->dwOpenFlags & CERT_STORE_READONLY_FLAG))
                    state |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
            }
            *(DWORD *)pvData = state;
            return TRUE;
        }
    }

    ret = CRYPT_GetCertificateContextProperty(ref->context, dwPropId,
     pvData, pcbData);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Copies cbData bytes from pbData to the context's property with ID
 * dwPropId.
 */
static BOOL CRYPT_SaveCertificateContextProperty(PWINE_CERT_CONTEXT context,
 DWORD dwPropId, const BYTE *pbData, size_t cbData)
{
    BOOL ret = FALSE;
    LPBYTE data;

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
        PWINE_CERT_PROPERTY prop;

        EnterCriticalSection(&context->cs);
        LIST_FOR_EACH_ENTRY(prop, &context->extendedProperties,
         WINE_CERT_PROPERTY, entry)
        {
            if (prop->hdr.propID == dwPropId)
                break;
        }
        if (prop && prop->entry.next != &context->extendedProperties)
        {
            CryptMemFree(prop->pbData);
            prop->hdr.cb = cbData;
            prop->pbData = cbData ? data : NULL;
            ret = TRUE;
        }
        else
        {
            prop = CryptMemAlloc(sizeof(WINE_CERT_PROPERTY));
            if (prop)
            {
                prop->hdr.propID = dwPropId;
                prop->hdr.unknown = 1;
                prop->hdr.cb = cbData;
                list_init(&prop->entry);
                prop->pbData = cbData ? data : NULL;
                list_add_tail(&context->extendedProperties, &prop->entry);
                ret = TRUE;
            }
            else
                CryptMemFree(data);
        }
        LeaveCriticalSection(&context->cs);
    }
    return ret;
}

static BOOL WINAPI CRYPT_SetCertificateContextProperty(
 PWINE_CERT_CONTEXT context, DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %08lx, %p)\n", context, dwPropId, dwFlags, pvData);

    if (!pvData)
    {
        PWINE_CERT_PROPERTY prop, next;

        EnterCriticalSection(&context->cs);
        LIST_FOR_EACH_ENTRY_SAFE(prop, next, &context->extendedProperties,
         WINE_CERT_PROPERTY, entry)
        {
            if (prop->hdr.propID == dwPropId)
            {
                list_remove(&prop->entry);
                CryptMemFree(prop->pbData);
                CryptMemFree(prop);
            }
        }
        LeaveCriticalSection(&context->cs);
        ret = TRUE;
    }
    else
    {
        switch (dwPropId)
        {
        case CERT_AUTO_ENROLL_PROP_ID:
        case CERT_CTL_USAGE_PROP_ID:
        case CERT_DESCRIPTION_PROP_ID:
        case CERT_FRIENDLY_NAME_PROP_ID:
        case CERT_HASH_PROP_ID:
        case CERT_KEY_IDENTIFIER_PROP_ID:
        case CERT_MD5_HASH_PROP_ID:
        case CERT_NEXT_UPDATE_LOCATION_PROP_ID:
        case CERT_PUBKEY_ALG_PARA_PROP_ID:
        case CERT_PVK_FILE_PROP_ID:
        case CERT_SIGNATURE_HASH_PROP_ID:
        case CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID:
        case CERT_SUBJECT_NAME_MD5_HASH_PROP_ID:
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
        case CERT_ENROLLMENT_PROP_ID:
        case CERT_CROSS_CERT_DIST_POINTS_PROP_ID:
        case CERT_RENEWAL_PROP_ID:
        {
            PCRYPT_DATA_BLOB blob = (PCRYPT_DATA_BLOB)pvData;

            ret = CRYPT_SaveCertificateContextProperty(context, dwPropId,
             blob->pbData, blob->cbData);
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            ret = CRYPT_SaveCertificateContextProperty(context, dwPropId,
             pvData, sizeof(FILETIME));
            break;
        default:
            FIXME("%ld: stub\n", dwPropId);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", pCertContext, dwPropId, dwFlags, pvData);

    /* Handle special cases for "read-only"/invalid prop IDs.  Windows just
     * crashes on most of these, I'll be safer.
     */
    switch (dwPropId)
    {
    case 0:
    case CERT_ACCESS_STATE_PROP_ID:
    case CERT_CERT_PROP_ID:
    case CERT_CRL_PROP_ID:
    case CERT_CTL_PROP_ID:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    }
    ret = CRYPT_SetCertificateContextProperty(ref->context, dwPropId,
     dwFlags, pvData);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Only the reference portion of the context is duplicated.  The returned
 * context has the cert store set to 0, to prevent the store's certificate free
 * function from getting called on partial data.
 * FIXME: is this okay?  Needs a test.
 */
PCCERT_CONTEXT WINAPI CertDuplicateCertificateContext(
 PCCERT_CONTEXT pCertContext)
{
    PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext, ret;

    TRACE("(%p)\n", pCertContext);
    if (ref)
    {
        ret = CryptMemAlloc(sizeof(WINE_CERT_CONTEXT_REF));
        if (ret)
        {
            memcpy(ret, ref, sizeof(*ret));
            ret->cert.hCertStore = 0;
            InterlockedIncrement(&ret->context->ref);
        }
    }
    else
        ret = NULL;
    return (PCCERT_CONTEXT)ret;
}

BOOL WINAPI CertAddCertificateContextToStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition,
 PCCERT_CONTEXT *ppStoreContext)
{
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
    PWINE_CERT_CONTEXT cert;
    BOOL ret;

    TRACE("(%p, %p, %08lx, %p)\n", hCertStore, pCertContext,
     dwAddDisposition, ppStoreContext);

    /* FIXME: some tests needed to verify return codes */
    if (!store)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (store->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cert = CRYPT_CreateCertificateContext(ref->context->cert.dwCertEncodingType,
     ref->context->cert.pbCertEncoded, ref->context->cert.cbCertEncoded);
    if (cert)
    {
        PWINE_CERT_PROPERTY prop;

        ret = TRUE;
        EnterCriticalSection(&ref->context->cs);
        LIST_FOR_EACH_ENTRY(prop, &ref->context->extendedProperties,
         WINE_CERT_PROPERTY, entry)
        {
            ret = CRYPT_SaveCertificateContextProperty(cert, prop->hdr.propID,
             prop->pbData, prop->hdr.cb);
            if (!ret)
                break;
        }
        LeaveCriticalSection(&ref->context->cs);
        if (ret)
        {
            ret = store->addCert(store, (PCCERT_CONTEXT)cert, dwAddDisposition);
            if (ret && ppStoreContext)
                *ppStoreContext = (PCCERT_CONTEXT)store->createCertRef(cert,
                 hCertStore);
        }
        if (!ret)
            CRYPT_FreeCert(cert);
    }
    else
        ret = FALSE;
    return ret;
}

BOOL WINAPI CertAddEncodedCertificateToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded,
 DWORD dwAddDisposition, PCCERT_CONTEXT *ppCertContext)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    BOOL ret;

    TRACE("(%p, %08lx, %p, %ld, %08lx, %p)\n", hCertStore, dwCertEncodingType,
     pbCertEncoded, cbCertEncoded, dwAddDisposition, ppCertContext);

    if (!hcs)
        ret = FALSE;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = FALSE;
    else
    {
        PWINE_CERT_CONTEXT cert = CRYPT_CreateCertificateContext(
         dwCertEncodingType, pbCertEncoded, cbCertEncoded);

        if (cert)
        {
            ret = hcs->addCert(hcs, (PCCERT_CONTEXT)cert, dwAddDisposition);
            if (ret && ppCertContext)
                *ppCertContext = (PCCERT_CONTEXT)hcs->createCertRef(cert,
                 hCertStore);
            if (!ret)
                CRYPT_FreeCert(cert);
        }
        else
            ret = FALSE;
    }
    return ret;
}

PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pPrev)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    PWINE_CERT_CONTEXT_REF prev = (PWINE_CERT_CONTEXT_REF)pPrev;
    PCCERT_CONTEXT ret;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (PCCERT_CONTEXT)hcs->enumCert(hcs, prev);
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

        if (!hcs)
            ret = TRUE;
        else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
            ret = FALSE;
        else
        {
            ret = hcs->deleteCert(hcs, pCertContext, 0);
            if (ret)
                CertFreeCertificateContext(pCertContext);
        }
    }
    return ret;
}

BOOL WINAPI CertAddEncodedCRLToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded,
 DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext)
{
    FIXME("(%p, %08lx, %p, %ld, %08lx, %p): stub\n", hCertStore,
     dwCertEncodingType, pbCrlEncoded, cbCrlEncoded, dwAddDisposition,
     ppCrlContext);
    return FALSE;
}

BOOL WINAPI CertAddCRLContextToStore( HCERTSTORE hCertStore,
             PCCRL_CONTEXT pCrlContext, DWORD dwAddDisposition,
             PCCRL_CONTEXT* ppStoreContext )
{
    FIXME("%p %p %08lx %p\n", hCertStore, pCrlContext,
          dwAddDisposition, ppStoreContext);
    return TRUE;
}

BOOL WINAPI CertFreeCRLContext( PCCRL_CONTEXT pCrlContext)
{
    FIXME("%p\n", pCrlContext );

    return TRUE;
}

BOOL WINAPI CertDeleteCRLFromStore(PCCRL_CONTEXT pCrlContext)
{
    FIXME("(%p): stub\n", pCrlContext);
    return TRUE;
}

PCCRL_CONTEXT WINAPI CertEnumCRLsInStore(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pPrev)
{
    FIXME("(%p, %p): stub\n", hCertStore, pPrev);
    return NULL;
}

PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwCertEncodingType,
  const BYTE* pbCtlEncoded, DWORD cbCtlEncoded)
{
    FIXME("(%08lx, %p, %08lx): stub\n", dwCertEncodingType, pbCtlEncoded,
     cbCtlEncoded);
    return NULL;
}

BOOL WINAPI CertAddEncodedCTLToStore(HCERTSTORE hCertStore,
 DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded,
 DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext)
{
    FIXME("(%p, %08lx, %p, %ld, %08lx, %p): stub\n", hCertStore,
     dwMsgAndCertEncodingType, pbCtlEncoded, cbCtlEncoded, dwAddDisposition,
     ppCtlContext);
    return FALSE;
}

BOOL WINAPI CertAddCTLContextToStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pCtlContext, DWORD dwAddDisposition,
 PCCTL_CONTEXT* ppStoreContext)
{
    FIXME("(%p, %p, %08lx, %p): stub\n", hCertStore, pCtlContext,
     dwAddDisposition, ppStoreContext);
    return TRUE;
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


BOOL WINAPI CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *) hCertStore;

    TRACE("(%p, %08lx)\n", hCertStore, dwFlags);

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
        TRACE("%p's ref count is %ld\n", hcs, hcs->ref);
    return TRUE;
}

BOOL WINAPI CertControlStore(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    BOOL ret;

    TRACE("(%p, %08lx, %ld, %p)\n", hCertStore, dwFlags, dwCtrlType,
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

BOOL WINAPI CertGetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    FIXME("(%p, %ld, %p, %p): stub\n", pCRLContext, dwPropId, pvData, pcbData);
    return FALSE;
}

BOOL WINAPI CertSetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    FIXME("(%p, %ld, %08lx, %p): stub\n", pCRLContext, dwPropId, dwFlags,
     pvData);
    return FALSE;
}

BOOL WINAPI CertSerializeCRLStoreElement(PCCRL_CONTEXT pCrlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    FIXME("(%p, %08lx, %p, %p): stub\n", pCrlContext, dwFlags, pbElement,
     pcbElement);
    return FALSE;
}

BOOL WINAPI CertGetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    FIXME("(%p, %ld, %p, %p): stub\n", pCTLContext, dwPropId, pvData, pcbData);
    return FALSE;
}

BOOL WINAPI CertSetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    FIXME("(%p, %ld, %08lx, %p): stub\n", pCTLContext, dwPropId, dwFlags,
     pvData);
    return FALSE;
}

BOOL WINAPI CertSerializeCTLStoreElement(PCCTL_CONTEXT pCtlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    FIXME("(%p, %08lx, %p, %p): stub\n", pCtlContext, dwFlags, pbElement,
     pcbElement);
    return FALSE;
}

BOOL WINAPI CertSerializeCertificateStoreElement(PCCERT_CONTEXT pCertContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    BOOL ret;

    TRACE("(%p, %08lx, %p, %p)\n", pCertContext, dwFlags, pbElement,
     pcbElement);

    if (pCertContext)
    {
        PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
        DWORD bytesNeeded = sizeof(WINE_CERT_PROP_HEADER) +
         pCertContext->cbCertEncoded;
        PWINE_CERT_PROPERTY prop;

        EnterCriticalSection(&ref->context->cs);
        LIST_FOR_EACH_ENTRY(prop, &ref->context->extendedProperties,
         WINE_CERT_PROPERTY, entry)
            bytesNeeded += sizeof(WINE_CERT_PROP_HEADER) + prop->hdr.cb;
        if (!pbElement)
        {
            *pcbElement = bytesNeeded;
            ret = TRUE;
        }
        else if (*pcbElement < bytesNeeded)
        {
            *pcbElement = bytesNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            PWINE_CERT_PROP_HEADER hdr;

            LIST_FOR_EACH_ENTRY(prop, &ref->context->extendedProperties,
             WINE_CERT_PROPERTY, entry)
            {
                memcpy(pbElement, &prop->hdr, sizeof(WINE_CERT_PROP_HEADER));
                pbElement += sizeof(WINE_CERT_PROP_HEADER);
                if (prop->hdr.cb)
                {
                    memcpy(pbElement, prop->pbData, prop->hdr.cb);
                    pbElement += prop->hdr.cb;
                }
            }
            hdr = (PWINE_CERT_PROP_HEADER)pbElement;
            hdr->propID = CERT_CERT_PROP_ID;
            hdr->unknown = 1;
            hdr->cb = pCertContext->cbCertEncoded;
            memcpy(pbElement + sizeof(WINE_CERT_PROP_HEADER),
             pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
            ret = TRUE;
        }
        LeaveCriticalSection(&ref->context->cs);
    }
    else
        ret = FALSE;
    return ret;
}

/* Looks for the property with ID propID in the buffer buf.  Returns a pointer
 * to its header if a valid header is found, NULL if not.  Valid means the
 * length of thte property won't overrun buf, and the unknown field is 1.
 */
static const WINE_CERT_PROP_HEADER *CRYPT_findPropID(const BYTE *buf,
 DWORD size, DWORD propID)
{
    const WINE_CERT_PROP_HEADER *ret = NULL;
    BOOL done = FALSE;

    while (size && !ret && !done)
    {
        if (size < sizeof(WINE_CERT_PROP_HEADER))
        {
            SetLastError(CRYPT_E_FILE_ERROR);
            done = TRUE;
        }
        else
        {
            const WINE_CERT_PROP_HEADER *hdr =
             (const WINE_CERT_PROP_HEADER *)buf;

            size -= sizeof(WINE_CERT_PROP_HEADER);
            buf += sizeof(WINE_CERT_PROP_HEADER);
            if (size < hdr->cb)
            {
                SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                done = TRUE;
            }
            else if (!hdr->propID)
            {
                /* assume a zero prop ID means the data are uninitialized, so
                 * stop looking.
                 */
                done = TRUE;
            }
            else if (hdr->unknown != 1)
            {
                SetLastError(ERROR_FILE_NOT_FOUND);
                done = TRUE;
            }
            else if (hdr->propID == propID)
                ret = hdr;
            else
            {
                buf += hdr->cb;
                size -= hdr->cb;
            }
        }
    }
    return ret;
}

static const void * WINAPI CRYPT_ReadSerializedElement(const BYTE *pbElement,
 DWORD cbElement, DWORD dwContextTypeFlags, DWORD *pdwContentType)
{
    const void *context;

    TRACE("(%p, %ld, %08lx, %p)\n", pbElement, cbElement, dwContextTypeFlags,
     pdwContentType);

    if (!cbElement)
    {
        SetLastError(ERROR_END_OF_MEDIA);
        return NULL;
    }

    __TRY
    {
        const WINE_CONTEXT_INTERFACE *contextInterface = NULL;
        const WINE_CERT_PROP_HEADER *hdr = NULL;
        DWORD type = 0;
        BOOL ret;

        ret = TRUE;
        context = NULL;
        if (dwContextTypeFlags == CERT_STORE_ALL_CONTEXT_FLAG)
        {
            hdr = CRYPT_findPropID(pbElement, cbElement, CERT_CERT_PROP_ID);
            if (hdr)
                type = CERT_STORE_CERTIFICATE_CONTEXT;
            else
            {
                hdr = CRYPT_findPropID(pbElement, cbElement, CERT_CRL_PROP_ID);
                if (hdr)
                    type = CERT_STORE_CRL_CONTEXT;
                else
                {
                    hdr = CRYPT_findPropID(pbElement, cbElement,
                     CERT_CTL_PROP_ID);
                    if (hdr)
                        type = CERT_STORE_CTL_CONTEXT;
                }
            }
        }
        else if (dwContextTypeFlags & CERT_STORE_CERTIFICATE_CONTEXT_FLAG)
        {
            hdr = CRYPT_findPropID(pbElement, cbElement, CERT_CERT_PROP_ID);
            type = CERT_STORE_CERTIFICATE_CONTEXT;
        }
        else if (dwContextTypeFlags & CERT_STORE_CRL_CONTEXT_FLAG)
        {
            hdr = CRYPT_findPropID(pbElement, cbElement, CERT_CRL_PROP_ID);
            type = CERT_STORE_CRL_CONTEXT;
        }
        else if (dwContextTypeFlags & CERT_STORE_CTL_CONTEXT_FLAG)
        {
            hdr = CRYPT_findPropID(pbElement, cbElement, CERT_CTL_PROP_ID);
            type = CERT_STORE_CTL_CONTEXT;
        }

        switch (type)
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
            SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
            ret = FALSE;
        }
        if (!hdr)
            ret = FALSE;

        if (ret)
            context = contextInterface->create(X509_ASN_ENCODING,
             (BYTE *)hdr + sizeof(WINE_CERT_PROP_HEADER), hdr->cb);
        if (ret && context)
        {
            BOOL noMoreProps = FALSE;

            while (!noMoreProps && ret)
            {
                if (cbElement < sizeof(WINE_CERT_PROP_HEADER))
                    ret = FALSE;
                else
                {
                    const WINE_CERT_PROP_HEADER *hdr =
                     (const WINE_CERT_PROP_HEADER *)pbElement;

                    TRACE("prop is %ld\n", hdr->propID);
                    cbElement -= sizeof(WINE_CERT_PROP_HEADER);
                    pbElement += sizeof(WINE_CERT_PROP_HEADER);
                    if (cbElement < hdr->cb)
                    {
                        SetLastError(HRESULT_FROM_WIN32(
                         ERROR_INVALID_PARAMETER));
                        ret = FALSE;
                    }
                    else if (!hdr->propID)
                    {
                        /* Like in CRYPT_findPropID, stop if the propID is zero
                         */
                        noMoreProps = TRUE;
                    }
                    else if (hdr->unknown != 1)
                    {
                        SetLastError(ERROR_FILE_NOT_FOUND);
                        ret = FALSE;
                    }
                    else if (hdr->propID != CERT_CERT_PROP_ID &&
                     hdr->propID != CERT_CRL_PROP_ID && hdr->propID !=
                     CERT_CTL_PROP_ID)
                    {
                        /* Have to create a blob for most types, but not
                         * for all.. arghh.
                         */
                        switch (hdr->propID)
                        {
                        case CERT_AUTO_ENROLL_PROP_ID:
                        case CERT_CTL_USAGE_PROP_ID:
                        case CERT_DESCRIPTION_PROP_ID:
                        case CERT_FRIENDLY_NAME_PROP_ID:
                        case CERT_HASH_PROP_ID:
                        case CERT_KEY_IDENTIFIER_PROP_ID:
                        case CERT_MD5_HASH_PROP_ID:
                        case CERT_NEXT_UPDATE_LOCATION_PROP_ID:
                        case CERT_PUBKEY_ALG_PARA_PROP_ID:
                        case CERT_PVK_FILE_PROP_ID:
                        case CERT_SIGNATURE_HASH_PROP_ID:
                        case CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID:
                        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
                        case CERT_ENROLLMENT_PROP_ID:
                        case CERT_CROSS_CERT_DIST_POINTS_PROP_ID:
                        case CERT_RENEWAL_PROP_ID:
                        {
                            CRYPT_DATA_BLOB blob = { hdr->cb,
                             (LPBYTE)pbElement };

                            ret = contextInterface->setProp(context,
                             hdr->propID, 0, &blob);
                            break;
                        }
                        case CERT_DATE_STAMP_PROP_ID:
                            ret = contextInterface->setProp(context,
                             hdr->propID, 0, pbElement);
                            break;
                        default:
                            FIXME("prop ID %ld: stub\n", hdr->propID);
                        }
                    }
                    pbElement += hdr->cb;
                    cbElement -= hdr->cb;
                    if (!cbElement)
                        noMoreProps = TRUE;
                }
            }
            if (ret)
            {
                if (pdwContentType)
                    *pdwContentType = type;
            }
            else
            {
                contextInterface->free(context);
                context = NULL;
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        context = NULL;
    }
    __ENDTRY
    return context;
}

BOOL WINAPI CertAddSerializedElementToStore(HCERTSTORE hCertStore,
 const BYTE *pbElement, DWORD cbElement, DWORD dwAddDisposition, DWORD dwFlags,
 DWORD dwContextTypeFlags, DWORD *pdwContentType, const void **ppvContext)
{
    const void *context;
    DWORD type;
    BOOL ret;

    TRACE("(%p, %p, %ld, %08lx, %08lx, %08lx, %p, %p)\n", hCertStore,
     pbElement, cbElement, dwAddDisposition, dwFlags, dwContextTypeFlags,
     pdwContentType, ppvContext);

    /* Call the internal function, then delete the hashes.  Tests show this
     * function uses real hash values, not whatever's stored in the hash
     * property.
     */
    context = CRYPT_ReadSerializedElement(pbElement, cbElement,
     dwContextTypeFlags, &type);
    if (context)
    {
        const WINE_CONTEXT_INTERFACE *contextInterface = NULL;

        switch (type)
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
            SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        }
        if (contextInterface)
        {
            contextInterface->setProp(context, CERT_HASH_PROP_ID, 0, NULL);
            contextInterface->setProp(context, CERT_MD5_HASH_PROP_ID, 0, NULL);
            contextInterface->setProp(context, CERT_SIGNATURE_HASH_PROP_ID, 0,
             NULL);
            if (pdwContentType)
                *pdwContentType = type;
            ret = contextInterface->addContextToStore(hCertStore, context,
             dwAddDisposition, ppvContext);
            contextInterface->free(context);
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}

static void CRYPT_UnrefCertificateContext(PWINE_CERT_CONTEXT_REF ref)
{
    if (InterlockedDecrement(&ref->context->ref) == 0)
    {
        TRACE("%p's ref count is 0, freeing\n", ref->context);
        CRYPT_FreeCert(ref->context);
    }
    else
        TRACE("%p's ref count is %ld\n", ref->context, ref->context->ref);
}

BOOL WINAPI CertFreeCertificateContext(PCCERT_CONTEXT pCertContext)
{
    TRACE("(%p)\n", pCertContext);

    if (pCertContext)
    {
        PWINE_CERT_CONTEXT_REF ref = (PWINE_CERT_CONTEXT_REF)pCertContext;
        PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)ref->cert.hCertStore;

        CRYPT_UnrefCertificateContext(ref);
        if (store && store->dwMagic == WINE_CRYPTCERTSTORE_MAGIC &&
         store->freeCert)
            store->freeCert(ref);
        TRACE("freeing %p\n", ref);
        CryptMemFree(ref);
    }
    return TRUE;
}

PCCERT_CONTEXT WINAPI CertFindCertificateInStore(HCERTSTORE hCertStore,
		DWORD dwCertEncodingType, DWORD dwFlags, DWORD dwType,
		const void *pvPara, PCCERT_CONTEXT pPrevCertContext)
{
    FIXME("stub: %p %ld %ld %ld %p %p\n", hCertStore, dwCertEncodingType,
	dwFlags, dwType, pvPara, pPrevCertContext);
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

BOOL WINAPI CertAddStoreToCollection(HCERTSTORE hCollectionStore,
 HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
    PWINE_COLLECTIONSTORE collection = (PWINE_COLLECTIONSTORE)hCollectionStore;
    WINECRYPT_CERTSTORE *sibling = (WINECRYPT_CERTSTORE *)hSiblingStore;
    PWINE_STORE_LIST_ENTRY entry;
    BOOL ret;

    TRACE("(%p, %p, %08lx, %ld)\n", hCollectionStore, hSiblingStore,
     dwUpdateFlags, dwPriority);

    if (!collection || !sibling)
        return TRUE;
    if (collection->hdr.dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    }
    if (collection->hdr.type != StoreTypeCollection)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    }
    if (sibling->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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
        list_init(&entry->entry);
        TRACE("%p: adding %p, priority %ld\n", collection, entry, dwPriority);
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
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return;
    }
    if (collection->hdr.type != StoreTypeCollection)
        return;
    if (sibling->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
    {
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr,
 CRYPT_ATTRIBUTE rgAttr[])
{
    PCRYPT_ATTRIBUTE ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cAttr, rgAttr);

    if (!cAttr)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cAttr; i++)
        if (rgAttr[i].pszObjId && !strcmp(pszObjId, rgAttr[i].pszObjId))
            ret = &rgAttr[i];
    return ret;
}

PCERT_EXTENSION WINAPI CertFindExtension(LPCSTR pszObjId, DWORD cExtensions,
 CERT_EXTENSION rgExtensions[])
{
    PCERT_EXTENSION ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cExtensions, rgExtensions);

    if (!cExtensions)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cExtensions; i++)
        if (rgExtensions[i].pszObjId && !strcmp(pszObjId,
         rgExtensions[i].pszObjId))
            ret = &rgExtensions[i];
    return ret;
}

PCERT_RDN_ATTR WINAPI CertFindRDNAttr(LPCSTR pszObjId, PCERT_NAME_INFO pName)
{
    PCERT_RDN_ATTR ret = NULL;
    DWORD i, j;

    TRACE("%s %p\n", debugstr_a(pszObjId), pName);

    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < pName->cRDN; i++)
        for (j = 0; !ret && j < pName->rgRDN[i].cRDNAttr; j++)
            if (pName->rgRDN[i].rgRDNAttr[j].pszObjId && !strcmp(pszObjId,
             pName->rgRDN[i].rgRDNAttr[j].pszObjId))
                ret = &pName->rgRDN[i].rgRDNAttr[j];
    return ret;
}

LONG WINAPI CertVerifyTimeValidity(LPFILETIME pTimeToVerify,
 PCERT_INFO pCertInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        SYSTEMTIME sysTime;

        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        pTimeToVerify = &fileTime;
    }
    if ((ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotBefore)) >= 0)
    {
        ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotAfter);
        if (ret < 0)
            ret = 0;
    }
    return ret;
}

BOOL WINAPI CryptHashCertificate(HCRYPTPROV hCryptProv, ALG_ID Algid,
 DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%ld, %d, %08lx, %p, %ld, %p, %p)\n", hCryptProv, Algid, dwFlags,
     pbEncoded, cbEncoded, pbComputedHash, pcbComputedHash);

    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    if (!Algid)
        Algid = CALG_SHA1;
    if (ret)
    {
        ret = CryptCreateHash(hCryptProv, Algid, 0, 0, &hHash);
        if (ret)
        {
            ret = CryptHashData(hHash, pbEncoded, cbEncoded, 0);
            if (ret)
                ret = CryptGetHashParam(hHash, HP_HASHVAL, pbComputedHash,
                 pcbComputedHash, 0);
            CryptDestroyHash(hHash);
        }
    }
    return ret;
}

BOOL WINAPI CryptSignCertificate(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, const BYTE *pbEncodedToBeSigned,
 DWORD cbEncodedToBeSigned, PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
 const void *pvHashAuxInfo, BYTE *pbSignature, DWORD *pcbSignature)
{
    BOOL ret;
    ALG_ID algID;
    HCRYPTHASH hHash;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %p, %p, %p)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, pbEncodedToBeSigned, cbEncodedToBeSigned,
     pSignatureAlgorithm, pvHashAuxInfo, pbSignature, pcbSignature);

    algID = CertOIDToAlgId(pSignatureAlgorithm->pszObjId);
    if (!algID)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    if (!hCryptProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hHash);
    if (ret)
    {
        ret = CryptHashData(hHash, pbEncodedToBeSigned, cbEncodedToBeSigned, 0);
        if (ret)
            ret = CryptSignHashW(hHash, dwKeySpec, NULL, 0, pbSignature,
             pcbSignature);
        CryptDestroyHash(hHash);
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignature(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 PCERT_PUBLIC_KEY_INFO pPublicKey)
{
    return CryptVerifyCertificateSignatureEx(hCryptProv, dwCertEncodingType,
     CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, (void *)pbEncoded,
     CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, pPublicKey, 0, NULL);
}

BOOL WINAPI CryptVerifyCertificateSignatureEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, DWORD dwSubjectType, void *pvSubject,
 DWORD dwIssuerType, void *pvIssuer, DWORD dwFlags, void *pvReserved)
{
    BOOL ret = TRUE;
    CRYPT_DATA_BLOB subjectBlob;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %08lx, %p)\n", hCryptProv,
     dwCertEncodingType, dwSubjectType, pvSubject, dwIssuerType, pvIssuer,
     dwFlags, pvReserved);

    switch (dwSubjectType)
    {
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB:
    {
        PCRYPT_DATA_BLOB blob = (PCRYPT_DATA_BLOB)pvSubject;

        subjectBlob.pbData = blob->pbData;
        subjectBlob.cbData = blob->cbData;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT:
    {
        PCERT_CONTEXT context = (PCERT_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCertEncoded;
        subjectBlob.cbData = context->cbCertEncoded;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL:
    {
        PCRL_CONTEXT context = (PCRL_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCrlEncoded;
        subjectBlob.cbData = context->cbCrlEncoded;
        break;
    }
    default:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        ret = FALSE;
    }

    if (ret)
    {
        PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT,
         subjectBlob.pbData, subjectBlob.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&signedCert, &size);
        if (ret)
        {
            switch (dwIssuerType)
            {
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY:
            {
                PCERT_PUBLIC_KEY_INFO pubKeyInfo =
                 (PCERT_PUBLIC_KEY_INFO)pvIssuer;
                ALG_ID algID = CertOIDToAlgId(pubKeyInfo->Algorithm.pszObjId);

                if (algID)
                {
                    HCRYPTKEY key;

                    ret = CryptImportPublicKeyInfoEx(hCryptProv,
                     dwCertEncodingType, pubKeyInfo, algID, 0, NULL, &key);
                    if (ret)
                    {
                        HCRYPTHASH hash;

                        ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hash);
                        if (ret)
                        {
                            ret = CryptHashData(hash,
                             signedCert->ToBeSigned.pbData,
                             signedCert->ToBeSigned.cbData, 0);
                            if (ret)
                            {
                                ret = CryptVerifySignatureW(hash,
                                 signedCert->Signature.pbData,
                                 signedCert->Signature.cbData, key, NULL, 0);
                            }
                            CryptDestroyHash(hash);
                        }
                        CryptDestroyKey(key);
                    }
                }
                else
                {
                    SetLastError(NTE_BAD_ALGID);
                    ret = FALSE;
                }
                break;
            }
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT:
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN:
                FIXME("issuer type %ld: stub\n", dwIssuerType);
                ret = FALSE;
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL:
                if (pvIssuer)
                {
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                else
                {
                    FIXME("unimplemented for NULL signer\n");
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                break;
            default:
                SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                ret = FALSE;
            }
            LocalFree(signedCert);
        }
    }
    return ret;
}

BOOL WINAPI CryptVerifyMessageSignature(/*PCRYPT_VERIFY_MESSAGE_PARA*/ void* pVerifyPara,
          DWORD dwSignerIndex, const BYTE* pbSignedBlob, DWORD cbSignedBlob,
          BYTE* pbDecoded, DWORD* pcbDecoded, PCCERT_CONTEXT* ppSignerCert)
{
    FIXME("stub: %p, %ld, %p, %ld, %p, %p, %p\n",
        pVerifyPara, dwSignerIndex, pbSignedBlob, cbSignedBlob,
        pbDecoded, pcbDecoded, ppSignerCert);
    return FALSE;
}
