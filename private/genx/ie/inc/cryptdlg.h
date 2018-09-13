//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cryptdlg.h
//
//  Contents:   Common Cryptographic Dialog API Prototypes and Definitions
//
//----------------------------------------------------------------------------

#ifndef __CRYPTDLG_H__
#define __CRYPTDLG_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef _CRYPTDLG_
#define CRYPTDLGAPI
#else
#define CRYPTDLGAPI DECLSPEC_IMPORT
#endif

#if (_WIN32_WINNT >= 0x0400) || defined(_MAC) || defined(WIN16)

#include <prsht.h>

#ifdef __cplusplus
extern "C" {
#endif

// Master flags to control how revocation is managed

#define CRYTPDLG_FLAGS_MASK                 0xff000000
#define CRYPTDLG_REVOCATION_DEFAULT         0x00000000
#define CRYPTDLG_REVOCATION_ONLINE          0x80000000
#define CRYPTDLG_REVOCATION_CACHE           0x40000000
#define CRYPTDLG_REVOCATION_NONE            0x20000000


// Policy flags which control how we deal with user's certificates

#define CRYPTDLG_POLICY_MASK                0x0000FFFF
#define POLICY_IGNORE_NON_CRITICAL_BC       0x00000001

#define CRYPTDLG_ACTION_MASK                0xFFFF0000
#define ACTION_REVOCATION_DEFAULT_ONLINE    0x00010000
#define ACTION_REVOCATION_DEFAULT_CACHE     0x00020000

//
//  Many of the common dialogs can be passed a filter proc to reduce
//      the set of certificates displayed.  A generic filter proc has been
//      provided to cover many of the generic cases.
//  Return TRUE to display and FALSE to hide

typedef BOOL (WINAPI * PFNCMFILTERPROC)(
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD,   // lCustData, a cookie
        IN DWORD,   // dwFlags
        IN DWORD);  // dwDisplayWell

//  Display Well Values
#define CERT_DISPWELL_SELECT                    1
#define CERT_DISPWELL_TRUST_CA_CERT             2
#define CERT_DISPWELL_TRUST_LEAF_CERT           3
#define CERT_DISPWELL_TRUST_ADD_CA_CERT         4
#define CERT_DISPWELL_TRUST_ADD_LEAF_CERT       5
#define CERT_DISPWELL_DISTRUST_CA_CERT          6
#define CERT_DISPWELL_DISTRUST_LEAF_CERT        7
#define CERT_DISPWELL_DISTRUST_ADD_CA_CERT      8
#define CERT_DISPWELL_DISTRUST_ADD_LEAF_CERT    9

//
typedef UINT (WINAPI * PFNCMHOOKPROC)(
        IN HWND hwndDialog,
        IN UINT message,
        IN WPARAM wParam,
        IN LPARAM lParam);

//
#define CSS_SELECTCERT_MASK             0x00ffffff
#define CSS_HIDE_PROPERTIES             0x00000001
#define CSS_ENABLEHOOK                  0x00000002
#define CSS_ALLOWMULTISELECT            0x00000004
#define CSS_SHOW_HELP                   0x00000010
#define CSS_ENABLETEMPLATE              0x00000020
#define CSS_ENABLETEMPLATEHANDLE        0x00000040

#define SELCERT_OK                      IDOK
#define SELCERT_CANCEL                  IDCANCEL
#define SELCERT_PROPERTIES              100
#define SELCERT_FINEPRINT               101
#define SELCERT_CERTLIST                102
#define SELCERT_HELP                    IDHELP
#define SELCERT_ISSUED_TO               103
#define SELCERT_VALIDITY                104
#define SELCERT_ALGORITHM               105
#define SELCERT_SERIAL_NUM              106
#define SELCERT_THUMBPRINT              107

typedef struct tagCSSA {
    DWORD               dwSize;
    HWND                hwndParent;
    HINSTANCE           hInstance;
    LPCSTR              pTemplateName;
    DWORD               dwFlags;
    LPCSTR              szTitle;
    DWORD               cCertStore;
    HCERTSTORE *        arrayCertStore;
    LPCSTR              szPurposeOid;
    DWORD               cCertContext;
    PCCERT_CONTEXT *    arrayCertContext;
    DWORD               lCustData;
    PFNCMHOOKPROC       pfnHook;
    PFNCMFILTERPROC     pfnFilter;
    LPCSTR              szHelpFileName;
    DWORD               dwHelpId;
    HCRYPTPROV          hprov;
} CERT_SELECT_STRUCT_A, *PCERT_SELECT_STRUCT_A;

typedef struct tagCSSW {
    DWORD               dwSize;
    HWND                hwndParent;
    HINSTANCE           hInstance;
    LPCWSTR             pTemplateName;
    DWORD               dwFlags;
    LPCWSTR             szTitle;
    DWORD               cCertStore;
    HCERTSTORE *        arrayCertStore;
    LPCSTR              szPurposeOid;
    DWORD               cCertContext;
    PCCERT_CONTEXT *    arrayCertContext;
    DWORD               lCustData;
    PFNCMHOOKPROC       pfnHook;
    PFNCMFILTERPROC     pfnFilter;
    LPCWSTR             szHelpFileName;
    DWORD               dwHelpId;
    HCRYPTPROV          hprov;
} CERT_SELECT_STRUCT_W, *PCERT_SELECT_STRUCT_W;

#ifdef UNICODE
typedef CERT_SELECT_STRUCT_W CERT_SELECT_STRUCT;
typedef PCERT_SELECT_STRUCT_W PCERT_SELECT_STRUCT;
#else
typedef CERT_SELECT_STRUCT_A CERT_SELECT_STRUCT;
typedef PCERT_SELECT_STRUCT_A PCERT_SELECT_STRUCT;
#endif // UNICODE

CRYPTDLGAPI
BOOL
WINAPI
CertSelectCertificateA(
        IN OUT PCERT_SELECT_STRUCT_A pCertSelectInfo
        );
#ifdef MAC
#define CertSelectCertificate CertSelectCertificateA
#else   // !MAC
CRYPTDLGAPI
BOOL
WINAPI
CertSelectCertificateW(
        IN OUT PCERT_SELECT_STRUCT_W pCertSelectInfo
        );
#ifdef UNICODE
#define CertSelectCertificate CertSelectCertificateW
#else
#define CertSelectCertificate CertSelectCertificateA
#endif
#endif  // MAC

/////////////////////////////////////////////////////////////

#define CM_VIEWFLAGS_MASK       0x00ffffff
#define CM_ENABLEHOOK           0x00000001
#define CM_SHOW_HELP            0x00000002
#define CM_SHOW_HELPICON        0x00000004
#define CM_ENABLETEMPLATE       0x00000008
#define CM_HIDE_ADVANCEPAGE     0x00000010
#define CM_HIDE_TRUSTPAGE       0x00000020
#define CM_NO_NAMECHANGE        0x00000040
#define CM_NO_EDITTRUST         0x00000080
#define CM_HIDE_DETAILPAGE      0x00000100
#define CM_ADD_CERT_STORES      0x00000200
#define CERTVIEW_CRYPTUI_LPARAM 0x00800000

typedef struct tagCERT_VIEWPROPERTIES_STRUCT_A {
    DWORD               dwSize;
    HWND                hwndParent;
    HINSTANCE           hInstance;
    DWORD               dwFlags;
    LPCSTR              szTitle;
    PCCERT_CONTEXT      pCertContext;
    LPSTR *             arrayPurposes;
    DWORD               cArrayPurposes;
    DWORD               cRootStores;    // Count of Root Stores
    HCERTSTORE *        rghstoreRoots;  // Array of root stores
    DWORD               cStores;        // Count of other stores to search
    HCERTSTORE *        rghstoreCAs;    // Array of other stores to search
    DWORD               cTrustStores;   // Count of trust stores
    HCERTSTORE *        rghstoreTrust;  // Array of trust stores
    HCRYPTPROV          hprov;          // Provider to use for verification
    DWORD               lCustData;
    DWORD               dwPad;
    LPCSTR              szHelpFileName;
    DWORD               dwHelpId;
    DWORD               nStartPage;
    DWORD               cArrayPropSheetPages;
    PROPSHEETPAGE *     arrayPropSheetPages;
} CERT_VIEWPROPERTIES_STRUCT_A, *PCERT_VIEWPROPERTIES_STRUCT_A;

typedef struct tagCERT_VIEWPROPERTIES_STRUCT_W {
    DWORD               dwSize;
    HWND                hwndParent;
    HINSTANCE           hInstance;
    DWORD               dwFlags;
    LPCWSTR             szTitle;
    PCCERT_CONTEXT      pCertContext;
    LPSTR *             arrayPurposes;
    DWORD               cArrayPurposes;
    DWORD               cRootStores;    // Count of Root Stores
    HCERTSTORE *        rghstoreRoots;  // Array of root stores
    DWORD               cStores;        // Count of other stores to search
    HCERTSTORE *        rghstoreCAs;    // Array of other stores to search
    DWORD               cTrustStores;   // Count of trust stores
    HCERTSTORE *        rghstoreTrust;  // Array of trust stores
    HCRYPTPROV          hprov;          // Provider to use for verification
    DWORD               lCustData;
    DWORD               dwPad;
    LPCWSTR             szHelpFileName;
    DWORD               dwHelpId;
    DWORD               nStartPage;
    DWORD               cArrayPropSheetPages;
    PROPSHEETPAGE *     arrayPropSheetPages;
} CERT_VIEWPROPERTIES_STRUCT_W, *PCERT_VIEWPROPERTIES_STRUCT_W;

#ifdef UNICODE
typedef CERT_VIEWPROPERTIES_STRUCT_W CERT_VIEWPROPERTIES_STRUCT;
typedef PCERT_VIEWPROPERTIES_STRUCT_W PCERT_VIEWPROPERTIES_STRUCT;
#else
typedef CERT_VIEWPROPERTIES_STRUCT_A CERT_VIEWPROPERTIES_STRUCT;
typedef PCERT_VIEWPROPERTIES_STRUCT_A PCERT_VIEWPROPERTIES_STRUCT;
#endif // UNICODE

CRYPTDLGAPI
BOOL
WINAPI
CertViewPropertiesA(
        PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo
        );
#ifdef MAC
#define CertViewProperties CertViewPropertiesA
#else   // !MAC
CRYPTDLGAPI
BOOL
WINAPI
CertViewPropertiesW(
        PCERT_VIEWPROPERTIES_STRUCT_W pCertViewInfo
        );

#ifdef UNICODE
#define CertViewProperties CertViewPropertiesW
#else
#define CertViewProperties CertViewPropertiesA
#endif
#endif  // MAC


//
//  We provide a default filter function that people can use to do some
//      of the most simple things.
//

#define CERT_FILTER_OP_EXISTS           1
#define CERT_FILTER_OP_NOT_EXISTS       2
#define CERT_FILTER_OP_EQUALITY         3

typedef struct tagCMOID {
    LPCSTR              szExtensionOID;         // Extension to filter on
    DWORD               dwTestOperation;
    LPBYTE              pbTestData;
    DWORD               cbTestData;
} CERT_FILTER_EXTENSION_MATCH;

#define CERT_FILTER_INCLUDE_V1_CERTS    0x0001
#define CERT_FILTER_VALID_TIME_RANGE    0x0002
#define CERT_FILTER_VALID_SIGNATURE     0x0004
#define CERT_FILTER_LEAF_CERTS_ONLY     0x0008
#define CERT_FILTER_ISSUER_CERTS_ONLY   0x0010
#define CERT_FILTER_KEY_EXISTS          0x0020

typedef struct tagCMFLTR {
    DWORD               dwSize;
    DWORD               cExtensionChecks;
    CERT_FILTER_EXTENSION_MATCH * arrayExtensionChecks;
    DWORD               dwCheckingFlags;
} CERT_FILTER_DATA;

//
//  Maybe this should not be here -- but until it goes into wincrypt.h
//

//
//   Get a formatted friendly name for a certificate

CRYPTDLGAPI
DWORD
WINAPI
GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pchBuffer,
                             DWORD cchBuffer);
CRYPTDLGAPI
DWORD
WINAPI
GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pchBuffer,
                              DWORD cchBuffer);
#ifdef UNICODE
#define GetFriendlyNameOfCert GetFriendlyNameOfCertW
#else
#define GetFriendlyNameOfCert GetFriendlyNameOfCertA
#endif


//
//  We also provide a WinTrust provider which performs the same set of
//      parameter checking that we do in order to validate certificates.
//

#define CERT_CERTIFICATE_ACTION_VERIFY  \
  { /* 7801ebd0-cf4b-11d0-851f-0060979387ea */  \
    0x7801ebd0, \
    0xcf4b,     \
    0x11d0,     \
    {0x85, 0x1f, 0x00, 0x60, 0x97, 0x93, 0x87, 0xea} \
  }
#define szCERT_CERTIFICATE_ACTION_VERIFY    \
    "{7801ebd0-cf4b-11d0-851f-0060979387ea}"  

typedef HRESULT (WINAPI * PFNTRUSTHELPER)(
        IN PCCERT_CONTEXT       pCertContext,
        IN DWORD                lCustData,
        IN BOOL                 fLeafCertificate,
        IN LPBYTE               pbTrustBlob);
//
//  Failure Reasons:
//

#define CERT_VALIDITY_BEFORE_START              0x00000001
#define CERT_VALIDITY_AFTER_END                 0x00000002
#define CERT_VALIDITY_SIGNATURE_FAILS           0x00000004
#define CERT_VALIDITY_CERTIFICATE_REVOKED       0x00000008
#define CERT_VALIDITY_KEY_USAGE_EXT_FAILURE     0x00000010
#define CERT_VALIDITY_EXTENDED_USAGE_FAILURE    0x00000020
#define CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE  0x00000040
#define CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION 0x00000080
#define CERT_VALIDITY_ISSUER_INVALID            0x00000100
#define CERT_VALIDITY_OTHER_EXTENSION_FAILURE   0x00000200
#define CERT_VALIDITY_PERIOD_NESTING_FAILURE    0x00000400
#define CERT_VALIDITY_OTHER_ERROR               0x00000800
#define CERT_VALIDITY_ISSUER_DISTRUST           0x02000000
#define CERT_VALIDITY_EXPLICITLY_DISTRUSTED     0x01000000
#define CERT_VALIDITY_NO_ISSUER_CERT_FOUND      0x10000000
#define CERT_VALIDITY_NO_CRL_FOUND              0x20000000
#define CERT_VALIDITY_CRL_OUT_OF_DATE           0x40000000
#define CERT_VALIDITY_NO_TRUST_DATA             0x80000000
#define CERT_VALIDITY_MASK_TRUST                0xffff0000
#define CERT_VALIDITY_MASK_VALIDITY             0x0000ffff

#define CERT_TRUST_MASK                         0x00ffffff
#define CERT_TRUST_DO_FULL_SEARCH               0x00000001
#define CERT_TRUST_PERMIT_MISSING_CRLS          0x00000002
#define CERT_TRUST_DO_FULL_TRUST                0x00000005
#define CERT_TRUST_ADD_CERT_STORES              CM_ADD_CERT_STORES

//
//  Trust data structure
//
//      Returned data arrays will be allocated using LocalAlloc and must
//      be freed by the caller.  The data in the TrustInfo array are individually
//      allocated and must be freed.  The data in rgChain must be freed by
//      calling CertFreeCertificateContext.
//
//  Defaults:
//      pszUsageOid == NULL     indicates that no trust validation should be done
//      cRootStores == 0        Will default to User's Root store
//      cStores == 0            Will default to User's CA and system's SPC stores
//      cTrustStores == 0       Will default to User's TRUST store
//      hprov == NULL           Will default to RSABase
//      any returned item which has a null pointer will not return that item.
//  Notes:
//      pfnTrustHelper is nyi

typedef struct _CERT_VERIFY_CERTIFICATE_TRUST {
    DWORD               cbSize;         // Size of this structure
    PCCERT_CONTEXT      pccert;         // Certificate to be verified
    DWORD               dwFlags;        // CERT_TRUST_*
    DWORD               dwIgnoreErr;    // Errors to ignore (CERT_VALIDITY_*)
    DWORD *             pdwErrors;      // Location to return error flags
    LPSTR               pszUsageOid;    // Extended Usage OID for Certificate
    HCRYPTPROV          hprov;          // Crypt Provider to use for validation
    DWORD               cRootStores;    // Count of Root Stores
    HCERTSTORE *        rghstoreRoots;  // Array of root stores
    DWORD               cStores;        // Count of other stores to search
    HCERTSTORE *        rghstoreCAs;    // Array of other stores to search
    DWORD               cTrustStores;   // Count of trust stores
    HCERTSTORE *        rghstoreTrust;  // Array of trust stores
    DWORD               lCustData;      //
    PFNTRUSTHELPER      pfnTrustHelper; // Callback function for cert validation
    DWORD *             pcChain;        // Count of items in the chain array
    PCCERT_CONTEXT **   prgChain;       // Chain of certificates used
    DWORD **            prgdwErrors;    // Errors on a per certificate basis
    DATA_BLOB **        prgpbTrustInfo; // Array of trust information used
} CERT_VERIFY_CERTIFICATE_TRUST, * PCERT_VERIFY_CERTIFICATE_TRUST;

//
//  Trust list manipulation routine
//
//  CertModifyCertificatesToTrust can be used to do modifications to the set of certificates
//      on trust lists for a given purpose.
//      if hcertstoreTrust is NULL, the System Store TRUST in Current User will be used
//      if pccertSigner is specified, it will be used to sign the resulting trust lists,
//              it also restricts the set of trust lists that may be modified.
//

#define CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED      1
#define CTL_MODIFY_REQUEST_REMOVE               2
#define CTL_MODIFY_REQUEST_ADD_TRUSTED          3

typedef struct _CTL_MODIFY_REQUEST {
    PCCERT_CONTEXT      pccert;         // Certificate to change trust on
    DWORD               dwOperation;    // Operation to be performed
    DWORD               dwError;        // Operation error code
} CTL_MODIFY_REQUEST, * PCTL_MODIFY_REQUEST;

CRYPTDLGAPI
HRESULT
WINAPI
CertModifyCertificatesToTrust(
        int cCerts,                     // Count of modifications to be done
        PCTL_MODIFY_REQUEST rgCerts,    // Array of modification requests
        LPCSTR szPurpose,               // Purpose OID to for modifications
        HWND hwnd,                      // HWND for any dialogs
        HCERTSTORE hcertstoreTrust,     // Cert Store to store trust information in
        PCCERT_CONTEXT pccertSigner);   // Certificate to be used in signing trust list

#ifdef WIN16
// Need to define export functions in WATCOM.
BOOL
WINAPI CertConfigureTrustA(void);

BOOL
WINAPI FormatVerisignExtension(
    DWORD /*dwCertEncodingType*/,
    DWORD /*dwFormatType*/,
    DWORD /*dwFormatStrType*/,
    void * /*pFormatStruct*/,
    LPCSTR /*lpszStructType*/,
    const BYTE * /*pbEncoded*/,
    DWORD /*cbEncoded*/,
    void * pbFormat,
    DWORD * pcbFormat);
#endif // !WIN16

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // (_WIN32_WINNT >= 0x0400)

#endif // _CRYPTDLG_H_
