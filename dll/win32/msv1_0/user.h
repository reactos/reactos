/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header for user.c
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#pragma once

NTSTATUS
NTAPI
SpInstanceInit(
    _In_ ULONG Version,
    _In_ PSECPKG_DLL_FUNCTIONS FunctionTable,
    _Inout_ PVOID *UserFunctions);

NTSTATUS
NTAPI
UsrSpMakeSignature(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG QualityOfProtection,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber);

NTSTATUS
NTAPI
UsrSpVerifySignature(
    _In_ LSA_SEC_HANDLE phContext,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo,
    _In_ PULONG pfQOP);

NTSTATUS
NTAPI
UsrSpSealMessage(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG QualityOfProtection,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber);

NTSTATUS
NTAPI
UsrSpUnsealMessage(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber,
    _In_ PULONG QualityOfProtection);

NTSTATUS
NTAPI
UsrSpGetContextToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _Inout_ PHANDLE ImpersonationToken);

NTSTATUS
NTAPI
UsrSpQueryContextAttributes(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG ContextAttribute,
    _Inout_ PVOID Buffer);

NTSTATUS
NTAPI
UsrSpCompleteAuthToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBufferDesc InputBuffer);

NTSTATUS
NTAPI
UsrSpDeleteUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle);

NTSTATUS
NTAPI
UsrSpFormatCredentials(
    _In_ PSecBuffer Credentials,
    _Inout_ PSecBuffer FormattedCredentials);

NTSTATUS
NTAPI
UsrSpMarshallSupplementalCreds(
    _In_ ULONG CredentialSize,
    _In_ PUCHAR Credentials,
    _Inout_ PULONG MarshalledCredSize,
    _Inout_ PVOID *MarshalledCreds);

NTSTATUS
NTAPI
UsrSpExportSecurityContext(
    _In_ LSA_SEC_HANDLE phContext,
    _In_ ULONG fFlags,
    _Inout_ PSecBuffer pPackedContext,
    _Inout_ PHANDLE pToken);

NTSTATUS
NTAPI
UsrSpImportSecurityContext(
    _In_ PSecBuffer pPackedContext,
    _In_ HANDLE Token,
    _Inout_ PLSA_SEC_HANDLE phContext);
