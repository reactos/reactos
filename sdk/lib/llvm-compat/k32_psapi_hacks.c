/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Win7 K32 psapi export shims for llvm-mingw runtime libraries
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

#include <windef.h>
#include <winbase.h>
#include <psapi.h>

/* llvm-mingw's libunwind looks up module unwind sections through this Win7+ kernel32 export: forward to
 * the classic psapi.dll one */
BOOL
WINAPI
K32EnumProcessModules(
    _In_ HANDLE hProcess,
    _Out_ HMODULE *lphModule,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded)
{
    return EnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
}
