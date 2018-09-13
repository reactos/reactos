//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       locals.h
//
//  Contents:   Microsoft Internet Security 
//
//  History:    09-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef LOCALS_H
#define LOCALS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define OID_REGPATH             L"Software\\Microsoft\\Cryptography\\OID"
#define PROVIDERS_REGPATH       L"Software\\Microsoft\\Cryptography\\Providers\\Trust"
#define SERVICES_REGPATH        L"Software\\Microsoft\\Cryptography\\Services"
#define SYSTEM_STORE_REGPATH    L"Software\\Microsoft\\SystemCertificates"
#define GROUP_POLICY_STORE_REGPATH  L"Software\\Policies\\Microsoft\\SystemCertificates"
#define ENTERPRISE_STORE_REGPATH L"Software\\Microsoft\\EnterpriseCertificates"

#define ROOT_STORE_REGPATH      L"Software\\Microsoft\\SystemCertificates\\Root"

//
//  initpki.cpp
//
extern HMODULE      hModule;

extern BOOL WINAPI InitializePKI(void);
extern HRESULT RegisterCryptoDlls(BOOL fSetFlags);
extern HRESULT UnregisterCryptoDlls(void);

//
//  initacl.cpp
//
extern BOOL InitializeHKLMAcls();

//
//  pkireg.cpp
//
typedef struct POLSET_
{
    DWORD       dwSetting;
    BOOL        fOn;

} POLSET;

extern POLSET   psPolicySettings[];

extern void CleanupRegistry(void);
extern BOOL _LoadAndRegister(char *pszDll, BOOL fUnregister);
extern BOOL _AdjustPolicyFlags(POLSET *pPolSet);




//
//  mvcerts.cpp
//
extern HRESULT  MoveCertificates(BOOL fDelete);
extern BOOL     TestIE30Store(HKEY hRegRoot, LPCSTR psLoc);
extern HRESULT  TransferIE30Certificates(HKEY hRegRoot, LPCSTR psLoc, HCERTSTORE hStore, BOOL fDelete);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // LOCALS_H
