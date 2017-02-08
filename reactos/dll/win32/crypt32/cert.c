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
 *
 */

#include "crypt32_private.h"

#include <rpc.h>

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* Internal version of CertGetCertificateContextProperty that gets properties
 * directly from the context (or the context it's linked to, depending on its
 * type.) Doesn't handle special-case properties, since they are handled by
 * CertGetCertificateContextProperty, and are particular to the store in which
 * the property exists (which is separate from the context.)
 */
static BOOL CertContext_GetProperty(cert_t *cert, DWORD dwPropId,
 void *pvData, DWORD *pcbData);

/* Internal version of CertSetCertificateContextProperty that sets properties
 * directly on the context (or the context it's linked to, depending on its
 * type.) Doesn't handle special cases, since they're handled by
 * CertSetCertificateContextProperty anyway.
 */
static BOOL CertContext_SetProperty(cert_t *cert, DWORD dwPropId,
 DWORD dwFlags, const void *pvData);

BOOL WINAPI CertAddEncodedCertificateToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded,
 DWORD dwAddDisposition, PCCERT_CONTEXT *ppCertContext)
{
    PCCERT_CONTEXT cert = CertCreateCertificateContext(dwCertEncodingType,
     pbCertEncoded, cbCertEncoded);
    BOOL ret;

    TRACE("(%p, %08x, %p, %d, %08x, %p)\n", hCertStore, dwCertEncodingType,
     pbCertEncoded, cbCertEncoded, dwAddDisposition, ppCertContext);

    if (cert)
    {
        ret = CertAddCertificateContextToStore(hCertStore, cert,
         dwAddDisposition, ppCertContext);
        CertFreeCertificateContext(cert);
    }
    else
        ret = FALSE;
    return ret;
}

BOOL WINAPI CertAddEncodedCertificateToSystemStoreA(LPCSTR pszCertStoreName,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded)
{
    HCERTSTORE store;
    BOOL ret = FALSE;

    TRACE("(%s, %p, %d)\n", debugstr_a(pszCertStoreName), pbCertEncoded,
     cbCertEncoded);

    store = CertOpenSystemStoreA(0, pszCertStoreName);
    if (store)
    {
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         pbCertEncoded, cbCertEncoded, CERT_STORE_ADD_USE_EXISTING, NULL);
        CertCloseStore(store, 0);
    }
    return ret;
}

BOOL WINAPI CertAddEncodedCertificateToSystemStoreW(LPCWSTR pszCertStoreName,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded)
{
    HCERTSTORE store;
    BOOL ret = FALSE;

    TRACE("(%s, %p, %d)\n", debugstr_w(pszCertStoreName), pbCertEncoded,
     cbCertEncoded);

    store = CertOpenSystemStoreW(0, pszCertStoreName);
    if (store)
    {
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         pbCertEncoded, cbCertEncoded, CERT_STORE_ADD_USE_EXISTING, NULL);
        CertCloseStore(store, 0);
    }
    return ret;
}

static const context_vtbl_t cert_vtbl;

static void Cert_free(context_t *context)
{
    cert_t *cert = (cert_t*)context;

    CryptMemFree(cert->ctx.pbCertEncoded);
    LocalFree(cert->ctx.pCertInfo);
}

static context_t *Cert_clone(context_t *context, WINECRYPT_CERTSTORE *store, BOOL use_link)
{
    cert_t *cert;

    if(use_link) {
        cert = (cert_t*)Context_CreateLinkContext(sizeof(CERT_CONTEXT), context, store);
        if(!cert)
            return NULL;
    }else {
        const cert_t *cloned = (const cert_t*)context;
        DWORD size = 0;
        BOOL res;

        cert = (cert_t*)Context_CreateDataContext(sizeof(CERT_CONTEXT), &cert_vtbl, store);
        if(!cert)
            return NULL;

        Context_CopyProperties(&cert->ctx, &cloned->ctx);

        cert->ctx.dwCertEncodingType = cloned->ctx.dwCertEncodingType;
        cert->ctx.pbCertEncoded = CryptMemAlloc(cloned->ctx.cbCertEncoded);
        memcpy(cert->ctx.pbCertEncoded, cloned->ctx.pbCertEncoded, cloned->ctx.cbCertEncoded);
        cert->ctx.cbCertEncoded = cloned->ctx.cbCertEncoded;

        /* FIXME: We don't need to decode the object here, we could just clone cert info. */
        res = CryptDecodeObjectEx(cert->ctx.dwCertEncodingType, X509_CERT_TO_BE_SIGNED,
         cert->ctx.pbCertEncoded, cert->ctx.cbCertEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
         &cert->ctx.pCertInfo, &size);
        if(!res) {
            CertFreeCertificateContext(&cert->ctx);
            return NULL;
        }
    }

    cert->ctx.hCertStore = store;
    return &cert->base;
}

static const context_vtbl_t cert_vtbl = {
    Cert_free,
    Cert_clone
};

static BOOL add_cert_to_store(WINECRYPT_CERTSTORE *store, const CERT_CONTEXT *cert,
 DWORD add_disposition, BOOL use_link, PCCERT_CONTEXT *ret_context)
{
    const CERT_CONTEXT *existing = NULL;
    BOOL ret = TRUE, inherit_props = FALSE;
    context_t *new_context = NULL;

    switch (add_disposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        break;
    case CERT_STORE_ADD_NEW:
    case CERT_STORE_ADD_REPLACE_EXISTING:
    case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
    case CERT_STORE_ADD_USE_EXISTING:
    case CERT_STORE_ADD_NEWER:
    case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
    {
        BYTE hashToAdd[20];
        DWORD size = sizeof(hashToAdd);

        ret = CertGetCertificateContextProperty(cert, CERT_HASH_PROP_ID,
         hashToAdd, &size);
        if (ret)
        {
            CRYPT_HASH_BLOB blob = { sizeof(hashToAdd), hashToAdd };

            existing = CertFindCertificateInStore(store, cert->dwCertEncodingType, 0,
             CERT_FIND_SHA1_HASH, &blob, NULL);
        }
        break;
    }
    default:
        FIXME("Unimplemented add disposition %d\n", add_disposition);
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    switch (add_disposition)
    {
    case CERT_STORE_ADD_ALWAYS:
        break;
    case CERT_STORE_ADD_NEW:
        if (existing)
        {
            TRACE("found matching certificate, not adding\n");
            SetLastError(CRYPT_E_EXISTS);
            return FALSE;
        }
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING:
        break;
    case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
        if (use_link)
            FIXME("CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES: semi-stub for links\n");
        if (existing)
            inherit_props = TRUE;
        break;
    case CERT_STORE_ADD_USE_EXISTING:
        if(use_link)
            FIXME("CERT_STORE_ADD_USE_EXISTING: semi-stub for links\n");
        if (existing)
        {
            Context_CopyProperties(existing, cert);
            if (ret_context)
                *ret_context = CertDuplicateCertificateContext(existing);
            return TRUE;
        }
        break;
    case CERT_STORE_ADD_NEWER:
        if (existing && CompareFileTime(&existing->pCertInfo->NotBefore, &cert->pCertInfo->NotBefore) >= 0)
        {
            TRACE("existing certificate is newer, not adding\n");
            SetLastError(CRYPT_E_EXISTS);
            return FALSE;
        }
        break;
    case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
        if (existing)
        {
            if (CompareFileTime(&existing->pCertInfo->NotBefore, &cert->pCertInfo->NotBefore) >= 0)
            {
                TRACE("existing certificate is newer, not adding\n");
                SetLastError(CRYPT_E_EXISTS);
                return FALSE;
            }
            inherit_props = TRUE;
        }
        break;
    }

    /* FIXME: We have tests that this works, but what should we really do in this case? */
    if(!store) {
        if(ret_context)
            *ret_context = CertDuplicateCertificateContext(cert);
        return TRUE;
    }

    ret = store->vtbl->certs.addContext(store, context_from_ptr(cert), existing ? context_from_ptr(existing) : NULL,
     (ret_context || inherit_props) ? &new_context : NULL, use_link);
    if(!ret)
        return FALSE;

    if(inherit_props)
        Context_CopyProperties(context_ptr(new_context), existing);

    if(ret_context)
        *ret_context = context_ptr(new_context);
    else if(new_context)
        Context_Release(new_context);

    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertAddCertificateContextToStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pCertContext,
 DWORD dwAddDisposition, PCCERT_CONTEXT *ppStoreContext)
{
    WINECRYPT_CERTSTORE *store = hCertStore;

    TRACE("(%p, %p, %08x, %p)\n", hCertStore, pCertContext, dwAddDisposition, ppStoreContext);

    return add_cert_to_store(store, pCertContext, dwAddDisposition, FALSE, ppStoreContext);
}

BOOL WINAPI CertAddCertificateLinkToStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition,
 PCCERT_CONTEXT *ppCertContext)
{
    static int calls;
    WINECRYPT_CERTSTORE *store = (WINECRYPT_CERTSTORE*)hCertStore;

    if (!(calls++))
        FIXME("(%p, %p, %08x, %p): semi-stub\n", hCertStore, pCertContext,
         dwAddDisposition, ppCertContext);
    if (store->dwMagic != WINE_CRYPTCERTSTORE_MAGIC)
        return FALSE;
    if (store->type == StoreTypeCollection)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    return add_cert_to_store(hCertStore, pCertContext, dwAddDisposition, TRUE, ppCertContext);
}

PCCERT_CONTEXT WINAPI CertCreateCertificateContext(DWORD dwCertEncodingType,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded)
{
    cert_t *cert = NULL;
    BYTE *data = NULL;
    BOOL ret;
    PCERT_INFO certInfo = NULL;
    DWORD size = 0;

    TRACE("(%08x, %p, %d)\n", dwCertEncodingType, pbCertEncoded,
     cbCertEncoded);

    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT_TO_BE_SIGNED,
     pbCertEncoded, cbCertEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &certInfo, &size);
    if (!ret)
        return NULL;

    cert = (cert_t*)Context_CreateDataContext(sizeof(CERT_CONTEXT), &cert_vtbl, &empty_store);
    if (!cert)
        return NULL;
    data = CryptMemAlloc(cbCertEncoded);
    if (!data)
    {
        Context_Release(&cert->base);
        return NULL;
    }

    memcpy(data, pbCertEncoded, cbCertEncoded);
    cert->ctx.dwCertEncodingType = dwCertEncodingType;
    cert->ctx.pbCertEncoded      = data;
    cert->ctx.cbCertEncoded      = cbCertEncoded;
    cert->ctx.pCertInfo          = certInfo;
    cert->ctx.hCertStore         = &empty_store;

    return &cert->ctx;
}

PCCERT_CONTEXT WINAPI CertDuplicateCertificateContext(PCCERT_CONTEXT pCertContext)
{
    TRACE("(%p)\n", pCertContext);

    if (!pCertContext)
        return NULL;

    Context_AddRef(&cert_from_ptr(pCertContext)->base);
    return pCertContext;
}

BOOL WINAPI CertFreeCertificateContext(PCCERT_CONTEXT pCertContext)
{
    TRACE("(%p)\n", pCertContext);

    if (pCertContext)
        Context_Release(&cert_from_ptr(pCertContext)->base);
    return TRUE;
}

DWORD WINAPI CertEnumCertificateContextProperties(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId)
{
    cert_t *cert = cert_from_ptr(pCertContext);
    DWORD ret;

    TRACE("(%p, %d)\n", pCertContext, dwPropId);

    if (cert->base.properties)
        ret = ContextPropertyList_EnumPropIDs(cert->base.properties, dwPropId);
    else
        ret = 0;
    return ret;
}

static BOOL CertContext_GetHashProp(cert_t *cert, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret && pvData)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CertContext_SetProperty(cert, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL CertContext_CopyParam(void *pvData, DWORD *pcbData, const void *pb,
 DWORD cb)
{
    BOOL ret = TRUE;

    if (!pvData)
        *pcbData = cb;
    else if (*pcbData < cb)
    {
        SetLastError(ERROR_MORE_DATA);
        *pcbData = cb;
        ret = FALSE;
    }
    else
    {
        memcpy(pvData, pb, cb);
        *pcbData = cb;
    }
    return ret;
}

static BOOL CertContext_GetProperty(cert_t *cert, DWORD dwPropId,
 void *pvData, DWORD *pcbData)
{
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    TRACE("(%p, %d, %p, %p)\n", cert, dwPropId, pvData, pcbData);

    if (cert->base.properties)
        ret = ContextPropertyList_FindProperty(cert->base.properties, dwPropId, &blob);
    else
        ret = FALSE;
    if (ret)
        ret = CertContext_CopyParam(pvData, pcbData, blob.pbData, blob.cbData);
    else
    {
        /* Implicit properties */
        switch (dwPropId)
        {
        case CERT_SHA1_HASH_PROP_ID:
            ret = CertContext_GetHashProp(cert, dwPropId, CALG_SHA1,
             cert->ctx.pbCertEncoded, cert->ctx.cbCertEncoded, pvData,
             pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CertContext_GetHashProp(cert, dwPropId, CALG_MD5,
             cert->ctx.pbCertEncoded, cert->ctx.cbCertEncoded, pvData,
             pcbData);
            break;
        case CERT_SUBJECT_NAME_MD5_HASH_PROP_ID:
            ret = CertContext_GetHashProp(cert, dwPropId, CALG_MD5,
             cert->ctx.pCertInfo->Subject.pbData,
             cert->ctx.pCertInfo->Subject.cbData,
             pvData, pcbData);
            break;
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
            ret = CertContext_GetHashProp(cert, dwPropId, CALG_MD5,
             cert->ctx.pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
             cert->ctx.pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
             pvData, pcbData);
            break;
        case CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID:
            ret = CertContext_GetHashProp(cert, dwPropId, CALG_MD5,
             cert->ctx.pCertInfo->SerialNumber.pbData,
             cert->ctx.pCertInfo->SerialNumber.cbData,
             pvData, pcbData);
            break;
        case CERT_SIGNATURE_HASH_PROP_ID:
            ret = CryptHashToBeSigned(0, cert->ctx.dwCertEncodingType,
             cert->ctx.pbCertEncoded, cert->ctx.cbCertEncoded, pvData,
             pcbData);
            if (ret && pvData)
            {
                CRYPT_DATA_BLOB blob = { *pcbData, pvData };

                ret = CertContext_SetProperty(cert, dwPropId, 0, &blob);
            }
            break;
        case CERT_KEY_IDENTIFIER_PROP_ID:
        {
            PCERT_EXTENSION ext = CertFindExtension(
             szOID_SUBJECT_KEY_IDENTIFIER, cert->ctx.pCertInfo->cExtension,
             cert->ctx.pCertInfo->rgExtension);

            if (ext)
            {
                CRYPT_DATA_BLOB value;
                DWORD size = sizeof(value);

                ret = CryptDecodeObjectEx(X509_ASN_ENCODING,
                 szOID_SUBJECT_KEY_IDENTIFIER, ext->Value.pbData,
                 ext->Value.cbData, CRYPT_DECODE_NOCOPY_FLAG, NULL, &value,
                 &size);
                if (ret)
                {
                    ret = CertContext_CopyParam(pvData, pcbData, value.pbData,
                     value.cbData);
                    CertContext_SetProperty(cert, dwPropId, 0, &value);
                }
            }
            else
                SetLastError(ERROR_INVALID_DATA);
            break;
        }
        default:
            SetLastError(CRYPT_E_NOT_FOUND);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

void CRYPT_FixKeyProvInfoPointers(PCRYPT_KEY_PROV_INFO info)
{
    DWORD i, containerLen, provNameLen;
    LPBYTE data = (LPBYTE)info + sizeof(CRYPT_KEY_PROV_INFO);

    info->pwszContainerName = (LPWSTR)data;
    containerLen = (lstrlenW(info->pwszContainerName) + 1) * sizeof(WCHAR);
    data += containerLen;

    info->pwszProvName = (LPWSTR)data;
    provNameLen = (lstrlenW(info->pwszProvName) + 1) * sizeof(WCHAR);
    data += provNameLen;

    info->rgProvParam = (PCRYPT_KEY_PROV_PARAM)data;
    data += info->cProvParam * sizeof(CRYPT_KEY_PROV_PARAM);

    for (i = 0; i < info->cProvParam; i++)
    {
        info->rgProvParam[i].pbData = data;
        data += info->rgProvParam[i].cbData;
    }
}

BOOL WINAPI CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    cert_t *cert = cert_from_ptr(pCertContext);
    BOOL ret;

    TRACE("(%p, %d, %p, %p)\n", pCertContext, dwPropId, pvData, pcbData);

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
        ret = CertGetStoreProperty(cert->ctx.hCertStore, dwPropId, pvData, pcbData);
        break;
    case CERT_KEY_PROV_HANDLE_PROP_ID:
    {
        CERT_KEY_CONTEXT keyContext;
        DWORD size = sizeof(keyContext);

        ret = CertContext_GetProperty(cert,
         CERT_KEY_CONTEXT_PROP_ID, &keyContext, &size);
        if (ret)
            ret = CertContext_CopyParam(pvData, pcbData, &keyContext.hCryptProv,
             sizeof(keyContext.hCryptProv));
        break;
    }
    case CERT_KEY_PROV_INFO_PROP_ID:
        ret = CertContext_GetProperty(cert, dwPropId, pvData,
         pcbData);
        if (ret && pvData)
            CRYPT_FixKeyProvInfoPointers(pvData);
        break;
    default:
        ret = CertContext_GetProperty(cert, dwPropId, pvData,
         pcbData);
    }

    TRACE("returning %d\n", ret);
    return ret;
}

/* Copies key provider info from from into to, where to is assumed to be a
 * contiguous buffer of memory large enough for from and all its associated
 * data, but whose pointers are uninitialized.
 * Upon return, to contains a contiguous copy of from, packed in the following
 * order:
 * - CRYPT_KEY_PROV_INFO
 * - pwszContainerName
 * - pwszProvName
 * - rgProvParam[0]...
 */
static void CRYPT_CopyKeyProvInfo(PCRYPT_KEY_PROV_INFO to,
 const CRYPT_KEY_PROV_INFO *from)
{
    DWORD i;
    LPBYTE nextData = (LPBYTE)to + sizeof(CRYPT_KEY_PROV_INFO);

    if (from->pwszContainerName)
    {
        to->pwszContainerName = (LPWSTR)nextData;
        lstrcpyW(to->pwszContainerName, from->pwszContainerName);
        nextData += (lstrlenW(from->pwszContainerName) + 1) * sizeof(WCHAR);
    }
    else
        to->pwszContainerName = NULL;
    if (from->pwszProvName)
    {
        to->pwszProvName = (LPWSTR)nextData;
        lstrcpyW(to->pwszProvName, from->pwszProvName);
        nextData += (lstrlenW(from->pwszProvName) + 1) * sizeof(WCHAR);
    }
    else
        to->pwszProvName = NULL;
    to->dwProvType = from->dwProvType;
    to->dwFlags = from->dwFlags;
    to->cProvParam = from->cProvParam;
    to->rgProvParam = (PCRYPT_KEY_PROV_PARAM)nextData;
    nextData += to->cProvParam * sizeof(CRYPT_KEY_PROV_PARAM);
    to->dwKeySpec = from->dwKeySpec;
    for (i = 0; i < to->cProvParam; i++)
    {
        memcpy(&to->rgProvParam[i], &from->rgProvParam[i],
         sizeof(CRYPT_KEY_PROV_PARAM));
        to->rgProvParam[i].pbData = nextData;
        memcpy(to->rgProvParam[i].pbData, from->rgProvParam[i].pbData,
         from->rgProvParam[i].cbData);
        nextData += from->rgProvParam[i].cbData;
    }
}

static BOOL CertContext_SetKeyProvInfoProperty(CONTEXT_PROPERTY_LIST *properties,
 const CRYPT_KEY_PROV_INFO *info)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = sizeof(CRYPT_KEY_PROV_INFO), i, containerSize, provNameSize;

    if (info->pwszContainerName)
        containerSize = (lstrlenW(info->pwszContainerName) + 1) * sizeof(WCHAR);
    else
        containerSize = 0;
    if (info->pwszProvName)
        provNameSize = (lstrlenW(info->pwszProvName) + 1) * sizeof(WCHAR);
    else
        provNameSize = 0;
    size += containerSize + provNameSize;
    for (i = 0; i < info->cProvParam; i++)
        size += sizeof(CRYPT_KEY_PROV_PARAM) + info->rgProvParam[i].cbData;
    buf = CryptMemAlloc(size);
    if (buf)
    {
        CRYPT_CopyKeyProvInfo((PCRYPT_KEY_PROV_INFO)buf, info);
        ret = ContextPropertyList_SetProperty(properties,
         CERT_KEY_PROV_INFO_PROP_ID, buf, size);
        CryptMemFree(buf);
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL CertContext_SetProperty(cert_t *cert, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %d, %08x, %p)\n", cert, dwPropId, dwFlags, pvData);

    if (!cert->base.properties)
        ret = FALSE;
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
        case CERT_EXTENDED_ERROR_INFO_PROP_ID:
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
        case CERT_ENROLLMENT_PROP_ID:
        case CERT_CROSS_CERT_DIST_POINTS_PROP_ID:
        case CERT_OCSP_RESPONSE_PROP_ID:
        case CERT_RENEWAL_PROP_ID:
        {
            if (pvData)
            {
                const CRYPT_DATA_BLOB *blob = pvData;

                ret = ContextPropertyList_SetProperty(cert->base.properties, dwPropId,
                 blob->pbData, blob->cbData);
            }
            else
            {
                ContextPropertyList_RemoveProperty(cert->base.properties, dwPropId);
                ret = TRUE;
            }
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            if (pvData)
                ret = ContextPropertyList_SetProperty(cert->base.properties, dwPropId,
                 pvData, sizeof(FILETIME));
            else
            {
                ContextPropertyList_RemoveProperty(cert->base.properties, dwPropId);
                ret = TRUE;
            }
            break;
        case CERT_KEY_CONTEXT_PROP_ID:
        {
            if (pvData)
            {
                const CERT_KEY_CONTEXT *keyContext = pvData;

                if (keyContext->cbSize != sizeof(CERT_KEY_CONTEXT))
                {
                    SetLastError(E_INVALIDARG);
                    ret = FALSE;
                }
                else
                    ret = ContextPropertyList_SetProperty(cert->base.properties, dwPropId,
                     (const BYTE *)keyContext, keyContext->cbSize);
            }
            else
            {
                ContextPropertyList_RemoveProperty(cert->base.properties, dwPropId);
                ret = TRUE;
            }
            break;
        }
        case CERT_KEY_PROV_INFO_PROP_ID:
            if (pvData)
                ret = CertContext_SetKeyProvInfoProperty(cert->base.properties, pvData);
            else
            {
                ContextPropertyList_RemoveProperty(cert->base.properties, dwPropId);
                ret = TRUE;
            }
            break;
        case CERT_KEY_PROV_HANDLE_PROP_ID:
        {
            CERT_KEY_CONTEXT keyContext;
            DWORD size = sizeof(keyContext);

            ret = CertContext_GetProperty(cert, CERT_KEY_CONTEXT_PROP_ID,
             &keyContext, &size);
            if (ret)
            {
                if (!(dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
                    CryptReleaseContext(keyContext.hCryptProv, 0);
            }
            keyContext.cbSize = sizeof(keyContext);
            if (pvData)
                keyContext.hCryptProv = *(const HCRYPTPROV *)pvData;
            else
            {
                keyContext.hCryptProv = 0;
                keyContext.dwKeySpec = AT_SIGNATURE;
            }
            ret = CertContext_SetProperty(cert, CERT_KEY_CONTEXT_PROP_ID,
             0, &keyContext);
            break;
        }
        default:
            FIXME("%d: stub\n", dwPropId);
            ret = FALSE;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %d, %08x, %p)\n", pCertContext, dwPropId, dwFlags, pvData);

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
    ret = CertContext_SetProperty(cert_from_ptr(pCertContext), dwPropId, dwFlags,
     pvData);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Acquires the private key using the key provider info, retrieving info from
 * the certificate if info is NULL.  The acquired provider is returned in
 * *phCryptProv, and the key spec for the provider is returned in *pdwKeySpec.
 */
static BOOL CRYPT_AcquirePrivateKeyFromProvInfo(PCCERT_CONTEXT pCert,
 PCRYPT_KEY_PROV_INFO info, HCRYPTPROV *phCryptProv, DWORD *pdwKeySpec)
{
    DWORD size = 0;
    BOOL allocated = FALSE, ret = TRUE;

    if (!info)
    {
        ret = CertGetCertificateContextProperty(pCert,
         CERT_KEY_PROV_INFO_PROP_ID, 0, &size);
        if (ret)
        {
            info = HeapAlloc(GetProcessHeap(), 0, size);
            if (info)
            {
                ret = CertGetCertificateContextProperty(pCert,
                 CERT_KEY_PROV_INFO_PROP_ID, info, &size);
                allocated = TRUE;
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                ret = FALSE;
            }
        }
        else
            SetLastError(CRYPT_E_NO_KEY_PROPERTY);
    }
    if (ret)
    {
        ret = CryptAcquireContextW(phCryptProv, info->pwszContainerName,
         info->pwszProvName, info->dwProvType, 0);
        if (ret)
        {
            DWORD i;

            for (i = 0; i < info->cProvParam; i++)
            {
                CryptSetProvParam(*phCryptProv,
                 info->rgProvParam[i].dwParam, info->rgProvParam[i].pbData,
                 info->rgProvParam[i].dwFlags);
            }
            *pdwKeySpec = info->dwKeySpec;
        }
        else
            SetLastError(CRYPT_E_NO_KEY_PROPERTY);
    }
    if (allocated)
        HeapFree(GetProcessHeap(), 0, info);
    return ret;
}

BOOL WINAPI CryptAcquireCertificatePrivateKey(PCCERT_CONTEXT pCert,
 DWORD dwFlags, void *pvReserved, HCRYPTPROV_OR_NCRYPT_KEY_HANDLE *phCryptProv,
 DWORD *pdwKeySpec, BOOL *pfCallerFreeProv)
{
    BOOL ret = FALSE, cache = FALSE;
    PCRYPT_KEY_PROV_INFO info = NULL;
    CERT_KEY_CONTEXT keyContext;
    DWORD size;

    TRACE("(%p, %08x, %p, %p, %p, %p)\n", pCert, dwFlags, pvReserved,
     phCryptProv, pdwKeySpec, pfCallerFreeProv);

    if (dwFlags & CRYPT_ACQUIRE_USE_PROV_INFO_FLAG)
    {
        DWORD size = 0;

        ret = CertGetCertificateContextProperty(pCert,
         CERT_KEY_PROV_INFO_PROP_ID, 0, &size);
        if (ret)
        {
            info = HeapAlloc(GetProcessHeap(), 0, size);
            ret = CertGetCertificateContextProperty(pCert,
             CERT_KEY_PROV_INFO_PROP_ID, info, &size);
            if (ret)
                cache = info->dwFlags & CERT_SET_KEY_CONTEXT_PROP_ID;
        }
    }
    else if (dwFlags & CRYPT_ACQUIRE_CACHE_FLAG)
        cache = TRUE;
    *phCryptProv = 0;
    if (cache)
    {
        size = sizeof(keyContext);
        ret = CertGetCertificateContextProperty(pCert, CERT_KEY_CONTEXT_PROP_ID,
         &keyContext, &size);
        if (ret)
        {
            *phCryptProv = keyContext.hCryptProv;
            if (pdwKeySpec)
                *pdwKeySpec = keyContext.dwKeySpec;
            if (pfCallerFreeProv)
                *pfCallerFreeProv = !cache;
        }
    }
    if (!*phCryptProv)
    {
        ret = CRYPT_AcquirePrivateKeyFromProvInfo(pCert, info,
         &keyContext.hCryptProv, &keyContext.dwKeySpec);
        if (ret)
        {
            *phCryptProv = keyContext.hCryptProv;
            if (pdwKeySpec)
                *pdwKeySpec = keyContext.dwKeySpec;
            if (cache)
            {
                keyContext.cbSize = sizeof(keyContext);
                if (CertSetCertificateContextProperty(pCert,
                 CERT_KEY_CONTEXT_PROP_ID, 0, &keyContext))
                {
                    if (pfCallerFreeProv)
                        *pfCallerFreeProv = FALSE;
                }
            }
            else
            {
                if (pfCallerFreeProv)
                    *pfCallerFreeProv = TRUE;
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, info);
    return ret;
}

static BOOL key_prov_info_matches_cert(PCCERT_CONTEXT pCert,
 const CRYPT_KEY_PROV_INFO *keyProvInfo)
{
    HCRYPTPROV csp;
    BOOL matches = FALSE;

    if (CryptAcquireContextW(&csp, keyProvInfo->pwszContainerName,
     keyProvInfo->pwszProvName, keyProvInfo->dwProvType, keyProvInfo->dwFlags))
    {
        DWORD size;

        /* Need to sign something to verify the sig.  What to sign?  Why not
         * the certificate itself?
         */
        if (CryptSignAndEncodeCertificate(csp, AT_SIGNATURE,
         pCert->dwCertEncodingType, X509_CERT_TO_BE_SIGNED, pCert->pCertInfo,
         &pCert->pCertInfo->SignatureAlgorithm, NULL, NULL, &size))
        {
            BYTE *certEncoded = CryptMemAlloc(size);

            if (certEncoded)
            {
                if (CryptSignAndEncodeCertificate(csp, AT_SIGNATURE,
                 pCert->dwCertEncodingType, X509_CERT_TO_BE_SIGNED,
                 pCert->pCertInfo, &pCert->pCertInfo->SignatureAlgorithm,
                 NULL, certEncoded, &size))
                {
                    if (size == pCert->cbCertEncoded &&
                     !memcmp(certEncoded, pCert->pbCertEncoded, size))
                        matches = TRUE;
                }
                CryptMemFree(certEncoded);
            }
        }
        CryptReleaseContext(csp, 0);
    }
    return matches;
}

static BOOL container_matches_cert(PCCERT_CONTEXT pCert, LPCSTR container,
 CRYPT_KEY_PROV_INFO *keyProvInfo)
{
    CRYPT_KEY_PROV_INFO copy;
    WCHAR containerW[MAX_PATH];
    BOOL matches;

    MultiByteToWideChar(CP_ACP, 0, container, -1,
     containerW, sizeof(containerW) / sizeof(containerW[0]));
    /* We make a copy of the CRYPT_KEY_PROV_INFO because the caller expects
     * keyProvInfo->pwszContainerName to be NULL or a heap-allocated container
     * name.
     */
    copy = *keyProvInfo;
    copy.pwszContainerName = containerW;
    matches = key_prov_info_matches_cert(pCert, &copy);
    if (matches)
    {
        keyProvInfo->pwszContainerName =
         CryptMemAlloc((strlenW(containerW) + 1) * sizeof(WCHAR));
        if (keyProvInfo->pwszContainerName)
        {
            strcpyW(keyProvInfo->pwszContainerName, containerW);
            keyProvInfo->dwKeySpec = AT_SIGNATURE;
        }
        else
            matches = FALSE;
    }
    return matches;
}

/* Searches the provider named keyProvInfo.pwszProvName for a container whose
 * private key matches pCert's public key.  Upon success, updates keyProvInfo
 * with the matching container's info (free keyProvInfo.pwszContainerName upon
 * success.)
 * Returns TRUE if found, FALSE if not.
 */
static BOOL find_key_prov_info_in_provider(PCCERT_CONTEXT pCert,
 CRYPT_KEY_PROV_INFO *keyProvInfo)
{
    HCRYPTPROV defProvider;
    BOOL ret, found = FALSE;
    char containerA[MAX_PATH];

    assert(keyProvInfo->pwszContainerName == NULL);
    if ((ret = CryptAcquireContextW(&defProvider, NULL,
     keyProvInfo->pwszProvName, keyProvInfo->dwProvType,
     keyProvInfo->dwFlags | CRYPT_VERIFYCONTEXT)))
    {
        DWORD enumFlags = keyProvInfo->dwFlags | CRYPT_FIRST;

        while (ret && !found)
        {
            DWORD size = sizeof(containerA);

            ret = CryptGetProvParam(defProvider, PP_ENUMCONTAINERS,
             (BYTE *)containerA, &size, enumFlags);
            if (ret)
                found = container_matches_cert(pCert, containerA, keyProvInfo);
            if (enumFlags & CRYPT_FIRST)
            {
                enumFlags &= ~CRYPT_FIRST;
                enumFlags |= CRYPT_NEXT;
            }
        }
        CryptReleaseContext(defProvider, 0);
    }
    return found;
}

static BOOL find_matching_provider(PCCERT_CONTEXT pCert, DWORD dwFlags)
{
    BOOL found = FALSE, ret = TRUE;
    DWORD index = 0, cbProvName = 0;
    CRYPT_KEY_PROV_INFO keyProvInfo;

    TRACE("(%p, %08x)\n", pCert, dwFlags);

    memset(&keyProvInfo, 0, sizeof(keyProvInfo));
    while (ret && !found)
    {
        DWORD size = 0;

        ret = CryptEnumProvidersW(index, NULL, 0, &keyProvInfo.dwProvType,
         NULL, &size);
        if (ret)
        {
            if (size <= cbProvName)
                ret = CryptEnumProvidersW(index, NULL, 0,
                 &keyProvInfo.dwProvType, keyProvInfo.pwszProvName, &size);
            else
            {
                CryptMemFree(keyProvInfo.pwszProvName);
                keyProvInfo.pwszProvName = CryptMemAlloc(size);
                if (keyProvInfo.pwszProvName)
                {
                    cbProvName = size;
                    ret = CryptEnumProvidersW(index, NULL, 0,
                     &keyProvInfo.dwProvType, keyProvInfo.pwszProvName, &size);
                    if (ret)
                    {
                        if (dwFlags & CRYPT_FIND_SILENT_KEYSET_FLAG)
                            keyProvInfo.dwFlags |= CRYPT_SILENT;
                        if (dwFlags & CRYPT_FIND_USER_KEYSET_FLAG ||
                         !(dwFlags & (CRYPT_FIND_USER_KEYSET_FLAG |
                         CRYPT_FIND_MACHINE_KEYSET_FLAG)))
                        {
                            keyProvInfo.dwFlags |= CRYPT_USER_KEYSET;
                            found = find_key_prov_info_in_provider(pCert,
                             &keyProvInfo);
                        }
                        if (!found)
                        {
                            if (dwFlags & CRYPT_FIND_MACHINE_KEYSET_FLAG ||
                             !(dwFlags & (CRYPT_FIND_USER_KEYSET_FLAG |
                             CRYPT_FIND_MACHINE_KEYSET_FLAG)))
                            {
                                keyProvInfo.dwFlags &= ~CRYPT_USER_KEYSET;
                                keyProvInfo.dwFlags |= CRYPT_MACHINE_KEYSET;
                                found = find_key_prov_info_in_provider(pCert,
                                 &keyProvInfo);
                            }
                        }
                    }
                }
                else
                    ret = FALSE;
            }
            index++;
        }
    }
    if (found)
        CertSetCertificateContextProperty(pCert, CERT_KEY_PROV_INFO_PROP_ID,
         0, &keyProvInfo);
    CryptMemFree(keyProvInfo.pwszProvName);
    CryptMemFree(keyProvInfo.pwszContainerName);
    return found;
}

static BOOL cert_prov_info_matches_cert(PCCERT_CONTEXT pCert)
{
    BOOL matches = FALSE;
    DWORD size;

    if (CertGetCertificateContextProperty(pCert, CERT_KEY_PROV_INFO_PROP_ID,
     NULL, &size))
    {
        CRYPT_KEY_PROV_INFO *keyProvInfo = CryptMemAlloc(size);

        if (keyProvInfo)
        {
            if (CertGetCertificateContextProperty(pCert,
             CERT_KEY_PROV_INFO_PROP_ID, keyProvInfo, &size))
                matches = key_prov_info_matches_cert(pCert, keyProvInfo);
            CryptMemFree(keyProvInfo);
        }
    }
    return matches;
}

BOOL WINAPI CryptFindCertificateKeyProvInfo(PCCERT_CONTEXT pCert,
 DWORD dwFlags, void *pvReserved)
{
    BOOL matches;

    TRACE("(%p, %08x, %p)\n", pCert, dwFlags, pvReserved);

    matches = cert_prov_info_matches_cert(pCert);
    if (!matches)
        matches = find_matching_provider(pCert, dwFlags);
    return matches;
}

BOOL WINAPI CertCompareCertificate(DWORD dwCertEncodingType,
 PCERT_INFO pCertId1, PCERT_INFO pCertId2)
{
    BOOL ret;

    TRACE("(%08x, %p, %p)\n", dwCertEncodingType, pCertId1, pCertId2);

    ret = CertCompareCertificateName(dwCertEncodingType, &pCertId1->Issuer,
     &pCertId2->Issuer) && CertCompareIntegerBlob(&pCertId1->SerialNumber,
     &pCertId2->SerialNumber);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertCompareCertificateName(DWORD dwCertEncodingType,
 PCERT_NAME_BLOB pCertName1, PCERT_NAME_BLOB pCertName2)
{
    BOOL ret;

    TRACE("(%08x, %p, %p)\n", dwCertEncodingType, pCertName1, pCertName2);

    if (pCertName1->cbData == pCertName2->cbData)
    {
        if (pCertName1->cbData)
            ret = !memcmp(pCertName1->pbData, pCertName2->pbData,
             pCertName1->cbData);
        else
            ret = TRUE;
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

/* Returns the number of significant bytes in pInt, where a byte is
 * insignificant if it's a leading 0 for positive numbers or a leading 0xff
 * for negative numbers.  pInt is assumed to be little-endian.
 */
static DWORD CRYPT_significantBytes(const CRYPT_INTEGER_BLOB *pInt)
{
    DWORD ret = pInt->cbData;

    while (ret > 1)
    {
        if (pInt->pbData[ret - 2] <= 0x7f && pInt->pbData[ret - 1] == 0)
            ret--;
        else if (pInt->pbData[ret - 2] >= 0x80 && pInt->pbData[ret - 1] == 0xff)
            ret--;
        else
            break;
    }
    return ret;
}

BOOL WINAPI CertCompareIntegerBlob(PCRYPT_INTEGER_BLOB pInt1,
 PCRYPT_INTEGER_BLOB pInt2)
{
    BOOL ret;
    DWORD cb1, cb2;

    TRACE("(%p, %p)\n", pInt1, pInt2);

    cb1 = CRYPT_significantBytes(pInt1);
    cb2 = CRYPT_significantBytes(pInt2);
    if (cb1 == cb2)
    {
        if (cb1)
            ret = !memcmp(pInt1->pbData, pInt2->pbData, cb1);
        else
            ret = TRUE;
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertComparePublicKeyInfo(DWORD dwCertEncodingType,
 PCERT_PUBLIC_KEY_INFO pPublicKey1, PCERT_PUBLIC_KEY_INFO pPublicKey2)
{
    BOOL ret;

    TRACE("(%08x, %p, %p)\n", dwCertEncodingType, pPublicKey1, pPublicKey2);

    switch (GET_CERT_ENCODING_TYPE(dwCertEncodingType))
    {
    case 0:	/* Seems to mean "raw binary bits" */
        if (pPublicKey1->PublicKey.cbData == pPublicKey2->PublicKey.cbData &&
         pPublicKey1->PublicKey.cUnusedBits == pPublicKey2->PublicKey.cUnusedBits)
        {
          if (pPublicKey2->PublicKey.cbData)
              ret = !memcmp(pPublicKey1->PublicKey.pbData,
               pPublicKey2->PublicKey.pbData, pPublicKey1->PublicKey.cbData);
          else
              ret = TRUE;
        }
        else
            ret = FALSE;
        break;
    default:
        WARN("Unknown encoding type %08x\n", dwCertEncodingType);
        /* FALLTHROUGH */
    case X509_ASN_ENCODING:
    {
        BLOBHEADER *pblob1, *pblob2;
        DWORD length;
        ret = FALSE;
        if (CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
                    pPublicKey1->PublicKey.pbData, pPublicKey1->PublicKey.cbData,
                    0, NULL, &length))
        {
            pblob1 = CryptMemAlloc(length);
            if (CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
                    pPublicKey1->PublicKey.pbData, pPublicKey1->PublicKey.cbData,
                    0, pblob1, &length))
            {
                if (CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
                            pPublicKey2->PublicKey.pbData, pPublicKey2->PublicKey.cbData,
                            0, NULL, &length))
                {
                    pblob2 = CryptMemAlloc(length);
                    if (CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
                            pPublicKey2->PublicKey.pbData, pPublicKey2->PublicKey.cbData,
                            0, pblob2, &length))
                    {
                        /* The RSAPUBKEY structure directly follows the BLOBHEADER */
                        RSAPUBKEY *pk1 = (LPVOID)(pblob1 + 1),
                                  *pk2 = (LPVOID)(pblob2 + 1);
                        ret = (pk1->bitlen == pk2->bitlen) && (pk1->pubexp == pk2->pubexp)
                                 && !memcmp(pk1 + 1, pk2 + 1, pk1->bitlen/8);
                    }
                    CryptMemFree(pblob2);
                }
            }
            CryptMemFree(pblob1);
        }

        break;
    }
    }
    return ret;
}

DWORD WINAPI CertGetPublicKeyLength(DWORD dwCertEncodingType,
 PCERT_PUBLIC_KEY_INFO pPublicKey)
{
    DWORD len = 0;

    TRACE("(%08x, %p)\n", dwCertEncodingType, pPublicKey);

    if (GET_CERT_ENCODING_TYPE(dwCertEncodingType) != X509_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
    }
    if (pPublicKey->Algorithm.pszObjId &&
     !strcmp(pPublicKey->Algorithm.pszObjId, szOID_RSA_DH))
    {
        FIXME("unimplemented for DH public keys\n");
        SetLastError(CRYPT_E_ASN1_BADTAG);
    }
    else
    {
        DWORD size;
        PBYTE buf;
        BOOL ret = CryptDecodeObjectEx(dwCertEncodingType,
         RSA_CSP_PUBLICKEYBLOB, pPublicKey->PublicKey.pbData,
         pPublicKey->PublicKey.cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
         &size);

        if (ret)
        {
            RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(buf + sizeof(BLOBHEADER));

            len = rsaPubKey->bitlen;
            LocalFree(buf);
        }
    }
    return len;
}

typedef BOOL (*CertCompareFunc)(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara);

static BOOL compare_cert_by_md5_hash(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;
    BYTE hash[16];
    DWORD size = sizeof(hash);

    ret = CertGetCertificateContextProperty(pCertContext,
     CERT_MD5_HASH_PROP_ID, hash, &size);
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

static BOOL compare_cert_by_sha1_hash(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;
    BYTE hash[20];
    DWORD size = sizeof(hash);

    ret = CertGetCertificateContextProperty(pCertContext,
     CERT_SHA1_HASH_PROP_ID, hash, &size);
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

static BOOL compare_cert_by_name(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    CERT_NAME_BLOB *blob = (CERT_NAME_BLOB *)pvPara, *toCompare;
    BOOL ret;

    if (dwType & CERT_INFO_SUBJECT_FLAG)
        toCompare = &pCertContext->pCertInfo->Subject;
    else
        toCompare = &pCertContext->pCertInfo->Issuer;
    ret = CertCompareCertificateName(pCertContext->dwCertEncodingType,
     toCompare, blob);
    return ret;
}

static BOOL compare_cert_by_public_key(PCCERT_CONTEXT pCertContext,
 DWORD dwType, DWORD dwFlags, const void *pvPara)
{
    CERT_PUBLIC_KEY_INFO *publicKey = (CERT_PUBLIC_KEY_INFO *)pvPara;
    BOOL ret;

    ret = CertComparePublicKeyInfo(pCertContext->dwCertEncodingType,
     &pCertContext->pCertInfo->SubjectPublicKeyInfo, publicKey);
    return ret;
}

static BOOL compare_cert_by_subject_cert(PCCERT_CONTEXT pCertContext,
 DWORD dwType, DWORD dwFlags, const void *pvPara)
{
    CERT_INFO *pCertInfo = (CERT_INFO *)pvPara;
    BOOL ret;

    /* Matching serial number and subject match.. */
    ret = CertCompareCertificateName(pCertContext->dwCertEncodingType,
     &pCertContext->pCertInfo->Subject, &pCertInfo->Issuer);
    if (ret)
        ret = CertCompareIntegerBlob(&pCertContext->pCertInfo->SerialNumber,
         &pCertInfo->SerialNumber);
    else
    {
        /* failing that, if the serial number and issuer match, we match */
        ret = CertCompareIntegerBlob(&pCertContext->pCertInfo->SerialNumber,
         &pCertInfo->SerialNumber);
        if (ret)
            ret = CertCompareCertificateName(pCertContext->dwCertEncodingType,
             &pCertContext->pCertInfo->Issuer, &pCertInfo->Issuer);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL compare_cert_by_cert_id(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    CERT_ID *id = (CERT_ID *)pvPara;
    BOOL ret;

    switch (id->dwIdChoice)
    {
    case CERT_ID_ISSUER_SERIAL_NUMBER:
        ret = CertCompareCertificateName(pCertContext->dwCertEncodingType,
         &pCertContext->pCertInfo->Issuer, &id->u.IssuerSerialNumber.Issuer);
        if (ret)
            ret = CertCompareIntegerBlob(&pCertContext->pCertInfo->SerialNumber,
             &id->u.IssuerSerialNumber.SerialNumber);
        break;
    case CERT_ID_SHA1_HASH:
        ret = compare_cert_by_sha1_hash(pCertContext, dwType, dwFlags,
         &id->u.HashId);
        break;
    case CERT_ID_KEY_IDENTIFIER:
    {
        DWORD size = 0;

        ret = CertGetCertificateContextProperty(pCertContext,
         CERT_KEY_IDENTIFIER_PROP_ID, NULL, &size);
        if (ret && size == id->u.KeyId.cbData)
        {
            LPBYTE buf = CryptMemAlloc(size);

            if (buf)
            {
                CertGetCertificateContextProperty(pCertContext,
                 CERT_KEY_IDENTIFIER_PROP_ID, buf, &size);
                ret = !memcmp(buf, id->u.KeyId.pbData, size);
                CryptMemFree(buf);
            }
            else
                ret = FALSE;
        }
        else
            ret = FALSE;
        break;
    }
    default:
        ret = FALSE;
        break;
    }
    return ret;
}

static BOOL compare_existing_cert(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    PCCERT_CONTEXT toCompare = pvPara;
    return CertCompareCertificate(pCertContext->dwCertEncodingType,
     pCertContext->pCertInfo, toCompare->pCertInfo);
}

static BOOL compare_cert_by_signature_hash(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    const CRYPT_HASH_BLOB *hash = pvPara;
    DWORD size = 0;
    BOOL ret;

    ret = CertGetCertificateContextProperty(pCertContext,
     CERT_SIGNATURE_HASH_PROP_ID, NULL, &size);
    if (ret && size == hash->cbData)
    {
        LPBYTE buf = CryptMemAlloc(size);

        if (buf)
        {
            CertGetCertificateContextProperty(pCertContext,
             CERT_SIGNATURE_HASH_PROP_ID, buf, &size);
            ret = !memcmp(buf, hash->pbData, size);
            CryptMemFree(buf);
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}

static inline PCCERT_CONTEXT cert_compare_certs_in_store(HCERTSTORE store,
 PCCERT_CONTEXT prev, CertCompareFunc compare, DWORD dwType, DWORD dwFlags,
 const void *pvPara)
{
    BOOL matches = FALSE;
    PCCERT_CONTEXT ret;

    ret = prev;
    do {
        ret = CertEnumCertificatesInStore(store, ret);
        if (ret)
            matches = compare(ret, dwType, dwFlags, pvPara);
    } while (ret != NULL && !matches);
    return ret;
}

typedef PCCERT_CONTEXT (*CertFindFunc)(HCERTSTORE store, DWORD dwType,
 DWORD dwFlags, const void *pvPara, PCCERT_CONTEXT prev);

static PCCERT_CONTEXT find_cert_any(HCERTSTORE store, DWORD dwType,
 DWORD dwFlags, const void *pvPara, PCCERT_CONTEXT prev)
{
    return CertEnumCertificatesInStore(store, prev);
}

static PCCERT_CONTEXT find_cert_by_issuer(HCERTSTORE store, DWORD dwType,
 DWORD dwFlags, const void *pvPara, PCCERT_CONTEXT prev)
{
    BOOL ret;
    PCCERT_CONTEXT found = NULL, subject = pvPara;
    PCERT_EXTENSION ext;
    DWORD size;

    if ((ext = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER,
     subject->pCertInfo->cExtension, subject->pCertInfo->rgExtension)))
    {
        CERT_AUTHORITY_KEY_ID_INFO *info;

        ret = CryptDecodeObjectEx(subject->dwCertEncodingType,
         X509_AUTHORITY_KEY_ID, ext->Value.pbData, ext->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &info, &size);
        if (ret)
        {
            CERT_ID id;

            if (info->CertIssuer.cbData && info->CertSerialNumber.cbData)
            {
                id.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
                memcpy(&id.u.IssuerSerialNumber.Issuer, &info->CertIssuer,
                 sizeof(CERT_NAME_BLOB));
                memcpy(&id.u.IssuerSerialNumber.SerialNumber,
                 &info->CertSerialNumber, sizeof(CRYPT_INTEGER_BLOB));
            }
            else if (info->KeyId.cbData)
            {
                id.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
                memcpy(&id.u.KeyId, &info->KeyId, sizeof(CRYPT_HASH_BLOB));
            }
            else
                ret = FALSE;
            if (ret)
                found = cert_compare_certs_in_store(store, prev,
                 compare_cert_by_cert_id, dwType, dwFlags, &id);
            LocalFree(info);
        }
    }
    else if ((ext = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER2,
     subject->pCertInfo->cExtension, subject->pCertInfo->rgExtension)))
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info;

        ret = CryptDecodeObjectEx(subject->dwCertEncodingType,
         X509_AUTHORITY_KEY_ID2, ext->Value.pbData, ext->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &info, &size);
        if (ret)
        {
            CERT_ID id;

            if (info->AuthorityCertIssuer.cAltEntry &&
             info->AuthorityCertSerialNumber.cbData)
            {
                PCERT_ALT_NAME_ENTRY directoryName = NULL;
                DWORD i;

                for (i = 0; !directoryName &&
                 i < info->AuthorityCertIssuer.cAltEntry; i++)
                    if (info->AuthorityCertIssuer.rgAltEntry[i].dwAltNameChoice
                     == CERT_ALT_NAME_DIRECTORY_NAME)
                        directoryName =
                         &info->AuthorityCertIssuer.rgAltEntry[i];
                if (directoryName)
                {
                    id.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
                    memcpy(&id.u.IssuerSerialNumber.Issuer,
                     &directoryName->u.DirectoryName, sizeof(CERT_NAME_BLOB));
                    memcpy(&id.u.IssuerSerialNumber.SerialNumber,
                     &info->AuthorityCertSerialNumber,
                     sizeof(CRYPT_INTEGER_BLOB));
                }
                else
                {
                    FIXME("no supported name type in authority key id2\n");
                    ret = FALSE;
                }
            }
            else if (info->KeyId.cbData)
            {
                id.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
                memcpy(&id.u.KeyId, &info->KeyId, sizeof(CRYPT_HASH_BLOB));
            }
            else
                ret = FALSE;
            if (ret)
                found = cert_compare_certs_in_store(store, prev,
                 compare_cert_by_cert_id, dwType, dwFlags, &id);
            LocalFree(info);
        }
    }
    else
       found = cert_compare_certs_in_store(store, prev,
        compare_cert_by_name, CERT_COMPARE_NAME | CERT_COMPARE_SUBJECT_CERT,
        dwFlags, &subject->pCertInfo->Issuer);
    return found;
}

static BOOL compare_cert_by_name_str(PCCERT_CONTEXT pCertContext,
 DWORD dwType, DWORD dwFlags, const void *pvPara)
{
    PCERT_NAME_BLOB name;
    DWORD len;
    BOOL ret = FALSE;

    if (dwType & CERT_INFO_SUBJECT_FLAG)
        name = &pCertContext->pCertInfo->Subject;
    else
        name = &pCertContext->pCertInfo->Issuer;
    len = CertNameToStrW(pCertContext->dwCertEncodingType, name,
     CERT_SIMPLE_NAME_STR, NULL, 0);
    if (len)
    {
        LPWSTR str = CryptMemAlloc(len * sizeof(WCHAR));

        if (str)
        {
            LPWSTR ptr;

            CertNameToStrW(pCertContext->dwCertEncodingType, name,
             CERT_SIMPLE_NAME_STR, str, len);
            for (ptr = str; *ptr; ptr++)
                *ptr = tolowerW(*ptr);
            if (strstrW(str, pvPara))
                ret = TRUE;
            CryptMemFree(str);
        }
    }
    return ret;
}

static PCCERT_CONTEXT find_cert_by_name_str_a(HCERTSTORE store, DWORD dwType,
 DWORD dwFlags, const void *pvPara, PCCERT_CONTEXT prev)
{
    PCCERT_CONTEXT found = NULL;

    TRACE("%s\n", debugstr_a(pvPara));

    if (pvPara)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, pvPara, -1, NULL, 0);
        LPWSTR str = CryptMemAlloc(len * sizeof(WCHAR));

        if (str)
        {
            LPWSTR ptr;

            MultiByteToWideChar(CP_ACP, 0, pvPara, -1, str, len);
            for (ptr = str; *ptr; ptr++)
                *ptr = tolowerW(*ptr);
            found = cert_compare_certs_in_store(store, prev,
             compare_cert_by_name_str, dwType, dwFlags, str);
            CryptMemFree(str);
        }
    }
    else
        found = find_cert_any(store, dwType, dwFlags, NULL, prev);
    return found;
}

static PCCERT_CONTEXT find_cert_by_name_str_w(HCERTSTORE store, DWORD dwType,
 DWORD dwFlags, const void *pvPara, PCCERT_CONTEXT prev)
{
    PCCERT_CONTEXT found = NULL;

    TRACE("%s\n", debugstr_w(pvPara));

    if (pvPara)
    {
        DWORD len = strlenW(pvPara);
        LPWSTR str = CryptMemAlloc((len + 1) * sizeof(WCHAR));

        if (str)
        {
            LPCWSTR src;
            LPWSTR dst;

            for (src = pvPara, dst = str; *src; src++, dst++)
                *dst = tolowerW(*src);
            *dst = 0;
           found = cert_compare_certs_in_store(store, prev,
            compare_cert_by_name_str, dwType, dwFlags, str);
           CryptMemFree(str);
        }
    }
    else
        found = find_cert_any(store, dwType, dwFlags, NULL, prev);
    return found;
}

PCCERT_CONTEXT WINAPI CertFindCertificateInStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, DWORD dwFlags, DWORD dwType, const void *pvPara,
 PCCERT_CONTEXT pPrevCertContext)
{
    PCCERT_CONTEXT ret;
    CertFindFunc find = NULL;
    CertCompareFunc compare = NULL;

    TRACE("(%p, %08x, %08x, %08x, %p, %p)\n", hCertStore, dwCertEncodingType,
	 dwFlags, dwType, pvPara, pPrevCertContext);

    switch (dwType >> CERT_COMPARE_SHIFT)
    {
    case CERT_COMPARE_ANY:
        find = find_cert_any;
        break;
    case CERT_COMPARE_MD5_HASH:
        compare = compare_cert_by_md5_hash;
        break;
    case CERT_COMPARE_SHA1_HASH:
        compare = compare_cert_by_sha1_hash;
        break;
    case CERT_COMPARE_NAME:
        compare = compare_cert_by_name;
        break;
    case CERT_COMPARE_PUBLIC_KEY:
        compare = compare_cert_by_public_key;
        break;
    case CERT_COMPARE_NAME_STR_A:
        find = find_cert_by_name_str_a;
        break;
    case CERT_COMPARE_NAME_STR_W:
        find = find_cert_by_name_str_w;
        break;
    case CERT_COMPARE_SUBJECT_CERT:
        compare = compare_cert_by_subject_cert;
        break;
    case CERT_COMPARE_CERT_ID:
        compare = compare_cert_by_cert_id;
        break;
    case CERT_COMPARE_ISSUER_OF:
        find = find_cert_by_issuer;
        break;
    case CERT_COMPARE_EXISTING:
        compare = compare_existing_cert;
        break;
    case CERT_COMPARE_SIGNATURE_HASH:
        compare = compare_cert_by_signature_hash;
        break;
    default:
        FIXME("find type %08x unimplemented\n", dwType);
    }

    if (find)
        ret = find(hCertStore, dwFlags, dwType, pvPara, pPrevCertContext);
    else if (compare)
        ret = cert_compare_certs_in_store(hCertStore, pPrevCertContext,
         compare, dwType, dwFlags, pvPara);
    else
        ret = NULL;
    if (!ret)
        SetLastError(CRYPT_E_NOT_FOUND);
    TRACE("returning %p\n", ret);
    return ret;
}

PCCERT_CONTEXT WINAPI CertGetSubjectCertificateFromStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, PCERT_INFO pCertId)
{
    TRACE("(%p, %08x, %p)\n", hCertStore, dwCertEncodingType, pCertId);

    if (!pCertId)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    return CertFindCertificateInStore(hCertStore, dwCertEncodingType, 0,
     CERT_FIND_SUBJECT_CERT, pCertId, NULL);
}

BOOL WINAPI CertVerifySubjectCertificateContext(PCCERT_CONTEXT pSubject,
 PCCERT_CONTEXT pIssuer, DWORD *pdwFlags)
{
    static const DWORD supportedFlags = CERT_STORE_REVOCATION_FLAG |
     CERT_STORE_SIGNATURE_FLAG | CERT_STORE_TIME_VALIDITY_FLAG;

    if (*pdwFlags & ~supportedFlags)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (*pdwFlags & CERT_STORE_REVOCATION_FLAG)
    {
        DWORD flags = 0;
        PCCRL_CONTEXT crl = CertGetCRLFromStore(pSubject->hCertStore, pSubject,
         NULL, &flags);

        /* FIXME: what if the CRL has expired? */
        if (crl)
        {
            if (CertVerifyCRLRevocation(pSubject->dwCertEncodingType,
             pSubject->pCertInfo, 1, (PCRL_INFO *)&crl->pCrlInfo))
                *pdwFlags &= CERT_STORE_REVOCATION_FLAG;
        }
        else
            *pdwFlags |= CERT_STORE_NO_CRL_FLAG;
    }
    if (*pdwFlags & CERT_STORE_TIME_VALIDITY_FLAG)
    {
        if (0 == CertVerifyTimeValidity(NULL, pSubject->pCertInfo))
            *pdwFlags &= ~CERT_STORE_TIME_VALIDITY_FLAG;
    }
    if (*pdwFlags & CERT_STORE_SIGNATURE_FLAG)
    {
        if (CryptVerifyCertificateSignatureEx(0, pSubject->dwCertEncodingType,
         CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT, (void *)pSubject,
         CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT, (void *)pIssuer, 0, NULL))
            *pdwFlags &= ~CERT_STORE_SIGNATURE_FLAG;
    }
    return TRUE;
}

PCCERT_CONTEXT WINAPI CertGetIssuerCertificateFromStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pSubjectContext, PCCERT_CONTEXT pPrevIssuerContext,
 DWORD *pdwFlags)
{
    PCCERT_CONTEXT ret;

    TRACE("(%p, %p, %p, %08x)\n", hCertStore, pSubjectContext,
     pPrevIssuerContext, *pdwFlags);

    if (!pSubjectContext)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    ret = CertFindCertificateInStore(hCertStore,
     pSubjectContext->dwCertEncodingType, 0, CERT_FIND_ISSUER_OF,
     pSubjectContext, pPrevIssuerContext);
    if (ret)
    {
        if (!CertVerifySubjectCertificateContext(pSubjectContext, ret,
         pdwFlags))
        {
            CertFreeCertificateContext(ret);
            ret = NULL;
        }
        if (CRYPT_IsCertificateSelfSigned(pSubjectContext))
        {
            CertFreeCertificateContext(ret);
            ret = NULL;
            SetLastError(CRYPT_E_SELF_SIGNED);
        }
    }
    TRACE("returning %p\n", ret);
    return ret;
}

typedef struct _OLD_CERT_REVOCATION_STATUS {
    DWORD cbSize;
    DWORD dwIndex;
    DWORD dwError;
    DWORD dwReason;
} OLD_CERT_REVOCATION_STATUS;

typedef BOOL (WINAPI *CertVerifyRevocationFunc)(DWORD, DWORD, DWORD,
 void **, DWORD, PCERT_REVOCATION_PARA, PCERT_REVOCATION_STATUS);

BOOL WINAPI CertVerifyRevocation(DWORD dwEncodingType, DWORD dwRevType,
 DWORD cContext, PVOID rgpvContext[], DWORD dwFlags,
 PCERT_REVOCATION_PARA pRevPara, PCERT_REVOCATION_STATUS pRevStatus)
{
    BOOL ret;

    TRACE("(%08x, %d, %d, %p, %08x, %p, %p)\n", dwEncodingType, dwRevType,
     cContext, rgpvContext, dwFlags, pRevPara, pRevStatus);

    if (pRevStatus->cbSize != sizeof(OLD_CERT_REVOCATION_STATUS) &&
     pRevStatus->cbSize != sizeof(CERT_REVOCATION_STATUS))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (cContext)
    {
        static HCRYPTOIDFUNCSET set = NULL;
        DWORD size;

        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_VERIFY_REVOCATION_FUNC, 0);
        ret = CryptGetDefaultOIDDllList(set, dwEncodingType, NULL, &size);
        if (ret)
        {
            if (size == 1)
            {
                /* empty list */
                SetLastError(CRYPT_E_NO_REVOCATION_DLL);
                ret = FALSE;
            }
            else
            {
                LPWSTR dllList = CryptMemAlloc(size * sizeof(WCHAR)), ptr;

                if (dllList)
                {
                    ret = CryptGetDefaultOIDDllList(set, dwEncodingType,
                     dllList, &size);
                    if (ret)
                    {
                        for (ptr = dllList; ret && *ptr;
                         ptr += lstrlenW(ptr) + 1)
                        {
                            CertVerifyRevocationFunc func;
                            HCRYPTOIDFUNCADDR hFunc;

                            ret = CryptGetDefaultOIDFunctionAddress(set,
                             dwEncodingType, ptr, 0, (void **)&func, &hFunc);
                            if (ret)
                            {
                                ret = func(dwEncodingType, dwRevType, cContext,
                                 rgpvContext, dwFlags, pRevPara, pRevStatus);
                                CryptFreeOIDFunctionAddress(hFunc, 0);
                            }
                        }
                    }
                    CryptMemFree(dllList);
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
        ret = TRUE;
    return ret;
}

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr,
 CRYPT_ATTRIBUTE rgAttr[])
{
    PCRYPT_ATTRIBUTE ret = NULL;
    DWORD i;

    TRACE("%s %d %p\n", debugstr_a(pszObjId), cAttr, rgAttr);

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

    TRACE("%s %d %p\n", debugstr_a(pszObjId), cExtensions, rgExtensions);

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

static BOOL find_matching_rdn_attr(DWORD dwFlags, const CERT_NAME_INFO *name,
 const CERT_RDN_ATTR *attr)
{
    DWORD i, j;
    BOOL match = FALSE;

    for (i = 0; !match && i < name->cRDN; i++)
    {
        for (j = 0; j < name->rgRDN[i].cRDNAttr; j++)
        {
            if (!strcmp(name->rgRDN[i].rgRDNAttr[j].pszObjId,
             attr->pszObjId) &&
             name->rgRDN[i].rgRDNAttr[j].dwValueType ==
             attr->dwValueType)
            {
                if (dwFlags & CERT_UNICODE_IS_RDN_ATTRS_FLAG)
                {
                    LPCWSTR nameStr =
                     (LPCWSTR)name->rgRDN[i].rgRDNAttr[j].Value.pbData;
                    LPCWSTR attrStr = (LPCWSTR)attr->Value.pbData;

                    if (attr->Value.cbData !=
                     name->rgRDN[i].rgRDNAttr[j].Value.cbData)
                        match = FALSE;
                    else if (dwFlags & CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG)
                        match = !strncmpiW(nameStr, attrStr,
                         attr->Value.cbData / sizeof(WCHAR));
                    else
                        match = !strncmpW(nameStr, attrStr,
                         attr->Value.cbData / sizeof(WCHAR));
                    TRACE("%s : %s => %d\n",
                     debugstr_wn(nameStr, attr->Value.cbData / sizeof(WCHAR)),
                     debugstr_wn(attrStr, attr->Value.cbData / sizeof(WCHAR)),
                     match);
                }
                else
                {
                    LPCSTR nameStr =
                     (LPCSTR)name->rgRDN[i].rgRDNAttr[j].Value.pbData;
                    LPCSTR attrStr = (LPCSTR)attr->Value.pbData;

                    if (attr->Value.cbData !=
                     name->rgRDN[i].rgRDNAttr[j].Value.cbData)
                        match = FALSE;
                    else if (dwFlags & CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG)
                        match = !strncasecmp(nameStr, attrStr,
                         attr->Value.cbData);
                    else
                        match = !strncmp(nameStr, attrStr, attr->Value.cbData);
                    TRACE("%s : %s => %d\n",
                     debugstr_an(nameStr, attr->Value.cbData),
                     debugstr_an(attrStr, attr->Value.cbData), match);
                }
            }
        }
    }
    return match;
}

BOOL WINAPI CertIsRDNAttrsInCertificateName(DWORD dwCertEncodingType,
 DWORD dwFlags, PCERT_NAME_BLOB pCertName, PCERT_RDN pRDN)
{
    CERT_NAME_INFO *name;
    LPCSTR type;
    DWORD size;
    BOOL ret;

    TRACE("(%08x, %08x, %p, %p)\n", dwCertEncodingType, dwFlags, pCertName,
     pRDN);

    type = dwFlags & CERT_UNICODE_IS_RDN_ATTRS_FLAG ? X509_UNICODE_NAME :
     X509_NAME;
    if ((ret = CryptDecodeObjectEx(dwCertEncodingType, type, pCertName->pbData,
     pCertName->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &name, &size)))
    {
        DWORD i;

        for (i = 0; ret && i < pRDN->cRDNAttr; i++)
            ret = find_matching_rdn_attr(dwFlags, name, &pRDN->rgRDNAttr[i]);
        if (!ret)
            SetLastError(CRYPT_E_NO_MATCH);
        LocalFree(name);
    }
    return ret;
}

LONG WINAPI CertVerifyTimeValidity(LPFILETIME pTimeToVerify,
 PCERT_INFO pCertInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        GetSystemTimeAsFileTime(&fileTime);
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

BOOL WINAPI CertVerifyValidityNesting(PCERT_INFO pSubjectInfo,
 PCERT_INFO pIssuerInfo)
{
    TRACE("(%p, %p)\n", pSubjectInfo, pIssuerInfo);

    return CertVerifyTimeValidity(&pSubjectInfo->NotBefore, pIssuerInfo) == 0
     && CertVerifyTimeValidity(&pSubjectInfo->NotAfter, pIssuerInfo) == 0;
}

BOOL WINAPI CryptHashCertificate(HCRYPTPROV_LEGACY hCryptProv, ALG_ID Algid,
 DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%08lx, %d, %08x, %p, %d, %p, %p)\n", hCryptProv, Algid, dwFlags,
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

BOOL WINAPI CryptHashPublicKeyInfo(HCRYPTPROV_LEGACY hCryptProv, ALG_ID Algid,
 DWORD dwFlags, DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo,
 BYTE *pbComputedHash, DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%08lx, %d, %08x, %d, %p, %p, %p)\n", hCryptProv, Algid, dwFlags,
     dwCertEncodingType, pInfo, pbComputedHash, pcbComputedHash);

    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    if (!Algid)
        Algid = CALG_MD5;
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    if (ret)
    {
        BYTE *buf;
        DWORD size = 0;

        ret = CRYPT_AsnEncodePubKeyInfoNoNull(dwCertEncodingType,
         X509_PUBLIC_KEY_INFO, pInfo, CRYPT_ENCODE_ALLOC_FLAG, NULL,
         (LPBYTE)&buf, &size);
        if (ret)
        {
            ret = CryptCreateHash(hCryptProv, Algid, 0, 0, &hHash);
            if (ret)
            {
                ret = CryptHashData(hHash, buf, size, 0);
                if (ret)
                    ret = CryptGetHashParam(hHash, HP_HASHVAL, pbComputedHash,
                     pcbComputedHash, 0);
                CryptDestroyHash(hHash);
            }
            LocalFree(buf);
        }
    }
    return ret;
}

BOOL WINAPI CryptHashToBeSigned(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 BYTE *pbComputedHash, DWORD *pcbComputedHash)
{
    BOOL ret;
    CERT_SIGNED_CONTENT_INFO *info;
    DWORD size;

    TRACE("(%08lx, %08x, %p, %d, %p, %d)\n", hCryptProv, dwCertEncodingType,
     pbEncoded, cbEncoded, pbComputedHash, *pcbComputedHash);

    ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT,
     pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    if (ret)
    {
        PCCRYPT_OID_INFO oidInfo;
        HCRYPTHASH hHash;

        if (!hCryptProv)
            hCryptProv = CRYPT_GetDefaultProvider();
        oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
         info->SignatureAlgorithm.pszObjId, 0);
        if (!oidInfo)
        {
            SetLastError(NTE_BAD_ALGID);
            ret = FALSE;
        }
        else
        {
            ret = CryptCreateHash(hCryptProv, oidInfo->u.Algid, 0, 0, &hHash);
            if (ret)
            {
                ret = CryptHashData(hHash, info->ToBeSigned.pbData,
                 info->ToBeSigned.cbData, 0);
                if (ret)
                    ret = CryptGetHashParam(hHash, HP_HASHVAL, pbComputedHash,
                     pcbComputedHash, 0);
                CryptDestroyHash(hHash);
            }
        }
        LocalFree(info);
    }
    return ret;
}

BOOL WINAPI CryptSignCertificate(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, const BYTE *pbEncodedToBeSigned,
 DWORD cbEncodedToBeSigned, PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
 const void *pvHashAuxInfo, BYTE *pbSignature, DWORD *pcbSignature)
{
    BOOL ret;
    PCCRYPT_OID_INFO info;
    HCRYPTHASH hHash;

    TRACE("(%08lx, %d, %d, %p, %d, %p, %p, %p, %p)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, pbEncodedToBeSigned, cbEncodedToBeSigned,
     pSignatureAlgorithm, pvHashAuxInfo, pbSignature, pcbSignature);

    info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     pSignatureAlgorithm->pszObjId, 0);
    if (!info)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    if (info->dwGroupId == CRYPT_HASH_ALG_OID_GROUP_ID)
    {
        if (!hCryptProv)
            hCryptProv = CRYPT_GetDefaultProvider();
        ret = CryptCreateHash(hCryptProv, info->u.Algid, 0, 0, &hHash);
        if (ret)
        {
            ret = CryptHashData(hHash, pbEncodedToBeSigned,
             cbEncodedToBeSigned, 0);
            if (ret)
                ret = CryptGetHashParam(hHash, HP_HASHVAL, pbSignature,
                 pcbSignature, 0);
            CryptDestroyHash(hHash);
        }
    }
    else
    {
        if (!hCryptProv)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
        }
        else
        {
            ret = CryptCreateHash(hCryptProv, info->u.Algid, 0, 0, &hHash);
            if (ret)
            {
                ret = CryptHashData(hHash, pbEncodedToBeSigned,
                 cbEncodedToBeSigned, 0);
                if (ret)
                    ret = CryptSignHashW(hHash, dwKeySpec, NULL, 0, pbSignature,
                     pcbSignature);
                CryptDestroyHash(hHash);
            }
        }
    }
    return ret;
}

BOOL WINAPI CryptSignAndEncodeCertificate(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
 const void *pvHashAuxInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD encodedSize, hashSize;

    TRACE("(%08lx, %d, %d, %s, %p, %p, %p, %p, %p)\n", hCryptProv, dwKeySpec,
     dwCertEncodingType, debugstr_a(lpszStructType), pvStructInfo,
     pSignatureAlgorithm, pvHashAuxInfo, pbEncoded, pcbEncoded);

    ret = CryptEncodeObject(dwCertEncodingType, lpszStructType, pvStructInfo,
     NULL, &encodedSize);
    if (ret)
    {
        PBYTE encoded = CryptMemAlloc(encodedSize);

        if (encoded)
        {
            ret = CryptEncodeObject(dwCertEncodingType, lpszStructType,
             pvStructInfo, encoded, &encodedSize);
            if (ret)
            {
                ret = CryptSignCertificate(hCryptProv, dwKeySpec,
                 dwCertEncodingType, encoded, encodedSize, pSignatureAlgorithm,
                 pvHashAuxInfo, NULL, &hashSize);
                if (ret)
                {
                    PBYTE hash = CryptMemAlloc(hashSize);

                    if (hash)
                    {
                        ret = CryptSignCertificate(hCryptProv, dwKeySpec,
                         dwCertEncodingType, encoded, encodedSize,
                         pSignatureAlgorithm, pvHashAuxInfo, hash, &hashSize);
                        if (ret)
                        {
                            CERT_SIGNED_CONTENT_INFO info = { { 0 } };

                            info.ToBeSigned.cbData = encodedSize;
                            info.ToBeSigned.pbData = encoded;
                            info.SignatureAlgorithm = *pSignatureAlgorithm;
                            info.Signature.cbData = hashSize;
                            info.Signature.pbData = hash;
                            info.Signature.cUnusedBits = 0;
                            ret = CryptEncodeObject(dwCertEncodingType,
                             X509_CERT, &info, pbEncoded, pcbEncoded);
                        }
                        CryptMemFree(hash);
                    }
                    else
                        ret = FALSE;
                }
            }
            CryptMemFree(encoded);
        }
        else
            ret = FALSE;
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignature(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 PCERT_PUBLIC_KEY_INFO pPublicKey)
{
    CRYPT_DATA_BLOB blob = { cbEncoded, (BYTE *)pbEncoded };

    return CryptVerifyCertificateSignatureEx(hCryptProv, dwCertEncodingType,
     CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &blob,
     CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, pPublicKey, 0, NULL);
}

static BOOL CRYPT_VerifyCertSignatureFromPublicKeyInfo(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pubKeyInfo,
 const CERT_SIGNED_CONTENT_INFO *signedCert)
{
    BOOL ret;
    HCRYPTKEY key;
    PCCRYPT_OID_INFO info;
    ALG_ID pubKeyID, hashID;

    info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     signedCert->SignatureAlgorithm.pszObjId, 0);
    if (!info || info->dwGroupId != CRYPT_SIGN_ALG_OID_GROUP_ID)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    hashID = info->u.Algid;
    if (info->ExtraInfo.cbData >= sizeof(ALG_ID))
        pubKeyID = *(ALG_ID *)info->ExtraInfo.pbData;
    else
        pubKeyID = hashID;
    /* Load the default provider if necessary */
    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    ret = CryptImportPublicKeyInfoEx(hCryptProv, dwCertEncodingType,
     pubKeyInfo, pubKeyID, 0, NULL, &key);
    if (ret)
    {
        HCRYPTHASH hash;

        ret = CryptCreateHash(hCryptProv, hashID, 0, 0, &hash);
        if (ret)
        {
            ret = CryptHashData(hash, signedCert->ToBeSigned.pbData,
             signedCert->ToBeSigned.cbData, 0);
            if (ret)
                ret = CryptVerifySignatureW(hash, signedCert->Signature.pbData,
                 signedCert->Signature.cbData, key, NULL, 0);
            CryptDestroyHash(hash);
        }
        CryptDestroyKey(key);
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignatureEx(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwCertEncodingType, DWORD dwSubjectType, void *pvSubject,
 DWORD dwIssuerType, void *pvIssuer, DWORD dwFlags, void *pvReserved)
{
    BOOL ret = TRUE;
    CRYPT_DATA_BLOB subjectBlob;

    TRACE("(%08lx, %d, %d, %p, %d, %p, %08x, %p)\n", hCryptProv,
     dwCertEncodingType, dwSubjectType, pvSubject, dwIssuerType, pvIssuer,
     dwFlags, pvReserved);

    switch (dwSubjectType)
    {
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB:
    {
        PCRYPT_DATA_BLOB blob = pvSubject;

        subjectBlob.pbData = blob->pbData;
        subjectBlob.cbData = blob->cbData;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT:
    {
        PCERT_CONTEXT context = pvSubject;

        subjectBlob.pbData = context->pbCertEncoded;
        subjectBlob.cbData = context->cbCertEncoded;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL:
    {
        PCRL_CONTEXT context = pvSubject;

        subjectBlob.pbData = context->pbCrlEncoded;
        subjectBlob.cbData = context->cbCrlEncoded;
        break;
    }
    default:
        SetLastError(E_INVALIDARG);
        ret = FALSE;
    }

    if (ret)
    {
        PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT,
         subjectBlob.pbData, subjectBlob.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &signedCert, &size);
        if (ret)
        {
            switch (dwIssuerType)
            {
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY:
                ret = CRYPT_VerifyCertSignatureFromPublicKeyInfo(hCryptProv,
                 dwCertEncodingType, pvIssuer,
                 signedCert);
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT:
                ret = CRYPT_VerifyCertSignatureFromPublicKeyInfo(hCryptProv,
                 dwCertEncodingType,
                 &((PCCERT_CONTEXT)pvIssuer)->pCertInfo->SubjectPublicKeyInfo,
                 signedCert);
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN:
                FIXME("CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN: stub\n");
                ret = FALSE;
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL:
                if (pvIssuer)
                {
                    SetLastError(E_INVALIDARG);
                    ret = FALSE;
                }
                else
                {
                    FIXME("unimplemented for NULL signer\n");
                    SetLastError(E_INVALIDARG);
                    ret = FALSE;
                }
                break;
            default:
                SetLastError(E_INVALIDARG);
                ret = FALSE;
            }
            LocalFree(signedCert);
        }
    }
    return ret;
}

BOOL WINAPI CertGetIntendedKeyUsage(DWORD dwCertEncodingType,
 PCERT_INFO pCertInfo, BYTE *pbKeyUsage, DWORD cbKeyUsage)
{
    PCERT_EXTENSION ext;
    BOOL ret = FALSE;

    TRACE("(%08x, %p, %p, %d)\n", dwCertEncodingType, pCertInfo, pbKeyUsage,
     cbKeyUsage);

    ext = CertFindExtension(szOID_KEY_USAGE, pCertInfo->cExtension,
     pCertInfo->rgExtension);
    if (ext)
    {
        CRYPT_BIT_BLOB usage;
        DWORD size = sizeof(usage);

        ret = CryptDecodeObjectEx(dwCertEncodingType, X509_BITS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_NOCOPY_FLAG, NULL,
         &usage, &size);
        if (ret)
        {
            if (cbKeyUsage < usage.cbData)
                ret = FALSE;
            else
            {
                memcpy(pbKeyUsage, usage.pbData, usage.cbData);
                if (cbKeyUsage > usage.cbData)
                    memset(pbKeyUsage + usage.cbData, 0,
                     cbKeyUsage - usage.cbData);
            }
        }
    }
    else
        SetLastError(0);
    return ret;
}

BOOL WINAPI CertGetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext, DWORD dwFlags,
 PCERT_ENHKEY_USAGE pUsage, DWORD *pcbUsage)
{
    PCERT_ENHKEY_USAGE usage = NULL;
    DWORD bytesNeeded;
    BOOL ret = TRUE;

    if (!pCertContext || !pcbUsage)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("(%p, %08x, %p, %d)\n", pCertContext, dwFlags, pUsage, *pcbUsage);

    if (!(dwFlags & CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG))
    {
        DWORD propSize = 0;

        if (CertGetCertificateContextProperty(pCertContext,
         CERT_ENHKEY_USAGE_PROP_ID, NULL, &propSize))
        {
            LPBYTE buf = CryptMemAlloc(propSize);

            if (buf)
            {
                if (CertGetCertificateContextProperty(pCertContext,
                 CERT_ENHKEY_USAGE_PROP_ID, buf, &propSize))
                {
                    ret = CryptDecodeObjectEx(pCertContext->dwCertEncodingType,
                     X509_ENHANCED_KEY_USAGE, buf, propSize,
                     CRYPT_ENCODE_ALLOC_FLAG, NULL, &usage, &bytesNeeded);
                }
                CryptMemFree(buf);
            }
        }
    }
    if (!usage && !(dwFlags & CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG))
    {
        PCERT_EXTENSION ext = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
         pCertContext->pCertInfo->cExtension,
         pCertContext->pCertInfo->rgExtension);

        if (ext)
        {
            ret = CryptDecodeObjectEx(pCertContext->dwCertEncodingType,
             X509_ENHANCED_KEY_USAGE, ext->Value.pbData, ext->Value.cbData,
             CRYPT_ENCODE_ALLOC_FLAG, NULL, &usage, &bytesNeeded);
        }
    }
    if (!usage)
    {
        /* If a particular location is specified, this should fail.  Otherwise
         * it should succeed with an empty usage.  (This is true on Win2k and
         * later, which we emulate.)
         */
        if (dwFlags)
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = FALSE;
        }
        else
            bytesNeeded = sizeof(CERT_ENHKEY_USAGE);
    }

    if (ret)
    {
        if (!pUsage)
            *pcbUsage = bytesNeeded;
        else if (*pcbUsage < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbUsage = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            *pcbUsage = bytesNeeded;
            if (usage)
            {
                DWORD i;
                LPSTR nextOID = (LPSTR)((LPBYTE)pUsage +
                 sizeof(CERT_ENHKEY_USAGE) +
                 usage->cUsageIdentifier * sizeof(LPSTR));

                pUsage->cUsageIdentifier = usage->cUsageIdentifier;
                pUsage->rgpszUsageIdentifier = (LPSTR *)((LPBYTE)pUsage +
                 sizeof(CERT_ENHKEY_USAGE));
                for (i = 0; i < usage->cUsageIdentifier; i++)
                {
                    pUsage->rgpszUsageIdentifier[i] = nextOID;
                    strcpy(nextOID, usage->rgpszUsageIdentifier[i]);
                    nextOID += strlen(nextOID) + 1;
                }
            }
            else
                pUsage->cUsageIdentifier = 0;
        }
    }
    if (usage)
        LocalFree(usage);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext,
 PCERT_ENHKEY_USAGE pUsage)
{
    BOOL ret;

    TRACE("(%p, %p)\n", pCertContext, pUsage);

    if (pUsage)
    {
        CRYPT_DATA_BLOB blob = { 0, NULL };

        ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE,
         pUsage, CRYPT_ENCODE_ALLOC_FLAG, NULL, &blob.pbData, &blob.cbData);
        if (ret)
        {
            ret = CertSetCertificateContextProperty(pCertContext,
             CERT_ENHKEY_USAGE_PROP_ID, 0, &blob);
            LocalFree(blob.pbData);
        }
    }
    else
        ret = CertSetCertificateContextProperty(pCertContext,
         CERT_ENHKEY_USAGE_PROP_ID, 0, NULL);
    return ret;
}

BOOL WINAPI CertAddEnhancedKeyUsageIdentifier(PCCERT_CONTEXT pCertContext,
 LPCSTR pszUsageIdentifier)
{
    BOOL ret;
    DWORD size;

    TRACE("(%p, %s)\n", pCertContext, debugstr_a(pszUsageIdentifier));

    if (CertGetEnhancedKeyUsage(pCertContext,
     CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &size))
    {
        PCERT_ENHKEY_USAGE usage = CryptMemAlloc(size);

        if (usage)
        {
            ret = CertGetEnhancedKeyUsage(pCertContext,
             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, usage, &size);
            if (ret)
            {
                DWORD i;
                BOOL exists = FALSE;

                /* Make sure usage doesn't already exist */
                for (i = 0; !exists && i < usage->cUsageIdentifier; i++)
                {
                    if (!strcmp(usage->rgpszUsageIdentifier[i],
                     pszUsageIdentifier))
                        exists = TRUE;
                }
                if (!exists)
                {
                    PCERT_ENHKEY_USAGE newUsage = CryptMemAlloc(size +
                     sizeof(LPSTR) + strlen(pszUsageIdentifier) + 1);

                    if (newUsage)
                    {
                        LPSTR nextOID;

                        newUsage->rgpszUsageIdentifier = (LPSTR *)
                         ((LPBYTE)newUsage + sizeof(CERT_ENHKEY_USAGE));
                        nextOID = (LPSTR)((LPBYTE)newUsage->rgpszUsageIdentifier
                          + (usage->cUsageIdentifier + 1) * sizeof(LPSTR));
                        for (i = 0; i < usage->cUsageIdentifier; i++)
                        {
                            newUsage->rgpszUsageIdentifier[i] = nextOID;
                            strcpy(nextOID, usage->rgpszUsageIdentifier[i]);
                            nextOID += strlen(nextOID) + 1;
                        }
                        newUsage->rgpszUsageIdentifier[i] = nextOID;
                        strcpy(nextOID, pszUsageIdentifier);
                        newUsage->cUsageIdentifier = i + 1;
                        ret = CertSetEnhancedKeyUsage(pCertContext, newUsage);
                        CryptMemFree(newUsage);
                    }
                    else
                        ret = FALSE;
                }
            }
            CryptMemFree(usage);
        }
        else
            ret = FALSE;
    }
    else
    {
        PCERT_ENHKEY_USAGE usage = CryptMemAlloc(sizeof(CERT_ENHKEY_USAGE) +
         sizeof(LPSTR) + strlen(pszUsageIdentifier) + 1);

        if (usage)
        {
            usage->rgpszUsageIdentifier =
             (LPSTR *)((LPBYTE)usage + sizeof(CERT_ENHKEY_USAGE));
            usage->rgpszUsageIdentifier[0] = (LPSTR)((LPBYTE)usage +
             sizeof(CERT_ENHKEY_USAGE) + sizeof(LPSTR));
            strcpy(usage->rgpszUsageIdentifier[0], pszUsageIdentifier);
            usage->cUsageIdentifier = 1;
            ret = CertSetEnhancedKeyUsage(pCertContext, usage);
            CryptMemFree(usage);
        }
        else
            ret = FALSE;
    }
    return ret;
}

BOOL WINAPI CertRemoveEnhancedKeyUsageIdentifier(PCCERT_CONTEXT pCertContext,
 LPCSTR pszUsageIdentifier)
{
    BOOL ret;
    DWORD size;
    CERT_ENHKEY_USAGE usage;

    TRACE("(%p, %s)\n", pCertContext, debugstr_a(pszUsageIdentifier));

    size = sizeof(usage);
    ret = CertGetEnhancedKeyUsage(pCertContext,
     CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, &usage, &size);
    if (!ret && GetLastError() == ERROR_MORE_DATA)
    {
        PCERT_ENHKEY_USAGE pUsage = CryptMemAlloc(size);

        if (pUsage)
        {
            ret = CertGetEnhancedKeyUsage(pCertContext,
             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
            if (ret)
            {
                if (pUsage->cUsageIdentifier)
                {
                    DWORD i;
                    BOOL found = FALSE;

                    for (i = 0; i < pUsage->cUsageIdentifier; i++)
                    {
                        if (!strcmp(pUsage->rgpszUsageIdentifier[i],
                         pszUsageIdentifier))
                            found = TRUE;
                        if (found && i < pUsage->cUsageIdentifier - 1)
                            pUsage->rgpszUsageIdentifier[i] =
                             pUsage->rgpszUsageIdentifier[i + 1];
                    }
                    pUsage->cUsageIdentifier--;
                    /* Remove the usage if it's empty */
                    if (pUsage->cUsageIdentifier)
                        ret = CertSetEnhancedKeyUsage(pCertContext, pUsage);
                    else
                        ret = CertSetEnhancedKeyUsage(pCertContext, NULL);
                }
            }
            CryptMemFree(pUsage);
        }
        else
            ret = FALSE;
    }
    else
    {
        /* it fit in an empty usage, therefore there's nothing to remove */
        ret = TRUE;
    }
    return ret;
}

struct BitField
{
    DWORD  cIndexes;
    DWORD *indexes;
};

#define BITS_PER_DWORD (sizeof(DWORD) * 8)

static void CRYPT_SetBitInField(struct BitField *field, DWORD bit)
{
    DWORD indexIndex = bit / BITS_PER_DWORD;

    if (indexIndex + 1 > field->cIndexes)
    {
        if (field->cIndexes)
            field->indexes = CryptMemRealloc(field->indexes,
             (indexIndex + 1) * sizeof(DWORD));
        else
            field->indexes = CryptMemAlloc(sizeof(DWORD));
        if (field->indexes)
        {
            field->indexes[indexIndex] = 0;
            field->cIndexes = indexIndex + 1;
        }
    }
    if (field->indexes)
        field->indexes[indexIndex] |= 1 << (bit % BITS_PER_DWORD);
}

static BOOL CRYPT_IsBitInFieldSet(const struct BitField *field, DWORD bit)
{
    BOOL set;
    DWORD indexIndex = bit / BITS_PER_DWORD;

    assert(field->cIndexes);
    set = field->indexes[indexIndex] & (1 << (bit % BITS_PER_DWORD));
    return set;
}

BOOL WINAPI CertGetValidUsages(DWORD cCerts, PCCERT_CONTEXT *rghCerts,
 int *cNumOIDs, LPSTR *rghOIDs, DWORD *pcbOIDs)
{
    BOOL ret = TRUE;
    DWORD i, cbOIDs = 0;
    BOOL allUsagesValid = TRUE;
    CERT_ENHKEY_USAGE validUsages = { 0, NULL };

    TRACE("(%d, %p, %d, %p, %d)\n", cCerts, rghCerts, *cNumOIDs,
     rghOIDs, *pcbOIDs);

    for (i = 0; i < cCerts; i++)
    {
        CERT_ENHKEY_USAGE usage;
        DWORD size = sizeof(usage);

        ret = CertGetEnhancedKeyUsage(rghCerts[i], 0, &usage, &size);
        /* Success is deliberately ignored: it implies all usages are valid */
        if (!ret && GetLastError() == ERROR_MORE_DATA)
        {
            PCERT_ENHKEY_USAGE pUsage = CryptMemAlloc(size);

            allUsagesValid = FALSE;
            if (pUsage)
            {
                ret = CertGetEnhancedKeyUsage(rghCerts[i], 0, pUsage, &size);
                if (ret)
                {
                    if (!validUsages.cUsageIdentifier)
                    {
                        DWORD j;

                        cbOIDs = pUsage->cUsageIdentifier * sizeof(LPSTR);
                        validUsages.cUsageIdentifier = pUsage->cUsageIdentifier;
                        for (j = 0; j < validUsages.cUsageIdentifier; j++)
                            cbOIDs += lstrlenA(pUsage->rgpszUsageIdentifier[j])
                             + 1;
                        validUsages.rgpszUsageIdentifier =
                         CryptMemAlloc(cbOIDs);
                        if (validUsages.rgpszUsageIdentifier)
                        {
                            LPSTR nextOID = (LPSTR)
                             ((LPBYTE)validUsages.rgpszUsageIdentifier +
                             validUsages.cUsageIdentifier * sizeof(LPSTR));

                            for (j = 0; j < validUsages.cUsageIdentifier; j++)
                            {
                                validUsages.rgpszUsageIdentifier[j] = nextOID;
                                lstrcpyA(validUsages.rgpszUsageIdentifier[j],
                                 pUsage->rgpszUsageIdentifier[j]);
                                nextOID += lstrlenA(nextOID) + 1;
                            }
                        }
                    }
                    else
                    {
                        struct BitField validIndexes = { 0, NULL };
                        DWORD j, k, numRemoved = 0;

                        /* Merge: build a bitmap of all the indexes of
                         * validUsages.rgpszUsageIdentifier that are in pUsage.
                         */
                        for (j = 0; j < pUsage->cUsageIdentifier; j++)
                        {
                            for (k = 0; k < validUsages.cUsageIdentifier; k++)
                            {
                                if (!strcmp(pUsage->rgpszUsageIdentifier[j],
                                 validUsages.rgpszUsageIdentifier[k]))
                                {
                                    CRYPT_SetBitInField(&validIndexes, k);
                                    break;
                                }
                            }
                        }
                        /* Merge by removing from validUsages those that are
                         * not in the bitmap.
                         */
                        for (j = 0; j < validUsages.cUsageIdentifier; j++)
                        {
                            if (!CRYPT_IsBitInFieldSet(&validIndexes, j))
                            {
                                if (j < validUsages.cUsageIdentifier - 1)
                                {
                                    memmove(&validUsages.rgpszUsageIdentifier[j],
                                     &validUsages.rgpszUsageIdentifier[j +
                                     numRemoved + 1],
                                     (validUsages.cUsageIdentifier - numRemoved
                                     - j - 1) * sizeof(LPSTR));
                                    cbOIDs -= lstrlenA(
                                     validUsages.rgpszUsageIdentifier[j]) + 1 +
                                     sizeof(LPSTR);
                                    validUsages.cUsageIdentifier--;
                                    numRemoved++;
                                }
                                else
                                    validUsages.cUsageIdentifier--;
                            }
                        }
                        CryptMemFree(validIndexes.indexes);
                    }
                }
                CryptMemFree(pUsage);
            }
        }
    }
    ret = TRUE;
    if (allUsagesValid)
    {
        *cNumOIDs = -1;
        *pcbOIDs = 0;
    }
    else
    {
        *cNumOIDs = validUsages.cUsageIdentifier;
        if (!rghOIDs)
            *pcbOIDs = cbOIDs;
        else if (*pcbOIDs < cbOIDs)
        {
            *pcbOIDs = cbOIDs;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            LPSTR nextOID = (LPSTR)((LPBYTE)rghOIDs +
             validUsages.cUsageIdentifier * sizeof(LPSTR));

            *pcbOIDs = cbOIDs;
            for (i = 0; i < validUsages.cUsageIdentifier; i++)
            {
                rghOIDs[i] = nextOID;
                lstrcpyA(nextOID, validUsages.rgpszUsageIdentifier[i]);
                nextOID += lstrlenA(nextOID) + 1;
            }
        }
    }
    CryptMemFree(validUsages.rgpszUsageIdentifier);
    TRACE("cNumOIDs: %d\n", *cNumOIDs);
    TRACE("returning %d\n", ret);
    return ret;
}

/* Sets the CERT_KEY_PROV_INFO_PROP_ID property of context from pInfo, or, if
 * pInfo is NULL, from the attributes of hProv.
 */
static void CertContext_SetKeyProvInfo(PCCERT_CONTEXT context,
 const CRYPT_KEY_PROV_INFO *pInfo, HCRYPTPROV hProv)
{
    CRYPT_KEY_PROV_INFO info = { 0 };
    BOOL ret;

    if (!pInfo)
    {
        DWORD size;
        int len;

        ret = CryptGetProvParam(hProv, PP_CONTAINER, NULL, &size, 0);
        if (ret)
        {
            LPSTR szContainer = CryptMemAlloc(size);

            if (szContainer)
            {
                ret = CryptGetProvParam(hProv, PP_CONTAINER,
                 (BYTE *)szContainer, &size, 0);
                if (ret)
                {
                    len = MultiByteToWideChar(CP_ACP, 0, szContainer, -1,
                     NULL, 0);
                    if (len)
                    {
                        info.pwszContainerName = CryptMemAlloc(len *
                         sizeof(WCHAR));
                        MultiByteToWideChar(CP_ACP, 0, szContainer, -1,
                         info.pwszContainerName, len);
                    }
                }
                CryptMemFree(szContainer);
            }
        }
        ret = CryptGetProvParam(hProv, PP_NAME, NULL, &size, 0);
        if (ret)
        {
            LPSTR szProvider = CryptMemAlloc(size);

            if (szProvider)
            {
                ret = CryptGetProvParam(hProv, PP_NAME, (BYTE *)szProvider,
                 &size, 0);
                if (ret)
                {
                    len = MultiByteToWideChar(CP_ACP, 0, szProvider, -1,
                     NULL, 0);
                    if (len)
                    {
                        info.pwszProvName = CryptMemAlloc(len *
                         sizeof(WCHAR));
                        MultiByteToWideChar(CP_ACP, 0, szProvider, -1,
                         info.pwszProvName, len);
                    }
                }
                CryptMemFree(szProvider);
            }
        }
        /* in case no CRYPT_KEY_PROV_INFO given,
         *  we always use AT_SIGNATURE key spec
         */
        info.dwKeySpec = AT_SIGNATURE;
        size = sizeof(info.dwProvType);
        ret = CryptGetProvParam(hProv, PP_PROVTYPE, (LPBYTE)&info.dwProvType,
         &size, 0);
        if (!ret)
            info.dwProvType = PROV_RSA_FULL;
        pInfo = &info;
    }

    CertSetCertificateContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID,
     0, pInfo);

    if (pInfo == &info)
    {
        CryptMemFree(info.pwszContainerName);
        CryptMemFree(info.pwszProvName);
    }
}

/* Creates a signed certificate context from the unsigned, encoded certificate
 * in blob, using the crypto provider hProv and the signature algorithm sigAlgo.
 */
static PCCERT_CONTEXT CRYPT_CreateSignedCert(const CRYPT_DER_BLOB *blob,
 HCRYPTPROV hProv, DWORD dwKeySpec, PCRYPT_ALGORITHM_IDENTIFIER sigAlgo)
{
    PCCERT_CONTEXT context = NULL;
    BOOL ret;
    DWORD sigSize = 0;

    ret = CryptSignCertificate(hProv, dwKeySpec, X509_ASN_ENCODING,
     blob->pbData, blob->cbData, sigAlgo, NULL, NULL, &sigSize);
    if (ret)
    {
        LPBYTE sig = CryptMemAlloc(sigSize);

        ret = CryptSignCertificate(hProv, dwKeySpec, X509_ASN_ENCODING,
         blob->pbData, blob->cbData, sigAlgo, NULL, sig, &sigSize);
        if (ret)
        {
            CERT_SIGNED_CONTENT_INFO signedInfo;
            BYTE *encodedSignedCert = NULL;
            DWORD encodedSignedCertSize = 0;

            signedInfo.ToBeSigned.cbData = blob->cbData;
            signedInfo.ToBeSigned.pbData = blob->pbData;
            signedInfo.SignatureAlgorithm = *sigAlgo;
            signedInfo.Signature.cbData = sigSize;
            signedInfo.Signature.pbData = sig;
            signedInfo.Signature.cUnusedBits = 0;
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CERT,
             &signedInfo, CRYPT_ENCODE_ALLOC_FLAG, NULL,
             &encodedSignedCert, &encodedSignedCertSize);
            if (ret)
            {
                context = CertCreateCertificateContext(X509_ASN_ENCODING,
                 encodedSignedCert, encodedSignedCertSize);
                LocalFree(encodedSignedCert);
            }
        }
        CryptMemFree(sig);
    }
    return context;
}

/* Copies data from the parameters into info, where:
 * pSerialNumber: The serial number.  Must not be NULL.
 * pSubjectIssuerBlob: Specifies both the subject and issuer for info.
 *                     Must not be NULL
 * pSignatureAlgorithm: Optional.
 * pStartTime: The starting time of the certificate.  If NULL, the current
 *             system time is used.
 * pEndTime: The ending time of the certificate.  If NULL, one year past the
 *           starting time is used.
 * pubKey: The public key of the certificate.  Must not be NULL.
 * pExtensions: Extensions to be included with the certificate.  Optional.
 */
static void CRYPT_MakeCertInfo(PCERT_INFO info, const CRYPT_DATA_BLOB *pSerialNumber,
 const CERT_NAME_BLOB *pSubjectIssuerBlob,
 const CRYPT_ALGORITHM_IDENTIFIER *pSignatureAlgorithm, const SYSTEMTIME *pStartTime,
 const SYSTEMTIME *pEndTime, const CERT_PUBLIC_KEY_INFO *pubKey,
 const CERT_EXTENSIONS *pExtensions)
{
    static CHAR oid[] = szOID_RSA_SHA1RSA;

    assert(info);
    assert(pSerialNumber);
    assert(pSubjectIssuerBlob);
    assert(pubKey);

    if (pExtensions && pExtensions->cExtension)
        info->dwVersion = CERT_V3;
    else
        info->dwVersion = CERT_V1;
    info->SerialNumber.cbData = pSerialNumber->cbData;
    info->SerialNumber.pbData = pSerialNumber->pbData;
    if (pSignatureAlgorithm)
        info->SignatureAlgorithm = *pSignatureAlgorithm;
    else
    {
        info->SignatureAlgorithm.pszObjId = oid;
        info->SignatureAlgorithm.Parameters.cbData = 0;
        info->SignatureAlgorithm.Parameters.pbData = NULL;
    }
    info->Issuer.cbData = pSubjectIssuerBlob->cbData;
    info->Issuer.pbData = pSubjectIssuerBlob->pbData;
    if (pStartTime)
        SystemTimeToFileTime(pStartTime, &info->NotBefore);
    else
        GetSystemTimeAsFileTime(&info->NotBefore);
    if (pEndTime)
        SystemTimeToFileTime(pEndTime, &info->NotAfter);
    else
    {
        SYSTEMTIME endTime;

        if (FileTimeToSystemTime(&info->NotBefore, &endTime))
        {
            endTime.wYear++;
            SystemTimeToFileTime(&endTime, &info->NotAfter);
        }
    }
    info->Subject.cbData = pSubjectIssuerBlob->cbData;
    info->Subject.pbData = pSubjectIssuerBlob->pbData;
    info->SubjectPublicKeyInfo = *pubKey;
    if (pExtensions)
    {
        info->cExtension = pExtensions->cExtension;
        info->rgExtension = pExtensions->rgExtension;
    }
    else
    {
        info->cExtension = 0;
        info->rgExtension = NULL;
    }
}
 
typedef RPC_STATUS (RPC_ENTRY *UuidCreateFunc)(UUID *);
typedef RPC_STATUS (RPC_ENTRY *UuidToStringFunc)(UUID *, unsigned char **);
typedef RPC_STATUS (RPC_ENTRY *RpcStringFreeFunc)(unsigned char **);

static HCRYPTPROV CRYPT_CreateKeyProv(void)
{
    HCRYPTPROV hProv = 0;
    HMODULE rpcrt = LoadLibraryA("rpcrt4");

    if (rpcrt)
    {
        UuidCreateFunc uuidCreate = (UuidCreateFunc)GetProcAddress(rpcrt,
         "UuidCreate");
        UuidToStringFunc uuidToString = (UuidToStringFunc)GetProcAddress(rpcrt,
         "UuidToStringA");
        RpcStringFreeFunc rpcStringFree = (RpcStringFreeFunc)GetProcAddress(
         rpcrt, "RpcStringFreeA");

        if (uuidCreate && uuidToString && rpcStringFree)
        {
            UUID uuid;
            RPC_STATUS status = uuidCreate(&uuid);

            if (status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY)
            {
                unsigned char *uuidStr;

                status = uuidToString(&uuid, &uuidStr);
                if (status == RPC_S_OK)
                {
                    BOOL ret = CryptAcquireContextA(&hProv, (LPCSTR)uuidStr,
                     MS_DEF_PROV_A, PROV_RSA_FULL, CRYPT_NEWKEYSET);

                    if (ret)
                    {
                        HCRYPTKEY key;

                        ret = CryptGenKey(hProv, AT_SIGNATURE, 0, &key);
                        if (ret)
                            CryptDestroyKey(key);
                    }
                    rpcStringFree(&uuidStr);
                }
            }
        }
        FreeLibrary(rpcrt);
    }
    return hProv;
}

PCCERT_CONTEXT WINAPI CertCreateSelfSignCertificate(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hProv,
 PCERT_NAME_BLOB pSubjectIssuerBlob, DWORD dwFlags,
 PCRYPT_KEY_PROV_INFO pKeyProvInfo,
 PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm, PSYSTEMTIME pStartTime,
 PSYSTEMTIME pEndTime, PCERT_EXTENSIONS pExtensions)
{
    PCCERT_CONTEXT context = NULL;
    BOOL ret, releaseContext = FALSE;
    PCERT_PUBLIC_KEY_INFO pubKey = NULL;
    DWORD pubKeySize = 0, dwKeySpec;

    TRACE("(%08lx, %p, %08x, %p, %p, %p, %p, %p)\n", hProv,
     pSubjectIssuerBlob, dwFlags, pKeyProvInfo, pSignatureAlgorithm, pStartTime,
     pExtensions, pExtensions);

    if(!pSubjectIssuerBlob)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    dwKeySpec = pKeyProvInfo ? pKeyProvInfo->dwKeySpec : AT_SIGNATURE;
    if (!hProv)
    {
        if (!pKeyProvInfo)
        {
            hProv = CRYPT_CreateKeyProv();
            releaseContext = TRUE;
        }
        else if (pKeyProvInfo->dwFlags & CERT_SET_KEY_PROV_HANDLE_PROP_ID)
        {
            SetLastError(NTE_BAD_FLAGS);
            return NULL;
        }
        else
        {
            HCRYPTKEY hKey = 0;
            /* acquire the context using the given information*/
            ret = CryptAcquireContextW(&hProv,pKeyProvInfo->pwszContainerName,
                    pKeyProvInfo->pwszProvName,pKeyProvInfo->dwProvType,
                    pKeyProvInfo->dwFlags);
            if (!ret)
            {
	        if(GetLastError() != NTE_BAD_KEYSET)
                    return NULL;
                /* create the key set */
                ret = CryptAcquireContextW(&hProv,pKeyProvInfo->pwszContainerName,
                    pKeyProvInfo->pwszProvName,pKeyProvInfo->dwProvType,
                    pKeyProvInfo->dwFlags|CRYPT_NEWKEYSET);
                if (!ret)
                    return NULL;
	    }
            /* check if the key is here */
            ret = CryptGetUserKey(hProv,dwKeySpec,&hKey);
            if(!ret)
            {
                if (NTE_NO_KEY == GetLastError())
                { /* generate the key */
                    ret = CryptGenKey(hProv,dwKeySpec,0,&hKey);
                }
                if (!ret)
                {
                    CryptReleaseContext(hProv,0);
                    SetLastError(NTE_BAD_KEYSET);
                    return NULL;
                }
            }
            CryptDestroyKey(hKey);
            releaseContext = TRUE;
        }
    }

    ret = CryptExportPublicKeyInfo(hProv, dwKeySpec, X509_ASN_ENCODING, NULL,
     &pubKeySize);
    if (!ret)
        goto end;
    pubKey = CryptMemAlloc(pubKeySize);
    if (pubKey)
    {
        ret = CryptExportPublicKeyInfo(hProv, dwKeySpec, X509_ASN_ENCODING,
         pubKey, &pubKeySize);
        if (ret)
        {
            CERT_INFO info = { 0 };
            CRYPT_DER_BLOB blob = { 0, NULL };
            BYTE serial[16];
            CRYPT_DATA_BLOB serialBlob = { sizeof(serial), serial };

            CryptGenRandom(hProv, sizeof(serial), serial);
            CRYPT_MakeCertInfo(&info, &serialBlob, pSubjectIssuerBlob,
             pSignatureAlgorithm, pStartTime, pEndTime, pubKey, pExtensions);
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
             &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &blob.pbData,
             &blob.cbData);
            if (ret)
            {
                if (!(dwFlags & CERT_CREATE_SELFSIGN_NO_SIGN))
                    context = CRYPT_CreateSignedCert(&blob, hProv,dwKeySpec,
                     &info.SignatureAlgorithm);
                else
                    context = CertCreateCertificateContext(X509_ASN_ENCODING,
                     blob.pbData, blob.cbData);
                if (context && !(dwFlags & CERT_CREATE_SELFSIGN_NO_KEY_INFO))
                    CertContext_SetKeyProvInfo(context, pKeyProvInfo, hProv);
                LocalFree(blob.pbData);
            }
        }
        CryptMemFree(pubKey);
    }
end:
    if (releaseContext)
        CryptReleaseContext(hProv, 0);
    return context;
}

BOOL WINAPI CertVerifyCTLUsage(DWORD dwEncodingType, DWORD dwSubjectType,
                               void *pvSubject, PCTL_USAGE pSubjectUsage, DWORD dwFlags,
                               PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
                               PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus)
{
    FIXME("(0x%x, %d, %p, %p, 0x%x, %p, %p): stub\n", dwEncodingType,
          dwSubjectType, pvSubject, pSubjectUsage, dwFlags, pVerifyUsagePara,
          pVerifyUsageStatus);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

const void * WINAPI CertCreateContext(DWORD dwContextType, DWORD dwEncodingType,
                                      const BYTE *pbEncoded, DWORD cbEncoded,
                                      DWORD dwFlags, PCERT_CREATE_CONTEXT_PARA pCreatePara)
{
    TRACE("(0x%x, 0x%x, %p, %d, 0x%08x, %p)\n", dwContextType, dwEncodingType,
          pbEncoded, cbEncoded, dwFlags, pCreatePara);

    if (dwFlags)
    {
        FIXME("dwFlags 0x%08x not handled\n", dwFlags);
        return NULL;
    }
    if (pCreatePara)
    {
        FIXME("pCreatePara not handled\n");
        return NULL;
    }

    switch (dwContextType)
    {
    case CERT_STORE_CERTIFICATE_CONTEXT:
        return CertCreateCertificateContext(dwEncodingType, pbEncoded, cbEncoded);
    case CERT_STORE_CRL_CONTEXT:
        return CertCreateCRLContext(dwEncodingType, pbEncoded, cbEncoded);
    case CERT_STORE_CTL_CONTEXT:
        return CertCreateCTLContext(dwEncodingType, pbEncoded, cbEncoded);
    default:
        WARN("unknown context type: 0x%x\n", dwContextType);
        return NULL;
    }
}
