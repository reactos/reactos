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
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicero/cicreg.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

/***********************************************************************
 *      TF_RegisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_RegisterLangBarAddIn(REFGUID rguid, LPCWSTR pszFilePath, DWORD dwFlags)
{
    if (!pszFilePath || IsEqualGUID(rguid, GUID_NULL))
        return E_INVALIDARG;

    WCHAR szBuff[MAX_PATH], szGUID[40];
    StringCchCopyW(szBuff, _countof(szBuff), L"SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\");
    StringFromGUID2(rguid, szGUID, _countof(szGUID));
    StringCchCatW(szBuff, _countof(szBuff), szGUID);

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Create(hBaseKey, szBuff);
    if (error == ERROR_SUCCESS)
    {
        error = regKey.SetSz(L"FilePath", pszFilePath);
        if (error == ERROR_SUCCESS)
            error = regKey.SetDword(L"Enable", !!(dwFlags & 4));
    }

    return ((error == ERROR_SUCCESS) ? S_OK : E_FAIL);
}

/***********************************************************************
 *      TF_UnregisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_UnregisterLangBarAddIn(REFGUID rguid, DWORD dwFlags)
{
    if (IsEqualGUID(rguid, GUID_NULL))
        return E_INVALIDARG;

    WCHAR szSubKey[MAX_PATH];
    StringCchCopyW(szSubKey, _countof(szSubKey), L"SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\");

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Open(hBaseKey, szSubKey, KEY_ALL_ACCESS);
    HRESULT hr = E_FAIL;
    if (error == ERROR_SUCCESS)
    {
        WCHAR szGUID[40];
        StringFromGUID2(rguid, szGUID, _countof(szGUID));
        regKey.RecurseDeleteKey(szGUID);
        hr = S_OK;
    }

    return hr;
}
