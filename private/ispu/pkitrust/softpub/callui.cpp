//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       callui.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubCallUI
//
//              *** local functions ***
//              _AllocGetOpusInfo
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "trustdb.h"
#include    "acui.h"

SPC_SP_OPUS_INFO *_AllocGetOpusInfo(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSigner,
                                    DWORD *pcbOpusInfo);

HRESULT SoftpubCallUI(CRYPT_PROVIDER_DATA *pProvData, DWORD dwError, BOOL fFinalCall)
{
    if (!(fFinalCall))
    {
        //  TBDTBD:  if we want the user to get involved along the way???
        return(ERROR_SUCCESS);
    }

    if (!(pProvData->dwRegPolicySettings & WTPF_ALLOWONLYPERTRUST) &&
        (pProvData->pWintrustData->dwUIChoice == WTD_UI_NONE))
    {
        return(dwError);
    }

    //
    //  call the ui
    //
    HRESULT                 hr;
    HINSTANCE               hModule;
    ACUI_INVOKE_INFO        aii;
    pfnACUIProviderInvokeUI pfn;
    IPersonalTrustDB        *pTrustDB;
    DWORD                   idxSigner;
    BOOL                    fTrusted;
    BOOL                    fCommercial;
    DWORD                   dwUIChoice;
    DWORD                   cbOpusInfo;
    CRYPT_PROVIDER_SGNR     *pRootSigner;
    CRYPT_PROVIDER_CERT     *pPubCert;

    memset(&aii, 0x00, sizeof(ACUI_INVOKE_INFO));

    hr          = E_NOTIMPL;
    hModule     = NULL;
    pfn         = NULL;
    pTrustDB    = NULL;
    fTrusted    = FALSE;
    fCommercial = FALSE;
    idxSigner   = 0;
    dwUIChoice  = pProvData->pWintrustData->dwUIChoice;
    pPubCert    = NULL;
    pRootSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    if ((pRootSigner) &&
        (pRootSigner->csCertChain > 0))
    {
        pPubCert    = WTHelperGetProvCertFromChain(pRootSigner, 0);
        fCommercial = pPubCert->fCommercial;

        //
        //  check the trust database.
        //
        if (dwError == ERROR_SUCCESS)
        {
            OpenTrustDB(NULL, IID_IPersonalTrustDB, (LPVOID*)&pTrustDB);

            if (pTrustDB) 
            {
                if (pTrustDB->IsTrustedCert(pProvData->dwEncoding, 
                                            pPubCert->pCert, 
                                            0, 
                                            fCommercial) == S_OK)
                {
                    fTrusted = TRUE;
                }
            }
            else
            {
                fTrusted = FALSE;  // If we can't open the trust DB, then we don't trust it
            }
        }
    }

    if (pProvData->dwRegPolicySettings & WTPF_ALLOWONLYPERTRUST)
    {
        if (!(pTrustDB))
        {
            OpenTrustDB(NULL, IID_IPersonalTrustDB, (LPVOID*)&pTrustDB);
        }

        if (!(pTrustDB) || 
            !(pPubCert) ||
            (pTrustDB->IsTrustedCert(pProvData->dwEncoding, pPubCert->pCert, 0, fCommercial) != S_OK))
        {
            //
            //  not in Trust DB and IEAK says don't allow!
            //
            goto ErrorSecuritySettings; // no ui
        }

        dwUIChoice = WTD_UI_NONE;
    }

    //
    //  if we're in the trust database and we're good....  don't bother the
    //  user with flashy UI!
    //
    if (fTrusted)
    {
        hr = ERROR_SUCCESS;

        goto CommonReturn;
    }

    if ((dwUIChoice == WTD_UI_NONE) ||
        ((dwUIChoice == WTD_UI_NOBAD) && (dwError != ERROR_SUCCESS)) ||
        ((dwUIChoice == WTD_UI_NOGOOD) && (dwError == ERROR_SUCCESS)))
    {
        hr = dwError;

        goto CommonReturn;
    }

    //
    // Setup the UI invokation
    //

    aii.cbSize                  = sizeof(ACUI_INVOKE_INFO);
    aii.hDisplay                = pProvData->hWndParent;
    aii.pProvData               = pProvData;
    aii.hrInvokeReason          = dwError;
    aii.pwcsAltDisplayName      = WTHelperGetFileName(pProvData->pWintrustData);
    aii.pPersonalTrustDB        = (IUnknown *)pTrustDB;

    if (pRootSigner)
    {
        aii.pOpusInfo   = _AllocGetOpusInfo(pProvData, pRootSigner, &cbOpusInfo);
    }
    
    //
    // Load the default authenticode UI.
    //
    if (hModule = LoadLibraryA(CVP_DLL))
    {
        pfn = (pfnACUIProviderInvokeUI)GetProcAddress(hModule, "ACUIProviderInvokeUI");
    }

    //
    // Invoke the UI
    //
    if (pfn)
    {
        hr = (*pfn)(&aii);
    }
    else 
    {
        if (hr == E_NOTIMPL)
        {
            hr = TRUST_E_PROVIDER_UNKNOWN;
        }
        pProvData->dwError = hr;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_UIPROV] = hr;

        DBG_PRINTF((DBG_SS, "Unable to load CRYPTUI.DLL\n"));

        goto CommonReturn;
    }

    //
    // Return the appropriate code
    //

    CommonReturn:
        if (pTrustDB)
        {
            pTrustDB->Release();
        }

        if (aii.pOpusInfo)
        {
            TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&aii.pOpusInfo);
        }

        if (hModule)
        {
            FreeLibrary(hModule);
        }

        return(hr);

    ErrorReturn:
        hr = GetLastError();
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, ErrorSecuritySettings, CRYPT_E_SECURITY_SETTINGS);
}


SPC_SP_OPUS_INFO *_AllocGetOpusInfo(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSigner,
                                    DWORD *pcbOpusInfo)
{
    PCRYPT_ATTRIBUTE    pAttr;
    PSPC_SP_OPUS_INFO   pInfo;

    pInfo   = NULL;

    if (!(pSigner->psSigner))
    {
        goto NoSigner;
    }

    if (pSigner->psSigner->AuthAttrs.cAttr == 0)
    {
        goto NoOpusAttribute;
    }

    if (!(pAttr = CertFindAttribute(SPC_SP_OPUS_INFO_OBJID, 
                                    pSigner->psSigner->AuthAttrs.cAttr,
                                    pSigner->psSigner->AuthAttrs.rgAttr)))
    {
        goto NoOpusAttribute;
    }

    if (!(pAttr->rgValue))
    {
        goto NoOpusAttribute;
    }

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, pcbOpusInfo, 200,
                      pProvData->dwEncoding, SPC_SP_OPUS_INFO_STRUCT, 
                      pAttr->rgValue->pbData, pAttr->rgValue->cbData, CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeError;
    }

    return(pInfo);

ErrorReturn:
    return(NULL);

    TRACE_ERROR_EX(DBG_SS, NoSigner);
    TRACE_ERROR_EX(DBG_SS, NoOpusAttribute);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

