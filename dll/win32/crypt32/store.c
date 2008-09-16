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
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

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

typedef struct _WINE_MEMSTORE
{
    WINECRYPT_CERTSTORE hdr;
    struct ContextList *certs;
    struct ContextList *crls;
    struct ContextList *ctls;
} WINE_MEMSTORE, *PWINE_MEMSTORE;

void CRYPT_InitStore(WINECRYPT_CERTSTORE *store, DWORD dwFlags,
 CertStoreType type)
{
    store->ref = 1;
    store->dwMagic = WINE_CRYPTCERTSTORE_MAGIC;
    store->type = type;
    store->dwOpenFlags = dwFlags;
    store->properties = NULL;
}

void CRYPT_FreeStore(PWINECRYPT_CERTSTORE store)
{
    if (store->properties)
        ContextPropertyList_Free(store->properties);
    CryptMemFree(store);
}

BOOL WINAPI I_CertUpdateStore(HCERTSTORE store1, HCERTSTORE store2, DWORD unk0,
 DWORD unk1)
{
    static BOOL warned = FALSE;
    const WINE_CONTEXT_INTERFACE * const interfaces[] = { pCertInterface,
     pCRLInterface, pCTLInterface };
    DWORD i;

    TRACE("(%p, %p, %08x, %08x)\n", store1, store2, unk0, unk1);
    if (!warned)
    {
        FIXME("semi-stub\n");
        warned = TRUE;
    }

    /* Poor-man's resync:  empty first store, then add everything from second
     * store to it.
     */
    for (i = 0; i < sizeof(interfaces) / sizeof(interfaces[0]); i++)
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

static BOOL CRYPT_MemAddCtl(PWINECRYPT_CERTSTORE store, void *ctl,
 void *toReplace, const void **ppStoreContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    PCTL_CONTEXT context;

    TRACE("(%p, %p, %p, %p)\n", store, ctl, toReplace, ppStoreContext);

    context = (PCTL_CONTEXT)ContextList_Add(ms->ctls, ctl, toReplace);
    if (context)
    {
        context->hCertStore = store;
        if (ppStoreContext)
            *ppStoreContext = CertDuplicateCTLContext(context);
    }
    return context ? TRUE : FALSE;
}

static void *CRYPT_MemEnumCtl(PWINECRYPT_CERTSTORE store, void *pPrev)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;
    void *ret;

    TRACE("(%p, %p)\n", store, pPrev);

    ret = ContextList_Enum(ms->ctls, pPrev);
    if (!ret)
        SetLastError(CRYPT_E_NOT_FOUND);

    TRACE("returning %p\n", ret);
    return ret;
}

static BOOL CRYPT_MemDeleteCtl(PWINECRYPT_CERTSTORE store, void *pCtlContext)
{
    WINE_MEMSTORE *ms = (WINE_MEMSTORE *)store;

    ContextList_Delete(ms->ctls, pCtlContext);
    return TRUE;
}

static void WINAPI CRYPT_MemCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINE_MEMSTORE *store = (WINE_MEMSTORE *)hCertStore;

    TRACE("(%p, %08x)\n", store, dwFlags);
    if (dwFlags)
        FIXME("Unimplemented flags: %08x\n", dwFlags);

    ContextList_Free(store->certs);
    ContextList_Free(store->crls);
    ContextList_Free(store->ctls);
    CRYPT_FreeStore((PWINECRYPT_CERTSTORE)store);
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
            CRYPT_InitStore(&store->hdr, dwFlags, StoreTypeMem);
            store->hdr.closeStore          = CRYPT_MemCloseStore;
            store->hdr.certs.addContext    = CRYPT_MemAddCert;
            store->hdr.certs.enumContext   = CRYPT_MemEnumCert;
            store->hdr.certs.deleteContext = CRYPT_MemDeleteCert;
            store->hdr.crls.addContext     = CRYPT_MemAddCrl;
            store->hdr.crls.enumContext    = CRYPT_MemEnumCrl;
            store->hdr.crls.deleteContext  = CRYPT_MemDeleteCrl;
            store->hdr.ctls.addContext     = CRYPT_MemAddCtl;
            store->hdr.ctls.enumContext    = CRYPT_MemEnumCtl;
            store->hdr.ctls.deleteContext  = CRYPT_MemDeleteCtl;
            store->hdr.control             = NULL;
            store->certs = ContextList_Create(pCertInterface,
             sizeof(CERT_CONTEXT));
            store->crls = ContextList_Create(pCRLInterface,
             sizeof(CRL_CONTEXT));
            store->ctls = ContextList_Create(pCTLInterface,
             sizeof(CTL_CONTEXT));
            /* Mem store doesn't need crypto provider, so close it */
            if (hCryptProv && !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                CryptReleaseContext(hCryptProv, 0);
        }
    }
    return (PWINECRYPT_CERTSTORE)store;
}

static PWINECRYPT_CERTSTORE CRYPT_SysRegOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    static const WCHAR rootW[] = { 'R','o','o','t',0 };
    static const WCHAR fmt[] = { '%','s','\\','%','s',0 };
    LPCWSTR storeName = (LPCWSTR)pvPara;
    LPWSTR storePath;
    PWINECRYPT_CERTSTORE store = NULL;
    HKEY root;
    LPCWSTR base;

    TRACE("(%ld, %08x, %s)\n", hCryptProv, dwFlags,
     debugstr_w((LPCWSTR)pvPara));

    if (!pvPara)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (!lstrcmpiW(storeName, rootW))
        return CRYPT_RootOpenStore(hCryptProv, dwFlags);

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

static void WINAPI CRYPT_MsgCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    HCRYPTMSG msg = hCertStore;

    TRACE("(%p, %08x)\n", msg, dwFlags);
    CryptMsgClose(msg);
}

static void *msgProvFuncs[] = {
    CRYPT_MsgCloseStore,
};

static PWINECRYPT_CERTSTORE CRYPT_MsgOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    PWINECRYPT_CERTSTORE store = NULL;
    HCRYPTMSG msg = (HCRYPTMSG)pvPara;
    PWINECRYPT_CERTSTORE memStore;

    TRACE("(%ld, %08x, %p)\n", hCryptProv, dwFlags, pvPara);

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
            provInfo.cStoreProvFunc = sizeof(msgProvFuncs) /
             sizeof(msgProvFuncs[0]);
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

static PWINECRYPT_CERTSTORE CRYPT_PKCSOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCRYPTMSG msg;
    PWINECRYPT_CERTSTORE store = NULL;
    const CRYPT_DATA_BLOB *data = (const CRYPT_DATA_BLOB *)pvPara;
    BOOL ret;
    DWORD msgOpenFlags = dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG ? 0 :
     CMSG_CRYPT_RELEASE_CONTEXT_FLAG;

    TRACE("(%ld, %08x, %p)\n", hCryptProv, dwFlags, pvPara);

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
 DWORD dwMsgAndCertEncodingType, HCRYPTPROV_LEGACY hCryptProv, DWORD dwFlags,
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
        {
            CertContext_CopyProperties(existing, pCertContext);
            *ppStoreContext = CertDuplicateCertificateContext(existing);
        }
        else
            toAdd = CertDuplicateCertificateContext(pCertContext);
        break;
    case CERT_STORE_ADD_NEWER:
        if (existing)
        {
            if (CompareFileTime(&existing->pCertInfo->NotBefore,
             &pCertContext->pCertInfo->NotBefore) >= 0)
            {
                TRACE("existing certificate is newer, not adding\n");
                SetLastError(CRYPT_E_EXISTS);
                ret = FALSE;
            }
            else
                toAdd = CertDuplicateCertificateContext(pCertContext);
        }
        else
            toAdd = CertDuplicateCertificateContext(pCertContext);
        break;
    default:
        FIXME("Unimplemented add disposition %d\n", dwAddDisposition);
        SetLastError(E_INVALIDARG);
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

BOOL WINAPI CertGetStoreProperty(HCERTSTORE hCertStore, DWORD dwPropId,
 void *pvData, DWORD *pcbData)
{
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    BOOL ret = FALSE;

    TRACE("(%p, %d, %p, %p)\n", hCertStore, dwPropId, pvData, pcbData);

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
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    BOOL ret = FALSE;

    TRACE("(%p, %d, %08x, %p)\n", hCertStore, dwPropId, dwFlags, pvData);

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
            const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvData;

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

    TRACE("(%08x, %p)\n", dwFlags, pvSystemStoreLocationPara);

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

    TRACE("(%08x, %p, %p, %p)\n", dwFlags, pvSystemStoreLocationPara, pvArg,
        pfnEnum);

    rc = CRYPT_OpenParentStore(dwFlags, pvArg, &key);
    if (!rc)
    {
        DWORD index = 0;
        CERT_SYSTEM_STORE_INFO info = { sizeof(info) };

        ret = TRUE;
        do {
            WCHAR name[MAX_PATH];
            DWORD size = sizeof(name) / sizeof(name[0]);

            rc = RegEnumKeyExW(key, index++, name, &size, NULL, NULL, NULL,
                NULL);
            if (!rc)
                ret = pfnEnum(name, 0, &info, NULL, pvArg);
        } while (ret && !rc);
        if (ret && rc != ERROR_NO_MORE_ITEMS)
            SetLastError(rc);
    }
    else
        SetLastError(rc);
    return ret;
}
