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
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* An extended certificate property in serialized form is prefixed by this
 * header.
 */
typedef struct _WINE_CERT_PROP_HEADER
{
    DWORD propID;
    DWORD unknown; /* always 1 */
    DWORD cb;
} WINE_CERT_PROP_HEADER;

struct store_CRYPT_KEY_PROV_INFO
{
    DWORD pwszContainerName;
    DWORD pwszProvName;
    DWORD dwProvType;
    DWORD dwFlags;
    DWORD cProvParam;
    DWORD rgProvParam;
    DWORD dwKeySpec;
};

struct store_CRYPT_KEY_PROV_PARAM
{
    DWORD dwParam;
    DWORD pbData;
    DWORD cbData;
    DWORD dwFlags;
};

static DWORD serialize_KeyProvInfoProperty(const CRYPT_KEY_PROV_INFO *info, struct store_CRYPT_KEY_PROV_INFO **ret)
{
    struct store_CRYPT_KEY_PROV_INFO *store;
    struct store_CRYPT_KEY_PROV_PARAM *param;
    DWORD size = sizeof(struct store_CRYPT_KEY_PROV_INFO), i;
    BYTE *data;

    if (info->pwszContainerName)
        size += (lstrlenW(info->pwszContainerName) + 1) * sizeof(WCHAR);
    if (info->pwszProvName)
        size += (lstrlenW(info->pwszProvName) + 1) * sizeof(WCHAR);

    for (i = 0; i < info->cProvParam; i++)
        size += sizeof(struct store_CRYPT_KEY_PROV_PARAM) + info->rgProvParam[i].cbData;

    if (!ret) return size;

    store = CryptMemAlloc(size);
    if (!store) return 0;

    param = (struct store_CRYPT_KEY_PROV_PARAM *)(store + 1);
    data = (BYTE *)param + sizeof(struct store_CRYPT_KEY_PROV_PARAM) * info->cProvParam;

    if (info->pwszContainerName)
    {
        store->pwszContainerName = data - (BYTE *)store;
        lstrcpyW((LPWSTR)data, info->pwszContainerName);
        data += (lstrlenW(info->pwszContainerName) + 1) * sizeof(WCHAR);
    }
    else
        store->pwszContainerName = 0;

    if (info->pwszProvName)
    {
        store->pwszProvName = data - (BYTE *)store;
        lstrcpyW((LPWSTR)data, info->pwszProvName);
        data += (lstrlenW(info->pwszProvName) + 1) * sizeof(WCHAR);
    }
    else
        store->pwszProvName = 0;

    store->dwProvType = info->dwProvType;
    store->dwFlags = info->dwFlags;
    store->cProvParam = info->cProvParam;
    store->rgProvParam = info->cProvParam ? (BYTE *)param - (BYTE *)store : 0;
    store->dwKeySpec = info->dwKeySpec;

    for (i = 0; i < info->cProvParam; i++)
    {
        param[i].dwParam = info->rgProvParam[i].dwParam;
        param[i].dwFlags = info->rgProvParam[i].dwFlags;
        param[i].cbData = info->rgProvParam[i].cbData;
        param[i].pbData = param[i].cbData ? data - (BYTE *)store : 0;
        memcpy(data, info->rgProvParam[i].pbData, info->rgProvParam[i].cbData);
        data += info->rgProvParam[i].cbData;
    }

    *ret = store;
    return size;
}

static BOOL CRYPT_SerializeStoreElement(const void *context,
 const BYTE *encodedContext, DWORD cbEncodedContext, DWORD contextPropID,
 const WINE_CONTEXT_INTERFACE *contextInterface, DWORD dwFlags, BOOL omitHashes,
 BYTE *pbElement, DWORD *pcbElement)
{
    BOOL ret;

    TRACE("(%p, %p, %08lx, %d, %p, %p)\n", context, contextInterface, dwFlags,
     omitHashes, pbElement, pcbElement);

    if (context)
    {
        DWORD bytesNeeded = sizeof(WINE_CERT_PROP_HEADER) + cbEncodedContext;
        DWORD prop = 0;

        ret = TRUE;
        do {
            prop = contextInterface->enumProps(context, prop);
            if (prop && (!omitHashes || !IS_CERT_HASH_PROP_ID(prop)))
            {
                DWORD propSize = 0;

                ret = contextInterface->getProp(context, prop, NULL, &propSize);
                if (ret)
                {
                    if (prop == CERT_KEY_PROV_INFO_PROP_ID)
                    {
                        BYTE *info = CryptMemAlloc(propSize);
                        contextInterface->getProp(context, prop, info, &propSize);
                        propSize = serialize_KeyProvInfoProperty((const CRYPT_KEY_PROV_INFO *)info, NULL);
                        CryptMemFree(info);
                    }
                    bytesNeeded += sizeof(WINE_CERT_PROP_HEADER) + propSize;
                }
            }
        } while (ret && prop != 0);

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
            WINE_CERT_PROP_HEADER *hdr;
            DWORD bufSize = 0;
            LPBYTE buf = NULL;

            prop = 0;
            do {
                prop = contextInterface->enumProps(context, prop);
                if (prop && (!omitHashes || !IS_CERT_HASH_PROP_ID(prop)))
                {
                    DWORD propSize = 0;

                    ret = contextInterface->getProp(context, prop, NULL,
                     &propSize);
                    if (ret)
                    {
                        if (bufSize < propSize)
                        {
                            if (buf)
                                buf = CryptMemRealloc(buf, propSize);
                            else
                                buf = CryptMemAlloc(propSize);
                            bufSize = propSize;
                        }
                        if (buf)
                        {
                            ret = contextInterface->getProp(context, prop, buf,
                             &propSize);
                            if (ret)
                            {
                                if (prop == CERT_KEY_PROV_INFO_PROP_ID)
                                {
                                    struct store_CRYPT_KEY_PROV_INFO *store;
                                    propSize = serialize_KeyProvInfoProperty((const CRYPT_KEY_PROV_INFO *)buf, &store);
                                    CryptMemFree(buf);
                                    buf = (BYTE *)store;
                                }

                                hdr = (WINE_CERT_PROP_HEADER*)pbElement;
                                hdr->propID = prop;
                                hdr->unknown = 1;
                                hdr->cb = propSize;
                                pbElement += sizeof(WINE_CERT_PROP_HEADER);
                                if (propSize)
                                {
                                    memcpy(pbElement, buf, propSize);
                                    pbElement += propSize;
                                }
                            }
                        }
                        else
                            ret = FALSE;
                    }
                }
            } while (ret && prop != 0);
            CryptMemFree(buf);

            hdr = (WINE_CERT_PROP_HEADER*)pbElement;
            hdr->propID = contextPropID;
            hdr->unknown = 1;
            hdr->cb = cbEncodedContext;
            memcpy(pbElement + sizeof(WINE_CERT_PROP_HEADER),
             encodedContext, cbEncodedContext);
        }
    }
    else
        ret = FALSE;
    return ret;
}

BOOL WINAPI CertSerializeCertificateStoreElement(PCCERT_CONTEXT pCertContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCertContext,
     pCertContext->pbCertEncoded, pCertContext->cbCertEncoded,
     CERT_CERT_PROP_ID, pCertInterface, dwFlags, FALSE, pbElement, pcbElement);
}

BOOL WINAPI CertSerializeCRLStoreElement(PCCRL_CONTEXT pCrlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCrlContext,
     pCrlContext->pbCrlEncoded, pCrlContext->cbCrlEncoded,
     CERT_CRL_PROP_ID, pCRLInterface, dwFlags, FALSE, pbElement, pcbElement);
}

BOOL WINAPI CertSerializeCTLStoreElement(PCCTL_CONTEXT pCtlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCtlContext,
     pCtlContext->pbCtlEncoded, pCtlContext->cbCtlEncoded,
     CERT_CTL_PROP_ID, pCTLInterface, dwFlags, FALSE, pbElement, pcbElement);
}

/* Looks for the property with ID propID in the buffer buf.  Returns a pointer
 * to its header if a valid header is found, NULL if not.  Valid means the
 * length of the property won't overrun buf, and the unknown field is 1.
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
                SetLastError(E_INVALIDARG);
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

static DWORD read_serialized_KeyProvInfoProperty(const struct store_CRYPT_KEY_PROV_INFO *store, CRYPT_KEY_PROV_INFO **ret)
{
    const struct store_CRYPT_KEY_PROV_PARAM *param;
    CRYPT_KEY_PROV_INFO *info;
    DWORD size = sizeof(CRYPT_KEY_PROV_INFO), i;
    const BYTE *base;
    BYTE *data;

    base = (const BYTE *)store;
    param = (const struct store_CRYPT_KEY_PROV_PARAM *)(base + store->rgProvParam);

    if (store->pwszContainerName)
        size += (lstrlenW((LPCWSTR)(base + store->pwszContainerName)) + 1) * sizeof(WCHAR);
    if (store->pwszProvName)
        size += (lstrlenW((LPCWSTR)(base + store->pwszProvName)) + 1) * sizeof(WCHAR);

    for (i = 0; i < store->cProvParam; i++)
        size += sizeof(CRYPT_KEY_PROV_PARAM) + param[i].cbData;

    info = CryptMemAlloc(size);
    if (!info)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    data = (BYTE *)(info + 1) + sizeof(CRYPT_KEY_PROV_PARAM) * store->cProvParam;

    if (store->pwszContainerName)
    {
        info->pwszContainerName = (LPWSTR)data;
        lstrcpyW(info->pwszContainerName, (LPCWSTR)((const BYTE *)store + store->pwszContainerName));
        data += (lstrlenW(info->pwszContainerName) + 1) * sizeof(WCHAR);
    }
    else
        info->pwszContainerName = NULL;

    if (store->pwszProvName)
    {
        info->pwszProvName = (LPWSTR)data;
        lstrcpyW(info->pwszProvName, (LPCWSTR)((const BYTE *)store + store->pwszProvName));
        data += (lstrlenW(info->pwszProvName) + 1) * sizeof(WCHAR);
    }
    else
        info->pwszProvName = NULL;

    info->dwProvType = store->dwProvType;
    info->dwFlags = store->dwFlags;
    info->dwKeySpec = store->dwKeySpec;
    info->cProvParam = store->cProvParam;

    if (info->cProvParam)
    {
        DWORD i;

        info->rgProvParam = (CRYPT_KEY_PROV_PARAM *)(info + 1);

        for (i = 0; i < info->cProvParam; i++)
        {
            info->rgProvParam[i].dwParam = param[i].dwParam;
            info->rgProvParam[i].dwFlags = param[i].dwFlags;
            info->rgProvParam[i].cbData = param[i].cbData;
            info->rgProvParam[i].pbData = param[i].cbData ? data : NULL;
            memcpy(info->rgProvParam[i].pbData, base + param[i].pbData, param[i].cbData);
            data += param[i].cbData;
        }
    }
    else
        info->rgProvParam = NULL;

    TRACE("%s,%s,%lu,%08lx,%lu,%p,%lu\n", debugstr_w(info->pwszContainerName), debugstr_w(info->pwszProvName),
          info->dwProvType, info->dwFlags, info->cProvParam, info->rgProvParam, info->dwKeySpec);

    *ret = info;
    return size;
}

static BOOL CRYPT_ReadContextProp(
 const WINE_CONTEXT_INTERFACE *contextInterface, const void *context,
 const WINE_CERT_PROP_HEADER *hdr, const BYTE *pbElement, DWORD cbElement)
{
    BOOL ret;

    if (cbElement < hdr->cb)
    {
        SetLastError(E_INVALIDARG);
        ret = FALSE;
    }
    else if (hdr->unknown != 1)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        ret = FALSE;
    }
    else if (hdr->propID >= CERT_FIRST_USER_PROP_ID && hdr->propID <= CERT_LAST_USER_PROP_ID)
    {
        CRYPT_DATA_BLOB blob = { hdr->cb, (LPBYTE)pbElement };

        ret = contextInterface->setProp(context, hdr->propID, 0, &blob);
    }
    else if (hdr->propID != CERT_CERT_PROP_ID &&
     hdr->propID != CERT_CRL_PROP_ID && hdr->propID != CERT_CTL_PROP_ID)
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
        case CERT_KEY_PROV_INFO_PROP_ID:
        {
            CRYPT_KEY_PROV_INFO *info;

            if (read_serialized_KeyProvInfoProperty((const struct store_CRYPT_KEY_PROV_INFO *)pbElement, &info))
            {
                ret = contextInterface->setProp(context, hdr->propID, 0, info);
                CryptMemFree(info);
            }
            else
                ret = FALSE;
            break;
        }
        case CERT_KEY_CONTEXT_PROP_ID:
        {
            CERT_KEY_CONTEXT ctx;
            CRYPT_ConvertKeyContext((struct store_CERT_KEY_CONTEXT *)pbElement, &ctx);
            ret = contextInterface->setProp(context, hdr->propID, 0, &ctx);
            break;
        }
        default:
            ret = FALSE;
        }
    }
    else
    {
        /* ignore the context itself */
        ret = TRUE;
    }
    return ret;
}

const void *CRYPT_ReadSerializedElement(const BYTE *pbElement, DWORD cbElement,
 DWORD dwContextTypeFlags, DWORD *pdwContentType)
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
            contextInterface = pCertInterface;
            break;
        case CERT_STORE_CRL_CONTEXT:
            contextInterface = pCRLInterface;
            break;
        case CERT_STORE_CTL_CONTEXT:
            contextInterface = pCTLInterface;
            break;
        default:
            SetLastError(E_INVALIDARG);
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
                    if (!hdr->propID)
                    {
                        /* Like in CRYPT_findPropID, stop if the propID is zero
                         */
                        noMoreProps = TRUE;
                    }
                    else
                        ret = CRYPT_ReadContextProp(contextInterface, context,
                         hdr, pbElement, cbElement);
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
                Context_Release(context_from_ptr(context));
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

static const BYTE fileHeader[] = { 0, 0, 0, 0, 'C','E','R','T' };

typedef BOOL (*read_serialized_func)(void *handle, void *buffer,
 DWORD bytesToRead, DWORD *bytesRead);

static BOOL CRYPT_ReadSerializedStore(void *handle,
 read_serialized_func read_func, HCERTSTORE store)
{
    BYTE fileHeaderBuf[sizeof(fileHeader)];
    DWORD read;
    BOOL ret;

    /* Failure reading is non-critical, we'll leave the store empty */
    ret = read_func(handle, fileHeaderBuf, sizeof(fileHeaderBuf), &read);
    if (ret)
    {
        if (!read)
            ; /* an empty file is okay */
        else if (read != sizeof(fileHeaderBuf))
            ret = FALSE;
        else if (!memcmp(fileHeaderBuf, fileHeader, read))
        {
            WINE_CERT_PROP_HEADER propHdr;
            const void *context = NULL;
            const WINE_CONTEXT_INTERFACE *contextInterface = NULL;
            LPBYTE buf = NULL;
            DWORD bufSize = 0;

            do {
                ret = read_func(handle, &propHdr, sizeof(propHdr), &read);
                if (ret && read == sizeof(propHdr))
                {
                    if (contextInterface && context &&
                     (propHdr.propID == CERT_CERT_PROP_ID ||
                     propHdr.propID == CERT_CRL_PROP_ID ||
                     propHdr.propID == CERT_CTL_PROP_ID))
                    {
                        /* We have a new context, so free the existing one */
                        Context_Release(context_from_ptr(context));
                    }
                    if (propHdr.cb > bufSize)
                    {
                        /* Not reusing realloc, because the old data aren't
                         * needed any longer.
                         */
                        CryptMemFree(buf);
                        buf = CryptMemAlloc(propHdr.cb);
                        bufSize = propHdr.cb;
                    }
                    if (!propHdr.cb)
                        ; /* Property is empty, nothing to do */
                    else if (buf)
                    {
                        ret = read_func(handle, buf, propHdr.cb, &read);
                        if (ret && read == propHdr.cb)
                        {
                            if (propHdr.propID == CERT_CERT_PROP_ID)
                            {
                                contextInterface = pCertInterface;
                                ret = contextInterface->addEncodedToStore(store,
                                 X509_ASN_ENCODING, buf, read,
                                 CERT_STORE_ADD_NEW, &context);
                            }
                            else if (propHdr.propID == CERT_CRL_PROP_ID)
                            {
                                contextInterface = pCRLInterface;
                                ret = contextInterface->addEncodedToStore(store,
                                 X509_ASN_ENCODING, buf, read,
                                 CERT_STORE_ADD_NEW, &context);
                            }
                            else if (propHdr.propID == CERT_CTL_PROP_ID)
                            {
                                contextInterface = pCTLInterface;
                                ret = contextInterface->addEncodedToStore(store,
                                 X509_ASN_ENCODING, buf, read,
                                 CERT_STORE_ADD_NEW, &context);
                            }
                            else
                            {
                                if (!contextInterface)
                                {
                                    WARN("prop id %ld before a context id\n",
                                     propHdr.propID);
                                    ret = FALSE;
                                }
                                else
                                    ret = CRYPT_ReadContextProp(
                                     contextInterface, context, &propHdr, buf,
                                     read);
                            }
                        }
                    }
                    else
                        ret = FALSE;
                }
            } while (ret && read > 0 && propHdr.cb);
            if (contextInterface && context)
            {
                /* Free the last context added */
                Context_Release(context_from_ptr(context));
            }
            CryptMemFree(buf);
            ret = TRUE;
        }
        else
            ret = FALSE;
    }
    else
        ret = TRUE;
    return ret;
}

static BOOL read_file_wrapper(void *handle, void *buffer, DWORD bytesToRead,
 DWORD *bytesRead)
{
    return ReadFile(handle, buffer, bytesToRead, bytesRead, NULL);
}

BOOL CRYPT_ReadSerializedStoreFromFile(HANDLE file, HCERTSTORE store)
{
    return CRYPT_ReadSerializedStore(file, read_file_wrapper, store);
}

struct BlobReader
{
    const CRYPT_DATA_BLOB *blob;
    DWORD current;
};

static BOOL read_blob_wrapper(void *handle, void *buffer, DWORD bytesToRead,
 DWORD *bytesRead)
{
    struct BlobReader *reader = handle;
    BOOL ret;

    if (reader->current < reader->blob->cbData)
    {
        *bytesRead = min(bytesToRead, reader->blob->cbData - reader->current);
        memcpy(buffer, reader->blob->pbData + reader->current, *bytesRead);
        reader->current += *bytesRead;
        ret = TRUE;
    }
    else if (reader->current == reader->blob->cbData)
    {
        *bytesRead = 0;
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
}

BOOL CRYPT_ReadSerializedStoreFromBlob(const CRYPT_DATA_BLOB *blob,
 HCERTSTORE store)
{
    struct BlobReader reader = { blob, 0 };

    return CRYPT_ReadSerializedStore(&reader, read_blob_wrapper, store);
}

static BOOL WINAPI CRYPT_SerializeCertNoHash(PCCERT_CONTEXT pCertContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCertContext,
     pCertContext->pbCertEncoded, pCertContext->cbCertEncoded,
     CERT_CERT_PROP_ID, pCertInterface, dwFlags, TRUE, pbElement, pcbElement);
}

static BOOL WINAPI CRYPT_SerializeCRLNoHash(PCCRL_CONTEXT pCrlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCrlContext,
     pCrlContext->pbCrlEncoded, pCrlContext->cbCrlEncoded,
     CERT_CRL_PROP_ID, pCRLInterface, dwFlags, TRUE, pbElement, pcbElement);
}

static BOOL WINAPI CRYPT_SerializeCTLNoHash(PCCTL_CONTEXT pCtlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCtlContext,
     pCtlContext->pbCtlEncoded, pCtlContext->cbCtlEncoded,
     CERT_CTL_PROP_ID, pCTLInterface, dwFlags, TRUE, pbElement, pcbElement);
}

typedef BOOL (*SerializedOutputFunc)(void *handle, const void *buffer,
 DWORD size);

static BOOL CRYPT_SerializeContextsToStream(SerializedOutputFunc output,
 void *handle, const WINE_CONTEXT_INTERFACE *contextInterface, HCERTSTORE store)
{
    const void *context = NULL;
    BOOL ret;

    do {
        context = contextInterface->enumContextsInStore(store, context);
        if (context)
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
                    ret = output(handle, buf, size);
            }
            CryptMemFree(buf);
        }
        else
            ret = TRUE;
    } while (ret && context != NULL);
    if (context)
        Context_Release(context_from_ptr(context));
    return ret;
}

static BOOL CRYPT_WriteSerializedStoreToStream(HCERTSTORE store,
 SerializedOutputFunc output, void *handle)
{
    static const BYTE fileTrailer[12] = { 0 };
    WINE_CONTEXT_INTERFACE interface;
    BOOL ret;

    ret = output(handle, fileHeader, sizeof(fileHeader));
    if (ret)
    {
        interface = *pCertInterface;
        interface.serialize = (SerializeElementFunc)CRYPT_SerializeCertNoHash;
        ret = CRYPT_SerializeContextsToStream(output, handle, &interface,
         store);
    }
    if (ret)
    {
        interface = *pCRLInterface;
        interface.serialize = (SerializeElementFunc)CRYPT_SerializeCRLNoHash;
        ret = CRYPT_SerializeContextsToStream(output, handle, &interface,
         store);
    }
    if (ret)
    {
        interface = *pCTLInterface;
        interface.serialize = (SerializeElementFunc)CRYPT_SerializeCTLNoHash;
        ret = CRYPT_SerializeContextsToStream(output, handle, &interface,
         store);
    }
    if (ret)
        ret = output(handle, fileTrailer, sizeof(fileTrailer));
    return ret;
}

static BOOL CRYPT_FileOutputFunc(void *handle, const void *buffer, DWORD size)
{
    return WriteFile(handle, buffer, size, &size, NULL);
}

static BOOL CRYPT_WriteSerializedStoreToFile(HANDLE file, HCERTSTORE store)
{
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    return CRYPT_WriteSerializedStoreToStream(store, CRYPT_FileOutputFunc,
     file);
}

static BOOL CRYPT_SavePKCSToMem(HCERTSTORE store,
 DWORD dwMsgAndCertEncodingType, void *handle)
{
    CERT_BLOB *blob = handle;
    CRYPT_SIGNED_INFO signedInfo = { 0 };
    PCCERT_CONTEXT cert = NULL;
    PCCRL_CONTEXT crl = NULL;
    DWORD size;
    BOOL ret = TRUE;

    TRACE("(%ld, %p)\n", blob->pbData ? blob->cbData : 0, blob->pbData);

    do {
        cert = CertEnumCertificatesInStore(store, cert);
        if (cert)
            signedInfo.cCertEncoded++;
    } while (cert);
    if (signedInfo.cCertEncoded)
    {
        signedInfo.rgCertEncoded = CryptMemAlloc(
         signedInfo.cCertEncoded * sizeof(CERT_BLOB));
        if (!signedInfo.rgCertEncoded)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
        else
        {
            DWORD i = 0;

            do {
                cert = CertEnumCertificatesInStore(store, cert);
                if (cert)
                {
                    signedInfo.rgCertEncoded[i].cbData = cert->cbCertEncoded;
                    signedInfo.rgCertEncoded[i].pbData = cert->pbCertEncoded;
                    i++;
                }
            } while (cert);
        }
    }

    do {
        crl = CertEnumCRLsInStore(store, crl);
        if (crl)
            signedInfo.cCrlEncoded++;
    } while (crl);
    if (signedInfo.cCrlEncoded)
    {
        signedInfo.rgCrlEncoded = CryptMemAlloc(
         signedInfo.cCrlEncoded * sizeof(CERT_BLOB));
        if (!signedInfo.rgCrlEncoded)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
        else
        {
            DWORD i = 0;

            do {
                crl = CertEnumCRLsInStore(store, crl);
                if (crl)
                {
                    signedInfo.rgCrlEncoded[i].cbData = crl->cbCrlEncoded;
                    signedInfo.rgCrlEncoded[i].pbData = crl->pbCrlEncoded;
                    i++;
                }
            } while (crl);
        }
    }
    if (ret)
    {
        ret = CRYPT_AsnEncodeCMSSignedInfo(&signedInfo, NULL, &size);
        if (ret)
        {
            if (!blob->pbData)
                blob->cbData = size;
            else if (blob->cbData < size)
            {
                blob->cbData = size;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                blob->cbData = size;
                ret = CRYPT_AsnEncodeCMSSignedInfo(&signedInfo, blob->pbData,
                 &blob->cbData);
            }
        }
    }
    CryptMemFree(signedInfo.rgCertEncoded);
    CryptMemFree(signedInfo.rgCrlEncoded);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_SavePKCSToFile(HCERTSTORE store,
 DWORD dwMsgAndCertEncodingType, void *handle)
{
    CERT_BLOB blob = { 0, NULL };
    BOOL ret;

    TRACE("(%p)\n", handle);

    ret = CRYPT_SavePKCSToMem(store, dwMsgAndCertEncodingType, &blob);
    if (ret)
    {
        blob.pbData = CryptMemAlloc(blob.cbData);
        if (blob.pbData)
        {
            ret = CRYPT_SavePKCSToMem(store, dwMsgAndCertEncodingType, &blob);
            if (ret)
                ret = WriteFile(handle, blob.pbData, blob.cbData,
                 &blob.cbData, NULL);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_SaveSerializedToFile(HCERTSTORE store,
 DWORD dwMsgAndCertEncodingType, void *handle)
{
    return CRYPT_WriteSerializedStoreToFile(handle, store);
}

struct MemWrittenTracker
{
    DWORD cbData;
    BYTE *pbData;
    DWORD written;
};

/* handle is a pointer to a MemWrittenTracker.  Assumes its pointer is valid. */
static BOOL CRYPT_MemOutputFunc(void *handle, const void *buffer, DWORD size)
{
    struct MemWrittenTracker *tracker = handle;
    BOOL ret;

    if (tracker->written + size > tracker->cbData)
    {
        SetLastError(ERROR_MORE_DATA);
        /* Update written so caller can notify its caller of the required size
         */
        tracker->written += size;
        ret = FALSE;
    }
    else
    {
        memcpy(tracker->pbData + tracker->written, buffer, size);
        tracker->written += size;
        ret = TRUE;
    }
    return ret;
}

static BOOL CRYPT_CountSerializedBytes(void *handle, const void *buffer,
 DWORD size)
{
    *(DWORD *)handle += size;
    return TRUE;
}

static BOOL CRYPT_SaveSerializedToMem(HCERTSTORE store,
 DWORD dwMsgAndCertEncodingType, void *handle)
{
    CERT_BLOB *blob = handle;
    DWORD size = 0;
    BOOL ret;

    ret = CRYPT_WriteSerializedStoreToStream(store, CRYPT_CountSerializedBytes,
     &size);
    if (ret)
    {
        if (!blob->pbData)
            blob->cbData = size;
        else if (blob->cbData < size)
        {
            SetLastError(ERROR_MORE_DATA);
            blob->cbData = size;
            ret = FALSE;
        }
        else
        {
            struct MemWrittenTracker tracker = { blob->cbData, blob->pbData,
             0 };

            ret = CRYPT_WriteSerializedStoreToStream(store, CRYPT_MemOutputFunc,
             &tracker);
            if (!ret && GetLastError() == ERROR_MORE_DATA)
                blob->cbData = tracker.written;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSaveStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType,
 DWORD dwSaveAs, DWORD dwSaveTo, void *pvSaveToPara, DWORD dwFlags)
{
    BOOL (*saveFunc)(HCERTSTORE, DWORD, void *);
    void *handle;
    BOOL ret, closeFile = TRUE;

    TRACE("(%p, %08lx, %ld, %ld, %p, %08lx)\n", hCertStore,
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);

    switch (dwSaveAs)
    {
    case CERT_STORE_SAVE_AS_STORE:
        if (dwSaveTo == CERT_STORE_SAVE_TO_MEMORY)
            saveFunc = CRYPT_SaveSerializedToMem;
        else
            saveFunc = CRYPT_SaveSerializedToFile;
        break;
    case CERT_STORE_SAVE_AS_PKCS7:
        if (dwSaveTo == CERT_STORE_SAVE_TO_MEMORY)
            saveFunc = CRYPT_SavePKCSToMem;
        else
            saveFunc = CRYPT_SavePKCSToFile;
        break;
    default:
        WARN("unimplemented for %ld\n", dwSaveAs);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    switch (dwSaveTo)
    {
    case CERT_STORE_SAVE_TO_FILE:
        handle = pvSaveToPara;
        closeFile = FALSE;
        break;
    case CERT_STORE_SAVE_TO_FILENAME_A:
        handle = CreateFileA(pvSaveToPara, GENERIC_WRITE, 0, NULL,
         CREATE_ALWAYS, 0, NULL);
        break;
    case CERT_STORE_SAVE_TO_FILENAME_W:
        handle = CreateFileW(pvSaveToPara, GENERIC_WRITE, 0, NULL,
         CREATE_ALWAYS, 0, NULL);
        break;
    case CERT_STORE_SAVE_TO_MEMORY:
        handle = pvSaveToPara;
        break;
    default:
        WARN("unimplemented for %ld\n", dwSaveTo);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ret = saveFunc(hCertStore, dwMsgAndCertEncodingType, handle);
    if (closeFile)
        CloseHandle(handle);
    TRACE("returning %d\n", ret);
    return ret;
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
            contextInterface = pCertInterface;
            break;
        case CERT_STORE_CRL_CONTEXT:
            contextInterface = pCRLInterface;
            break;
        case CERT_STORE_CTL_CONTEXT:
            contextInterface = pCTLInterface;
            break;
        default:
            SetLastError(E_INVALIDARG);
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
            Context_Release(context_from_ptr(context));
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}
