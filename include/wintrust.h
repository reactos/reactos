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

#include <poppack.h>


#ifdef __cplusplus
extern "C" {
#endif

BOOL      WINAPI WintrustAddActionID(GUID*,DWORD,CRYPT_REGISTER_ACTIONID*);
void      WINAPI WintrustGetRegPolicyFlags(DWORD*);
LONG      WINAPI WinVerifyTrust(HWND,GUID*,WINTRUST_DATA*);

#ifdef __cplusplus
}
#endif

#endif
