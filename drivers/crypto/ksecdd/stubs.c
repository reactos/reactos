/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>

typedef PVOID PSECURITY_PACKAGE_OPTIONS, PSecurityUserData;

/* FUNCTIONS ******************************************************************/

SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(
    _In_opt_ PCredHandle phCredential,
    _In_opt_ PCtxtHandle phContext,
    _In_opt_ PSecBufferDesc pInput,
    _In_ ULONG fContextReq,
    _In_ ULONG TargetDataRep,
    _In_opt_ PCtxtHandle phNewContext,
    _In_opt_ PSecBufferDesc pOutput,
    _Out_ PULONG pfContextAttr,
    _Out_opt_ PTimeStamp ptsExpiry)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(
    _In_opt_ PSSPI_SEC_STRING pPrincipal,
    _In_ PSSPI_SEC_STRING pPackage,
    _In_ ULONG fCredentialUse,
    _In_opt_ PVOID pvLogonId,
    _In_opt_ PVOID pAuthData,
    _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
    _In_opt_ PVOID pvGetKeyArgument,
    _Out_ PCredHandle phCredential,
    _Out_opt_ PTimeStamp ptsExpiry)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
AddCredentialsW(
    _In_ PCredHandle hCredentials,
    _In_opt_ PSSPI_SEC_STRING pPrincipal,
    _In_ PSSPI_SEC_STRING pPackage,
    _In_ ULONG fCredentialUse,
    _In_opt_ PVOID pAuthData,
    _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
    _In_opt_ PVOID pvGetKeyArgument,
    _Out_opt_ PTimeStamp ptsExpiry)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(
    _In_ PCtxtHandle phContext,
    _In_ PSecBufferDesc pInput)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
CredMarshalTargetInfo(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(
    _In_ PCtxtHandle phContext)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
EfsDecryptFek(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
SEC_ENTRY
EfsGenerateKey(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(
    _Out_ PULONG pcPackages,
    _Deref_out_ PSecPkgInfoW* ppPackageInfo)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
ExportSecurityContext(
    _In_ PCtxtHandle phContext,
    _In_ ULONG fFlags,
    _Out_ PSecBuffer pPackedContext,
    _Out_ PVOID* pToken)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(
    _Inout_ PVOID pvContextBuffer)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(
    _In_ PCredHandle phCredential)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
GenerateDirEfs(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
SEC_ENTRY
GenerateSessionKey(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
GetSecurityUserInfo(
    _In_opt_    PLUID LogonId,
    _In_        ULONG Flags,
    _Outptr_    PSecurityUserData *UserInformation)
{
    UNIMPLEMENTED;
    *UserInformation = NULL;
    return STATUS_UNSUCCESSFUL;
}

SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(
    _In_ PCtxtHandle phContext)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
ImportSecurityContextW(
    _In_ PSSPI_SEC_STRING pszPackage,
    _In_ PSecBuffer pPackedContext,
    _In_ PVOID Token,
    _Out_ PCtxtHandle phContext)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(
    _In_opt_ PCredHandle phCredential,
    _In_opt_ PCtxtHandle phContext,
    _In_opt_ PSSPI_SEC_STRING pTargetName,
    _In_ ULONG fContextReq,
    _In_ ULONG Reserved1,
    _In_ ULONG TargetDataRep,
    _In_opt_ PSecBufferDesc pInput,
    _In_ ULONG Reserved2,
    _Inout_opt_ PCtxtHandle phNewContext,
    _Inout_opt_ PSecBufferDesc pOutput,
    _Out_ PULONG pfContextAttr,
    _Out_opt_ PTimeStamp ptsExpiry)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

PSecurityFunctionTableW
SEC_ENTRY
InitSecurityInterfaceW(void)
{

    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

VOID
SEC_ENTRY
KSecRegisterSecurityProvider(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
SEC_ENTRY
KSecValidateBuffer(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
SEC_ENTRY
LsaEnumerateLogonSessions(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
SEC_ENTRY
LsaGetLogonSessionData(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
MakeSignature(
    _In_ PCtxtHandle phContext,
    _In_ ULONG fQOP,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
MapSecurityError(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    _In_ PCtxtHandle phContext,
    _In_ ULONG ulAttribute,
    _Out_ PVOID pBuffer)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesW(
    _In_ PCredHandle phCredential,
    _In_ ULONG ulAttribute,
    _Inout_ PVOID pBuffer)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
    _In_ PCtxtHandle phContext,
    _Out_ PVOID* Token)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoW(
    _In_ PSSPI_SEC_STRING pPackageName,
    _Deref_out_ PSecPkgInfoW *ppPackageInfo)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(
    _In_ PCtxtHandle phContext)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
SealMessage(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

NTSTATUS
SEC_ENTRY
SecLookupAccountName(
    _In_ PUNICODE_STRING Name,
    _Inout_ PULONG SidSize,
    _Out_ PSID Sid,
    _Out_ PSID_NAME_USE NameUse,
    _Out_opt_ PULONG DomainSize,
    _Inout_opt_ PUNICODE_STRING ReferencedDomain)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

NTSTATUS
SEC_ENTRY
SecLookupAccountSid(
    _In_ PSID Sid,
    _Out_ PULONG NameSize,
    _Inout_ PUNICODE_STRING NameBuffer,
    _Out_ PULONG DomainSize OPTIONAL,
    _Out_opt_ PUNICODE_STRING DomainBuffer,
    _Out_ PSID_NAME_USE NameUse)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

NTSTATUS
SEC_ENTRY
SecLookupWellKnownSid(
    _In_ WELL_KNOWN_SID_TYPE SidType,
    _Out_ PSID Sid,
    _In_ ULONG SidBufferSize,
    _Inout_opt_ PULONG SidSize)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

NTSTATUS
NTAPI
SecMakeSPN(
    _In_ PUNICODE_STRING ServiceClass,
    _In_ PUNICODE_STRING ServiceName,
    _In_opt_ PUNICODE_STRING InstanceName,
    _In_opt_ USHORT InstancePort,
    _In_opt_ PUNICODE_STRING Referrer,
    _Inout_ PUNICODE_STRING Spn,
    _Out_opt_ PULONG Length,
    _In_ BOOLEAN Allocate)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

NTSTATUS
NTAPI
SecMakeSPNEx(
    _In_ PUNICODE_STRING ServiceClass,
    _In_ PUNICODE_STRING ServiceName,
    _In_opt_ PUNICODE_STRING InstanceName,
    _In_opt_ USHORT InstancePort,
    _In_opt_ PUNICODE_STRING Referrer,
    _In_opt_ PUNICODE_STRING TargetInfo,
    _Inout_ PUNICODE_STRING Spn,
    _Out_opt_ PULONG Length,
    _In_ BOOLEAN Allocate)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

VOID
SEC_ENTRY
SecSetPagingMode(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}
VOID
SEC_ENTRY
UnsealMessage(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
}

SECURITY_STATUS
SEC_ENTRY
VerifySignature(
    _In_ PCtxtHandle phContext,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo,
    _Out_ PULONG pfQOP)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}
