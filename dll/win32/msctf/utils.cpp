/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Text Framework Services
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define COBJMACROS
#define INITGUID
#define _EXTYPES_H

#include <windows.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <cguid.h>
#include <tchar.h>
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicero/cicreg.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

BOOL StringFromGUID2A(REFGUID rguid, LPSTR pszGUID, INT cchGUID)
{
    pszGUID[0] = ANSI_NULL;

    WCHAR szWide[40];
    szWide[0] = UNICODE_NULL;
    BOOL ret = StringFromGUID2(rguid, szWide, _countof(szWide));
    ::WideCharToMultiByte(CP_ACP, 0, szWide, -1, pszGUID, cchGUID, NULL, NULL);
    return ret;
}

#ifdef UNICODE
    #define StringFromGUID2T StringFromGUID2
#else
    #define StringFromGUID2T StringFromGUID2A
#endif

/***********************************************************************
 *      TF_RegisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_RegisterLangBarAddIn(
    _In_ REFGUID rguid,
    _In_ LPCWSTR pszFilePath,
    _In_ DWORD dwFlags)
{
    TRACE("(%s, %s, 0x%lX)\n", debugstr_guid(&rguid), debugstr_w(pszFilePath), dwFlags);

    if (!pszFilePath || IsEqualGUID(rguid, GUID_NULL))
    {
        ERR("E_INVALIDARG\n");
        return E_INVALIDARG;
    }

    TCHAR szBuff[MAX_PATH], szGUID[40];
    StringCchCopy(szBuff, _countof(szBuff), TEXT("SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\"));
    StringFromGUID2T(rguid, szGUID, _countof(szGUID));
    StringCchCat(szBuff, _countof(szBuff), szGUID);

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Create(hBaseKey, szBuff);
    if (error == ERROR_SUCCESS)
    {
        error = regKey.SetSzW(L"FilePath", pszFilePath);
        if (error == ERROR_SUCCESS)
            error = regKey.SetDword(TEXT("Enable"), !!(dwFlags & 4));
    }

    return ((error == ERROR_SUCCESS) ? S_OK : E_FAIL);
}

/***********************************************************************
 *      TF_UnregisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_UnregisterLangBarAddIn(
    _In_ REFGUID rguid,
    _In_ DWORD dwFlags)
{
    TRACE("(%s, 0x%lX)\n", debugstr_guid(&rguid), dwFlags);

    if (IsEqualGUID(rguid, GUID_NULL))
    {
        ERR("E_INVALIDARG\n");
        return E_INVALIDARG;
    }

    TCHAR szSubKey[MAX_PATH];
    StringCchCopy(szSubKey, _countof(szSubKey), TEXT("SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\"));

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Open(hBaseKey, szSubKey, KEY_ALL_ACCESS);
    HRESULT hr = E_FAIL;
    if (error == ERROR_SUCCESS)
    {
        TCHAR szGUID[40];
        StringFromGUID2T(rguid, szGUID, _countof(szGUID));
        regKey.RecurseDeleteKey(szGUID);
        hr = S_OK;
    }

    return hr;
}
