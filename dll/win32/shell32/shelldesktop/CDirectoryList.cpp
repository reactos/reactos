/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "CDirectoryList.h"
#include <assert.h>      // for assert

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

BOOL CDirectoryList::ContainsPath(LPCWSTR pszPath) const
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty())
            continue;

        if (m_items[i].EqualPath(pszPath))
            return TRUE; // matched
    }
    return FALSE;
}

BOOL CDirectoryList::AddPath(LPCWSTR pszPath)
{
    assert(!PathIsRelativeW(pszPath));
    if (ContainsPath(pszPath))
        return FALSE;
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty())
        {
            m_items[i].SetPath(pszPath);
            return TRUE;
        }
    }
    return m_items.Add(pszPath);
}

BOOL CDirectoryList::RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2)
{
    assert(!PathIsRelativeW(pszPath1));
    assert(!PathIsRelativeW(pszPath2));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].EqualPath(pszPath1))
        {
            // matched
            m_items[i].SetPath(pszPath2);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CDirectoryList::DeletePath(LPCWSTR pszPath)
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].EqualPath(pszPath))
        {
            // matched
            m_items[i].SetPath(NULL);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CDirectoryList::AddPathsFromDirectory(LPCWSTR pszDirectoryPath)
{
    // get the full path
    WCHAR szPath[MAX_PATH];
    lstrcpynW(szPath, pszDirectoryPath, _countof(szPath));
    assert(!PathIsRelativeW(szPath));

    // is it a directory?
    if (!PathIsDirectoryW(szPath))
        return FALSE;

    // add the path
    if (!AddPath(szPath))
        return FALSE;

    // enumerate the file items to remember
    PathAppendW(szPath, L"*");
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ERR("FindFirstFileW failed\n");
        return FALSE;
    }

    LPWSTR pch;
    do
    {
        // ignore "." and ".."
        pch = find.cFileName;
        if (pch[0] == L'.' && (pch[1] == 0 || (pch[1] == L'.' && pch[2] == 0)))
            continue;

        // build a path
        PathRemoveFileSpecW(szPath);
        if (lstrlenW(szPath) + lstrlenW(find.cFileName) + 1 > MAX_PATH)
        {
            ERR("szPath is too long\n");
            continue;
        }
        PathAppendW(szPath, find.cFileName);

        // add the path and do recurse
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (m_fRecursive)
                AddPathsFromDirectory(szPath);
            else
                AddPath(szPath);
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);

    return TRUE;
}
