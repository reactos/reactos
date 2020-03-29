#ifndef _SPLSA_H_
#define _SPLSA_H_

/* Version returned by (Lsa)SpGetInfo */
#define SECPKG_VERSION 1
#define SECPKG_ID_NONE 10
#define SECPKG_MAX_TOKEN_SIZE 1904

// functions provided by LSA in SpInitialize
extern PLSA_SECPKG_FUNCTION_TABLE LsaFunctions;
// functions we provide to LSA in SpLsaModeInitialize
extern SECPKG_FUNCTION_TABLE NtlmLsaFn[1];

NTSTATUS NTAPI
LsaApLogonUser(
    LPWSTR p1,
    LPWSTR p2,
    LPWSTR p3,
    LPWSTR p4,
    DWORD p5,
    DWORD p6,
    PHANDLE p7);

NTSTATUS NTAPI
LsaApLogonUserEx(
    PLSA_CLIENT_REQUEST p1,
    SECURITY_LOGON_TYPE p2,
    PVOID p3,
    PVOID p4,
    ULONG p5,
    PVOID *p6,
    PULONG p7,
    PLUID p8,
    PNTSTATUS p9,
    PLSA_TOKEN_INFORMATION_TYPE p10,
    PVOID *p11,
    PUNICODE_STRING *p12,
    PUNICODE_STRING *p13,
    PUNICODE_STRING *p14);

NTSTATUS NTAPI
SpInitialize(
    _In_ ULONG_PTR PackageId,
    _In_ PSECPKG_PARAMETERS Parameters,
    _In_ PLSA_SECPKG_FUNCTION_TABLE FunctionTable);

NTSTATUS NTAPI
LsaSpShutDown(void);

NTSTATUS
NTAPI
SpAcceptCredentials(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING AccountName,
    _In_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
    _In_ PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials);

NTSTATUS NTAPI
LsaSpAcquireCredentialsHandle(
    PUNICODE_STRING PrincipalName,
    ULONG CredentialUseFlags,
    PLUID LogonId,
    PVOID AuthorizationData,
    PVOID GetKeyFunciton,
    PVOID GetKeyArgument,
    PLSA_SEC_HANDLE CredentialHandle,
    PTimeStamp ExpirationTime);

NTSTATUS NTAPI
LsaSpQueryCredentialsAttributes(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PVOID p3);

NTSTATUS NTAPI
LsaSpFreeCredentialsHandle(
    IN LSA_SEC_HANDLE CredentialHandle);

NTSTATUS NTAPI
LsaSpSaveCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2);

NTSTATUS NTAPI
LsaSpGetCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2);

NTSTATUS NTAPI
LsaSpDeleteCredentials(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2);

NTSTATUS
NTAPI
LsaSpGetInfoW(
    _Out_ PSecPkgInfoW PackageInfo);

NTSTATUS NTAPI
LsaSpInitLsaModeContext(
    LSA_SEC_HANDLE CredentialHandle,
    LSA_SEC_HANDLE ContextHandle,
    PUNICODE_STRING TargetName,
    ULONG ContextRequirements,
    ULONG TargetDataRep,
    PSecBufferDesc InputBuffers,
    PLSA_SEC_HANDLE NewContextHandle,
    PSecBufferDesc OutputBuffers,
    PULONG ContextAttributes,
    PTimeStamp ExpirationTime,
    PBOOLEAN MappedContext,
    PSecBuffer ContextData);

NTSTATUS NTAPI
LsaSpAcceptLsaModeContext(
    LSA_SEC_HANDLE CredentialHandle,
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc InputBuffer,
    ULONG ContextRequirements,
    ULONG TargetDataRep,
    PLSA_SEC_HANDLE NewContextHandle,
    PSecBufferDesc OutputBuffer,
    PULONG ContextAttributes,
    PTimeStamp ExpirationTime,
    PBOOLEAN MappedContext,
    PSecBuffer ContextData);

NTSTATUS NTAPI
LsaSpDeleteContext(
    LSA_SEC_HANDLE ContextHandle);

NTSTATUS NTAPI
LsaSpApplyControlToken(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2);

NTSTATUS NTAPI
LsaSpGetUserInfo(
    PLUID p1,
    ULONG p2,
    PSecurityUserData *p3);

NTSTATUS
NTAPI
LsaSpGetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _Out_ PSECPKG_EXTENDED_INFORMATION *ppInfo);

NTSTATUS NTAPI
LsaSpQueryContextAttributes(
    LSA_SEC_HANDLE CredentialHandle,
    ULONG CredentialAttribute,
    PVOID Buffer);

NTSTATUS NTAPI
LsaSpAddCredentials(
    LSA_SEC_HANDLE p1,
    PUNICODE_STRING p2,
    PUNICODE_STRING p3,
    ULONG p4,
    PVOID p5,
    PVOID p6,
    PVOID p7,
    PTimeStamp p8);

NTSTATUS NTAPI
LsaSpSetExtendedInformation(
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION pInfo);

#endif /* _SPLSA_H_ */
