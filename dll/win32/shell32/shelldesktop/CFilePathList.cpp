/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "CFilePathList.h"
#include <assert.h>      // for assert

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

BOOL CFilePathList::ContainsPath(LPCWSTR pszPath, BOOL fIsDirectory) const
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty() || fIsDirectory != m_items[i].IsDirectory())
            continue;

        if (m_items[i].EqualPath(pszPath))
            return TRUE; // matched
    }
    return FALSE;
}

static DWORD GetSizeOfFile(LPCWSTR pszPath)
{
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(pszPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(hFind);
    return find.nFileSizeLow;
}

BOOL CFilePathList::AddPath(LPCWSTR pszPath, DWORD dwFileSize, BOOL fIsDirectory)
{
    assert(!PathIsRelativeW(pszPath));

    if (dwFileSize == INVALID_FILE_SIZE)
    {
        dwFileSize = GetSizeOfFile(pszPath);
    }

    CFilePathItem item(pszPath, dwFileSize, fIsDirectory);
    return m_items.Add(item);
}

BOOL CFilePathList::RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2, BOOL fIsDirectory)
{
    assert(!PathIsRelativeW(pszPath1));
    assert(!PathIsRelativeW(pszPath2));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsDirectory() == fIsDirectory && m_items[i].EqualPath(pszPath1))
        {
            // matched
            m_items[i].SetPath(pszPath2);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CFilePathList::DeletePath(LPCWSTR pszPath, BOOL fIsDirectory)
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsDirectory() == fIsDirectory && m_items[i].EqualPath(pszPath))
        {
            // matched
            m_items[i].SetPath(NULL);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CFilePathList::AddPathsFromDirectory(LPCWSTR pszDirectoryPath, BOOL fRecursive)
{
    // get the full path
    WCHAR szPath[MAX_PATH];
    lstrcpynW(szPath, pszDirectoryPath, _countof(szPath));
    assert(!PathIsRelativeW(szPath));

    // is it a directory?
    if (!PathIsDirectoryW(szPath))
        return FALSE;

    // add the path
    if (!AddPath(szPath, 0, TRUE))
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

    do
    {
        // ignore "." and ".."
        if (lstrcmpW(find.cFileName, L".") == 0 ||
            lstrcmpW(find.cFileName, L"..") == 0)
        {
            continue;
        }

        // build a path
        PathRemoveFileSpecW(szPath);
        if (lstrlenW(szPath) + lstrlenW(find.cFileName) + 1 > MAX_PATH)
        {
            ERR("szPath is too long\n");
            continue;
        }
        PathAppendW(szPath, find.cFileName);

        BOOL fIsDirectory = !!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

        // add the path and do recurse
        if (fRecursive && fIsDirectory)
            AddPathsFromDirectory(szPath, fRecursive);
        else
            AddPath(szPath, find.nFileSizeLow, fIsDirectory);
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);

    return TRUE;
}

BOOL CFilePathList::GetFirstChange(LPWSTR pszPath) const
{
    // validate paths
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty())
            continue;

        if (m_items[i].IsDirectory()) // item is a directory
        {
            if (!PathIsDirectoryW(m_items[i].GetPath()))
            {
                // mismatched
                lstrcpynW(pszPath, m_items[i].GetPath(), MAX_PATH);
                return TRUE;
            }
        }
        else // item is a normal file
        {
            if (!PathFileExistsW(m_items[i].GetPath()) ||
                PathIsDirectoryW(m_items[i].GetPath()))
            {
                // mismatched
                lstrcpynW(pszPath, m_items[i].GetPath(), MAX_PATH);
                return TRUE;
            }
        }
    }

    // check sizes
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty() || m_items[i].IsDirectory())
            continue;

        // get size
        hFind = FindFirstFileW(m_items[i].GetPath(), &find);
        FindClose(hFind);

        if (hFind == INVALID_HANDLE_VALUE ||
            find.nFileSizeLow != m_items[i].GetSize())
        {
            // different size
            lstrcpynW(pszPath, m_items[i].GetPath(), MAX_PATH);
            return TRUE;
        }
    }

    return FALSE;
}
