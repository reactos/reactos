//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       testprov.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  DllRegisterServer
//              DllUnregisterServer
//              TestprovInitialize
//              TestprovObjectProv
//              TestprovSigProv
//              TestprovCertCheckProv
//              TestprovFinalProv
//              TestprovCleanup
//              TestprovTester
//
//  History:    28-Jul-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    <wincrypt.h>
#include    <wintrust.h>    // structures and APIs
#include    "wintrustp.h"    // structures and APIs
#include    <softpub.h>     // reference for Authenticode
#include    <acui.h>        // ui module DACUI.DLL

#include    "testprov.h"    // my stuff


HRESULT     CallUI(CRYPT_PROVIDER_DATA *pProvData, DWORD dwError);
DWORD       GetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData);
HRESULT     CheckCertificateChain(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pProvSgnr);
HRESULT     CheckRevocation(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSgnr);
BOOL        CheckCertAnyUnknownCriticalExtensions(CRYPT_PROVIDER_DATA *pProvData, PCERT_INFO pCertInfo);


//////////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//----------------------------------------------------------------------------
//  Register "my" provider
//  

STDAPI DllRegisterServer(void)
{
    GUID                        gTestprov = TESTPROV_ACTION_TEST;

    CRYPT_REGISTER_ACTIONID     sRegAID;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = TP_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = TP_INIT_FUNCTION;

    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = TP_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = TP_OBJTRUST_FUNCTION;

    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = TP_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = TP_SIGTRUST_FUNCTION;

    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;     // set to wintrust.dll
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION; // use wintrust's standard!

    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = TP_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = TP_CHKCERT_FUNCTION;

    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = TP_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = TP_FINALPOLICY_FUNCTION;

    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = TP_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = TP_CLEANUPPOLICY_FUNCTION;

    sRegAID.sTestPolicyProvider.cbStruct                = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sTestPolicyProvider.pwszDLLName             = TP_DLL_NAME;
    sRegAID.sTestPolicyProvider.pwszFunctionName        = TP_TESTDUMPPOLICY_FUNCTION_TEST;

    
    if (WintrustAddActionID(&gTestprov, 0, &sRegAID))
    {
        return(S_OK);
    }

    return(S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//----------------------------------------------------------------------------
//  unregisters "my" provider
//  

STDAPI DllUnregisterServer(void)
{
    GUID    gTestprov = TESTPROV_ACTION_TEST;

    WintrustRemoveActionID(&gTestprov);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
//
// Initialization Provider function: testprovInitialize
//----------------------------------------------------------------------------
//  allocates and sets up "my" data.
//  

HRESULT WINAPI TestprovInitialize(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    //
    //  add our private data to the array...
    //
    CRYPT_PROVIDER_PRIVDATA sMyData;
    TESTPROV_PRIVATE_DATA   *pMyData;
    GUID                    gMyId = TESTPROV_ACTION_TEST;

    memset(&sMyData, 0x00, sizeof(CRYPT_PROVIDER_PRIVDATA));
    sMyData.cbStruct        = sizeof(CRYPT_PROVIDER_PRIVDATA);

    memcpy(&sMyData.gProviderID, &gMyId, sizeof(GUID));

    if (!(sMyData.pvProvData = pProvData->psPfns->pfnAlloc(sizeof(TESTPROV_PRIVATE_DATA))))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_SYSTEM_ERROR;
        return(S_FALSE);
    }

    memset(sMyData.pvProvData, 0x00, sizeof(TESTPROV_PRIVATE_DATA));

    pMyData             = (TESTPROV_PRIVATE_DATA *)sMyData.pvProvData;
    pMyData->cbStruct   = sizeof(TESTPROV_PRIVATE_DATA);

    //
    //  fill in the Authenticode Functions
    //
    GUID                        gSP = WINTRUST_ACTION_TRUSTPROVIDER_TEST;

    pMyData->sAuthenticodePfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS_ORMORE);

    if (!(WintrustLoadFunctionPointers(&gSP, (CRYPT_PROVIDER_FUNCTIONS *)&pMyData->sAuthenticodePfns)))
    {
        pProvData->psPfns->pfnFree(sMyData.pvProvData);
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_PROVIDER_UNKNOWN;
        return(S_FALSE);
    }

    //
    //  add my data to the chain!
    //
    pProvData->psPfns->pfnAddPrivData2Chain(pProvData, &sMyData);


    return(pMyData->sAuthenticodePfns.pfnInitialize(pProvData));
}

//////////////////////////////////////////////////////////////////////////////
//
// Object Provider function: TestprovObjectProv
//----------------------------------------------------------------------------
//  we don't do anything here -- we're not authenticating a signed object.
//  

HRESULT WINAPI TestprovObjectProv(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    CRYPT_PROVIDER_PRIVDATA *pPrivData;
    TESTPROV_PRIVATE_DATA   *pMyData;
    GUID                    gMyId = TESTPROV_ACTION_TEST;

    if (!(pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gMyId)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = ERROR_INVALID_PARAMETER;
        return(S_FALSE);
    }

    pMyData = (TESTPROV_PRIVATE_DATA *)pPrivData->pvProvData;

    //
    //  we are verifying a low-level type choice (eg: cert or signer)
    //
    switch (pProvData->pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_CERT:
        case WTD_CHOICE_SIGNER:
                    break;

        default:
                    return(pMyData->sAuthenticodePfns.pfnObjectTrust(pProvData));
    }

    //
    //  no object to be verified here!
    //
    return(ERROR_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
//
// Signature Provider function: TestprovInitialize
//----------------------------------------------------------------------------
//  We are going to let Authenticode take care of most of this stuff!
//  

HRESULT WINAPI TestprovSigProv(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != ERROR_SUCCESS) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    CRYPT_PROVIDER_PRIVDATA *pPrivData;
    TESTPROV_PRIVATE_DATA   *pMyData;
    GUID                    gMyId = TESTPROV_ACTION_TEST;

    if (!(pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gMyId)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = ERROR_INVALID_PARAMETER;
        return(S_FALSE);
    }

    pMyData = (TESTPROV_PRIVATE_DATA *)pPrivData->pvProvData;

    //
    //  we are verifying a low-level type choice (eg: cert or signer)
    //
    switch (pProvData->pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_CERT:
        case WTD_CHOICE_SIGNER:
                    break;

        default:
                    return(pMyData->sAuthenticodePfns.pfnSignatureTrust(pProvData));
    }

    if (pMyData->sAuthenticodePfns.pfnSignatureTrust)
    {
        return(pMyData->sAuthenticodePfns.pfnSignatureTrust(pProvData));
    }

    return(S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// Certificate Check Provider function: TestprovCertCheckProv
//----------------------------------------------------------------------------
//  just check basic stuff about a certificate.  return FALSE to stop
//  building the chain, TRUE to continue.
//  

BOOL WINAPI TestprovCheckCertProv(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, 
                                  BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;
    PCCERT_CONTEXT          pCertContext;

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, idxSigner, fCounterSignerChain, idxCounterSigner);

    pCert = WTHelperGetProvCertFromChain(pSgnr, pSgnr->csCertChain - 1);

    pCert->fTrustedRoot = FALSE;

    //
    //  only self signed certificates in the root store are "trusted" roots
    //
    if (pCert->fSelfSigned)
    {
        pCertContext = pCert->pCert;

        if (pCertContext)
        {
            if (pProvData->chStores > 0)
            {
                if (pCertContext->hCertStore == pProvData->pahStores[0])
                {
                    //
                    //  it's in the root store!
                    //
                    pCert->fTrustedRoot = TRUE;
                    
                    return(FALSE);
                }
            }

            if (!(pCert->fTrustedRoot))
            {
                if (pCert->fTestCert)
                {
                    //
                    //  check the policy flags (setreg.exe) to see if we trust
                    //  the test root.
                    //
                    if (pProvData->dwRegPolicySettings & WTPF_TRUSTTEST)
                    {
                        pCert->fTrustedRoot = TRUE;
                        return(FALSE);
                    }
                }
            }
        }

        //
        //  the cert is self-signed... we need to stop regardless
        //
        return(FALSE);
    }

    return(TRUE);
}

//////////////////////////////////////////////////////////////////////////////
//
// Final Policy Provider function: TestprovFinalProv
//----------------------------------------------------------------------------
//  Check the outcome of the previous functions and display UI based on this.
//  

HRESULT WINAPI TestprovFinalProv(CRYPT_PROVIDER_DATA *pProvData)
{
    CRYPT_PROVIDER_SGNR *pSigner;
    DWORD               dwError;

    if ((dwError = GetErrorBasedOnStepErrors(pProvData)) != ERROR_SUCCESS)
    {
        return(CallUI(pProvData, dwError));
    }

    pSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    if ((dwError = CheckCertificateChain(pProvData, pSigner)) != ERROR_SUCCESS)
    {
        return(CallUI(pProvData, dwError));
    }

    return(CallUI(pProvData, dwError));
}

//////////////////////////////////////////////////////////////////////////////
//
// Cleanup Provider function: TestprovCleanup
//----------------------------------------------------------------------------
//  call all other policy provider cleanup functions, then, free "my" stuff.
//  

HRESULT WINAPI TestprovCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                    gMyId = TESTPROV_ACTION_TEST;
    CRYPT_PROVIDER_PRIVDATA *pPrivData;
    TESTPROV_PRIVATE_DATA   *pMyData;

    //
    //  we have used the Authenticode Provider.  we need to call its
    //  cleanup routine just in case....  so, get our private data
    //  which will have the Authenticode functions in our structure..
    //

    if (!(pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gMyId)))
    {
        return(S_FALSE);
    }

    if (!(pPrivData->pvProvData))
    {
        return(S_FALSE);
    }

    pMyData = (TESTPROV_PRIVATE_DATA *)pPrivData->pvProvData;

    if (pMyData->sAuthenticodePfns.pfnCleanupPolicy)
    {
        pMyData->sAuthenticodePfns.pfnCleanupPolicy(pProvData);
    }

    //
    //  now, we need to delete our private data
    //
    pProvData->psPfns->pfnFree(pPrivData->pvProvData);
    pPrivData->cbProvData   = 0;
    pPrivData->pvProvData   = NULL;

    return(ERROR_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
//
// Test Provider function: TestprovTester
//----------------------------------------------------------------------------
//  In here, we are going to check an environment variable and if set we 
//  will call Authenticode's "dump" function.
//  

HRESULT WINAPI TestprovTester(CRYPT_PROVIDER_DATA *pProvData)
{
    BYTE                    abEnv[MAX_PATH + 1];
    CRYPT_PROVIDER_PRIVDATA *pPrivData;
    TESTPROV_PRIVATE_DATA   *pMyData;
    GUID                    gMyId = TESTPROV_ACTION_TEST;

    abEnv[0] = NULL;

    if (GetEnvironmentVariable("TestProvUseDump", (char *)&abEnv[0], MAX_PATH) < 1)
    {
        return(ERROR_SUCCESS);
    }

    if ((abEnv[0] != '1') && (toupper(abEnv[0]) != 'T'))
    {
        return(ERROR_SUCCESS);
    }

    if (!(pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gMyId)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = ERROR_INVALID_PARAMETER;
        return(S_FALSE);
    }

    pMyData = (TESTPROV_PRIVATE_DATA *)pPrivData->pvProvData;

    if (pMyData->sAuthenticodePfns.pfnTestFinalPolicy)
    {
        return(pMyData->sAuthenticodePfns.pfnTestFinalPolicy(pProvData));
    }

    return(S_FALSE);
}

///////////////////////////////////////////////////////////////////////////////////
//
//      Local Functions
//
///////////////////////////////////////////////////////////////////////////////////

DWORD GetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData)
{
    //
    //  initial allocation of the step errors?
    //
    if (!(pProvData->padwTrustStepErrors))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
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

HRESULT CheckCertificateChain(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pProvSgnr)
{
    CRYPT_PROVIDER_CERT *pCert;

    for (int i = 0; i < (int)pProvSgnr->csCertChain; i++)
    {
        pCert = WTHelperGetProvCertFromChain(pProvSgnr, i);


        if (!(pProvData->dwRegPolicySettings  & WTPF_IGNOREEXPIRATION))
        {
            //
            //  this check was done in the Certificate Provider, however, it may not have passed
            //  because all its looking for is a confidence level and didn't check the end..
            //
            if (CertVerifyTimeValidity(&pProvSgnr->sftVerifyAsOf, pCert->pCert->pCertInfo) != 0)
            {
                pCert->dwError  = CERT_E_EXPIRED;

                return(pCert->dwError);
            }
        }

        //
        //  check any unknown critical extensions
        //
        if (!(CheckCertAnyUnknownCriticalExtensions(pProvData, pCert->pCert->pCertInfo)))
        {
            pCert->dwError  = CERT_E_MALFORMED;

            return(pCert->dwError);
        }

        if ((i + 1) < (int)pProvSgnr->csCertChain)
        {
            //
            //  check time nesting...
            //
            if (!(pCert->dwConfidence & CERT_CONFIDENCE_TIMENEST))
            {
                pCert->dwError  = CERT_E_VALIDITYPERIODNESTING;

                return(pCert->dwError);
            }
            
        }
    }

    if (!(pProvData->dwRegPolicySettings & WTPF_IGNOREREVOKATION))
    {
        // root cert is test?
        pCert = WTHelperGetProvCertFromChain(pProvSgnr, pProvSgnr->csCertChain - 1);

        if (pCert)
        {
            if (!(pCert->fTestCert))
            {
                //
                //  if the caller already told WVT to check, we don't have to!
                //
                if (pProvData->pWintrustData->fdwRevocationChecks != WTD_REVOKE_NONE)
                {
                    //
                    //  not a test root, check signer cert for revocation
                    //
                    pCert = WTHelperGetProvCertFromChain(pProvSgnr, 0);

                    return(CheckRevocation(pProvData, pProvSgnr));
                }
            }
        }
    }

    return(ERROR_SUCCESS);
}


HRESULT CallUI(CRYPT_PROVIDER_DATA *pProvData, DWORD dwError)
{
    HRESULT                 hr;
    HINSTANCE               hModule;
    ACUI_INVOKE_INFO        aii;
    pfnACUIProviderInvokeUI pfn;
    DWORD                   idxSigner;
    BOOL                    fTrusted;
    BOOL                    fCommercial;
    DWORD                   dwUIChoice;
    CRYPT_PROVIDER_SGNR     *pRootSigner;
    CRYPT_PROVIDER_CERT     *pPubCert;

    hr          = E_NOTIMPL;

    pfn         = NULL;

    fTrusted    = FALSE;

    fCommercial = FALSE;

    idxSigner   = 0;

    dwUIChoice  = pProvData->pWintrustData->dwUIChoice;

    pRootSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);


    if ((dwUIChoice == WTD_UI_NONE) ||
        ((dwUIChoice == WTD_UI_NOBAD) && (dwError != ERROR_SUCCESS)) ||
        ((dwUIChoice == WTD_UI_NOGOOD) && (dwError == ERROR_SUCCESS)))
    {
        return(dwError);
    }

    //
    // Setup the UI invokation
    //
    memset(&aii, 0x00, sizeof(ACUI_INVOKE_INFO));

    aii.cbSize                  = sizeof(ACUI_INVOKE_INFO);
    aii.hDisplay                = pProvData->hWndParent;
    aii.pProvData               = pProvData;
    aii.hrInvokeReason          = dwError;
    aii.pwcsAltDisplayName      = WTHelperGetFileName(pProvData->pWintrustData);

    //
    // Load the default authenticode UI.
    //
    if (hModule = LoadLibrary("dacui.dll"))
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
    else if ((pProvData->pWintrustData->dwUIChoice != WTD_UI_NONE) &&
             (pProvData->pWintrustData->dwUIChoice != WTD_UI_NOBAD))
    {
        //TBDTBD!!!
        //
        //  display error dialog "unable to load UI provider"
        //
        if (hr == E_NOTIMPL)
        {
            hr = TRUST_E_PROVIDER_UNKNOWN;
        }
        pProvData->dwError = hr;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_UIPROV] = hr;
    }

    //
    //  free the ui library
    //

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    //
    // Return the appropriate code
    //
    return(hr);
}

static const char *rgpszKnownExtObjId[] = 
{
    szOID_AUTHORITY_KEY_IDENTIFIER,
    szOID_KEY_ATTRIBUTES,
    szOID_KEY_USAGE_RESTRICTION,
    szOID_SUBJECT_ALT_NAME,
    szOID_ISSUER_ALT_NAME,
    szOID_BASIC_CONSTRAINTS,
    SPC_COMMON_NAME_OBJID,
    SPC_SP_AGENCY_INFO_OBJID,
    SPC_MINIMAL_CRITERIA_OBJID,
    SPC_FINANCIAL_CRITERIA_OBJID,
    szOID_CERT_POLICIES,
    szOID_POLICY_MAPPINGS,
    szOID_SUBJECT_DIR_ATTRS,
    NULL
};


BOOL CheckCertAnyUnknownCriticalExtensions(CRYPT_PROVIDER_DATA *pProvData, PCERT_INFO pCertInfo)
{
    PCERT_EXTENSION     pExt;
    DWORD               cExt;
    const char          **ppszObjId;
    const char          *pszObjId;
    
    cExt = pCertInfo->cExtension;
    pExt = pCertInfo->rgExtension;

    for ( ; cExt > 0; cExt--, pExt++) 
    {
        if (pExt->fCritical) 
        {
            ppszObjId = (const char **)rgpszKnownExtObjId;

            while (pszObjId = *ppszObjId++) 
            {
                if (strcmp(pszObjId, pExt->pszObjId) == 0)
                {
                    break;
                }
            }
            if (!(pszObjId))
            {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

HRESULT CheckRevocation(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSgnr)
{
    CERT_REVOCATION_PARA    sRevPara;
    CERT_REVOCATION_STATUS  sRevStatus;
    PCERT_CONTEXT           pasCertContext[1];
    CRYPT_PROVIDER_CERT     *pCert;


    memset(&sRevPara, 0x00, sizeof(CERT_REVOCATION_PARA));

    sRevPara.cbSize         = sizeof(CERT_REVOCATION_PARA);

    // issuer cert = 1
    pCert = WTHelperGetProvCertFromChain(pSgnr, 1);
    sRevPara.pIssuerCert    = pCert->pCert;

    memset(&sRevStatus, 0x00, sizeof(CERT_REVOCATION_STATUS));

    sRevStatus.cbSize       = sizeof(CERT_REVOCATION_STATUS);

    // publisher cert = 0
    pCert = WTHelperGetProvCertFromChain(pSgnr, 0);
    pasCertContext[0]       = (PCERT_CONTEXT)pCert->pCert;

    if (!(CertVerifyRevocation(pProvData->dwEncoding,
                               CERT_CONTEXT_REVOCATION_TYPE,
                               1,
                               (void **)pasCertContext,
                               0, // CERT_VERIFY_REV_CHAIN_FLAG,
                               &sRevPara,
                               &sRevStatus)))
    {
        pCert->dwRevokedReason  = sRevStatus.dwReason;

        switch(sRevStatus.dwError)
        {
            case CRYPT_E_REVOKED:
                return(CERT_E_REVOKED);

            case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
                return(ERROR_SUCCESS);

            case CRYPT_E_REVOCATION_OFFLINE:
                if ((pProvData->dwRegPolicySettings & WTPF_OFFLINEOK_IND) ||
                    (pProvData->dwRegPolicySettings & WTPF_OFFLINEOKNBU_IND))
                {
                    return(ERROR_SUCCESS);
                }
                
                return(CERT_E_REVOCATION_FAILURE);

            default:
                return(CERT_E_REVOCATION_FAILURE);

        }
    }

    return(ERROR_SUCCESS);
}
