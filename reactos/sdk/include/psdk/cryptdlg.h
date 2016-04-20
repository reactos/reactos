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
#ifndef __CRYPTDLG_H__
#define __CRYPTDLG_H__

#include <prsht.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPTDLG_FLAGS_MASK         0xff000000
#define CRYPTDLG_REVOCATION_DEFAULT 0x00000000
#define CRYPTDLG_REVOCATION_ONLINE  0x80000000
#define CRYPTDLG_REVOCATION_CACHE   0x40000000
#define CRYPTDLG_REVOCATION_NONE    0x20000000

#define CRYPTDLG_POLICY_MASK          0x0000ffff
#define POLICY_IGNORE_NON_CRITICAL_BC 0x00000001

#define CRYPTDLG_ACTION_MASK             0xffff0000
#define ACTION_REVOCATION_DEFAULT_ONLINE 0x00010000
#define ACTION_REVOCATION_DEFAULT_CACHE  0x00020000

typedef BOOL (WINAPI *PFNCMFILTERPROC)(PCCERT_CONTEXT, DWORD, DWORD, DWORD);

#define CERT_DISPWELL_SELECT                 1
#define CERT_DISPWELL_TRUST_CA_CERT          2
#define CERT_DISPWELL_TRUST_LEAF_CERT        3
#define CERT_DISPWELL_TRUST_ADD_CA_CERT      4
#define CERT_DISPWELL_TRUST_ADD_LEAF_CERT    5
#define CERT_DISPWELL_DISTRUST_CA_CERT       6
#define CERT_DISPWELL_DISTRUST_LEAF_CERT     7
#define CERT_DISPWELL_DISTRUST_ADD_CA_CERT   8
#define CERT_DISPWELL_DISTRUST_ADD_LEAF_CERT 9

typedef UINT (WINAPI *PFNCMHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

#define CSS_SELECTCERT_MASK      0x00ffffff
#define CSS_HIDE_PROPERTIES      0x00000001
#define CSS_ENABLEHOOK           0x00000002
#define CSS_ALLOWMULTISELECT     0x00000004
#define CSS_SHOW_HELP            0x00000010
#define CSS_ENABLETEMPLATE       0x00000020
#define CSS_ENABLETEMPLATEHANDLE 0x00000040

#define SELCERT_OK         IDOK
#define SELCERT_CANCEL     IDCANCEL
#define SELCERT_PROPERTIES 100
#define SELCERT_FINEPRINT  101
#define SELCERT_CERTLIST   102
#define SELCERT_HELP       IDHELP
#define SELCERT_ISSUED_TO  103
#define SELCERT_VALIDITY   104
#define SELCERT_ALGORITHM  105
#define SELCERT_SERIAL_NUM 106
#define SELCERT_THUMBPRINT 107

typedef struct tagCSSA
{
    DWORD           dwSize;
    HWND            hwndParent;
    HINSTANCE       hInstance;
    LPCSTR          pTemplateName;
    DWORD           dwFlags;
    LPCSTR          szTitle;
    DWORD           cCertStore;
    HCERTSTORE     *arrayCertStore;
    LPCSTR          szPurposeOid;
    DWORD           cCertContext;
    PCCERT_CONTEXT *arrayCertContext;
    DWORD           lCustData;
    PFNCMHOOKPROC   pfnHook;
    PFNCMFILTERPROC pfnFilter;
    LPCSTR          szHelpFileName;
    DWORD           dwHelpId;
    HCRYPTPROV      hprov;
} CERT_SELECT_STRUCT_A, *PCERT_SELECT_STRUCT_A;

typedef struct tagCSSW
{
    DWORD           dwSize;
    HWND            hwndParent;
    HINSTANCE       hInstance;
    LPCWSTR         pTemplateName;
    DWORD           dwFlags;
    LPCWSTR         szTitle;
    DWORD           cCertStore;
    HCERTSTORE     *arrayCertStore;
    LPCSTR          szPurposeOid;
    DWORD           cCertContext;
    PCCERT_CONTEXT *arrayCertContext;
    DWORD           lCustData;
    PFNCMHOOKPROC   pfnHook;
    PFNCMFILTERPROC pfnFilter;
    LPCWSTR         szHelpFileName;
    DWORD           dwHelpId;
    HCRYPTPROV      hprov;
} CERT_SELECT_STRUCT_W, *PCERT_SELECT_STRUCT_W;

#define CERT_SELECT_STRUCT WINELIB_NAME_AW(CERT_SELECT_STRUCT_)

BOOL WINAPI CertSelectCertificateA(PCERT_SELECT_STRUCT_A pCertSelectInfo);
BOOL WINAPI CertSelectCertificateW(PCERT_SELECT_STRUCT_W pCertSelectInfo);
#define CertSelectCertificate WINELIB_NAME_AW(CertSelectCertificate)

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

typedef struct tagCERT_VIEWPROPERTIES_STRUCT_A
{
    DWORD           dwSize;
    HWND            hwndParent;
    HINSTANCE       hInstance;
    DWORD           dwFlags;
    LPCSTR          szTitle;
    PCCERT_CONTEXT  pCertContext;
    LPSTR          *arrayPurposes;
    DWORD           cArrayPurposes;
    DWORD           cRootStores;
    HCERTSTORE     *rghstoreRoots;
    DWORD           cStores;
    HCERTSTORE     *rghstoreCAs;
    DWORD           cTrustStores;
    HCERTSTORE     *rghstoreTrust;
    HCRYPTPROV      hprov;
    DWORD           lCustData;
    DWORD           dwPad;
    LPCSTR          szHelpFileName;
    DWORD           dwHelpId;
    DWORD           nStartPage;
    DWORD           cArrayPropSheetPages;
    /* FIXME: PSDK declares arrayPropSheetPages as a PROPSHEETPAGE *, which we
     * don't allow in our own headers.  It's probably wrong, but we're not
     * compatible.
     */
    PROPSHEETPAGEA *arrayPropSheetPages;
} CERT_VIEWPROPERTIES_STRUCT_A, *PCERT_VIEWPROPERTIES_STRUCT_A;

typedef struct tagCERT_VIEWPROPERTIES_STRUCT_W
{
    DWORD           dwSize;
    HWND            hwndParent;
    HINSTANCE       hInstance;
    DWORD           dwFlags;
    LPCWSTR         szTitle;
    PCCERT_CONTEXT  pCertContext;
    LPSTR          *arrayPurposes;
    DWORD           cArrayPurposes;
    DWORD           cRootStores;
    HCERTSTORE     *rghstoreRoots;
    DWORD           cStores;
    HCERTSTORE     *rghstoreCAs;
    DWORD           cTrustStores;
    HCERTSTORE     *rghstoreTrust;
    HCRYPTPROV      hprov;
    DWORD           lCustData;
    DWORD           dwPad;
    LPCWSTR         szHelpFileName;
    DWORD           dwHelpId;
    DWORD           nStartPage;
    DWORD           cArrayPropSheetPages;
    /* FIXME: PSDK declares arrayPropSheetPages as a PROPSHEETPAGE *, which we
     * don't allow in our own headers.  It's probably wrong, but we're not
     * compatible.
     */
    PROPSHEETPAGEW *arrayPropSheetPages;
} CERT_VIEWPROPERTIES_STRUCT_W, *PCERT_VIEWPROPERTIES_STRUCT_W;

#define CERT_VIEWPROPERTIES_STRUCT WINELIB_NAME_AW(CERT_VIEWPROPERTIES_STRUCT_)
#define PCERT_VIEWPROPERTIES_STRUCT \
 WINELIB_NAME_AW(PCERT_VIEWPROPERTIES_STRUCT_)

BOOL WINAPI CertViewPropertiesA(PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo);
BOOL WINAPI CertViewPropertiesW(PCERT_VIEWPROPERTIES_STRUCT_W pCertViewInfo);
#define CertViewProperties WINELIB_NAME_AW(CertViewProperties)

#define CERT_FILTER_OP_EXISTS     1
#define CERT_FILTER_OP_NOT_EXISTS 2
#define CERT_FILTER_OP_EQUALITY   3

typedef struct tagCMOID
{
    LPCSTR szExtensionOID;
    DWORD  dwTestOperation;
    LPBYTE pbTestData;
    DWORD  cbTestData;
} CERT_FILTER_EXTENSION_MATCH;

#define CERT_FILTER_INCLUDE_V1_CERTS  0x0001
#define CERT_FILTER_VALID_TIME_RANGE  0x0002
#define CERT_FILTER_VALID_SIGNATURE   0x0004
#define CERT_FILTER_LEAF_CERTS_ONLY   0x0008
#define CERT_FILTER_ISSUER_CERTS_ONLY 0x0010
#define CERT_FILTER_KEY_EXISTS        0x0020

typedef struct tagCMFLTR
{
    DWORD                        dwSize;
    DWORD                        cExtensionChecks;
    CERT_FILTER_EXTENSION_MATCH *arrayExtensionChecks;
    DWORD                        dwCheckingFlags;
} CERT_FILTER_DATA;

DWORD WINAPI GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pchBuffer,
 DWORD cchBuffer);
DWORD WINAPI GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pchBuffer,
 DWORD cchBuffer);
#define GetFriendlyNameOfCert WINELIB_NAME_AW(GetFriendlyNameOfCert)

#define CERT_CERTIFICATE_ACTION_VERIFY \
 { 0x7801ebd0, 0xcf4b, 0x11d0, { 0x85,0x1f,0x00,0x60,0x97,0x93,0x87,0xea }}
#define szCERT_CERTIFICATE_ACTION_VERIFY \
 "{7801ebd0-cf4b-11d0-851f-0060979387ea}"

typedef HRESULT (WINAPI *PFNTRUSTHELPER)(PCCERT_CONTEXT, DWORD, BOOL, LPBYTE);

#define CERT_VALIDITY_MASK_VALIDITY              0x0000ffff
#define CERT_VALIDITY_BEFORE_START               0x00000001
#define CERT_VALIDITY_AFTER_END                  0x00000002
#define CERT_VALIDITY_SIGNATURE_FAILS            0x00000004
#define CERT_VALIDITY_CERTIFICATE_REVOKED        0x00000008
#define CERT_VALIDITY_KEY_USAGE_EXT_FAILURE      0x00000010
#define CERT_VALIDITY_EXTENDED_USAGE_FAILURE     0x00000020
#define CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE   0x00000040
#define CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION 0x00000080
#define CERT_VALIDITY_ISSUER_INVALID             0x00000100
#define CERT_VALIDITY_OTHER_EXTENSION_FAILURE    0x00000200
#define CERT_VALIDITY_PERIOD_NESTING_FAILURE     0x00000400
#define CERT_VALIDITY_OTHER_ERROR                0x00000800

#define CERT_VALIDITY_MASK_TRUST                 0xffff0000
#define CERT_VALIDITY_EXPLICITLY_DISTRUSTED      0x01000000
#define CERT_VALIDITY_ISSUER_DISTRUST            0x02000000
#define CERT_VALIDITY_NO_ISSUER_CERT_FOUND       0x10000000
#define CERT_VALIDITY_NO_CRL_FOUND               0x20000000
#define CERT_VALIDITY_CRL_OUT_OF_DATE            0x40000000
#define CERT_VALIDITY_NO_TRUST_DATA              0x80000000

#define CERT_TRUST_MASK                0x00ffffff
#define CERT_TRUST_DO_FULL_SEARCH      0x00000001
#define CERT_TRUST_PERMIT_MISSING_CRLS 0x00000002
#define CERT_TRUST_DO_FULL_TRUST       0x00000005
#define CERT_TRUST_ADD_CERT_STORES     CM_ADD_CERT_STORES

typedef struct _CERT_VERIFY_CERTIFICATE_TRUST
{
    DWORD            cbSize;
    PCCERT_CONTEXT   pccert;
    DWORD            dwFlags;
    DWORD            dwIgnoreErr;
    DWORD           *pdwErrors;
    LPSTR            pszUsageOid;
    HCRYPTPROV       hprov;
    DWORD            cRootStores;
    HCERTSTORE      *rghstoreRoots;
    DWORD            cStores;
    HCERTSTORE      *rghstoreCAs;
    DWORD            cTrustStores;
    HCERTSTORE      *rghstoreTrust;
    DWORD            lCustData;
    PFNTRUSTHELPER   pfnTrustHelper;
    DWORD           *pcchain;
    PCCERT_CONTEXT **prgChain;
    DWORD          **prgdwErrors;
    DATA_BLOB      **prgpbTrustInfo;
} CERT_VERIFY_CERTIFICATE_TRUST, *PCERT_VERIFY_CERTIFICATE_TRUST;

#define CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED 1
#define CTL_MODIFY_REQUEST_REMOVE          2
#define CTL_MODIFY_REQUEST_ADD_TRUSTED     3

typedef struct _CTL_MODIFY_REQUEST
{
    PCCERT_CONTEXT pccert;
    DWORD          dwOperation;
    DWORD          dwError;
} CTL_MODIFY_REQUEST, *PCTL_MODIFY_REQUEST;

HRESULT WINAPI CertModifyCertificatesToTrust(int cCertStore,
 PCTL_MODIFY_REQUEST rgCerts, LPCSTR szPurpose, HWND hwnd,
 HCERTSTORE hcertstoreTrust);

#ifdef __cplusplus
}
#endif

#endif
