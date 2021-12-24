/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NTLM Functions returned from SpUserModeInitialize (PSECPKG_USER_FUNCTION_TABLE)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

NTSTATUS
NTAPI
SpInstanceInit(
    _In_ ULONG Version,
    _In_ PSECPKG_DLL_FUNCTIONS FunctionTable,
    _Inout_ PVOID *UserFunctions)
{
    TRACE("SpInstanceInit(Version 0x%lx, 0x%p, 0x%p)\n",
          Version, FunctionTable, UserFunctions);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpMakeSignature(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG QualityOfProtection,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber)
{
    TRACE("UsrSpMakeSignature(0x%p 0x%x 0x%p 0x%x)\n",
          ContextHandle, QualityOfProtection,
          MessageBuffers, MessageSequenceNumber);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpVerifySignature(
    _In_ LSA_SEC_HANDLE phContext,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo,
    _In_ PULONG pfQOP)
{
    TRACE("UsrSpVerifySignature(0x%p 0x%x 0x%x 0x%p)\n",
          phContext, pMessage, MessageSeqNo, pfQOP);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
UsrSpSealMessage(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG QualityOfProtection,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber)
{
    TRACE("UsrSpSealMessage(0x%p 0x%x 0x%p 0x%x)\n",
          ContextHandle, QualityOfProtection,
          MessageBuffers, MessageSequenceNumber);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpUnsealMessage(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _Inout_ PSecBufferDesc MessageBuffers,
    _In_ ULONG MessageSequenceNumber,
    _In_ PULONG QualityOfProtection)
{
    TRACE("UsrSpUnsealMessage(0x%p 0x%x 0x%p 0x%x)\n",
          ContextHandle, MessageBuffers,
          MessageSequenceNumber, QualityOfProtection);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpGetContextToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _Inout_ PHANDLE ImpersonationToken)
{
    TRACE("UsrSpGetContextToken(0x%p 0x%p)\n",
          ContextHandle, ImpersonationToken);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpQueryContextAttributes(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ ULONG ContextAttribute,
    _Inout_ PVOID Buffer)
{
    TRACE("UsrSpQueryContextAttributes(0x%p 0x%x 0x%p)\n",
            ContextHandle, ContextAttribute, Buffer);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpCompleteAuthToken(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBufferDesc InputBuffer)
{
    TRACE("UsrSpCompleteAuthToken(0x%p 0x%p)\n",
          ContextHandle, InputBuffer);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpDeleteUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle)
{
    TRACE("UsrSpDeleteUserModeContext(0x%p)\n",
          ContextHandle);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsrSpFormatCredentials(
    _In_ PSecBuffer Credentials,
    _Inout_ PSecBuffer FormattedCredentials)
{
    TRACE("UsrSpFormatCredentials(0x%p 0x%p)\n",
          Credentials, FormattedCredentials);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
UsrSpMarshallSupplementalCreds(
    _In_ ULONG CredentialSize,
    _In_ PUCHAR Credentials,
    _Inout_ PULONG MarshalledCredSize,
    _Inout_ PVOID *MarshalledCreds)
{
    TRACE("UsrSpMarshallSupplementalCreds(0x%x 0x%p 0x%p 0x%p)\n",
          CredentialSize, Credentials, MarshalledCredSize, MarshalledCreds);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
UsrSpExportSecurityContext(
    _In_ LSA_SEC_HANDLE phContext,
    _In_ ULONG fFlags,
    _Inout_ PSecBuffer pPackedContext,
    _Inout_ PHANDLE pToken)
{
    TRACE("UsrSpExportSecurityContext(0x%p 0x%x 0x%p 0x%p)\n",
          phContext, fFlags, pPackedContext, pToken);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
UsrSpImportSecurityContext(
    _In_ PSecBuffer pPackedContext,
    _In_ HANDLE Token,
    _Inout_ PLSA_SEC_HANDLE phContext)
{
    TRACE("UsrSpImportSecurityContext(0x%p 0x%x 0x%p)\n",
          pPackedContext, Token, phContext);

    return ERROR_NOT_SUPPORTED;
}
