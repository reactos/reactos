//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wtoride.h
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  History:    28-Jul-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef WTORIDE_H
#define WTORIDE_H

#ifdef __cplusplus
extern "C" 
{
#endif

//
//  override wintrust data with both more than and less than
//

typedef struct _WINTRUST_DATA_ORLESS
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_DATA)
    LPVOID          pPolicyCallbackData;        // optional: used to pass data between the app and policy
    LPVOID          pSIPClientData;             // optional: used to pass data between the app and SIP.
    DWORD           dwUIChoice;                 // required: UI choice.  One of the following.
    DWORD           fdwRevocationChecks;        // required: certificate revocation check options
    DWORD           dwUnionChoice;              // required: which structure is being passed in?
    union
    {
        struct WINTRUST_FILE_INFO_      *pFile;         // individual file
        struct WINTRUST_CATALOG_INFO_   *pCatalog;      // member of a Catalog File
        struct WINTRUST_BLOB_INFO_      *pBlob;         // memory blob
        struct WINTRUST_SGNR_INFO_      *pSgnr;         // signer structure only
        struct WINTRUST_CERT_INFO_      *pCert;
    };

} WINTRUST_DATA_ORLESS, *PWINTRUST_DATA_ORLESS;

typedef struct WINTRUST_FILE_INFO_ORLESS_
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_FILE_INFO)
    LPCWSTR         pcwszFilePath;              // required, file name to be verified

} WINTRUST_FILE_INFO_ORLESS, *PWINTRUST_FILE_INFO_ORLESS;



typedef struct _WINTRUST_DATA_ORMORE
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_DATA)
    LPVOID          pPolicyCallbackData;        // optional: used to pass data between the app and policy
    LPVOID          pSIPClientData;             // optional: used to pass data between the app and SIP.
    DWORD           dwUIChoice;                 // required: UI choice.  One of the following.
    DWORD           fdwRevocationChecks;        // required: certificate revocation check options
    DWORD           dwUnionChoice;              // required: which structure is being passed in?
    union
    {
        struct WINTRUST_FILE_INFO_      *pFile;         // individual file
        struct WINTRUST_CATALOG_INFO_   *pCatalog;      // member of a Catalog File
        struct WINTRUST_BLOB_INFO_      *pBlob;         // memory blob
        struct WINTRUST_SGNR_INFO_      *pSgnr;         // signer structure only
        struct WINTRUST_CERT_INFO_      *pCert;
    };
    DWORD           dwStateAction;                      // optional
    HANDLE          hWVTStateData;                      // optional
    WCHAR           *pwszURLReference;          // optional: currently used to determine zone.

    DWORD           dwExtra[40];

} WINTRUST_DATA_ORMORE, *PWINTRUST_DATA_ORMORE;


typedef struct WINTRUST_FILE_INFO_OR_
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_FILE_INFO)
    LPCWSTR         pcwszFilePath;              // required, file name to be verified
    HANDLE          hFile;                      // optional, open handle to pcwszFilePath

    DWORD           dwExtra[20];
      
} WINTRUST_FILE_INFO_OR, *PWINTRUST_FILE_INFO_OR;


typedef struct _CRYPT_PROVIDER_FUNCTIONS_ORMORE
{
    DWORD                               cbStruct;

    PFN_CPD_MEM_ALLOC                   pfnAlloc;               // set in WVT
    PFN_CPD_MEM_FREE                    pfnFree;                // set in WVT

    PFN_CPD_ADD_STORE                   pfnAddStore2Chain;      // call to add a store to the chain.
    PFN_CPD_ADD_SGNR                    pfnAddSgnr2Chain;       // call to add a sgnr struct to a msg struct sgnr chain
    PFN_CPD_ADD_CERT                    pfnAddCert2Chain;       // call to add a cert struct to a sgnr struct cert chain
    PFN_CPD_ADD_PRIVDATA                pfnAddPrivData2Chain;   // call to add provider private data to struct.

    PFN_PROVIDER_INIT_CALL              pfnInitialize;          // initialize Policy data.
    PFN_PROVIDER_OBJTRUST_CALL          pfnObjectTrust;         // build info up to the signer info(s).
    PFN_PROVIDER_SIGTRUST_CALL          pfnSignatureTrust;      // build info to the signing cert
    PFN_PROVIDER_CERTTRUST_CALL         pfnCertificateTrust;    // build the chain
    PFN_PROVIDER_FINALPOLICY_CALL       pfnFinalPolicy;         // final call to policy
    PFN_PROVIDER_CERTCHKPOLICY_CALL     pfnCertCheckPolicy;     // check each cert will building chain
    PFN_PROVIDER_TESTFINALPOLICY_CALL   pfnTestFinalPolicy;     // dump structures to a file (or whatever the policy chooses)

    struct _CRYPT_PROVUI_FUNCS          *psUIpfns;

                    // the following was added on 7/23/1997: pberkman
    PFN_PROVIDER_CLEANUP_CALL           pfnCleanupPolicy;       // PRIVDATA cleanup routine.

    DWORD                               dwExtra[40];

} CRYPT_PROVIDER_FUNCTIONS_ORMORE, *PCRYPT_PROVIDER_FUNCTIONS_ORMORE;

typedef struct _CRYPT_PROVIDER_FUNCTIONS_ORLESS
{
    DWORD                               cbStruct;

    PFN_CPD_MEM_ALLOC                   pfnAlloc;               // set in WVT
    PFN_CPD_MEM_FREE                    pfnFree;                // set in WVT

    PFN_CPD_ADD_STORE                   pfnAddStore2Chain;      // call to add a store to the chain.
    PFN_CPD_ADD_SGNR                    pfnAddSgnr2Chain;       // call to add a sgnr struct to a msg struct sgnr chain
    PFN_CPD_ADD_CERT                    pfnAddCert2Chain;       // call to add a cert struct to a sgnr struct cert chain
    PFN_CPD_ADD_PRIVDATA                pfnAddPrivData2Chain;   // call to add provider private data to struct.

    PFN_PROVIDER_INIT_CALL              pfnInitialize;          // initialize Policy data.
    PFN_PROVIDER_OBJTRUST_CALL          pfnObjectTrust;         // build info up to the signer info(s).
    PFN_PROVIDER_SIGTRUST_CALL          pfnSignatureTrust;      // build info to the signing cert
    PFN_PROVIDER_CERTTRUST_CALL         pfnCertificateTrust;    // build the chain
    PFN_PROVIDER_FINALPOLICY_CALL       pfnFinalPolicy;         // final call to policy
    PFN_PROVIDER_CERTCHKPOLICY_CALL     pfnCertCheckPolicy;     // check each cert will building chain
    PFN_PROVIDER_TESTFINALPOLICY_CALL   pfnTestFinalPolicy;     // dump structures to a file (or whatever the policy chooses)

} CRYPT_PROVIDER_FUNCTIONS_ORLESS, *PCRYPT_PROVIDER_FUNCTIONS_ORLESS;




typedef struct _CRYPT_PROVIDER_CERT_ORMORE
{
    DWORD                               cbStruct;
                                        
    PCCERT_CONTEXT                      pCert;              // must have its own ref-count!
                                        
    BOOL                                fCommercial;
    BOOL                                fTrustedRoot;       // certchk policy should set this.
    BOOL                                fSelfSigned;        // set in cert provider
                                        
    BOOL                                fTestCert;          // certchk policy will set
                                        
    DWORD                               dwRevokedReason;
                                        
    DWORD                               dwConfidence;       // set in the Certificate Provider
                                        
    DWORD                               dwError;

    CTL_CONTEXT                         *pTrustListContext;

    DWORD                               dwExtra[40];

} CRYPT_PROVIDER_CERT_ORMORE, *PCRYPT_PROVIDER_CERT_ORMORE;

typedef struct _CRYPT_PROVIDER_CERT_ORLESS
{
    DWORD                               cbStruct;
                                        
    PCCERT_CONTEXT                      pCert;              // must have its own ref-count!
                                        
    BOOL                                fCommercial;
    BOOL                                fTrustedRoot;       // certchk policy should set this.
    BOOL                                fSelfSigned;        // set in cert provider
                                        
    BOOL                                fTestCert;          // certchk policy will set
                                        
    DWORD                               dwRevokedReason;
                                        
    DWORD                               dwConfidence;       // set in the Certificate Provider
                                        
    DWORD                               dwError;

} CRYPT_PROVIDER_CERT_ORLESS, *PCRYPT_PROVIDER_CERT_ORLESS;



#ifdef __cplusplus
}
#endif

#endif // WTORIDE_H
