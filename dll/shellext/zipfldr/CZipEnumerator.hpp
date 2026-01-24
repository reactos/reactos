/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CZipEnumerator
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

static inline DWORD CalculateFilenameCRC32(PCSTR filename)
{
    DWORD crc = 0;
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)filename, strlen(filename));
    return crc;
}

#define MINIZIP_EF_HEADER_SIZE 4 // Extra field header size (ID + size)

static inline bool
ExtractUnicodePathFromExtra(const BYTE* extraField, DWORD extraFieldLen,
                            PCSTR originalName, CStringW& unicodeName)
{
    if (!extraField || extraFieldLen < MINIZIP_EF_HEADER_SIZE)
        return false;

    const BYTE* ptr = extraField;
    const BYTE* end = extraField + extraFieldLen;

    while (ptr + MINIZIP_EF_HEADER_SIZE <= end)
    {
        WORD fieldId = *(WORD*)ptr;
        WORD fieldSize = *(WORD*)(ptr + 2);

        if (fieldId == MINIZIP_EF_UNIPATH_ID)
        {
            const BYTE* fieldData = ptr + MINIZIP_EF_HEADER_SIZE;

            if (ptr + MINIZIP_EF_HEADER_SIZE + fieldSize > end)
                return false;

            if (fieldSize < 5)
                return false;

            BYTE version = fieldData[0];
            if (version != 1)
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
        }

        ptr += MINIZIP_EF_HEADER_SIZE + fieldSize;
    }

    return false;
}

struct CZipEnumerator
{
private:
    CComPtr<IZip> m_Zip;
    bool m_First;
    CAtlList<CStringW> m_Returned;
    UINT m_nCodePage;
public:
    CZipEnumerator()
        : m_First(true)
        , m_nCodePage(GetZipCodePage(TRUE))
    {
    }

    bool initialize(IZip* zip)
    {
        m_Zip = zip;
        return reset();
    }

    bool reset()
    {
        unzFile uf = m_Zip->getZip();
        m_First = true;
        if (unzGoToFirstFile(uf) != UNZ_OK)
            return false;
        m_Returned.RemoveAll();
        return true;
    }

    bool next_unique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info)
    {
        size_t len = wcslen(prefix);
        CStringW tmp;
        while (next(tmp, info))
        {
            if (!StrCmpNIW(tmp, prefix, len))
            {
                int pos = tmp.Find(L'/', len);
                if (pos < 0)
                {
                    name = tmp.Mid(len);
                    folder = false;
                }
                else
                {
                    name = tmp.Mid(len, pos - len);
                    folder = true;
                }
                tmp = name;
                tmp.MakeLower();

                POSITION it = m_Returned.Find(tmp);
                if (!name.IsEmpty() && !it)
                {
                    m_Returned.AddTail(tmp);
                    return true;
                }
            }
        }
        return false;
    }

    bool next(CStringW& name, unz_file_info64& info)
    {
        int err;
        unzFile uf = m_Zip->getZip();

        if (!m_First)
        {
            err = unzGoToNextFile(uf);
            if (err == UNZ_END_OF_LIST_OF_FILE)
            {
                return false;
            }
        }
        m_First = false;

        err = unzGetCurrentFileInfo64(uf, &info, NULL, 0, NULL, 0, NULL, 0);
        if (err != UNZ_OK)
            return false;

        CStringA nameA;
        PSTR buf = nameA.GetBuffer(info.size_filename);
        err = unzGetCurrentFileInfo64(uf, NULL, buf, nameA.GetAllocLength(), NULL, 0, NULL, 0);
        nameA.ReleaseBuffer(info.size_filename);

        if (err != UNZ_OK)
            return false;

        nameA.Replace('\\', '/');

        CStringW unicodeName;
        bool hasUnicodePath = false;

        if (info.size_file_extra > 0)
        {
            CAtlArray<BYTE> extraField;
            extraField.SetCount(info.size_file_extra);

            err = unzGetCurrentFileInfo64(uf, NULL, NULL, 0,
                                          extraField.GetData(), info.size_file_extra,
                                          NULL, 0);
            if (err == UNZ_OK)
            {
                hasUnicodePath = ExtractUnicodePathFromExtra(
                    extraField.GetData(),
                    info.size_file_extra,
                    nameA,
                    unicodeName);
            }
        }

        if (hasUnicodePath)
            name = unicodeName;
        else if (info.flag & MINIZIP_UTF8_FLAG)
            name = CA2WEX<MAX_PATH>(nameA, CP_UTF8);
        else
            name = CA2WEX<MAX_PATH>(nameA, m_nCodePage);

        return true;
    }
};
