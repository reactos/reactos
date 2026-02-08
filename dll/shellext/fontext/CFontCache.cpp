/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     font list cache handling
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

CFontCache* g_FontCache = NULL;

CFontInfo::CFontInfo(PCWSTR name, PCWSTR value)
    : m_Name(name)
    , m_File(value)
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
        if (Valid() && m_File.IsEmpty())
        {
            // Read the filename stored in the registry.
            // This can be either a filename or a full path
            CRegKey key;
            if (key.Open(FONT_HIVE, FONT_KEY, KEY_READ) == ERROR_SUCCESS)
            {
                CStringW Value;
                DWORD dwAllocated = MAX_PATH;
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
    if (m_Fonts.GetSize() == 0)
        Read();

    return m_Fonts.GetSize();
}

CStringW CFontCache::Name(size_t Index)
{
    if (m_Fonts.GetSize() == 0)
        Read();

    if ((INT)Index >= m_Fonts.GetSize())
        return CStringW();

    return m_Fonts[Index].Name();
}

CStringW CFontCache::File(size_t Index)
{
    if (m_Fonts.GetSize() == 0)
        Read();

    if ((INT)Index >= m_Fonts.GetSize())
        return CStringW();

    return m_Fonts[Index].File();
}

CFontInfo* CFontCache::Find(const FontPidlEntry* fontEntry)
{
    if (m_Fonts.GetSize() == 0)
        Read();

    for (INT i = 0; i < m_Fonts.GetSize(); ++i)
    {
        if (m_Fonts[i].Name().CompareNoCase(fontEntry->Name()) == 0)
        {
            return &m_Fonts[i];
        }
    }
    return nullptr;
}

BOOL CFontCache::IsMarkDeleted(size_t Index) const
{
    if ((INT)Index >= m_Fonts.GetSize())
        return FALSE;
    return m_Fonts[Index].IsMarkDeleted();
}

// The item must exist until its visibility is removed, because the change
// notification UI must work for existing items.
void CFontCache::MarkDeleted(const FontPidlEntry* fontEntry)
{
    for (INT i = 0; i < m_Fonts.GetSize(); ++i)
    {
        if (m_Fonts[i].Name().CompareNoCase(fontEntry->Name()) == 0)
        {
            m_Fonts[i].MarkDeleted();
            break;
        }
    }
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

void CFontCache::Insert(CAtlList<CFontInfo>& fonts, const CStringW& KeyName, PCWSTR Value)
{
    CFontInfo newInfo(KeyName, Value);

    CStringW strFontFile = g_FontCache->GetFontFilePath(newInfo.File());
    if (!PathFileExistsW(strFontFile))
        return;

    POSITION it = fonts.GetHeadPosition();
    while (it != NULL)
    {
        POSITION lastit = it;
        const CFontInfo& info = fonts.GetNext(it);
        if (info.Name().CompareNoCase(KeyName) >= 0)
        {
            fonts.InsertBefore(lastit, newInfo);
            return;
        }
    }

    fonts.AddTail(newInfo);
}

CStringW CFontCache::GetFontFilePath(const LPCWSTR Path) const
{
    if (PathIsRelativeW(Path))
        return m_FontFolderPath + Path;
    return Path;
}

void CFontCache::Read()
{
    CAtlList<CFontInfo> fonts;
    CRegKey key;

    // Enumerate all registered font names
    if (key.Open(FONT_HIVE, FONT_KEY, KEY_READ) == ERROR_SUCCESS)
    {
        LSTATUS Status;
        DWORD dwAllocated = MAX_PATH;
        DWORD ilIndex = 0;
        CStringW KeyName;
        do
        {
            DWORD dwSize = dwAllocated;
            PWSTR Buffer = KeyName.GetBuffer(dwSize);
            WCHAR szValue[MAX_PATH];
            DWORD cbValue = sizeof(szValue);
            Status = RegEnumValueW(key.m_hKey, ilIndex, Buffer, &dwSize, NULL, NULL,
                                   (PBYTE)szValue, &cbValue);
            KeyName.ReleaseBuffer(dwSize);
            if (Status == ERROR_SUCCESS)
            {
                szValue[_countof(szValue) - 1] = UNICODE_NULL; // Avoid buffer overrun
                if (!KeyName.IsEmpty() && szValue[0])
                {
                    // Insert will create an ordered list
                    Insert(fonts, KeyName, szValue);
                }
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
    m_Fonts.RemoveAll();
    POSITION it = fonts.GetHeadPosition();
    while (it != NULL)
        m_Fonts.Add(fonts.GetNext(it));
}
