//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       trustapi.cpp
//
//  Contents:   Microsoft Internet Security Trust APIs
//
//  Functions:  TrustFindIssuerCertificate
//              TrustOpenStores
//              TrustDecode
//              TrustFreeDecode
//
//              *** local functions ***
//              _CompareAuthKeyId
//              _CompareAuthKeyId2
//              _SetCertErrorAndHygiene
//              _GetExternalIssuerCert
//
//  History:    20-Nov-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


BOOL _CompareAuthKeyId(DWORD dwEncoding, PCCERT_CONTEXT pChildContext, 
                       PCCERT_CONTEXT pParentContext);
BOOL _CompareAuthKeyId2(DWORD dwEncoding, PCCERT_CONTEXT pChildContext, 
                        PCCERT_CONTEXT pParentContext);
BOOL _SetCertErrorAndHygiene(PCCERT_CONTEXT pSubjectContext, 
                             PCCERT_CONTEXT pIssuerContext,
                             DWORD dwCurrentConfidence, DWORD *pdwError);
PCCERT_CONTEXT _GetExternalIssuerCert(PCCERT_CONTEXT pContext, 
                                      DWORD dwEncoding,
                                      DWORD *pdwRetError, 
                                      DWORD *pdwConfidence,
                                      FILETIME *psftVerifyAsOf);

void _SetConfidenceOnIssuer(DWORD dwEncoding, PCCERT_CONTEXT pChildCert, PCCERT_CONTEXT pTestIssuerCert, 
                            DWORD dwVerificationFlag, FILETIME *psftVerifyAsOf, DWORD *pdwConfidence, 
                            DWORD *pdwError);

PCCERT_CONTEXT WINAPI TrustFindIssuerCertificate(PCCERT_CONTEXT pChildContext,
                                                 DWORD dwEncoding,
                                                 DWORD chStores,
                                                 HCERTSTORE  *pahStores,
                                                 FILETIME *psftVerifyAsOf,
                                                 DWORD *pdwConfidence,
                                                 DWORD *pdwError,
                                                 DWORD dwFlags)
{
    if (!(pChildContext) ||
        !(pahStores) ||
        !(psftVerifyAsOf) ||
        (dwFlags != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    PCCERT_CONTEXT  pCertContext;
    DWORD           fdwRetError;
    DWORD           fdwWork;
    DWORD           dwError;
    PCCERT_CONTEXT  pCertWithHighestConfidence;
    DWORD           dwHighestConfidence;
    DWORD           dwConfidence;

    if (pdwError)
    {
        *pdwError       = ERROR_SUCCESS;
    }

    dwConfidence    = 0;

    dwHighestConfidence         = 0;
    pCertWithHighestConfidence  = NULL;

    fdwRetError                 = 0;
    fdwWork                     = 0;

    for (int i = 0; i < (int)chStores; i++)
    {
        fdwWork         = CERT_STORE_SIGNATURE_FLAG;

        pCertContext    = NULL;

        while (pCertContext = CertGetIssuerCertificateFromStore(pahStores[i],
                                                                pChildContext,
                                                                pCertContext,
                                                                &fdwWork))
        {
            _SetConfidenceOnIssuer(dwEncoding, pChildContext, pCertContext, fdwWork,
                                   psftVerifyAsOf, &dwConfidence, &dwError);
        
            if (dwConfidence > dwHighestConfidence)
            {
                if (pCertWithHighestConfidence)
                {
                    CertFreeCertificateContext(pCertWithHighestConfidence);
                }

                dwHighestConfidence         = dwConfidence;
                pCertWithHighestConfidence  = CertDuplicateCertificateContext(pCertContext);
                fdwRetError                 = dwError;
            }

            if (dwConfidence >= CERT_CONFIDENCE_HIGHEST)
            {
                if (pdwError)
                {
                    *pdwError       = dwError;
                }

                if (pdwConfidence)
                {
                    *pdwConfidence  = dwConfidence;
                }

                CertFreeCertificateContext(pCertContext);

                return(pCertWithHighestConfidence);
            }

            fdwWork = CERT_STORE_SIGNATURE_FLAG;
        }
    }

    if (!(dwHighestConfidence & CERT_CONFIDENCE_HYGIENE))
    {
        if (pCertContext = _GetExternalIssuerCert(pChildContext, 
                                                  dwEncoding,
                                                  &fdwRetError, 
                                                  &dwConfidence,
                                                  psftVerifyAsOf))
        {
            if (dwHighestConfidence < dwConfidence)
            {
                CertFreeCertificateContext(pCertWithHighestConfidence);

                pCertWithHighestConfidence  = pCertContext;

                dwHighestConfidence         = dwConfidence;
            }
        }
    }

    if (pdwError)
    {
        *pdwError       = fdwRetError;
    }

    if (pdwConfidence)
    {
        *pdwConfidence  = dwHighestConfidence;
    }

    return(pCertWithHighestConfidence);
}

BOOL WINAPI TrustOpenStores(HCRYPTPROV hProv, OUT DWORD *pchStores, 
                            HCERTSTORE *pahStores, DWORD dwFlags)
{
    BOOL        fRet;
    DWORD       cs;
    HCERTSTORE  pas[WVT_STOREID_MAX];

    fRet = FALSE;

    if (!(pchStores) ||
        (dwFlags != 0))
    {
        goto ErrorInvalidParam;
    }

    cs          = 0;

    //
    //  ROOT store - ALWAYS #0 !!!!
    //
    if (!(pas[cs] = StoreProviderGetStore(hProv, WVT_STOREID_ROOT)))
    {
        goto ErrorNoRootStore;
    }

    cs++;

    if (pas[cs] = StoreProviderGetStore(hProv, WVT_STOREID_TRUST))
    {
        cs++;
    }

    if (pas[cs] = StoreProviderGetStore(hProv, WVT_STOREID_CA))
    {
        cs++;
    }
    
    if (pas[cs] = StoreProviderGetStore(hProv, WVT_STOREID_MY))
    {
        cs++;
    }

    if ((pahStores) && (cs > *pchStores))
    {
        *pchStores = cs;
        goto ErrorMoreData;
    }

    *pchStores = cs;

    fRet = TRUE;

    if (!(pahStores))
    {
        goto ErrorMoreData;
    }

    DWORD   i;

    for (i = 0; i < cs; i++)
    {
        pahStores[i] = pas[i];
    }

CommonReturn:
    return(fRet);


ErrorReturn:
    while (cs > 0)
    {
        CertCloseStore(pas[cs - 1], 0);
        cs--;
    }

    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, ErrorMoreData,     ERROR_MORE_DATA);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNoRootStore,  TRUST_E_SYSTEM_ERROR);
    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
}


BOOL WINAPI TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,
                                         DWORD dwEncoding, 
                                         DWORD dwFlags)
{
    if (!(pContext) ||
        (dwFlags != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(CertCompareCertificateName(dwEncoding, 
                                     &pContext->pCertInfo->Issuer,
                                     &pContext->pCertInfo->Subject)))
    {
        return(FALSE);
    }

    DWORD   dwFlag;

    dwFlag = CERT_STORE_SIGNATURE_FLAG;

    if (!(CertVerifySubjectCertificateContext(pContext, pContext, &dwFlag)) || 
        (dwFlag & CERT_STORE_SIGNATURE_FLAG))
    {
        return(FALSE);
    }

    return(TRUE);
}

#define sz_CRYPTNET_DLL                 "cryptnet.dll"
#define sz_CryptGetObjectUrl            "CryptGetObjectUrl"
#define sz_CryptRetrieveObjectByUrlW    "CryptRetrieveObjectByUrlW"
typedef BOOL (WINAPI *PFN_CRYPT_GET_OBJECT_URL)(
    IN LPCSTR pszUrlOid,
    IN LPVOID pvPara,
    IN DWORD dwFlags,
    OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
    IN OUT DWORD* pcbUrlArray,
    OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
    IN OUT OPTIONAL DWORD* pcbUrlInfo,
    IN OPTIONAL LPVOID pvReserved
    );

typedef BOOL (WINAPI *PFN_CRYPT_RETRIEVE_OBJECT_BY_URLW)(
    IN LPCWSTR pszUrl,
    IN LPCSTR pszObjectOid,
    IN DWORD dwRetrievalFlags,
    IN DWORD dwTimeout,
    OUT LPVOID* ppvObject,
    IN HCRYPTASYNC hAsyncRetrieve,
    IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
    IN OPTIONAL LPVOID pvVerify,
    IN OPTIONAL LPVOID pvReserved
    );


PCCERT_CONTEXT _GetExternalIssuerCert(PCCERT_CONTEXT pContext, 
                                      DWORD dwEncoding,
                                      DWORD *pdwRetError,
                                      DWORD *pdwConfidence,
                                      FILETIME *psftVerifyAsOf)
{
    *pdwConfidence  = 0;

#if (USE_IEv4CRYPT32)

    return(NULL);

#else


    DWORD                       cbUrlArray;
    CRYPT_URL_ARRAY             *pUrlArray;
    PCCERT_CONTEXT              pIssuer;
    PCCERT_CONTEXT              pCertBestMatch;
    DWORD                       dwHighestConfidence;
    DWORD                       dwConfidence;
    DWORD                       dwStatus;
    DWORD                       dwError;
    DWORD                       i;

    pCertBestMatch      = NULL;
    pIssuer             = NULL;
    pUrlArray           = NULL;
    cbUrlArray          = 0;
    dwHighestConfidence = 0;

    HMODULE hDll = NULL;
    PFN_CRYPT_GET_OBJECT_URL pfnCryptGetObjectUrl;
    PFN_CRYPT_RETRIEVE_OBJECT_BY_URLW pfnCryptRetrieveObjectByUrlW;

    if (NULL == (hDll = LoadLibraryA(sz_CRYPTNET_DLL)))
        goto LoadCryptNetDllError;

    if (NULL == (pfnCryptGetObjectUrl =
            (PFN_CRYPT_GET_OBJECT_URL) GetProcAddress(hDll,
                sz_CryptGetObjectUrl)))
        goto CryptGetObjectUrlProcAddressError;

    if (NULL == (pfnCryptRetrieveObjectByUrlW =
            (PFN_CRYPT_RETRIEVE_OBJECT_BY_URLW) GetProcAddress(hDll,
                sz_CryptRetrieveObjectByUrlW)))
        goto CryptRetrieveObjectByUrlWProcAddressError;


    if (!(pfnCryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)pContext, 0, NULL, &cbUrlArray, NULL, NULL, NULL)) ||
        (cbUrlArray < 1))
    {
        goto GetObjectUrlFailed;
    }

    if (!(pUrlArray = (CRYPT_URL_ARRAY *) new BYTE[cbUrlArray]))
    {
        goto MemoryError;
    }

    memset(pUrlArray, 0x00, cbUrlArray);

    if (!(pfnCryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)pContext, 0, pUrlArray, &cbUrlArray, NULL, NULL, NULL)))
    {
        goto GetObjectUrlFailed;
    }

    for (i = 0; i < pUrlArray->cUrl; i++)
    {
        if (pIssuer)
        {
            CertFreeCertificateContext(pIssuer);
            pIssuer = NULL;
        }

        if (pfnCryptRetrieveObjectByUrlW(pUrlArray->rgwszUrl[i], CONTEXT_OID_CERTIFICATE, 0, 0, (void **)&pIssuer,
                                      NULL, NULL, NULL, NULL))
        {
            if (!(CertCompareCertificateName(X509_ASN_ENCODING, &pContext->pCertInfo->Issuer,
                                             &pIssuer->pCertInfo->Subject)))
            {
                continue;
            }
    
            dwStatus = CERT_STORE_SIGNATURE_FLAG;

            if (!(CertVerifySubjectCertificateContext(pContext, pIssuer, &dwStatus)))
            {
                continue;
            }

            dwError = 0;
            _SetConfidenceOnIssuer(dwEncoding, pContext, pIssuer, dwStatus, psftVerifyAsOf, 
                                   &dwConfidence, &dwError);

            if (dwError != 0)
            {
                continue;
            }

            if (dwConfidence > dwHighestConfidence)
            {
                if (pCertBestMatch)
                {
                    CertFreeCertificateContext(pCertBestMatch);
                }

                dwHighestConfidence = dwConfidence;
                pCertBestMatch      = CertDuplicateCertificateContext(pIssuer);
            }

            if (dwConfidence >= CERT_CONFIDENCE_HIGHEST)
            {
                goto CommonReturn;
            }
        }
    }

    goto RetrieveObjectFailed;


CommonReturn:
    if (hDll)
        FreeLibrary(hDll);

    if (pIssuer)
    {
        CertFreeCertificateContext(pIssuer);
    }

    if (pUrlArray)
    {
        delete pUrlArray;
    }

    *pdwConfidence  = dwHighestConfidence;

    return(pCertBestMatch);

ErrorReturn:
    
    if (pCertBestMatch)
    {
        CertFreeCertificateContext(pCertBestMatch);
        pCertBestMatch = NULL;
    }

    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, LoadCryptNetDllError)
    TRACE_ERROR_EX(DBG_SS, CryptGetObjectUrlProcAddressError)
    TRACE_ERROR_EX(DBG_SS, CryptRetrieveObjectByUrlWProcAddressError)

    TRACE_ERROR_EX(DBG_SS, GetObjectUrlFailed);
    TRACE_ERROR_EX(DBG_SS, RetrieveObjectFailed);

    SET_ERROR_VAR_EX(DBG_SS, MemoryError, ERROR_NOT_ENOUGH_MEMORY);

#endif // USE_IEv4CRYPT32
}

BOOL WINAPI TrustDecode(DWORD dwModuleId, BYTE **ppbRet, DWORD *pcbRet, DWORD cbHint,
                        DWORD dwEncoding, const char *pcszOID, const BYTE *pbEncoded, DWORD cbEncoded,
                        DWORD dwDecodeFlags)
{
    if (!(*ppbRet = new BYTE[cbHint]))
    {
        goto MemoryError;
    }

    *pcbRet = cbHint;

    if (!(CryptDecodeObject(dwEncoding, pcszOID, pbEncoded, cbEncoded, dwDecodeFlags,
                            *ppbRet, pcbRet)))
    {
        if (GetLastError() != ERROR_MORE_DATA)
        {
            goto DecodeError;
        }
    }

    if (cbHint < *pcbRet)
    {
        DBG_PRINTF((DBG_SS, "****** TrustDecode(0x%08.8lX): recalling due to bad size: hint: %lu actual: %lu\r\n", 
                    dwModuleId, cbHint, *pcbRet));

        DELETE_OBJECT(*ppbRet);

        return(TrustDecode(dwModuleId, ppbRet, pcbRet, *pcbRet, dwEncoding, pcszOID, 
                           pbEncoded, cbEncoded, dwDecodeFlags));
    }

#   if DBG
        if ((cbHint / 3) > *pcbRet)
        {
            DBG_PRINTF((DBG_SS, "TrustDecode(0x%08.8lX): hint too big. hint: %lu actual: %lu\r\n", 
                        dwModuleId, cbHint, *pcbRet));
        }
#   endif

    return(TRUE);

ErrorReturn:
    DELETE_OBJECT(*ppbRet);
    return(FALSE);

    TRACE_ERROR_EX(DBG_SS, DecodeError);
    SET_ERROR_VAR_EX(DBG_SS, MemoryError, ERROR_NOT_ENOUGH_MEMORY);
}

BOOL WINAPI TrustFreeDecode(DWORD dwModuleId, BYTE **pbAllocated)
{
    DELETE_OBJECT(*pbAllocated);

    return(TRUE);
}

void _SetConfidenceOnIssuer(DWORD dwEncoding, PCCERT_CONTEXT pChildCert, PCCERT_CONTEXT pTestIssuerCert, 
                            DWORD dwVerificationFlag, FILETIME *psftVerifyAsOf, DWORD *pdwConfidence, 
                            DWORD *pdwError)
{
    *pdwConfidence = 0;

    if (!(dwVerificationFlag & CERT_STORE_SIGNATURE_FLAG))
    {
        *pdwConfidence  |= CERT_CONFIDENCE_SIG;
    }

    if (CertVerifyTimeValidity(psftVerifyAsOf, pTestIssuerCert->pCertInfo) == 0)
    {
        *pdwConfidence  |= CERT_CONFIDENCE_TIME;
    }

    if (CertVerifyValidityNesting(pChildCert->pCertInfo, pTestIssuerCert->pCertInfo))
    {
        *pdwConfidence  |= CERT_CONFIDENCE_TIMENEST;
    }

    if (_CompareAuthKeyId(dwEncoding, pChildCert, pTestIssuerCert))
    {
        *pdwConfidence  |= CERT_CONFIDENCE_AUTHIDEXT;
    }
    else if (_CompareAuthKeyId2(dwEncoding, pChildCert, pTestIssuerCert))
    {
        *pdwConfidence  |= CERT_CONFIDENCE_AUTHIDEXT;
    }

    if (_SetCertErrorAndHygiene(pChildCert, pTestIssuerCert, *pdwConfidence, pdwError))
    {
        *pdwConfidence  |= CERT_CONFIDENCE_HYGIENE;
    }
}

BOOL _SetCertErrorAndHygiene(PCCERT_CONTEXT pSubjectContext, PCCERT_CONTEXT pIssuerContext,
                             DWORD dwCurrentConfidence, DWORD *pdwError)
{
    *pdwError = ERROR_SUCCESS;

    if (!(dwCurrentConfidence & CERT_CONFIDENCE_SIG))
    {
        *pdwError = TRUST_E_CERT_SIGNATURE;
        return(FALSE);
    }

    if ((dwCurrentConfidence & CERT_CONFIDENCE_SIG)        &&
        (dwCurrentConfidence & CERT_CONFIDENCE_TIME)       &&
        (dwCurrentConfidence & CERT_CONFIDENCE_TIMENEST)   &&
        (dwCurrentConfidence & CERT_CONFIDENCE_AUTHIDEXT))
    {
        return(TRUE);
    }

    if (dwCurrentConfidence & CERT_CONFIDENCE_AUTHIDEXT)
    {
        return(TRUE);
    }

    return(FALSE);
}


BOOL _CompareAuthKeyId2(DWORD dwEncoding, PCCERT_CONTEXT pChildContext, PCCERT_CONTEXT pParentContext)
{
    DWORD                           i;
    PCERT_EXTENSION                 pExt;
    DWORD                           cbIdInfo;
    PCERT_AUTHORITY_KEY_ID2_INFO    pIdInfo;
    BOOL                            fRet;


    pIdInfo = NULL;

    if (pChildContext->pCertInfo->cExtension < 1)
    {
        goto NoExtensions;
    }

    if (!(pExt = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER2, pChildContext->pCertInfo->cExtension,
                                   pChildContext->pCertInfo->rgExtension)))
    {
        goto NoExtensions;
    }

    if (!(TrustDecode(WVT_MODID_WINTRUST, (BYTE **)&pIdInfo, &cbIdInfo, 103, 
                      dwEncoding, X509_AUTHORITY_KEY_ID2, pExt->Value.pbData, pExt->Value.cbData,
                      CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeFailed;
    }
    
    for (i = 0; i < pIdInfo->AuthorityCertIssuer.cAltEntry; i++)
    {
        if (pIdInfo->AuthorityCertIssuer.rgAltEntry[i].dwAltNameChoice == 
                        CERT_ALT_NAME_DIRECTORY_NAME)
        {
            break;
        }
    }

    if (i == pIdInfo->AuthorityCertIssuer.cAltEntry)
    {
        goto NoAltDirectoryName;
    }

    if (!(CertCompareCertificateName(dwEncoding, 
                                     &pIdInfo->AuthorityCertIssuer.rgAltEntry[i].DirectoryName,
                                     &pParentContext->pCertInfo->Issuer)))
    {
        goto IncorrectIssuer;
    }

    //
    //  issuer certificate's serial number must match
    //
    if (!(CertCompareIntegerBlob(&pIdInfo->AuthorityCertSerialNumber,
                                 &pParentContext->pCertInfo->SerialNumber)))
    {
        goto IncorrectIssuer;
    }

    fRet = TRUE;

CommonReturn:
    if (pIdInfo)
    {
        TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pIdInfo);
    }
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, NoExtensions);
    TRACE_ERROR_EX(DBG_SS, DecodeFailed);
    TRACE_ERROR_EX(DBG_SS, IncorrectIssuer);
    TRACE_ERROR_EX(DBG_SS, NoAltDirectoryName);
}

BOOL _CompareAuthKeyId(DWORD dwEncoding, PCCERT_CONTEXT pChildContext, PCCERT_CONTEXT pParentContext)
{
    PCERT_EXTENSION             pExt;
    PCERT_AUTHORITY_KEY_ID_INFO pChildKeyIdInfo;
    DWORD                       cbKeyIdInfo;
    BOOL                        fRet;

    pChildKeyIdInfo = NULL;
    pExt            = NULL;

    if (pChildContext->pCertInfo->cExtension < 1)
    {
        goto NoExtensions;
    }

    pChildKeyIdInfo     = NULL;

    if (!(pExt = CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER,
                                   pChildContext->pCertInfo->cExtension,
                                   pChildContext->pCertInfo->rgExtension)))
    {
        goto NoExtensions;
    }

    if (!(TrustDecode(WVT_MODID_WINTRUST, (BYTE **)&pChildKeyIdInfo, &cbKeyIdInfo, 104, 
                      dwEncoding, X509_AUTHORITY_KEY_ID, pExt->Value.pbData, pExt->Value.cbData,
                      CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeFailed;
    }
    
    if ((pChildKeyIdInfo->CertIssuer.cbData < 1) ||
        (pChildKeyIdInfo->CertSerialNumber.cbData < 1))
    {
        goto NoKeyId;
    }

    //
    //  issuer certificate's issuer name must match
    //
    if (!(CertCompareCertificateName(dwEncoding, &pChildKeyIdInfo->CertIssuer, 
                                     &pParentContext->pCertInfo->Issuer)))
    {
        goto IncorrectIssuer;
    }

    //
    //  issuer certificate's serial number must match
    //
    if (!(CertCompareIntegerBlob(&pChildKeyIdInfo->CertSerialNumber,
                                 &pParentContext->pCertInfo->SerialNumber)))
    {
        goto IncorrectIssuer;
    }

    fRet = TRUE;

CommonReturn:
    if (pChildKeyIdInfo)
    {
        TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pChildKeyIdInfo);
    }
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, NoExtensions);
    TRACE_ERROR_EX(DBG_SS, DecodeFailed);
    TRACE_ERROR_EX(DBG_SS, NoKeyId);
    TRACE_ERROR_EX(DBG_SS, IncorrectIssuer);
}

