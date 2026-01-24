/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CZipEnumerator
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#define EF_UNIPATH 0x7075 // Unicode Path extra field ID
#define MINIZIP_EF_HEADER_SIZE 4 // Extra field header size (ID + size)

static DWORD CalculateFilenameCRC32(PCSTR filename)
{
    DWORD crc = 0;
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)filename, strlen(filename));
    return crc;
}

static bool
ExtractUnicodePathFromExtra(const BYTE* extraField, DWORD extraFieldLen,
                            PCSTR originalName, CStringW& unicodeName)
{
    if (!extraField || extraFieldLen < MINIZIP_EF_HEADER_SIZE)
        return false;

    const BYTE* ptr = extraField;
    const BYTE* end = extraField + extraFieldLen;

    while (ptr + MINIZIP_EF_HEADER_SIZE <= end)
    {
        WORD fieldId = *(WORD*)ptr, fieldSize = *(WORD*)(ptr + 2);
        if (fieldId != EF_UNIPATH)
        {
            ptr += MINIZIP_EF_HEADER_SIZE + fieldSize;
            continue;
        }

        const BYTE* fieldData = ptr + MINIZIP_EF_HEADER_SIZE;

        if (ptr + MINIZIP_EF_HEADER_SIZE + fieldSize > end)
            return false;

        if (fieldSize < 5 || fieldData[0] != 1) // size and version check
            return false;

        DWORD storedCRC = *(DWORD*)(fieldData + 1);
        DWORD calculatedCRC = CalculateFilenameCRC32(originalName);
        if (storedCRC != calculatedCRC)
            return false;

        DWORD utf8NameLen = fieldSize - 5;
        if (utf8NameLen > 0)
        {
            CStringA utf8Name((LPCSTR)(fieldData + 5), utf8NameLen);
            unicodeName = CA2WEX<MAX_PATH>(utf8Name, CP_UTF8);
            return true;
        }

        ptr += MINIZIP_EF_HEADER_SIZE + fieldSize;
    }

    return false;
}

CZipEnumerator::CZipEnumerator()
    : m_First(true)
    , m_nCodePage(GetZipCodePage(TRUE))
{
}

bool CZipEnumerator::initialize(IZip* zip)
{
    m_Zip = zip;
    return reset();
}

bool CZipEnumerator::reset()
{
    unzFile uf = m_Zip->getZip();
    m_First = true;
    if (unzGoToFirstFile(uf) != UNZ_OK)
        return false;
    m_Returned.RemoveAll();
    return true;
}

bool CZipEnumerator::next_unique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info)
{
    SIZE_T len = wcslen(prefix);
    CStringW tmp;
    while (next(tmp, info))
    {
        if (StrCmpNIW(tmp, prefix, len) != 0)
            continue;

        INT pos = tmp.Find(L'/', len);
        folder = (pos >= 0);
        if (folder)
            name = tmp.Mid(len, pos - len);
        else
            name = tmp.Mid(len);

        tmp = name;
        tmp.MakeLower();

        POSITION it = m_Returned.Find(tmp);
        if (!name.IsEmpty() && !it)
        {
            m_Returned.AddTail(tmp);
            return true;
        }
    }
    return false;
}

bool CZipEnumerator::next(CStringW& name, unz_file_info64& info)
{
    INT err;
    unzFile uf = m_Zip->getZip();

    if (!m_First)
    {
        err = unzGoToNextFile(uf);
        if (err == UNZ_END_OF_LIST_OF_FILE)
            return false;
    }
    m_First = false;

    err = unzGetCurrentFileInfo64(uf, &info, NULL, 0, NULL, 0, NULL, 0);
    if (err != UNZ_OK)
        return false;

    CAtlArray<BYTE> extra;
    extra.SetCount(info.size_file_extra);

    CStringA nameA;
    PSTR buf = nameA.GetBuffer(info.size_filename);
    err = unzGetCurrentFileInfo64(uf, NULL, buf, nameA.GetAllocLength(),
        (info.size_file_extra > 0) ? extra.GetData() : NULL, info.size_file_extra,
        NULL, 0);
    nameA.ReleaseBuffer(info.size_filename);

    if (err != UNZ_OK)
        return false;

    nameA.Replace('\\', '/');

    CStringW unicodeName;
    bool hasUnicodePath = false;
    if (info.size_file_extra > 0)
    {
        hasUnicodePath = ExtractUnicodePathFromExtra(
            extra.GetData(), info.size_file_extra, nameA, unicodeName);
    }

    if (hasUnicodePath)
        name = unicodeName;
    else if (info.flag & MINIZIP_UTF8_FLAG)
        name = CA2WEX<MAX_PATH>(nameA, CP_UTF8);
    else
        name = CA2WEX<MAX_PATH>(nameA, m_nCodePage);

    return true;
}
