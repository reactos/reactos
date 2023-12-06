/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   ReactOS Team
 */

#pragma once

inline BOOL
RegValueExists(HKEY hKey, LPCWSTR Name)
{
    return RegQueryValueExW(hKey, Name, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
}

inline BOOL
RegKeyExists(HKEY hKey, LPCWSTR Path)
{
    BOOL ret = !RegOpenKeyExW(hKey, Path, 0, MAXIMUM_ALLOWED, &hKey);
    if (ret)
        RegCloseKey(hKey);
    return ret;
}

inline DWORD
RegSetOrDelete(HKEY hKey, LPCWSTR Name, DWORD Type, LPCVOID Data, DWORD Size)
{
    if (Data)
        return RegSetValueExW(hKey, Name, 0, Type, LPBYTE(Data), Size);
    else
        return RegDeleteValueW(hKey, Name);
}

inline DWORD
RegSetString(HKEY hKey, LPCWSTR Name, LPCWSTR Str, DWORD Type = REG_SZ)
{
    return RegSetValueExW(hKey, Name, 0, Type, LPBYTE(Str), (lstrlenW(Str) + 1) * sizeof(WCHAR));
}

// SHExtractIconsW is a forward, use this function instead inside shell32
inline HICON
SHELL32_SHExtractIcon(LPCWSTR File, int Index, int cx, int cy)
{
    HICON hIco;
    int r = PrivateExtractIconsW(File, Index, cx, cy, &hIco, NULL, 1, 0);
    return r > 0 ? hIco : NULL;
}
