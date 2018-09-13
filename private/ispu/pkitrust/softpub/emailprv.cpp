//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       emailprv.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  EmailRegisterServer
//              EmailUnregisterServer
//              EmailCertCheckProv
//              EmailFinalProv
//
//  History:    18-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

//////////////////////////////////////////////////////////////////////////////
//
// EmailRegisterServer
//----------------------------------------------------------------------------
//  Register the Email provider
//  

STDAPI EmailRegisterServer(void)
{
    GUID                        gProv = EMAIL_ACTIONID_VERIFY;
    BOOL                        fRet;
    CRYPT_REGISTER_ACTIONID     sRegAID;
    CRYPT_PROVIDER_REGDEFUSAGE  sDefUsage;

    fRet = TRUE;

    //
    //  set the usages we want
    //
    memset(&sDefUsage, 0x00, sizeof(CRYPT_PROVIDER_REGDEFUSAGE));

    sDefUsage.cbStruct                                  = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    sDefUsage.pgActionID                                = &gProv;
    sDefUsage.pwszDllName                               = SP_POLICY_PROVIDER_DLL_NAME;
    sDefUsage.pwszLoadCallbackDataFunctionName          = "SoftpubLoadDefUsageCallData";
    sDefUsage.pwszFreeCallbackDataFunctionName          = "SoftpubFreeDefUsageCallData";

    fRet &= WintrustAddDefaultForUsage(szOID_PKIX_KP_EMAIL_PROTECTION, &sDefUsage);

    //
    //  set our provider
    //
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

    // wintrust's certificate provider
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;     // set to wintrust.dll
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION; // use wintrust's standard!

    // custom cert check due to different CTL usages
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
//TBDTBD    sRegAID.sCertificatePolicyProvider.pwszFunctionName = HTTPS_CHKCERT_FUNCTION;

    // custom final ...
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
//TBDTBD    sRegAID.sFinalPolicyProvider.pwszFunctionName       = HTTPS_FINALPOLICY_FUNCTION;

    // Authenticode cleanup -- we don't store any data.
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = SP_CLEANUPPOLICY_FUNCTION;

    fRet &= WintrustAddActionID(&gProv, 0, &sRegAID);

    return((fRet) ? S_OK : S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//----------------------------------------------------------------------------
//  unregisters email provider
//  

STDAPI HTTPSUnregisterServer(void)
{
    GUID    gProv = EMAIL_ACTIONID_VERIFY;

    WintrustRemoveActionID(&gProv);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
//
// Exported functions for wintrust
//

BOOL WINAPI EmailCheckCertProv(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, 
                               BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
}

HRESULT WINAPI HTTPSFinalProv(CRYPT_PROVIDER_DATA *pProvData)
{
}

///////////////////////////////////////////////////////////////////////////////////
//
//      Local Functions
//
///////////////////////////////////////////////////////////////////////////////////
