/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utils for msv1_0 (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier <staubim@quantentunnel.de>
 */

#pragma once

bool
NtlmUStrAlloc(
    _Out_ PUNICODE_STRING Dst,
    _In_ UINT16 SizeInBytes,
    _In_ UINT16 InitLength);

VOID
NtlmUStrFree(
    _In_ PUNICODE_STRING String);

bool
NtlmUStrWriteToStruct(
    _In_ PVOID DataStart,
    _In_ ULONG DataSize,
    _Out_ PUNICODE_STRING DstData,
    _In_ const PUNICODE_STRING SrcData,
    _In_ OUT PBYTE* AbsoluteOffsetPtr,
    _In_ bool TerminateWith0);

/* misc */
bool
NtlmFixupAndValidateUStr(
    _Inout_ PUNICODE_STRING String,
    _In_ ULONG_PTR FixupOffset);

bool
NtlmFixupAStr(
    _Inout_ PSTRING String,
    _In_ ULONG_PTR FixupOffset);

/* ClientBuffer */
typedef struct _NTLM_CLIENT_BUFFER
{
    PVOID ClientBaseAddress;
    PVOID LocalBuffer;
} NTLM_CLIENT_BUFFER, *PNTLM_CLIENT_BUFFER;

NTSTATUS
NtlmAllocateClientBuffer(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ ULONG BufferLength,
    _Inout_ PNTLM_CLIENT_BUFFER Buffer);

NTSTATUS
NtlmCopyToClientBuffer(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ ULONG BufferLength,
    _In_ OUT PNTLM_CLIENT_BUFFER Buffer);

VOID
NtlmFreeClientBuffer(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ bool FreeClientBuffer,
    _Inout_ PNTLM_CLIENT_BUFFER Buffer);
