/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Recent file entry class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CRecentFileEntry
{
private:
    CAtlString m_FileName;
    CAtlString m_DisplayName;

    void BuildDisplayName();

public:
    CRecentFileEntry(const CAtlString& FileName);
    ~CRecentFileEntry();

    const CAtlString& FileName() const { return m_FileName; }
    const CAtlString& DisplayName() const { return m_DisplayName; }
};
