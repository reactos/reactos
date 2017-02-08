/*
 * Copyright (C) 2006 Robert Shearman (for CodeWeavers)
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

#ifndef _WINCRED_H_
#define _WINCRED_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ADVAPI32_
#define WINADVAPI
#else
#define WINADVAPI DECLSPEC_IMPORT
#endif

#ifndef __SECHANDLE_DEFINED__
#define __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#endif

#ifndef __WINE_CTXTHANDLE_DEFINED__
#define __WINE_CTXTHANDLE_DEFINED__
typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;
#endif

typedef struct _CREDENTIAL_ATTRIBUTEA
{
    LPSTR Keyword;
    DWORD Flags;
    DWORD ValueSize;
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEA, *PCREDENTIAL_ATTRIBUTEA;

typedef struct _CREDENTIAL_ATTRIBUTEW
{
    LPWSTR Keyword;
    DWORD Flags;
    DWORD ValueSize;
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEW, *PCREDENTIAL_ATTRIBUTEW;

DECL_WINELIB_TYPE_AW(CREDENTIAL_ATTRIBUTE)
DECL_WINELIB_TYPE_AW(PCREDENTIAL_ATTRIBUTE)

typedef struct _CREDENTIALA
{
    DWORD Flags;
    DWORD Type;
    LPSTR TargetName;
    LPSTR Comment;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    _Field_size_bytes_(CredentialBlobSize) LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
    PCREDENTIAL_ATTRIBUTEA Attributes;
    LPSTR TargetAlias;
    LPSTR UserName;
} CREDENTIALA, *PCREDENTIALA;

typedef struct _CREDENTIALW
{
    DWORD Flags;
    DWORD Type;
    LPWSTR TargetName;
    LPWSTR Comment;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
    PCREDENTIAL_ATTRIBUTEW Attributes;
    LPWSTR TargetAlias;
    LPWSTR UserName;
} CREDENTIALW, *PCREDENTIALW;

DECL_WINELIB_TYPE_AW(CREDENTIAL)
DECL_WINELIB_TYPE_AW(PCREDENTIAL)

typedef struct _CREDENTIAL_TARGET_INFORMATIONA
{
    LPSTR TargetName;
    LPSTR NetbiosServerName;
    LPSTR DnsServerName;
    LPSTR NetbiosDomainName;
    LPSTR DnsDomainName;
    LPSTR DnsTreeName;
    LPSTR PackageName;
    DWORD Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONA, *PCREDENTIAL_TARGET_INFORMATIONA;

typedef struct _CREDENTIAL_TARGET_INFORMATIONW
{
    LPWSTR TargetName;
    LPWSTR NetbiosServerName;
    LPWSTR DnsServerName;
    LPWSTR NetbiosDomainName;
    LPWSTR DnsDomainName;
    LPWSTR DnsTreeName;
    LPWSTR PackageName;
    DWORD Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONW, *PCREDENTIAL_TARGET_INFORMATIONW;

DECL_WINELIB_TYPE_AW(CREDENTIAL_TARGET_INFORMATION)
DECL_WINELIB_TYPE_AW(PCREDENTIAL_TARGET_INFORMATION)

typedef struct _CREDUI_INFOA
{
    DWORD cbSize;
    HWND hwndParent;
    PCSTR pszMessageText;
    PCSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOA, *PCREDUI_INFOA;

typedef struct _CREDUI_INFOW
{
    DWORD cbSize;
    HWND hwndParent;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOW, *PCREDUI_INFOW;

DECL_WINELIB_TYPE_AW(CREDUI_INFO)
DECL_WINELIB_TYPE_AW(PCREDUI_INFO)

typedef enum _CRED_MARSHAL_TYPE
{
    CertCredential = 1,
    UsernameTargetCredential,
    BinaryBlobCredential
} CRED_MARSHAL_TYPE, *PCRED_MARSHAL_TYPE;

#define CERT_HASH_LENGTH    20

typedef struct _CERT_CREDENTIAL_INFO
{
    ULONG cbSize;
    UCHAR rgbHashOfCert[CERT_HASH_LENGTH];
} CERT_CREDENTIAL_INFO, *PCERT_CREDENTIAL_INFO;

typedef struct _USERNAME_TARGET_CREDENTIAL_INFO
{
    LPWSTR UserName;
} USERNAME_TARGET_CREDENTIAL_INFO;

typedef struct _BINARY_BLOB_CREDENTIAL_INFO
{
    ULONG cbBlob;
    LPBYTE pbBlob;
} BINARY_BLOB_CREDENTIAL_INFO, *PBINARY_BLOB_CREDENTIAL_INFO;

#define CRED_MAX_STRING_LENGTH              256
#define CRED_MAX_USERNAME_LENGTH            513
#define CRED_MAX_GENERIC_TARGET_NAME_LENGTH 32767
#define CRED_MAX_DOMAIN_TARGET_NAME_LENGTH  337
#define CRED_MAX_VALUE_SIZE                 256
#define CRED_MAX_ATTRIBUTES                 64

#define CRED_MAX_BLOB_SIZE                  512
#define CRED_MAX_CREDENTIAL_BLOB_SIZE       (5 * CRED_MAX_BLOB_SIZE)

#define CREDUI_MAX_MESSAGE_LENGTH 32767
#define CREDUI_MAX_CAPTION_LENGTH 128
#define CREDUI_MAX_GENERIC_TARGET_LENGTH CRED_MAX_GENERIC_TARGET_NAME_LENGTH
#define CREDUI_MAX_DOMAIN_TARGET_LENGTH CRED_MAX_DOMAIN_TARGET_LENGTH
#define CREDUI_MAX_USERNAME_LENGTH CRED_MAX_USERNAME_LENGTH
#define CREDUI_MAX_PASSWORD_LENGTH (CRED_MAX_CREDENTIAL_BLOB_SIZE / 2)

/* flags for CREDENTIAL::Flags */
#define CRED_FLAGS_PASSWORD_FOR_CERT                0x0001
#define CRED_FLAGS_PROMPT_NOW                       0x0002
#define CRED_FLAGS_USERNAME_TARGET                  0x0004
#define CRED_FLAGS_OWF_CRED_BLOB                    0x0008
#define CRED_FLAGS_VALID_FLAGS                      0x000f

/* values for CREDENTIAL::Type */
#define CRED_TYPE_GENERIC                           1
#define CRED_TYPE_DOMAIN_PASSWORD                   2
#define CRED_TYPE_DOMAIN_CERTIFICATE                3
#define CRED_TYPE_DOMAIN_VISIBLE_PASSWORD           4
#define CRED_TYPE_GENERIC_CERTIFICATE               5
#define CRED_TYPE_MAXIMUM                           6
#define CRED_TYPE_MAXIMUM_EX                        (CRED_TYPE_MAXIMUM+1000)

/* values for CREDENTIAL::Persist */
#define CRED_PERSIST_NONE                           0
#define CRED_PERSIST_SESSION                        1
#define CRED_PERSIST_LOCAL_MACHINE                  2
#define CRED_PERSIST_ENTERPRISE                     3

/* values for CREDENTIAL_TARGET_INFORMATION::Flags */
#define CRED_TI_SERVER_FORMAT_UNKNOWN               1
#define CRED_TI_DOMAIN_FORMAT_UNKNOWN               2
#define CRED_TI_ONLY_PASSWORD_REQUIRED              4

#define CREDUI_FLAGS_INCORRECT_PASSWORD             0x00000001
#define CREDUI_FLAGS_DO_NOT_PERSIST                 0x00000002
#define CREDUI_FLAGS_REQUEST_ADMINISTRATOR          0x00000004
#define CREDUI_FLAGS_EXCLUDE_CERTIFICATES           0x00000008
#define CREDUI_FLAGS_REQUIRE_CERTIFICATE            0x00000010
#define CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX            0x00000040
#define CREDUI_FLAGS_ALWAYS_SHOW_UI                 0x00000080
#define CREDUI_FLAGS_REQUIRE_SMARTCARD              0x00000100
#define CREDUI_FLAGS_PASSWORD_ONLY_OK               0x00000200
#define CREDUI_FLAGS_VALIDATE_USERNAME              0x00000400
#define CREDUI_FLAGS_COMPLETE_USERNAME              0x00000800
#define CREDUI_FLAGS_PERSIST                        0x00001000
#define CREDUI_FLAGS_SERVER_CREDENTIAL              0x00004000
#define CREDUI_FLAGS_EXPECT_CONFIRMATION            0x00020000
#define CREDUI_FLAGS_GENERIC_CREDENTIALS            0x00040000
#define CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS    0x00080000
#define CREDUI_FLAGS_KEEP_USERNAME                  0x00100000

/* flags for CredWrite and CredWriteDomainCredentials */
#define CRED_PRESERVE_CREDENTIAL_BLOB               0x00000001

WINADVAPI BOOL WINAPI CredDeleteA(_In_ LPCSTR, _In_ DWORD, _Reserved_ DWORD);
WINADVAPI BOOL WINAPI CredDeleteW(_In_ LPCWSTR, _In_ DWORD, _Reserved_ DWORD);
#define CredDelete WINELIB_NAME_AW(CredDelete)

WINADVAPI
BOOL
WINAPI
CredEnumerateA(
  _In_opt_ LPCSTR,
  _Reserved_ DWORD,
  _Out_ DWORD *Count,
  _Outptr_result_buffer_(*Count) PCREDENTIALA **);

WINADVAPI
BOOL
WINAPI
CredEnumerateW(
  _In_opt_ LPCWSTR,
  _Reserved_ DWORD,
  _Out_ DWORD *Count,
  _Outptr_result_buffer_(*Count) PCREDENTIALW **);

#define CredEnumerate WINELIB_NAME_AW(CredEnumerate)

WINADVAPI VOID WINAPI CredFree(_In_ PVOID);

WINADVAPI
BOOL
WINAPI
CredGetSessionTypes(
  _In_ DWORD MaximumPersistCount,
  _Out_writes_(MaximumPersistCount) LPDWORD);

WINADVAPI BOOL WINAPI CredIsMarshaledCredentialA(_In_ LPCSTR);
WINADVAPI BOOL WINAPI CredIsMarshaledCredentialW(_In_ LPCWSTR);
#define CredIsMarshaledCredential WINELIB_NAME_AW(CredIsMarshaledCredential)

WINADVAPI
BOOL
WINAPI
CredMarshalCredentialA(
  _In_ CRED_MARSHAL_TYPE,
  _In_ PVOID,
  _Out_ LPSTR *);

WINADVAPI
BOOL
WINAPI
CredMarshalCredentialW(
  _In_ CRED_MARSHAL_TYPE,
  _In_ PVOID,
  _Out_ LPWSTR *);

#define CredMarshalCredential WINELIB_NAME_AW(CredMarshalCredential)

WINADVAPI
BOOL
WINAPI
CredReadA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Reserved_ DWORD,
  _Out_ PCREDENTIALA *);

WINADVAPI
BOOL
WINAPI
CredReadW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Reserved_ DWORD,
  _Out_ PCREDENTIALW *);

#define CredRead WINELIB_NAME_AW(CredRead)

WINADVAPI
BOOL
WINAPI
CredReadDomainCredentialsA(
  _In_ PCREDENTIAL_TARGET_INFORMATIONA,
  _In_ DWORD,
  _Out_ DWORD *Count,
  _Outptr_result_buffer_(*Count) PCREDENTIALA **);

WINADVAPI
BOOL
WINAPI
CredReadDomainCredentialsW(
  _In_ PCREDENTIAL_TARGET_INFORMATIONW,
  _In_ DWORD,
  _Out_ DWORD *Count,
  _Outptr_result_buffer_(*Count) PCREDENTIALW **);

#define CredReadDomainCredentials WINELIB_NAME_AW(CredReadDomainCredentials)

WINADVAPI
BOOL
WINAPI
CredRenameA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _Reserved_ DWORD);

WINADVAPI
BOOL
WINAPI
CredRenameW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _Reserved_ DWORD);

#define CredRename WINELIB_NAME_AW(CredRename)

WINADVAPI
BOOL
WINAPI
CredUnmarshalCredentialA(
  _In_ LPCSTR,
  _Out_ PCRED_MARSHAL_TYPE,
  _Out_ PVOID *);

WINADVAPI
BOOL
WINAPI
CredUnmarshalCredentialW(
  _In_ LPCWSTR,
  _Out_ PCRED_MARSHAL_TYPE,
  _Out_ PVOID *);

#define CredUnmarshalCredential WINELIB_NAME_AW(CredUnmarshalCredential)

WINADVAPI BOOL WINAPI CredWriteA(_In_ PCREDENTIALA, _In_ DWORD);
WINADVAPI BOOL WINAPI CredWriteW(_In_ PCREDENTIALW, _In_ DWORD);
#define CredWrite WINELIB_NAME_AW(CredWrite)

DWORD
WINAPI
CredUICmdLinePromptForCredentialsW(
  _In_opt_ PCWSTR,
  _Reserved_ PCtxtHandle,
  _In_ DWORD,
  _Inout_updates_(ulUserBufferSize) PWSTR,
  _In_ ULONG ulUserBufferSize,
  _Inout_updates_(ulPasswordBufferSize) PWSTR,
  _In_ ULONG ulPasswordBufferSize,
  _Inout_opt_ PBOOL,
  _In_ DWORD);

DWORD
WINAPI
CredUICmdLinePromptForCredentialsA(
  _In_opt_ PCSTR,
  _Reserved_ PCtxtHandle,
  _In_ DWORD,
  _Inout_updates_(ulUserBufferSize) PSTR,
  _In_ ULONG ulUserBufferSize,
  _Inout_updates_(ulPasswordBufferSize) PSTR,
  _In_ ULONG ulPasswordBufferSize,
  _Inout_opt_ PBOOL,
  _In_ DWORD);

#define CredUICmdLinePromptForCredentials WINELIB_NAME_AW(CredUICmdLinePromptForCredentials)

DWORD WINAPI CredUIConfirmCredentialsW(_In_ PCWSTR, _In_ BOOL);
DWORD WINAPI CredUIConfirmCredentialsA(_In_ PCSTR, _In_ BOOL);
#define CredUIConfirmCredentials WINELIB_NAME_AW(CredUIConfirmCredentials)

DWORD
WINAPI
CredUIParseUserNameW(
  _In_ PCWSTR,
  _Out_writes_(userBufferSize) PWSTR,
  _In_ ULONG userBufferSize,
  _Out_writes_(domainBufferSize) PWSTR,
  _In_ ULONG domainBufferSize);

DWORD
WINAPI
CredUIParseUserNameA(
  _In_ PCSTR,
  _Out_writes_(userBufferSize) PSTR,
  _In_ ULONG userBufferSize,
  _Out_writes_(domainBufferSize) PSTR,
  _In_ ULONG domainBufferSize);

#define CredUIParseUserName WINELIB_NAME_AW(CredUIParseUserName)

DWORD
WINAPI
CredUIPromptForCredentialsW(
  _In_opt_ PCREDUI_INFOW,
  _In_opt_ PCWSTR,
  _Reserved_ PCtxtHandle,
  _In_ DWORD,
  _Inout_updates_(ulUserNameBufferSize) PWSTR,
  _In_ ULONG ulUserNameBufferSize,
  _Inout_updates_(ulPasswordBufferSize) PWSTR,
  _In_ ULONG ulPasswordBufferSize,
  _Inout_opt_ PBOOL,
  _In_ DWORD);

DWORD
WINAPI
CredUIPromptForCredentialsA(
  _In_opt_ PCREDUI_INFOA,
  _In_opt_ PCSTR,
  _Reserved_ PCtxtHandle,
  _In_ DWORD,
  _Inout_updates_(ulUserNameBufferSize) PSTR,
  _In_ ULONG ulUserNameBufferSize,
  _Inout_updates_(ulPasswordBufferSize) PSTR,
  _In_ ULONG ulPasswordBufferSize,
  _Inout_opt_ PBOOL,
  _In_ DWORD);

#define CredUIPromptForCredentials WINELIB_NAME_AW(CredUIPromptForCredentials)

DWORD
WINAPI
CredUIStoreSSOCredW(
  _In_opt_ PCWSTR,
  _In_ PCWSTR,
  _In_ PCWSTR,
   _In_ BOOL);

/* Note: no CredUIStoreSSOCredA in PSDK header */
DWORD WINAPI CredUIReadSSOCredW(_In_opt_ PCWSTR, _Outptr_ PWSTR*);
/* Note: no CredUIReadSSOCredA in PSDK header */

#ifdef __cplusplus
}
#endif

#endif /* _WINCRED_H_ */
