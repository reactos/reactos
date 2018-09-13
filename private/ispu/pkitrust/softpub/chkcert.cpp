//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chkcert.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubCheckCert
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


BOOL WINAPI SoftpubCheckCert(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, 
                             BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;
    PCCERT_CONTEXT          pCertContext;

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, idxSigner, fCounterSignerChain, idxCounterSigner);

    if (_ISINSTRUCT(CRYPT_PROVIDER_SGNR, pSgnr->cbStruct, pChainContext) &&
            pSgnr->pChainContext &&
        _ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct,
                dwProvFlags) &&
            0 == (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG)) {
        pProvData->dwProvFlags |= CPD_USE_NT5_CHAIN_FLAG;
        return TRUE;
    }


    pCert = WTHelperGetProvCertFromChain(pSgnr, pSgnr->csCertChain - 1);

    //
    //  only self signed certificates in the root store are "trusted" roots
    //
    if (pCert->fSelfSigned)
    {
        pCertContext = pCert->pCert;

        if (pCertContext)
        {
            //
            //  check to see if it is a test root...
            //
            if (pCert->fTestCert)
            {
                if (pProvData->dwRegPolicySettings & WTPF_TRUSTTEST)
                {
                    pCert->fTrustedRoot = TRUE;
                }
            }

            if (pCert->fTrustedRoot)
            {
                return(FALSE);
            }

            if (pProvData->chStores > 0)
            {
                PCCERT_CONTEXT  pCTLSignerContext;

                //
                //  check the Certificate Trust List for code signing
                //
                pCTLSignerContext   = NULL;

                if (IsInTrustList(pProvData, pCertContext, &pCTLSignerContext, pProvData->pszUsageOID))
                {
                    if (pCTLSignerContext)
                    {
                        pProvData->psPfns->pfnAddCert2Chain(pProvData, 
                                                          idxSigner, 
                                                          fCounterSignerChain,
                                                          idxCounterSigner, 
                                                          pCTLSignerContext);

                        CertFreeCertificateContext(pCTLSignerContext);

                        // change pCert back to the previous cert
                        pCert = WTHelperGetProvCertFromChain(pSgnr, pSgnr->csCertChain - 2);

                        // set the confidence on the child cert
                        pCert->dwConfidence = CERT_CONFIDENCE_HIGHEST;

                        // change the cert to the CTL signer's cert
                        pCert = WTHelperGetProvCertFromChain(pSgnr, pSgnr->csCertChain - 1);

                        // set the confidence on the ctl signer's cert
                        pCert->dwConfidence = CERT_CONFIDENCE_HIGHEST;

                        // let everyone know we are a ctl signer..
                        pCert->fTrustListSignerCert = TRUE;

                        if (WTHelperIsInRootStore(pProvData, pCert->pCert))
                        {
                            // make it known that it is trusted.
                            pCert->fTrustedRoot = TRUE;

                            return(FALSE);
                        }

                        return(TRUE);

                    }

                    return(FALSE);
                }
            }
        }

        return(FALSE);
    }

    return(TRUE);
}


BOOL IsInTrustList(CRYPT_PROVIDER_DATA *pProvData, PCCERT_CONTEXT pCertContext, PCCERT_CONTEXT *ppCTLSigner,
                   LPSTR pszUsage)
{
    CTL_USAGE               sUsage;
    CTL_VERIFY_USAGE_PARA   sVerify;
    CTL_VERIFY_USAGE_STATUS sStatus;
    HCERTSTORE              *pahCtlStores;
    PCCERT_CONTEXT          pCTLSignerContext;

    if (pProvData->chStores < 2)
    {
        return(FALSE);
    }

    pCTLSignerContext   = NULL;
    *ppCTLSigner        = NULL;

    pahCtlStores        = &pProvData->pahStores[1];

    memset(&sUsage, 0x00, sizeof(CTL_USAGE));
    sUsage.cUsageIdentifier     = 1;
    sUsage.rgpszUsageIdentifier = &pszUsage;

    memset(&sVerify, 0x00, sizeof(CTL_VERIFY_USAGE_PARA));
    sVerify.cbSize              = sizeof(CTL_VERIFY_USAGE_PARA);
    sVerify.cCtlStore           = pProvData->chStores - 1;
    sVerify.rghCtlStore         = pahCtlStores;
    sVerify.cSignerStore        = pProvData->chStores;
    sVerify.rghSignerStore      = &pProvData->pahStores[0];

    memset(&sStatus, 0x00, sizeof(CTL_VERIFY_USAGE_STATUS));
    sStatus.cbSize              = sizeof(CTL_VERIFY_USAGE_STATUS);
    sStatus.ppSigner            = &pCTLSignerContext;

    if (CertVerifyCTLUsage(CRYPT_ASN_ENCODING,
                           CTL_CERT_SUBJECT_TYPE,
                           (void *)pCertContext,
                           &sUsage,
			   CERT_VERIFY_ALLOW_MORE_USAGE_FLAG,
                           &sVerify,
                           &sStatus))
    {
        *ppCTLSigner    = pCTLSignerContext;

        return(TRUE);
    }

    return(FALSE);
}
