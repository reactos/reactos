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

PCCRL_CONTEXT WINAPI CertCreateCRLContext(DWORD dwCertEncodingType,
 const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    PCRL_CONTEXT crl = NULL;
    BOOL ret;
    PCRL_INFO crlInfo = NULL;
    DWORD size = 0;

    TRACE("(%08x, %p, %d)\n", dwCertEncodingType, pbCrlEncoded,
     cbCrlEncoded);

    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT_CRL_TO_BE_SIGNED,
     pbCrlEncoded, cbCrlEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &crlInfo, &size);
    if (ret)
    {
        BYTE *data = NULL;

        crl = Context_CreateDataContext(sizeof(CRL_CONTEXT));
        if (!crl)
            goto end;
        data = CryptMemAlloc(cbCrlEncoded);
        if (!data)
        {
            CryptMemFree(crl);
            crl = NULL;
            goto end;
        }
        memcpy(data, pbCrlEncoded, cbCrlEncoded);
        crl->dwCertEncodingType = dwCertEncodingType;
        crl->pbCrlEncoded       = data;
        crl->cbCrlEncoded       = cbCrlEncoded;
        crl->pCrlInfo           = crlInfo;
        crl->hCertStore         = 0;
    }

end:
    return crl;
}

BOOL WINAPI CertAddEncodedCRLToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded,
 DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext)
{
    PCCRL_CONTEXT crl = CertCreateCRLContext(dwCertEncodingType,
     pbCrlEncoded, cbCrlEncoded);
    BOOL ret;

    TRACE("(%p, %08x, %p, %d, %08x, %p)\n", hCertStore, dwCertEncodingType,
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
         &issuer->pCertInfo->Issuer, &pCrlContext->pCrlInfo->Issuer);
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

PCCRL_CONTEXT WINAPI CertFindCRLInStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType,
 const void *pvFindPara, PCCRL_CONTEXT pPrevCrlContext)
{
    PCCRL_CONTEXT ret;
    CrlCompareFunc compare;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hCertStore, dwCertEncodingType,
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
    default:
        FIXME("find type %08x unimplemented\n", dwFindType);
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

    TRACE("(%p, %p, %p, %08x)\n", hCertStore, pIssuerContext, pPrevCrlContext,
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
    Context_AddRef((void *)pCrlContext, sizeof(CRL_CONTEXT));
    return pCrlContext;
}

static void CrlDataContext_Free(void *context)
{
    PCRL_CONTEXT crlContext = context;

    CryptMemFree(crlContext->pbCrlEncoded);
    LocalFree(crlContext->pCrlInfo);
}

BOOL WINAPI CertFreeCRLContext( PCCRL_CONTEXT pCrlContext)
{
    TRACE("(%p)\n", pCrlContext);

    if (pCrlContext)
        Context_Release((void *)pCrlContext, sizeof(CRL_CONTEXT),
         CrlDataContext_Free);
    return TRUE;
}

DWORD WINAPI CertEnumCRLContextProperties(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId)
{
    PCONTEXT_PROPERTY_LIST properties = Context_GetProperties(
     pCRLContext, sizeof(CRL_CONTEXT));
    DWORD ret;

    TRACE("(%p, %d)\n", pCRLContext, dwPropId);

    if (properties)
        ret = ContextPropertyList_EnumPropIDs(properties, dwPropId);
    else
        ret = 0;
    return ret;
}

static BOOL CRLContext_SetProperty(PCCRL_CONTEXT context, DWORD dwPropId,
                                   DWORD dwFlags, const void *pvData);

static BOOL CRLContext_GetHashProp(PCCRL_CONTEXT context, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret && pvData)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CRLContext_SetProperty(context, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL CRLContext_GetProperty(PCCRL_CONTEXT context, DWORD dwPropId,
                                   void *pvData, DWORD *pcbData)
{
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CRL_CONTEXT));
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
            ret = CRLContext_GetHashProp(context, dwPropId, CALG_SHA1,
                                         context->pbCrlEncoded, context->cbCrlEncoded, pvData,
             pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CRLContext_GetHashProp(context, dwPropId, CALG_MD5,
                                         context->pbCrlEncoded, context->cbCrlEncoded, pvData,
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

    TRACE("(%p, %d, %p, %p)\n", pCRLContext, dwPropId, pvData, pcbData);

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
            if (pCRLContext->hCertStore)
                ret = CertGetStoreProperty(pCRLContext->hCertStore, dwPropId,
                 pvData, pcbData);
            else
                *(DWORD *)pvData = 0;
            ret = TRUE;
        }
        break;
    default:
        ret = CRLContext_GetProperty(pCRLContext, dwPropId, pvData,
         pcbData);
    }
    return ret;
}

static BOOL CRLContext_SetProperty(PCCRL_CONTEXT context, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CRL_CONTEXT));
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
             pvData, sizeof(FILETIME));
            break;
        default:
            FIXME("%d: stub\n", dwPropId);
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

    TRACE("(%p, %d, %08x, %p)\n", pCRLContext, dwPropId, dwFlags, pvData);

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
    ret = CRLContext_SetProperty(pCRLContext, dwPropId, dwFlags, pvData);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertIsValidCRLForCertificate(PCCERT_CONTEXT pCert,
 PCCRL_CONTEXT pCrl, DWORD dwFlags, void *pvReserved)
{
    TRACE("(%p, %p, %08x, %p)\n", pCert, pCrl, dwFlags, pvReserved);
    return TRUE;
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
    TRACE("(%p, %p, %08x, %p, %p)\n", pCert, pCrlContext, dwFlags, pvReserved,
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

    TRACE("(%08x, %p, %d, %p)\n", dwCertEncodingType, pCertId, cCrlInfo,
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
