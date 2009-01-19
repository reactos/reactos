/*
 * Copyright 2008 Juan Lang
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
 */

#include <assert.h>
#include <stdarg.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define CtlContext_CopyProperties(to, from) \
 Context_CopyProperties((to), (from), sizeof(CTL_CONTEXT))

BOOL WINAPI CertAddCTLContextToStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pCtlContext, DWORD dwAddDisposition,
 PCCTL_CONTEXT* ppStoreContext)
{
    PWINECRYPT_CERTSTORE store = (PWINECRYPT_CERTSTORE)hCertStore;
    BOOL ret = TRUE;
    PCCTL_CONTEXT toAdd = NULL, existing = NULL;

    TRACE("(%p, %p, %08x, %p)\n", hCertStore, pCtlContext, dwAddDisposition,
     ppStoreContext);

    if (dwAddDisposition != CERT_STORE_ADD_ALWAYS)
    {
        existing = CertFindCTLInStore(hCertStore, 0, 0, CTL_FIND_EXISTING,
         pCtlContext, NULL);
    }

    switch (dwAddDisposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    case CERT_STORE_ADD_NEW:
        if (existing)
        {
            TRACE("found matching CTL, not adding\n");
            SetLastError(CRYPT_E_EXISTS);
            ret = FALSE;
        }
        else
            toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    case CERT_STORE_ADD_NEWER:
        if (existing)
        {
            LONG newer = CompareFileTime(&existing->pCtlInfo->ThisUpdate,
             &pCtlContext->pCtlInfo->ThisUpdate);

            if (newer < 0)
                toAdd = CertDuplicateCTLContext(pCtlContext);
            else
            {
                TRACE("existing CTL is newer, not adding\n");
                SetLastError(CRYPT_E_EXISTS);
                ret = FALSE;
            }
        }
        else
            toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
        if (existing)
        {
            LONG newer = CompareFileTime(&existing->pCtlInfo->ThisUpdate,
             &pCtlContext->pCtlInfo->ThisUpdate);

            if (newer < 0)
            {
                toAdd = CertDuplicateCTLContext(pCtlContext);
                CtlContext_CopyProperties(existing, pCtlContext);
            }
            else
            {
                TRACE("existing CTL is newer, not adding\n");
                SetLastError(CRYPT_E_EXISTS);
                ret = FALSE;
            }
        }
        else
            toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING:
        toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
        toAdd = CertDuplicateCTLContext(pCtlContext);
        if (existing)
            CtlContext_CopyProperties(toAdd, existing);
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if (existing)
            CtlContext_CopyProperties(existing, pCtlContext);
        break;
    default:
        FIXME("Unimplemented add disposition %d\n", dwAddDisposition);
        ret = FALSE;
    }

    if (toAdd)
    {
        if (store)
            ret = store->ctls.addContext(store, (void *)toAdd,
             (void *)existing, (const void **)ppStoreContext);
        else if (ppStoreContext)
            *ppStoreContext = CertDuplicateCTLContext(toAdd);
        CertFreeCTLContext(toAdd);
    }
    CertFreeCTLContext(existing);

    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertAddEncodedCTLToStore(HCERTSTORE hCertStore,
 DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded,
 DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext)
{
    PCCTL_CONTEXT ctl = CertCreateCTLContext(dwMsgAndCertEncodingType,
     pbCtlEncoded, cbCtlEncoded);
    BOOL ret;

    TRACE("(%p, %08x, %p, %d, %08x, %p)\n", hCertStore,
     dwMsgAndCertEncodingType, pbCtlEncoded, cbCtlEncoded, dwAddDisposition,
     ppCtlContext);

    if (ctl)
    {
        ret = CertAddCTLContextToStore(hCertStore, ctl, dwAddDisposition,
         ppCtlContext);
        CertFreeCTLContext(ctl);
    }
    else
        ret = FALSE;
    return ret;
}

PCCTL_CONTEXT WINAPI CertEnumCTLsInStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pPrev)
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *)hCertStore;
    PCCTL_CONTEXT ret;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (PCCTL_CONTEXT)hcs->ctls.enumContext(hcs, (void *)pPrev);
    return ret;
}

typedef BOOL (*CtlCompareFunc)(PCCTL_CONTEXT pCtlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara);

static BOOL compare_ctl_any(PCCTL_CONTEXT pCtlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    return TRUE;
}

static BOOL compare_ctl_by_md5_hash(PCCTL_CONTEXT pCtlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;
    BYTE hash[16];
    DWORD size = sizeof(hash);

    ret = CertGetCTLContextProperty(pCtlContext, CERT_MD5_HASH_PROP_ID, hash,
     &size);
    if (ret)
    {
        const CRYPT_HASH_BLOB *pHash = (const CRYPT_HASH_BLOB *)pvPara;

        if (size == pHash->cbData)
            ret = !memcmp(pHash->pbData, hash, size);
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL compare_ctl_by_sha1_hash(PCCTL_CONTEXT pCtlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;
    BYTE hash[20];
    DWORD size = sizeof(hash);

    ret = CertGetCTLContextProperty(pCtlContext, CERT_SHA1_HASH_PROP_ID, hash,
     &size);
    if (ret)
    {
        const CRYPT_HASH_BLOB *pHash = (const CRYPT_HASH_BLOB *)pvPara;

        if (size == pHash->cbData)
            ret = !memcmp(pHash->pbData, hash, size);
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL compare_ctl_existing(PCCTL_CONTEXT pCtlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;

    if (pvPara)
    {
        PCCTL_CONTEXT ctl = (PCCTL_CONTEXT)pvPara;

        if (pCtlContext->cbCtlContext == ctl->cbCtlContext)
        {
            if (ctl->cbCtlContext)
                ret = !memcmp(pCtlContext->pbCtlContext, ctl->pbCtlContext,
                 ctl->cbCtlContext);
            else
                ret = TRUE;
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}

PCCTL_CONTEXT WINAPI CertFindCTLInStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType,
 const void *pvFindPara, PCCTL_CONTEXT pPrevCtlContext)
{
    PCCTL_CONTEXT ret;
    CtlCompareFunc compare;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hCertStore, dwCertEncodingType,
	 dwFindFlags, dwFindType, pvFindPara, pPrevCtlContext);

    switch (dwFindType)
    {
    case CTL_FIND_ANY:
        compare = compare_ctl_any;
        break;
    case CTL_FIND_SHA1_HASH:
        compare = compare_ctl_by_sha1_hash;
        break;
    case CTL_FIND_MD5_HASH:
        compare = compare_ctl_by_md5_hash;
        break;
    case CTL_FIND_EXISTING:
        compare = compare_ctl_existing;
        break;
    default:
        FIXME("find type %08x unimplemented\n", dwFindType);
        compare = NULL;
    }

    if (compare)
    {
        BOOL matches = FALSE;

        ret = pPrevCtlContext;
        do {
            ret = CertEnumCTLsInStore(hCertStore, ret);
            if (ret)
                matches = compare(ret, dwFindType, dwFindFlags, pvFindPara);
        } while (ret != NULL && !matches);
        if (!ret)
            SetLastError(CRYPT_E_NOT_FOUND);
    }
    else
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        ret = NULL;
    }
    return ret;
}

BOOL WINAPI CertDeleteCTLFromStore(PCCTL_CONTEXT pCtlContext)
{
    BOOL ret;

    TRACE("(%p)\n", pCtlContext);

    if (!pCtlContext)
        ret = TRUE;
    else if (!pCtlContext->hCertStore)
    {
        ret = TRUE;
        CertFreeCTLContext(pCtlContext);
    }
    else
    {
        PWINECRYPT_CERTSTORE hcs =
         (PWINECRYPT_CERTSTORE)pCtlContext->hCertStore;

        if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
            ret = FALSE;
        else
            ret = hcs->ctls.deleteContext(hcs, (void *)pCtlContext);
        CertFreeCTLContext(pCtlContext);
    }
    return ret;
}

PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwMsgAndCertEncodingType,
 const BYTE *pbCtlEncoded, DWORD cbCtlEncoded)
{
    PCTL_CONTEXT ctl = NULL;
    HCRYPTMSG msg;
    BOOL ret;
    BYTE *content = NULL;
    DWORD contentSize = 0, size;
    PCTL_INFO ctlInfo = NULL;

    TRACE("(%08x, %p, %d)\n", dwMsgAndCertEncodingType, pbCtlEncoded,
     cbCtlEncoded);

    if (GET_CERT_ENCODING_TYPE(dwMsgAndCertEncodingType) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (!pbCtlEncoded || !cbCtlEncoded)
    {
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 0, 0,
     0, NULL, NULL);
    if (!msg)
        return NULL;
    ret = CryptMsgUpdate(msg, pbCtlEncoded, cbCtlEncoded, TRUE);
    if (!ret)
    {
        SetLastError(ERROR_INVALID_DATA);
        goto end;
    }
    /* Check that it's really a CTL */
    ret = CryptMsgGetParam(msg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL, &size);
    if (ret)
    {
        char *innerContent = CryptMemAlloc(size);

        if (innerContent)
        {
            ret = CryptMsgGetParam(msg, CMSG_INNER_CONTENT_TYPE_PARAM, 0,
             innerContent, &size);
            if (ret)
            {
                if (strcmp(innerContent, szOID_CTL))
                {
                    SetLastError(ERROR_INVALID_DATA);
                    ret = FALSE;
                }
            }
            CryptMemFree(innerContent);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    if (!ret)
        goto end;
    ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, NULL, &contentSize);
    if (!ret)
        goto end;
    content = CryptMemAlloc(contentSize);
    if (content)
    {
        ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, content,
         &contentSize);
        if (ret)
        {
            ret = CryptDecodeObjectEx(dwMsgAndCertEncodingType, PKCS_CTL,
             content, contentSize, CRYPT_DECODE_ALLOC_FLAG, NULL,
             (BYTE *)&ctlInfo, &size);
            if (ret)
            {
                ctl = Context_CreateDataContext(sizeof(CTL_CONTEXT));
                if (ctl)
                {
                    BYTE *data = CryptMemAlloc(cbCtlEncoded);

                    if (data)
                    {
                        memcpy(data, pbCtlEncoded, cbCtlEncoded);
                        ctl->dwMsgAndCertEncodingType =
                         X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
                        ctl->pbCtlEncoded             = data;
                        ctl->cbCtlEncoded             = cbCtlEncoded;
                        ctl->pCtlInfo                 = ctlInfo;
                        ctl->hCertStore               = NULL;
                        ctl->hCryptMsg                = msg;
                        ctl->pbCtlContext             = content;
                        ctl->cbCtlContext             = contentSize;
                    }
                    else
                    {
                        SetLastError(ERROR_OUTOFMEMORY);
                        ret = FALSE;
                    }
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
    }

end:
    if (!ret)
    {
        CryptMemFree(ctl);
        ctl = NULL;
        LocalFree(ctlInfo);
        CryptMemFree(content);
        CryptMsgClose(msg);
    }
    return (PCCTL_CONTEXT)ctl;
}

PCCTL_CONTEXT WINAPI CertDuplicateCTLContext(PCCTL_CONTEXT pCtlContext)
{
    TRACE("(%p)\n", pCtlContext);
    Context_AddRef((void *)pCtlContext, sizeof(CTL_CONTEXT));
    return pCtlContext;
}

static void CTLDataContext_Free(void *context)
{
    PCTL_CONTEXT ctlContext = (PCTL_CONTEXT)context;

    CryptMsgClose(ctlContext->hCryptMsg);
    CryptMemFree(ctlContext->pbCtlEncoded);
    CryptMemFree(ctlContext->pbCtlContext);
    LocalFree(ctlContext->pCtlInfo);
}

BOOL WINAPI CertFreeCTLContext(PCCTL_CONTEXT pCTLContext)
{
    TRACE("(%p)\n", pCTLContext);

    if (pCTLContext)
        Context_Release((void *)pCTLContext, sizeof(CTL_CONTEXT),
         CTLDataContext_Free);
    return TRUE;
}

DWORD WINAPI CertEnumCTLContextProperties(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId)
{
    PCONTEXT_PROPERTY_LIST properties = Context_GetProperties(
     (void *)pCTLContext, sizeof(CTL_CONTEXT));
    DWORD ret;

    TRACE("(%p, %d)\n", pCTLContext, dwPropId);

    if (properties)
        ret = ContextPropertyList_EnumPropIDs(properties, dwPropId);
    else
        ret = 0;
    return ret;
}

static BOOL CTLContext_SetProperty(PCCTL_CONTEXT context, DWORD dwPropId,
                                   DWORD dwFlags, const void *pvData);

static BOOL CTLContext_GetHashProp(PCCTL_CONTEXT context, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret && pvData)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CTLContext_SetProperty(context, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL CTLContext_GetProperty(PCCTL_CONTEXT context, DWORD dwPropId,
                                   void *pvData, DWORD *pcbData)
{
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CTL_CONTEXT));
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    TRACE("(%p, %d, %p, %p)\n", context, dwPropId, pvData, pcbData);

    if (properties)
        ret = ContextPropertyList_FindProperty(properties, dwPropId, &blob);
    else
        ret = FALSE;
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
    {
        /* Implicit properties */
        switch (dwPropId)
        {
        case CERT_SHA1_HASH_PROP_ID:
            ret = CTLContext_GetHashProp(context, dwPropId, CALG_SHA1,
             context->pbCtlEncoded, context->cbCtlEncoded, pvData, pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CTLContext_GetHashProp(context, dwPropId, CALG_MD5,
             context->pbCtlEncoded, context->cbCtlEncoded, pvData, pcbData);
            break;
        default:
            SetLastError(CRYPT_E_NOT_FOUND);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertGetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    BOOL ret;

    TRACE("(%p, %d, %p, %p)\n", pCTLContext, dwPropId, pvData, pcbData);

    switch (dwPropId)
    {
    case 0:
    case CERT_CERT_PROP_ID:
    case CERT_CRL_PROP_ID:
    case CERT_CTL_PROP_ID:
        SetLastError(E_INVALIDARG);
        ret = FALSE;
        break;
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
            ret = FALSE;
        }
        else
        {
            if (pCTLContext->hCertStore)
                ret = CertGetStoreProperty(pCTLContext->hCertStore, dwPropId,
                 pvData, pcbData);
            else
                *(DWORD *)pvData = 0;
            ret = TRUE;
        }
        break;
    default:
        ret = CTLContext_GetProperty(pCTLContext, dwPropId, pvData,
         pcbData);
    }
    return ret;
}

static BOOL CTLContext_SetProperty(PCCTL_CONTEXT context, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CTL_CONTEXT));
    BOOL ret;

    TRACE("(%p, %d, %08x, %p)\n", context, dwPropId, dwFlags, pvData);

    if (!properties)
        ret = FALSE;
    else if (!pvData)
    {
        ContextPropertyList_RemoveProperty(properties, dwPropId);
        ret = TRUE;
    }
    else
    {
        switch (dwPropId)
        {
        case CERT_AUTO_ENROLL_PROP_ID:
        case CERT_CTL_USAGE_PROP_ID: /* same as CERT_ENHKEY_USAGE_PROP_ID */
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

            ret = ContextPropertyList_SetProperty(properties, dwPropId,
             blob->pbData, blob->cbData);
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            ret = ContextPropertyList_SetProperty(properties, dwPropId,
             (const BYTE *)pvData, sizeof(FILETIME));
            break;
        default:
            FIXME("%d: stub\n", dwPropId);
            ret = FALSE;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetCTLContextProperty(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %d, %08x, %p)\n", pCTLContext, dwPropId, dwFlags, pvData);

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
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    ret = CTLContext_SetProperty(pCTLContext, dwPropId, dwFlags, pvData);
    TRACE("returning %d\n", ret);
    return ret;
}
