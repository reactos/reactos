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

    TRACE("(%08lx, %Id, %ld08x %p, %ld)\n", dwMsgAndCertEncodingType, hCryptProv,
     dwFlags, pbSignedBlob, cbSignedBlob);

    return CertOpenStore(CERT_STORE_PROV_PKCS7, dwMsgAndCertEncodingType,
     hCryptProv, dwFlags, &blob);
}

LONG WINAPI CryptGetMessageSignerCount(DWORD dwMsgEncodingType,
 const BYTE *pbSignedBlob, DWORD cbSignedBlob)
{
    HCRYPTMSG msg;
    LONG count = -1;

    TRACE("(%08lx, %p, %ld)\n", dwMsgEncodingType, pbSignedBlob, cbSignedBlob);

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
    else
        SetLastError(CRYPT_E_UNEXPECTED_MSG_TYPE);
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

BOOL WINAPI CryptVerifyDetachedMessageSignature(
 PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara, DWORD dwSignerIndex,
 const BYTE *pbDetachedSignBlob, DWORD cbDetachedSignBlob, DWORD cToBeSigned,
 const BYTE *rgpbToBeSigned[], DWORD rgcbToBeSigned[],
 PCCERT_CONTEXT *ppSignerCert)
{
    BOOL ret = FALSE;
    HCRYPTMSG msg;

    TRACE("(%p, %ld, %p, %ld, %ld, %p, %p, %p)\n", pVerifyPara, dwSignerIndex,
     pbDetachedSignBlob, cbDetachedSignBlob, cToBeSigned, rgpbToBeSigned,
     rgcbToBeSigned, ppSignerCert);

    if (ppSignerCert)
        *ppSignerCert = NULL;
    if (!pVerifyPara ||
     pVerifyPara->cbSize != sizeof(CRYPT_VERIFY_MESSAGE_PARA) ||
     GET_CMSG_ENCODING_TYPE(pVerifyPara->dwMsgAndCertEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    msg = CryptMsgOpenToDecode(pVerifyPara->dwMsgAndCertEncodingType,
     CMSG_DETACHED_FLAG, 0, pVerifyPara->hCryptProv, NULL, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, pbDetachedSignBlob, cbDetachedSignBlob, TRUE);
        if (ret)
        {
            DWORD i;

            for (i = 0; ret && i < cToBeSigned; i++)
                ret = CryptMsgUpdate(msg, rgpbToBeSigned[i], rgcbToBeSigned[i],
                 i == cToBeSigned - 1);
        }
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
                    else
                        SetLastError(CRYPT_E_NOT_FOUND);
                    CertCloseStore(store, 0);
                }
                CryptMemFree(certInfo);
            }
        }
        CryptMsgClose(msg);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptVerifyMessageSignature(PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
 DWORD dwSignerIndex, const BYTE* pbSignedBlob, DWORD cbSignedBlob,
 BYTE* pbDecoded, DWORD* pcbDecoded, PCCERT_CONTEXT* ppSignerCert)
{
    BOOL ret = FALSE;
    HCRYPTMSG msg;

    TRACE("(%p, %ld, %p, %ld, %p, %p, %p)\n",
     pVerifyPara, dwSignerIndex, pbSignedBlob, cbSignedBlob,
     pbDecoded, pcbDecoded, ppSignerCert);

    if (ppSignerCert)
        *ppSignerCert = NULL;
    if (!pVerifyPara ||
     pVerifyPara->cbSize != sizeof(CRYPT_VERIFY_MESSAGE_PARA) ||
     GET_CMSG_ENCODING_TYPE(pVerifyPara->dwMsgAndCertEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        if(pcbDecoded)
            *pcbDecoded = 0;
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    msg = CryptMsgOpenToDecode(pVerifyPara->dwMsgAndCertEncodingType, 0, 0,
     pVerifyPara->hCryptProv, NULL, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, pbSignedBlob, cbSignedBlob, TRUE);
        if (ret && pcbDecoded)
            ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, pbDecoded,
             pcbDecoded);
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
    if(!ret && pcbDecoded)
        *pcbDecoded = 0;
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

    TRACE("(%p, %d, %ld, %p, %p, %p, %p, %p, %p)\n", pHashPara, fDetachedHash,
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
    info.HashAlgorithm = pHashPara->HashAlgorithm;
    info.pvHashAuxInfo = pHashPara->pvHashAuxInfo;
    msg = CryptMsgOpenToEncode(pHashPara->dwMsgEncodingType, flags, CMSG_HASHED,
     &info, NULL, NULL);
    if (msg)
    {
        for (i = 0, ret = TRUE; ret && i < cToBeHashed; i++)
            ret = CryptMsgUpdate(msg, rgpbToBeHashed[i], rgcbToBeHashed[i], i == cToBeHashed - 1);
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

    TRACE("(%p, %p, %ld, %ld, %p, %p, %p, %p)\n", pHashPara, pbDetachedHashBlob,
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
                     rgcbToBeHashed[i], i == cToBeHashed - 1);
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

BOOL WINAPI CryptVerifyMessageHash(PCRYPT_HASH_MESSAGE_PARA pHashPara,
 BYTE *pbHashedBlob, DWORD cbHashedBlob, BYTE *pbToBeHashed,
 DWORD *pcbToBeHashed, BYTE *pbComputedHash, DWORD *pcbComputedHash)
{
    HCRYPTMSG msg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %ld, %p, %p, %p, %p)\n", pHashPara, pbHashedBlob,
     cbHashedBlob, pbToBeHashed, pcbToBeHashed, pbComputedHash,
     pcbComputedHash);

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
    msg = CryptMsgOpenToDecode(pHashPara->dwMsgEncodingType, 0, 0,
     pHashPara->hCryptProv, NULL, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, pbHashedBlob, cbHashedBlob, TRUE);
        if (ret)
        {
            ret = CryptMsgControl(msg, 0, CMSG_CTRL_VERIFY_HASH, NULL);
            if (ret && pcbToBeHashed)
                ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0,
                 pbToBeHashed, pcbToBeHashed);
            if (ret && pcbComputedHash)
                ret = CryptMsgGetParam(msg, CMSG_COMPUTED_HASH_PARAM, 0,
                 pbComputedHash, pcbComputedHash);
        }
        CryptMsgClose(msg);
    }
    return ret;
}

BOOL WINAPI CryptSignMessage(PCRYPT_SIGN_MESSAGE_PARA pSignPara,
 BOOL fDetachedSignature, DWORD cToBeSigned, const BYTE *rgpbToBeSigned[],
 DWORD rgcbToBeSigned[], BYTE *pbSignedBlob, DWORD *pcbSignedBlob)
{
    HCRYPTPROV hCryptProv;
    BOOL ret, freeProv = FALSE;
    DWORD i, keySpec;
    PCERT_BLOB certBlob = NULL;
    PCRL_BLOB crlBlob = NULL;
    CMSG_SIGNED_ENCODE_INFO signInfo;
    CMSG_SIGNER_ENCODE_INFO signer;
    HCRYPTMSG msg = 0;

    TRACE("(%p, %d, %ld, %p, %p, %p, %p)\n", pSignPara, fDetachedSignature,
     cToBeSigned, rgpbToBeSigned, rgcbToBeSigned, pbSignedBlob, pcbSignedBlob);

    if (pSignPara->cbSize != sizeof(CRYPT_SIGN_MESSAGE_PARA) ||
     GET_CMSG_ENCODING_TYPE(pSignPara->dwMsgEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        *pcbSignedBlob = 0;
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!pSignPara->pSigningCert)
        return TRUE;

    ret = CryptAcquireCertificatePrivateKey(pSignPara->pSigningCert,
     CRYPT_ACQUIRE_CACHE_FLAG, NULL, &hCryptProv, &keySpec, &freeProv);
    if (!ret)
        return FALSE;

    memset(&signer, 0, sizeof(signer));
    signer.cbSize = sizeof(signer);
    signer.pCertInfo = pSignPara->pSigningCert->pCertInfo;
    signer.hCryptProv = hCryptProv;
    signer.dwKeySpec = keySpec;
    signer.HashAlgorithm = pSignPara->HashAlgorithm;
    signer.pvHashAuxInfo = pSignPara->pvHashAuxInfo;
    signer.cAuthAttr = pSignPara->cAuthAttr;
    signer.rgAuthAttr = pSignPara->rgAuthAttr;
    signer.cUnauthAttr = pSignPara->cUnauthAttr;
    signer.rgUnauthAttr = pSignPara->rgUnauthAttr;

    memset(&signInfo, 0, sizeof(signInfo));
    signInfo.cbSize = sizeof(signInfo);
    signInfo.cSigners = 1;
    signInfo.rgSigners = &signer;

    if (pSignPara->cMsgCert)
    {
        certBlob = CryptMemAlloc(sizeof(CERT_BLOB) * pSignPara->cMsgCert);
        if (certBlob)
        {
            for (i = 0; i < pSignPara->cMsgCert; ++i)
            {
                certBlob[i].cbData = pSignPara->rgpMsgCert[i]->cbCertEncoded;
                certBlob[i].pbData = pSignPara->rgpMsgCert[i]->pbCertEncoded;
            }
            signInfo.cCertEncoded = pSignPara->cMsgCert;
            signInfo.rgCertEncoded = certBlob;
        }
        else
            ret = FALSE;
    }
    if (pSignPara->cMsgCrl)
    {
        crlBlob = CryptMemAlloc(sizeof(CRL_BLOB) * pSignPara->cMsgCrl);
        if (crlBlob)
        {
            for (i = 0; i < pSignPara->cMsgCrl; ++i)
            {
                crlBlob[i].cbData = pSignPara->rgpMsgCrl[i]->cbCrlEncoded;
                crlBlob[i].pbData = pSignPara->rgpMsgCrl[i]->pbCrlEncoded;
            }
            signInfo.cCrlEncoded = pSignPara->cMsgCrl;
            signInfo.rgCrlEncoded = crlBlob;
        }
        else
            ret = FALSE;
    }
    if (pSignPara->dwFlags || pSignPara->dwInnerContentType)
        FIXME("unimplemented feature\n");

    if (ret)
        msg = CryptMsgOpenToEncode(pSignPara->dwMsgEncodingType,
         fDetachedSignature ? CMSG_DETACHED_FLAG : 0, CMSG_SIGNED, &signInfo,
         NULL, NULL);
    if (msg)
    {
        if (cToBeSigned)
        {
            for (i = 0; ret && i < cToBeSigned; ++i)
            {
                ret = CryptMsgUpdate(msg, rgpbToBeSigned[i], rgcbToBeSigned[i],
                 i == cToBeSigned - 1);
            }
        }
        else
            ret = CryptMsgUpdate(msg, NULL, 0, TRUE);
        if (ret)
            ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, pbSignedBlob,
             pcbSignedBlob);
        CryptMsgClose(msg);
    }
    else
        ret = FALSE;

    CryptMemFree(crlBlob);
    CryptMemFree(certBlob);
    if (freeProv)
        CryptReleaseContext(hCryptProv, 0);
    return ret;
}

BOOL WINAPI CryptEncryptMessage(PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
 DWORD cRecipientCert, PCCERT_CONTEXT rgpRecipientCert[],
 const BYTE *pbToBeEncrypted, DWORD cbToBeEncrypted, BYTE *pbEncryptedBlob,
 DWORD *pcbEncryptedBlob)
{
    BOOL ret = TRUE;
    DWORD i;
    PCERT_INFO *certInfo = NULL;
    CMSG_ENVELOPED_ENCODE_INFO envelopedInfo;
    HCRYPTMSG msg = 0;

    TRACE("(%p, %ld, %p, %p, %ld, %p, %p)\n", pEncryptPara, cRecipientCert,
     rgpRecipientCert, pbToBeEncrypted, cbToBeEncrypted, pbEncryptedBlob,
     pcbEncryptedBlob);

    if (pEncryptPara->cbSize != sizeof(CRYPT_ENCRYPT_MESSAGE_PARA) ||
     GET_CMSG_ENCODING_TYPE(pEncryptPara->dwMsgEncodingType) !=
     PKCS_7_ASN_ENCODING)
    {
        *pcbEncryptedBlob = 0;
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    memset(&envelopedInfo, 0, sizeof(envelopedInfo));
    envelopedInfo.cbSize = sizeof(envelopedInfo);
    envelopedInfo.hCryptProv = pEncryptPara->hCryptProv;
    envelopedInfo.ContentEncryptionAlgorithm =
     pEncryptPara->ContentEncryptionAlgorithm;
    envelopedInfo.pvEncryptionAuxInfo = pEncryptPara->pvEncryptionAuxInfo;

    if (cRecipientCert)
    {
        certInfo = CryptMemAlloc(sizeof(PCERT_INFO) * cRecipientCert);
        if (certInfo)
        {
            for (i = 0; i < cRecipientCert; ++i)
                certInfo[i] = rgpRecipientCert[i]->pCertInfo;
            envelopedInfo.cRecipients = cRecipientCert;
            envelopedInfo.rgpRecipientCert = certInfo;
        }
        else
            ret = FALSE;
    }

    if (ret)
        msg = CryptMsgOpenToEncode(pEncryptPara->dwMsgEncodingType, 0,
         CMSG_ENVELOPED, &envelopedInfo, NULL, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, pbToBeEncrypted, cbToBeEncrypted, TRUE);
        if (ret)
            ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, pbEncryptedBlob,
             pcbEncryptedBlob);
        CryptMsgClose(msg);
    }
    else
        ret = FALSE;

    CryptMemFree(certInfo);
    if (!ret) *pcbEncryptedBlob = 0;
    return ret;
}
