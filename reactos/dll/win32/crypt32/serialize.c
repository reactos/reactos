/*
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
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "excpt.h"
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
 PCWINE_CONTEXT_INTERFACE contextInterface, DWORD dwFlags, BYTE *pbElement,
 DWORD *pcbElement)
{
    BOOL ret;

    TRACE("(%p, %p, %08lx, %p, %p)\n", context, contextInterface, dwFlags,
     pbElement, pcbElement);

    if (context)
    {
        DWORD bytesNeeded = sizeof(WINE_CERT_PROP_HEADER) + cbEncodedContext;
        DWORD prop = 0;

        ret = TRUE;
        do {
            prop = contextInterface->enumProps(context, prop);
            if (prop)
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
                if (prop)
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
     CERT_CERT_PROP_ID, pCertInterface, dwFlags, pbElement, pcbElement);
}

BOOL WINAPI CertSerializeCRLStoreElement(PCCRL_CONTEXT pCrlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCrlContext,
     pCrlContext->pbCrlEncoded, pCrlContext->cbCrlEncoded,
     CERT_CRL_PROP_ID, pCRLInterface, dwFlags, pbElement, pcbElement);
}

BOOL WINAPI CertSerializeCTLStoreElement(PCCTL_CONTEXT pCtlContext,
 DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement)
{
    return CRYPT_SerializeStoreElement(pCtlContext,
     pCtlContext->pbCtlEncoded, pCtlContext->cbCtlEncoded,
     CERT_CTL_PROP_ID, pCRLInterface, dwFlags, pbElement, pcbElement);
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
                    if (cbElement < hdr->cb)
                    {
                        SetLastError(E_INVALIDARG);
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
