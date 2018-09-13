//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       winvtrst.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  WinVerifyTrustEx
//              WinVerifyTrust
//
//              *** local functions ***
//              _FillProviderData
//              _CleanupProviderData
//              _CleanupProviderNonStateData
//              _WVTSipFreeSubjectInfo
//              _WVTSipFreeSubjectInfoKeepState
//              _WVTSetupProviderData
//
//  History:    31-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "wvtver1.h"

BOOL    _FillProviderData(CRYPT_PROVIDER_DATA *pProvData, HWND hWnd, WINTRUST_DATA *pWinTrustData);
void    _CleanupProviderData(CRYPT_PROVIDER_DATA *pProvData);
void    _CleanupProviderNonStateData(CRYPT_PROVIDER_DATA *ProvData);

BOOL    _WVTSipFreeSubjectInfo(SIP_SUBJECTINFO *pSubj);
BOOL    _WVTSetupProviderData(CRYPT_PROVIDER_DATA *psProvData,
                             CRYPT_PROVIDER_DATA *psStateProvData);
BOOL    _WVTSipFreeSubjectInfoKeepState(SIP_SUBJECTINFO *pSubj);

VOID FreeWintrustStateData (WINTRUST_DATA* pWintrustData);

extern CCatalogCache g_CatalogCache;
//////////////////////////////////////////////////////////////////////////////////////
//
// WinVerifyTrustEx
//
//
extern "C" HRESULT WINAPI WinVerifyTrustEx(HWND hWnd, GUID *pgActionID, WINTRUST_DATA *pWinTrustData)
{
    return((HRESULT)WinVerifyTrust(hWnd, pgActionID, pWinTrustData));
}

extern "C" LONG WINAPI WinVerifyTrust(HWND hWnd, GUID *pgActionID, LPVOID pOld)
{
    WINTRUST_DATA           *pWinTrustData;
    CRYPT_PROVIDER_DATA     sProvData;
    CRYPT_PROVIDER_DATA     *pStateProvData;
    HRESULT                 hr;
    BOOL                    fVersion1;
    BOOL                    fCacheableCall;
    PCATALOG_CACHED_STATE   pCachedState = NULL;
    BOOL                    fVersion1WVTCalled = FALSE;

    pWinTrustData           = (WINTRUST_DATA *)pOld;
    hr                      = TRUST_E_PROVIDER_UNKNOWN;
    pStateProvData          = NULL;

    fCacheableCall = g_CatalogCache.IsCacheableWintrustCall( pWinTrustData );

    if ( fCacheableCall == TRUE )
    {
        g_CatalogCache.LockCache();

        if ( pWinTrustData->dwStateAction == WTD_STATEACTION_AUTO_CACHE_FLUSH )
        {
            g_CatalogCache.FlushCache();
            g_CatalogCache.UnlockCache();

            return( ERROR_SUCCESS );
        }

        pCachedState = g_CatalogCache.FindCachedState( pWinTrustData );

        g_CatalogCache.AdjustWintrustDataToCachedState(
                             pWinTrustData,
                             pCachedState,
                             FALSE
                             );
    }

    if (WintrustIsVersion1ActionID(pgActionID))
    {
        fVersion1 = TRUE;
    }
    else
    {
        fVersion1 = FALSE;

        if (_ISINSTRUCT(WINTRUST_DATA, pWinTrustData->cbStruct, hWVTStateData))
        {
            if ((pWinTrustData->dwStateAction == WTD_STATEACTION_VERIFY) ||
                (pWinTrustData->dwStateAction == WTD_STATEACTION_CLOSE))
            {
                pStateProvData = WTHelperProvDataFromStateData(pWinTrustData->hWVTStateData);

                if (pWinTrustData->dwStateAction == WTD_STATEACTION_CLOSE)
                {
                    if (pWinTrustData->hWVTStateData)
                    {
                        _CleanupProviderData(pStateProvData);
                        DELETE_OBJECT(pWinTrustData->hWVTStateData);
                    }

                    assert( fCacheableCall == FALSE );

                    return(ERROR_SUCCESS);
                }
            }
        }
    }

    if (_WVTSetupProviderData(&sProvData, pStateProvData))
    {
        sProvData.pgActionID  = pgActionID;

        if (!(pStateProvData))
        {
            if (!(WintrustLoadFunctionPointers(pgActionID, sProvData.psPfns)))
            {
                //
                //  it may be that we are looking for a version 1 trust provider.
                //
                hr = Version1_WinVerifyTrust(hWnd, pgActionID, pWinTrustData);
                fVersion1WVTCalled = TRUE;
            }

            if ( fVersion1WVTCalled == FALSE )
            {
                if (fVersion1)
                {
                    //
                    //  backwards compatibility with IE3.x and previous
                    //
                    WINTRUST_DATA       sWinTrustData;
                    WINTRUST_FILE_INFO  sWinTrustFileInfo;

                    pWinTrustData   = ConvertDataFromVersion1(hWnd, pgActionID, &sWinTrustData, &sWinTrustFileInfo,
                                                              pWinTrustData);
                }

                _FillProviderData(&sProvData, hWnd, pWinTrustData);
            }
        }

        if ( fVersion1WVTCalled == FALSE )
        {
            if (sProvData.psPfns->pfnInitialize)
            {
                (*sProvData.psPfns->pfnInitialize)(&sProvData);
            }

            if (sProvData.psPfns->pfnObjectTrust)
            {
                (*sProvData.psPfns->pfnObjectTrust)(&sProvData);
            }

            if (sProvData.psPfns->pfnSignatureTrust)
            {
                (*sProvData.psPfns->pfnSignatureTrust)(&sProvData);
            }

            if (sProvData.psPfns->pfnCertificateTrust)
            {
                (*sProvData.psPfns->pfnCertificateTrust)(&sProvData);
            }

            if (sProvData.psPfns->pfnFinalPolicy)
            {
                hr = (*sProvData.psPfns->pfnFinalPolicy)(&sProvData);
            }

            if (sProvData.psPfns->pfnTestFinalPolicy)
            {
                (*sProvData.psPfns->pfnTestFinalPolicy)(&sProvData);
            }

            if (sProvData.psPfns->pfnCleanupPolicy)
            {
                (*sProvData.psPfns->pfnCleanupPolicy)(&sProvData);
            }

            if (!(pStateProvData))
            {
                //
                //  no previous state saved
                //
                if ((_ISINSTRUCT(WINTRUST_DATA, pWinTrustData->cbStruct, hWVTStateData)) &&
                    (pWinTrustData->dwStateAction == WTD_STATEACTION_VERIFY))
                {
                    //
                    //  first time call and asking to maintain state...
                    //
                    if (!(pWinTrustData->hWVTStateData = (HANDLE)WVTNew(sizeof(CRYPT_PROVIDER_DATA))))
                    {
                        _CleanupProviderData(&sProvData);
                        hr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        _CleanupProviderNonStateData(&sProvData);

                        memcpy(pWinTrustData->hWVTStateData, &sProvData, sizeof(CRYPT_PROVIDER_DATA));
                    }
                }
                else
                {
                    _CleanupProviderData(&sProvData);
                }
            }
            else
            {
                //
                //  only free up memory specific to this object/member
                //
                _CleanupProviderNonStateData(&sProvData);
                memcpy(pWinTrustData->hWVTStateData, &sProvData, sizeof(CRYPT_PROVIDER_DATA));
            }

            //
            //  in version 1, when called by IE3.x and earlier, if security level is HIGH,
            //  then the no bad UI is set.  If we had an error, we want to
            //  set the error to TRUST_E_FAIL.  If we do not trust the object, every other
            //  case sets it to TRUST_E_SUBJECT_NOT_TRUSTED and IE throws NO UI....
            //
            if (fVersion1)
            {
                if (hr != ERROR_SUCCESS)
                {
                    if ((pWinTrustData) &&
                        (_ISINSTRUCT(WINTRUST_DATA, pWinTrustData->cbStruct, dwUIChoice)))
                    {
                        if (pWinTrustData->dwUIChoice == WTD_UI_NOBAD)
                        {
                            hr = TRUST_E_FAIL;  // ie throws UI.
                        }
                        else
                        {
                            hr = TRUST_E_SUBJECT_NOT_TRUSTED; // ie throws no UI.
                        }
                    }
                    else
                    {
                        hr = TRUST_E_SUBJECT_NOT_TRUSTED; // ie throws no UI.
                    }
                }
            }
        }
    }
    else
    {
        hr = TRUST_E_SYSTEM_ERROR;
    }

    if ( fCacheableCall == TRUE )
    {
        if ( pCachedState == NULL )
        {
            if ( g_CatalogCache.CreateCachedStateFromWintrustData(
                                       pWinTrustData,
                                       &pCachedState
                                       ) == TRUE )
            {
                g_CatalogCache.AddCachedState( pCachedState );
            }
        }

        if ( pCachedState == NULL )
        {
            FreeWintrustStateData( pWinTrustData );
        }

        g_CatalogCache.AdjustWintrustDataToCachedState(
                             pWinTrustData,
                             pCachedState,
                             TRUE
                             );

        g_CatalogCache.ReleaseCachedState( pCachedState );

        g_CatalogCache.UnlockCache();
    }

    return(hr);
}

//////////////////////////////////////////////////////////////////////////////////////
//
//  local utility functions
//
//
BOOL _FillProviderData(CRYPT_PROVIDER_DATA *pProvData, HWND hWnd, WINTRUST_DATA *pWinTrustData)
{
    //
    //  remember:  we do NOT want to return FALSE unless it is an absolutely
    //              catastrophic error!  Let the Trust provider handle (eg: none!)
    //

    if ((hWnd == INVALID_HANDLE_VALUE) || !(hWnd))
    {
        if (pWinTrustData->dwUIChoice != WTD_UI_NONE)
        {
            hWnd = GetDesktopWindow();
        }
    }
    pProvData->hWndParent       = hWnd;
    pProvData->hProv            = I_CryptGetDefaultCryptProv(0);  // get the default and DONT RELEASE IT!!!!
    pProvData->dwEncoding       = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    pProvData->pWintrustData    = pWinTrustData;
    pProvData->dwError          = ERROR_SUCCESS;

    // allocate errors
    if (!(pProvData->padwTrustStepErrors))
    {
        if (!(pProvData->padwTrustStepErrors = (DWORD *)WVTNew(TRUSTERROR_MAX_STEPS * sizeof(DWORD))))
        {
            pProvData->dwError = GetLastError();
            return(TRUE);
        }

        pProvData->cdwTrustStepErrors = TRUSTERROR_MAX_STEPS;
    }

    memset(pProvData->padwTrustStepErrors, 0x00, sizeof(DWORD) * TRUSTERROR_MAX_STEPS);

    WintrustGetRegPolicyFlags(&pProvData->dwRegPolicySettings);
    GetRegSecuritySettings(&pProvData->dwRegSecuritySettings);

    if (!(pWinTrustData) ||
        !(_ISINSTRUCT(WINTRUST_DATA, pWinTrustData->cbStruct, dwUIChoice)))

    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] = (DWORD)ERROR_INVALID_PARAMETER;
    }

    if (pWinTrustData && _ISINSTRUCT(WINTRUST_DATA,
            pWinTrustData->cbStruct, dwProvFlags))
        pProvData->dwProvFlags = pWinTrustData->dwProvFlags &
            WTD_PROV_FLAGS_MASK;

#if DBG
    {
        char *pszEnvVar;
        if (pszEnvVar = getenv("DEBUG_TRUST_PROV_FLAGS")) {
            long lDebugProvFlags = atol(pszEnvVar);
            pProvData->dwProvFlags |= (DWORD) lDebugProvFlags &
                WTD_PROV_FLAGS_MASK;
        }
    }
#endif

    return(TRUE);
}

void _CleanupProviderData(CRYPT_PROVIDER_DATA *pProvData)
{
    // pProvData->hProv: we're using crypt32's default

    // pProvData->pWintrustData->xxx->hFile
    if ((pProvData->fOpenedFile) && (pProvData->pWintrustData != NULL))
    {
        HANDLE  *phFile;

        phFile  = NULL;

        switch (pProvData->pWintrustData->dwUnionChoice)
        {
            case WTD_CHOICE_FILE:
                phFile = &pProvData->pWintrustData->pFile->hFile;
                break;

            case WTD_CHOICE_CATALOG:
                phFile = &pProvData->pWintrustData->pCatalog->hMemberFile;
                break;
        }

        if ((phFile) && (*phFile) && (*phFile != INVALID_HANDLE_VALUE))
        {
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
            pProvData->fOpenedFile = FALSE;
        }
    }

    if (pProvData->dwSubjectChoice == CPD_CHOICE_SIP)
    {
        DELETE_OBJECT(pProvData->pPDSip->pSip);
        DELETE_OBJECT(pProvData->pPDSip->pCATSip);

        _WVTSipFreeSubjectInfo(pProvData->pPDSip->psSipSubjectInfo);
        DELETE_OBJECT(pProvData->pPDSip->psSipSubjectInfo);

        _WVTSipFreeSubjectInfo(pProvData->pPDSip->psSipCATSubjectInfo);
        DELETE_OBJECT(pProvData->pPDSip->psSipCATSubjectInfo);

        TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pProvData->pPDSip->psIndirectData);

        DELETE_OBJECT(pProvData->pPDSip);
    }


    if (pProvData->hMsg)
    {
        CryptMsgClose(pProvData->hMsg);
        pProvData->hMsg = NULL;
    }

    // signer structure
    for (int i = 0; i < (int)pProvData->csSigners; i++)
    {
        TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pProvData->pasSigners[i].psSigner);

        DeallocateCertChain(pProvData->pasSigners[i].csCertChain,
                            &pProvData->pasSigners[i].pasCertChain);

        DELETE_OBJECT(pProvData->pasSigners[i].pasCertChain);

        if (_ISINSTRUCT(CRYPT_PROVIDER_SGNR,
                    pProvData->pasSigners[i].cbStruct, pChainContext) &&
                pProvData->pasSigners[i].pChainContext)
            CertFreeCertificateChain(pProvData->pasSigners[i].pChainContext);

        // counter signers
        for (int i2 = 0; i2 < (int)pProvData->pasSigners[i].csCounterSigners; i2++)
        {
            TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pProvData->pasSigners[i].pasCounterSigners[i2].psSigner);

            DeallocateCertChain(pProvData->pasSigners[i].pasCounterSigners[i2].csCertChain,
                                &pProvData->pasSigners[i].pasCounterSigners[i2].pasCertChain);

            DELETE_OBJECT(pProvData->pasSigners[i].pasCounterSigners[i2].pasCertChain);
            if (_ISINSTRUCT(CRYPT_PROVIDER_SGNR,
                    pProvData->pasSigners[i].pasCounterSigners[i2].cbStruct,
                        pChainContext) &&
                    pProvData->pasSigners[i].pasCounterSigners[i2].pChainContext)
                CertFreeCertificateChain(
                    pProvData->pasSigners[i].pasCounterSigners[i2].pChainContext);
        }

        DELETE_OBJECT(pProvData->pasSigners[i].pasCounterSigners);
    }

    DELETE_OBJECT(pProvData->pasSigners);

    // MUST BE DONE LAST!!!  Using the force flag!!!
    if (pProvData->pahStores)
    {
        DeallocateStoreChain(pProvData->chStores, pProvData->pahStores);

        DELETE_OBJECT(pProvData->pahStores);
    }

    pProvData->chStores = 0;

    // pProvData->padwTrustStepErrors
    DELETE_OBJECT(pProvData->padwTrustStepErrors);

    // pProvData->pasProvPrivData
    DELETE_OBJECT(pProvData->pasProvPrivData);
    pProvData->csProvPrivData = 0;

    // pProvData->psPfns
    if (pProvData->psPfns)
    {
        if (pProvData->psPfns->psUIpfns)
        {
            DELETE_OBJECT(pProvData->psPfns->psUIpfns->psUIData);
            DELETE_OBJECT(pProvData->psPfns->psUIpfns);
        }

        DELETE_OBJECT(pProvData->psPfns);
    }
}

void _CleanupProviderNonStateData(CRYPT_PROVIDER_DATA *pProvData)
{
    // pProvData->hProv: we're using default!

    // pProvData->pWintrustData->xxx->hFile: close!
    if ((pProvData->fOpenedFile) && (pProvData->pWintrustData != NULL))
    {
        HANDLE  *phFile;

        phFile  = NULL;

        switch (pProvData->pWintrustData->dwUnionChoice)
        {
            case WTD_CHOICE_FILE:
                phFile = &pProvData->pWintrustData->pFile->hFile;
                break;

            case WTD_CHOICE_CATALOG:
                phFile = &pProvData->pWintrustData->pCatalog->hMemberFile;
                break;
        }

        if ((phFile) && (*phFile) && (*phFile != INVALID_HANDLE_VALUE))
        {
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
            pProvData->fOpenedFile = FALSE;
        }
    }

    if (pProvData->dwSubjectChoice == CPD_CHOICE_SIP)
    {
        DELETE_OBJECT(pProvData->pPDSip->pSip);

        _WVTSipFreeSubjectInfoKeepState(pProvData->pPDSip->psSipSubjectInfo);

        // pProvData->pPDSip->psSipSubjectInfo: keep

        // pProvData->pPDSip->pCATSip: keep

        // pProvData->pPDSip->psSipCATSubjectInfo: keep

        TrustFreeDecode(WVT_MODID_WINTRUST, (BYTE **)&pProvData->pPDSip->psIndirectData);

        // pProvData->pPDSip: keep
    }


    // pProvData->hMsg: keep

    // signer structure: keep

    // pProvData->pahStores: keep

    // pProvData->padwTrustStepErrors: keep

    // pProvData->pasProvPrivData: keep

    // pProvData->psPfns: keep
}

BOOL _WVTSipFreeSubjectInfo(SIP_SUBJECTINFO *pSubj)
{
    if (!(pSubj))
    {
        return(FALSE);
    }

    DELETE_OBJECT(pSubj->pgSubjectType);

    switch(pSubj->dwUnionChoice)
    {
        case MSSIP_ADDINFO_BLOB:
            DELETE_OBJECT(pSubj->psBlob);
            break;

        case MSSIP_ADDINFO_CATMEMBER:
            if (pSubj->psCatMember)
            {
                // The following APIs are in DELAYLOAD'ed mscat32.dll. If the
                // DELAYLOAD fails an exception is raised.
                __try {
                    CryptCATClose(
                        CryptCATHandleFromStore(pSubj->psCatMember->pStore));
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    DWORD dwExceptionCode = GetExceptionCode();
                }

                DELETE_OBJECT(pSubj->psCatMember);
            }
            break;
    }

    return(TRUE);
}

BOOL _WVTSipFreeSubjectInfoKeepState(SIP_SUBJECTINFO *pSubj)
{
    if (!(pSubj))
    {
        return(FALSE);
    }

    DELETE_OBJECT(pSubj->pgSubjectType);

    switch(pSubj->dwUnionChoice)
    {
        case MSSIP_ADDINFO_BLOB:
            DELETE_OBJECT(pSubj->psBlob);
            break;

        case MSSIP_ADDINFO_CATMEMBER:
            break;
    }

    return(TRUE);
}

BOOL _WVTSetupProviderData(CRYPT_PROVIDER_DATA *psProvData, CRYPT_PROVIDER_DATA *psState)
{
    if (psState)
    {
        memcpy(psProvData, psState, sizeof(CRYPT_PROVIDER_DATA));

        if (_ISINSTRUCT(CRYPT_PROVIDER_DATA, psProvData->cbStruct, fRecallWithState))
        {
            psProvData->fRecallWithState = TRUE;
        }

        return(TRUE);
    }

    memset(psProvData, 0x00, sizeof(CRYPT_PROVIDER_DATA));

    psProvData->cbStruct    = sizeof(CRYPT_PROVIDER_DATA);

    if (!(psProvData->psPfns = (CRYPT_PROVIDER_FUNCTIONS *)WVTNew(sizeof(CRYPT_PROVIDER_FUNCTIONS))))
    {
        return(FALSE);
    }
    memset(psProvData->psPfns, 0x00, sizeof(CRYPT_PROVIDER_FUNCTIONS));
    psProvData->psPfns->cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);

    if (!(psProvData->psPfns->psUIpfns = (CRYPT_PROVUI_FUNCS *)WVTNew(sizeof(CRYPT_PROVUI_FUNCS))))
    {
        return(FALSE);
    }
    memset(psProvData->psPfns->psUIpfns, 0x00, sizeof(CRYPT_PROVUI_FUNCS));
    psProvData->psPfns->psUIpfns->cbStruct = sizeof(CRYPT_PROVUI_FUNCS);

    if (!(psProvData->psPfns->psUIpfns->psUIData = (CRYPT_PROVUI_DATA *)WVTNew(sizeof(CRYPT_PROVUI_DATA))))
    {
        return(FALSE);
    }
    memset(psProvData->psPfns->psUIpfns->psUIData, 0x00, sizeof(CRYPT_PROVUI_DATA));
    psProvData->psPfns->psUIpfns->psUIData->cbStruct = sizeof(CRYPT_PROVUI_DATA);

    GetSystemTimeAsFileTime(&psProvData->sftSystemTime);

    return(TRUE);
}

VOID FreeWintrustStateData (WINTRUST_DATA* pWintrustData)
{
    PCRYPT_PROVIDER_DATA pStateProvData;

    pStateProvData = WTHelperProvDataFromStateData(
                             pWintrustData->hWVTStateData
                             );

    if ( pStateProvData != NULL )
    {
        _CleanupProviderData( pStateProvData );
        DELETE_OBJECT( pWintrustData->hWVTStateData );
    }
}

