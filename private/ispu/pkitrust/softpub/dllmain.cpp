//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dllmain.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//
//              *** local functions ***
//              SPNew
//
//  History:    28-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

HINSTANCE   hinst;

//////////////////////////////////////////////////////////////////////////////////////
//
// standard DLL exports ...
//
//

BOOL WINAPI SoftpubDllMain(HANDLE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hinst = (HINSTANCE)hInstDLL;
    }

    return(TRUE);
}


#include    "wvtver1.h"

STDAPI SoftpubDllRegisterServer(void)
{
    GUID                            gV1UISup    = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID                            gV1UINoBad  = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;
    GUID                            gV2         = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                            gV2TrstTest = WINTRUST_ACTION_TRUSTPROVIDER_TEST;
    GUID                            gCert       = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    CRYPT_REGISTER_ACTIONID         sRegAID;
    CRYPT_PROVIDER_REGDEFUSAGE      sDefUsage;

    BOOL                            fRet;

    fRet = TRUE;

    memset(&sDefUsage, 0x00, sizeof(CRYPT_PROVIDER_REGDEFUSAGE));

    sDefUsage.cbStruct                                  = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    sDefUsage.pgActionID                                = &gV2;

    fRet &= WintrustAddDefaultForUsage(szOID_PKIX_KP_CODE_SIGNING, &sDefUsage);

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = SP_INIT_FUNCTION;

    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION;

    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = SP_FINALPOLICY_FUNCTION;

    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = SP_CLEANUPPOLICY_FUNCTION;


    //
    //  V2
    //
    fRet &= WintrustAddActionID(&gV2, 0, &sRegAID);

    //
    //  support for V1
    //
    fRet &= WintrustAddActionID(&gV1UISup, 0, &sRegAID);
    fRet &= WintrustAddActionID(&gV1UINoBad, 0, &sRegAID);

    sRegAID.sInitProvider.pwszFunctionName              = SP_GENERIC_CERT_INIT_FUNCTION;
    fRet &= WintrustAddActionID(&gCert, 0, &sRegAID);
    sRegAID.sInitProvider.pwszFunctionName              = SP_INIT_FUNCTION;

    //
    //  testing support
    //
    sRegAID.sTestPolicyProvider.cbStruct                = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sTestPolicyProvider.pwszDLLName             = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sTestPolicyProvider.pwszFunctionName        = SP_TESTDUMPPOLICY_FUNCTION_TEST;
    fRet &= WintrustAddActionID(&gV2TrstTest, 0, &sRegAID);

    memset(&sRegAID.sTestPolicyProvider, 0x00, sizeof(CRYPT_TRUST_REG_ENTRY));

    if (fRet)
    {
        HTTPSRegisterServer();
        OfficeRegisterServer();
        DriverRegisterServer();
        GenericChainRegisterServer();

        return(S_OK);
    }

    return(S_FALSE);
}

STDAPI SoftpubDllUnregisterServer(void)
{
    GUID    gV1UISup    = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID    gV1UINoBad  = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;
    GUID    gV2         = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID    gV2TrstTest = WINTRUST_ACTION_TRUSTPROVIDER_TEST;
    GUID    gCert       = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    WintrustRemoveActionID(&gV1UISup);
    WintrustRemoveActionID(&gV1UINoBad);
    WintrustRemoveActionID(&gV2);
    WintrustRemoveActionID(&gV2TrstTest);
    WintrustRemoveActionID(&gCert);

    HTTPSUnregisterServer();
    OfficeUnregisterServer();
    DriverUnregisterServer();
    GenericChainUnregisterServer();

    return(S_OK);
}
