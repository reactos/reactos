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

CZipEnumerator::CZipEnumerator()
{
}

BOOL CZipEnumerator::Initialize(IZip* zip)
{
    ATLASSERT(zip);
    m_Zip = zip;
    return Reset();
}

DWORD CZipEnumerator::CalculateFilenameCRC32(PCSTR filename)
{
    ATLASSERT(filename);
    DWORD crc = 0;
    crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)filename, strlen(filename));
    return crc;
}

BOOL
CZipEnumerator::GetUtf8Name(
    CStringA& utf8Name,
    PCSTR originalName,
    const BYTE* extraField,
    DWORD extraFieldLen)
{
    ATLASSERT(extraField);

    if (extraFieldLen < EF_HEADER_SIZE)
        return FALSE;

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

        const BYTE* fieldData = ptr + EF_HEADER_SIZE;

        if (ptr + EF_HEADER_SIZE + fieldSize > end || fieldSize < 5 || fieldData[0] != 1)
            return FALSE;

        DWORD storedCRC = *(DWORD*)(fieldData + 1);
        DWORD calculatedCRC = CalculateFilenameCRC32(originalName);
        if (storedCRC != calculatedCRC)
            return FALSE;

        DWORD utf8NameLen = fieldSize - 5;
        if (utf8NameLen > 0)
        {
            utf8Name = CStringA((LPCSTR)(fieldData + 5), utf8NameLen);
            return TRUE;
        }

        ptr += EF_HEADER_SIZE + fieldSize;
    }

    return FALSE;
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
    BOOL hasUtf8Name = FALSE;
    if (info.size_file_extra > 0)
        hasUtf8Name = GetUtf8Name(utf8Name, nameA, extra.GetData(), info.size_file_extra);

    if (hasUtf8Name)
        name = CA2WEX<MAX_PATH>(utf8Name, CP_UTF8);
    else if (info.flag & MINIZIP_UTF8_FLAG)
        name = CA2WEX<MAX_PATH>(nameA, CP_UTF8);
    else
        name = CA2WEX<MAX_PATH>(nameA, m_nCodePage);

    return TRUE;
}

BOOL CZipEnumerator::NextUnique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info)
{
    SIZE_T len = wcslen(prefix);
    CStringW tmp;
    while (Next(tmp, info))
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
            return TRUE;
        }
    }
    return FALSE;
}
