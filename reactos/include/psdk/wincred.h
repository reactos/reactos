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

#ifndef __SECHANDLE_DEFINED__
#define __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#endif

typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;

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
    LPBYTE CredentialBlob;
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

typedef enum _CRED_MARSHAL_TYPE {
    CertCredential = 1,
    UsernameTargetCredential
} CRED_MARSHAL_TYPE, *PCRED_MARSHAL_TYPE;

typedef struct _CREDENTIAL_TARGET_INFORMATIONA {
    LPSTR TargetName;
    LPSTR NetbiosServerName;
    LPSTR DnsServerName;
    LPSTR NetbiosDomainName;
    LPSTR DnsDomainName;
    LPSTR DnsTreeName;
    LPSTR PackageName;
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONA, *PCREDENTIAL_TARGET_INFORMATIONA;

typedef struct _CREDENTIAL_TARGET_INFORMATIONW {
    LPWSTR TargetName;
    LPWSTR NetbiosServerName;
    LPWSTR DnsServerName;
    LPWSTR NetbiosDomainName;
    LPWSTR DnsDomainName;
    LPWSTR DnsTreeName;
    LPWSTR PackageName;
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONW, *PCREDENTIAL_TARGET_INFORMATIONW;

#ifdef UNICODE
typedef CREDENTIAL_TARGET_INFORMATIONW CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONW PCREDENTIAL_TARGET_INFORMATION;
#else
typedef CREDENTIAL_TARGET_INFORMATIONA CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONA PCREDENTIAL_TARGET_INFORMATION;
#endif /* UNICODE */

DECL_WINELIB_TYPE_AW(CREDUI_INFO)
DECL_WINELIB_TYPE_AW(PCREDUI_INFO)

#define CRED_MAX_STRING_LENGTH              256
#define CRED_MAX_USERNAME_LENGTH            513
#define CRED_MAX_GENERIC_TARGET_NAME_LENGTH 32767
#define CRED_MAX_DOMAIN_TARGET_NAME_LENGTH  337
#define CRED_MAX_VALUE_SIZE                 256
#define CRED_MAX_ATTRIBUTES                 64

#define CRED_MAX_BLOB_SIZE                  512

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
#define CRED_TYPE_MAXIMUM                           5

/* values for CREDENTIAL::Persist */
#define CRED_PERSIST_NONE                           0
#define CRED_PERSIST_SESSION                        1
#define CRED_PERSIST_LOCAL_MACHINE                  2
#define CRED_PERSIST_ENTERPRISE                     3

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

BOOL  WINAPI CredDeleteA(LPCSTR,DWORD,DWORD);
BOOL  WINAPI CredDeleteW(LPCWSTR,DWORD,DWORD);
#define      CredDelete WINELIB_NAME_AW(CredDelete)
BOOL  WINAPI CredEnumerateA(LPCSTR,DWORD,DWORD *,PCREDENTIALA **);
BOOL  WINAPI CredEnumerateW(LPCWSTR,DWORD,DWORD *,PCREDENTIALW **);
#define      CredEnumerate WINELIB_NAME_AW(CredEnumerate)
VOID  WINAPI CredFree(PVOID);
BOOL  WINAPI CredReadA(LPCSTR,DWORD,DWORD,PCREDENTIALA *);
BOOL  WINAPI CredReadW(LPCWSTR,DWORD,DWORD,PCREDENTIALW *);
#define      CredRead WINELIB_NAME_AW(CredRead)
BOOL  WINAPI CredRenameA(LPCSTR,LPCSTR,DWORD,DWORD);
BOOL  WINAPI CredRenameW(LPCWSTR,LPCWSTR,DWORD,DWORD);
#define      CredRename WINELIB_NAME_AW(CredRename)
BOOL  WINAPI CredWriteA(PCREDENTIALA,DWORD);
BOOL  WINAPI CredWriteW(PCREDENTIALW,DWORD);
#define      CredWrite WINELIB_NAME_AW(CredWrite)

DWORD WINAPI CredUICmdLinePromptForCredentialsW(PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,PBOOL,DWORD);
DWORD WINAPI CredUICmdLinePromptForCredentialsA(PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,PBOOL,DWORD);
#define      CredUICmdLinePromptForCredentials WINELIB_NAME_AW(CredUICmdLinePromptForCredentials)
DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR,BOOL);
DWORD WINAPI CredUIConfirmCredentialsA(PCSTR,BOOL);
#define      CredUIConfirmCredentials WINELIB_NAME_AW(CredUIConfirmCredentials)
DWORD WINAPI CredUIParseUserNameW(PCWSTR,PWSTR,ULONG,PWSTR,ULONG);
DWORD WINAPI CredUIParseUserNameA(PCSTR,PSTR,ULONG,PSTR,ULONG);
#define      CredUIParseUserName WINELIB_NAME_AW(CredUIParseUserName)
DWORD WINAPI CredUIPromptForCredentialsW(PCREDUI_INFOW,PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,PBOOL,DWORD);
DWORD WINAPI CredUIPromptForCredentialsA(PCREDUI_INFOA,PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,PBOOL,DWORD);
#define      CredUIPromptForCredentials WINELIB_NAME_AW(CredUIPromptForCredentials)
DWORD WINAPI CredUIStoreSSOCredW(PCWSTR,PCWSTR,PCWSTR,BOOL);
/* Note: no CredUIStoreSSOCredA in PSDK header */
DWORD WINAPI CredUIReadSSOCredW(PCWSTR,PWSTR*);
/* Note: no CredUIReadSSOCredA in PSDK header */

BOOL WINAPI CredReadDomainCredentialsW(PCREDENTIAL_TARGET_INFORMATIONW TargetInfo, DWORD Flags, DWORD *Count, PCREDENTIALW **Credential);
BOOL WINAPI CredReadDomainCredentialsA(PCREDENTIAL_TARGET_INFORMATIONA TargetInfo, DWORD Flags, DWORD *Count, PCREDENTIALA **Credential);
BOOL WINAPI CredWriteDomainCredentialsW(PCREDENTIAL_TARGET_INFORMATIONW TargetInfo, PCREDENTIALW Credential, DWORD Flags);
BOOL WINAPI CredWriteDomainCredentialsA(PCREDENTIAL_TARGET_INFORMATIONA TargetInfo, PCREDENTIALA Credential, DWORD Flags);
BOOL WINAPI CredUnmarshalCredentialW(LPCWSTR MarshaledCredential, PCRED_MARSHAL_TYPE CredType, PVOID *Credential);
BOOL WINAPI CredUnmarshalCredentialA(LPCSTR MarshaledCredential, PCRED_MARSHAL_TYPE CredType, PVOID *Credential);

#ifdef UNICODE
#define CredReadDomainCredentials CredReadDomainCredentialsW
#define CredWriteDomainCredentials CredWriteDomainCredentialsW
#define CredUnmarshalCredential CredUnmarshalCredentialW
#else
#define CredReadDomainCredentials CredReadDomainCredentialsA
#define CredWriteDomainCredentials CredWriteDomainCredentialsA
#define CredUnmarshalCredential CredUnmarshalCredentialA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif /* _WINCRED_H_ */
