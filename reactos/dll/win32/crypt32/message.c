/*
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

HCERTSTORE WINAPI CryptGetMessageCertificates(DWORD dwMsgAndCertEncodingType,
 HCRYPTPROV_LEGACY hCryptProv, DWORD dwFlags, const BYTE* pbSignedBlob,
 DWORD cbSignedBlob)
{
    CRYPT_DATA_BLOB blob = { cbSignedBlob, (LPBYTE)pbSignedBlob };

    TRACE("(%08x, %ld, %d08x %p, %d)\n", dwMsgAndCertEncodingType, hCryptProv,
     dwFlags, pbSignedBlob, cbSignedBlob);

    return CertOpenStore(CERT_STORE_PROV_PKCS7, dwMsgAndCertEncodingType,
     hCryptProv, dwFlags, &blob);
}

LONG WINAPI CryptGetMessageSignerCount(DWORD dwMsgEncodingType,
 const BYTE *pbSignedBlob, DWORD cbSignedBlob)
{
    HCRYPTMSG msg;
    LONG count = -1;

    TRACE("(%08x, %p, %d)\n", dwMsgEncodingType, pbSignedBlob, cbSignedBlob);

    msg = CryptMsgOpenToDecode(dwMsgEncodingType, 0, 0, 0, NULL, NULL);
    if (msg)
    {
        if (CryptMsgUpdate(msg, pbSignedBlob, cbSignedBlob, TRUE))
        {
            DWORD size = sizeof(count);

            CryptMsgGetParam(msg, CMSG_SIGNER_COUNT_PARAM, 0, &count, &size);
        }
        CryptMsgClose(msg);
    }
    return count;
}

static BOOL CRYPT_CopyParam(void *pvData, DWORD *pcbData, const void *src,
 DWORD len)
{
    BOOL ret = TRUE;

    if (!pvData)
        *pcbData = len;
    else if (*pcbData < len)
    {
        *pcbData = len;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pcbData = len;
        memcpy(pvData, src, len);
    }
    return ret;
}

static CERT_INFO *CRYPT_GetSignerCertInfoFromMsg(HCRYPTMSG msg,
 DWORD dwSignerIndex)
{
    CERT_INFO *certInfo = NULL;
    DWORD size;

    if (CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM, dwSignerIndex, NULL,
     &size))
    {
        certInfo = CryptMemAlloc(size);
        if (certInfo)
        {
            if (!CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM,
             dwSignerIndex, certInfo, &size))
            {
                CryptMemFree(certInfo);
                certInfo = NULL;
            }
        }
    }
    return certInfo;
}

static PCCERT_CONTEXT WINAPI CRYPT_DefaultGetSignerCertificate(void *pvGetArg,
 DWORD dwCertEncodingType, PCERT_INFO pSignerId, HCERTSTORE hMsgCertStore)
{
    return CertFindCertificateInStore(hMsgCertStore, dwCertEncodingType, 0,
     CERT_FIND_SUBJECT_CERT, pSignerId, NULL);
}

static inline PCCERT_CONTEXT CRYPT_GetSignerCertificate(HCRYPTMSG msg,
 PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara, PCERT_INFO certInfo, HCERTSTORE store)
{
    PFN_CRYPT_GET_SIGNER_CERTIFICATE getCert;

    if (pVerifyPara->pfnGetSignerCertificate)
        getCert = pVerifyPara->pfnGetSignerCertificate;
    else
        getCert = CRYPT_DefaultGetSignerCertificate;
    return getCert(pVerifyPara->pvGetArg,
     pVerifyPara->dwMsgAndCertEncodingType, certInfo, store);
}

BOOL WINAPI CryptVerifyMessageSignature(PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
 DWORD dwSignerIndex, const BYTE* pbSignedBlob, DWORD cbSignedBlob,
 BYTE* pbDecoded, DWORD* pcbDecoded, PCCERT_CONTEXT* ppSignerCert)
{
    BOOL ret = FALSE;
    DWORD size;
    CRYPT_CONTENT_INFO *contentInfo;
    HCRYPTMSG msg;

    TRACE("(%p, %d, %p, %d, %p, %p, %p)\n",
     pVerifyPara, dwSignerIndex, pbSignedBlob, cbSignedBlob,
     pbDecoded, pcbDecoded, ppSignerCert);

    if (ppSignerCert)
        *ppSignerCert = NULL;
    if (pcbDecoded)
        *pcbDecoded = 0;
    if (!pVerifyPara ||
     pVerifyPara->cbSize != sizeof(CRYPT_VERIFY_MESSAGE_PARA) ||
     GET_CMSG_ENCODING_TYPE(pVerifyPara->dwMsgAndCertEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (!CryptDecodeObjectEx(pVerifyPara->dwMsgAndCertEncodingType,
     PKCS_CONTENT_INFO, pbSignedBlob, cbSignedBlob,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
     (LPBYTE)&contentInfo, &size))
        return FALSE;
    if (strcmp(contentInfo->pszObjId, szOID_RSA_signedData))
    {
        LocalFree(contentInfo);
        SetLastError(CRYPT_E_UNEXPECTED_MSG_TYPE);
        return FALSE;
    }
    msg = CryptMsgOpenToDecode(pVerifyPara->dwMsgAndCertEncodingType, 0,
     CMSG_SIGNED, pVerifyPara->hCryptProv, NULL, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, contentInfo->Content.pbData,
         contentInfo->Content.cbData, TRUE);
        if (ret && pcbDecoded)
            ret = CRYPT_CopyParam(pbDecoded, pcbDecoded,
             contentInfo->Content.pbData, contentInfo->Content.cbData);
        if (ret)
        {
            CERT_INFO *certInfo = CRYPT_GetSignerCertInfoFromMsg(msg,
             dwSignerIndex);

            ret = FALSE;
            if (certInfo)
            {
                HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MSG,
                 pVerifyPara->dwMsgAndCertEncodingType,
                 pVerifyPara->hCryptProv, 0, msg);

                if (store)
                {
                    PCCERT_CONTEXT cert = CRYPT_GetSignerCertificate(
                     msg, pVerifyPara, certInfo, store);

                    if (cert)
                    {
                        ret = CryptMsgControl(msg, 0,
                         CMSG_CTRL_VERIFY_SIGNATURE, cert->pCertInfo);
                        if (ret && ppSignerCert)
                            *ppSignerCert = cert;
                        else
                            CertFreeCertificateContext(cert);
                    }
                    CertCloseStore(store, 0);
                }
            }
            CryptMemFree(certInfo);
        }
        CryptMsgClose(msg);
    }
    LocalFree(contentInfo);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptHashMessage(PCRYPT_HASH_MESSAGE_PARA pHashPara,
 BOOL fDetachedHash, DWORD cToBeHashed, const BYTE *rgpbToBeHashed[],
 DWORD rgcbToBeHashed[], BYTE *pbHashedBlob, DWORD *pcbHashedBlob,
 BYTE *pbComputedHash, DWORD *pcbComputedHash)
{
    DWORD i, flags;
    BOOL ret = FALSE;
    HCRYPTMSG msg;
    CMSG_HASHED_ENCODE_INFO info;

    TRACE("(%p, %d, %d, %p, %p, %p, %p, %p, %p)\n", pHashPara, fDetachedHash,
     cToBeHashed, rgpbToBeHashed, rgcbToBeHashed, pbHashedBlob, pcbHashedBlob,
     pbComputedHash, pcbComputedHash);

    if (pHashPara->cbSize != sizeof(CRYPT_HASH_MESSAGE_PARA))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    /* Native seems to ignore any encoding type other than the expected
     * PKCS_7_ASN_ENCODING
     */
    if (GET_CMSG_ENCODING_TYPE(pHashPara->dwMsgEncodingType) !=
     PKCS_7_ASN_ENCODING)
        return TRUE;
    /* Native also seems to do nothing if the output parameter isn't given */
    if (!pcbHashedBlob)
        return TRUE;

    flags = fDetachedHash ? CMSG_DETACHED_FLAG : 0;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.hCryptProv = pHashPara->hCryptProv;
    memcpy(&info.HashAlgorithm, &pHashPara->HashAlgorithm,
     sizeof(info.HashAlgorithm));
    info.pvHashAuxInfo = pHashPara->pvHashAuxInfo;
    msg = CryptMsgOpenToEncode(pHashPara->dwMsgEncodingType, flags, CMSG_HASHED,
     &info, NULL, NULL);
    if (msg)
    {
        for (i = 0, ret = TRUE; ret && i < cToBeHashed; i++)
            ret = CryptMsgUpdate(msg, rgpbToBeHashed[i], rgcbToBeHashed[i],
             i == cToBeHashed - 1 ? TRUE : FALSE);
        if (ret)
        {
            ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, pbHashedBlob,
             pcbHashedBlob);
            if (ret && pcbComputedHash)
                ret = CryptMsgGetParam(msg, CMSG_COMPUTED_HASH_PARAM, 0,
                 pbComputedHash, pcbComputedHash);
        }
        CryptMsgClose(msg);
    }
    return ret;
}

BOOL WINAPI CryptVerifyDetachedMessageHash(PCRYPT_HASH_MESSAGE_PARA pHashPara,
 BYTE *pbDetachedHashBlob, DWORD cbDetachedHashBlob, DWORD cToBeHashed,
 const BYTE *rgpbToBeHashed[], DWORD rgcbToBeHashed[], BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    HCRYPTMSG msg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %d, %d, %p, %p, %p, %p)\n", pHashPara, pbDetachedHashBlob,
     cbDetachedHashBlob, cToBeHashed, rgpbToBeHashed, rgcbToBeHashed,
     pbComputedHash, pcbComputedHash);

    if (pHashPara->cbSize != sizeof(CRYPT_HASH_MESSAGE_PARA))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (GET_CMSG_ENCODING_TYPE(pHashPara->dwMsgEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    msg = CryptMsgOpenToDecode(pHashPara->dwMsgEncodingType, CMSG_DETACHED_FLAG,
     0, pHashPara->hCryptProv, NULL, NULL);
    if (msg)
    {
        DWORD i;

        ret = CryptMsgUpdate(msg, pbDetachedHashBlob, cbDetachedHashBlob, TRUE);
        if (ret)
        {
            if (cToBeHashed)
            {
                for (i = 0; ret && i < cToBeHashed; i++)
                {
                    ret = CryptMsgUpdate(msg, rgpbToBeHashed[i],
                     rgcbToBeHashed[i], i == cToBeHashed - 1 ? TRUE : FALSE);
                }
            }
            else
                ret = CryptMsgUpdate(msg, NULL, 0, TRUE);
        }
        if (ret)
        {
            ret = CryptMsgControl(msg, 0, CMSG_CTRL_VERIFY_HASH, NULL);
            if (ret && pcbComputedHash)
                ret = CryptMsgGetParam(msg, CMSG_COMPUTED_HASH_PARAM, 0,
                 pbComputedHash, pcbComputedHash);
        }
        CryptMsgClose(msg);
    }
    return ret;
}
