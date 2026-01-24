/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CZipEnumerator
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

struct CZipEnumerator
{
    CComPtr<IZip> m_Zip;
    BOOL m_First = TRUE;
    CAtlList<CStringW> m_Returned;
    UINT m_nCodePage = GetZipCodePage(TRUE);

    static DWORD CalculateFilenameCRC32(PCSTR filename);

    static BOOL GetUtf8Name(
        CStringA& utf8Name,
        PCSTR originalName,
        const BYTE* extraField,
        DWORD extraFieldLen);

public:
    CZipEnumerator();

    BOOL Initialize(IZip* zip);
    BOOL Reset();
    BOOL Next(CStringW& name, unz_file_info64& info);
    BOOL NextUnique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info);
};
