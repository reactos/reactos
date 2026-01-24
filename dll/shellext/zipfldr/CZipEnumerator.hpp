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
    bool m_First;
    CAtlList<CStringW> m_Returned;
    UINT m_nCodePage;

public:
    CZipEnumerator();

    bool initialize(IZip* zip);
    bool reset();
    bool next_unique(PCWSTR prefix, CStringW& name, bool& folder, unz_file_info64& info);
    bool next(CStringW& name, unz_file_info64& info);
};
