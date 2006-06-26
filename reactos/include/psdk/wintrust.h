/*
 * Copyright (C) 2004 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINTRUST_H
#define __WINE_WINTRUST_H

#include <wincrypt.h>


#include <pshpack8.h>

typedef struct WINTRUST_FILE_INFO_
{
    DWORD   cbStruct;
    LPCWSTR pcwszFilePath;
    HANDLE  hFile;
    GUID*   pgKnownSubject;
} WINTRUST_FILE_INFO, *PWINTRUST_FILE_INFO;

typedef struct WINTRUST_CATALOG_INFO_
{
    DWORD         cbStruct;
    DWORD         dwCatalogVersion;
    LPCWSTR       pcwszCatalogFilePath;
    LPCWSTR       pcwszMemberTag;
    LPCWSTR       pcwszMemberFilePath;
    HANDLE        hMemberFile;
    BYTE*         pbCalculatedFileHash;
    DWORD         cbCalculatedFileHash;
    PCCTL_CONTEXT pcCatalogContext;
} WINTRUST_CATALOG_INFO, *PWINTRUST_CATALOG_INFO;

typedef struct WINTRUST_BLOB_INFO_
{
    DWORD   cbStruct;
    GUID    gSubject;
    LPCWSTR pcwszDisplayName;
    DWORD   cbMemObject;
    BYTE*   pbMemObject;
    DWORD   cbMemSignedMsg;
    BYTE*   pbMemSignedMsg;
} WINTRUST_BLOB_INFO, *PWINTRUST_BLOB_INFO;

typedef struct WINTRUST_SGNR_INFO_
{
    DWORD             cbStruct;
    LPCWSTR           pcwszDisplayName;
    CMSG_SIGNER_INFO* psSignerInfo;
    DWORD             chStores;
    HCERTSTORE*       pahStores;
} WINTRUST_SGNR_INFO, *PWINTRUST_SGNR_INFO;

typedef struct WINTRUST_CERT_INFO_
{
    DWORD         cbStruct;
    LPCWSTR       pcwszDisplayName;
    CERT_CONTEXT* psCertContext;
    DWORD         chStores;
    HCERTSTORE*   pahStores;
    DWORD         dwFlags;
    FILETIME*     psftVerifyAsOf;
} WINTRUST_CERT_INFO, *PWINTRUST_CERT_INFO;


typedef struct _WINTRUST_DATA
{
    DWORD  cbStruct;
    LPVOID pPolicyCallbackData;
    LPVOID pSIPClientData;
    DWORD  dwUIChoice;
    DWORD  fdwRevocationChecks;
    DWORD  dwUnionChoice;
    union
    {
        struct WINTRUST_FILE_INFO_*    pFile;
        struct WINTRUST_CATALOG_INFO_* pCatalog;
        struct WINTRUST_BLOB_INFO_*    pBlob;
        struct WINTRUST_SGNR_INFO_*    pSgnr;
        struct WINTRUST_CERT_INFO_*    pCert;
    } DUMMYUNIONNAME;

    DWORD  dwStateAction;
    HANDLE hWVTStateData;
    WCHAR* pwszURLReference;
    DWORD  dwProvFlags;
    DWORD  dwUIContext;
} WINTRUST_DATA, *PWINTRUST_DATA;

typedef struct _CRYPT_TRUST_REG_ENTRY
{
    DWORD cbStruct;
    WCHAR *pwszDLLName;
    WCHAR *pwszFunctionName;
} CRYPT_TRUST_REG_ENTRY, *PCRYPT_TRUST_REG_ENTRY;

typedef struct _CRYPT_REGISTER_ACTIONID
{
    DWORD cbStruct;
    CRYPT_TRUST_REG_ENTRY sInitProvider;
    CRYPT_TRUST_REG_ENTRY sObjectProvider;
    CRYPT_TRUST_REG_ENTRY sSignatureProvider;
    CRYPT_TRUST_REG_ENTRY sCertificateProvider;
    CRYPT_TRUST_REG_ENTRY sCertificatePolicyProvider;
    CRYPT_TRUST_REG_ENTRY sFinalPolicyProvider;
    CRYPT_TRUST_REG_ENTRY sTestPolicyProvider;
    CRYPT_TRUST_REG_ENTRY sCleanupProvider;
} CRYPT_REGISTER_ACTIONID, *PCRYPT_REGISTER_ACTIONID;

typedef struct _CRYPT_PROVUI_DATA {
    DWORD cbStruct;
    DWORD dwFinalError;
    WCHAR *pYesButtonText;
    WCHAR *pNoButtonText;
    WCHAR *pMoreInfoButtonText;
    WCHAR *pAdvancedLinkText;
    WCHAR *pCopyActionText;
    WCHAR *pCopyActionTextNoTS;
    WCHAR *pCopyActionTextNotSigned;
} CRYPT_PROVUI_DATA, *PCRYPT_PROVUI_DATA;

typedef struct _CRYPT_PROVIDER_CERT {
    DWORD               cbStruct;
    PCCERT_CONTEXT      pCert;
    BOOL                fCommercial;
    BOOL                fTrustedRoot;
    BOOL                fSelfSigned;
    BOOL                fTestCert;
    DWORD               dwRevokedReason;
    DWORD               dwConfidence;
    DWORD               dwError;
    CTL_CONTEXT        *pTrustListContext;
    BOOL                fTrustListSignerCert;
    PCCTL_CONTEXT       pCtlContext;
    DWORD               dwCtlError;
    BOOL                fIsCyclic;
    PCERT_CHAIN_ELEMENT pChainElement;
} CRYPT_PROVIDER_CERT, *PCRYPT_PROVIDER_CERT;

typedef struct _CRYPT_PROVIDER_SGNR {
    DWORD                cbStruct;
    FILETIME             sftVerifyAsOf;
    DWORD                csCertChain;
    CRYPT_PROVIDER_CERT *pasCertChain;
    DWORD                dwSignerType;
    CMSG_SIGNER_INFO    *psSigner;
    DWORD                dwError;
    DWORD                csCounterSigners;
    struct _CRYPT_PROVIDER_SGNR *pasCounterSigners;
    PCCERT_CHAIN_CONTEXT pChainContext;
} CRYPT_PROVIDER_SGNR, *PCRYPT_PROVIDER_SGNR;

typedef struct _CRYPT_PROVIDER_PRIVDATA {
    DWORD cbStruct;
    GUID  gProviderID;
    DWORD cbProvData;
    void *pvProvData;
} CRYPT_PROVIDER_PRIVDATA, *PCRYPT_PROVIDER_PRIVDATA;

struct _CRYPT_PROVIDER_DATA;

typedef void * (*PFN_CPD_MEM_ALLOC)(DWORD cbSize);
typedef void (*PFN_CPD_MEM_FREE)(void *pvMem2Free);
typedef BOOL (*PFN_CPD_ADD_STORE)(struct _CRYPT_PROVIDER_DATA *pProvData,
 HCERTSTORE hStore2Add);
typedef BOOL (*PFN_CPD_ADD_SGNR)(struct _CRYPT_PROVIDER_DATA *pProvData,
 BOOL fCounterSigner, DWORD idxSigner, struct _CRYPT_PROVIDER_SGNR *pSgnr2Add);
typedef BOOL (*PFN_CPD_ADD_CERT)(struct _CRYPT_PROVIDER_DATA *pProvData,
 DWORD idxSigner, BOOL fCounterSigner, DWORD idxCounterSigner,
 PCCERT_CONTEXT pCert2Add);
typedef BOOL (*PFN_CPD_ADD_PRIVDATA)(struct _CRYPT_PROVIDER_DATA *pProvData,
 struct _CRYPT_PROVIDER_PRIVDATA *pPrivData2Add);
typedef HRESULT (*PFN_PROVIDER_INIT_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_OBJTRUST_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_SIGTRUST_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_CERTTTRUST_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_FINALPOLICY_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_TESTFINALPOLICY_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef HRESULT (*PFN_PROVIDER_CLEANUP_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData);
typedef BOOL (*PFN_PROVIDER_CERTCHKPOLICY_CALL)(
 struct _CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner,
 BOOL fCounterSignerChain, DWORD idxCounterSigner);

typedef struct _CRYPT_PROVIDER_FUNCTIONS {
    DWORD cbStruct;
    PFN_CPD_MEM_ALLOC    pfnAlloc;
    PFN_CPD_MEM_FREE     pfnFree;
    PFN_CPD_ADD_STORE    pfnAddStore2Chain;
    PFN_CPD_ADD_SGNR     pfnAddSgnr2Chain;
    PFN_CPD_ADD_CERT     pfnAddCert2Chain;
    PFN_CPD_ADD_PRIVDATA pfnAddPrivData2Chain;
    PFN_PROVIDER_INIT_CALL            pfnInitialize;
    PFN_PROVIDER_OBJTRUST_CALL        pfnObjectTrust;
    PFN_PROVIDER_SIGTRUST_CALL        pfnSignatureTrust;
    PFN_PROVIDER_CERTTTRUST_CALL      pfnCertificateTrust;
    PFN_PROVIDER_FINALPOLICY_CALL     pfnFinalPolicy;
    PFN_PROVIDER_CERTCHKPOLICY_CALL   pfnCertCheckPolicy;
    PFN_PROVIDER_TESTFINALPOLICY_CALL pfnTestFinalPolicy;
    struct _CRYPT_PROVUI_FUNCS       *psUIpfns;
    PFN_PROVIDER_CLEANUP_CALL         pfnCleanupPolicy;
} CRYPT_PROVIDER_FUNCTIONS, *PCRYPT_PROVIDER_FUNCTIONS;

struct SIP_DISPATCH_INFO_;
struct SIP_SUBJECTINFO_;
struct SIP_INDIRECT_DATA_;

typedef struct _PROVDATA_SIP {
    DWORD cbStruct;
    GUID  gSubject;
    struct SIP_DISPATCH_INFO_ *pSip;
    struct SIP_DISPATCH_INFO_ *pCATSip;
    struct SIP_SUBJECTINFO_   *psSipSubjectInfo;
    struct SIP_SUBJECTINFO_   *psSipCATSubjectInfo;
    struct SIP_INDIRECT_DATA_ *psIndirectData;
} PROVDATA_SIP, *PPROVDATA_SIP;

typedef struct _CRYPT_PROVIDER_DATA {
    DWORD                     cbStruct;
    WINTRUST_DATA            *pWintrustData;
    BOOL                      fOpenedFile;
    HWND                      hWndParent;
    GUID                     *pgActionID;
    HCRYPTPROV                hProv;
    DWORD                     dwError;
    DWORD                     dwRegSecuritySettings;
    DWORD                     dwRegPolicySettings;
    CRYPT_PROVIDER_FUNCTIONS *psPfns;
    DWORD                     cdwTrustStepErrors;
    DWORD                    *padwTrustStepErrors;
    DWORD                     chStores;
    HCERTSTORE               *pahStores;
    DWORD                     dwEncoding;
    HCRYPTMSG                 hMsg;
    DWORD                     csSigners;
    CRYPT_PROVIDER_SGNR      *pasSigners;
    DWORD                     dwSubjectChoice;
    union {
        struct _PROVDATA_SIP        *pPDSip;
    } DUMMYUNIONNAME;
    char                     *pszUsageOID;
    BOOL                      fRecallWithState;
    FILETIME                  sftSystemTime;
    char                      *pszCTLSignerUsageOID;
    DWORD                     dwProvFlags;
    DWORD                     dwFinalError;
    PCERT_USAGE_MATCH         pRequestUsage;
    DWORD                     dwTrustPubSettings;
    DWORD                     dwUIStateFlags;
} CRYPT_PROVIDER_DATA, *PCRYPT_PROVIDER_DATA;

typedef BOOL (*PFN_PROVUI_CALL)(HWND hWndSecurityDialog,
 struct _CRYPT_PROVIDER_DATA *pProvData);

typedef struct _CRYPT_PROVUI_FUNCS {
    DWORD cbStruct;
    CRYPT_PROVUI_DATA psUIData;
    PFN_PROVUI_CALL pfnOnMoreInfoClick;
    PFN_PROVUI_CALL pfnOnMoreInfoClickDefault;
    PFN_PROVUI_CALL pfnOnAdvancedClick;
    PFN_PROVUI_CALL pfnOnAdvancedClickDefault;
} CRYPT_PROVUI_FUNCS, *PCRYPT_PROVUI_FUNCS;

#include <poppack.h>


#ifdef __cplusplus
extern "C" {
#endif

BOOL      WINAPI WintrustAddActionID(GUID*,DWORD,CRYPT_REGISTER_ACTIONID*);
void      WINAPI WintrustGetRegPolicyFlags(DWORD*);
LONG      WINAPI WinVerifyTrust(HWND,GUID*,WINTRUST_DATA*);
HRESULT   WINAPI WinVerifyTrustEx(HWND,GUID*,WINTRUST_DATA*);

CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(
 CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner,
 DWORD idxCounterSigner);
CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData);

#ifdef __cplusplus
}
#endif

#endif
