/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     font list cache handling
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

CFontCache* g_FontCache = NULL;

CFontInfo::CFontInfo(LPCWSTR name)
    : m_Name(name)
    , m_FileRead(false)
{
}

const CStringW& CFontInfo::Name() const
{
    return m_Name;
}

const bool CFontInfo::Valid() const
{
    return !m_Name.IsEmpty();
}

const CStringW& CFontInfo::File()
{
    if (!m_FileRead)
    {
        if (Valid())
        {
            // Read the filename stored in the registry.
            // This can be either a filename or a full path
            CRegKey key;
            if (key.Open(FONT_HIVE, FONT_KEY, KEY_READ) == ERROR_SUCCESS)
            {
                CStringW Value;
                DWORD dwAllocated = 128;
                LSTATUS Status;
                do
                {
                    DWORD dwSize = dwAllocated;
                    PWSTR Buffer = Value.GetBuffer(dwSize);
                    Status = key.QueryStringValue(m_Name, Buffer, &dwSize);
                    Value.ReleaseBuffer(dwSize);
                    if (Status == ERROR_SUCCESS)
                    {
                        // Ensure we do not re-use the same string object, by passing it a PCWSTR
                        m_File = Value.GetString();
                        break;
                    }
                    dwAllocated += 128;
                } while (Status == ERROR_MORE_DATA);
            }
        }
        m_FileRead = true;
    }
    return m_File;
}



CFontCache::CFontCache()
{
}

void CFontCache::SetFontDir(const LPCWSTR Path)
{
    if (m_FontFolderPath.IsEmpty())
        m_FontFolderPath = Path;
}

size_t CFontCache::Size()
{
    if (m_Fonts.GetCount() == 0u)
        Read();

    return m_Fonts.GetCount();
}

CStringW CFontCache::Name(size_t Index)
{
    if (m_Fonts.GetCount() == 0u)
        Read();

    if (Index >= m_Fonts.GetCount())
        return CStringW();

    return m_Fonts[Index].Name();
}

CStringW CFontCache::Filename(const FontPidlEntry* fontEntry, bool alwaysFullPath)
{
    CStringW File;

    if (fontEntry->Index < m_Fonts.GetCount())
    {
        CFontInfo& info = m_Fonts[fontEntry->Index];

        if (info.Name().CompareNoCase(fontEntry->Name) == 0)
            File = info.File();
    }

    for (UINT n = 0; File.IsEmpty() && n < Size(); ++n)
    {
        if (m_Fonts[n].Name().CompareNoCase(fontEntry->Name) == 0)
            File = m_Fonts[n].File();
    }

    if (!File.IsEmpty() && alwaysFullPath)
    {
        // Ensure this is a full path
        if (PathIsRelativeW(File))
        {
            File = m_FontFolderPath + File;
        }
    }

    return File;
}

void CFontCache::Insert(CAtlList<CFontInfo>& fonts, const CStringW& KeyName)
{
    POSITION it = fonts.GetHeadPosition();
    while (it != NULL)
    {
        POSITION lastit = it;
        const CFontInfo& info = fonts.GetNext(it);
        if (info.Name().CompareNoCase(KeyName) >= 0)
        {
            fonts.InsertBefore(lastit, CFontInfo(KeyName));
            return;
        }
    }
    fonts.AddTail(CFontInfo(KeyName));
}

void CFontCache::Read()
{
    CAtlList<CFontInfo> fonts;
    CRegKey key;

    // Enumerate all registered font names
    if (key.Open(FONT_HIVE, FONT_KEY, KEY_READ) == ERROR_SUCCESS)
    {
        LSTATUS Status;
        DWORD dwAllocated = 128;
        DWORD ilIndex = 0;
        CStringW KeyName;
        do
        {
            DWORD dwSize = dwAllocated;
            PWSTR Buffer = KeyName.GetBuffer(dwSize);
            Status = RegEnumValueW(key.m_hKey, ilIndex, Buffer, &dwSize, NULL, NULL, NULL, NULL);
            KeyName.ReleaseBuffer(dwSize);
            if (Status == ERROR_SUCCESS)
            {
                // Insert will create an ordered list
                Insert(fonts, KeyName);
                ilIndex++;
                continue;
            }
            if (Status == ERROR_NO_MORE_ITEMS)
                break;
            else if (Status == ERROR_MORE_DATA)
            {
                dwAllocated += 128;
            }
        } while (Status == ERROR_MORE_DATA || Status == ERROR_SUCCESS);
    }

    // Move the fonts from a list to an array (for easy indexing)
    m_Fonts.SetCount(fonts.GetCount());
    size_t Index = 0;
    POSITION it = fonts.GetHeadPosition();
    while (it != NULL)
    {
        m_Fonts[Index] = fonts.GetNext(it);
        Index++;
    }
}

