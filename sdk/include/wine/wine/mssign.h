/*
 * Copyright 2019 Gijs Vermeulen
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

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "mssip.h"

#define SIGNER_CERT_SPC_FILE    1
#define SIGNER_CERT_STORE       2
#define SIGNER_CERT_SPC_CHAIN   3

#define SIGNER_CERT_POLICY_STORE            0x1
#define SIGNER_CERT_POLICY_CHAIN            0x2
#define SIGNER_CERT_POLICY_CHAIN_NO_ROOT    0x8

#define SIGNER_NO_ATTR          0
#define SIGNER_AUTHCODE_ATTR    1

typedef struct _SIGNER_CONTEXT {
    DWORD cbSize;
    DWORD cbBlob;
    BYTE  *pbBlob;
} SIGNER_CONTEXT, *PSIGNER_CONTEXT;

typedef struct _SIGNER_FILE_INFO {
    DWORD       cbSize;
    const WCHAR *pwszFileName;
    HANDLE      hFile;
} SIGNER_FILE_INFO, *PSIGNER_FILE_INFO;

typedef struct _SIGNER_BLOB_INFO {
    DWORD       cbSize;
    GUID        *pGuidSubject;
    DWORD       cbBlob;
    BYTE        *pbBlob;
    const WCHAR *pwszDisplayName;
} SIGNER_BLOB_INFO, *PSIGNER_BLOB_INFO;

typedef struct _SIGNER_SUBJECT_INFO {
    DWORD cbSize;
    DWORD *pdwIndex;
    DWORD dwSubjectChoice;
    union {
        SIGNER_FILE_INFO *pSignerFileInfo;
        SIGNER_BLOB_INFO *pSignerBlobInfo;
    };
} SIGNER_SUBJECT_INFO, *PSIGNER_SUBJECT_INFO;

typedef struct _SIGNER_CERT_STORE_INFO {
    DWORD              cbSize;
    const CERT_CONTEXT *pSigningCert;
    DWORD              dwCertPolicy;
    HCERTSTORE         hCertStore;
} SIGNER_CERT_STORE_INFO, *PSIGNER_CERT_STORE_INFO;

typedef struct _SIGNER_SPC_CHAIN_INFO {
    DWORD       cbSize;
    const WCHAR *pwszSpcFile;
    DWORD       dwCertPolicy;
    HCERTSTORE  hCertStore;
} SIGNER_SPC_CHAIN_INFO, *PSIGNER_SPC_CHAIN_INFO;

typedef struct _SIGNER_CERT {
    DWORD cbSize;
    DWORD dwCertChoice;
    union {
        const WCHAR            *pwszSpcFile;
        SIGNER_CERT_STORE_INFO *pCertStoreInfo;
        SIGNER_SPC_CHAIN_INFO  *pSpcChainInfo;
    };
    HWND  hwnd;
} SIGNER_CERT, *PSIGNER_CERT;

typedef struct _SIGNER_ATTR_AUTHCODE {
    DWORD       cbSize;
    BOOL        fCommercial;
    BOOL        fIndividual;
    const WCHAR *pwszName;
    const WCHAR *pwszInfo;
} SIGNER_ATTR_AUTHCODE, *PSIGNER_ATTR_AUTHCODE;

typedef struct _SIGNER_SIGNATURE_INFO {
    DWORD            cbSize;
    ALG_ID           algidHash;
    DWORD            dwAttrChoice;
    union {
        SIGNER_ATTR_AUTHCODE *pAttrAuthcode;
    };
    CRYPT_ATTRIBUTES *psAuthenticated;
    CRYPT_ATTRIBUTES *psUnauthenticated;
} SIGNER_SIGNATURE_INFO, *PSIGNER_SIGNATURE_INFO;

typedef struct _SIGNER_PROVIDER_INFO {
    DWORD       cbSize;
    const WCHAR *pwszProviderName;
    DWORD       dwProviderType;
    DWORD       dwKeySpec;
    DWORD       dwPvkChoice;
    union {
        WCHAR *pwszPvkFileName;
        WCHAR *pwszKeyContainer;
    };
} SIGNER_PROVIDER_INFO, *PSIGNER_PROVIDER_INFO;
