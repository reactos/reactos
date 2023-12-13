/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Manipulate module path
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

struct CModulePath
{
    WCHAR m_szPath[MAX_PATH];
    SIZE_T m_cchPath;

    CModulePath()
    {
        m_szPath[0] = UNICODE_NULL;
        m_cchPath = 0;
    }

    BOOL Init(_In_ LPCWSTR pszFileName, _In_ BOOL bSysWinDir);
};

// Get an instance handle that is already loaded
static inline HINSTANCE
GetSystemModuleHandle(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return GetModuleHandleW(ModPath.m_szPath);
}

// Load a system library
static inline HINSTANCE
LoadSystemLibrary(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return ::LoadLibraryW(ModPath.m_szPath);
}

/******************************************************************************/

inline BOOL
CModulePath::Init(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    SIZE_T cchPath;
    if (bSysWinDir)
    {
        // Usually C:\Windows or C:\ReactOS
        cchPath = ::GetSystemWindowsDirectory(m_szPath, _countof(m_szPath));
    }
    else
    {
        // Usually C:\Windows\system32 or C:\ReactOS\system32
        cchPath = ::GetSystemDirectoryW(m_szPath, _countof(m_szPath));
    }

    m_szPath[_countof(m_szPath) - 1] = UNICODE_NULL; // Avoid buffer overrun

    if ((cchPath == 0) || (cchPath > _countof(m_szPath) - 2))
        goto Failure;

    // Add backslash if necessary
    if ((cchPath > 0) && (m_szPath[cchPath - 1] != L'\\'))
    {
        m_szPath[cchPath + 0] = L'\\';
        m_szPath[cchPath + 1] = UNICODE_NULL;
    }

    // Append pszFileName
    if (FAILED(StringCchCatW(m_szPath, _countof(m_szPath), pszFileName)))
        goto Failure;

    m_cchPath = wcslen(m_szPath);
    return TRUE;

Failure:
    m_szPath[0] = UNICODE_NULL;
    m_cchPath = 0;
    return FALSE;
}
