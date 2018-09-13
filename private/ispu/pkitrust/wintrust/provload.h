//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       provload.h
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  History:    29-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef PROVLOAD_H
#define PROVLOAD_H

typedef struct _LOADED_PROVIDER 
{

    struct _LOADED_PROVIDER             *pNext;
    struct _LOADED_PROVIDER             *pPrev;
    GUID                                gActionID;

    HINSTANCE                           hInitDLL;
    HINSTANCE                           hObjectDLL;
    HINSTANCE                           hSignatureDLL;
    HINSTANCE                           hCertTrustDLL;
    HINSTANCE                           hFinalPolicyDLL;
    HINSTANCE                           hCertPolicyDLL;
    HINSTANCE                           hTestFinalPolicyDLL;
    HINSTANCE                           hCleanupPolicyDLL;

    PFN_PROVIDER_INIT_CALL              pfnInitialize;          // initialize Policy 
    PFN_PROVIDER_OBJTRUST_CALL          pfnObjectTrust;         // build info to the msg
    PFN_PROVIDER_SIGTRUST_CALL          pfnSignatureTrust;      // build info to the signing cert
    PFN_PROVIDER_CERTTRUST_CALL         pfnCertificateTrust;    // build the chain
    PFN_PROVIDER_FINALPOLICY_CALL       pfnFinalPolicy;         // final call to policy
    PFN_PROVIDER_CERTCHKPOLICY_CALL     pfnCertCheckPolicy;     // check each cert will building chain
    PFN_PROVIDER_TESTFINALPOLICY_CALL   pfnTestFinalPolicy;
    PFN_PROVIDER_CLEANUP_CALL           pfnCleanupPolicy;

} LOADED_PROVIDER, *PLOADED_PROVIDER;


extern BOOL WintrustUnloadProviderList(void);

#endif // PROVLOAD_H
