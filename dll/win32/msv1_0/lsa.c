/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NTLM functions returned from SpLsaModeInitialize (PSECPKG_FUNCTION_TABLE)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);


NTSTATUS
NTAPI
SpInitialize(
    _In_ ULONG_PTR PackageId,
    _In_ PSECPKG_PARAMETERS Parameters,
    _In_ PLSA_SECPKG_FUNCTION_TABLE FunctionTable)
{
    TRACE("LsaSpInitialize (0x%p, 0x%p, 0x%p)\n",
          PackageId, Parameters, FunctionTable);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpShutDown(VOID)
{
    TRACE("LsaSpShutDown\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* MS doc says name must be SpAcceptCredentials! */
NTSTATUS
NTAPI
SpAcceptCredentials(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING AccountName,
    _In_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
    _In_ PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials)
{
    TRACE("LsaSpAcceptCredentials(%li %wZ 0x%p 0x%p)\n",
          LogonType, AccountName, PrimaryCredentials, SupplementalCredentials);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpAcquireCredentialsHandle(
    _In_ PUNICODE_STRING PrincipalName,
    _In_ ULONG CredentialUseFlags,
    _In_ PLUID LogonId,
    _In_ PVOID AuthorizationData,
    _In_ PVOID GetKeyFunciton,
    _In_ PVOID GetKeyArgument,
    _Out_ PLSA_SEC_HANDLE CredentialHandle,
    _Out_ PTimeStamp ExpirationTime)
{
    TRACE("LsaSpAcquireCredentialsHandle(%wZ 0x%lx 0x%p 0x%p 0x%p 0x%p 0x%p 0x%p)\n",
          PrincipalName, CredentialUseFlags, LogonId,
          AuthorizationData, GetKeyFunciton, GetKeyArgument,
          CredentialHandle, ExpirationTime);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpQueryCredentialsAttributes(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ ULONG CredentialAttribute,
    _Inout_ PVOID Buffer)
{
    TRACE("LsaSpQueryCredentialsAttributes(0x%p 0x%lx 0x%p)\n",
          CredentialHandle, CredentialAttribute, Buffer);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpFreeCredentialsHandle(
    _In_ LSA_SEC_HANDLE CredentialHandle)
{
    TRACE("LsaSpFreeCredentialsHandle(0x%p)", CredentialHandle);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpSaveCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ PSecBuffer Credentials)
{
    TRACE("LsaSpSaveCredentials(0x%p 0x%p)\n", CredentialHandle, Credentials);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpGetCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _Inout_ PSecBuffer Credentials)
{
    TRACE("LsaSpGetCredentials(0x%p 0x%p)\n", CredentialHandle, Credentials);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpDeleteCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ PSecBuffer Key)
{
    TRACE("LsaSpDeleteCredentials(0x%p 0x%p)\n", CredentialHandle, Key);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpGetInfoW(
    _Out_ PSecPkgInfoW PackageInfo)
{
    TRACE("LsaGetInfo(0x%p)\n", PackageInfo);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpInitLsaModeContext(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PUNICODE_STRING TargetName,
    _In_ ULONG ContextRequirements,
    _In_ ULONG TargetDataRep,
    _In_ PSecBufferDesc InputBuffers,
    _Out_ PLSA_SEC_HANDLE NewContextHandle,
    _Inout_ PSecBufferDesc OutputBuffers,
    _Out_ PULONG ContextAttributes,
    _Out_ PTimeStamp ExpirationTime,
    _Out_ PBOOLEAN MappedContext,
    _Out_ PSecBuffer ContextData)
{
    TRACE("LsaSpInitLsaModeContext(0x%p 0x%p %wZ 0x%lx %i 0x%p 0x%p 0x%p "
          "0x%p 0x%p 0x%p 0x%p 0x%p 0x%p 0x%p 0x%p)\n",
          CredentialHandle, ContextHandle, TargetName,
          ContextRequirements, TargetDataRep, InputBuffers,
          NewContextHandle, OutputBuffers, ContextAttributes,
          ExpirationTime, MappedContext, ContextData);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpAcceptLsaModeContext(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBufferDesc InputBuffer,
    _In_ ULONG ContextRequirements,
    _In_ ULONG TargetDataRep,
    _Out_ PLSA_SEC_HANDLE NewContextHandle,
    _Inout_ PSecBufferDesc OutputBuffer,
    _Out_ PULONG ContextAttributes,
    _Out_ PTimeStamp ExpirationTime,
    _Out_ PBOOLEAN MappedContext,
    _Out_ PSecBuffer ContextData)
{
    TRACE("LsaSpAcceptLsaModeContext(0x%p 0x%p 0x%p %i %i 0x%p 0x%p 0x%p "
          "0x%p 0x%p 0x%p)\n",
          CredentialHandle, ContextHandle, InputBuffer, ContextRequirements,
          TargetDataRep, NewContextHandle, OutputBuffer,
          ContextAttributes, ExpirationTime, MappedContext, ContextData);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpDeleteContext(
    _In_ LSA_SEC_HANDLE ContextHandle)
{
    TRACE("LsaSpDeleteContext(0x%p)\n", ContextHandle);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpApplyControlToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBufferDesc ControlToken)
{
    TRACE("LsaSpApplyControlToken(0x%p 0x%p)\n", ContextHandle, ControlToken);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpGetUserInfo(
    _In_ PLUID LogonId,
    _In_ ULONG Flags,
    _Out_ PSecurityUserData *UserData)
{
    TRACE("LsaSpGetUserInfo(0x%p 0x%lx 0x%p)\n", LogonId, Flags, UserData);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpGetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _Out_ PSECPKG_EXTENDED_INFORMATION *ppInfo)
{
    TRACE("LsaSpGetExtendedInformation(0x%lx 0x%p)\n",
          Class, ppInfo);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LsaSpSetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _In_ PSECPKG_EXTENDED_INFORMATION Info)
{
    TRACE("LsaSpSetExtendedInformation(0x%lx 0x%p)\n",
          Class, Info);
    return STATUS_NOT_IMPLEMENTED;
}
