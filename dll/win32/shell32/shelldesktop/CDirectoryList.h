/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#include <atlsimpcoll.h> // for CSimpleArray

//////////////////////////////////////////////////////////////////////////////

class CFSNode;

// the directory list
class CDirectoryList
{
public:
    CDirectoryList(CFSNode *pRoot);
    CDirectoryList(CFSNode *pRoot, LPCWSTR pszDirectoryPath, BOOL fRecursive);
    ~CDirectoryList();

    BOOL ContainsPath(LPCWSTR pszPath) const;
    BOOL AddPath(LPCWSTR pszPath);
    BOOL AddPathsFromDirectory(LPCWSTR pszDirectoryPath);
    BOOL RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2);
    BOOL DeletePath(LPCWSTR pszPath);
    void RemoveAll();

protected:
    CFSNode *m_pRoot;
    BOOL m_fRecursive;
};
