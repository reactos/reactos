/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     font list cache handling
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

class CFontInfo
{
private:
    CStringW m_Name;
    CStringW m_File;
    BOOL m_bMarkDeleted = FALSE;
    bool m_FileRead;
    bool m_AttrsRead;
    LARGE_INTEGER m_FileSize;
    FILETIME m_FileWriteTime;
    DWORD m_dwFileAttributes;

    void ReadAttrs();

public:
    CFontInfo(PCWSTR name = L"", PCWSTR value = L"");

    const CStringW& Name() const;   // Font display name stored in the registry
    const bool Valid() const;

    BOOL IsMarkDeleted() const { return m_bMarkDeleted; }
    void MarkDeleted() { m_bMarkDeleted = TRUE; }

    const CStringW& File();         // Full path or file, depending on how it's stored in the registry
    const LARGE_INTEGER& FileSize();
    const FILETIME& FileWriteTime();
    DWORD FileAttributes();
};

class CFontCache
{
private:
    CSimpleArray<CFontInfo> m_Fonts;
    CStringW m_FontFolderPath;

protected:
    CFontCache();

    void Insert(CAtlList<CFontInfo>& fonts, const CStringW& KeyName, PCWSTR Value);

public:
    void Read();

    void SetFontDir(const LPCWSTR Path);
    const CStringW& FontPath() const { return m_FontFolderPath; }
    CStringW GetFontFilePath(const PCWSTR Path) const;

    size_t Size();
    CStringW Name(size_t Index);    // Font display name stored in the registry
    CStringW File(size_t Index);

    CFontInfo* Find(const FontPidlEntry* fontEntry);
    BOOL IsMarkDeleted(size_t Index) const;
    void MarkDeleted(const FontPidlEntry* fontEntry);
    CStringW Filename(CFontInfo* info, bool alwaysFullPath = false);

    friend class CFontExtModule;
};

extern CFontCache* g_FontCache;
