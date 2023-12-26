/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero registry handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

class CicRegKey
{
public:
    HKEY m_hKey;

    CicRegKey() : m_hKey(NULL) { }

    virtual ~CicRegKey()
    {
        Close();
    }

    operator HKEY() { return m_hKey; }

    void Close();

    LSTATUS Open(
        HKEY hKey,
        LPCWSTR lpSubKey,
        REGSAM samDesired = KEY_READ);

    LSTATUS Create(
        HKEY hKey,
        LPCWSTR lpSubKey,
        LPWSTR lpClass = NULL,
        DWORD dwOptions = REG_OPTION_NON_VOLATILE,
        REGSAM samDesired = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL,
        LPDWORD pdwDisposition = NULL);

    LSTATUS QueryDword(LPCWSTR pszValueName, LPDWORD pdwValue)
    {
        DWORD cbData = sizeof(DWORD);
        return ::RegQueryValueExW(m_hKey, pszValueName, 0, NULL, (LPBYTE)pdwValue, &cbData);
    }

    LSTATUS SetDword(LPCWSTR pszValueName, DWORD dwValue)
    {
        return ::RegSetValueExW(m_hKey, pszValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    }

    LSTATUS QuerySz(LPCWSTR pszValueName, LPWSTR pszValue, DWORD cchValueMax);

    LSTATUS SetSz(LPCWSTR pszValueName, LPCWSTR pszValue)
    {
        DWORD cbValue = (lstrlenW(pszValue) + 1) * sizeof(WCHAR);
        return ::RegSetValueExW(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
    }

    LSTATUS DeleteSubKey(LPCWSTR lpSubKey)
    {
        return ::RegDeleteKeyW(m_hKey, lpSubKey);
    }

    LSTATUS RecurseDeleteKey(LPCWSTR lpSubKey);
};

/******************************************************************************/

inline void
CicRegKey::Close()
{
    if (!m_hKey)
        return;

    ::RegCloseKey(m_hKey);
    m_hKey = NULL;
}

inline LSTATUS
CicRegKey::Open(
    HKEY hKey,
    LPCWSTR lpSubKey,
    REGSAM samDesired)
{
    HKEY hNewKey = NULL;
    LSTATUS error = ::RegOpenKeyExW(hKey, lpSubKey, 0, samDesired, &hNewKey);
    if (error != ERROR_SUCCESS)
        return error;

    Close();
    m_hKey = hNewKey;
    return error;
}

inline LSTATUS
CicRegKey::Create(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    LPDWORD pdwDisposition)
{
    HKEY hNewKey = NULL;
    LSTATUS error = ::RegCreateKeyExW(hKey,
                                      lpSubKey,
                                      0,
                                      lpClass,
                                      dwOptions,
                                      samDesired,
                                      lpSecurityAttributes,
                                      &hNewKey,
                                      pdwDisposition);
    if (error != ERROR_SUCCESS)
        return error;

    Close();
    m_hKey = hNewKey;
    return error;
}

inline LSTATUS
CicRegKey::QuerySz(LPCWSTR pszValueName, LPWSTR pszValue, DWORD cchValueMax)
{
    DWORD cchSaveMax = cchValueMax;

    cchValueMax *= sizeof(WCHAR);
    LSTATUS error = ::RegQueryValueExW(m_hKey, pszValueName, 0, NULL,
                                       (LPBYTE)pszValue, &cchValueMax);
    if (cchSaveMax > 0)
        pszValue[(error == ERROR_SUCCESS) ? (cchSaveMax - 1) : 0] = UNICODE_NULL;

    return error;
}

inline LSTATUS
CicRegKey::RecurseDeleteKey(LPCWSTR lpSubKey)
{
    CicRegKey regKey;
    LSTATUS error = regKey.Open(m_hKey, lpSubKey, KEY_READ | KEY_WRITE);
    if (error != ERROR_SUCCESS)
        return error;

    do
    {
        WCHAR szName[MAX_PATH];
        DWORD cchName = _countof(szName);
        error = ::RegEnumKeyExW(regKey, 0, szName, &cchName, NULL, NULL, NULL, NULL);
        if (error != ERROR_SUCCESS)
            break;

        szName[_countof(szName) - 1] = UNICODE_NULL;
        error = regKey.RecurseDeleteKey(szName);
    } while (error != ERROR_SUCCESS);

    if (error == ERROR_SUCCESS)
        error = regKey.DeleteSubKey(lpSubKey);

    return error;
}
