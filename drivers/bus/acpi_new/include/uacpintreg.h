/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     uACPI-NT Registry handling code
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

NTSTATUS
NTAPI
UacpiNtRegQueryValue(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ PULONG Type,
    _Out_writes_bytes_to_opt_(*DataLength, *DataLength) PVOID Data,
    _Inout_opt_ PULONG DataLength);

NTSTATUS
NTAPI
UacpiNtRegOpenKey(
    _In_opt_ HANDLE ParentKeyHandle,
    _In_z_ PCWSTR KeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE KeyHandle);

/* Leaves Value untouched unless a REG_DWORD was read */
NTSTATUS
NTAPI
UacpiNtRegQueryUlong(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Inout_ PULONG Value);

BOOLEAN
NTAPI
UacpiNtRegQueryAnsiString(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Out_writes_z_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize);
