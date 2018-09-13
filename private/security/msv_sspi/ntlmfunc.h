//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        ntlmfunc.h
//
// Contents:    prototypes for export functions as in secpkg.h
//
//
// History:     ChandanS  26-Jul-96   Stolen from kerberos\client2\kerbfunc.h
//
//------------------------------------------------------------------------

#ifndef __NTLMFUNC_H__
#define __NTLMFUNC_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

SpInitializeFn                  SpInitialize;
//LSA_AP_INITIALIZE_PACKAGE       LsaApInitializePackage;

//
// BUGBUG
NTSTATUS
LsaApInitializePackage(
    ULONG PackageId,
    PLSA_DISPATCH_TABLE Table,
    PSTRING Database,
    PSTRING Confidentiality,
    OUT PSTRING * PackageName);
SpGetInfoFn                     SpGetInfo;
LSA_AP_LOGON_USER_EX2           LsaApLogonUserEx2;

SpAcceptCredentialsFn           SpAcceptCredentials;
SpAcquireCredentialsHandleFn    SpAcquireCredentialsHandle;
SpFreeCredentialsHandleFn       SpFreeCredentialsHandle;
SpQueryCredentialsAttributesFn  SpQueryCredentialsAttributes;
SpSaveCredentialsFn             SpSaveCredentials;
SpGetCredentialsFn              SpGetCredentials;
SpDeleteCredentialsFn           SpDeleteCredentials;

SpInitLsaModeContextFn          SpInitLsaModeContext;
SpDeleteContextFn               SpDeleteContext;
SpAcceptLsaModeContextFn        SpAcceptLsaModeContext;

LSA_AP_LOGON_TERMINATED         LsaApLogonTerminated;
SpApplyControlTokenFn           SpApplyControlToken;
LSA_AP_CALL_PACKAGE             LsaApCallPackage;
LSA_AP_CALL_PACKAGE             LsaApCallPackageUntrusted;
LSA_AP_CALL_PACKAGE_PASSTHROUGH LsaApCallPackagePassthrough;
SpShutdownFn                    SpShutdown;
SpGetUserInfoFn                 SpGetUserInfo;

SpInstanceInitFn                SpInstanceInit;
SpInitUserModeContextFn         SpInitUserModeContext;
SpMakeSignatureFn               SpMakeSignature;
SpVerifySignatureFn             SpVerifySignature;
SpSealMessageFn                 SpSealMessage;
SpUnsealMessageFn               SpUnsealMessage;
SpGetContextTokenFn             SpGetContextToken;
SpQueryContextAttributesFn      SpQueryContextAttributes;
SpDeleteContextFn               SpDeleteUserModeContext;
SpCompleteAuthTokenFn           SpCompleteAuthToken;
SpFormatCredentialsFn           SpFormatCredentials;
SpMarshallSupplementalCredsFn   SpMarshallSupplementalCreds;
SpExportSecurityContextFn       SpExportSecurityContext;
SpImportSecurityContextFn       SpImportSecurityContext;
SpGetExtendedInformationFn      SpGetExtendedInformation ;
SpSetExtendedInformationFn      SpSetExtendedInformation ;
SpQueryCredentialsAttributesFn  SpQueryCredentialsAttributes ;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __NTLMFUNC_H__

