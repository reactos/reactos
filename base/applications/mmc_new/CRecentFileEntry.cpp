/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Recent file entry class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

CRecentFileEntry::CRecentFileEntry(const CAtlString& FileName)
{
    m_FileName = FileName;
    BuildDisplayName();
}

CRecentFileEntry::~CRecentFileEntry()
{
    delete m_FileName;
    delete m_DisplayName;
}

void
CRecentFileEntry::BuildDisplayName()
{
    WCHAR SystemDirectory[MAX_PATH];
    UINT Length;

    Length = ::GetSystemDirectoryW(SystemDirectory, MAX_PATH);

    if (_wcsnicmp(m_FileName.GetString(), SystemDirectory, Length) == 0)
    {
        m_DisplayName = m_FileName.Mid(Length + 1);
    }
    else
    {
        PWSTR pBackslash1 = NULL, pBackslash2 = NULL, pBackslash3 = NULL;

        pBackslash1 = wcschr(m_FileName.GetString(), L'\\');

        if (pBackslash1)
            pBackslash2 = wcschr(pBackslash1 + 1, L'\\');

        if (pBackslash2)
            pBackslash3 = wcsrchr(m_FileName.GetString(), L'\\');

        if (pBackslash1 && pBackslash2 && pBackslash3 && (pBackslash2 != pBackslash3))
        {
            int Length = pBackslash2 - m_FileName.GetString();
            CAtlString str(m_FileName.Left(Length + 1));
            str.Append(L"...");
            str.Append(pBackslash3);
            m_DisplayName = str;
        }
        else
        {
            m_DisplayName = m_FileName;
        }
    }
}
