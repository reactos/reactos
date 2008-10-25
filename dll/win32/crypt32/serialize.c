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

#include "config.h"
#include "wine/port.h"

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
} WINE_CERT_PROP_HEADER, *PWINE_CERT_PROP_HEADER;

static BOOL CRYPT_SerializeStoreElement(const void *context,
 const BYTE *encodedContext, DWORD cbEncodedContext, DWORD contextPropID,
 PCWINE_CONTEXT_INTERFACE contextInterface, DWORD dwFlags, BOOL omitHashes,
 BYTE *pbElement, DWORD *pcbElement)
{
    BOOL ret;

    TRACE("(%p, %p, %08x, %d, %p, %p)\n", context, contextInterface, dwFlags,
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
                    bytesNeeded += sizeof(WINE_CERT_PROP_HEADER) + propSize;
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
            PWINE_CERT_PROP_HEADER hdr;
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
                                hdr = (PWINE_CERT_PROP_HEADER)pbElement;
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

            hdr = (PWINE_CERT_PROP_HEADER)pbElement;
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
            PCRYPT_KEY_PROV_INFO info =
             (PCRYPT_KEY_PROV_INFO)pbElement;

            CRYPT_FixKeyProvInfoPointers(info);
            ret = contextInterface->setProp(context,
             hdr->propID, 0, pbElement);
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

    TRACE("(%p, %d, %08x, %p)\n", pbElement, cbElement, dwContextTypeFlags,
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

                    TRACE("prop is %d\n", hdr->propID);
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

static const BYTE fileHeader[] = { 0, 0, 0, 0, 'C','E','R','T' };

BOOL CRYPT_ReadSerializedStoreFromFile(HANDLE file, HCERTSTORE store)
{
    BYTE fileHeaderBuf[sizeof(fileHeader)];
    DWORD read;
    BOOL ret;

    /* Failure reading is non-critical, we'll leave the store empty */
    ret = ReadFile(file, fileHeaderBuf, sizeof(fileHeaderBuf), &read, NULL);
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
                ret = ReadFile(file, &propHdr, sizeof(propHdr), &read, NULL);
                if (ret && read == sizeof(propHdr))
                {
                    if (contextInterface && context &&
                     (propHdr.propID == CERT_CERT_PROP_ID ||
                     propHdr.propID == CERT_CRL_PROP_ID ||
                     propHdr.propID == CERT_CTL_PROP_ID))
                    {
                        /* We have a new context, so free the existing one */
                        contextInterface->free(context);
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
                    if (buf)
                    {
                        ret = ReadFile(file, buf, propHdr.cb, &read, NULL);
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
                                ret = CRYPT_ReadContextProp(contextInterface,
                                 context, &propHdr, buf, read);
                        }
                    }
                    else
                        ret = FALSE;
                }
            } while (ret && read > 0);
            if (contextInterface && context)
            {
                /* Free the last context added */
                contextInterface->free(context);
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
        contextInterface->free(context);
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
        memcpy(&interface, pCertInterface, sizeof(interface));
        interface.serialize = (SerializeElementFunc)CRYPT_SerializeCertNoHash;
        ret = CRYPT_SerializeContextsToStream(output, handle, &interface,
         store);
    }
    if (ret)
    {
        memcpy(&interface, pCRLInterface, sizeof(interface));
        interface.serialize = (SerializeElementFunc)CRYPT_SerializeCRLNoHash;
        ret = CRYPT_SerializeContextsToStream(output, handle, &interface,
         store);
    }
    if (ret)
    {
        memcpy(&interface, pCTLInterface, sizeof(interface));
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
    CERT_BLOB *blob = (CERT_BLOB *)handle;
    CRYPT_SIGNED_INFO signedInfo = { 0 };
    PCCERT_CONTEXT cert = NULL;
    PCCRL_CONTEXT crl = NULL;
    DWORD size;
    BOOL ret = TRUE;

    TRACE("(%d, %p)\n", blob->pbData ? blob->cbData : 0, blob->pbData);

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
    struct MemWrittenTracker *tracker = (struct MemWrittenTracker *)handle;
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
    CERT_BLOB *blob = (CERT_BLOB *)handle;
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
    BOOL ret;

    TRACE("(%p, %08x, %d, %d, %p, %08x)\n", hCertStore,
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);

    switch (dwSaveAs)
    {
    case CERT_STORE_SAVE_AS_STORE:
    case CERT_STORE_SAVE_AS_PKCS7:
        break;
    default:
        WARN("unimplemented for %d\n", dwSaveAs);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    switch (dwSaveTo)
    {
    case CERT_STORE_SAVE_TO_FILE:
        handle = pvSaveToPara;
        saveFunc = dwSaveAs == CERT_STORE_SAVE_AS_STORE ?
         CRYPT_SaveSerializedToFile : CRYPT_SavePKCSToFile;
        break;
    case CERT_STORE_SAVE_TO_FILENAME_A:
        handle = CreateFileA((LPCSTR)pvSaveToPara, GENERIC_WRITE, 0, NULL,
         CREATE_ALWAYS, 0, NULL);
        saveFunc = dwSaveAs == CERT_STORE_SAVE_AS_STORE ?
         CRYPT_SaveSerializedToFile : CRYPT_SavePKCSToFile;
        break;
    case CERT_STORE_SAVE_TO_FILENAME_W:
        handle = CreateFileW((LPCWSTR)pvSaveToPara, GENERIC_WRITE, 0, NULL,
         CREATE_ALWAYS, 0, NULL);
        saveFunc = dwSaveAs == CERT_STORE_SAVE_AS_STORE ?
         CRYPT_SaveSerializedToFile : CRYPT_SavePKCSToFile;
        break;
    case CERT_STORE_SAVE_TO_MEMORY:
        handle = pvSaveToPara;
        saveFunc = dwSaveAs == CERT_STORE_SAVE_AS_STORE ?
         CRYPT_SaveSerializedToMem : CRYPT_SavePKCSToMem;
        break;
    default:
        WARN("unimplemented for %d\n", dwSaveTo);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ret = saveFunc(hCertStore, dwMsgAndCertEncodingType, handle);
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

    TRACE("(%p, %p, %d, %08x, %08x, %08x, %p, %p)\n", hCertStore,
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
            contextInterface->free(context);
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}
