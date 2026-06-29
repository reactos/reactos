/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/sysinfoansi.c
 * PURPOSE:         System information functions (ANSI)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* Forward declarations (may be missing from headers depending on SDK level) */
_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize,
    _Out_opt_ PDWORD pdwAttribubutes);

BOOL
WINAPI
SetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize,
    _In_ DWORD dwAttributes);

/* FUNCTIONS ****************************************************************/

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize)
{
    return GetFirmwareEnvironmentVariableExA(lpName, lpGuid, pBuffer, nSize, NULL);
}

BOOL
WINAPI
SetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize)
{
    return SetFirmwareEnvironmentVariableExA(lpName,
                                             lpGuid,
                                             pValue,
                                             nSize,
                                             VARIABLE_ATTRIBUTE_NON_VOLATILE);
}

/* EOF */
