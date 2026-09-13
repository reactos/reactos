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

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static void CTL_free(context_t *context)
{
    ctl_t *ctl = (ctl_t*)context;

    CryptMsgClose(ctl->ctx.hCryptMsg);
    CryptMemFree(ctl->ctx.pbCtlEncoded);
    CryptMemFree(ctl->ctx.pbCtlContext);
    LocalFree(ctl->ctx.pCtlInfo);
}

static context_t *CTL_clone(context_t *context, WINECRYPT_CERTSTORE *store, BOOL use_link)
{
    ctl_t *ctl;

    if(!use_link) {
        FIXME("Only links supported\n");
        return NULL;
    }

    ctl = (ctl_t*)Context_CreateLinkContext(sizeof(CTL_CONTEXT), context, store);
    if(!ctl)
        return NULL;

    ctl->ctx.hCertStore = store;
    return &ctl->base;
}

static const context_vtbl_t ctl_vtbl = {
    CTL_free,
    CTL_clone
};

BOOL WINAPI CertAddCTLContextToStore(HCERTSTORE hCertStore,
 PCCTL_CONTEXT pCtlContext, DWORD dwAddDisposition,
 PCCTL_CONTEXT* ppStoreContext)
{
    WINECRYPT_CERTSTORE *store = hCertStore;
    BOOL ret = TRUE;
    PCCTL_CONTEXT toAdd = NULL, existing = NULL;

    TRACE("(%p, %p, %08lx, %p)\n", hCertStore, pCtlContext, dwAddDisposition,
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
                Context_CopyProperties(existing, pCtlContext);
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
            Context_CopyProperties(toAdd, existing);
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if (existing)
        {
            Context_CopyProperties(existing, pCtlContext);
            if (ppStoreContext)
                *ppStoreContext = CertDuplicateCTLContext(existing);
        }
        else
            toAdd = CertDuplicateCTLContext(pCtlContext);
        break;
    default:
        FIXME("Unimplemented add disposition %ld\n", dwAddDisposition);
        ret = FALSE;
    }

    if (toAdd)
    {
        if (store) {
            context_t *ret_ctx;

            ret = store->vtbl->ctls.addContext(store, context_from_ptr(toAdd),
             existing ? context_from_ptr(existing) : NULL, ppStoreContext ? &ret_ctx : NULL, TRUE);
            if(ret && ppStoreContext)
                *ppStoreContext = context_ptr(ret_ctx);
        }else if (ppStoreContext) {
            *ppStoreContext = CertDuplicateCTLContext(toAdd);
        }
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

    TRACE("(%p, %08lx, %p, %ld, %08lx, %p)\n", hCertStore,
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

PCCTL_CONTEXT WINAPI CertEnumCTLsInStore(HCERTSTORE hCertStore, PCCTL_CONTEXT pPrev)
{
    ctl_t *prev = pPrev ? ctl_from_ptr(pPrev) : NULL, *ret;
    WINECRYPT_CERTSTORE *hcs = hCertStore;

    TRACE("(%p, %p)\n", hCertStore, pPrev);
    if (!hCertStore)
        ret = NULL;
    else if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        ret = NULL;
    else
        ret = (ctl_t*)hcs->vtbl->ctls.enumContext(hcs, prev ? &prev->base : NULL);
    return ret ? &ret->ctx : NULL;
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
        const CRYPT_HASH_BLOB *pHash = pvPara;

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
        const CRYPT_HASH_BLOB *pHash = pvPara;

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
        PCCTL_CONTEXT ctl = pvPara;

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

    TRACE("(%p, %ld, %ld, %ld, %p, %p)\n", hCertStore, dwCertEncodingType,
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
        FIXME("find type %08lx unimplemented\n", dwFindType);
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
    WINECRYPT_CERTSTORE *hcs;
    ctl_t *ctl = ctl_from_ptr(pCtlContext);
    BOOL ret;

    TRACE("(%p)\n", pCtlContext);

    if (!pCtlContext)
        return TRUE;

    hcs = pCtlContext->hCertStore;

    if (hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
            return FALSE;

    ret = hcs->vtbl->ctls.delete(hcs, &ctl->base);
    if (ret)
        ret = CertFreeCTLContext(pCtlContext);
    return ret;
}

PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwMsgAndCertEncodingType,
 const BYTE *pbCtlEncoded, DWORD cbCtlEncoded)
{
    ctl_t *ctl = NULL;
    HCRYPTMSG msg;
    BOOL ret;
    BYTE *content = NULL;
    DWORD contentSize = 0, size;
    PCTL_INFO ctlInfo = NULL;

    TRACE("(%08lx, %p, %ld)\n", dwMsgAndCertEncodingType, pbCtlEncoded,
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
             &ctlInfo, &size);
            if (ret)
            {
                ctl = (ctl_t*)Context_CreateDataContext(sizeof(CTL_CONTEXT), &ctl_vtbl, &empty_store);
                if (ctl)
                {
                    BYTE *data = CryptMemAlloc(cbCtlEncoded);

                    if (data)
                    {
                        memcpy(data, pbCtlEncoded, cbCtlEncoded);
                        ctl->ctx.dwMsgAndCertEncodingType =
                         X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
                        ctl->ctx.pbCtlEncoded             = data;
                        ctl->ctx.cbCtlEncoded             = cbCtlEncoded;
                        ctl->ctx.pCtlInfo                 = ctlInfo;
                        ctl->ctx.hCertStore               = &empty_store;
                        ctl->ctx.hCryptMsg                = msg;
                        ctl->ctx.pbCtlContext             = content;
                        ctl->ctx.cbCtlContext             = contentSize;
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
        if(ctl)
            Context_Release(&ctl->base);
        ctl = NULL;
        LocalFree(ctlInfo);
        CryptMemFree(content);
        CryptMsgClose(msg);
        return NULL;
    }
    return &ctl->ctx;
}

PCCTL_CONTEXT WINAPI CertDuplicateCTLContext(PCCTL_CONTEXT pCtlContext)
{
    TRACE("(%p)\n", pCtlContext);
    if (pCtlContext)
        Context_AddRef(&ctl_from_ptr(pCtlContext)->base);
    return pCtlContext;
}

BOOL WINAPI CertFreeCTLContext(PCCTL_CONTEXT pCTLContext)
{
    TRACE("(%p)\n", pCTLContext);

    if (pCTLContext)
        Context_Release(&ctl_from_ptr(pCTLContext)->base);
    return TRUE;
}

DWORD WINAPI CertEnumCTLContextProperties(PCCTL_CONTEXT pCTLContext,
 DWORD dwPropId)
{
    ctl_t *ctl = ctl_from_ptr(pCTLContext);
    DWORD ret;

    TRACE("(%p, %ld)\n", pCTLContext, dwPropId);

    if (ctl->base.properties)
        ret = ContextPropertyList_EnumPropIDs(ctl->base.properties, dwPropId);
    else
        ret = 0;
    return ret;
}

static BOOL CTLContext_SetProperty(ctl_t *ctl, DWORD dwPropId,
                                   DWORD dwFlags, const void *pvData);

static BOOL CTLContext_GetHashProp(ctl_t *ctl, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret && pvData)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CTLContext_SetProperty(ctl, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL CTLContext_GetProperty(ctl_t *ctl, DWORD dwPropId,
                                   void *pvData, DWORD *pcbData)
{
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    TRACE("(%p, %ld, %p, %p)\n", ctl, dwPropId, pvData, pcbData);

    if (ctl->base.properties)
        ret = ContextPropertyList_FindProperty(ctl->base.properties, dwPropId, &blob);
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
            ret = CTLContext_GetHashProp(ctl, dwPropId, CALG_SHA1,
             ctl->ctx.pbCtlEncoded, ctl->ctx.cbCtlEncoded, pvData, pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CTLContext_GetHashProp(ctl, dwPropId, CALG_MD5,
             ctl->ctx.pbCtlEncoded, ctl->ctx.cbCtlEncoded, pvData, pcbData);
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

    TRACE("(%p, %ld, %p, %p)\n", pCTLContext, dwPropId, pvData, pcbData);

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
            ret = CertGetStoreProperty(pCTLContext->hCertStore, dwPropId, pvData, pcbData);
        }
        break;
    default:
        ret = CTLContext_GetProperty(ctl_from_ptr(pCTLContext), dwPropId, pvData,
         pcbData);
    }
    return ret;
}

static BOOL CTLContext_SetProperty(ctl_t *ctl, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", ctl, dwPropId, dwFlags, pvData);

    if (!ctl->base.properties)
        ret = FALSE;
    else if (!pvData)
    {
        ContextPropertyList_RemoveProperty(ctl->base.properties, dwPropId);
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

            ret = ContextPropertyList_SetProperty(ctl->base.properties, dwPropId,
             blob->pbData, blob->cbData);
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            ret = ContextPropertyList_SetProperty(ctl->base.properties, dwPropId,
             pvData, sizeof(FILETIME));
            break;
        default:
            FIXME("%ld: stub\n", dwPropId);
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

    TRACE("(%p, %ld, %08lx, %p)\n", pCTLContext, dwPropId, dwFlags, pvData);

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
    ret = CTLContext_SetProperty(ctl_from_ptr(pCTLContext), dwPropId, dwFlags, pvData);
    TRACE("returning %d\n", ret);
    return ret;
}
