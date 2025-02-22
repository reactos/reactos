/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero registry handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CicRegKey
{
public:
    HKEY m_hKey;

    CicRegKey() : m_hKey(NULL) { }
    ~CicRegKey() { Close(); }

    operator HKEY() { return m_hKey; }

    LSTATUS Open(HKEY hKey, LPCTSTR lpSubKey, REGSAM samDesired = KEY_READ);
    LSTATUS Create(HKEY hKey, LPCTSTR lpSubKey);
    void Close();

    LSTATUS QueryDword(LPCTSTR pszValueName, LPDWORD pdwValue);
    LSTATUS SetDword(LPCTSTR pszValueName, DWORD dwValue);

    LSTATUS QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax);
    LSTATUS SetSz(LPCTSTR pszValueName, LPCTSTR pszValue);
    LSTATUS SetSzW(LPCWSTR pszValueName, LPCWSTR pszValue);

    LSTATUS DeleteValue(LPCTSTR pszValueName);
    LSTATUS DeleteSubKey(LPCTSTR lpSubKey);
    LSTATUS RecurseDeleteKey(LPCTSTR lpSubKey);

    LSTATUS EnumValue(DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName);
};

/***********************************************************************/

// FIXME: Here, directly using C++ methods causes compile errors... Why?
EXTERN_C LSTATUS _cicRegKey_Open(CicRegKey& self, HKEY hKey, LPCTSTR lpSubKey, REGSAM samDesired);
EXTERN_C LSTATUS _cicRegKey_Create(CicRegKey& self, HKEY hKey, LPCTSTR lpSubKey);
EXTERN_C LSTATUS _cicRegKey_RecurseDeleteKey(CicRegKey& self, LPCTSTR lpSubKey);

EXTERN_C LSTATUS
_cicRegKey_EnumValue(CicRegKey& self, DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName);

EXTERN_C LSTATUS
_cicRegKey_QuerySz(CicRegKey& self, LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax);

inline LSTATUS CicRegKey::Open(HKEY hKey, LPCTSTR lpSubKey, REGSAM samDesired)
{
    return _cicRegKey_Open(*this, hKey, lpSubKey, samDesired);
}

inline LSTATUS CicRegKey::Create(HKEY hKey, LPCTSTR lpSubKey)
{
    return _cicRegKey_Create(*this, hKey, lpSubKey);
}

inline LSTATUS CicRegKey::QueryDword(LPCTSTR pszValueName, LPDWORD pdwValue)
{
    DWORD cbData = sizeof(DWORD);
    return ::RegQueryValueEx(m_hKey, pszValueName, 0, NULL, (LPBYTE)pdwValue, &cbData);
}

inline LSTATUS CicRegKey::SetDword(LPCTSTR pszValueName, DWORD dwValue)
{
    return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
}

inline LSTATUS CicRegKey::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    DWORD cbValue = (lstrlen(pszValue) + 1) * sizeof(TCHAR);
    return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
}

inline LSTATUS CicRegKey::SetSzW(LPCWSTR pszValueName, LPCWSTR pszValue)
{
    DWORD cbValue = (lstrlenW(pszValue) + 1) * sizeof(WCHAR);
    return ::RegSetValueExW(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
}

inline LSTATUS CicRegKey::QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax)
{
    return _cicRegKey_QuerySz(*this, pszValueName, pszValue, cchValueMax);
}

inline void CicRegKey::Close()
{
    if (!m_hKey)
        return;

    ::RegCloseKey(m_hKey);
    m_hKey = NULL;
}

inline LSTATUS CicRegKey::DeleteValue(LPCTSTR pszValueName)
{
    return ::RegDeleteValue(m_hKey, pszValueName);
}

inline LSTATUS CicRegKey::DeleteSubKey(LPCTSTR lpSubKey)
{
    return ::RegDeleteKey(m_hKey, lpSubKey);
}

inline LSTATUS CicRegKey::EnumValue(DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName)
{
    return _cicRegKey_EnumValue(*this, dwIndex, lpValueName, cchValueName);
}

inline LSTATUS CicRegKey::RecurseDeleteKey(LPCTSTR lpSubKey)
{
    return _cicRegKey_RecurseDeleteKey(*this, lpSubKey);
}
