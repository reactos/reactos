/*
 * PROJECT:     ReactOS CTF Monitor
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

HINSTANCE GetSystemModuleHandle(_In_ LPCWSTR pszFileName, _In_ BOOL bSysWinDir);
HINSTANCE LoadSystemLibrary(_In_ LPCWSTR pszFileName, _In_ BOOL bSysWinDir);
