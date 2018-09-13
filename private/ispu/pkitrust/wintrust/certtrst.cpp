//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       certtrst.cpp
//
//  Contents:   Microsoft Internet Security Provider
//
//  Functions:  WintrustCertificateTrust
//
//              *** local functions ***
//              _WalkChain
//
//  History:    07-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

//
//  support for MS test roots!!!!
//
static BYTE rgbTestRoot[] = 
{
#   include "certs\\mstest1.h"
};

static BYTE rgbTestRootCorrected[] = 
{
#   include "certs\\mstest2.h"
};

static BYTE rgbTestRootBeta1[] = 
{
#   include "certs\\mstestb1.h"
};

#define NTESTROOTS  3
static CERT_PUBLIC_KEY_INFO rgTestRootPublicKeyInfo[NTESTROOTS] = 
{
    {szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRoot),           rgbTestRoot,            0},
    {szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRootCorrected),  rgbTestRootCorrected,   0},
    {szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRootBeta1),      rgbTestRootBeta1,       0}
};


#define     MAX_CERT_CHAIN              20


BOOL _WalkChain(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, DWORD *pdwError,
                BOOL fCounterSigner, DWORD idxCounterSigner);
BOOL _IE4TrustWalkChain(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, DWORD *pdwError,
                BOOL fCounterSigner, DWORD idxCounterSigner);


HRESULT WINAPI WintrustCertificateTrust(CRYPT_PROVIDER_DATA *pProvData)
{
    if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, fRecallWithState)) &&
        (pProvData->fRecallWithState == TRUE))
    {
        return(S_OK);
    }

    DWORD                   dwError;

    dwError = S_OK;

    if (pProvData->csSigners < 1)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_NOSIGNATURE;
        return(S_FALSE);
    }

    if (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG) {
        if (pProvData->chStores == 0)
        {
            //
            //  signature provider didn't open them....  let's do it now!
            //
            DWORD       i;
            DWORD       cs;
            HCERTSTORE  pas[WVT_STOREID_MAX];

            cs = WVT_STOREID_MAX;
            if (TrustOpenStores(pProvData->hProv, &cs, &pas[0], 0))
            {
                for (i = 0; i < cs; i++)
                {
                    pProvData->psPfns->pfnAddStore2Chain(pProvData, pas[i]);
                    CertCloseStore(pas[i], 0);
                }
            }
        }
    }

    //
    //  loop through all signers
    //
    for (int i = 0; i < (int)pProvData->csSigners; i++)
    {
        if (pProvData->pasSigners[i].csCertChain < 1)
        {
            pProvData->pasSigners[i].dwError = TRUST_E_NO_SIGNER_CERT;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_NO_SIGNER_CERT;
            continue;
        }

        if (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG)
            _IE4TrustWalkChain(pProvData, i, &dwError, FALSE, 0);
        else
            _WalkChain(pProvData, i, &dwError, FALSE, 0);

        for (int i2 = 0; i2 < (int)pProvData->pasSigners[i].csCounterSigners; i2++)
        {
            if (pProvData->pasSigners[i].pasCounterSigners[i2].csCertChain < 1)
            {
                pProvData->pasSigners[i].pasCounterSigners[i2].dwError = TRUST_E_NO_SIGNER_CERT;
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_COUNTER_SIGNER;
                dwError = S_FALSE;
                continue;
            }

            if (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG)
                _IE4TrustWalkChain(pProvData, i, &dwError, TRUE, i2);
            else
                _WalkChain(pProvData, i, &dwError, TRUE, i2);
        }
    }

    return(dwError);
}


HCERTCHAINENGINE GetChainEngine(
    IN CRYPT_PROVIDER_DATA *pProvData
    )
{
    CERT_CHAIN_ENGINE_CONFIG Config;
    HCERTSTORE hStore = NULL;
    HCERTCHAINENGINE hChainEngine = NULL;

    if (NULL == pProvData->pWintrustData ||
            pProvData->pWintrustData->dwUnionChoice != WTD_CHOICE_CERT ||
            !_ISINSTRUCT(WINTRUST_CERT_INFO,
                pProvData->pWintrustData->pCert->cbStruct, dwFlags) ||
            0 == (pProvData->pWintrustData->pCert->dwFlags & 
                    (WTCI_DONT_OPEN_STORES | WTCI_OPEN_ONLY_ROOT)))
        return NULL;

    memset(&Config, 0, sizeof(Config));
    Config.cbSize = sizeof(Config);

    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto OpenMemoryStoreError;

    if (pProvData->pWintrustData->pCert->dwFlags & WTCI_DONT_OPEN_STORES)
        Config.hRestrictedRoot = hStore;
    Config.hRestrictedTrust = hStore;
    Config.hRestrictedOther = hStore;

    if (!CertCreateCertificateChainEngine(
            &Config,
            &hChainEngine
            ))
        goto CreateChainEngineError;

CommonReturn:
    CertCloseStore(hStore, 0);
    return hChainEngine;
ErrorReturn:
    hChainEngine = NULL;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS, OpenMemoryStoreError)
TRACE_ERROR_EX(DBG_SS, CreateChainEngineError)
}


HCERTSTORE GetChainAdditionalStore(
    IN CRYPT_PROVIDER_DATA *pProvData
    )
{
    HCERTSTORE hStore = NULL;

    if (0 == pProvData->chStores)
        return NULL;

    if (1 < pProvData->chStores) {
        if (hStore = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )) {
            DWORD i;
            for (i = 0; i < pProvData->chStores; i++)
                CertAddStoreToCollection(
                    hStore,
                    pProvData->pahStores[i],
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
        }
    } else
        hStore = CertDuplicateStore(pProvData->pahStores[0]);

#if 0
    CertSaveStore(
            hStore,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            CERT_STORE_SAVE_AS_STORE,
            CERT_STORE_SAVE_TO_FILENAME_A,
            (void *) "C:\\temp\\wintrust.sto",
            0                   // dwFlags
            );
#endif

    return hStore;
}

BOOL UpdateCertProvChain(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN DWORD idxSigner,
    OUT DWORD *pdwError, 
    IN BOOL fCounterSigner,
    IN DWORD idxCounterSigner,
    IN PCRYPT_PROVIDER_SGNR pSgnr,
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    BOOL fTestCert = FALSE;
    DWORD dwSgnrError = 0;
    DWORD i;

    // The chain better have at least the certificate we passed in
    assert(0 < pChainContext->cChain &&
        0 < pChainContext->rgpChain[0]->cElement);

    for (i = 0; i < pChainContext->cChain; i++) {
        DWORD j;
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        for (j = 0; j < pChain->cElement; j++) {
            PCERT_CHAIN_ELEMENT pEle = pChain->rgpElement[j];
            DWORD dwEleError = pEle->TrustStatus.dwErrorStatus;
            DWORD dwEleInfo = pEle->TrustStatus.dwInfoStatus;
            PCRYPT_PROVIDER_CERT pProvCert;

            if (0 != i || 0 != j) {
                if (!(pProvData->psPfns->pfnAddCert2Chain(
                        pProvData, idxSigner, fCounterSigner, 
                        idxCounterSigner, pEle->pCertContext)))
                {
                    pProvData->dwError = GetLastError();
                    dwSgnrError = TRUST_E_SYSTEM_ERROR;
                    goto CommonReturn;
                }
            }
            //
            // else
            //  Signer cert has already been added
            pProvCert = &pSgnr->pasCertChain[pSgnr->csCertChain -1];

            pProvCert->fSelfSigned =
                0 != (dwEleInfo & CERT_TRUST_IS_SELF_SIGNED) &&
                0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID);

            pProvCert->fTrustedRoot =
                pProvCert->fSelfSigned &&
                i == pChainContext->cChain - 1 &&
                j == pChain->cElement - 1 &&
                0 == (dwEleError & CERT_TRUST_IS_UNTRUSTED_ROOT);


            if (pProvCert->fSelfSigned) {
                // Check if one of the "test" roots
                DWORD k;

                for (k = 0; k < NTESTROOTS; k++) 
                {
                    if (CertComparePublicKeyInfo(
                            pProvData->dwEncoding,
                            &pProvCert->pCert->pCertInfo->SubjectPublicKeyInfo,
                            &rgTestRootPublicKeyInfo[k]))
                    {
                        pProvCert->fTestCert = TRUE;
                        fTestCert = TRUE;
                        if (pProvData->dwRegPolicySettings & WTPF_TRUSTTEST)
                            pProvCert->fTrustedRoot = TRUE;
                        break;
                    }
                }
            }

            // First Element in all but the first simple chain
            pProvCert->fTrustListSignerCert = (0 < i && 0 == j);

            pProvCert->fIsCyclic = (0 != (dwEleError & CERT_TRUST_IS_CYCLIC));

            // Map to IE4Trust confidence
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_SIG;
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_TIME_VALID))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_TIME;

            // On Sep 10, 1998 Trevor/Brian wanted time nesting checks to
            // be disabled
            // if (0 == (dwEleError & CERT_TRUST_IS_NOT_TIME_NESTED))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_TIMENEST;

            if (0 != (dwEleInfo & CERT_TRUST_HAS_EXACT_MATCH_ISSUER))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_AUTHIDEXT;
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID) &&
                    0 != (dwEleInfo & CERT_TRUST_HAS_EXACT_MATCH_ISSUER))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_HYGIENE;

            if (pEle->pRevocationInfo) {
                assert(dwEleError & (CERT_TRUST_IS_REVOKED |
                     CERT_TRUST_REVOCATION_STATUS_UNKNOWN));
                pProvCert->dwRevokedReason =
                    pEle->pRevocationInfo->dwRevocationResult;
            }

            // Update any signature or revocations errors
            if (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID) {
                pProvCert->dwError = TRUST_E_CERT_SIGNATURE;
                assert(pChainContext->TrustStatus.dwErrorStatus &
                    CERT_TRUST_IS_NOT_SIGNATURE_VALID);
                dwSgnrError = TRUST_E_CERT_SIGNATURE;
            } else if (dwEleError & CERT_TRUST_IS_REVOKED) {
                pProvCert->dwError = CERT_E_REVOKED;
                assert(pChainContext->TrustStatus.dwErrorStatus &
                    CERT_TRUST_IS_REVOKED);
                if (0 == dwSgnrError ||
                        CERT_E_REVOCATION_FAILURE == dwSgnrError)
                    dwSgnrError = CERT_E_REVOKED;
            } else if (dwEleError & CERT_TRUST_REVOCATION_STATUS_UNKNOWN) {
                pProvCert->dwError = CERT_E_REVOCATION_FAILURE;
                assert(pChainContext->TrustStatus.dwErrorStatus &
                    CERT_TRUST_REVOCATION_STATUS_UNKNOWN);
                if (0 == dwSgnrError)
                    dwSgnrError = CERT_E_REVOCATION_FAILURE;
            }

            // If last element in simple chain, check if it was in a
            // CTL and update CryptProvData if it was.
            if (j == pChain->cElement - 1 && pChain->pTrustListInfo &&
                    pChain->pTrustListInfo->pCtlContext) {
                DWORD dwChainError = pChain->TrustStatus.dwErrorStatus;

                // Note, don't need to AddRef since we already hold an
                // AddRef on the ChainContext.
                pProvCert->pCtlContext = pChain->pTrustListInfo->pCtlContext;

                if (dwChainError & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID) {
                    pProvCert->dwCtlError = TRUST_E_CERT_SIGNATURE;
                    dwSgnrError = TRUST_E_CERT_SIGNATURE;
                } else if (dwChainError & CERT_TRUST_CTL_IS_NOT_TIME_VALID) {
                    if (0 == (pProvData->dwRegPolicySettings &
                            WTPF_IGNOREEXPIRATION))
                    pProvCert->dwCtlError = CERT_E_EXPIRED;
                } else if (dwChainError &
                        CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE) {
                    pProvCert->dwCtlError = CERT_E_WRONG_USAGE;
                }
            }


            if (pProvData->psPfns->pfnCertCheckPolicy) {
                if (! (*pProvData->psPfns->pfnCertCheckPolicy)(
                    pProvData, idxSigner, fCounterSigner, idxCounterSigner))
                goto CommonReturn;
            }
        }
    }

CommonReturn:
    if (fTestCert) {
        if (CERT_TRUST_IS_REVOKED == dwSgnrError ||
                CERT_E_REVOCATION_FAILURE == dwSgnrError) {
            // No revocation errors for "test" roots
            dwSgnrError = 0;

            // Loop through certs and remove any revocation error status
            for (i = 0; i < pSgnr->csCertChain; i++) {
                PCRYPT_PROVIDER_CERT pProvCert = &pSgnr->pasCertChain[i];
                pProvCert->dwError = 0;
                pProvCert->dwRevokedReason = 0;
            }
        }
    }

    if (CERT_E_REVOCATION_FAILURE == dwSgnrError &&
            pProvData->pWintrustData->fdwRevocationChecks !=
                WTD_REVOKE_WHOLECHAIN)
        // Will check during Final Policy
        dwSgnrError = 0;

    if (dwSgnrError) {
        pSgnr->dwError = dwSgnrError;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
            dwSgnrError;
        *pdwError = S_FALSE;
        return FALSE;
    } else
        return TRUE;
}

BOOL _WalkChain(
    CRYPT_PROVIDER_DATA *pProvData,
    DWORD idxSigner,
    DWORD *pdwError, 
    BOOL fCounterSigner,
    DWORD idxCounterSigner
    )
{
    BOOL fResult;
    DWORD dwCreateChainFlags;
    DWORD dwSgnrError = 0;
    CRYPT_PROVIDER_SGNR *pSgnr;         // not allocated
    PCCERT_CONTEXT pCertContext;        // not allocated

    CERT_CHAIN_PARA ChainPara;
    HCERTCHAINENGINE hChainEngine = NULL;
    HCERTSTORE hAdditionalStore = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    LPSTR pszUsage = NULL;

    if (fCounterSigner)
        pSgnr = &pProvData->pasSigners[idxSigner].pasCounterSigners[
            idxCounterSigner];
    else
        pSgnr = &pProvData->pasSigners[idxSigner];
    assert(pSgnr);

    //
    //  at this stage, the last cert in the chain "should be" the signers cert.
    //  eg: there should only be one cert in the chain from the Signature
    //  Provider
    //
    if (1 != pSgnr->csCertChain ||
            (NULL == (pCertContext =
                pSgnr->pasCertChain[pSgnr->csCertChain - 1].pCert))) {
        dwSgnrError = TRUST_E_NO_SIGNER_CERT;
        goto NoSignerCertError;
    }


    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    if (fCounterSigner && SGNR_TYPE_TIMESTAMP == pSgnr->dwSignerType) {
        pszUsage = szOID_PKIX_KP_TIMESTAMP_SIGNING;
    } else if(NULL != pProvData->pRequestUsage) {
        ChainPara.RequestedUsage = *pProvData->pRequestUsage;
    } else {
        pszUsage = pProvData->pszUsageOID;
    }
    
    if ( (0 == (pProvData->dwProvFlags & WTD_NO_POLICY_USAGE_FLAG)) && pszUsage) {
        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &pszUsage;
    }
    hChainEngine = GetChainEngine(pProvData);
    hAdditionalStore = GetChainAdditionalStore(pProvData);

    dwCreateChainFlags = 0;
    if (pProvData->dwProvFlags & CPD_REVOCATION_CHECK_NONE) {
        ;
    } else if (pProvData->dwProvFlags & CPD_REVOCATION_CHECK_END_CERT) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    } else if (pProvData->dwProvFlags & CPD_REVOCATION_CHECK_CHAIN) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    } else if (pProvData->dwProvFlags &
            CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    } else if (pProvData->dwProvFlags & WTD_REVOCATION_CHECK_NONE) {
        ;
    } else if (pProvData->dwProvFlags & WTD_REVOCATION_CHECK_END_CERT) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    } else if (pProvData->dwProvFlags & WTD_REVOCATION_CHECK_CHAIN) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    } else if (pProvData->dwProvFlags &
            WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    } else if (pProvData->pWintrustData->fdwRevocationChecks ==
            WTD_REVOKE_WHOLECHAIN) {
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    } else if (fCounterSigner && SGNR_TYPE_TIMESTAMP == pSgnr->dwSignerType) {
        if (0 == (pProvData->dwRegPolicySettings & WTPF_IGNOREREVOCATIONONTS))
            dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    } else if (0 == (pProvData->dwRegPolicySettings & WTPF_IGNOREREVOKATION))
        dwCreateChainFlags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;

    if (!(pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CERT ||
            pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_SIGNER))
        // Certificate was obtained from either the message store, or one
        // of our known stores
        dwCreateChainFlags |= CERT_CHAIN_CACHE_END_CERT;
    // else
    //  Can't implicitly cache a context passed to us


#if 0
    {
        HCERTSTORE hDebugStore;
        if (hDebugStore = CertOpenSystemStoreA(0, "wintrust")) {
            CertAddCertificateContextToStore(
                hDebugStore,
                pCertContext,
                CERT_STORE_ADD_ALWAYS,
                NULL
                );
            CertCloseStore(hDebugStore, 0);
        }
    }
#endif

    FILETIME fileTime;

    memset(&fileTime, 0, sizeof(fileTime));

    
    if (!CertGetCertificateChain (
            hChainEngine,
            pCertContext,
            (memcmp(&pSgnr->sftVerifyAsOf, &fileTime, sizeof(fileTime)) == 0) ? 
                    NULL : &pSgnr->sftVerifyAsOf,
            hAdditionalStore,
            &ChainPara,
            dwCreateChainFlags,
            NULL,                       // pvReserved,
            &pChainContext
            )) {
        pProvData->dwError = GetLastError();
        dwSgnrError = TRUST_E_SYSTEM_ERROR;
        goto GetChainError;
    }

    pSgnr->pChainContext = pChainContext;
    if (pProvData->dwProvFlags & WTD_NO_IE4_CHAIN_FLAG) {
        pProvData->dwProvFlags |= CPD_USE_NT5_CHAIN_FLAG;
        fResult = TRUE;
    } else
        fResult = UpdateCertProvChain(
            pProvData,
            idxSigner,
            pdwError, 
            fCounterSigner,
            idxCounterSigner,
            pSgnr,
            pChainContext
            );
                                        
CommonReturn:
    if (hChainEngine)
        CertFreeCertificateChainEngine(hChainEngine);
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);
    return fResult;
ErrorReturn:
    pSgnr->dwError = dwSgnrError;
    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
        dwSgnrError;
    *pdwError = S_FALSE;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS, NoSignerCertError)
TRACE_ERROR_EX(DBG_SS, GetChainError)
}

BOOL _IE4TrustWalkChain(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, DWORD *pdwError, 
                BOOL fCounterSigner, DWORD idxCounterSigner)
{
    CRYPT_PROVIDER_SGNR     *pSgnr;
    PCCERT_CONTEXT          pCertContext;
    DWORD                   cCertsOld;
    DWORD                   dwSaveConfidence;
    int                     cLoops;
    FILETIME                *psftCheckTime;

    if (fCounterSigner)
    {
        pSgnr = &pProvData->pasSigners[idxSigner].pasCounterSigners[idxCounterSigner];
    }
    else
    {
        pSgnr = &pProvData->pasSigners[idxSigner];
    }

    //
    //  at this stage, the last cert in the chain "should be" the signers cert.
    //  eg: there should only be one cert in the chain from the Signature Provider
    //
    pCertContext = pSgnr->pasCertChain[pSgnr->csCertChain - 1].pCert;

    if (!(pCertContext))
    {
        pSgnr->dwError = TRUST_E_NO_SIGNER_CERT;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_NO_SIGNER_CERT;
        *pdwError       = S_FALSE;
        return(FALSE);
    }

    psftCheckTime = &pSgnr->sftVerifyAsOf;

    pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence = 0;

    if (CertVerifyTimeValidity(&pSgnr->sftVerifyAsOf, pCertContext->pCertInfo) == 0)
    {
        pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence = CERT_CONFIDENCE_TIME;
    }
    //
    //  start getting issuers
    //
    cLoops = 0;

    for EVER
    {
        //
        //  we could be in an infinite loop!
        //
        if (++cLoops > MAX_CERT_CHAIN)
        {
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = CERT_E_CHAINING;
            *pdwError       = S_FALSE;
            return(FALSE);
        }

        //
        //  set if self-signed
        //
        if (TrustIsCertificateSelfSigned(pSgnr->pasCertChain[pSgnr->csCertChain - 1].pCert,
                                         pProvData->dwEncoding, 
                                         0))
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].fSelfSigned = TRUE;
        }

        //
        //  is in root store
        //
        if (WTHelperIsInRootStore(pProvData, pCertContext))
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].fTrustedRoot = TRUE;
        }
        else
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].fTrustedRoot = FALSE;
        }

        //
        //  is one of our "test" certs?
        //
        for (int i = 0; i < NTESTROOTS; i++) 
        {
            if (CertComparePublicKeyInfo(pProvData->dwEncoding,
                                         &pCertContext->pCertInfo->SubjectPublicKeyInfo,
                                         &rgTestRootPublicKeyInfo[i]))
            {
                pSgnr->pasCertChain[pSgnr->csCertChain - 1].fTestCert = TRUE;
                break;
            }
        }

        // 
        //  call the Certificate Check Provider if applicable.
        //
        if (pProvData->psPfns->pfnCertCheckPolicy)
        {
            cCertsOld = pSgnr->csCertChain;

            if (!(*pProvData->psPfns->pfnCertCheckPolicy)(pProvData, idxSigner, 
                                                          fCounterSigner, idxCounterSigner))
            {
                // certificate policy says "quit"!
                break;
            }

            if (cCertsOld != pSgnr->csCertChain)
            {
                //
                //  the Certificate Check Provider added a certificate.  back to the
                //  top!
                //
                continue;
            }
            else if (pSgnr->pasCertChain[pSgnr->csCertChain - 1].fSelfSigned)
            {
                //
                //  we've called the cert policy and it said to keep going.  However
                //  this can't be done on a self-signed cert!
                //
                break;
            }

            pCertContext = pSgnr->pasCertChain[pSgnr->csCertChain - 1].pCert;
        }
        else if (pSgnr->pasCertChain[pSgnr->csCertChain - 1].fSelfSigned)
        {
            //
            //  no cert check policy, and we're self-signed... get out!
            //
            break;
        }

        //
        //  save this one and put selected bits back later that really pertain to
        //  the child cert.
        //
        dwSaveConfidence = pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence;

        if (pSgnr->pasCertChain[pSgnr->csCertChain - 1].fTrustListSignerCert)
        {
            //
            //  the trust list chain must be verified using the system time.
            //
            psftCheckTime = &pProvData->sftSystemTime;
        }

        //
        //  find the issuer
        //
        pCertContext = TrustFindIssuerCertificate(pCertContext,
                                                  pProvData->dwEncoding,
                                                  pProvData->chStores,
                                                  pProvData->pahStores,
                                                  psftCheckTime,
                                                  &pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence,
                                                  pdwError,
                                                  0);

        if (!(pCertContext))
        {
            // no more... get out!
            break;
        }

        if (*pdwError != ERROR_SUCCESS)
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwError = *pdwError;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = *pdwError;
        }


        //
        //  add to our chain...
        //
        if (!(pProvData->psPfns->pfnAddCert2Chain(pProvData, idxSigner, fCounterSigner, 
                                                  idxCounterSigner, pCertContext)))
        {
            pProvData->dwError = GetLastError();
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_SYSTEM_ERROR;
        
            CertFreeCertificateContext(pCertContext);
        
            *pdwError   = S_FALSE;
            return(FALSE);
        }
        
        CertFreeCertificateContext(pCertContext);

        //
        //  assign the bits from the child (csCertChain - 2) that 
        //  pertain to the issuer (csCertChain = 1)
        //
        if (pSgnr->pasCertChain[pSgnr->csCertChain - 2].dwConfidence & CERT_CONFIDENCE_TIME)
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence |= CERT_CONFIDENCE_TIME;
        }
        else
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 1].dwConfidence &= ~(CERT_CONFIDENCE_TIME);
        }

        //
        //  reset the bits that we saved of the childs (csCertChain - 2)
        //
        if (dwSaveConfidence & CERT_CONFIDENCE_TIME)
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 2].dwConfidence |= CERT_CONFIDENCE_TIME;
        }
        else
        {
            pSgnr->pasCertChain[pSgnr->csCertChain - 2].dwConfidence &= ~(CERT_CONFIDENCE_TIME);
        }

        //
        //  assign it again...  because the addcert2chain does a dup and the cert check policy
        //  could have added another....
        //
        pCertContext    = pSgnr->pasCertChain[pSgnr->csCertChain - 1].pCert;

    }   // end of for EVER.

    //
    //  check application requested revocation checks....
    //
    if (pProvData->pWintrustData->fdwRevocationChecks != WTD_REVOKE_WHOLECHAIN)
    {
        return(TRUE);
    }

    //
    //  no revokation checking on test certs!
    //
    for (int i2 = 0; i2 < (int)pSgnr->csCertChain; i2++)
    {
        if (pSgnr->pasCertChain[i2].fTestCert)
        {
            return(TRUE);
        }
    }


    CERT_REVOCATION_PARA    sRevPara;
    CERT_REVOCATION_STATUS  sRevStatus;
    PCERT_CONTEXT           *papsCertContext;

    if (!(papsCertContext = (PCERT_CONTEXT *)pProvData->psPfns->pfnAlloc(sizeof(PCERT_CONTEXT) * 
                                                                         pSgnr->csCertChain)))
    {
        pProvData->dwError = GetLastError();

        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = TRUST_E_SYSTEM_ERROR;

        *pdwError   = S_FALSE;

        return(FALSE);
    }

    for (int i = 0; i < (int)pSgnr->csCertChain; i++)
    {
        papsCertContext[i] = (PCERT_CONTEXT)pSgnr->pasCertChain[i].pCert;
    }

    memset(&sRevPara, 0x00, sizeof(CERT_REVOCATION_PARA));
    sRevPara.cbSize         = sizeof(CERT_REVOCATION_PARA);

    memset(&sRevStatus, 0x00, sizeof(CERT_REVOCATION_STATUS));
    sRevStatus.cbSize       = sizeof(CERT_REVOCATION_STATUS);

    if (!(CertVerifyRevocation(pProvData->dwEncoding,
                               CERT_CONTEXT_REVOCATION_TYPE,
                               pSgnr->csCertChain,
                               (void **)papsCertContext,
                               CERT_VERIFY_REV_CHAIN_FLAG,
                               &sRevPara,
                               &sRevStatus)))
    {
        pProvData->psPfns->pfnFree(papsCertContext);

        pSgnr->pasCertChain[sRevStatus.dwIndex].dwRevokedReason = sRevStatus.dwReason;

        switch(sRevStatus.dwError)
        {
            case CRYPT_E_REVOKED:
                pSgnr->pasCertChain[sRevStatus.dwIndex].dwError                 = CERT_E_REVOKED;
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]  = CERT_E_REVOKED;
                return(FALSE);
        }

        pSgnr->pasCertChain[sRevStatus.dwIndex].dwError                 = CERT_E_REVOCATION_FAILURE;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]  = CERT_E_REVOCATION_FAILURE;
        return(FALSE);
    }

    pProvData->psPfns->pfnFree(papsCertContext);

    return(TRUE);
}

