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
#include "winnls.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef struct _WINE_FILESTOREINFO
{
    DWORD      dwOpenFlags;
    HCERTSTORE memStore;
    HANDLE     file;
    DWORD      type;
    BOOL       dirty;
} WINE_FILESTOREINFO;

static void WINAPI CRYPT_FileCloseStore(HCERTSTORE hCertStore, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %08lx)\n", store, dwFlags);
    if (store->dirty)
        CertSaveStore(store->memStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
         store->type, CERT_STORE_SAVE_TO_FILE, store->file, 0);
    CloseHandle(store->file);
    CryptMemFree(store);
}

static BOOL WINAPI CRYPT_FileWriteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT cert, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %ld)\n", hCertStore, cert, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %08lx)\n", hCertStore, pCertContext, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileWriteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT crl, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %ld)\n", hCertStore, crl, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileDeleteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %08lx)\n", hCertStore, pCrlContext, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileWriteCTL(HCERTSTORE hCertStore,
 PCCTL_CONTEXT ctl, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %ld)\n", hCertStore, ctl, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL WINAPI CRYPT_FileDeleteCTL(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pCtlContext, DWORD dwFlags)
{
    WINE_FILESTOREINFO *store = hCertStore;

    TRACE("(%p, %p, %08lx)\n", hCertStore, pCtlContext, dwFlags);
    store->dirty = TRUE;
    return TRUE;
}

static BOOL CRYPT_ReadBlobFromFile(HANDLE file, PCERT_BLOB blob)
{
    BOOL ret = TRUE;

    blob->cbData = GetFileSize(file, NULL);
    if (blob->cbData)
    {
        blob->pbData = CryptMemAlloc(blob->cbData);
        if (blob->pbData)
        {
            DWORD read;

            ret = ReadFile(file, blob->pbData, blob->cbData, &read, NULL) && read == blob->cbData;
            if (!ret) CryptMemFree(blob->pbData);
        }
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_FileControl(HCERTSTORE hCertStore, DWORD dwFlags,
 DWORD dwCtrlType, void const *pvCtrlPara)
{
    WINE_FILESTOREINFO *store = hCertStore;
    BOOL ret;

    TRACE("(%p, %08lx, %ld, %p)\n", hCertStore, dwFlags, dwCtrlType,
     pvCtrlPara);

    switch (dwCtrlType)
    {
    case CERT_STORE_CTRL_RESYNC:
        store->dirty = FALSE;
        if (store->type == CERT_STORE_SAVE_AS_STORE)
        {
            HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);

            /* FIXME: if I could translate a handle to a path, I could use
             * CryptQueryObject instead, but there's no API to do so yet.
             */
            ret = CRYPT_ReadSerializedStoreFromFile(store->file, memStore);
            if (ret)
                I_CertUpdateStore(store->memStore, memStore, 0, 0);
            CertCloseStore(memStore, 0);
        }
        else if (store->type == CERT_STORE_SAVE_AS_PKCS7)
        {
            CERT_BLOB blob = { 0, NULL };

            ret = CRYPT_ReadBlobFromFile(store->file, &blob);
            if (ret)
            {
                HCERTSTORE messageStore;

                ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
                 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                 CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL,
                 &messageStore, NULL, NULL);
                I_CertUpdateStore(store->memStore, messageStore, 0, 0);
                CertCloseStore(messageStore, 0);
                CryptMemFree(blob.pbData);
            }
        }
        else
        {
            WARN("unknown type %ld\n", store->type);
            ret = FALSE;
        }
        break;
    case CERT_STORE_CTRL_COMMIT:
        if (!(store->dwOpenFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG))
        {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            ret = FALSE;
        }
        else if (store->dirty)
            ret = CertSaveStore(store->memStore,
             X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
             store->type, CERT_STORE_SAVE_TO_FILE, store->file, 0);
        else
            ret = TRUE;
        break;
    default:
        FIXME("%ld: stub\n", dwCtrlType);
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
    CRYPT_FileWriteCTL,
    CRYPT_FileDeleteCTL,
    NULL, /* CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC */
    CRYPT_FileControl,
};

static WINECRYPT_CERTSTORE *CRYPT_CreateFileStore(DWORD dwFlags,
 HCERTSTORE memStore, HANDLE file, DWORD type)
{
    WINECRYPT_CERTSTORE *store = NULL;
    WINE_FILESTOREINFO *info = CryptMemAlloc(sizeof(WINE_FILESTOREINFO));

    if (info)
    {
        CERT_STORE_PROV_INFO provInfo = { 0 };

        info->dwOpenFlags = dwFlags;
        info->memStore = memStore;
        info->file = file;
        info->type = type;
        info->dirty = FALSE;
        provInfo.cbSize = sizeof(provInfo);
        provInfo.cStoreProvFunc = ARRAY_SIZE(fileProvFuncs);
        provInfo.rgpvStoreProvFunc = fileProvFuncs;
        provInfo.hStoreProv = info;
        store = CRYPT_ProvCreateStore(dwFlags, memStore, &provInfo);
    }
    return store;
}

WINECRYPT_CERTSTORE *CRYPT_FileOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara)
{
    WINECRYPT_CERTSTORE *store = NULL;
    HANDLE file = (HANDLE)pvPara;

    TRACE("(%Id, %08lx, %p)\n", hCryptProv, dwFlags, pvPara);

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
        HCERTSTORE memStore;

        memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (memStore)
        {
            if (CRYPT_ReadSerializedStoreFromFile(file, memStore))
            {
                store = CRYPT_CreateFileStore(dwFlags, memStore, file,
                 CERT_STORE_SAVE_AS_STORE);
                /* File store doesn't need crypto provider, so close it */
                if (hCryptProv &&
                 !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                    CryptReleaseContext(hCryptProv, 0);
            }
        }
    }
    TRACE("returning %p\n", store);
    return store;
}

WINECRYPT_CERTSTORE *CRYPT_FileNameOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara)
{
    HCERTSTORE store = 0;
    LPCWSTR fileName = pvPara;
    DWORD access, create;
    HANDLE file;

    TRACE("(%Id, %08lx, %s)\n", hCryptProv, dwFlags, debugstr_w(fileName));

    if (!fileName)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return NULL;
    }
    if ((dwFlags & CERT_STORE_READONLY_FLAG) &&
     (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG))
    {
        SetLastError(E_INVALIDARG);
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
        HCERTSTORE memStore = NULL;
        DWORD size = GetFileSize(file, NULL), type = 0;

        /* If the file isn't empty, try to get the type from the file itself */
        if (size)
        {
            DWORD contentType;
            BOOL ret;

            /* Close the file so CryptQueryObject can succeed.. */
            CloseHandle(file);
            ret = CryptQueryObject(CERT_QUERY_OBJECT_FILE, fileName,
             CERT_QUERY_CONTENT_FLAG_CERT |
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
             CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
             CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, &contentType, NULL,
             &memStore, NULL, NULL);
            if (ret)
            {
                if (contentType == CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED)
                    type = CERT_STORE_SAVE_AS_PKCS7;
                else
                    type = CERT_STORE_SAVE_AS_STORE;
                /* and reopen the file. */
                file = CreateFileW(fileName, access, FILE_SHARE_READ, NULL,
                 create, FILE_ATTRIBUTE_NORMAL, NULL);
            }
        }
        else
        {
            LPCWSTR ext = wcsrchr(fileName, '.');

            if (ext)
            {
                ext++;
                if (!lstrcmpiW(ext, L"spc") || !lstrcmpiW(ext, L"p7c"))
                    type = CERT_STORE_SAVE_AS_PKCS7;
            }
            if (!type)
                type = CERT_STORE_SAVE_AS_STORE;
            memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);
        }
        if (memStore)
        {
            store = CRYPT_CreateFileStore(dwFlags, memStore, file, type);
            /* File store doesn't need crypto provider, so close it */
            if (hCryptProv && !(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                CryptReleaseContext(hCryptProv, 0);
        }
    }
    return store;
}

WINECRYPT_CERTSTORE *CRYPT_FileNameOpenStoreA(HCRYPTPROV hCryptProv,
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
            ret = CRYPT_FileNameOpenStoreW(hCryptProv, dwFlags, storeName);
            CryptMemFree(storeName);
        }
    }
    return ret;
}
