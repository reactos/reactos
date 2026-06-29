/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero registry handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <cicreg.h>

EXTERN_C LSTATUS
_cicRegKey_Open(CicRegKey& self, HKEY hKey, LPCTSTR lpSubKey, REGSAM samDesired)
{
    HKEY hNewKey;
    LSTATUS error = ::RegOpenKeyEx(hKey, lpSubKey, 0, samDesired, &hNewKey);
    if (error != ERROR_SUCCESS)
        return error;

    self.Close();
    self.m_hKey = hNewKey;
    return error;
}

EXTERN_C LSTATUS
_cicRegKey_Create(CicRegKey& self, HKEY hKey, LPCTSTR lpSubKey)
{
    HKEY hNewKey;
    LSTATUS error = ::RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                     KEY_ALL_ACCESS, NULL, &hNewKey, NULL);
    if (error != ERROR_SUCCESS)
        return error;

    self.Close();
    self.m_hKey = hNewKey;
    return error;
}

EXTERN_C LSTATUS
_cicRegKey_EnumValue(CicRegKey& self, DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName)
{
    DWORD dwSaveLen = cchValueName;
    LSTATUS error = ::RegEnumValue(self.m_hKey, dwIndex, lpValueName, &cchValueName,
                                   NULL, NULL, NULL, NULL);
    if (dwSaveLen)
        lpValueName[error == ERROR_SUCCESS ? dwSaveLen - 1 : 0] = 0;
    return error;
}

EXTERN_C LSTATUS
_cicRegKey_QuerySz(CicRegKey& self, LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValueMax)
{
    DWORD cbValueMax = cchValueMax * sizeof(TCHAR);
    LSTATUS error = ::RegQueryValueEx(self.m_hKey, pszValueName, 0, NULL, (LPBYTE)pszValue, &cbValueMax);
    if (cchValueMax > 0)
        pszValue[(error == ERROR_SUCCESS) ? (cchValueMax - 1) : 0] = UNICODE_NULL;
    return error;
}

EXTERN_C LSTATUS
_cicRegKey_RecurseDeleteKey(CicRegKey& self, LPCTSTR lpSubKey)
{
    CicRegKey regKey;
    LSTATUS error = regKey.Open(self.m_hKey, lpSubKey, KEY_READ | KEY_WRITE);
    if (error != ERROR_SUCCESS)
        return error;

    TCHAR szName[MAX_PATH];
    DWORD cchName;
    do
    {
        cchName = _countof(szName);
        error = ::RegEnumKeyEx(regKey, 0, szName, &cchName, NULL, NULL, NULL, NULL);
        if (error != ERROR_SUCCESS)
            break;

        szName[_countof(szName) - 1] = UNICODE_NULL;
        error = regKey.RecurseDeleteKey(szName);
    } while (error == ERROR_SUCCESS);

    regKey.Close();

    return self.DeleteSubKey(lpSubKey);
}
