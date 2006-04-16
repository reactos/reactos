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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr,
 CRYPT_ATTRIBUTE rgAttr[])
{
    PCRYPT_ATTRIBUTE ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cAttr, rgAttr);

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

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cExtensions, rgExtensions);

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

LONG WINAPI CertVerifyTimeValidity(LPFILETIME pTimeToVerify,
 PCERT_INFO pCertInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        SYSTEMTIME sysTime;

        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
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

BOOL WINAPI CryptHashCertificate(HCRYPTPROV hCryptProv, ALG_ID Algid,
 DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%ld, %d, %08lx, %p, %ld, %p, %p)\n", hCryptProv, Algid, dwFlags,
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

BOOL WINAPI CryptSignCertificate(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, const BYTE *pbEncodedToBeSigned,
 DWORD cbEncodedToBeSigned, PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
 const void *pvHashAuxInfo, BYTE *pbSignature, DWORD *pcbSignature)
{
    BOOL ret;
    ALG_ID algID;
    HCRYPTHASH hHash;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %p, %p, %p)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, pbEncodedToBeSigned, cbEncodedToBeSigned,
     pSignatureAlgorithm, pvHashAuxInfo, pbSignature, pcbSignature);

    algID = CertOIDToAlgId(pSignatureAlgorithm->pszObjId);
    if (!algID)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    if (!hCryptProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hHash);
    if (ret)
    {
        ret = CryptHashData(hHash, pbEncodedToBeSigned, cbEncodedToBeSigned, 0);
        if (ret)
            ret = CryptSignHashW(hHash, dwKeySpec, NULL, 0, pbSignature,
             pcbSignature);
        CryptDestroyHash(hHash);
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignature(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 PCERT_PUBLIC_KEY_INFO pPublicKey)
{
    return CryptVerifyCertificateSignatureEx(hCryptProv, dwCertEncodingType,
     CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, (void *)pbEncoded,
     CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, pPublicKey, 0, NULL);
}

BOOL WINAPI CryptVerifyCertificateSignatureEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, DWORD dwSubjectType, void *pvSubject,
 DWORD dwIssuerType, void *pvIssuer, DWORD dwFlags, void *pvReserved)
{
    BOOL ret = TRUE;
    CRYPT_DATA_BLOB subjectBlob;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %08lx, %p)\n", hCryptProv,
     dwCertEncodingType, dwSubjectType, pvSubject, dwIssuerType, pvIssuer,
     dwFlags, pvReserved);

    switch (dwSubjectType)
    {
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB:
    {
        PCRYPT_DATA_BLOB blob = (PCRYPT_DATA_BLOB)pvSubject;

        subjectBlob.pbData = blob->pbData;
        subjectBlob.cbData = blob->cbData;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT:
    {
        PCERT_CONTEXT context = (PCERT_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCertEncoded;
        subjectBlob.cbData = context->cbCertEncoded;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL:
    {
        PCRL_CONTEXT context = (PCRL_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCrlEncoded;
        subjectBlob.cbData = context->cbCrlEncoded;
        break;
    }
    default:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        ret = FALSE;
    }

    if (ret)
    {
        PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT,
         subjectBlob.pbData, subjectBlob.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&signedCert, &size);
        if (ret)
        {
            switch (dwIssuerType)
            {
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY:
            {
                PCERT_PUBLIC_KEY_INFO pubKeyInfo =
                 (PCERT_PUBLIC_KEY_INFO)pvIssuer;
                ALG_ID algID = CertOIDToAlgId(pubKeyInfo->Algorithm.pszObjId);

                if (algID)
                {
                    HCRYPTKEY key;

                    ret = CryptImportPublicKeyInfoEx(hCryptProv,
                     dwCertEncodingType, pubKeyInfo, algID, 0, NULL, &key);
                    if (ret)
                    {
                        HCRYPTHASH hash;

                        ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hash);
                        if (ret)
                        {
                            ret = CryptHashData(hash,
                             signedCert->ToBeSigned.pbData,
                             signedCert->ToBeSigned.cbData, 0);
                            if (ret)
                            {
                                ret = CryptVerifySignatureW(hash,
                                 signedCert->Signature.pbData,
                                 signedCert->Signature.cbData, key, NULL, 0);
                            }
                            CryptDestroyHash(hash);
                        }
                        CryptDestroyKey(key);
                    }
                }
                else
                {
                    SetLastError(NTE_BAD_ALGID);
                    ret = FALSE;
                }
                break;
            }
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT:
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN:
                FIXME("issuer type %ld: stub\n", dwIssuerType);
                ret = FALSE;
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL:
                if (pvIssuer)
                {
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                else
                {
                    FIXME("unimplemented for NULL signer\n");
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                break;
            default:
                SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                ret = FALSE;
            }
            LocalFree(signedCert);
        }
    }
    return ret;
}

BOOL WINAPI CertGetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext, DWORD dwFlags,
 PCERT_ENHKEY_USAGE pUsage, DWORD *pcbUsage)
{
    PCERT_ENHKEY_USAGE usage = NULL;
    DWORD bytesNeeded;
    BOOL ret = TRUE;

    TRACE("(%p, %08lx, %p, %ld)\n", pCertContext, dwFlags, pUsage, *pcbUsage);

    if (!pCertContext || !pcbUsage)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
                PCERT_ENHKEY_USAGE newUsage = CryptMemAlloc(size +
                 sizeof(LPSTR) + strlen(pszUsageIdentifier) + 1);

                if (newUsage)
                {
                    LPSTR nextOID;
                    DWORD i;

                    newUsage->rgpszUsageIdentifier =
                     (LPSTR *)((LPBYTE)newUsage + sizeof(CERT_ENHKEY_USAGE));
                    nextOID = (LPSTR)((LPBYTE)newUsage->rgpszUsageIdentifier +
                     (usage->cUsageIdentifier + 1) * sizeof(LPSTR));
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

BOOL WINAPI CertGetValidUsages(DWORD cCerts, PCCERT_CONTEXT *rghCerts,
 int *cNumOIDSs, LPSTR *rghOIDs, DWORD *pcbOIDs)
{
    BOOL ret = TRUE;
    DWORD i, cbOIDs = 0;
    BOOL allUsagesValid = TRUE;
    CERT_ENHKEY_USAGE validUsages = { 0, NULL };

    TRACE("(%ld, %p, %p, %p, %ld)\n", cCerts, *rghCerts, cNumOIDSs,
     rghOIDs, *pcbOIDs);

    for (i = 0; ret && i < cCerts; i++)
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
                        else
                            ret = FALSE;
                    }
                    else
                    {
                        DWORD j, k, validIndexes = 0, numRemoved = 0;

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
                                    validIndexes |= (1 << k);
                                    break;
                                }
                            }
                        }
                        /* Merge by removing from validUsages those that are
                         * not in the bitmap.
                         */
                        for (j = 0; j < validUsages.cUsageIdentifier; j++)
                        {
                            if (!(validIndexes & (1 << j)))
                            {
                                if (j < validUsages.cUsageIdentifier - 1)
                                {
                                    memcpy(&validUsages.rgpszUsageIdentifier[j],
                                     &validUsages.rgpszUsageIdentifier[j +
                                     numRemoved + 1],
                                     (validUsages.cUsageIdentifier - numRemoved
                                     - j - 1) * sizeof(LPSTR));
                                    cbOIDs -= lstrlenA(
                                     validUsages.rgpszUsageIdentifier[j]) + 1 +
                                     sizeof(LPSTR);
                                    numRemoved++;
                                }
                                else
                                    validUsages.cUsageIdentifier--;
                            }
                        }
                    }
                }
                CryptMemFree(pUsage);
            }
            else
                ret = FALSE;
        }
    }
    if (ret)
    {
        if (allUsagesValid)
        {
            *cNumOIDSs = -1;
            *pcbOIDs = 0;
        }
        else
        {
            if (!rghOIDs || *pcbOIDs < cbOIDs)
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
                *cNumOIDSs = validUsages.cUsageIdentifier;
                for (i = 0; i < validUsages.cUsageIdentifier; i++)
                {
                    rghOIDs[i] = nextOID;
                    lstrcpyA(nextOID, validUsages.rgpszUsageIdentifier[i]);
                    nextOID += lstrlenA(nextOID) + 1;
                }
            }
        }
    }
    CryptMemFree(validUsages.rgpszUsageIdentifier);
    return ret;
}
