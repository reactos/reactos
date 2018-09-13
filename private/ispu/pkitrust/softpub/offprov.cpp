//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       offprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  OfficeRegisterServer
//              OfficeUnregisterServer
//              OfficeInitializePolicy
//              OfficeCleanupPolicy
//
//              *** local functions ***
//              _SetOverrideText
//
//  History:    18-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

void _SetOverrideText(CRYPT_PROVIDER_DATA *pProvData, WCHAR **ppwszRet, DWORD dwStringId);

//////////////////////////////////////////////////////////////////////////////
//
// OfficeRegisterServer
//----------------------------------------------------------------------------
//  Register the office provider
//  

STDAPI OfficeRegisterServer(void)
{
    GUID                        gOfficeProv = OFFICESIGN_ACTION_VERIFY;

    CRYPT_REGISTER_ACTIONID     sRegAID;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    // my initialization provider
    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = OFFICE_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = OFFICE_INITPROV_FUNCTION;

    // Authenticode object provider
    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    // Authenticode signature provider
    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    // wintrust's certificate provider
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;     // set to wintrust.dll
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION; // use wintrust's standard!

    // Authenticode certificate checker
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    // Authenticode final
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = SP_FINALPOLICY_FUNCTION;

    // Authenticode cleanup
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = OFFICE_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = OFFICE_CLEANUPPOLICY_FUNCTION;

    if (WintrustAddActionID(&gOfficeProv, 0, &sRegAID))
    {
        return(S_OK);
    }

    return(S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//----------------------------------------------------------------------------
//  unregisters office provider
//  

STDAPI OfficeUnregisterServer(void)
{
    GUID    gOfficeProv = OFFICESIGN_ACTION_VERIFY;

    WintrustRemoveActionID(&gOfficeProv);

    return(S_OK);
}


typedef struct _OFFPROV_PRIVATE_DATA
{
    DWORD                       cbStruct;

    CRYPT_PROVIDER_FUNCTIONS    sAuthenticodePfns;

} OFFPROV_PRIVATE_DATA, *POFFPROV_PRIVATE_DATA;


//////////////////////////////////////////////////////////////////////////////
//
// Initialize Policy Provider function: OfficeInitializePolicy
//----------------------------------------------------------------------------
//  change the OID to the email OID for Usage....
//  

static char *pszOfficeUsage = szOID_PKIX_KP_CODE_SIGNING;

HRESULT WINAPI OfficeInitializePolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_INVALID_PARAMETER;
        return(S_FALSE);
    }

    GUID                        gAuthenticode = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                        gOfficeProv = OFFICESIGN_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     sPrivData;
    CRYPT_PROVIDER_PRIVDATA     *pPrivData;
    OFFPROV_PRIVATE_DATA        *pOfficeData;
    HRESULT                     hr;

    memset(&sPrivData, 0x00, sizeof(CRYPT_PROVIDER_PRIVDATA));
    sPrivData.cbStruct      = sizeof(CRYPT_PROVIDER_PRIVDATA);

    memcpy(&sPrivData.gProviderID, &gOfficeProv, sizeof(GUID));

    //
    //  add my data to the chain!
    //
    pProvData->psPfns->pfnAddPrivData2Chain(pProvData, &sPrivData);

    //
    //  get the new reference
    //
    pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gOfficeProv);


    //
    //  allocate space for my struct
    //
    if (!(pPrivData->pvProvData = pProvData->psPfns->pfnAlloc(sizeof(OFFPROV_PRIVATE_DATA))))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_SYSTEM_ERROR;
        return(S_FALSE);
    }

    memset(pPrivData->pvProvData, 0x00, sizeof(OFFPROV_PRIVATE_DATA));
    pPrivData->cbProvData   = sizeof(OFFPROV_PRIVATE_DATA);

    pOfficeData             = (OFFPROV_PRIVATE_DATA *)pPrivData->pvProvData;
    pOfficeData->cbStruct   = sizeof(OFFPROV_PRIVATE_DATA);

    //
    //  fill in the Authenticode Functions
    //
    pOfficeData->sAuthenticodePfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);

    if (!(WintrustLoadFunctionPointers(&gAuthenticode, &pOfficeData->sAuthenticodePfns)))
    {
        pProvData->psPfns->pfnFree(sPrivData.pvProvData);
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_PROVIDER_UNKNOWN;
        return(S_FALSE);
    }

    if (pOfficeData->sAuthenticodePfns.pfnInitialize)
    {
        hr = pOfficeData->sAuthenticodePfns.pfnInitialize(pProvData);
    }

    //
    //  assign our usage
    //
    pProvData->pszUsageOID  = pszOfficeUsage;

    //
    //  change the text on the dialog buttons
    //
    if (pProvData->psPfns->psUIpfns)
    {
        if (pProvData->psPfns->psUIpfns->psUIData)
        {
            if (!(_ISINSTRUCT(CRYPT_PROVUI_DATA, pProvData->psPfns->psUIpfns->psUIData->cbStruct, pCopyActionTextNotSigned)))
            {
                return(hr);
            }

            _SetOverrideText(pProvData, &pProvData->psPfns->psUIpfns->psUIData->pYesButtonText,        
                             IDS_OFFICE_YES_BUTTON_TEXT);
            _SetOverrideText(pProvData, &pProvData->psPfns->psUIpfns->psUIData->pNoButtonText,         
                             IDS_OFFICE_NO_BUTTON_TEXT);
            _SetOverrideText(pProvData, &pProvData->psPfns->psUIpfns->psUIData->pCopyActionText,       
                             IDS_OFFICE_COPYACTION_TEXT);
            _SetOverrideText(pProvData, &pProvData->psPfns->psUIpfns->psUIData->pCopyActionTextNoTS,   
                             IDS_OFFICE_COPYACTION_NOTS_TEXT);
            _SetOverrideText(pProvData, &pProvData->psPfns->psUIpfns->psUIData->pCopyActionTextNotSigned, 
                             IDS_OFFICE_COPYACTION_NOSIGN_TEXT);
        }
    }

    return(hr);
}

void _SetOverrideText(CRYPT_PROVIDER_DATA *pProvData, WCHAR **ppwszRet, DWORD dwStringId)
{
    WCHAR                       wsz[MAX_PATH];

    if (*ppwszRet)
    {
        pProvData->psPfns->pfnFree(*ppwszRet);
        *ppwszRet = NULL;
    }

    wsz[0] = NULL;
    LoadStringU(hinst, dwStringId, &wsz[0], MAX_PATH);

    if (wsz[0])
    {
        if (*ppwszRet = (WCHAR *)pProvData->psPfns->pfnAlloc((wcslen(&wsz[0]) + 1) * sizeof(WCHAR)))
        {
            wcscpy(*ppwszRet, &wsz[0]);
        }
    }
}

HRESULT WINAPI OfficeCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                        gOfficeProv = OFFICESIGN_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     *pMyData;
    OFFPROV_PRIVATE_DATA        *pOfficeData;
    HRESULT                     hr;

    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    pMyData = WTHelperGetProvPrivateDataFromChain(pProvData, &gOfficeProv);

    if (pMyData)
    {
        pOfficeData = (OFFPROV_PRIVATE_DATA *)pMyData->pvProvData;
        //
        //  remove the data we allocated except for the "MyData" which WVT will clean up for us!
        //

        if (pOfficeData->sAuthenticodePfns.pfnCleanupPolicy)
        {
            hr = pOfficeData->sAuthenticodePfns.pfnCleanupPolicy(pProvData);
        }

        pProvData->psPfns->pfnFree(pMyData->pvProvData);
        pMyData->pvProvData = NULL;
    }

    if (pProvData->psPfns->psUIpfns)
    {
        if (pProvData->psPfns->psUIpfns->psUIData)
        {
            if (_ISINSTRUCT(CRYPT_PROVUI_DATA, pProvData->psPfns->psUIpfns->psUIData->cbStruct, pCopyActionText))
            {
                pProvData->psPfns->pfnFree(pProvData->psPfns->psUIpfns->psUIData->pYesButtonText);
                pProvData->psPfns->psUIpfns->psUIData->pYesButtonText = NULL;

                pProvData->psPfns->pfnFree(pProvData->psPfns->psUIpfns->psUIData->pNoButtonText);
                pProvData->psPfns->psUIpfns->psUIData->pNoButtonText = NULL;

                pProvData->psPfns->pfnFree(pProvData->psPfns->psUIpfns->psUIData->pCopyActionText);
                pProvData->psPfns->psUIpfns->psUIData->pCopyActionText = NULL;
            }
        }
    }

    return(hr);
}
