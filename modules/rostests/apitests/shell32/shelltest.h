#ifndef _SHELLTEST_H_
#define _SHELLTEST_H_

//#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <apitest.h>
#include <winreg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>

// Vista's shell32 is buggy so we need to skip some tests there.
static const BOOL g_bVista = (GetNTVersion() == _WIN32_WINNT_VISTA);

VOID PathToIDList(LPCWSTR pszPath, ITEMIDLIST** ppidl);

static inline LPCWSTR RegNameDisp(LPCWSTR s)
{
    return (s && *s) ? s : L"(Default)";
}

static inline UINT RegSetStringEx(HKEY hKey, LPCWSTR Path, LPCWSTR Name, LPCWSTR Str, DWORD Type)
{
    return SHSetValueW(hKey, Path, Name, Type, (LPCVOID)Str, (lstrlenW(Str) + 1) * sizeof(WCHAR));
}

static inline UINT RegSetString(HKEY hKey, LPCWSTR Path, LPCWSTR Name, LPCWSTR Str)
{
    return RegSetStringEx(hKey, Path, Name, Str, REG_SZ);
}

#endif /* !_SHELLTEST_H_ */
