/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     font list cache handling
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once


class CFontInfo
{
private:
    CStringW m_Name;
    CStringW m_File;
    bool m_FileRead;
public:
    CFontInfo(LPCWSTR name = L"");

    const CStringW& Name() const;
    const CStringW& File();
    const bool Valid() const;
};


class CFontCache
{
private:
    CAtlArray<CFontInfo> m_Fonts;
    CStringW m_FontFolderPath;

protected:
    CFontCache();

    void Insert(CAtlList<CFontInfo>& fonts, const CStringW& KeyName);
    void Read();

public:
    void SetFontDir(const LPCWSTR Path);
    const CStringW& FontPath() const { return m_FontFolderPath; }

    size_t Size();
    CStringW Name(size_t Index);
    CStringW Filename(const FontPidlEntry* fontEntry, bool alwaysFullPath = false);

    friend class CFontExtModule;
};


extern CFontCache* g_FontCache;


