//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       locals.h
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//
//  History:    28-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef LOCALS_H
#define LOCALS_H

#ifdef __cplusplus
extern "C" 
{
#endif



#define     MY_NAME             "WINTRUST.DLL"
#define     W_MY_NAME           L"WINTRUST.DLL"


//
//  dllmain.cpp
//
extern HANDLE       hMeDLL;

extern LIST_LOCK    sProvLock;
extern LIST_LOCK    sStoreLock;

extern HANDLE       hStoreEvent;



//
//  memory.cpp
//
extern void         *WVTNew(DWORD cbSize);
extern void         WVTDelete(void *pvMem);
extern BOOL         WVTAddStore(CRYPT_PROVIDER_DATA *pProvData, HCERTSTORE hStore);
extern BOOL         WVTAddSigner(CRYPT_PROVIDER_DATA *pProvData, 
                                 BOOL fCounterSigner,
                                 DWORD idxSigner,
                                 CRYPT_PROVIDER_SGNR *pSngr2Add);
extern BOOL         WVTAddCertContext(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, 
                                      BOOL fCounterSigner, DWORD idxCounterSigner, 
                                      PCCERT_CONTEXT pCert);
extern BOOL         WVTAddPrivateData(CRYPT_PROVIDER_DATA *pProvData, 
                                      CRYPT_PROVIDER_PRIVDATA *pPrivData2Add);

//
//  registry.cpp
//
extern BOOL         GetRegProvider(GUID *pgActionID, WCHAR *pwszRegKey, 
                                   WCHAR *pwszRetDLLName, char *pszRetFuncName);
extern BOOL         SetRegProvider(GUID *pgActionID, WCHAR *pwszRegKey, 
                                   WCHAR *pwszDLLName, WCHAR *pwszFuncName);
extern void         GetRegSecuritySettings(DWORD *pdwState);
extern BOOL         RemoveRegProvider(GUID *pgActionID, WCHAR *pwszRegKey);

//
//  chains.cpp
//
extern BOOL         AddToStoreChain(HCERTSTORE hStore2Add, DWORD *pchStores, 
                                    HCERTSTORE **pphStoreChain);
extern BOOL         AddToCertChain(CRYPT_PROVIDER_CERT *pPCert2Add, DWORD *pcPCerts,
                                   CRYPT_PROVIDER_CERT **ppPCertChain);
extern BOOL         AddToSignerChain(CRYPT_PROVIDER_SGNR *psSgnr2Add, DWORD *pcSgnrs, 
                                     CRYPT_PROVIDER_SGNR **ppSgnrChain);

extern void         DeallocateCertChain(DWORD csPCert, CRYPT_PROVIDER_CERT **pasPCertChain);
extern void         DeallocateStoreChain(DWORD csStore, HCERTSTORE *phStoreChain);

extern BOOL         AllocateNewChain(DWORD cbMember, void *pNewMember, DWORD *pcChain, 
                                     void **ppChain, DWORD cbAssumeSize);
extern BOOL         AllocateNewChainWithErrors(DWORD cbMember, void *pNewMember, DWORD *pcChain, 
                                               void **ppChain, DWORD **ppdwErrors);

//
//  provload.cpp
//
extern LOADED_PROVIDER  *WintrustFindProvider(GUID *pgActionID);

//
//  certtrst.cpp
//
extern HRESULT WINAPI WintrustCertificateTrust(CRYPT_PROVIDER_DATA *pProvData);

//
//  wvtver1.cpp
//
extern LONG         Version1_WinVerifyTrust(HWND hwnd, GUID *ActionID, LPVOID ActionData);


//
//  wthelper.cpp
//
extern void *       WTHelperCertAllocAndDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
                                                     const BYTE *pbEncoded, DWORD cbEncoded,
                                                     DWORD *pcbStructInfo);



#ifdef __cplusplus
}
#endif

#endif // LOCALS_H
