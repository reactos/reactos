/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CZipEnumerator
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#define EF_UNIPATH 0x7075 // Unicode Path extra field ID
#define EF_HEADER_SIZE 4 // Extra field header size (ID + size)
#define EF_UNIPATH_VERSION 1 // Unicode Path extra field version

CZipEnumerator::CZipEnumerator()
{
}

BOOL CZipEnumerator::Initialize(IZip* zip)
{
    ATLASSERT(zip);
    m_Zip = zip;
    return Reset();
}

BOOL CZipEnumerator::Reset()
{
    unzFile uf = m_Zip->getZip();
    m_First = TRUE;
    if (unzGoToFirstFile(uf) != UNZ_OK)
        return FALSE;
    m_Returned.RemoveAll();
    return TRUE;
}

DWORD CZipEnumerator::CalculateFilenameCRC32(PCSTR filename)
{
    ATLASSERT(filename);
    DWORD crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)filename, strlen(filename));
    return crc;
}

CStringA
CZipEnumerator::GetUtf8Name(
    PCSTR originalName,
    const BYTE* extraField,
    DWORD extraFieldLen)
{
    ATLASSERT(originalName);
    ATLASSERT(extraField);

    if (extraFieldLen < EF_HEADER_SIZE)
        return "";

    const BYTE* ptr = extraField;
    const BYTE* end = extraField + extraFieldLen;

    while (ptr + EF_HEADER_SIZE <= end)
    {
        WORD fieldId = *(WORD*)ptr, fieldSize = *(WORD*)(ptr + 2);
        if (fieldId != EF_UNIPATH)
        {
            ptr += EF_HEADER_SIZE + fieldSize;
            continue;
        }

        if (fieldSize < 5 || ptr + EF_HEADER_SIZE + fieldSize > end)
            return "";

        const BYTE* fieldData = ptr + EF_HEADER_SIZE;
        BYTE version = fieldData[0];
        if (version != EF_UNIPATH_VERSION)
            return "";

        DWORD storedCRC = *(DWORD*)(fieldData + 1);
        DWORD calculatedCRC = CalculateFilenameCRC32(originalName);
        if (storedCRC != calculatedCRC)
            return "";

        DWORD utf8NameLen = fieldSize - 5;
        if (utf8NameLen > 0)
            return CStringA((LPCSTR)(fieldData + 5), utf8NameLen);

        ptr += EF_HEADER_SIZE + fieldSize;
    }

    return "";
}

BOOL CZipEnumerator::Next(CStringW& name, unz_file_info64& info)
{
    INT err;
    unzFile uf = m_Zip->getZip();

    if (!m_First)
    {
        err = unzGoToNextFile(uf);
        if (err == UNZ_END_OF_LIST_OF_FILE)
            return FALSE;
    }
    m_First = FALSE;

    err = unzGetCurrentFileInfo64(uf, &info, NULL, 0, NULL, 0, NULL, 0);
    if (err != UNZ_OK)
        return FALSE;

    CAtlArray<BYTE> extra;
    extra.SetCount(info.size_file_extra);

    CStringA nameA;
    PSTR buf = nameA.GetBuffer(info.size_filename);
    err = unzGetCurrentFileInfo64(uf, NULL, buf, nameA.GetAllocLength(),
        (info.size_file_extra > 0) ? extra.GetData() : NULL, info.size_file_extra,
        NULL, 0);
    nameA.ReleaseBuffer(info.size_filename);

    if (err != UNZ_OK)
        return FALSE;

    nameA.Replace('\\', '/');

    CStringA utf8Name;
    if (info.size_file_extra > 0)
        utf8Name = GetUtf8Name(nameA, extra.GetData(), info.size_file_extra);

    if (utf8Name.GetLength() > 0)
        name = CA2WEX<MAX_PATH>(utf8Name, CP_UTF8);
    else if (info.flag & MINIZIP_UTF8_FLAG)
        name = CA2WEX<MAX_PATH>(nameA, CP_UTF8);
    else
        name = CA2WEX<MAX_PATH>(nameA, m_nCodePage);

    return TRUE;
}

BOOL CZipEnumerator::NextUnique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info)
{
    ATLASSERT(prefix);
    CStringW tmp;
    SIZE_T cchPrefix = wcslen(prefix);
    while (Next(tmp, info))
    {
        if (StrCmpNIW(tmp, prefix, cchPrefix) != 0)
            continue;

        INT ichSlash = tmp.Find(L'/', cchPrefix);
        folder = (ichSlash >= 0);
        tmp = name = (folder ? tmp.Mid(cchPrefix, ichSlash - cchPrefix) : tmp.Mid(cchPrefix));
        tmp.MakeLower();

        POSITION it = m_Returned.Find(tmp);
        if (!name.IsEmpty() && !it)
        {
            m_Returned.AddTail(tmp);
            return TRUE;
        }
    }
    return FALSE;
}
