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
    virtual ~CicRegKey() { Close(); }

    operator HKEY() { return m_hKey; }

    void Close();

    LSTATUS Open(
        HKEY hKey,
        LPCTSTR lpSubKey,
        REGSAM samDesired = KEY_READ);

    LSTATUS Create(
        HKEY hKey,
        LPCTSTR lpSubKey,
        LPTSTR lpClass = NULL,
        DWORD dwOptions = REG_OPTION_NON_VOLATILE,
        REGSAM samDesired = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL,
        LPDWORD pdwDisposition = NULL);

    LSTATUS QueryDword(LPCTSTR pszValueName, LPDWORD pdwValue)
    {
        DWORD cbData = sizeof(DWORD);
        return ::RegQueryValueEx(m_hKey, pszValueName, 0, NULL, (LPBYTE)pdwValue, &cbData);
    }

    LSTATUS SetDword(LPCTSTR pszValueName, DWORD dwValue)
    {
        return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    }

    LSTATUS QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax);

    LSTATUS SetSz(LPCTSTR pszValueName, LPCTSTR pszValue)
    {
        DWORD cbValue = (lstrlen(pszValue) + 1) * sizeof(TCHAR);
        return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
    }
    LSTATUS SetSzW(LPCWSTR pszValueName, LPCWSTR pszValue)
    {
        DWORD cbValue = (lstrlenW(pszValue) + 1) * sizeof(WCHAR);
        return ::RegSetValueExW(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
    }

    LSTATUS DeleteSubKey(LPCTSTR lpSubKey)
    {
        return ::RegDeleteKey(m_hKey, lpSubKey);
    }

    LSTATUS RecurseDeleteKey(LPCTSTR lpSubKey);

    LSTATUS EnumValue(DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName);
};

inline LSTATUS
CicRegKey::Open(
    HKEY hKey,
    LPCTSTR lpSubKey,
    REGSAM samDesired)
{
    HKEY hNewKey = NULL;
    LSTATUS error = ::RegOpenKeyEx(hKey, lpSubKey, 0, samDesired, &hNewKey);
    if (error != ERROR_SUCCESS)
        return error;

    Close();
    m_hKey = hNewKey;
    return error;
}

inline LSTATUS
CicRegKey::Create(
    HKEY hKey,
    LPCTSTR lpSubKey,
    LPTSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    LPDWORD pdwDisposition)
{
    HKEY hNewKey = NULL;
    LSTATUS error = ::RegCreateKeyEx(hKey,
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
CicRegKey::QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax)
{
    DWORD cchSaveMax = cchValueMax;

    cchValueMax *= sizeof(TCHAR);
    LSTATUS error = ::RegQueryValueEx(m_hKey, pszValueName, 0, NULL,
                                      (LPBYTE)pszValue, &cchValueMax);
    if (cchSaveMax > 0)
        pszValue[(error == ERROR_SUCCESS) ? (cchSaveMax - 1) : 0] = UNICODE_NULL;

    return error;
}
