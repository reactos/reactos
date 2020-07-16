/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header for lsa.c
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#pragma once

NTSTATUS
NTAPI
SpInitialize(
    _In_ ULONG_PTR PackageId,
    _In_ PSECPKG_PARAMETERS Parameters,
    _In_ PLSA_SECPKG_FUNCTION_TABLE FunctionTable);

NTSTATUS
NTAPI
LsaSpShutDown(VOID);

NTSTATUS
NTAPI
SpAcceptCredentials(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING AccountName,
    _In_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
    _In_ PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials);

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
    _Out_ PTimeStamp ExpirationTime);

NTSTATUS
NTAPI
LsaSpQueryCredentialsAttributes(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ ULONG CredentialAttribute,
    _Inout_ PVOID Buffer);

NTSTATUS
NTAPI
LsaSpFreeCredentialsHandle(
    _In_ LSA_SEC_HANDLE CredentialHandle);

NTSTATUS
NTAPI
LsaSpSaveCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ PSecBuffer Credentials);

NTSTATUS
NTAPI
LsaSpGetCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _Inout_ PSecBuffer Credentials);

NTSTATUS
NTAPI
LsaSpDeleteCredentials(
    _In_ LSA_SEC_HANDLE CredentialHandle,
    _In_ PSecBuffer Key);

NTSTATUS
NTAPI
LsaSpGetInfoW(
    _Out_ PSecPkgInfoW PackageInfo);

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
    _Out_ PSecBuffer ContextData);

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
    _Out_ PSecBuffer ContextData);

NTSTATUS
NTAPI
LsaSpDeleteContext(
    _In_ LSA_SEC_HANDLE ContextHandle);

NTSTATUS
NTAPI
LsaSpApplyControlToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBufferDesc ControlToken);

NTSTATUS
NTAPI
LsaSpGetUserInfo(
    _In_ PLUID LogonId,
    _In_ ULONG Flags,
    _Out_ PSecurityUserData *UserData);

NTSTATUS
NTAPI
LsaSpGetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _Out_ PSECPKG_EXTENDED_INFORMATION *ppInfo);

NTSTATUS
NTAPI
LsaSpSetExtendedInformation(
    _In_ SECPKG_EXTENDED_INFORMATION_CLASS Class,
    _In_ PSECPKG_EXTENDED_INFORMATION Info);
