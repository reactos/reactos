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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#define WTCI_DONT_OPEN_STORES 0x00000001
#define WTCI_OPEN_ONLY_ROOT   0x00000002

/* dwUIChoice */
#define WTD_UI_ALL                1
#define WTD_UI_NONE               2
#define WTD_UI_NOBAD              3
#define WTD_UI_NOGOOD             4
/* fdwRevocationChecks */
#define WTD_REVOKE_NONE           0
#define WTD_REVOKE_WHOLECHAIN     1
/* dwUnionChoice */
#define WTD_CHOICE_FILE           1
#define WTD_CHOICE_CATALOG        2
#define WTD_CHOICE_BLOB           3
#define WTD_CHOICE_SIGNER         4
#define WTD_CHOICE_CERT           5

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

#define WTD_STATEACTION_IGNORE           0
#define WTD_STATEACTION_VERIFY           1
#define WTD_STATEACTION_CLOSE            2
#define WTD_STATEACTION_AUTO_CACHE       3
#define WTD_STATEACTION_AUTO_CACHE_FLUSH 4

#define WTD_PROV_FLAGS_MASK                     0x0000ffff
#define WTD_USE_IE4_TRUST_FLAG                  0x00000001
#define WTD_NO_IE4_CHAIN_FLAG                   0x00000002
#define WTD_NO_POLICY_USAGE_FLAG                0x00000004
#define WTD_REVOCATION_CHECK_NONE               0x00000010
#define WTD_REVOCATION_CHECK_END_CERT           0x00000020
#define WTD_REVOCATION_CHECK_CHAIN              0x00000040
#define WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x00000080
#define WTD_SAFER_FLAG                          0x00000100
#define WTD_HASH_ONLY_FLAG                      0x00000200
#define WTD_USE_DEFAULT_OSVER_CHECK             0x00000400
#define WTD_LIFETIME_SIGNING_FLAG               0x00000800

#define WTD_UICONTEXT_EXECUTE 0
#define WTD_UICONTEXT_INSTALL 1

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

typedef struct _CRYPT_PROVIDER_REGDEFUSAGE
{
    DWORD cbStruct;
    GUID  *pgActionID;
    WCHAR *pwszDllName;
    char  *pwszLoadCallbackDataFunctionName;
    char  *pwszFreeCallbackDataFunctionName;
} CRYPT_PROVIDER_REGDEFUSAGE, *PCRYPT_PROVIDER_REGDEFUSAGE;

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

#define WTPF_TRUSTTEST            0x00000020
#define WTPF_TESTCANBEVALID       0x00000080
#define WTPF_IGNOREEXPIRATION     0x00000100
#define WTPF_IGNOREREVOKATION     0x00000200
#define WTPF_OFFLINEOK_IND        0x00000400
#define WTPF_OFFLINEOK_COM        0x00000800
#define WTPF_OFFLINEOKNBU_IND     0x00001000
#define WTPF_OFFLINEOKNBU_COM     0x00002000
#define WTPF_VERIFY_V1_OFF        0x00010000
#define WTPF_IGNOREREVOCATIONONTS 0x00020000
#define WTPF_ALLOWONLYPERTRUST    0x00040000

#define WT_ADD_ACTION_ID_RET_RESULT_FLAG 1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
#define WT_PROVIDER_CERTTRUST_FUNCTION (const WCHAR []) \
    {'W','i','n','t','r','u','s','t','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0}
#elif defined(_MSC_VER)
#define WT_PROVIDER_CERTTRUST_FUNCTION L"WintrustCertificateTrust"
#else
static const WCHAR WT_PROVIDER_CERTTRUST_FUNCTION[] =
    {'W','i','n','t','r','u','s','t','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0};
#endif

BOOL      WINAPI WintrustAddActionID(GUID*,DWORD,CRYPT_REGISTER_ACTIONID*);
BOOL      WINAPI WintrustRemoveActionID(GUID*);
BOOL      WINAPI WintrustLoadFunctionPointers(GUID*,CRYPT_PROVIDER_FUNCTIONS*);
BOOL      WINAPI WintrustAddDefaultForUsage(const char*,CRYPT_PROVIDER_REGDEFUSAGE*);
void      WINAPI WintrustGetRegPolicyFlags(DWORD*);
LONG      WINAPI WinVerifyTrust(HWND,GUID*,LPVOID);
HRESULT   WINAPI WinVerifyTrustEx(HWND,GUID*,WINTRUST_DATA*);

CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(
 CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner,
 DWORD idxCounterSigner);
CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData);

#define SPC_INDIRECT_DATA_OBJID      "1.3.6.1.4.1.311.2.1.4"
#define SPC_SP_AGENCY_INFO_OBJID     "1.3.6.1.4.1.311.2.1.10"
#define SPC_STATEMENT_TYPE_OBJID     "1.3.6.1.4.1.311.2.1.11"
#define SPC_SP_OPUS_INFO_OBJID       "1.3.6.1.4.1.311.2.1.12"
#define SPC_PE_IMAGE_DATA_OBJID      "1.3.6.1.4.1.311.2.1.15"
#define SPC_JAVA_CLASS_DATA_OBJID    "1.3.6.1.4.1.311.2.1.20"
#define SPC_CAB_DATA_OBJID           "1.3.6.1.4.1.311.2.1.25"
#define SPC_MINIMAL_CRITERIA_OBJID   "1.3.6.1.4.1.311.2.1.26"
#define SPC_FINANCIAL_CRITERIA_OBJID "1.3.6.1.4.1.311.2.1.27"
#define SPC_LINK_OBJID               "1.3.6.1.4.1.311.2.1.28"
#define SPC_SIGINFO_OBJID            "1.3.6.1.4.1.311.2.1.30"
#define CAT_NAMEVALUE_OBJID          "1.3.6.1.4.1.311.12.2.1"
#define CAT_MEMBERINFO_OBJID         "1.3.6.1.4.1.311.12.2.2"

#define SPC_SP_AGENCY_INFO_STRUCT        ((LPCSTR) 2000)
#define SPC_MINIMAL_CRITERIA_STRUCT      ((LPCSTR) 2001)
#define SPC_FINANCIAL_CRITERIA_STRUCT    ((LPCSTR) 2002)
#define SPC_INDIRECT_DATA_CONTENT_STRUCT ((LPCSTR) 2003)
#define SPC_PE_IMAGE_DATA_STRUCT         ((LPCSTR) 2004)
#define SPC_LINK_STRUCT                  ((LPCSTR) 2005)
#define SPC_STATEMENT_TYPE_STRUCT        ((LPCSTR) 2006)
#define SPC_SP_OPUS_INFO_STRUCT          ((LPCSTR) 2007)
#define SPC_CAB_DATA_STRUCT              ((LPCSTR) 2008)
#define SPC_JAVA_CLASS_DATA_STRUCT       ((LPCSTR) 2009)
#define SPC_SIGINFO_STRUCT               ((LPCSTR) 2130)
#define CAT_NAMEVALUE_STRUCT             ((LPCSTR) 2221)
#define CAT_MEMBERINFO_STRUCT            ((LPCSTR) 2222)

#define WIN_SPUB_ACTION_PUBLISHED_SOFTWARE \
     { 0x64b9d180, 0x8da2, 0x11cf, { 0x87,0x36,0x00,0xaa,0x00,0xa4,0x85,0xeb }}

#ifdef __cplusplus
}
#endif

#endif
