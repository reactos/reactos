/*
 * Copyright (c) 2016 Austin English
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

#ifndef __NCRYPT_H__
#define __NCRYPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef __SECSTATUS_DEFINED__
typedef LONG SECURITY_STATUS;
#define __SECSTATUS_DEFINED__
#endif

typedef struct _NCryptAlgorithmName {
    LPWSTR pszName;
    DWORD dwClass;
    DWORD dwAlgOperations;
    DWORD dwFlags;
} NCryptAlgorithmName;

typedef struct _NCryptBuffer {
    ULONG cbBuffer;
    ULONG BufferType;
    PVOID pvBuffer;
} NCryptBuffer, *PNCryptBuffer;

typedef struct _NCryptBufferDesc {
    ULONG ulVersion;
    ULONG cBuffers;
    PNCryptBuffer pBuffers;
} NCryptBufferDesc, *PNCryptBufferDesc;

typedef struct NCryptKeyName {
    LPWSTR pszName;
    LPWSTR pszAlgid;
    DWORD dwLegacyKeySpec;
    DWORD dwFlags;
} NCryptKeyName;

typedef struct __NCRYPT_SUPPORTED_LENGTHS {
    DWORD dwMinLength;
    DWORD dwMaxLength;
    DWORD dwIncrement;
    DWORD dwDefaultLength;
} NCRYPT_SUPPORTED_LENGTHS;

typedef struct __NCRYPT_UI_POLICY {
    DWORD dwVersion;
    DWORD dwFlags;
    LPCWSTR pszCreationTitle;
    LPCWSTR pszFriendlyName;
    LPCWSTR pszDescription;
} NCRYPT_UI_POLICY;

typedef ULONG_PTR NCRYPT_HANDLE;
typedef ULONG_PTR NCRYPT_PROV_HANDLE;
typedef ULONG_PTR NCRYPT_KEY_HANDLE;
typedef ULONG_PTR NCRYPT_HASH_HANDLE;
typedef ULONG_PTR NCRYPT_SECRET_HANDLE;

#define NCRYPT_KEY_STORAGE_INTERFACE        0x00010001
#define NCRYPT_SCHANNEL_INTERFACE           0x00010002
#define NCRYPT_SCHANNEL_SIGNATURE_INTERFACE 0x00010003
#define NCRYPT_KEY_PROTECTION_INTERFACE     0x00010004

#define NCRYPT_SILENT_FLAG 0x00000040

#define NCRYPT_NO_PADDING_FLAG 0x00000001
#define NCRYPT_PAD_PKCS1_FLAG  0x00000002
#define NCRYPT_PAD_OAEP_FLAG   0x00000004
#define NCRYPT_PAD_PSS_FLAG    0x00000008

#define NCRYPT_ALLOW_DECRYPT_FLAG       0x00000001
#define NCRYPT_ALLOW_SIGNING_FLAG       0x00000002
#define NCRYPT_ALLOW_KEY_AGREEMENT_FLAG 0x00000004
#define NCRYPT_ALLOW_KEY_IMPORT_FLAG    0x00000008
#define NCRYPT_ALLOW_ALL_USAGES         0x00ffffff

#define NCRYPT_ALLOW_EXPORT_FLAG              0x00000001
#define NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG    0x00000002
#define NCRYPT_ALLOW_ARCHIVING_FLAG           0x00000004
#define NCRYPT_ALLOW_PLAINTEXT_ARCHIVING_FLAG 0x00000008

#define NCRYPT_NAME_PROPERTY                    L"Name"
#define NCRYPT_UNIQUE_NAME_PROPERTY             L"Unique Name"
#define NCRYPT_ALGORITHM_PROPERTY               L"Algorithm Name"
#define NCRYPT_LENGTH_PROPERTY                  L"Length"
#define NCRYPT_LENGTHS_PROPERTY                 L"Lengths"
#define NCRYPT_BLOCK_LENGTH_PROPERTY            L"Block Length"
#define NCRYPT_UI_POLICY_PROPERTY               L"UI Policy"
#define NCRYPT_EXPORT_POLICY_PROPERTY           L"Export Policy"
#define NCRYPT_WINDOW_HANDLE_PROPERTY           L"HWND Handle"
#define NCRYPT_USE_CONTEXT_PROPERTY             L"Use Context"
#define NCRYPT_IMPL_TYPE_PROPERTY               L"Impl Type"
#define NCRYPT_KEY_USAGE_PROPERTY               L"Key Usage"
#define NCRYPT_KEY_TYPE_PROPERTY                L"Key Type"
#define NCRYPT_VERSION_PROPERTY                 L"Version"
#define NCRYPT_SECURITY_DESCR_SUPPORT_PROPERTY  L"Security Descr Support"
#define NCRYPT_SECURITY_DESCR_PROPERTY          L"Security Descr"
#define NCRYPT_USE_COUNT_ENABLED_PROPERTY       L"Enabled Use Count"
#define NCRYPT_USE_COUNT_PROPERTY               L"Use Count"
#define NCRYPT_LAST_MODIFIED_PROPERTY           L"Modified"
#define NCRYPT_MAX_NAME_LENGTH_PROPERTY         L"Max Name Length"
#define NCRYPT_ALGORITHM_GROUP_PROPERTY         L"Algorithm Group"
#define NCRYPT_PROVIDER_HANDLE_PROPERTY         L"Provider Handle"
#define NCRYPT_PIN_PROPERTY                     L"SmartCardPin"
#define NCRYPT_READER_PROPERTY                  L"SmartCardReader"
#define NCRYPT_SMARTCARD_GUID_PROPERTY          L"SmartCardGuid"
#define NCRYPT_CERTIFICATE_PROPERTY             L"SmartCardKeyCertificate"
#define NCRYPT_PIN_PROMPT_PROPERTY              L"SmartCardPinPrompt"
#define NCRYPT_USER_CERTSTORE_PROPERTY          L"SmartCardUserCertStore"
#define NCRYPT_ROOT_CERTSTORE_PROPERTY          L"SmartcardRootCertStore"
#define NCRYPT_SECURE_PIN_PROPERTY              L"SmartCardSecurePin"
#define NCRYPT_ASSOCIATED_ECDH_KEY              L"SmartCardAssociatedECDHKey"
#define NCRYPT_SCARD_PIN_ID                     L"SmartCardPinId"
#define NCRYPT_SCARD_PIN_INFO                   L"SmartCardPinInfo"

SECURITY_STATUS WINAPI NCryptCreatePersistedKey(NCRYPT_PROV_HANDLE, NCRYPT_KEY_HANDLE *, const WCHAR *, const WCHAR *,
                                                DWORD, DWORD);
SECURITY_STATUS WINAPI NCryptDecrypt(NCRYPT_KEY_HANDLE, BYTE *, DWORD, void *, BYTE *, DWORD, DWORD *, DWORD);
SECURITY_STATUS WINAPI NCryptEncrypt(NCRYPT_KEY_HANDLE, BYTE *, DWORD, void *, BYTE *, DWORD, DWORD *, DWORD);
SECURITY_STATUS WINAPI NCryptExportKey(NCRYPT_KEY_HANDLE, NCRYPT_KEY_HANDLE, const WCHAR *, NCryptBufferDesc *, BYTE *,
                                       DWORD, DWORD *, DWORD);
SECURITY_STATUS WINAPI NCryptFinalizeKey(NCRYPT_KEY_HANDLE, DWORD);
SECURITY_STATUS WINAPI NCryptFreeObject(NCRYPT_HANDLE);
SECURITY_STATUS WINAPI NCryptGetProperty(NCRYPT_HANDLE, const WCHAR *, BYTE *, DWORD, DWORD *, DWORD);
SECURITY_STATUS WINAPI NCryptImportKey(NCRYPT_PROV_HANDLE, NCRYPT_KEY_HANDLE, const WCHAR *, NCryptBufferDesc *,
                                       NCRYPT_KEY_HANDLE *, BYTE *, DWORD, DWORD);
SECURITY_STATUS WINAPI NCryptIsAlgSupported(NCRYPT_PROV_HANDLE, const WCHAR *, DWORD);
SECURITY_STATUS WINAPI NCryptOpenKey(NCRYPT_PROV_HANDLE, NCRYPT_KEY_HANDLE *, const WCHAR *, DWORD, DWORD);
SECURITY_STATUS WINAPI NCryptOpenStorageProvider(NCRYPT_PROV_HANDLE *, const WCHAR *, DWORD);
SECURITY_STATUS WINAPI NCryptSetProperty(NCRYPT_HANDLE, const WCHAR *, BYTE *, DWORD, DWORD);
SECURITY_STATUS WINAPI NCryptSignHash(NCRYPT_KEY_HANDLE, void *, BYTE *, DWORD, BYTE *, DWORD, DWORD *, DWORD);
SECURITY_STATUS WINAPI NCryptVerifySignature(NCRYPT_KEY_HANDLE, void *, BYTE *, DWORD, BYTE *, DWORD, DWORD);

#ifdef __cplusplus
}
#endif

#endif /* __NCRYPT_H__ */
