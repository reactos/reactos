/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 */

#pragma once

/* FUNCTIONS ****************************************************************/
FORCEINLINE
BOOL
IsHKCRKey(_In_ HKEY hKey)
{
    return ((ULONG_PTR)hKey & 0x2) != 0;
}

FORCEINLINE
void
MakeHKCRKey(_Inout_ HKEY* hKey)
{
    *hKey = (HKEY)((ULONG_PTR)(*hKey) | 0x2);
}

LONG
WINAPI
CreateHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ DWORD Reserved,
    _In_opt_ LPWSTR lpClass,
    _In_ DWORD dwOptions,
    _In_ REGSAM samDesired,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _Out_ PHKEY phkResult,
    _Out_opt_ LPDWORD lpdwDisposition);

LONG
WINAPI
OpenHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ DWORD ulOptions,
    _In_ REGSAM samDesired,
    _In_ PHKEY phkResult);

LONG
WINAPI
DeleteHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ REGSAM RegSam,
    _In_ DWORD Reserved);

LONG
WINAPI
QueryHKCRValue(
    _In_ HKEY hKey,
    _In_ LPCWSTR Name,
    _In_ LPDWORD Reserved,
    _In_ LPDWORD Type,
    _In_ LPBYTE Data,
    _In_ LPDWORD Count);

LONG
WINAPI
SetHKCRValue(
    _In_ HKEY hKey,
    _In_ LPCWSTR Name,
    _In_ DWORD Reserved,
    _In_ DWORD Type,
    _In_ CONST BYTE* Data,
    _In_ DWORD DataSize);

LONG
WINAPI
EnumHKCRKey(
    _In_ HKEY hKey,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _Inout_ LPDWORD lpcbName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPWSTR lpClass,
    _Inout_opt_ LPDWORD lpcbClass,
    _Out_opt_ PFILETIME lpftLastWriteTime);

LONG
WINAPI
EnumHKCRValue(
    _In_ HKEY hKey,
    _In_ DWORD index,
    _Out_ LPWSTR value,
    _Inout_ PDWORD val_count,
    _Reserved_ PDWORD reserved,
    _Out_opt_ PDWORD type,
    _Out_opt_ LPBYTE data,
    _Inout_opt_ PDWORD count);

