/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NT6+ Firmware API
 * COPYRIGHT:   Copyright 2023-2024 Ratin Gao <ratin@knsoft.org>
 */

/* We need NT6+ Ex definitions */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VISTA
#include <ndk/exfuncs.h>

#include "k32_vista.h"

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize,
    _Out_opt_ PDWORD pdwAttribubutes)
{
    NTSTATUS Status;
    UNICODE_STRING VariableName, Namespace;
    GUID VendorGuid;
    ULONG Length;

    /* Check input parameters and build NT strings */
    if (!lpName || !lpGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    RtlInitUnicodeString(&VariableName, lpName);
    RtlInitUnicodeString(&Namespace, lpGuid);
    Status = RtlGUIDFromString(&Namespace, &VendorGuid);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Query firmware system environment variable value */
    Length = nSize;
    Status = NtQuerySystemEnvironmentValueEx(&VariableName,
                                             &VendorGuid,
                                             pBuffer,
                                             &Length,
                                             pdwAttribubutes);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return Length;
}

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize,
    _Out_opt_ PDWORD pdwAttribubutes)
{
    NTSTATUS Status;
    DWORD Length;
    UNICODE_STRING VariableName, Namespace;
    ANSI_STRING AnsiVariableName, AnsiNamespace;

    /* Check input parameters and build NT strings */
    if (!lpName || !lpGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    RtlInitString(&AnsiVariableName, lpName);
    Status = RtlAnsiStringToUnicodeString(&VariableName, &AnsiVariableName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }
    RtlInitString(&AnsiNamespace, lpGuid);
    Status = RtlAnsiStringToUnicodeString(&Namespace, &AnsiNamespace, TRUE);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&VariableName);
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Call unicode version interface */
    Length = GetFirmwareEnvironmentVariableExW(VariableName.Buffer,
                                               Namespace.Buffer,
                                               pBuffer,
                                               nSize,
                                               pdwAttribubutes);

    /* Cleanup and return */
    RtlFreeUnicodeString(&Namespace);
    RtlFreeUnicodeString(&VariableName);
    return Length;
}

BOOL
WINAPI
SetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize,
    _In_ DWORD dwAttributes)
{
    NTSTATUS Status;
    UNICODE_STRING VariableName, Namespace;
    GUID VendorGuid;

    /* Check input parameters and build NT strings */
    if (!lpName || !lpGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    RtlInitUnicodeString(&VariableName, lpName);
    RtlInitUnicodeString(&Namespace, lpGuid);
    Status = RtlGUIDFromString(&Namespace, &VendorGuid);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Set firmware system environment variable value */
    Status = NtSetSystemEnvironmentValueEx(&VariableName,
                                           &VendorGuid,
                                           pValue,
                                           nSize,
                                           dwAttributes);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
SetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize,
    _In_ DWORD dwAttributes)
{
    NTSTATUS Status;
    BOOL Result;
    UNICODE_STRING VariableName, Namespace;
    ANSI_STRING AnsiVariableName, AnsiNamespace;

    /* Check input parameters and build NT strings */
    if (!lpName || !lpGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    RtlInitString(&AnsiVariableName, lpName);
    Status = RtlAnsiStringToUnicodeString(&VariableName, &AnsiVariableName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    RtlInitString(&AnsiNamespace, lpGuid);
    Status = RtlAnsiStringToUnicodeString(&Namespace, &AnsiNamespace, TRUE);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&VariableName);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Call unicode version interface */
    Result = SetFirmwareEnvironmentVariableExW(VariableName.Buffer,
                                               Namespace.Buffer,
                                               pValue,
                                               nSize,
                                               dwAttributes);

    /* Cleanup and return */
    RtlFreeUnicodeString(&Namespace);
    RtlFreeUnicodeString(&VariableName);
    return Result;
}

_Success_(return)
BOOL
WINAPI
GetFirmwareType(_Out_ PFIRMWARE_TYPE FirmwareType)
{
    NTSTATUS Status;
    SYSTEM_BOOT_ENVIRONMENT_INFORMATION BootInfo;

    /* Check input parameters */
    if (FirmwareType == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Query firmware type */
    Status = NtQuerySystemInformation(SystemBootEnvironmentInformation,
                                      &BootInfo,
                                      sizeof(BootInfo),
                                      0);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *FirmwareType = BootInfo.FirmwareType;
    return TRUE;
}
