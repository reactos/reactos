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

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "winuser.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static const WINE_CONTEXT_INTERFACE gCertInterface = {
    (CreateContextFunc)CertCreateCertificateContext,
    (AddContextToStoreFunc)CertAddCertificateContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCertificateToStore,
    (EnumContextsInStoreFunc)CertEnumCertificatesInStore,
    (EnumPropertiesFunc)CertEnumCertificateContextProperties,
    (GetContextPropertyFunc)CertGetCertificateContextProperty,
    (SetContextPropertyFunc)CertSetCertificateContextProperty,
    (SerializeElementFunc)CertSerializeCertificateStoreElement,
    (DeleteContextFunc)CertDeleteCertificateFromStore,
};
const WINE_CONTEXT_INTERFACE *pCertInterface = &gCertInterface;

static const WINE_CONTEXT_INTERFACE gCRLInterface = {
    (CreateContextFunc)CertCreateCRLContext,
    (AddContextToStoreFunc)CertAddCRLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCRLToStore,
    (EnumContextsInStoreFunc)CertEnumCRLsInStore,
    (EnumPropertiesFunc)CertEnumCRLContextProperties,
    (GetContextPropertyFunc)CertGetCRLContextProperty,
    (SetContextPropertyFunc)CertSetCRLContextProperty,
    (SerializeElementFunc)CertSerializeCRLStoreElement,
    (DeleteContextFunc)CertDeleteCRLFromStore,
};
const WINE_CONTEXT_INTERFACE *pCRLInterface = &gCRLInterface;

static const WINE_CONTEXT_INTERFACE gCTLInterface = {
    (CreateContextFunc)CertCreateCTLContext,
    (AddContextToStoreFunc)CertAddCTLContextToStore,
    (AddEncodedContextToStoreFunc)CertAddEncodedCTLToStore,
    (EnumContextsInStoreFunc)CertEnumCTLsInStore,
    (EnumPropertiesFunc)CertEnumCTLContextProperties,
    (GetContextPropertyFunc)CertGetCTLContextProperty,
    (SetContextPropertyFunc)CertSetCTLContextProperty,
    (SerializeElementFunc)CertSerializeCTLStoreElement,
    (DeleteContextFunc)CertDeleteCTLFromStore,
};
const WINE_CONTEXT_INTERFACE *pCTLInterface = &gCTLInterface;

typedef struct _WINE_MEMSTORE
{
    WINECRYPT_CERTSTORE hdr;
    CRITICAL_SECTION cs;
    struct list certs;
    struct list crls;
    struct list ctls;
} WINE_MEMSTORE;

void CRYPT_InitStore(WINECRYPT_CERTSTORE *store, DWORD dwFlags, CertStoreType type, const store_vtbl_t *vtbl)
{
    store->ref = 1;
    store->dwMagic = WINE_CRYPTCERTSTORE_MAGIC;
    store->type = type;
    store->dwOpenFlags = dwFlags;
    store->vtbl = vtbl;
    store->properties = NULL;
}

void CRYPT_FreeStore(WINECRYPT_CERTSTORE *store)
{
    if (store->properties)
        ContextPropertyList_Free(store->properties);
    store->dwMagic = 0;
    CryptMemFree(store);
}

BOOL WINAPI I_CertUpdateStore(HCERTSTORE store1, HCERTSTORE store2, DWORD unk0,
 DWORD unk1)
{
    static BOOL warned = FALSE;
    const WINE_CONTEXT_INTERFACE * const interfaces[] = { pCertInterface,
     pCRLInterface, pCTLInterface };
    DWORD i;

    TRACE("(%p, %p, %08lx, %08lx)\n", store1, store2, unk0, unk1);
    if (!warned)
    {
        FIXME("semi-stub\n");
        warned = TRUE;
    }

    /* Poor-man's resync:  empty first store, then add everything from second
     * store to it.
     */
    for (i = 0; i < ARRAY_SIZE(interfaces); i++)
    {
        const void *context;

        do {
            context = interfaces[i]->enumContextsInStore(store1, NULL);
            if (context)
                interfaces[i]->deleteFromStore(context);
        } while (context);
        do {
            context = interfaces[i]->enumContextsInStore(store2, context);
            if (context)
                interfaces[i]->addContextToStore(store1, context,
                 CERT_STORE_ADD_ALWAYS, NULL);
        } while (context);
    }
    return TRUE;
}

static BOOL MemStore_addContext(WINE_MEMSTORE *store, struct list *list, context_t *orig_context,
 context_t *existing, context_t **ret_context, BOOL use_link)
{
    context_t *context;

    context = orig_context->vtbl->clone(orig_context, &store->hdr, use_link);
    if (!context)
        return FALSE;

    TRACE("adding %p\n", context);
    EnterCriticalSection(&store->cs);
    if (existing) {
        context->u.entry.prev = existing->u.entry.prev;
        context->u.entry.next = existing->u.entry.next;
        context->u.entry.prev->next = &context->u.entry;
        context->u.entry.next->prev = &context->u.entry;
        list_init(&existing->u.entry);
        if(!existing->ref)
            Context_Release(existing);
    }else {
        list_add_head(list, &context->u.entry);
    }
    LeaveCriticalSection(&store->cs);

    if(ret_context)
        *ret_context = context;
    else
        Context_Release(context);
    return TRUE;
}

static context_t *MemStore_enumContext(WINE_MEMSTORE *store, struct list *list, context_t *prev)
{
    struct list *next;
    context_t *ret;

    EnterCriticalSection(&store->cs);
    if (prev) {
        next = list_next(list, &prev->u.entry);
        Context_Release(prev);
    }else {
        next = list_next(list, list);
    }
    LeaveCriticalSection(&store->cs);

    if (!next) {
        SetLastError(CRYPT_E_NOT_FOUND);
        return NULL;
    }

    ret = LIST_ENTRY(next, context_t, u.entry);
    Context_AddRef(ret);
    return ret;
}

static BOOL MemStore_deleteContext(WINE_MEMSTORE *store, context_t *context)
{
    BOOL in_list = FALSE;

    EnterCriticalSection(&store->cs);
    if (!list_empty(&context->u.entry)) {
        list_remove(&context->u.entry);
        list_init(&context->u.entry);
        in_list = TRUE;
    }
    LeaveCriticalSection(&store->cs);

    if(in_list && !context->ref)
        Context_Free(context);
    return TRUE;
}

static void free_contexts(struct list *list)
{
    context_t *context, *next;

    LIST_FOR_EACH_ENTRY_SAFE(context, next, list, context_t, u.entry)
    {
        TRACE("freeing %p\n", context);
        list_remove(&context->u.entry);
        Context_Free(context);
    }
}

static void MemStore_releaseContext(WINECRYPT_CERTSTORE *store, context_t *context)
{
    /* Free the context only if it's not in a list. Otherwise it may be reused later. */
    if(list_empty(&context->u.entry))
        Context_Free(context);
}

static BOOL MemStore_addCert(WINECRYPT_CERTSTORE *store, context_t *cert,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p, %p, %p)\n", store, cert, toReplace, ppStoreContext);
    return MemStore_addContext(ms, &ms->certs, cert, toReplace, ppStoreContext, use_link);
}

static context_t *MemStore_enumCert(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, prev);

    return MemStore_enumContext(ms, &ms->certs, prev);
}

static BOOL MemStore_deleteCert(WINECRYPT_CERTSTORE *store, context_t *context)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, context);

    return MemStore_deleteContext(ms, context);
}

static BOOL MemStore_addCRL(WINECRYPT_CERTSTORE *store, context_t *crl,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p, %p, %p)\n", store, crl, toReplace, ppStoreContext);

    return MemStore_addContext(ms, &ms->crls, crl, toReplace, ppStoreContext, use_link);
}

static context_t *MemStore_enumCRL(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, prev);

    return MemStore_enumContext(ms, &ms->crls, prev);
}

static BOOL MemStore_deleteCRL(WINECRYPT_CERTSTORE *store, context_t *context)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, context);

    return MemStore_deleteContext(ms, context);
}

static BOOL MemStore_addCTL(WINECRYPT_CERTSTORE *store, context_t *ctl,
 context_t *toReplace, context_t **ppStoreContext, BOOL use_link)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p, %p, %p)\n", store, ctl, toReplace, ppStoreContext);

    return MemStore_addContext(ms, &ms->ctls, ctl, toReplace, ppStoreContext, use_link);
}

static context_t *MemStore_enumCTL(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, prev);

    return MemStore_enumContext(ms, &ms->ctls, prev);
}

static BOOL MemStore_deleteCTL(WINECRYPT_CERTSTORE *store, context_t *context)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    TRACE("(%p, %p)\n", store, context);

    return MemStore_deleteContext(ms, context);
}

static void MemStore_addref(WINECRYPT_CERTSTORE *store)
{
    LONG ref = InterlockedIncrement(&store->ref);
    TRACE("ref = %ld\n", ref);
}

static DWORD MemStore_release(WINECRYPT_CERTSTORE *cert_store, DWORD flags)
{
    WINE_MEMSTORE *store = (WINE_MEMSTORE*)cert_store;
    LONG ref;

    if(flags & ~CERT_CLOSE_STORE_CHECK_FLAG)
        FIXME("Unimplemented flags %lx\n", flags);

    ref = InterlockedDecrement(&store->hdr.ref);
    TRACE("(%p) ref=%ld\n", store, ref);
    if(ref)
        return (flags & CERT_CLOSE_STORE_CHECK_FLAG) ? CRYPT_E_PENDING_CLOSE : ERROR_SUCCESS;

    free_contexts(&store->certs);
    free_contexts(&store->crls);
    free_contexts(&store->ctls);
    store->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&store->cs);
    CRYPT_FreeStore(&store->hdr);
    return ERROR_SUCCESS;
}

static BOOL MemStore_control(WINECRYPT_CERTSTORE *store, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static const store_vtbl_t MemStoreVtbl = {
    MemStore_addref,
    MemStore_release,
    MemStore_releaseContext,
    MemStore_control,
    {
        MemStore_addCert,
        MemStore_enumCert,
        MemStore_deleteCert
    }, {
        MemStore_addCRL,
        MemStore_enumCRL,
        MemStore_deleteCRL
    }, {
        MemStore_addCTL,
        MemStore_enumCTL,
        MemStore_deleteCTL
    }
};

static WINECRYPT_CERTSTORE *CRYPT_MemOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    WINE_MEMSTORE *store;

    TRACE("(%Id, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

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
            CRYPT_InitStore(&store->hdr, dwFlags, StoreTypeMem, &MemStoreVtbl);
            InitializeCriticalSectionEx(&store->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
            store->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ContextList.cs");
            list_init(&store->certs);
            list_init(&store->crls);
            list_init(&store->ctls);
            /* Mem store doesn't need crypto provider, so close it */
            if (hCryptProv && !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                CryptReleaseContext(hCryptProv, 0);
        }
    }
    return (WINECRYPT_CERTSTORE*)store;
}

static WINECRYPT_CERTSTORE *CRYPT_SysRegOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    LPCWSTR storeName = pvPara;
    LPWSTR storePath;
    WINECRYPT_CERTSTORE *store = NULL;
    HKEY root;
    LPCWSTR base;

    TRACE("(%Id, %08lx, %s)\n", hCryptProv, dwFlags,
     debugstr_w(pvPara));

    if (!pvPara)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    switch (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK)
    {
    case CERT_SYSTEM_STORE_LOCAL_MACHINE:
        root = HKEY_LOCAL_MACHINE;
        base = CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH;
        /* If the HKLM\Root certs are requested, expressing system certs into the registry */
        if (!lstrcmpiW(storeName, L"Root"))
            CRYPT_ImportSystemRootCertsToReg();
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

        wsprintfW(storePath, L"%s\\%s", base, storeName);
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

static WINECRYPT_CERTSTORE *CRYPT_SysRegOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    int len;
    WINECRYPT_CERTSTORE *ret = NULL;

    TRACE("(%Id, %08lx, %s)\n", hCryptProv, dwFlags,
     debugstr_a(pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    len = MultiByteToWideChar(CP_ACP, 0, pvPara, -1, NULL, 0);
    if (len)
    {
        LPWSTR storeName = CryptMemAlloc(len * sizeof(WCHAR));

        if (storeName)
        {
            MultiByteToWideChar(CP_ACP, 0, pvPara, -1, storeName, len);
            ret = CRYPT_SysRegOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}

static WINECRYPT_CERTSTORE *CRYPT_SysOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCERTSTORE store = 0;
    BOOL ret;

    TRACE("(%Id, %08lx, %s)\n", hCryptProv, dwFlags,
     debugstr_w(pvPara));

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
         0, 0, dwFlags, pvPara);

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
                 0, dwFlags, pvPara);
                if (regStore)
                {
                    CertAddStoreToCollection(store, regStore,
                     dwFlags & CERT_STORE_READONLY_FLAG ? 0 :
                     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
                    CertCloseStore(regStore, 0);
                }
            }
            /* System store doesn't need crypto provider, so close it */
            if (hCryptProv && !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                CryptReleaseContext(hCryptProv, 0);
        }
    }
    return store;
}

static WINECRYPT_CERTSTORE *CRYPT_SysOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    int len;
    WINECRYPT_CERTSTORE *ret = NULL;

    TRACE("(%Id, %08lx, %s)\n", hCryptProv, dwFlags,
     debugstr_a(pvPara));

    if (!pvPara)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    len = MultiByteToWideChar(CP_ACP, 0, pvPara, -1, NULL, 0);
    if (len)
    {
        LPWSTR storeName = CryptMemAlloc(len * sizeof(WCHAR));

        if (storeName)
        {
            MultiByteToWideChar(CP_ACP, 0, pvPara, -1, storeName, len);
            ret = CRYPT_SysOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}

static void WINAPI CRYPT_MsgCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    HCRYPTMSG msg = hCertStore;

    TRACE("(%p, %08lx)\n", msg, dwFlags);
    CryptMsgClose(msg);
}

static void *msgProvFuncs[] = {
    CRYPT_MsgCloseStore,
};

static WINECRYPT_CERTSTORE *CRYPT_MsgOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    WINECRYPT_CERTSTORE *store = NULL;
    HCRYPTMSG msg = (HCRYPTMSG)pvPara;
    WINECRYPT_CERTSTORE *memStore;

    TRACE("(%Id, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

    memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (memStore)
    {
        BOOL ret;
        DWORD size, count, i;

        size = sizeof(count);
        ret = CryptMsgGetParam(msg, CMSG_CERT_COUNT_PARAM, 0, &count, &size);
        for (i = 0; ret && i < count; i++)
        {
            size = 0;
            ret = CryptMsgGetParam(msg, CMSG_CERT_PARAM, i, NULL, &size);
            if (ret)
            {
                LPBYTE buf = CryptMemAlloc(size);

                if (buf)
                {
                    ret = CryptMsgGetParam(msg, CMSG_CERT_PARAM, i, buf, &size);
                    if (ret)
                        ret = CertAddEncodedCertificateToStore(memStore,
                         X509_ASN_ENCODING, buf, size, CERT_STORE_ADD_ALWAYS,
                         NULL);
                    CryptMemFree(buf);
                }
            }
        }
        size = sizeof(count);
        ret = CryptMsgGetParam(msg, CMSG_CRL_COUNT_PARAM, 0, &count, &size);
        for (i = 0; ret && i < count; i++)
        {
            size = 0;
            ret = CryptMsgGetParam(msg, CMSG_CRL_PARAM, i, NULL, &size);
            if (ret)
            {
                LPBYTE buf = CryptMemAlloc(size);

                if (buf)
                {
                    ret = CryptMsgGetParam(msg, CMSG_CRL_PARAM, i, buf, &size);
                    if (ret)
                        ret = CertAddEncodedCRLToStore(memStore,
                         X509_ASN_ENCODING, buf, size, CERT_STORE_ADD_ALWAYS,
                         NULL);
                    CryptMemFree(buf);
                }
            }
        }
        if (ret)
        {
            CERT_STORE_PROV_INFO provInfo = { 0 };

            provInfo.cbSize = sizeof(provInfo);
            provInfo.cStoreProvFunc = ARRAY_SIZE(msgProvFuncs);
            provInfo.rgpvStoreProvFunc = msgProvFuncs;
            provInfo.hStoreProv = CryptMsgDuplicate(msg);
            store = CRYPT_ProvCreateStore(dwFlags, memStore, &provInfo);
            /* Msg store doesn't need crypto provider, so close it */
            if (hCryptProv && !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                CryptReleaseContext(hCryptProv, 0);
        }
        else
            CertCloseStore(memStore, 0);
    }
    TRACE("returning %p\n", store);
    return store;
}

static WINECRYPT_CERTSTORE *CRYPT_PKCSOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCRYPTMSG msg;
    WINECRYPT_CERTSTORE *store = NULL;
    const CRYPT_DATA_BLOB *data = pvPara;
    BOOL ret;
    DWORD msgOpenFlags = dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG ? 0 :
     CMSG_CRYPT_RELEASE_CONTEXT_FLAG;

    TRACE("(%Id, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, msgOpenFlags, CMSG_SIGNED,
     hCryptProv, NULL, NULL);
    ret = CryptMsgUpdate(msg, data->pbData, data->cbData, TRUE);
    if (!ret)
    {
        CryptMsgClose(msg);
        msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, msgOpenFlags, 0,
         hCryptProv, NULL, NULL);
        ret = CryptMsgUpdate(msg, data->pbData, data->cbData, TRUE);
        if (ret)
        {
            DWORD type, size = sizeof(type);

            /* Only signed messages are allowed, check type */
            ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, &type, &size);
            if (ret && type != CMSG_SIGNED)
            {
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
                ret = FALSE;
            }
        }
    }
    if (ret)
        store = CRYPT_MsgOpenStore(0, dwFlags, msg);
    CryptMsgClose(msg);
    TRACE("returning %p\n", store);
    return store;
}

static WINECRYPT_CERTSTORE *CRYPT_SerializedOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCERTSTORE store;
    const CRYPT_DATA_BLOB *data = pvPara;

    TRACE("(%Id, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return NULL;
    }

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (store)
    {
        if (!CRYPT_ReadSerializedStoreFromBlob(data, store))
        {
            CertCloseStore(store, 0);
            store = NULL;
        }
    }
    TRACE("returning %p\n", store);
    return (WINECRYPT_CERTSTORE*)store;
}

static WINECRYPT_CERTSTORE *CRYPT_PhysOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
        FIXME("(%Id, %08lx, %p): stub\n", hCryptProv, dwFlags, pvPara);
    else
        FIXME("(%Id, %08lx, %s): stub\n", hCryptProv, dwFlags,
         debugstr_w(pvPara));
    return NULL;
}

HCERTSTORE WINAPI CertOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwMsgAndCertEncodingType, HCRYPTPROV_LEGACY hCryptProv, DWORD dwFlags,
 const void* pvPara)
{
    WINECRYPT_CERTSTORE *hcs;
    StoreOpenFunc openFunc = NULL;

    TRACE("(%s, %08lx, %08Ix, %08lx, %p)\n", debugstr_a(lpszStoreProvider),
          dwMsgAndCertEncodingType, hCryptProv, dwFlags, pvPara);

    if (IS_INTOID(lpszStoreProvider))
    {
        switch (LOWORD(lpszStoreProvider))
        {
        case LOWORD(CERT_STORE_PROV_MSG):
            openFunc = CRYPT_MsgOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_MEMORY):
            openFunc = CRYPT_MemOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_FILE):
            openFunc = CRYPT_FileOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_PKCS7):
            openFunc = CRYPT_PKCSOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_SERIALIZED):
            openFunc = CRYPT_SerializedOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_REG):
            openFunc = CRYPT_RegOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_FILENAME_A):
            openFunc = CRYPT_FileNameOpenStoreA;
            break;
        case LOWORD(CERT_STORE_PROV_FILENAME_W):
            openFunc = CRYPT_FileNameOpenStoreW;
            break;
        case LOWORD(CERT_STORE_PROV_COLLECTION):
            openFunc = CRYPT_CollectionOpenStore;
            break;
        case LOWORD(CERT_STORE_PROV_SYSTEM_A):
            openFunc = CRYPT_SysOpenStoreA;
            break;
        case LOWORD(CERT_STORE_PROV_SYSTEM_W):
            openFunc = CRYPT_SysOpenStoreW;
            break;
        case LOWORD(CERT_STORE_PROV_SYSTEM_REGISTRY_A):
            openFunc = CRYPT_SysRegOpenStoreA;
            break;
        case LOWORD(CERT_STORE_PROV_SYSTEM_REGISTRY_W):
            openFunc = CRYPT_SysRegOpenStoreW;
            break;
        case LOWORD(CERT_STORE_PROV_PHYSICAL_W):
            openFunc = CRYPT_PhysOpenStoreW;
            break;
        default:
            if (LOWORD(lpszStoreProvider))
                FIXME("unimplemented type %d\n", LOWORD(lpszStoreProvider));
        }
    }
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_MEMORY))
        openFunc = CRYPT_MemOpenStore;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_FILENAME_W))
        openFunc = CRYPT_FileOpenStore;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_SYSTEM))
        openFunc = CRYPT_SysOpenStoreW;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_PKCS7))
        openFunc = CRYPT_PKCSOpenStore;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_SERIALIZED))
        openFunc = CRYPT_SerializedOpenStore;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_COLLECTION))
        openFunc = CRYPT_CollectionOpenStore;
    else if (!stricmp(lpszStoreProvider, sz_CERT_STORE_PROV_SYSTEM_REGISTRY))
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
    return hcs;
}

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV_LEGACY hProv,
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

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV_LEGACY hProv,
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

PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrev)
{
    cert_t *prev = pPrev ? cert_from_ptr(pPrev) : NULL, *ret;
    WINECRYPT_CERTSTORE *hcs = hCertStore;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (cert_t*)hcs->vtbl->certs.enumContext(hcs, prev ? &prev->base : NULL);
    return ret ? &ret->ctx : NULL;
}

BOOL WINAPI CertDeleteCertificateFromStore(PCCERT_CONTEXT pCertContext)
{
    WINECRYPT_CERTSTORE *hcs;

    TRACE("(%p)\n", pCertContext);

    if (!pCertContext)
        return TRUE;

    hcs = pCertContext->hCertStore;

    if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        return FALSE;

    return hcs->vtbl->certs.delete(hcs, &cert_from_ptr(pCertContext)->base);
}

BOOL WINAPI CertAddCRLContextToStore(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pCrlContext, DWORD dwAddDisposition,
 PCCRL_CONTEXT* ppStoreContext)
{
    WINECRYPT_CERTSTORE *store = hCertStore;
    BOOL ret = TRUE;
    PCCRL_CONTEXT toAdd = NULL, existing = NULL;

    TRACE("(%p, %p, %08lx, %p)\n", hCertStore, pCrlContext,
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
    case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
        if (existing)
        {
            LONG newer = CompareFileTime(&existing->pCrlInfo->ThisUpdate,
             &pCrlContext->pCrlInfo->ThisUpdate);

            if (newer < 0)
            {
                toAdd = CertDuplicateCRLContext(pCrlContext);
                Context_CopyProperties(toAdd, existing);
            }
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
            Context_CopyProperties(toAdd, existing);
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if (existing)
        {
            Context_CopyProperties(existing, pCrlContext);
            if (ppStoreContext)
                *ppStoreContext = CertDuplicateCRLContext(existing);
        }
        else
            toAdd = CertDuplicateCRLContext(pCrlContext);
        break;
    default:
        FIXME("Unimplemented add disposition %ld\n", dwAddDisposition);
        ret = FALSE;
    }

    if (toAdd)
    {
        if (store) {
            context_t *ret_context;
            ret = store->vtbl->crls.addContext(store, context_from_ptr(toAdd),
             existing ? context_from_ptr(existing) : NULL, ppStoreContext ? &ret_context : NULL, FALSE);
            if (ret && ppStoreContext)
                *ppStoreContext = context_ptr(ret_context);
        }else if (ppStoreContext) {
            *ppStoreContext = CertDuplicateCRLContext(toAdd);
        }
        CertFreeCRLContext(toAdd);
    }
    if (existing)
        CertFreeCRLContext(existing);

    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertDeleteCRLFromStore(PCCRL_CONTEXT pCrlContext)
{
    WINECRYPT_CERTSTORE *hcs;
    BOOL ret;

    TRACE("(%p)\n", pCrlContext);

    if (!pCrlContext)
        return TRUE;

    hcs = pCrlContext->hCertStore;

    if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        return FALSE;

    ret = hcs->vtbl->crls.delete(hcs, &crl_from_ptr(pCrlContext)->base);
    if (ret)
        ret = CertFreeCRLContext(pCrlContext);
    return ret;
}

PCCRL_CONTEXT WINAPI CertEnumCRLsInStore(HCERTSTORE hCertStore, PCCRL_CONTEXT pPrev)
{
    crl_t *ret, *prev = pPrev ? crl_from_ptr(pPrev) : NULL;
    WINECRYPT_CERTSTORE *hcs = hCertStore;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (crl_t*)hcs->vtbl->crls.enumContext(hcs, prev ? &prev->base : NULL);
    return ret ? &ret->ctx : NULL;
}

HCERTSTORE WINAPI CertDuplicateStore(HCERTSTORE hCertStore)
{
    WINECRYPT_CERTSTORE *hcs = hCertStore;

    TRACE("(%p)\n", hCertStore);

    if (hcs && hcs->dwMagic == WINE_CRYPTCERTSTORE_MAGIC)
        hcs->vtbl->addref(hcs);
    return hCertStore;
}

BOOL WINAPI CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINECRYPT_CERTSTORE *hcs = hCertStore;
    DWORD res;

    TRACE("(%p, %08lx)\n", hCertStore, dwFlags);

    if( ! hCertStore )
        return TRUE;

    if ( hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC )
        return FALSE;

    res = hcs->vtbl->release(hcs, dwFlags);
    if (res != ERROR_SUCCESS) {
        SetLastError(res);
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI CertControlStore(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    WINECRYPT_CERTSTORE *hcs = hCertStore;
    BOOL ret;

    TRACE("(%p, %08lx, %ld, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    if (!hcs)
        ret = FALSE;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = FALSE;
    else
    {
        if (hcs->vtbl->control)
            ret = hcs->vtbl->control(hcs, dwFlags, dwCtrlType, pvCtrlPara);
        else
            ret = TRUE;
    }
    return ret;
}

BOOL WINAPI CertGetStoreProperty(HCERTSTORE hCertStore, DWORD dwPropId,
 void *pvData, DWORD *pcbData)
{
    WINECRYPT_CERTSTORE *store = hCertStore;
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %p, %p)\n", hCertStore, dwPropId, pvData, pcbData);

    switch (dwPropId)
    {
    case CERT_ACCESS_STATE_PROP_ID:
        if (!pvData)
        {
            *pcbData = sizeof(DWORD);
            ret = TRUE;
        }
        else if (*pcbData < sizeof(DWORD))
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbData = sizeof(DWORD);
        }
        else
        {
            DWORD state = 0;

            if (store->type != StoreTypeMem &&
             !(store->dwOpenFlags & CERT_STORE_READONLY_FLAG))
                state |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
            *(DWORD *)pvData = state;
            ret = TRUE;
        }
        break;
    default:
        if (store->properties)
        {
            CRYPT_DATA_BLOB blob;

            ret = ContextPropertyList_FindProperty(store->properties, dwPropId,
             &blob);
            if (ret)
            {
                if (!pvData)
                    *pcbData = blob.cbData;
                else if (*pcbData < blob.cbData)
                {
                    SetLastError(ERROR_MORE_DATA);
                    *pcbData = blob.cbData;
                    ret = FALSE;
                }
                else
                {
                    memcpy(pvData, blob.pbData, blob.cbData);
                    *pcbData = blob.cbData;
                }
            }
            else
                SetLastError(CRYPT_E_NOT_FOUND);
        }
        else
            SetLastError(CRYPT_E_NOT_FOUND);
    }
    return ret;
}

BOOL WINAPI CertSetStoreProperty(HCERTSTORE hCertStore, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    WINECRYPT_CERTSTORE *store = hCertStore;
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %08lx, %p)\n", hCertStore, dwPropId, dwFlags, pvData);

    if (!store->properties)
        store->properties = ContextPropertyList_Create();
    switch (dwPropId)
    {
    case CERT_ACCESS_STATE_PROP_ID:
        SetLastError(E_INVALIDARG);
        break;
    default:
        if (pvData)
        {
            const CRYPT_DATA_BLOB *blob = pvData;

            ret = ContextPropertyList_SetProperty(store->properties, dwPropId,
             blob->pbData, blob->cbData);
        }
        else
        {
            ContextPropertyList_RemoveProperty(store->properties, dwPropId);
            ret = TRUE;
        }
    }
    return ret;
}

static LONG CRYPT_OpenParentStore(DWORD dwFlags,
    void *pvSystemStoreLocationPara, HKEY *key)
{
    HKEY root;
    LPCWSTR base;

    TRACE("(%08lx, %p)\n", dwFlags, pvSystemStoreLocationPara);

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
        FIXME("CERT_SYSTEM_STORE_CURRENT_SERVICE\n");
        return ERROR_FILE_NOT_FOUND;
    case CERT_SYSTEM_STORE_SERVICES:
        /* hklm\Software\Microsoft\Cryptography\Services\servicename\
         * SystemCertificates
         */
        FIXME("CERT_SYSTEM_STORE_SERVICES\n");
        return ERROR_FILE_NOT_FOUND;
    case CERT_SYSTEM_STORE_USERS:
        /* hku\user sid\Software\Microsoft\SystemCertificates */
        FIXME("CERT_SYSTEM_STORE_USERS\n");
        return ERROR_FILE_NOT_FOUND;
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
        FIXME("CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE\n");
        return ERROR_FILE_NOT_FOUND;
    default:
        return ERROR_FILE_NOT_FOUND;
    }

    return RegOpenKeyExW(root, base, 0, KEY_READ, key);
}

BOOL WINAPI CertEnumSystemStore(DWORD dwFlags, void *pvSystemStoreLocationPara,
    void *pvArg, PFN_CERT_ENUM_SYSTEM_STORE pfnEnum)
{
    BOOL ret = FALSE;
    LONG rc;
    HKEY key;
    CERT_SYSTEM_STORE_INFO info = { sizeof(info) };

    TRACE("(%08lx, %p, %p, %p)\n", dwFlags, pvSystemStoreLocationPara, pvArg,
        pfnEnum);

    rc = CRYPT_OpenParentStore(dwFlags, pvArg, &key);
    if (!rc)
    {
        DWORD index = 0;

        ret = TRUE;
        do {
            WCHAR name[MAX_PATH];
            DWORD size = ARRAY_SIZE(name);

            rc = RegEnumKeyExW(key, index++, name, &size, NULL, NULL, NULL,
                NULL);
            if (!rc)
                ret = pfnEnum(name, dwFlags, &info, NULL, pvArg);
        } while (ret && !rc);
        if (ret && rc != ERROR_NO_MORE_ITEMS)
            SetLastError(rc);
    }
    else
        SetLastError(rc);
    /* Include root store for the local machine location (it isn't in the
     * registry)
     */
    if (ret && (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) ==
     CERT_SYSTEM_STORE_LOCAL_MACHINE)
        ret = pfnEnum(L"Root", dwFlags, &info, NULL, pvArg);
    return ret;
}

BOOL WINAPI CertEnumPhysicalStore(const void *pvSystemStore, DWORD dwFlags,
 void *pvArg, PFN_CERT_ENUM_PHYSICAL_STORE pfnEnum)
{
    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
        FIXME("(%p, %08lx, %p, %p): stub\n", pvSystemStore, dwFlags, pvArg,
         pfnEnum);
    else
        FIXME("(%s, %08lx, %p, %p): stub\n", debugstr_w(pvSystemStore),
         dwFlags, pvArg,
         pfnEnum);
    return FALSE;
}

BOOL WINAPI CertRegisterPhysicalStore(const void *pvSystemStore, DWORD dwFlags,
 LPCWSTR pwszStoreName, PCERT_PHYSICAL_STORE_INFO pStoreInfo, void *pvReserved)
{
    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
        FIXME("(%p, %08lx, %s, %p, %p): stub\n", pvSystemStore, dwFlags,
         debugstr_w(pwszStoreName), pStoreInfo, pvReserved);
    else
        FIXME("(%s, %08lx, %s, %p, %p): stub\n", debugstr_w(pvSystemStore),
         dwFlags, debugstr_w(pwszStoreName), pStoreInfo, pvReserved);
    return FALSE;
}

BOOL WINAPI CertUnregisterPhysicalStore(const void *pvSystemStore, DWORD dwFlags,
 LPCWSTR pwszStoreName)
{
    FIXME("(%p, %08lx, %s): stub\n", pvSystemStore, dwFlags, debugstr_w(pwszStoreName));
    return TRUE;
}

BOOL WINAPI CertRegisterSystemStore(const void *pvSystemStore, DWORD dwFlags,
  PCERT_SYSTEM_STORE_INFO pStoreInfo, void *pvReserved)
{
    HCERTSTORE hstore;

    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG )
    {
        FIXME("(%p, %08lx, %p, %p): flag not supported\n", pvSystemStore, dwFlags, pStoreInfo, pvReserved);
        return FALSE;
    }

    TRACE("(%s, %08lx, %p, %p)\n", debugstr_w(pvSystemStore), dwFlags, pStoreInfo, pvReserved);

    hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, dwFlags, pvSystemStore);
    if (hstore)
    {
        CertCloseStore(hstore, 0);
        return TRUE;
    }

    return FALSE;
}

BOOL WINAPI CertUnregisterSystemStore(const void *pvSystemStore, DWORD dwFlags)
{
    HCERTSTORE hstore;

    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
    {
        FIXME("(%p, %08lx): flag not supported\n", pvSystemStore, dwFlags);
        return FALSE;
    }
    TRACE("(%s, %08lx)\n", debugstr_w(pvSystemStore), dwFlags);

    hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, dwFlags | CERT_STORE_OPEN_EXISTING_FLAG, pvSystemStore);
    if (hstore == NULL)
        return FALSE;

    hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, dwFlags | CERT_STORE_DELETE_FLAG, pvSystemStore);
    if (hstore == NULL && GetLastError() == 0)
        return TRUE;

    return FALSE;
}

static void EmptyStore_addref(WINECRYPT_CERTSTORE *store)
{
    TRACE("(%p)\n", store);
}

static DWORD EmptyStore_release(WINECRYPT_CERTSTORE *store, DWORD flags)
{
    TRACE("(%p)\n", store);
    return E_UNEXPECTED;
}

static void EmptyStore_releaseContext(WINECRYPT_CERTSTORE *store, context_t *context)
{
    Context_Free(context);
}

static BOOL EmptyStore_add(WINECRYPT_CERTSTORE *store, context_t *context,
 context_t *replace, context_t **ret_context, BOOL use_link)
{
    TRACE("(%p, %p, %p, %p)\n", store, context, replace, ret_context);

    /* FIXME: We should clone the context */
    if(ret_context) {
        Context_AddRef(context);
        *ret_context = context;
    }

    return TRUE;
}

static context_t *EmptyStore_enum(WINECRYPT_CERTSTORE *store, context_t *prev)
{
    TRACE("(%p, %p)\n", store, prev);

    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

static BOOL EmptyStore_delete(WINECRYPT_CERTSTORE *store, context_t *context)
{
    return TRUE;
}

static BOOL EmptyStore_control(WINECRYPT_CERTSTORE *store, DWORD flags, DWORD ctrl_type, void const *ctrl_para)
{
    TRACE("()\n");

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static const store_vtbl_t EmptyStoreVtbl = {
    EmptyStore_addref,
    EmptyStore_release,
    EmptyStore_releaseContext,
    EmptyStore_control,
    {
        EmptyStore_add,
        EmptyStore_enum,
        EmptyStore_delete
    }, {
        EmptyStore_add,
        EmptyStore_enum,
        EmptyStore_delete
    }, {
        EmptyStore_add,
        EmptyStore_enum,
        EmptyStore_delete
    }
};

WINECRYPT_CERTSTORE empty_store;

void init_empty_store(void)
{
    CRYPT_InitStore(&empty_store, CERT_STORE_READONLY_FLAG, StoreTypeEmpty, &EmptyStoreVtbl);
}
