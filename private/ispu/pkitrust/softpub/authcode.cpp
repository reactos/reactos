//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       authcode.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubAuthenticode
//
//  History:    05-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

HRESULT WINAPI IE4TrustSoftpubAuthenticode(CRYPT_PROVIDER_DATA *pProvData);

static void UpdateCertError(
    IN PCRYPT_PROVIDER_SGNR pSgnr,
    IN PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    PCCERT_CHAIN_CONTEXT pChainContext = pSgnr->pChainContext;
    LONG lChainIndex = pPolicyStatus->lChainIndex;
    LONG lElementIndex = pPolicyStatus->lElementIndex;
    DWORD dwProvCertIndex;
    LONG i;

    assert (lChainIndex < (LONG) pChainContext->cChain);
    if (0 > lChainIndex || lChainIndex >= (LONG) pChainContext->cChain ||
            0 > lElementIndex) {
        if (CERT_E_CHAINING == pPolicyStatus->dwError) {
            if (0 < pSgnr->csCertChain) {
                PCRYPT_PROVIDER_CERT pProvCert;

                pProvCert = WTHelperGetProvCertFromChain(
                    pSgnr, pSgnr->csCertChain - 1);
                if (0 == pProvCert->dwError)
                    pProvCert->dwError = pPolicyStatus->dwError;
            }
        }
        return;
    }

    dwProvCertIndex = 0;
    for (i = 0; i < lChainIndex; i++) {
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        dwProvCertIndex += pChain->cElement;
    }
    dwProvCertIndex += lElementIndex;

    if (dwProvCertIndex < pSgnr->csCertChain) {
        PCRYPT_PROVIDER_CERT pProvCert;

        pProvCert = WTHelperGetProvCertFromChain(pSgnr, dwProvCertIndex);
        pProvCert->dwError = pPolicyStatus->dwError;
    }
}
    
HRESULT WINAPI SoftpubAuthenticode(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwFinalError) ||
            (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG) ||
            0 == (pProvData->dwProvFlags & CPD_USE_NT5_CHAIN_FLAG))
        return IE4TrustSoftpubAuthenticode(pProvData);

    DWORD dwError;
    DWORD i1;

    AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA ExtraPolicyPara;
    memset(&ExtraPolicyPara, 0, sizeof(ExtraPolicyPara));
    ExtraPolicyPara.cbSize = sizeof(ExtraPolicyPara);
    ExtraPolicyPara.dwRegPolicySettings = pProvData->dwRegPolicySettings;

    CERT_CHAIN_POLICY_PARA PolicyPara;
    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = (void *) &ExtraPolicyPara;

    AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS ExtraPolicyStatus;
    memset(&ExtraPolicyStatus, 0, sizeof(ExtraPolicyStatus));
    ExtraPolicyStatus.cbSize = sizeof(ExtraPolicyStatus);

    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.pvExtraPolicyStatus = (void *) &ExtraPolicyStatus;


    //
    // check the high level error codes.
    //
    if (0 != (dwError = checkGetErrorBasedOnStepErrors(pProvData)))
        goto CommonReturn;


    //
    //  check each signer
    //
    for (i1 = 0; i1 < pProvData->csSigners; i1++) {
        CRYPT_PROVIDER_SGNR *pSgnr;
        LPSTR pszUsage;

        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i1, FALSE, 0);

        pszUsage = pProvData->pszUsageOID;
        if (pszUsage && 0 != strcmp(pszUsage, szOID_PKIX_KP_CODE_SIGNING))
            // Inhibit checking of signer purpose
            ExtraPolicyPara.pSignerInfo = NULL;
        else
            ExtraPolicyPara.pSignerInfo = pSgnr->psSigner;

        if (!CertVerifyCertificateChainPolicy(
                CERT_CHAIN_POLICY_AUTHENTICODE,
                pSgnr->pChainContext,
                &PolicyPara,
                &PolicyStatus
                )) {
            dwError = TRUST_E_SYSTEM_ERROR;
            goto CommonReturn;
        }

        if (ExtraPolicyStatus.fCommercial) {
            // Mark all the certificates in the chain as being commercial
            for (DWORD i2 = 0; i2 < pSgnr->csCertChain; i2++) {
                PCRYPT_PROVIDER_CERT pProvCert =
                    WTHelperGetProvCertFromChain(pSgnr, i2);
                pProvCert->fCommercial = TRUE;
            }
        }

        if (0 != PolicyStatus.dwError) {
            dwError = PolicyStatus.dwError;
            UpdateCertError(pSgnr, &PolicyStatus);
            goto CommonReturn;
        } else if (0 < pSgnr->csCertChain) {
            PCRYPT_PROVIDER_CERT pProvCert;

            pProvCert = WTHelperGetProvCertFromChain(pSgnr, 0);
            if (CERT_E_REVOCATION_FAILURE == pProvCert->dwError) {
                // Policy says to ignore offline revocation errors
                pProvCert->dwError = 0;
                pProvCert->dwRevokedReason = 0;
            }
        }

        if (pSgnr->csCounterSigners) {
            AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA TSExtraPolicyPara;
            memset(&TSExtraPolicyPara, 0, sizeof(TSExtraPolicyPara));
            TSExtraPolicyPara.cbSize = sizeof(TSExtraPolicyPara);
            TSExtraPolicyPara.dwRegPolicySettings =
                pProvData->dwRegPolicySettings;
            TSExtraPolicyPara.fCommercial = ExtraPolicyStatus.fCommercial;

            CERT_CHAIN_POLICY_PARA TSPolicyPara;
            memset(&TSPolicyPara, 0, sizeof(TSPolicyPara));
            TSPolicyPara.cbSize = sizeof(TSPolicyPara);
            TSPolicyPara.pvExtraPolicyPara = (void *) &TSExtraPolicyPara;

            CERT_CHAIN_POLICY_STATUS TSPolicyStatus;
            memset(&TSPolicyStatus, 0, sizeof(TSPolicyStatus));
            TSPolicyStatus.cbSize = sizeof(TSPolicyStatus);


            //
            //  check counter signers
            //
            for (DWORD i2 = 0; i2 < pSgnr->csCounterSigners; i2++)
            {
                PCRYPT_PROVIDER_SGNR pCounterSgnr =
                    WTHelperGetProvSignerFromChain(pProvData, i1, TRUE, i2);

                //
                //  do we care about this counter signer?
                //
                if (pCounterSgnr->dwSignerType != SGNR_TYPE_TIMESTAMP)
                    continue;

                if (!CertVerifyCertificateChainPolicy(
                        CERT_CHAIN_POLICY_AUTHENTICODE_TS,
                        pCounterSgnr->pChainContext,
                        &TSPolicyPara,
                        &TSPolicyStatus
                        )) {
                    dwError = TRUST_E_SYSTEM_ERROR;
                    goto CommonReturn;
                } else if (0 != TSPolicyStatus.dwError) {
                    // On April 13, 1999 changed to map all time stamp errors
                    // to TRUST_E_TIME_STAMP
                    dwError = TRUST_E_TIME_STAMP;
//                    dwError = TSPolicyStatus.dwError;
                    UpdateCertError(pCounterSgnr, &TSPolicyStatus);
                    goto CommonReturn;
                } else if (0 < pCounterSgnr->csCertChain) {
                    PCRYPT_PROVIDER_CERT pProvCert;

                    pProvCert = WTHelperGetProvCertFromChain(pCounterSgnr, 0);
                    if (CERT_E_REVOCATION_FAILURE == pProvCert->dwError) {
                        // Policy says to ignore offline revocation errors
                        pProvCert->dwError = 0;
                        pProvCert->dwRevokedReason = 0;
                    }
                }
            }
        }
    }
    
    dwError = 0;
CommonReturn:
    pProvData->dwFinalError = dwError;
    return SoftpubCallUI(pProvData, dwError, TRUE);
}


HRESULT WINAPI IE4TrustSoftpubAuthenticode(CRYPT_PROVIDER_DATA *pProvData)
{
    DWORD   dwError;

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_SGNR     *pCounterSgnr;
    CRYPT_PROVIDER_CERT     *pRootCert;

    dwError = ERROR_SUCCESS;

    //
    // check the high level error codes.
    //
    if ((dwError = checkGetErrorBasedOnStepErrors(pProvData)) != ERROR_SUCCESS)
    {
        return(SoftpubCallUI(pProvData, dwError, TRUE));
    }

    //
    //  check each message
    //
    dwError         = 0;

    for (int i1 = 0; i1 < (int)pProvData->csSigners; i1++)
    {
        pSgnr       = WTHelperGetProvSignerFromChain(pProvData, i1, FALSE, 0);
        pRootCert   = WTHelperGetProvCertFromChain(pSgnr, pSgnr->csCertChain - 1);

        //
        //  cert chain of signer
        //
        if (!(pRootCert))
        {
            //
            //  should get caught above in checkGetErrorBasedOnStepErrors!
            //
            return(SoftpubCallUI(pProvData, CERT_E_CHAINING, TRUE));
        }

        if (!(checkIsTrustedRoot(pRootCert)))
        {
            if (pRootCert->fTestCert)
            {
                return(SoftpubCallUI(pProvData, CERT_E_UNTRUSTEDTESTROOT, TRUE));
            }

            if (pRootCert->fSelfSigned)
            {
                return(SoftpubCallUI(pProvData, CERT_E_UNTRUSTEDROOT, TRUE));
            }

            return(SoftpubCallUI(pProvData, CERT_E_CHAINING, TRUE));
        }

        if (!(checkCertificateChain(pProvData, pSgnr, &dwError)))
        {
            return(SoftpubCallUI(pProvData, dwError, TRUE));
        }

        //
        //  check counter signers
        //
        for (int i2 = 0; i2 < (int)pSgnr->csCounterSigners; i2++)
        {
            pCounterSgnr    = WTHelperGetProvSignerFromChain(pProvData, i1, TRUE, i2);
            pRootCert       = WTHelperGetProvCertFromChain(pCounterSgnr, pCounterSgnr->csCertChain - 1);

            //
            //  do we care about this counter signer?
            //
            if (!(pCounterSgnr->dwSignerType & SGNR_TYPE_TIMESTAMP))
            {
                continue;
            }

            //
            //  cert chain of signer
            //
            if (pCounterSgnr->csCertChain < 1)
            {
                return(SoftpubCallUI(pProvData, CERT_E_CHAINING, TRUE));
            }

            if (!(checkIsTrustedRoot(pRootCert)))
            {
                if (pRootCert->fTestCert)
                {
                    return(SoftpubCallUI(pProvData, CERT_E_UNTRUSTEDTESTROOT, TRUE));
                }
                
                if (pRootCert->fSelfSigned)
                {
                    return(SoftpubCallUI(pProvData, CERT_E_UNTRUSTEDROOT, TRUE));
                }
                
                return(SoftpubCallUI(pProvData, CERT_E_CHAINING, TRUE));
            }

            if (pCounterSgnr->dwSignerType & SGNR_TYPE_TIMESTAMP)
            {
                if (!(checkTimeStampCertificateChain(pProvData, pCounterSgnr, &dwError)))
                {
                    return(SoftpubCallUI(pProvData, dwError, TRUE));
                }
            }
            else
            {
                if (!(checkCertificateChain(pProvData, pCounterSgnr, &dwError)))
                {
                    return(SoftpubCallUI(pProvData, dwError, TRUE));
                }
            }
        }
    }
    

    return(SoftpubCallUI(pProvData, dwError, TRUE));
}

