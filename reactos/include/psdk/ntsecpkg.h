/*
 * Copyright (C) 2007 Yuval Fledel
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

#ifndef _NTSECPKG_H
#define _NTSECPKG_H

/* Flags for the MachineState field in SECPKG_PARAMETERS */
#define SECPKG_STATE_ENCRYPTION_PERMITTED               0x01
#define SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED        0x02
#define SECPKG_STATE_DOMAIN_CONTROLLER                  0x04
#define SECPKG_STATE_WORKSTATION                        0x08
#define SECPKG_STATE_STANDALONE                         0x10

/* Version magics as passed to or returned from Sp[Lsa,Mode]ModeInitialize */
#define SECPKG_INTERFACE_VERSION                     0x10000
#define SECPKG_INTERFACE_VERSION_2                   0x20000
#define SECPKG_INTERFACE_VERSION_3                   0x40000

/* enum definitions for Secure Service Provider/Authentication Packages */
typedef enum _LSA_TOKEN_INFORMATION_TYPE {
    LsaTokenInformationNull,
    LsaTokenInformationV1
} LSA_TOKEN_INFORMATION_TYPE, *PLSA_TOKEN_INFORMATION_TYPE;

typedef enum _SECPKG_EXTENDED_INFORMATION_CLASS
{
    SecpkgGssInfo = 1,
    SecpkgContextThunks,
    SecpkgMutualAuthLevel,
    SecpkgMaxInfo
} SECPKG_EXTENDED_INFORMATION_CLASS;

typedef enum _SECPKG_NAME_TYPE {
    SecNameSamCompatible,
    SecNameAlternateId,
    SecNameFlat,
    SecNameDN
} SECPKG_NAME_TYPE;

/* struct definitions for SSP/AP */
typedef struct _SECPKG_PRIMARY_CRED {
    LUID LogonId;
    UNICODE_STRING DownlevelName;
    UNICODE_STRING DomainName;
    UNICODE_STRING Password;
    UNICODE_STRING OldPassword;
    PSID UserSid;
    ULONG Flags;
    UNICODE_STRING DnsDomainName;
    UNICODE_STRING Upn;
    UNICODE_STRING LogonServer;
    UNICODE_STRING Spare1;
    UNICODE_STRING Spare2;
    UNICODE_STRING Spare3;
    UNICODE_STRING Spare4;
} SECPKG_PRIMARY_CRED, *PSECPKG_PRIMARY_CRED;

typedef struct _SECPKG_SUPPLEMENTAL_CRED {
    UNICODE_STRING PackageName;
    ULONG CredentialSize;
    PUCHAR Credentials;
} SECPKG_SUPPLEMENTAL_CRED, *PSECPKG_SUPPLEMENTAL_CRED;

typedef struct _SECPKG_SUPPLEMENTAL_CRED_ARRAY {
    ULONG CredentialCount;
    SECPKG_SUPPLEMENTAL_CRED Credentials[1];
} SECPKG_SUPPLEMENTAL_CRED_ARRAY, *PSECPKG_SUPPLEMENTAL_CRED_ARRAY;

typedef struct _SECPKG_PARAMETERS {
    ULONG Version;
    ULONG MachineState;
    ULONG SetupMode;
    PSID DomainSid;
    UNICODE_STRING DomainName;
    UNICODE_STRING DnsDomainName;
    GUID DomainGuid;
} SECPKG_PARAMETERS, *PSECPKG_PARAMETERS,
  SECPKG_EVENT_DOMAIN_CHANGE, *PSECPKG_EVENT_DOMAIN_CHANGE;

typedef struct _SECPKG_CLIENT_INFO {
    LUID LogonId;
    ULONG ProcessID;
    ULONG ThreadID;
    BOOLEAN HasTcbPrivilege;
    BOOLEAN Impersonating;
    BOOLEAN Restricted;
} SECPKG_CLIENT_INFO,
 *PSECPKG_CLIENT_INFO;

typedef struct _SECURITY_USER_DATA {
    UNICODE_STRING UserName;
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING LogonServer;
    PSID pSid;
} SECURITY_USER_DATA, *PSECURITY_USER_DATA,
  SecurityUserData, *PSecurityUserData;

typedef struct _SECPKG_GSS_INFO {
    ULONG EncodedIdLength;
    UCHAR EncodedId[4];
} SECPKG_GSS_INFO, *PSECPKG_GSS_INFO;

typedef struct _SECPKG_CONTEXT_THUNKS {
    ULONG InfoLevelCount;
    ULONG Levels[1];
} SECPKG_CONTEXT_THUNKS, *PSECPKG_CONTEXT_THUNKS;

typedef struct _SECPKG_MUTUAL_AUTH_LEVEL {
    ULONG MutualAuthLevel;
} SECPKG_MUTUAL_AUTH_LEVEL, *PSECPKG_MUTUAL_AUTH_LEVEL;

typedef struct _SECPKG_CALL_INFO {
    ULONG ProcessId;
    ULONG ThreadId;
    ULONG Attributes;
    ULONG CallCount;
} SECPKG_CALL_INFO, *PSECPKG_CALL_INFO;

typedef struct _SECPKG_EXTENDED_INFORMATION {
    SECPKG_EXTENDED_INFORMATION_CLASS Class;
    union {
        SECPKG_GSS_INFO GssInfo;
        SECPKG_CONTEXT_THUNKS ContextThunks;
        SECPKG_MUTUAL_AUTH_LEVEL MutualAuthLevel;
    } Info;
} SECPKG_EXTENDED_INFORMATION, *PSECPKG_EXTENDED_INFORMATION;

/* callbacks implemented by SSP/AP dlls and called by the LSA */
typedef VOID (NTAPI *PLSA_CALLBACK_FUNCTION)(ULONG_PTR, ULONG_PTR, PSecBuffer,
 PSecBuffer);

/* misc typedefs used in the below prototypes */
typedef PVOID *PLSA_CLIENT_REQUEST;
typedef ULONG LSA_SEC_HANDLE, *PLSA_SEC_HANDLE;
typedef LPTHREAD_START_ROUTINE SEC_THREAD_START;
typedef PSECURITY_ATTRIBUTES SEC_ATTRS;

/* functions used by SSP/AP obtainable by dispatch tables */
typedef NTSTATUS (NTAPI *PLSA_REGISTER_CALLBACK)(ULONG, PLSA_CALLBACK_FUNCTION);
typedef NTSTATUS (NTAPI *PLSA_CREATE_LOGON_SESSION)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_DELETE_LOGON_SESSION)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_ADD_CREDENTIAL)(PLUID, ULONG, PLSA_STRING,
 PLSA_STRING);
typedef NTSTATUS (NTAPI *PLSA_GET_CREDENTIALS)(PLUID, ULONG, PULONG, BOOLEAN,
 PLSA_STRING, PULONG, PLSA_STRING);
typedef NTSTATUS (NTAPI *PLSA_DELETE_CREDENTIAL)(PLUID, ULONG, PLSA_STRING);
typedef PVOID (NTAPI *PLSA_ALLOCATE_LSA_HEAP)(ULONG);
typedef VOID (NTAPI *PLSA_FREE_LSA_HEAP)(PVOID);
typedef NTSTATUS (NTAPI *PLSA_ALLOCATE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST,
 ULONG, PVOID*);
typedef NTSTATUS (NTAPI *PLSA_FREE_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, PVOID);
typedef NTSTATUS (NTAPI *PLSA_COPY_TO_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST, ULONG,
 PVOID, PVOID);
typedef NTSTATUS (NTAPI *PLSA_COPY_FROM_CLIENT_BUFFER)(PLSA_CLIENT_REQUEST,
 ULONG, PVOID, PVOID);
typedef NTSTATUS (NTAPI *PLSA_IMPERSONATE_CLIENT)(void);
typedef NTSTATUS (NTAPI *PLSA_UNLOAD_PACKAGE)(void);
typedef NTSTATUS (NTAPI *PLSA_DUPLICATE_HANDLE)(HANDLE, PHANDLE);
typedef NTSTATUS (NTAPI *PLSA_SAVE_SUPPLEMENTAL_CREDENTIALS)(PLUID, ULONG,
 PVOID, BOOLEAN);
typedef HANDLE (NTAPI *PLSA_CREATE_THREAD)(SEC_ATTRS, ULONG, SEC_THREAD_START,
 PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI *PLSA_GET_CLIENT_INFO)(PSECPKG_CLIENT_INFO);
typedef HANDLE (NTAPI *PLSA_REGISTER_NOTIFICATION)(SEC_THREAD_START, PVOID,
 ULONG, ULONG, ULONG, ULONG, HANDLE);
typedef NTSTATUS (NTAPI *PLSA_CANCEL_NOTIFICATION)(HANDLE);
typedef NTSTATUS (NTAPI *PLSA_MAP_BUFFER)(PSecBuffer, PSecBuffer);
typedef NTSTATUS (NTAPI *PLSA_CREATE_TOKEN)(PLUID, PTOKEN_SOURCE,
 SECURITY_LOGON_TYPE, SECURITY_IMPERSONATION_LEVEL, LSA_TOKEN_INFORMATION_TYPE,
 PVOID, PTOKEN_GROUPS, PUNICODE_STRING, PUNICODE_STRING, PUNICODE_STRING,
 PUNICODE_STRING, PHANDLE, PNTSTATUS);
typedef VOID (NTAPI *PLSA_AUDIT_LOGON)(NTSTATUS, NTSTATUS, PUNICODE_STRING,
 PUNICODE_STRING, PUNICODE_STRING, OPTIONAL PSID, SECURITY_LOGON_TYPE,
 PTOKEN_SOURCE, PLUID);
typedef NTSTATUS (NTAPI *PLSA_CALL_PACKAGE)(PUNICODE_STRING, PVOID, ULONG,
 PVOID*, PULONG, PNTSTATUS);
typedef BOOLEAN (NTAPI *PLSA_GET_CALL_INFO)(PSECPKG_CALL_INFO);
typedef NTSTATUS (NTAPI *PLSA_CALL_PACKAGEEX)(PUNICODE_STRING, PVOID, PVOID,
 ULONG, PVOID*, PULONG, PNTSTATUS);
typedef PVOID (NTAPI *PLSA_CREATE_SHARED_MEMORY)(ULONG, ULONG);
typedef PVOID (NTAPI *PLSA_ALLOCATE_SHARED_MEMORY)(PVOID, ULONG);
typedef VOID (NTAPI *PLSA_FREE_SHARED_MEMORY)(PVOID, PVOID);
typedef BOOLEAN (NTAPI *PLSA_DELETE_SHARED_MEMORY)(PVOID);
typedef NTSTATUS (NTAPI *PLSA_OPEN_SAM_USER)(PUNICODE_STRING, SECPKG_NAME_TYPE,
 PUNICODE_STRING, BOOLEAN, ULONG, PVOID*);
typedef NTSTATUS (NTAPI *PLSA_GET_USER_CREDENTIALS)(PVOID, PVOID *, PULONG,
 PVOID *, PULONG);
typedef NTSTATUS (NTAPI *PLSA_GET_USER_AUTH_DATA)(PVOID, PUCHAR *, PULONG);
typedef NTSTATUS (NTAPI *PLSA_CLOSE_SAM_USER)(PVOID);
typedef NTSTATUS (NTAPI *PLSA_CONVERT_AUTH_DATA_TO_TOKEN)(PVOID, ULONG,
 SECURITY_IMPERSONATION_LEVEL, PTOKEN_SOURCE, SECURITY_LOGON_TYPE,
 PUNICODE_STRING, PHANDLE, PLUID, PUNICODE_STRING, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_CLIENT_CALLBACK)(PCHAR, ULONG_PTR, ULONG_PTR,
 PSecBuffer, PSecBuffer);
typedef NTSTATUS (NTAPI *PLSA_UPDATE_PRIMARY_CREDENTIALS)(PSECPKG_PRIMARY_CRED, PSECPKG_SUPPLEMENTAL_CRED_ARRAY);
typedef NTSTATUS (NTAPI *PLSA_GET_AUTH_DATA_FOR_USER)(PUNICODE_STRING,
 SECPKG_NAME_TYPE, PUNICODE_STRING, PUCHAR *, PULONG, PUNICODE_STRING);
typedef NTSTATUS (NTAPI *PLSA_CRACK_SINGLE_NAME)(ULONG, BOOLEAN,
 PUNICODE_STRING, PUNICODE_STRING, ULONG, PUNICODE_STRING, PUNICODE_STRING,
 PULONG);
typedef NTSTATUS (NTAPI *PLSA_AUDIT_ACCOUNT_LOGON)(ULONG, BOOLEAN,
 PUNICODE_STRING, PUNICODE_STRING, PUNICODE_STRING, NTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_CALL_PACKAGE_PASSTHROUGH)(PUNICODE_STRING, PVOID,
 PVOID, ULONG, PVOID*, PULONG, PNTSTATUS);

/* Dispatch tables of functions used by SSP/AP */
typedef struct SECPKG_DLL_FUNCTIONS {
    PLSA_ALLOCATE_LSA_HEAP AllocateHeap;
    PLSA_FREE_LSA_HEAP FreeHeap;
    PLSA_REGISTER_CALLBACK RegisterCallback;
} SECPKG_DLL_FUNCTIONS,
 *PSECPKG_DLL_FUNCTIONS;

typedef struct LSA_DISPATCH_TABLE {
    PLSA_CREATE_LOGON_SESSION CreateLogonSession;
    PLSA_DELETE_LOGON_SESSION DeleteLogonSession;
    PLSA_ADD_CREDENTIAL AddCredential;
    PLSA_GET_CREDENTIALS GetCredentials;
    PLSA_DELETE_CREDENTIAL DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
    PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
    PLSA_COPY_TO_CLIENT_BUFFER CopyToClientBuffer;
    PLSA_COPY_FROM_CLIENT_BUFFER CopyFromClientBuffer;
} LSA_DISPATCH_TABLE,
 *PLSA_DISPATCH_TABLE;

typedef struct _LSA_SECPKG_FUNCTION_TABLE {
    PLSA_CREATE_LOGON_SESSION CreateLogonSession;
    PLSA_DELETE_LOGON_SESSION DeleteLogonSession;
    PLSA_ADD_CREDENTIAL AddCredential;
    PLSA_GET_CREDENTIALS GetCredentials;
    PLSA_DELETE_CREDENTIAL DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
    PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
    PLSA_COPY_TO_CLIENT_BUFFER CopyToClientBuffer;
    PLSA_COPY_FROM_CLIENT_BUFFER CopyFromClientBuffer;
    PLSA_IMPERSONATE_CLIENT ImpersonateClient;
    PLSA_UNLOAD_PACKAGE UnloadPackage;
    PLSA_DUPLICATE_HANDLE DuplicateHandle;
    PLSA_SAVE_SUPPLEMENTAL_CREDENTIALS SaveSupplementalCredentials;
    PLSA_CREATE_THREAD CreateThread;
    PLSA_GET_CLIENT_INFO GetClientInfo;
    PLSA_REGISTER_NOTIFICATION RegisterNotification;
    PLSA_CANCEL_NOTIFICATION CancelNotification;
    PLSA_MAP_BUFFER MapBuffer;
    PLSA_CREATE_TOKEN CreateToken;
    PLSA_AUDIT_LOGON AuditLogon;
    PLSA_CALL_PACKAGE CallPackage;
    PLSA_FREE_LSA_HEAP FreeReturnBuffer;
    PLSA_GET_CALL_INFO GetCallInfo;
    PLSA_CALL_PACKAGEEX CallPackageEx;
    PLSA_CREATE_SHARED_MEMORY CreateSharedMemory;
    PLSA_ALLOCATE_SHARED_MEMORY AllocateSharedMemory;
    PLSA_FREE_SHARED_MEMORY FreeSharedMemory;
    PLSA_DELETE_SHARED_MEMORY DeleteSharedMemory;
    PLSA_OPEN_SAM_USER OpenSamUser;
    PLSA_GET_USER_CREDENTIALS GetUserCredentials;
    PLSA_GET_USER_AUTH_DATA GetUserAuthData;
    PLSA_CLOSE_SAM_USER CloseSamUser;
    PLSA_CONVERT_AUTH_DATA_TO_TOKEN ConvertAuthDataToToken;
    PLSA_CLIENT_CALLBACK ClientCallback;
    PLSA_UPDATE_PRIMARY_CREDENTIALS UpdateCredentials;
    PLSA_GET_AUTH_DATA_FOR_USER GetAuthDataForUser;
    PLSA_CRACK_SINGLE_NAME CrackSingleName;
    PLSA_AUDIT_ACCOUNT_LOGON AuditAccountLogon;
    PLSA_CALL_PACKAGE_PASSTHROUGH CallPackagePassthrough;
} LSA_SECPKG_FUNCTION_TABLE,
 *PLSA_SECPKG_FUNCTION_TABLE;

/* LSA-mode functions implemented by SSP/AP obtainable by a dispatch table */
typedef NTSTATUS (NTAPI *PLSA_AP_INITIALIZE_PACKAGE)(ULONG, PLSA_DISPATCH_TABLE,
 PLSA_STRING, PLSA_STRING, PLSA_STRING *);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER)(LPWSTR, LPWSTR, LPWSTR, LPWSTR,
 DWORD, DWORD, PHANDLE);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE)(PUNICODE_STRING, PVOID, ULONG,
 PVOID *, PULONG, PNTSTATUS);
typedef VOID (NTAPI *PLSA_AP_LOGON_TERMINATED)(PLUID);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_UNTRUSTED)(PLSA_CLIENT_REQUEST,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_CALL_PACKAGE_PASSTHROUGH)(PUNICODE_STRING,
 PVOID, PVOID, ULONG, PVOID *, PULONG, PNTSTATUS);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX)(PLSA_CLIENT_REQUEST,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PLSA_TOKEN_INFORMATION_TYPE, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *);
typedef NTSTATUS (NTAPI *PLSA_AP_LOGON_USER_EX2)(PLSA_CLIENT_REQUEST,
 SECURITY_LOGON_TYPE, PVOID, PVOID, ULONG, PVOID *, PULONG, PLUID, PNTSTATUS,
 PLSA_TOKEN_INFORMATION_TYPE, PVOID *, PUNICODE_STRING *, PUNICODE_STRING *,
 PUNICODE_STRING *, PSECPKG_PRIMARY_CRED, PSECPKG_SUPPLEMENTAL_CRED_ARRAY *);
typedef NTSTATUS (SpInitializeFn)(ULONG_PTR, PSECPKG_PARAMETERS,
 PLSA_SECPKG_FUNCTION_TABLE);
typedef NTSTATUS (NTAPI SpShutDownFn)(void);
typedef NTSTATUS (NTAPI SpGetInfoFn)(PSecPkgInfoW);
typedef NTSTATUS (NTAPI SpAcceptCredentialsFn)(SECURITY_LOGON_TYPE,
 PUNICODE_STRING, PSECPKG_PRIMARY_CRED, PSECPKG_SUPPLEMENTAL_CRED);
typedef NTSTATUS (NTAPI SpAcquireCredentialsHandleFn)(PUNICODE_STRING, ULONG,
 PLUID, PVOID, PVOID, PVOID, PLSA_SEC_HANDLE, PTimeStamp);
typedef NTSTATUS (NTAPI SpQueryCredentialsAttributesFn)(LSA_SEC_HANDLE, ULONG,
 PVOID);
typedef NTSTATUS (NTAPI SpFreeCredentialsHandleFn)(LSA_SEC_HANDLE);
typedef NTSTATUS (NTAPI SpSaveCredentialsFn)(LSA_SEC_HANDLE, PSecBuffer);
typedef NTSTATUS (NTAPI SpGetCredentialsFn)(LSA_SEC_HANDLE, PSecBuffer);
typedef NTSTATUS (NTAPI SpDeleteCredentialsFn)(LSA_SEC_HANDLE, PSecBuffer);
typedef NTSTATUS (NTAPI SpInitLsaModeContextFn)(LSA_SEC_HANDLE, LSA_SEC_HANDLE,
 PUNICODE_STRING, ULONG, ULONG, PSecBufferDesc, PLSA_SEC_HANDLE, PSecBufferDesc,
 PULONG, PTimeStamp, PBOOLEAN, PSecBuffer);
typedef NTSTATUS (NTAPI SpAcceptLsaModeContextFn)(LSA_SEC_HANDLE,
 LSA_SEC_HANDLE, PSecBufferDesc, ULONG, ULONG, PLSA_SEC_HANDLE, PSecBufferDesc,
 PULONG, PTimeStamp, PBOOLEAN, PSecBuffer);
typedef NTSTATUS (NTAPI SpDeleteContextFn)(LSA_SEC_HANDLE);
typedef NTSTATUS (NTAPI SpApplyControlTokenFn)(LSA_SEC_HANDLE, PSecBufferDesc);
typedef NTSTATUS (NTAPI SpGetUserInfoFn)(PLUID, ULONG, PSecurityUserData *);
typedef NTSTATUS (NTAPI SpGetExtendedInformationFn)(
 SECPKG_EXTENDED_INFORMATION_CLASS, PSECPKG_EXTENDED_INFORMATION *);
typedef NTSTATUS (NTAPI SpQueryContextAttributesFn)(LSA_SEC_HANDLE, ULONG,
 PVOID);
typedef NTSTATUS (NTAPI SpAddCredentialsFn)(LSA_SEC_HANDLE, PUNICODE_STRING,
 PUNICODE_STRING, ULONG, PVOID, PVOID, PVOID, PTimeStamp);
typedef NTSTATUS (NTAPI SpSetExtendedInformationFn)(
 SECPKG_EXTENDED_INFORMATION_CLASS, PSECPKG_EXTENDED_INFORMATION);
typedef NTSTATUS (NTAPI SpSetContextAttributesFn)(LSA_SEC_HANDLE, ULONG, PVOID,
 ULONG);
typedef NTSTATUS (NTAPI SpSetCredentialsAttributesFn)(LSA_SEC_HANDLE, ULONG,
 PVOID, ULONG);

/* User-mode functions implemented by SSP/AP obtainable by a dispatch table */
typedef NTSTATUS (NTAPI SpInstanceInitFn)(ULONG, PSECPKG_DLL_FUNCTIONS,
 PVOID *);
typedef NTSTATUS (NTAPI SpInitUserModeContextFn)(LSA_SEC_HANDLE, PSecBuffer);
typedef NTSTATUS (NTAPI SpMakeSignatureFn)(LSA_SEC_HANDLE, ULONG,
 PSecBufferDesc, ULONG);
typedef NTSTATUS (NTAPI SpVerifySignatureFn)(LSA_SEC_HANDLE, PSecBufferDesc,
 ULONG, PULONG);
typedef NTSTATUS (NTAPI SpSealMessageFn)(LSA_SEC_HANDLE, ULONG, PSecBufferDesc,
 ULONG);
typedef NTSTATUS (NTAPI SpUnsealMessageFn)(LSA_SEC_HANDLE, PSecBufferDesc,
 ULONG, PULONG);
typedef NTSTATUS (NTAPI SpGetContextTokenFn)(LSA_SEC_HANDLE, PHANDLE);
typedef NTSTATUS (NTAPI SpCompleteAuthTokenFn)(LSA_SEC_HANDLE, PSecBufferDesc);
typedef NTSTATUS (NTAPI SpFormatCredentialsFn)(PSecBuffer, PSecBuffer);
typedef NTSTATUS (NTAPI SpMarshallSupplementalCredsFn)(ULONG, PUCHAR, PULONG,
 PVOID *);
typedef NTSTATUS (NTAPI SpExportSecurityContextFn)(LSA_SEC_HANDLE, ULONG,
 PSecBuffer, PHANDLE);
typedef NTSTATUS (NTAPI SpImportSecurityContextFn)(PSecBuffer, HANDLE,
 PLSA_SEC_HANDLE);

#ifdef WINE_NO_UNICODE_MACROS
#undef SetContextAttributes
#endif

/* dispatch tables of LSA-mode functions implemented by SSP/AP */
typedef struct SECPKG_FUNCTION_TABLE {
    PLSA_AP_INITIALIZE_PACKAGE InitializePackage;
    PLSA_AP_LOGON_USER LsaLogonUser;
    PLSA_AP_CALL_PACKAGE CallPackage;
    PLSA_AP_LOGON_TERMINATED LogonTerminated;
    PLSA_AP_CALL_PACKAGE_UNTRUSTED CallPackageUntrusted;
    PLSA_AP_CALL_PACKAGE_PASSTHROUGH CallPackagePassthrough;
    PLSA_AP_LOGON_USER_EX LogonUserEx;
    PLSA_AP_LOGON_USER_EX2 LogonUserEx2;
    SpInitializeFn *Initialize;
    SpShutDownFn *Shutdown;
    SpGetInfoFn *GetInfo;
    SpAcceptCredentialsFn *AcceptCredentials;
    SpAcquireCredentialsHandleFn *SpAcquireCredentialsHandle;
    SpQueryCredentialsAttributesFn *SpQueryCredentialsAttributes;
    SpFreeCredentialsHandleFn *FreeCredentialsHandle;
    SpSaveCredentialsFn *SaveCredentials;
    SpGetCredentialsFn *GetCredentials;
    SpDeleteCredentialsFn *DeleteCredentials;
    SpInitLsaModeContextFn *InitLsaModeContext;
    SpAcceptLsaModeContextFn *AcceptLsaModeContext;
    SpDeleteContextFn *DeleteContext;
    SpApplyControlTokenFn *ApplyControlToken;
    SpGetUserInfoFn *GetUserInfo;
    SpGetExtendedInformationFn *GetExtendedInformation;
    SpQueryContextAttributesFn *SpQueryContextAttributes;
    SpAddCredentialsFn *SpAddCredentials;
    SpSetExtendedInformationFn *SetExtendedInformation;
    /* Packages with version SECPKG_INTERFACE_VERSION end here */
    SpSetContextAttributesFn *SetContextAttributes;
    /* Packages with version SECPKG_INTERFACE_VERSION_2 end here */
    SpSetCredentialsAttributesFn *SetCredentialsAttributes;
    /* Packages with version SECPKG_INTERFACE_VERSION_3 end here */
} SECPKG_FUNCTION_TABLE,
 *PSECPKG_FUNCTION_TABLE;

/* dispatch tables of user-mode functions implemented by SSP/AP */
typedef struct SECPKG_USER_FUNCTION_TABLE {
    SpInstanceInitFn *InstanceInit;
    SpInitUserModeContextFn *InitUserModeContext;
    SpMakeSignatureFn *MakeSignature;
    SpVerifySignatureFn *VerifySignature;
    SpSealMessageFn *SealMessage;
    SpUnsealMessageFn *UnsealMessage;
    SpGetContextTokenFn *GetContextToken;
    SpQueryContextAttributesFn *SpQueryContextAttributes;
    SpCompleteAuthTokenFn *CompleteAuthToken;
    SpDeleteContextFn *DeleteUserModeContext;
    SpFormatCredentialsFn *FormatCredentials;
    SpMarshallSupplementalCredsFn *MarshallSupplementalCreds;
    SpExportSecurityContextFn *ExportContext;
    SpImportSecurityContextFn *ImportContext;
} SECPKG_USER_FUNCTION_TABLE,
 *PSECPKG_USER_FUNCTION_TABLE;

/* LSA-mode entry point to SSP/APs */
typedef NTSTATUS (NTAPI *SpLsaModeInitializeFn)(ULONG, PULONG,
 PSECPKG_FUNCTION_TABLE *, PULONG);

/* User-mode entry point to SSP/APs */
typedef NTSTATUS (WINAPI *SpUserModeInitializeFn)(ULONG, PULONG,
 PSECPKG_USER_FUNCTION_TABLE *, PULONG);

#define ISC_REQ_DELEGATE 1
#define ISC_REQ_MUTUAL_AUTH 2
#define ISC_REQ_REPLAY_DETECT 4
#define ISC_REQ_SEQUENCE_DETECT 8
#define ISC_REQ_CONFIDENTIALITY  16
#define ISC_REQ_USE_SESSION_KEY 32
#define ISC_REQ_PROMPT_FOR_CREDS 64
#define ISC_REQ_USE_SUPPLIED_CREDS  128
#define ISC_REQ_ALLOCATE_MEMORY 256
#define ISC_REQ_USE_DCE_STYLE 512
#define ISC_REQ_DATAGRAM 1024
#define ISC_REQ_CONNECTION 2048
#define ISC_REQ_EXTENDED_ERROR 16384
#define ISC_REQ_STREAM 32768
#define ISC_REQ_INTEGRITY 65536
#define ISC_REQ_MANUAL_CRED_VALIDATION 524288
#define ISC_REQ_HTTP  268435456

#define ISC_RET_EXTENDED_ERROR 16384

#define ASC_REQ_DELEGATE 1
#define ASC_REQ_MUTUAL_AUTH 2
#define ASC_REQ_REPLAY_DETECT 4
#define ASC_REQ_SEQUENCE_DETECT 8
#define ASC_REQ_CONFIDENTIALITY 16
#define ASC_REQ_USE_SESSION_KEY 32
#define ASC_REQ_ALLOCATE_MEMORY 256
#define ASC_REQ_USE_DCE_STYLE 512
#define ASC_REQ_DATAGRAM 1024
#define ASC_REQ_CONNECTION 2048
#define ASC_REQ_EXTENDED_ERROR 32768
#define ASC_REQ_STREAM 65536
#define ASC_REQ_INTEGRITY 131072

#define SECURITY_NATIVE_DREP  16
#define SECURITY_NETWORK_DREP 0

#endif /* _NTSECPKG_H */
