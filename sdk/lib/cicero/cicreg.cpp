/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero registry handling
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "cicreg.h"

void CicRegKey::Close()
{
    if (!m_hKey)
        return;

    ::RegCloseKey(m_hKey);
    m_hKey = NULL;
}

LSTATUS
CicRegKey::RecurseDeleteKey(LPCTSTR lpSubKey)
{
    CicRegKey regKey;
    LSTATUS error = regKey.Open(m_hKey, lpSubKey, KEY_READ | KEY_WRITE);
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

    return DeleteSubKey(lpSubKey);
}

LSTATUS
CicRegKey::EnumValue(DWORD dwIndex, LPTSTR lpValueName, DWORD cchValueName)
{
    DWORD dwSaveLen = cchValueName;
    LSTATUS error = ::RegEnumValue(m_hKey, dwIndex, lpValueName, &cchValueName,
                                   NULL, NULL, NULL, NULL);
    if (dwSaveLen)
        lpValueName[error == ERROR_SUCCESS ? dwSaveLen - 1 : 0] = 0;
    return error;
}
