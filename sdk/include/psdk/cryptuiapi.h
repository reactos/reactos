/*
 * Copyright (C) 2008 Juan Lang
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
#ifndef __CRYPTUIAPI_H__
#define __CRYPTUIAPI_H__

#include <wintrust.h>
#include <wincrypt.h>
#include <prsht.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

BOOL WINAPI CryptUIDlgViewContext(DWORD dwContextType, LPVOID pvContext,
 HWND hwnd, LPCWSTR pwszTitle, DWORD dwFlags, LPVOID pvReserved);

/* Values for dwDontUseColumn */
#define CRYPTUI_SELECT_ISSUEDTO_COLUMN     0x00000001
#define CRYPTUI_SELECT_ISSUEDBY_COLUMN     0x00000002
#define CRYPTUI_SELECT_INTENDEDUSE_COLUMN  0x00000004
#define CRYPTUI_SELECT_FRIENDLYNAME_COLUMN 0x00000008
#define CRYPTUI_SELECT_LOCATION_COLUMN     0x00000010
#define CRYPTUI_SELECT_EXPIRATION_COLUMN   0x00000020

PCCERT_CONTEXT WINAPI CryptUIDlgSelectCertificateFromStore(
 HCERTSTORE hCertStore, HWND hwnd, LPCWSTR pwszTitle, LPCWSTR pwszDisplayString,
 DWORD dwDontUseColumn, DWORD dwFlags, LPVOID pvReserved);

/* Values for CRYPTUI_CERT_MGR_STRUCT's dwFlags */
#define CRYPTUI_CERT_MGR_PUBLISHER_TAB   0x00000004
#define CRYPTUI_CERT_MGR_TAB_MASK        0x0000000f
#define CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG 0x00008000

typedef struct _CRYPTUI_CERT_MGR_STRUCT
{
    DWORD   dwSize;
    HWND    hwndParent;
    DWORD   dwFlags;
    LPCWSTR pwszTitle;
    LPCSTR  pszInitUsageOID;
} CRYPTUI_CERT_MGR_STRUCT, *PCRYPTUI_CERT_MGR_STRUCT;
typedef const struct _CRYPTUI_CERT_MGR_STRUCT *PCCRYPTUI_CERT_MGR_STRUCT;

BOOL WINAPI CryptUIDlgCertMgr(PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr);

typedef BOOL (WINAPI *PFNCFILTERPROC)(PCCERT_CONTEXT pCertContext,
 BOOL *pfInitialSelectedCert, void *pvCallbackData);

typedef struct tagCRYPTUI_INITDIALOG_STRUCT
{
    LPARAM         lParam;
    PCCERT_CONTEXT pCertContext;
} CRYPTUI_INITDIALOG_STRUCT, *PCRYPTUI_INITDIALOG_STRUCT;

/* Values for CRYPTUI_VIEWCERTIFICATE_STRUCT's dwFlags */
#define CRYPTUI_HIDE_HIERARCHYPAGE               0x00000001
#define CRYPTUI_HIDE_DETAILPAGE                  0x00000002
#define CRYPTUI_DISABLE_EDITPROPERTIES           0x00000004
#define CRYPTUI_ENABLE_EDITPROPERTIES            0x00000008
#define CRYPTUI_DISABLE_ADDTOSTORE               0x00000010
#define CRYPTUI_ENABLE_ADDTOSTORE                0x00000020
#define CRYPTUI_ACCEPT_DECLINE_STYLE             0x00000040
#define CRYPTUI_IGNORE_UNTRUSTED_ROOT            0x00000080
#define CRYPTUI_DONT_OPEN_STORES                 0x00000100
#define CRYPTUI_ONLY_OPEN_ROOT_STORE             0x00000200
#define CRYPTUI_WARN_UNTRUSTED_ROOT              0x00000400
#define CRYPTUI_ENABLE_REVOCATION_CHECKING       0x00000800
#define CRYPTUI_WARN_REMOTE_TRUST                0x00001000
#define CRYPTUI_DISABLE_EXPORT                   0x00002000
#define CRYPTUI_ENABLE_REVOCATION_CHECK_END_CERT 0x00004000
#define CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN    0x00008000
#define CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT CRYPTUI_ENABLE_REVOCATION_CHECKING
#define CRYPTUI_DISABLE_HTMLLINK                 0x00010000
#define CRYPTUI_DISABLE_ISSUERSTATEMENT          0x00020000

typedef struct tagCRYPTUI_VIEWCERTIFICATE_STRUCTA
{
    DWORD             dwSize;
    HWND              hwndParent;
    DWORD             dwFlags;
    LPCSTR            szTitle;
    PCCERT_CONTEXT    pCertContext;
    LPCSTR           *rgszPurposes;
    DWORD             cPurposes;
    union {
        CRYPT_PROVIDER_DATA const *pCryptProviderData;
        HANDLE                     hWVTStateData;
    } DUMMYUNIONNAME;
    BOOL              fpCryptProviderDataTrustedUsage;
    DWORD             idxSigner;
    DWORD             idxCert;
    BOOL              fCounterSigner;
    DWORD             idxCounterSigner;
    DWORD             cStores;
    HCERTSTORE       *rghStores;
    DWORD             cPropSheetPages;
    LPCPROPSHEETPAGEA rgPropSheetPages;
    DWORD             nStartPage;
} CRYPTUI_VIEWCERTIFICATE_STRUCTA, *PCRYPTUI_VIEWCERTIFICATE_STRUCTA;
typedef const CRYPTUI_VIEWCERTIFICATE_STRUCTA *PCCRYPTUI_VIEWCERTIFICATE_STRUCTA;

typedef struct tagCRYPTUI_VIEWCERTIFICATE_STRUCTW
{
    DWORD             dwSize;
    HWND              hwndParent;
    DWORD             dwFlags;
    LPCWSTR           szTitle;
    PCCERT_CONTEXT    pCertContext;
    LPCSTR           *rgszPurposes;
    DWORD             cPurposes;
    union {
        CRYPT_PROVIDER_DATA const *pCryptProviderData;
        HANDLE                     hWVTStateData;
    } DUMMYUNIONNAME;
    BOOL              fpCryptProviderDataTrustedUsage;
    DWORD             idxSigner;
    DWORD             idxCert;
    BOOL              fCounterSigner;
    DWORD             idxCounterSigner;
    DWORD             cStores;
    HCERTSTORE       *rghStores;
    DWORD             cPropSheetPages;
    LPCPROPSHEETPAGEW rgPropSheetPages;
    DWORD             nStartPage;
} CRYPTUI_VIEWCERTIFICATE_STRUCTW, *PCRYPTUI_VIEWCERTIFICATE_STRUCTW;
typedef const CRYPTUI_VIEWCERTIFICATE_STRUCTW *PCCRYPTUI_VIEWCERTIFICATE_STRUCTW;

#define CRYPTUI_VIEWCERTIFICATE_STRUCT   WINELIB_NAME_AW(CRYPTUI_VIEWCERTIFICATE_STRUCT)
#define PCCRYPTUI_VIEWCERTIFICATE_STRUCT WINELIB_NAME_AW(PCCRYPTUI_VIEWCERTIFICATE_STRUCT)

BOOL WINAPI CryptUIDlgViewCertificateA(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTA pCertViewInfo, BOOL *pfPropertiesChanged);
BOOL WINAPI CryptUIDlgViewCertificateW(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo, BOOL *pfPropertiesChanged);
#define CryptUIDlgViewCertificate WINELIB_NAME_AW(CryptUIDlgViewCertificate)

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO
{
    DWORD   dwSize;
    GUID   *pGuidSubject;
    DWORD   cbBlob;
    BYTE   *pbBlob;
    LPCWSTR pwszDisplayName;
} CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO *
 PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO
{
    DWORD          dwSize;
    DWORD          cCertStore;
    HCERTSTORE    *rghCertStore;
    PFNCFILTERPROC pFilterCallback;
    void          *pvCallbackData;
} CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO *
 PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO
{
    DWORD  dwSize;
    LPWSTR pwszPvkFileName;
    LPWSTR pwszProvName;
    DWORD  dwProvType;
} CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO,
 *PCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO *
 PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO
{
    DWORD  dwSize;
    LPWSTR pwszSigningCertFileName;
    DWORD  dwPvkChoice;
    union {
        PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO pPvkFileInfo;
        PCRYPT_KEY_PROV_INFO                    pPvkProvInfo;
    } DUMMYUNIONNAME;
} CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO,
 *PCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *
 PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO
{
    DWORD             dwSize;
    DWORD             dwAttrFlags;
    LPCWSTR           pwszDescription;
    LPCWSTR           pwszMoreInfoLocation;
    LPCSTR            pszHashAlg;
    LPCWSTR           pwszSigningCertDisplayString;
    HCERTSTORE        hAdditionalCertStore;
    PCRYPT_ATTRIBUTES psAuthenticated;
    PCRYPT_ATTRIBUTES psUnauthenticated;
} CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO,
 *PCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO *
 PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_INFO
{
    DWORD   dwSize;
    DWORD   dwSubjectChoice;
    union {
        LPCWSTR                              pwszFileName;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO pSignBlobInfo;
    } DUMMYUNIONNAME1;
    DWORD   dwSigningCertChoice;
    union {
        PCCERT_CONTEXT                           pSigningCertContext;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO    pSigningCertStore;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO pSigningCertPvkInfo;
    } DUMMYUNIONNAME2;
    LPCWSTR pwszTimestampURL;
    DWORD   dwAdditionalCertChoice;
    PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO pSignExtInfo;
} CRYPTUI_WIZ_DIGITAL_SIGN_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_INFO;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO;

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT
{
    DWORD dwSize;
    DWORD cbBlob;
    BYTE *pbBlob;
} CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT, *PCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;
typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;

/* Values for CryptUIWizDigitalSign's dwFlags */
#define CRYPTUI_WIZ_NO_UI                            0x00000001
#define CRYPTUI_WIZ_DIGITAL_SIGN_EXCLUDE_PAGE_HASHES 0x00000002
#define CRYPTUI_WIZ_DIGITAL_SIGN_INCLUDE_PAGE_HASHES 0x00000004

BOOL WINAPI CryptUIWizDigitalSign(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO pDigitalSignInfo,
 PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *ppSignContext);

BOOL WINAPI CryptUIWizFreeDigitalSignContext(
 PCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT pSignContext);

/* Values for CRYPTUI_WIZ_EXPORT_INFO's dwSubjectChoice */
#define CRYPTUI_WIZ_EXPORT_CERT_CONTEXT                 1
#define CRYPTUI_WIZ_EXPORT_CTL_CONTEXT                  2
#define CRYPTUI_WIZ_EXPORT_CRL_CONTEXT                  3
#define CRYPTUI_WIZ_EXPORT_CERT_STORE                   4
#define CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY 5
#define CRYPTUI_WIZ_EXPORT_FORMAT_CRL                   6
#define CRYPTUI_WIZ_EXPORT_FORMAT_CTL                   7

typedef struct _CRYPTUI_WIZ_EXPORT_INFO
{
    DWORD       dwSize;
    LPCWSTR     pwszExportFileName;
    DWORD       dwSubjectChoice;
    union {
        PCCERT_CONTEXT pCertContext;
        PCCTL_CONTEXT  pCTLContext;
        PCCRL_CONTEXT  pCRLContext;
        HCERTSTORE     hCertStore;
    } DUMMYUNIONNAME;
    DWORD       cStores;
    HCERTSTORE *rghStores;
} CRYPTUI_WIZ_EXPORT_INFO, *PCRYPTUI_WIZ_EXPORT_INFO;
typedef const CRYPTUI_WIZ_EXPORT_INFO *PCCRYPTUI_WIZ_EXPORT_INFO;

/* Values for CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO's dwExportFormat */
#define CRYPTUI_WIZ_EXPORT_FORMAT_DER                   1
#define CRYPTUI_WIZ_EXPORT_FORMAT_PFX                   2
#define CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7                 3
#define CRYPTUI_WIZ_EXPORT_FORMAT_BASE64                4
#define CRYPTUI_WIZ_EXPORT_FORMAT_SERIALIZED_CERT_STORE 5

typedef struct _CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO
{
    DWORD   dwSize;
    DWORD   dwExportFormat;
    BOOL    fExportChain;
    BOOL    fExportPrivateKeys;
    LPCWSTR pwszPassword;
    BOOL    fStrongEncryption;
} CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO, *PCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO;
typedef const CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO *
 PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO;

BOOL WINAPI CryptUIWizExport(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo, void *pvoid);

/* Values for CRYPTUI_WIZ_IMPORT_SRC_INFO's dwSubjectChoice */
#define CRYPTUI_WIZ_IMPORT_SUBJECT_FILE         1
#define CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT 2
#define CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT  3
#define CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT  4
#define CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE   5

typedef struct _CRYPTUI_WIZ_IMPORT_SUBJECT_INFO
{
    DWORD   dwSize;
    DWORD   dwSubjectChoice;
    union {
        LPCWSTR        pwszFileName;
        PCCERT_CONTEXT pCertContext;
        PCCTL_CONTEXT  pCTLContext;
        PCCRL_CONTEXT  pCRLContext;
        HCERTSTORE     hCertStore;
    } DUMMYUNIONNAME;
    DWORD   dwFlags;
    LPCWSTR pwszPassword;
} CRYPTUI_WIZ_IMPORT_SRC_INFO, *PCRYPTUI_WIZ_IMPORT_SRC_INFO;
typedef const CRYPTUI_WIZ_IMPORT_SRC_INFO *PCCRYPTUI_WIZ_IMPORT_SRC_INFO;

/* Values for CryptUIWizImport's dwFlags */
#define CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE 0x00010000
#define CRYPTUI_WIZ_IMPORT_ALLOW_CERT           0x00020000
#define CRYPTUI_WIZ_IMPORT_ALLOW_CRL            0x00040000
#define CRYPTUI_WIZ_IMPORT_ALLOW_CTL            0x00080000
#define CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE      0x00100000
#define CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER       0x00200000
#define CRYPTUI_WIZ_IMPORT_REMOTE_DEST_STORE    0x00400000

BOOL WINAPI CryptUIWizImport(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc,
 HCERTSTORE hDestCertStore);

/* Definitions missing from PSDK's cryptuiapi.h, but documented on MSDN. */
typedef BOOL (WINAPI *PFNCCERTDISPLAYPROC)(PCCERT_CONTEXT pCertContext,
 HWND hWndSelCertDlg, void *pvCallbackData);

/* Values for CRYPTUI_SELECTCERTIFICATE_STRUCT's dwFlags */
#define CRYPTUI_SELECTCERT_MULTISELECT 0x00000001

typedef struct _CRYPTUI_SELECTCERTIFICATE_STRUCTA
{
    DWORD               dwSize;
    HWND                hwndParent;
    DWORD               dwFlags;
    LPCSTR              szTitle;
    DWORD               dwDontUseColumn;
    LPCSTR              szDisplayString;
    PFNCFILTERPROC      pFilterCallback;
    PFNCCERTDISPLAYPROC pDisplayCallback;
    void               *pvCallbackData;
    DWORD               cDisplayStores;
    HCERTSTORE         *rghDisplayStores;
    DWORD               cStores;
    HCERTSTORE         *rghStores;
    DWORD               cPropSheetPages;
    LPCPROPSHEETPAGEA   rgPropSheetPages;
    HCERTSTORE          hSelectedCertStore;
} CRYPTUI_SELECTCERTIFICATE_STRUCTA, *PCRYPTUI_SELECTCERTIFICATE_STRUCTA;
typedef const CRYPTUI_SELECTCERTIFICATE_STRUCTA *
 PCCRYPTUI_SELECTCERTIFICATE_STRUCTA;

typedef struct _CRYPTUI_SELECTCERTIFICATE_STRUCTW
{
    DWORD               dwSize;
    HWND                hwndParent;
    DWORD               dwFlags;
    LPCWSTR             szTitle;
    DWORD               dwDontUseColumn;
    LPCWSTR             szDisplayString;
    PFNCFILTERPROC      pFilterCallback;
    PFNCCERTDISPLAYPROC pDisplayCallback;
    void               *pvCallbackData;
    DWORD               cDisplayStores;
    HCERTSTORE         *rghDisplayStores;
    DWORD               cStores;
    HCERTSTORE         *rghStores;
    DWORD               cPropSheetPages;
    LPCPROPSHEETPAGEW   rgPropSheetPages;
    HCERTSTORE          hSelectedCertStore;
} CRYPTUI_SELECTCERTIFICATE_STRUCTW, *PCRYPTUI_SELECTCERTIFICATE_STRUCTW;
typedef const CRYPTUI_SELECTCERTIFICATE_STRUCTW *
 PCCRYPTUI_SELECTCERTIFICATE_STRUCTW;

PCCERT_CONTEXT WINAPI CryptUIDlgSelectCertificateA(
 PCCRYPTUI_SELECTCERTIFICATE_STRUCTA pcsc);
PCCERT_CONTEXT WINAPI CryptUIDlgSelectCertificateW(
 PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc);

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCTA
{
    DWORD             dwSize;
    HWND              hwndParent;
    DWORD             dwFlags;
    LPCSTR            szTitle;
    CMSG_SIGNER_INFO *pSignerInfo;
    HCRYPTMSG         hMsg;
    LPCSTR            pszOID;
    DWORD_PTR         dwReserved;
    DWORD             cStores;
    HCERTSTORE       *rghStores;
    DWORD             cPropSheetPages;
    LPCPROPSHEETPAGEA rgPropSheetPages;
} CRYPTUI_VIEWSIGNERINFO_STRUCTA, *PCRYPTUI_VIEWSIGNERINFO_STRUCTA;

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCTW
{
    DWORD             dwSize;
    HWND              hwndParent;
    DWORD             dwFlags;
    LPCWSTR           szTitle;
    CMSG_SIGNER_INFO *pSignerInfo;
    HCRYPTMSG         hMsg;
    LPCSTR            pszOID;
    DWORD_PTR         dwReserved;
    DWORD             cStores;
    HCERTSTORE       *rghStores;
    DWORD             cPropSheetPages;
    LPCPROPSHEETPAGEW rgPropSheetPages;
} CRYPTUI_VIEWSIGNERINFO_STRUCTW, *PCRYPTUI_VIEWSIGNERINFO_STRUCTW;

BOOL WINAPI CryptUIDlgViewSignerInfoA(CRYPTUI_VIEWSIGNERINFO_STRUCTA *pcvsi);
BOOL WINAPI CryptUIDlgViewSignerInfoW(CRYPTUI_VIEWSIGNERINFO_STRUCTW *pcvsi);

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif
