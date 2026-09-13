/*
 * Copyright 2006 Juan Lang
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

static void CRL_free(context_t *context)
{
    crl_t *crl = (crl_t*)context;

    CryptMemFree(crl->ctx.pbCrlEncoded);
    LocalFree(crl->ctx.pCrlInfo);
}

static const context_vtbl_t crl_vtbl;

static char *copy_string(char *p, char **dst, const char *src)
{
    size_t size = strlen(src) + 1;

    *dst = memcpy(p, src, size);
    return p + size;
}

static char *copy_blob(char *p, DATA_BLOB *dst, const DATA_BLOB *src)
{
    size_t size = src->cbData;

    dst->cbData = size;
    dst->pbData = memcpy(p, src->pbData, size);
    return p + size;
}

static char *copy_extension(char *p, CERT_EXTENSION *dst, const CERT_EXTENSION *src)
{
    p = copy_string(p, &dst->pszObjId, src->pszObjId);
    dst->fCritical = src->fCritical;
    return copy_blob(p, &dst->Value, &src->Value);
}

static CRL_INFO *clone_crl_info(const CRL_INFO *src)
{
    size_t size = sizeof(CRL_INFO);
    CRL_INFO *dst;
    DWORD i, j;
    char *p;

    if (src->SignatureAlgorithm.pszObjId)
        size += strlen(src->SignatureAlgorithm.pszObjId) + 1;
    size += src->SignatureAlgorithm.Parameters.cbData;
    size += src->Issuer.cbData;
    for (i = 0; i < src->cCRLEntry; ++i)
    {
        const CRL_ENTRY *entry = &src->rgCRLEntry[i];

        size += sizeof(CRL_ENTRY);
        size += entry->SerialNumber.cbData;
        for (j = 0; j < entry->cExtension; ++j)
        {
            const CERT_EXTENSION *ext = &entry->rgExtension[j];

            size += sizeof(CERT_EXTENSION);
            size += strlen(ext->pszObjId) + 1;
            size += ext->Value.cbData;
        }
    }

    for (j = 0; j < src->cExtension; ++j)
    {
        const CERT_EXTENSION *ext = &src->rgExtension[j];

        size += sizeof(CERT_EXTENSION);
        size += strlen(ext->pszObjId) + 1;
        size += ext->Value.cbData;
    }

    if (!(dst = LocalAlloc(LPTR, size)))
        return NULL;
    p = (char *)(dst + 1);

    dst->dwVersion = src->dwVersion;
    if (src->SignatureAlgorithm.pszObjId)
        p = copy_string(p, &dst->SignatureAlgorithm.pszObjId, src->SignatureAlgorithm.pszObjId);
    p = copy_blob(p, &dst->SignatureAlgorithm.Parameters, &src->SignatureAlgorithm.Parameters);
    p = copy_blob(p, &dst->Issuer, &src->Issuer);
    dst->ThisUpdate = src->ThisUpdate;
    dst->NextUpdate = src->NextUpdate;

    dst->cCRLEntry = src->cCRLEntry;
    dst->rgCRLEntry = (CRL_ENTRY *)p;
    p += src->cCRLEntry * sizeof(CRL_ENTRY);

    dst->cExtension = src->cExtension;
    dst->rgExtension = (CERT_EXTENSION *)p;
    p += src->cExtension * sizeof(CERT_EXTENSION);

    for (i = 0; i < src->cCRLEntry; ++i)
    {
        const CRL_ENTRY *src_entry = &src->rgCRLEntry[i];
        CRL_ENTRY *dst_entry = &dst->rgCRLEntry[i];

        p = copy_blob(p, &dst_entry->SerialNumber, &src_entry->SerialNumber);
        dst_entry->RevocationDate = src_entry->RevocationDate;
        dst_entry->cExtension = src_entry->cExtension;
        dst_entry->rgExtension = (CERT_EXTENSION *)p;
        p += src_entry->cExtension * sizeof(CERT_EXTENSION);

        for (j = 0; j < src_entry->cExtension; ++j)
            p = copy_extension(p, &dst_entry->rgExtension[j], &src_entry->rgExtension[j]);
    }

    for (j = 0; j < src->cExtension; ++j)
        p = copy_extension(p, &dst->rgExtension[j], &src->rgExtension[j]);

    assert(p - (char *)dst == size);
    return dst;
}

static context_t *CRL_clone(context_t *context, WINECRYPT_CERTSTORE *store, BOOL use_link)
{
    crl_t *dst;

    if(use_link) {
        if (!(dst = (crl_t *)Context_CreateLinkContext(sizeof(CRL_CONTEXT), context, store)))
            return NULL;
    }else {
        const crl_t *src = (const crl_t*)context;

        if (!(dst = (crl_t *)Context_CreateDataContext(sizeof(CRL_CONTEXT), &crl_vtbl, store)))
            return NULL;

        Context_CopyProperties(&dst->ctx, &src->ctx);

        dst->ctx.dwCertEncodingType = src->ctx.dwCertEncodingType;
        dst->ctx.pbCrlEncoded = CryptMemAlloc(src->ctx.cbCrlEncoded);
        memcpy(dst->ctx.pbCrlEncoded, src->ctx.pbCrlEncoded, src->ctx.cbCrlEncoded);
        dst->ctx.cbCrlEncoded = src->ctx.cbCrlEncoded;

        if (!(dst->ctx.pCrlInfo = clone_crl_info(src->ctx.pCrlInfo)))
        {
            CertFreeCRLContext(&dst->ctx);
            return NULL;
        }
    }

    dst->ctx.hCertStore = store;
    return &dst->base;
}

static const context_vtbl_t crl_vtbl = {
    CRL_free,
    CRL_clone
};

PCCRL_CONTEXT WINAPI CertCreateCRLContext(DWORD dwCertEncodingType,
 const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    crl_t *crl = NULL;
    BOOL ret;
    PCRL_INFO crlInfo = NULL;
    BYTE *data = NULL;
    DWORD size = 0;

    TRACE("(%08lx, %p, %ld)\n", dwCertEncodingType, pbCrlEncoded,
     cbCrlEncoded);

    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT_CRL_TO_BE_SIGNED,
     pbCrlEncoded, cbCrlEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &crlInfo, &size);
    if (!ret)
        return NULL;

    crl = (crl_t*)Context_CreateDataContext(sizeof(CRL_CONTEXT), &crl_vtbl, &empty_store);
    if (!crl)
        return NULL;

    data = CryptMemAlloc(cbCrlEncoded);
    if (!data)
    {
        Context_Release(&crl->base);
        return NULL;
    }

    memcpy(data, pbCrlEncoded, cbCrlEncoded);
    crl->ctx.dwCertEncodingType = dwCertEncodingType;
    crl->ctx.pbCrlEncoded       = data;
    crl->ctx.cbCrlEncoded       = cbCrlEncoded;
    crl->ctx.pCrlInfo           = crlInfo;
    crl->ctx.hCertStore         = &empty_store;

    return &crl->ctx;
}

BOOL WINAPI CertAddEncodedCRLToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded,
 DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext)
{
    PCCRL_CONTEXT crl = CertCreateCRLContext(dwCertEncodingType,
     pbCrlEncoded, cbCrlEncoded);
    BOOL ret;

    TRACE("(%p, %08lx, %p, %ld, %08lx, %p)\n", hCertStore, dwCertEncodingType,
     pbCrlEncoded, cbCrlEncoded, dwAddDisposition, ppCrlContext);

    if (crl)
    {
        ret = CertAddCRLContextToStore(hCertStore, crl, dwAddDisposition,
         ppCrlContext);
        CertFreeCRLContext(crl);
    }
    else
        ret = FALSE;
    return ret;
}

typedef BOOL (*CrlCompareFunc)(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara);

static BOOL compare_crl_any(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    return TRUE;
}

static BOOL compare_crl_issued_by(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;

    if (pvPara)
    {
        PCCERT_CONTEXT issuer = pvPara;

        ret = CertCompareCertificateName(issuer->dwCertEncodingType,
         &issuer->pCertInfo->Subject, &pCrlContext->pCrlInfo->Issuer);
        if (ret && (dwFlags & CRL_FIND_ISSUED_BY_SIGNATURE_FLAG))
            ret = CryptVerifyCertificateSignatureEx(0,
             issuer->dwCertEncodingType,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL, (void *)pCrlContext,
             CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT, (void *)issuer, 0, NULL);
        if (ret && (dwFlags & CRL_FIND_ISSUED_BY_AKI_FLAG))
        {
            PCERT_EXTENSION ext = CertFindExtension(
             szOID_AUTHORITY_KEY_IDENTIFIER2, pCrlContext->pCrlInfo->cExtension,
             pCrlContext->pCrlInfo->rgExtension);

            if (ext)
            {
                CERT_AUTHORITY_KEY_ID2_INFO *info;
                DWORD size;

                if ((ret = CryptDecodeObjectEx(X509_ASN_ENCODING,
                 X509_AUTHORITY_KEY_ID2, ext->Value.pbData, ext->Value.cbData,
                 CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size)))
                {
                    if (info->AuthorityCertIssuer.cAltEntry &&
                     info->AuthorityCertSerialNumber.cbData)
                    {
                        PCERT_ALT_NAME_ENTRY directoryName = NULL;
                        DWORD i;

                        for (i = 0; !directoryName &&
                         i < info->AuthorityCertIssuer.cAltEntry; i++)
                            if (info->AuthorityCertIssuer.rgAltEntry[i].
                             dwAltNameChoice == CERT_ALT_NAME_DIRECTORY_NAME)
                                directoryName =
                                 &info->AuthorityCertIssuer.rgAltEntry[i];
                        if (directoryName)
                        {
                            ret = CertCompareCertificateName(
                             issuer->dwCertEncodingType,
                             &issuer->pCertInfo->Subject,
                             &directoryName->DirectoryName);
                            if (ret)
                                ret = CertCompareIntegerBlob(
                                 &issuer->pCertInfo->SerialNumber,
                                 &info->AuthorityCertSerialNumber);
                        }
                        else
                        {
                            FIXME("no supported name type in authority key id2\n");
                            ret = FALSE;
                        }
                    }
                    else if (info->KeyId.cbData)
                    {
                        DWORD size;

                        ret = CertGetCertificateContextProperty(issuer,
                         CERT_KEY_IDENTIFIER_PROP_ID, NULL, &size);
                        if (ret && size == info->KeyId.cbData)
                        {
                            LPBYTE buf = CryptMemAlloc(size);

                            if (buf)
                            {
                                CertGetCertificateContextProperty(issuer,
                                 CERT_KEY_IDENTIFIER_PROP_ID, buf, &size);
                                ret = !memcmp(buf, info->KeyId.pbData, size);
                                CryptMemFree(buf);
                            }
                            else
                                ret = FALSE;
                        }
                        else
                            ret = FALSE;
                    }
                    else
                    {
                        FIXME("unsupported value for AKI extension\n");
                        ret = FALSE;
                    }
                    LocalFree(info);
                }
            }
            /* else: a CRL without an AKI matches any cert */
        }
    }
    else
        ret = TRUE;
    return ret;
}

static BOOL compare_crl_existing(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;

    if (pvPara)
    {
        PCCRL_CONTEXT crl = pvPara;

        ret = CertCompareCertificateName(pCrlContext->dwCertEncodingType,
         &pCrlContext->pCrlInfo->Issuer, &crl->pCrlInfo->Issuer);
    }
    else
        ret = TRUE;
    return ret;
}

static BOOL compare_crl_issued_for(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    const CRL_FIND_ISSUED_FOR_PARA *para = pvPara;
    BOOL ret;

    ret = CertCompareCertificateName(para->pIssuerCert->dwCertEncodingType,
     &para->pIssuerCert->pCertInfo->Issuer, &pCrlContext->pCrlInfo->Issuer);
    return ret;
}

PCCRL_CONTEXT WINAPI CertFindCRLInStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType,
 const void *pvFindPara, PCCRL_CONTEXT pPrevCrlContext)
{
    PCCRL_CONTEXT ret;
    CrlCompareFunc compare;

    TRACE("(%p, %ld, %ld, %ld, %p, %p)\n", hCertStore, dwCertEncodingType,
	 dwFindFlags, dwFindType, pvFindPara, pPrevCrlContext);

    switch (dwFindType)
    {
    case CRL_FIND_ANY:
        compare = compare_crl_any;
        break;
    case CRL_FIND_ISSUED_BY:
        compare = compare_crl_issued_by;
        break;
    case CRL_FIND_EXISTING:
        compare = compare_crl_existing;
        break;
    case CRL_FIND_ISSUED_FOR:
        compare = compare_crl_issued_for;
        break;
    default:
        FIXME("find type %08lx unimplemented\n", dwFindType);
        compare = NULL;
    }

    if (compare)
    {
        BOOL matches = FALSE;

        ret = pPrevCrlContext;
        do {
            ret = CertEnumCRLsInStore(hCertStore, ret);
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

PCCRL_CONTEXT WINAPI CertGetCRLFromStore(HCERTSTORE hCertStore,
 PCCERT_CONTEXT pIssuerContext, PCCRL_CONTEXT pPrevCrlContext, DWORD *pdwFlags)
{
    static const DWORD supportedFlags = CERT_STORE_SIGNATURE_FLAG |
     CERT_STORE_TIME_VALIDITY_FLAG | CERT_STORE_BASE_CRL_FLAG |
     CERT_STORE_DELTA_CRL_FLAG;
    PCCRL_CONTEXT ret;

    TRACE("(%p, %p, %p, %08lx)\n", hCertStore, pIssuerContext, pPrevCrlContext,
     *pdwFlags);

    if (*pdwFlags & ~supportedFlags)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (pIssuerContext)
        ret = CertFindCRLInStore(hCertStore, pIssuerContext->dwCertEncodingType,
         0, CRL_FIND_ISSUED_BY, pIssuerContext, pPrevCrlContext);
    else
        ret = CertFindCRLInStore(hCertStore, 0, 0, CRL_FIND_ANY, NULL,
         pPrevCrlContext);
    if (ret)
    {
        if (*pdwFlags & CERT_STORE_TIME_VALIDITY_FLAG)
        {
            if (0 == CertVerifyCRLTimeValidity(NULL, ret->pCrlInfo))
                *pdwFlags &= ~CERT_STORE_TIME_VALIDITY_FLAG;
        }
        if (*pdwFlags & CERT_STORE_SIGNATURE_FLAG)
        {
            if (CryptVerifyCertificateSignatureEx(0, ret->dwCertEncodingType,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL, (void *)ret,
             CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT, (void *)pIssuerContext, 0,
             NULL))
                *pdwFlags &= ~CERT_STORE_SIGNATURE_FLAG;
        }
    }
    return ret;
}

PCCRL_CONTEXT WINAPI CertDuplicateCRLContext(PCCRL_CONTEXT pCrlContext)
{
    TRACE("(%p)\n", pCrlContext);
    if (pCrlContext)
        Context_AddRef(&crl_from_ptr(pCrlContext)->base);
    return pCrlContext;
}

BOOL WINAPI CertFreeCRLContext(PCCRL_CONTEXT pCrlContext)
{
    TRACE("(%p)\n", pCrlContext);

    if (pCrlContext)
        Context_Release(&crl_from_ptr(pCrlContext)->base);
    return TRUE;
}

DWORD WINAPI CertEnumCRLContextProperties(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId)
{
    TRACE("(%p, %ld)\n", pCRLContext, dwPropId);

    return ContextPropertyList_EnumPropIDs(crl_from_ptr(pCRLContext)->base.properties, dwPropId);
}

static BOOL CRLContext_SetProperty(crl_t *crl, DWORD dwPropId,
                                   DWORD dwFlags, const void *pvData);

static BOOL CRLContext_GetHashProp(crl_t *crl, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret && pvData)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CRLContext_SetProperty(crl, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL CRLContext_GetProperty(crl_t *crl, DWORD dwPropId,
                                   void *pvData, DWORD *pcbData)
{
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    TRACE("(%p, %ld, %p, %p)\n", crl, dwPropId, pvData, pcbData);

    if (crl->base.properties)
        ret = ContextPropertyList_FindProperty(crl->base.properties, dwPropId, &blob);
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
            ret = CRLContext_GetHashProp(crl, dwPropId, CALG_SHA1,
                                         crl->ctx.pbCrlEncoded, crl->ctx.cbCrlEncoded, pvData,
             pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CRLContext_GetHashProp(crl, dwPropId, CALG_MD5,
                                         crl->ctx.pbCrlEncoded, crl->ctx.cbCrlEncoded, pvData,
             pcbData);
            break;
        default:
            SetLastError(CRYPT_E_NOT_FOUND);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertGetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    BOOL ret;

    TRACE("(%p, %ld, %p, %p)\n", pCRLContext, dwPropId, pvData, pcbData);

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
            ret = CertGetStoreProperty(pCRLContext->hCertStore, dwPropId, pvData, pcbData);
        }
        break;
    default:
        ret = CRLContext_GetProperty(crl_from_ptr(pCRLContext), dwPropId, pvData, pcbData);
    }
    return ret;
}

static BOOL CRLContext_SetProperty(crl_t *crl, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", crl, dwPropId, dwFlags, pvData);

    if (!crl->base.properties)
        ret = FALSE;
    else if (!pvData)
    {
        ContextPropertyList_RemoveProperty(crl->base.properties, dwPropId);
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

            ret = ContextPropertyList_SetProperty(crl->base.properties, dwPropId,
             blob->pbData, blob->cbData);
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            ret = ContextPropertyList_SetProperty(crl->base.properties, dwPropId,
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

BOOL WINAPI CertSetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", pCRLContext, dwPropId, dwFlags, pvData);

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
    ret = CRLContext_SetProperty(crl_from_ptr(pCRLContext), dwPropId, dwFlags, pvData);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL compare_dist_point_name(const CRL_DIST_POINT_NAME *name1,
 const CRL_DIST_POINT_NAME *name2)
{
    BOOL match;

    if (name1->dwDistPointNameChoice == name2->dwDistPointNameChoice)
    {
        match = TRUE;
        if (name1->dwDistPointNameChoice == CRL_DIST_POINT_FULL_NAME)
        {
            if (name1->FullName.cAltEntry == name2->FullName.cAltEntry)
            {
                DWORD i;

                for (i = 0; match && i < name1->FullName.cAltEntry; i++)
                {
                    const CERT_ALT_NAME_ENTRY *entry1 =
                     &name1->FullName.rgAltEntry[i];
                    const CERT_ALT_NAME_ENTRY *entry2 =
                     &name2->FullName.rgAltEntry[i];

                    if (entry1->dwAltNameChoice == entry2->dwAltNameChoice)
                    {
                        switch (entry1->dwAltNameChoice)
                        {
                        case CERT_ALT_NAME_URL:
                            match = !wcsicmp(entry1->pwszURL,
                             entry2->pwszURL);
                            break;
                        case CERT_ALT_NAME_DIRECTORY_NAME:
                            match = (entry1->DirectoryName.cbData ==
                             entry2->DirectoryName.cbData) &&
                             !memcmp(entry1->DirectoryName.pbData,
                             entry2->DirectoryName.pbData,
                             entry1->DirectoryName.cbData);
                            break;
                        default:
                            FIXME("unimplemented for type %ld\n",
                             entry1->dwAltNameChoice);
                            match = FALSE;
                        }
                    }
                    else
                        match = FALSE;
                }
            }
            else
                match = FALSE;
        }
    }
    else
        match = FALSE;
    return match;
}

static BOOL match_dist_point_with_issuing_dist_point(
 const CRL_DIST_POINT *distPoint, const CRL_ISSUING_DIST_POINT *idp)
{
    BOOL match;

    /* While RFC 5280, section 4.2.1.13 recommends against segmenting
     * CRL distribution points by reasons, it doesn't preclude doing so.
     * "This profile RECOMMENDS against segmenting CRLs by reason code."
     * If the issuing distribution point for this CRL is only valid for
     * some reasons, only match if the reasons covered also match the
     * reasons in the CRL distribution point.
     */
    if (idp->OnlySomeReasonFlags.cbData)
    {
        if (idp->OnlySomeReasonFlags.cbData == distPoint->ReasonFlags.cbData)
        {
            DWORD i;

            match = TRUE;
            for (i = 0; match && i < distPoint->ReasonFlags.cbData; i++)
                if (idp->OnlySomeReasonFlags.pbData[i] !=
                 distPoint->ReasonFlags.pbData[i])
                    match = FALSE;
        }
        else
            match = FALSE;
    }
    else
        match = TRUE;
    if (match)
        match = compare_dist_point_name(&idp->DistPointName,
         &distPoint->DistPointName);
    return match;
}

BOOL WINAPI CertIsValidCRLForCertificate(PCCERT_CONTEXT pCert,
 PCCRL_CONTEXT pCrl, DWORD dwFlags, void *pvReserved)
{
    PCERT_EXTENSION ext;
    BOOL ret;

    TRACE("(%p, %p, %08lx, %p)\n", pCert, pCrl, dwFlags, pvReserved);

    if (!pCert)
        return TRUE;

    if ((ext = CertFindExtension(szOID_ISSUING_DIST_POINT,
     pCrl->pCrlInfo->cExtension, pCrl->pCrlInfo->rgExtension)))
    {
        CRL_ISSUING_DIST_POINT *idp;
        DWORD size;

        if ((ret = CryptDecodeObjectEx(pCrl->dwCertEncodingType,
         X509_ISSUING_DIST_POINT, ext->Value.pbData, ext->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &idp, &size)))
        {
            if ((ext = CertFindExtension(szOID_CRL_DIST_POINTS,
             pCert->pCertInfo->cExtension, pCert->pCertInfo->rgExtension)))
            {
                CRL_DIST_POINTS_INFO *distPoints;

                if ((ret = CryptDecodeObjectEx(pCert->dwCertEncodingType,
                 X509_CRL_DIST_POINTS, ext->Value.pbData, ext->Value.cbData,
                 CRYPT_DECODE_ALLOC_FLAG, NULL, &distPoints, &size)))
                {
                    DWORD i;

                    ret = FALSE;
                    for (i = 0; !ret && i < distPoints->cDistPoint; i++)
                        ret = match_dist_point_with_issuing_dist_point(
                         &distPoints->rgDistPoint[i], idp);
                    if (!ret)
                        SetLastError(CRYPT_E_NO_MATCH);
                    LocalFree(distPoints);
                }
            }
            else
            {
                /* no CRL dist points extension in cert, can't match the CRL
                 * (which has an issuing dist point extension)
                 */
                ret = FALSE;
                SetLastError(CRYPT_E_NO_MATCH);
            }
            LocalFree(idp);
        }
    }
    else
        ret = TRUE;
    return ret;
}

static PCRL_ENTRY CRYPT_FindCertificateInCRL(PCERT_INFO cert, const CRL_INFO *crl)
{
    DWORD i;
    PCRL_ENTRY entry = NULL;

    for (i = 0; !entry && i < crl->cCRLEntry; i++)
        if (CertCompareIntegerBlob(&crl->rgCRLEntry[i].SerialNumber,
         &cert->SerialNumber))
            entry = &crl->rgCRLEntry[i];
    return entry;
}

BOOL WINAPI CertFindCertificateInCRL(PCCERT_CONTEXT pCert,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags, void *pvReserved,
 PCRL_ENTRY *ppCrlEntry)
{
    TRACE("(%p, %p, %08lx, %p, %p)\n", pCert, pCrlContext, dwFlags, pvReserved,
     ppCrlEntry);

    *ppCrlEntry = CRYPT_FindCertificateInCRL(pCert->pCertInfo,
     pCrlContext->pCrlInfo);
    return TRUE;
}

BOOL WINAPI CertVerifyCRLRevocation(DWORD dwCertEncodingType,
 PCERT_INFO pCertId, DWORD cCrlInfo, PCRL_INFO rgpCrlInfo[])
{
    DWORD i;
    PCRL_ENTRY entry = NULL;

    TRACE("(%08lx, %p, %ld, %p)\n", dwCertEncodingType, pCertId, cCrlInfo,
     rgpCrlInfo);

    for (i = 0; !entry && i < cCrlInfo; i++)
        entry = CRYPT_FindCertificateInCRL(pCertId, rgpCrlInfo[i]);
    return entry == NULL;
}

LONG WINAPI CertVerifyCRLTimeValidity(LPFILETIME pTimeToVerify,
 PCRL_INFO pCrlInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        GetSystemTimeAsFileTime(&fileTime);
        pTimeToVerify = &fileTime;
    }
    if ((ret = CompareFileTime(pTimeToVerify, &pCrlInfo->ThisUpdate)) >= 0)
    {
        ret = CompareFileTime(pTimeToVerify, &pCrlInfo->NextUpdate);
        if (ret < 0)
            ret = 0;
    }
    return ret;
}
