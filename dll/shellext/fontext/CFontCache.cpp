/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     font list cache handling
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

CFontCache* g_FontCache = NULL;

CFontInfo::CFontInfo(LPCWSTR name)
    : m_Name(name)
    , m_FileRead(false)
    , m_AttrsRead(false)
    , m_FileWriteTime({})
    , m_dwFileAttributes(0)
{
    m_FileSize.QuadPart = 0;
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

void CFontInfo::ReadAttrs()
{
    CStringW File = g_FontCache->Filename(this, true);

    m_AttrsRead = true;

    WIN32_FIND_DATAW findFileData;
    HANDLE hFile = FindFirstFileW(File, &findFileData);
    if (hFile != INVALID_HANDLE_VALUE)
    {

        // File write time
        FileTimeToLocalFileTime(&findFileData.ftLastWriteTime, &m_FileWriteTime);

        // File size
        m_FileSize.HighPart = findFileData.nFileSizeHigh;
        m_FileSize.LowPart = findFileData.nFileSizeLow;

        m_dwFileAttributes = findFileData.dwFileAttributes;
        FindClose(hFile);
    }
}

const LARGE_INTEGER& CFontInfo::FileSize()
{
    if (!m_AttrsRead)
        ReadAttrs();

    return m_FileSize;
}

const FILETIME& CFontInfo::FileWriteTime()
{
    if (!m_AttrsRead)
        ReadAttrs();

    return m_FileWriteTime;
}

DWORD CFontInfo::FileAttributes()
{
    if (!m_AttrsRead)
        ReadAttrs();

    return m_dwFileAttributes;
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

CFontInfo* CFontCache::Find(const FontPidlEntry* fontEntry)
{
    if (fontEntry->Index < m_Fonts.GetCount())
    {
        if (m_Fonts[fontEntry->Index].Name().CompareNoCase(fontEntry->Name) == 0)
            return &m_Fonts[fontEntry->Index];
    }

    for (UINT n = 0; n < Size(); ++n)
    {
        if (m_Fonts[n].Name().CompareNoCase(fontEntry->Name) == 0)
        {
            return &m_Fonts[n];
        }
    }
    return nullptr;
}


CStringW CFontCache::Filename(CFontInfo* info, bool alwaysFullPath)
{
    CStringW File;
    if (info)
    {
        File = info->File();

        if (!File.IsEmpty() && alwaysFullPath)
        {
            // Ensure this is a full path
            if (PathIsRelativeW(File))
            {
                File = m_FontFolderPath + File;
            }
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

