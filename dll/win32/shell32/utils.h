/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   ReactOS Team
 */

#pragma once

#ifdef __cplusplus
static inline LPWSTR
SHStrDupW(LPCWSTR Src)
{
    LPWSTR Dup;
    return SUCCEEDED(SHStrDupW(Src, &Dup)) ? Dup : NULL;
}
#endif

static inline BOOL
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

static inline DWORD
RegSetString(HKEY hKey, LPCWSTR Name, LPCWSTR Str, DWORD Type = REG_SZ)
{
    return RegSetValueExW(hKey, Name, 0, Type, LPBYTE(Str), (lstrlenW(Str) + 1) * sizeof(WCHAR));
}

typedef struct {
    LPCSTR Verb;
    WORD CmdId;
} CMVERBMAP;

HRESULT
SHELL_MapContextMenuVerbToCmdId(LPCMINVOKECOMMANDINFO pICI, const CMVERBMAP *pMap);
HRESULT
SHELL_GetCommandStringImpl(SIZE_T CmdId, UINT uFlags, LPSTR Buf, UINT cchBuf, const CMVERBMAP *pMap);

// SHExtractIconsW is a forward, use this function instead inside shell32
inline HICON
SHELL32_SHExtractIcon(LPCWSTR File, int Index, int cx, int cy)
{
    HICON hIco;
    int r = PrivateExtractIconsW(File, Index, cx, cy, &hIco, NULL, 1, 0);
    return r > 0 ? hIco : NULL;
}

HRESULT
SHELL_CreateShell32DefaultExtractIcon(int IconIndex, REFIID riid, LPVOID *ppvOut);

static inline HRESULT
SHELL_CreateFallbackExtractIconForFolder(REFIID riid, LPVOID *ppvOut)
{
    const int id = IDI_SHELL_FOLDER;
    return SHELL_CreateShell32DefaultExtractIcon(id > 1 ? -id : 0, riid, ppvOut);
}

static inline HRESULT
SHELL_CreateFallbackExtractIconForNoAssocFile(REFIID riid, LPVOID *ppvOut)
{
    const int id = IDI_SHELL_DOCUMENT;
    return SHELL_CreateShell32DefaultExtractIcon(id > 1 ? -id : 0, riid, ppvOut);
}
