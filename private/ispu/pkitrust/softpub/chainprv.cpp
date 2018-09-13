//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chainprv.cpp
//
//  Contents:   Microsoft Internet Security Generic Chain Policy Provider
//
//  Functions:  GenericChainRegisterServer
//              GenericChainUnregisterServer
//              GenericChainCertificateTrust
//              GenericChainFinalProv
//
//  History:    21-Feb-1998 philh      created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


//////////////////////////////////////////////////////////////////////////////
//
// GenericChainRegisterServer
//----------------------------------------------------------------------------
//  Register the GenericChain provider
//  

STDAPI GenericChainRegisterServer(void)
{
    GUID gGenericChainProv = WINTRUST_ACTION_GENERIC_CHAIN_VERIFY;
    BOOL fRet;
    CRYPT_REGISTER_ACTIONID sRegAID;

    fRet = TRUE;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    // Authenticode initialization provider
    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = SP_INIT_FUNCTION;

    // Authenticode object provider
    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    // Authenticode signature provider
    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    //------------------------------------------------------------------------
    // Our Generic Chain certificate provider (builds the chain)
    //+-----------------------------------------------------------------------
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = GENERIC_CHAIN_CERTTRUST_FUNCTION;

    // authenticode cert policy
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    //------------------------------------------------------------------------
    // Our Generic Chain final provider (chain policy callback)
    //+-----------------------------------------------------------------------
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = GENERIC_CHAIN_FINALPOLICY_FUNCTION;

    // Authenticode cleanup -- we don't store any data.
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = SP_CLEANUPPOLICY_FUNCTION;

    fRet &= WintrustAddActionID(&gGenericChainProv, 0, &sRegAID);
    return((fRet) ? S_OK : S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//----------------------------------------------------------------------------
//  unregisters GenericChain provider
//  

STDAPI GenericChainUnregisterServer(void)
{
    GUID gGenericChainProv = WINTRUST_ACTION_GENERIC_CHAIN_VERIFY;

    WintrustRemoveActionID(&gGenericChainProv);
    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
//
// GenericChainCertificateTrust
//----------------------------------------------------------------------------
//  Creates the chains for the signers and counter signers
//  

void GenericChainWalkSigner(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN OUT PCRYPT_PROVIDER_SGNR pSgnr,
    IN PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO pChainInfo
    );

HRESULT
WINAPI
GenericChainCertificateTrust(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    HRESULT hr;
    WTD_GENERIC_CHAIN_POLICY_DATA DefaultPolicyData;
    PWTD_GENERIC_CHAIN_POLICY_DATA pPolicyData;

    if (_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct,
            fRecallWithState) && pProvData->fRecallWithState == TRUE)
        return S_OK;

    pPolicyData = (PWTD_GENERIC_CHAIN_POLICY_DATA)
        pProvData->pWintrustData->pPolicyCallbackData;

    if (NULL == pPolicyData) {
        memset(&DefaultPolicyData, 0, sizeof(DefaultPolicyData));
        DefaultPolicyData.cbSize = sizeof(DefaultPolicyData);
        pPolicyData = &DefaultPolicyData;
    }

    if (!_ISINSTRUCT(WTD_GENERIC_CHAIN_POLICY_DATA,
            pPolicyData->cbSize, pvPolicyArg) ||
        !_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwProvFlags) ||
            (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG)) {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = 
            ERROR_INVALID_PARAMETER;
        return S_FALSE;
    }

    if (pProvData->csSigners < 1) {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
            TRUST_E_NOSIGNATURE;
        return S_FALSE;
    }

    pProvData->dwProvFlags |= CPD_USE_NT5_CHAIN_FLAG;
    hr = S_OK;
    //
    //  loop through all signers
    //
    for (DWORD i = 0; i < pProvData->csSigners; i++) {
        PCRYPT_PROVIDER_SGNR pSgnr;

        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i, FALSE, 0);
        if (pSgnr->csCertChain < 1) {
            pSgnr->dwError = TRUST_E_NO_SIGNER_CERT;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
                TRUST_E_NO_SIGNER_CERT;
            hr = S_FALSE;
            continue;
        }

        GenericChainWalkSigner(
            pProvData,
            pSgnr,
            pPolicyData->pSignerChainInfo
            );

        //
        //  loop through all counter signers
        //
        for (DWORD j = 0; j < pSgnr->csCounterSigners; j++) {
            PCRYPT_PROVIDER_SGNR pCounterSgnr;

            pCounterSgnr = WTHelperGetProvSignerFromChain(
                pProvData, i, TRUE, j);
            if (pCounterSgnr->csCertChain < 1) {
                pCounterSgnr->dwError = TRUST_E_NO_SIGNER_CERT;
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
                     TRUST_E_COUNTER_SIGNER;
                hr = S_FALSE;
                continue;
            }

            GenericChainWalkSigner(
                pProvData,
                pCounterSgnr,
                pPolicyData->pCounterSignerChainInfo
                );
        }
    }

    return hr;
}


HCERTSTORE GenericChainGetAdditionalStore(
    IN CRYPT_PROVIDER_DATA *pProvData
    )
{
    if (0 == pProvData->chStores)
        return NULL;

    if (1 < pProvData->chStores) {
        HCERTSTORE hCollectionStore;

        if (hCollectionStore = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )) {
            DWORD i;
            for (i = 0; i < pProvData->chStores; i++)
                CertAddStoreToCollection(
                    hCollectionStore,
                    pProvData->pahStores[i],
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
        }
        return hCollectionStore;
    } else
        return CertDuplicateStore(pProvData->pahStores[0]);
}

void GenericChainWalkSigner(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN OUT PCRYPT_PROVIDER_SGNR pSgnr,
    IN PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO pChainInfo
    )
{
    DWORD dwSgnrError;
    WTD_GENERIC_CHAIN_POLICY_CREATE_INFO DefaultChainInfo;
    CERT_CHAIN_PARA ChainPara;
    HCERTSTORE hAdditionalStore = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext;

    if (pChainInfo) {
        if (!_ISINSTRUCT(WTD_GENERIC_CHAIN_POLICY_CREATE_INFO,
                pChainInfo->cbSize, pvReserved)) {
            dwSgnrError = ERROR_INVALID_PARAMETER;
            goto InvalidParameter;
        }
    } else {
        memset(&ChainPara, 0, sizeof(ChainPara));
        ChainPara.cbSize = sizeof(ChainPara);

        memset(&DefaultChainInfo, 0, sizeof(DefaultChainInfo));
        DefaultChainInfo.cbSize =  sizeof(DefaultChainInfo);
        DefaultChainInfo.pChainPara = &ChainPara;
        pChainInfo = &DefaultChainInfo;
    }

    hAdditionalStore = GenericChainGetAdditionalStore(pProvData);

    if (!CertGetCertificateChain (
            pChainInfo->hChainEngine,
            WTHelperGetProvCertFromChain(pSgnr, 0)->pCert,
            &pSgnr->sftVerifyAsOf,
            hAdditionalStore,
            pChainInfo->pChainPara,
            pChainInfo->dwFlags,
            pChainInfo->pvReserved,
            &pChainContext
            )) {
        pProvData->dwError = GetLastError();
        dwSgnrError = TRUST_E_SYSTEM_ERROR;
        goto GetChainError;
    }

    pSgnr->pChainContext = pChainContext;

CommonReturn:
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);
    return;

ErrorReturn:
    pSgnr->dwError = dwSgnrError;
    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
        dwSgnrError;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS, InvalidParameter)
TRACE_ERROR_EX(DBG_SS, GetChainError)
}


//////////////////////////////////////////////////////////////////////////////
//
// Final Policy Provider function: GenericChainFinalProv
//----------------------------------------------------------------------------
//  Check the outcome of the previous functions and display UI based on this.
//  


DWORD GenericChainGetErrorBasedOnStepErrors(
    IN PCRYPT_PROVIDER_DATA pProvData
    );

HRESULT
WINAPI
GenericChainDefaultPolicyCallback(
    IN PCRYPT_PROVIDER_DATA pProvData,
    IN DWORD dwStepError,
    IN DWORD dwRegPolicySettings,
    IN DWORD cSigner,
    IN PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *rgpSigner,
    IN void *pvPolicyArg
    );

HRESULT
WINAPI
GenericChainFinalProv(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    HRESULT hr;

    WTD_GENERIC_CHAIN_POLICY_DATA DefaultPolicyData;
    PWTD_GENERIC_CHAIN_POLICY_DATA pPolicyData;
    DWORD dwStepError;
    DWORD cSigner = 0;
    DWORD i;
    PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *ppSignerInfo = NULL;
    PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pSignerInfo;      // not allocated
    PFN_WTD_GENERIC_CHAIN_POLICY_CALLBACK pfnPolicyCallback;

    pPolicyData = (PWTD_GENERIC_CHAIN_POLICY_DATA)
        pProvData->pWintrustData->pPolicyCallbackData;
    if (NULL == pPolicyData) {
        memset(&DefaultPolicyData, 0, sizeof(DefaultPolicyData));
        DefaultPolicyData.cbSize = sizeof(DefaultPolicyData);
        pPolicyData = &DefaultPolicyData;
    }

    if (!_ISINSTRUCT(WTD_GENERIC_CHAIN_POLICY_DATA,
            pPolicyData->cbSize, pvPolicyArg) ||
        !_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwFinalError) ||
            (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG) ||
            0 == (pProvData->dwProvFlags & CPD_USE_NT5_CHAIN_FLAG))
        goto InvalidParameter;

    dwStepError = GenericChainGetErrorBasedOnStepErrors(pProvData);

    cSigner = pProvData->csSigners;
    if (0 == cSigner)
        goto NoSignature;

    if (NULL == (ppSignerInfo =
        (PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *) malloc(
            sizeof(PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cSigner +
            sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cSigner)))
        goto OutOfMemory;
    memset(ppSignerInfo, 0,
            sizeof(PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cSigner +
            sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cSigner);

    //
    //  Update allocated info for each signer
    //
    pSignerInfo = (PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO)
         &ppSignerInfo[cSigner];
    i = 0;
    for ( ; i < cSigner; i++, pSignerInfo++) {
        CRYPT_PROVIDER_SGNR *pSgnr;
        DWORD cCounterSigner;

        ppSignerInfo[i] = pSignerInfo;
        pSignerInfo->cbSize = sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO);

        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i, FALSE, 0);
        pSignerInfo->pChainContext = pSgnr->pChainContext;
        pSignerInfo->dwSignerType = pSgnr->dwSignerType;
        pSignerInfo->pMsgSignerInfo = pSgnr->psSigner;
        pSignerInfo->dwError = pSgnr->dwError;

        cCounterSigner = pSgnr->csCounterSigners;
        if (cCounterSigner) {
            DWORD j;
            PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *ppCounterSignerInfo;
            PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pCounterSignerInfo;

            if (NULL == (ppCounterSignerInfo =
                (PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *) malloc(
                    sizeof(PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) *
                        cCounterSigner +
                    sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) *
                        cCounterSigner)))
                goto OutOfMemory;
            memset(ppCounterSignerInfo, 0,
                sizeof(PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cCounterSigner +
                sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO) * cCounterSigner);
            pSignerInfo->cCounterSigner = cCounterSigner;
            pSignerInfo->rgpCounterSigner = ppCounterSignerInfo;

            //
            //  Update allocated info for each counter signer
            //
            pCounterSignerInfo = (PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO)
                &ppCounterSignerInfo[cCounterSigner];
            j = 0;
            for ( ; j < cCounterSigner; j++, pCounterSignerInfo++) {
                PCRYPT_PROVIDER_SGNR pCounterSgnr;

                ppCounterSignerInfo[j] = pCounterSignerInfo;
                pCounterSignerInfo->cbSize =
                    sizeof(WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO);

                pCounterSgnr = WTHelperGetProvSignerFromChain(pProvData, i,
                    TRUE, j);
                pCounterSignerInfo->pChainContext = pCounterSgnr->pChainContext;
                pCounterSignerInfo->dwSignerType = pCounterSgnr->dwSignerType;
                pCounterSignerInfo->pMsgSignerInfo = pCounterSgnr->psSigner;
                pCounterSignerInfo->dwError = pCounterSgnr->dwError;
            }
        }
    }

    if (pPolicyData->pfnPolicyCallback)
        pfnPolicyCallback = pPolicyData->pfnPolicyCallback;
    else
        pfnPolicyCallback = GenericChainDefaultPolicyCallback;

    hr = pfnPolicyCallback(
        pProvData,
        dwStepError,
        pProvData->dwRegPolicySettings,
        cSigner,
        ppSignerInfo,
        pPolicyData->pvPolicyArg
        );

CommonReturn:
    if (ppSignerInfo) {
        while (cSigner--) {
            pSignerInfo = ppSignerInfo[cSigner];
            if (pSignerInfo && pSignerInfo->rgpCounterSigner)
                free(pSignerInfo->rgpCounterSigner);
        }
        free(ppSignerInfo);
    }
    pProvData->dwFinalError = (DWORD) hr;
    return hr;

ErrorReturn:
    hr = (HRESULT) GetLastError();
    if (0 == hr)
        hr = E_UNEXPECTED;
    goto CommonReturn;

SET_ERROR_EX(DBG_SS, InvalidParameter, ERROR_INVALID_PARAMETER)
SET_ERROR_EX(DBG_SS, OutOfMemory, ERROR_NOT_ENOUGH_MEMORY)
SET_ERROR_EX(DBG_SS, NoSignature, TRUST_E_NOSIGNATURE);
}

DWORD GenericChainGetErrorBasedOnStepErrors(
    IN PCRYPT_PROVIDER_DATA pProvData
    )
{
    //
    //  initial allocation of the step errors?
    //
    if (NULL == pProvData->padwTrustStepErrors)
        return ERROR_NOT_ENOUGH_MEMORY;

    // problem with file
    if ((pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FILEIO] != 0) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_CATALOGFILE] != 0))
    {
        return(CRYPT_E_FILE_ERROR);
    }

    //
    //  did we fail in one of the last steps?
    //
    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    }

    return(ERROR_SUCCESS);
}

HRESULT
WINAPI
GenericChainDefaultPolicyCallback(
    IN PCRYPT_PROVIDER_DATA pProvData,
    IN DWORD dwStepError,
    IN DWORD dwRegPolicySettings,
    IN DWORD cSigner,
    IN PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *rgpSigner,
    IN void *pvPolicyArg
    )
{
    HRESULT hr;
    DWORD i;

    AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA ExtraPolicyPara;
    memset(&ExtraPolicyPara, 0, sizeof(ExtraPolicyPara));
    ExtraPolicyPara.cbSize = sizeof(ExtraPolicyPara);
    ExtraPolicyPara.dwRegPolicySettings = dwRegPolicySettings;

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
    if (0 != dwStepError) {
        hr = (HRESULT) dwStepError;
        goto CommonReturn;
    }


    //
    //  check each signer
    //
    for (i = 0; i < cSigner; i++) {
        PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pSigner = rgpSigner[i];
        ExtraPolicyPara.pSignerInfo = pSigner->pMsgSignerInfo;

        if (!CertVerifyCertificateChainPolicy(
                CERT_CHAIN_POLICY_AUTHENTICODE,
                pSigner->pChainContext,
                &PolicyPara,
                &PolicyStatus
                )) {
            hr = TRUST_E_SYSTEM_ERROR;
            goto CommonReturn;
        }

        if (0 != PolicyStatus.dwError) {
            hr = (HRESULT) PolicyStatus.dwError;
            goto CommonReturn;
        }

        if (pSigner->cCounterSigner) {
            AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA TSExtraPolicyPara;
            memset(&TSExtraPolicyPara, 0, sizeof(TSExtraPolicyPara));
            TSExtraPolicyPara.cbSize = sizeof(TSExtraPolicyPara);
            TSExtraPolicyPara.dwRegPolicySettings = dwRegPolicySettings;
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
            for (DWORD j = 0; j < pSigner->cCounterSigner; j++) {
                PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pCounterSigner =
                    pSigner->rgpCounterSigner[j];

                //
                //  do we care about this counter signer?
                //
                if (pCounterSigner->dwSignerType != SGNR_TYPE_TIMESTAMP)
                    continue;

                if (!CertVerifyCertificateChainPolicy(
                        CERT_CHAIN_POLICY_AUTHENTICODE_TS,
                        pCounterSigner->pChainContext,
                        &TSPolicyPara,
                        &TSPolicyStatus
                        )) {
                    hr = TRUST_E_SYSTEM_ERROR;
                    goto CommonReturn;
                } else if (0 != TSPolicyStatus.dwError) {
                    hr = (HRESULT) TSPolicyStatus.dwError;
                    goto CommonReturn;
                }
            }
        }
    }
    
    hr = S_OK;
CommonReturn:
    return hr;
}
