/*
 * crypt32 Crypt*Object functions
 *
 * Copyright 2007 Juan Lang
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
#include "mssip.h"
#include "crypt32_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static BOOL CRYPT_ReadBlobFromFile(LPCWSTR fileName, PCERT_BLOB blob)
{
    BOOL ret = FALSE;
    HANDLE file;

    TRACE("%s\n", debugstr_w(fileName));

    file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
     OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        ret = TRUE;
        blob->cbData = GetFileSize(file, NULL);
        if (blob->cbData)
        {
            blob->pbData = CryptMemAlloc(blob->cbData);
            if (blob->pbData)
            {
                DWORD read;

                ret = ReadFile(file, blob->pbData, blob->cbData, &read, NULL);
            }
        }
        CloseHandle(file);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_QueryContextObject(DWORD dwObjectType, const void *pvObject,
 DWORD dwExpectedContentTypeFlags, DWORD *pdwMsgAndCertEncodingType,
 DWORD *pdwContentType, HCERTSTORE *phCertStore, const void **ppvContext)
{
    CERT_BLOB fileBlob;
    const CERT_BLOB *blob;
    HCERTSTORE store;
    DWORD contentType;
    BOOL ret;

    switch (dwObjectType)
    {
    case CERT_QUERY_OBJECT_FILE:
        /* Cert, CRL, and CTL contexts can't be "embedded" in a file, so
         * just read the file directly
         */
        ret = CRYPT_ReadBlobFromFile((LPCWSTR)pvObject, &fileBlob);
        blob = &fileBlob;
        break;
    case CERT_QUERY_OBJECT_BLOB:
        blob = (const CERT_BLOB *)pvObject;
        ret = TRUE;
        break;
    default:
        SetLastError(E_INVALIDARG); /* FIXME: is this the correct error? */
        ret = FALSE;
    }
    if (!ret)
        return FALSE;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = FALSE;
    if (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CERT)
    {
        ret = pCertInterface->addEncodedToStore(store, X509_ASN_ENCODING,
         blob->pbData, blob->cbData, CERT_STORE_ADD_ALWAYS, ppvContext);
        if (ret)
            contentType = CERT_QUERY_CONTENT_CERT;
    }
    if (!ret && (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CRL))
    {
        ret = pCRLInterface->addEncodedToStore(store, X509_ASN_ENCODING,
         blob->pbData, blob->cbData, CERT_STORE_ADD_ALWAYS, ppvContext);
        if (ret)
            contentType = CERT_QUERY_CONTENT_CRL;
    }
    if (!ret && (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CTL))
    {
        ret = pCTLInterface->addEncodedToStore(store, X509_ASN_ENCODING,
         blob->pbData, blob->cbData, CERT_STORE_ADD_ALWAYS, ppvContext);
        if (ret)
            contentType = CERT_QUERY_CONTENT_CTL;
    }
    if (ret)
    {
        if (pdwMsgAndCertEncodingType)
            *pdwMsgAndCertEncodingType = X509_ASN_ENCODING;
        if (pdwContentType)
            *pdwContentType = contentType;
        if (phCertStore)
            *phCertStore = CertDuplicateStore(store);
    }
    CertCloseStore(store, 0);
    if (blob == &fileBlob)
        CryptMemFree(blob->pbData);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_QuerySerializedContextObject(DWORD dwObjectType,
 const void *pvObject, DWORD dwExpectedContentTypeFlags,
 DWORD *pdwMsgAndCertEncodingType, DWORD *pdwContentType,
 HCERTSTORE *phCertStore, const void **ppvContext)
{
    CERT_BLOB fileBlob;
    const CERT_BLOB *blob;
    const WINE_CONTEXT_INTERFACE *contextInterface = NULL;
    const void *context;
    DWORD contextType;
    BOOL ret;

    switch (dwObjectType)
    {
    case CERT_QUERY_OBJECT_FILE:
        /* Cert, CRL, and CTL contexts can't be "embedded" in a file, so
         * just read the file directly
         */
        ret = CRYPT_ReadBlobFromFile((LPCWSTR)pvObject, &fileBlob);
        blob = &fileBlob;
        break;
    case CERT_QUERY_OBJECT_BLOB:
        blob = (const CERT_BLOB *)pvObject;
        ret = TRUE;
        break;
    default:
        SetLastError(E_INVALIDARG); /* FIXME: is this the correct error? */
        ret = FALSE;
    }
    if (!ret)
        return FALSE;

    context = CRYPT_ReadSerializedElement(blob->pbData, blob->cbData,
     CERT_STORE_ALL_CONTEXT_FLAG, &contextType);
    if (context)
    {
        DWORD contentType, certStoreOffset;

        ret = TRUE;
        switch (contextType)
        {
        case CERT_STORE_CERTIFICATE_CONTEXT:
            contextInterface = pCertInterface;
            contentType = CERT_QUERY_CONTENT_SERIALIZED_CERT;
            certStoreOffset = offsetof(CERT_CONTEXT, hCertStore);
            if (!(dwExpectedContentTypeFlags &
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT))
            {
                SetLastError(ERROR_INVALID_DATA);
                ret = FALSE;
                goto end;
            }
            break;
        case CERT_STORE_CRL_CONTEXT:
            contextInterface = pCRLInterface;
            contentType = CERT_QUERY_CONTENT_SERIALIZED_CRL;
            certStoreOffset = offsetof(CRL_CONTEXT, hCertStore);
            if (!(dwExpectedContentTypeFlags &
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL))
            {
                SetLastError(ERROR_INVALID_DATA);
                ret = FALSE;
                goto end;
            }
            break;
        case CERT_STORE_CTL_CONTEXT:
            contextInterface = pCTLInterface;
            contentType = CERT_QUERY_CONTENT_SERIALIZED_CTL;
            certStoreOffset = offsetof(CTL_CONTEXT, hCertStore);
            if (!(dwExpectedContentTypeFlags &
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL))
            {
                SetLastError(ERROR_INVALID_DATA);
                ret = FALSE;
                goto end;
            }
            break;
        default:
            SetLastError(ERROR_INVALID_DATA);
            ret = FALSE;
            goto end;
        }
        if (pdwMsgAndCertEncodingType)
            *pdwMsgAndCertEncodingType = X509_ASN_ENCODING;
        if (pdwContentType)
            *pdwContentType = contentType;
        if (phCertStore)
            *phCertStore = CertDuplicateStore(
             *(HCERTSTORE *)((const BYTE *)context + certStoreOffset));
        if (ppvContext)
            *ppvContext = contextInterface->duplicate(context);
    }

end:
    if (contextInterface && context)
        contextInterface->free(context);
    if (blob == &fileBlob)
        CryptMemFree(blob->pbData);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_QuerySerializedStoreObject(DWORD dwObjectType,
 const void *pvObject, DWORD *pdwMsgAndCertEncodingType, DWORD *pdwContentType,
 HCERTSTORE *phCertStore, HCRYPTMSG *phMsg)
{
    LPCWSTR fileName = (LPCWSTR)pvObject;
    HANDLE file;
    BOOL ret = FALSE;

    if (dwObjectType != CERT_QUERY_OBJECT_FILE)
    {
        FIXME("unimplemented for non-file type %d\n", dwObjectType);
        SetLastError(E_INVALIDARG); /* FIXME: is this the correct error? */
        return FALSE;
    }
    TRACE("%s\n", debugstr_w(fileName));
    file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
     OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        ret = CRYPT_ReadSerializedStoreFromFile(file, store);
        if (ret)
        {
            if (pdwMsgAndCertEncodingType)
                *pdwMsgAndCertEncodingType = X509_ASN_ENCODING;
            if (pdwContentType)
                *pdwContentType = CERT_QUERY_CONTENT_SERIALIZED_STORE;
            if (phCertStore)
                *phCertStore = CertDuplicateStore(store);
        }
        CertCloseStore(store, 0);
        CloseHandle(file);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Used to decode non-embedded messages */
static BOOL CRYPT_QueryMessageObject(DWORD dwObjectType, const void *pvObject,
 DWORD dwExpectedContentTypeFlags, DWORD *pdwMsgAndCertEncodingType,
 DWORD *pdwContentType, HCERTSTORE *phCertStore, HCRYPTMSG *phMsg)
{
    CERT_BLOB fileBlob;
    const CERT_BLOB *blob;
    BOOL ret;
    HCRYPTMSG msg = NULL;
    DWORD encodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

    switch (dwObjectType)
    {
    case CERT_QUERY_OBJECT_FILE:
        /* This isn't an embedded PKCS7 message, so just read the file
         * directly
         */
        ret = CRYPT_ReadBlobFromFile((LPCWSTR)pvObject, &fileBlob);
        blob = &fileBlob;
        break;
    case CERT_QUERY_OBJECT_BLOB:
        blob = (const CERT_BLOB *)pvObject;
        ret = TRUE;
        break;
    default:
        SetLastError(E_INVALIDARG); /* FIXME: is this the correct error? */
        ret = FALSE;
    }
    if (!ret)
        return FALSE;

    ret = FALSE;
    /* Try it first as a PKCS content info */
    if ((dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED))
    {
        msg = CryptMsgOpenToDecode(encodingType, 0, 0, 0, NULL, NULL);
        if (msg)
        {
            ret = CryptMsgUpdate(msg, blob->pbData, blob->cbData, TRUE);
            if (ret)
            {
                DWORD type, len = sizeof(type);

                ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, &type, &len);
                if (ret)
                {
                    if ((dwExpectedContentTypeFlags &
                     CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED))
                    {
                        if (type != CMSG_SIGNED)
                        {
                            SetLastError(ERROR_INVALID_DATA);
                            ret = FALSE;
                        }
                        else if (pdwContentType)
                            *pdwContentType = CERT_QUERY_CONTENT_PKCS7_SIGNED;
                    }
                    else if ((dwExpectedContentTypeFlags &
                     CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED))
                    {
                        if (type != CMSG_DATA)
                        {
                            SetLastError(ERROR_INVALID_DATA);
                            ret = FALSE;
                        }
                        else if (pdwContentType)
                            *pdwContentType = CERT_QUERY_CONTENT_PKCS7_UNSIGNED;
                    }
                }
            }
            if (!ret)
            {
                CryptMsgClose(msg);
                msg = NULL;
            }
        }
    }
    /* Failing that, try explicitly typed messages */
    if (!ret &&
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED))
    {
        msg = CryptMsgOpenToDecode(encodingType, 0, CMSG_SIGNED, 0, NULL, NULL);
        if (msg)
        {
            ret = CryptMsgUpdate(msg, blob->pbData, blob->cbData, TRUE);
            if (!ret)
            {
                CryptMsgClose(msg);
                msg = NULL;
            }
        }
        if (msg && pdwContentType)
            *pdwContentType = CERT_QUERY_CONTENT_PKCS7_SIGNED;
    }
    if (!ret &&
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED))
    {
        msg = CryptMsgOpenToDecode(encodingType, 0, CMSG_DATA, 0, NULL, NULL);
        if (msg)
        {
            ret = CryptMsgUpdate(msg, blob->pbData, blob->cbData, TRUE);
            if (!ret)
            {
                CryptMsgClose(msg);
                msg = NULL;
            }
        }
        if (msg && pdwContentType)
            *pdwContentType = CERT_QUERY_CONTENT_PKCS7_UNSIGNED;
    }
    if (pdwMsgAndCertEncodingType)
        *pdwMsgAndCertEncodingType = encodingType;
    if (msg)
    {
        if (phMsg)
            *phMsg = msg;
        if (phCertStore)
            *phCertStore = CertOpenStore(CERT_STORE_PROV_MSG, encodingType, 0,
             0, msg);
    }
    if (blob == &fileBlob)
        CryptMemFree(blob->pbData);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_QueryEmbeddedMessageObject(DWORD dwObjectType,
 const void *pvObject, DWORD dwExpectedContentTypeFlags,
 DWORD *pdwMsgAndCertEncodingType, DWORD *pdwContentType,
 HCERTSTORE *phCertStore, HCRYPTMSG *phMsg)
{
    HANDLE file;
    GUID subject;
    BOOL ret = FALSE;

    TRACE("%s\n", debugstr_w((LPCWSTR)pvObject));

    if (dwObjectType != CERT_QUERY_OBJECT_FILE)
    {
        FIXME("don't know what to do for type %d embedded signed messages\n",
         dwObjectType);
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    file = CreateFileW((LPCWSTR)pvObject, GENERIC_READ, FILE_SHARE_READ,
     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        ret = CryptSIPRetrieveSubjectGuid((LPCWSTR)pvObject, file, &subject);
        if (ret)
        {
            SIP_DISPATCH_INFO sip;

            memset(&sip, 0, sizeof(sip));
            sip.cbSize = sizeof(sip);
            ret = CryptSIPLoad(&subject, 0, &sip);
            if (ret)
            {
                SIP_SUBJECTINFO subjectInfo;
                CERT_BLOB blob;
                DWORD encodingType;

                memset(&subjectInfo, 0, sizeof(subjectInfo));
                subjectInfo.cbSize = sizeof(subjectInfo);
                subjectInfo.pgSubjectType = &subject;
                subjectInfo.hFile = file;
                subjectInfo.pwsFileName = (LPCWSTR)pvObject;
                ret = sip.pfGet(&subjectInfo, &encodingType, 0, &blob.cbData,
                 NULL);
                if (ret)
                {
                    blob.pbData = CryptMemAlloc(blob.cbData);
                    if (blob.pbData)
                    {
                        ret = sip.pfGet(&subjectInfo, &encodingType, 0,
                         &blob.cbData, blob.pbData);
                        if (ret)
                        {
                            ret = CRYPT_QueryMessageObject(
                             CERT_QUERY_OBJECT_BLOB, &blob,
                             CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                             pdwMsgAndCertEncodingType, NULL, phCertStore,
                             phMsg);
                            if (ret && pdwContentType)
                                *pdwContentType =
                                 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED;
                        }
                        CryptMemFree(blob.pbData);
                    }
                    else
                    {
                        SetLastError(ERROR_OUTOFMEMORY);
                        ret = FALSE;
                    }
                }
            }
        }
        CloseHandle(file);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptQueryObject(DWORD dwObjectType, const void *pvObject,
 DWORD dwExpectedContentTypeFlags, DWORD dwExpectedFormatTypeFlags,
 DWORD dwFlags, DWORD *pdwMsgAndCertEncodingType, DWORD *pdwContentType,
 DWORD *pdwFormatType, HCERTSTORE *phCertStore, HCRYPTMSG *phMsg,
 const void **ppvContext)
{
    static const DWORD unimplementedTypes =
     CERT_QUERY_CONTENT_FLAG_PKCS10 | CERT_QUERY_CONTENT_FLAG_PFX |
     CERT_QUERY_CONTENT_FLAG_CERT_PAIR;
    BOOL ret = TRUE;

    TRACE("(%08x, %p, %08x, %08x, %08x, %p, %p, %p, %p, %p, %p)\n",
     dwObjectType, pvObject, dwExpectedContentTypeFlags,
     dwExpectedFormatTypeFlags, dwFlags, pdwMsgAndCertEncodingType,
     pdwContentType, pdwFormatType, phCertStore, phMsg, ppvContext);

    if (dwExpectedContentTypeFlags & unimplementedTypes)
        WARN("unimplemented for types %08x\n",
         dwExpectedContentTypeFlags & unimplementedTypes);
    if (!(dwExpectedFormatTypeFlags & CERT_QUERY_FORMAT_FLAG_BINARY))
    {
        FIXME("unimplemented for anything but binary\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    if (pdwFormatType)
        *pdwFormatType = CERT_QUERY_FORMAT_BINARY;

    if (phCertStore)
        *phCertStore = NULL;
    if (phMsg)
        *phMsg = NULL;
    if (ppvContext)
        *ppvContext = NULL;

    ret = FALSE;
    if ((dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CERT) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CRL) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_CTL))
    {
        ret = CRYPT_QueryContextObject(dwObjectType, pvObject,
         dwExpectedContentTypeFlags, pdwMsgAndCertEncodingType, pdwContentType,
         phCertStore, ppvContext);
    }
    if (!ret &&
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE))
    {
        ret = CRYPT_QuerySerializedStoreObject(dwObjectType, pvObject,
         pdwMsgAndCertEncodingType, pdwContentType, phCertStore, phMsg);
    }
    if (!ret &&
     ((dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL)))
    {
        ret = CRYPT_QuerySerializedContextObject(dwObjectType, pvObject,
         dwExpectedContentTypeFlags, pdwMsgAndCertEncodingType, pdwContentType,
         phCertStore, ppvContext);
    }
    if (!ret &&
     ((dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED) ||
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED)))
    {
        ret = CRYPT_QueryMessageObject(dwObjectType, pvObject,
         dwExpectedContentTypeFlags, pdwMsgAndCertEncodingType, pdwContentType,
         phCertStore, phMsg);
    }
    if (!ret &&
     (dwExpectedContentTypeFlags & CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED))
    {
        ret = CRYPT_QueryEmbeddedMessageObject(dwObjectType, pvObject,
         dwExpectedContentTypeFlags, pdwMsgAndCertEncodingType, pdwContentType,
         phCertStore, phMsg);
    }
    TRACE("returning %d\n", ret);
    return ret;
}
