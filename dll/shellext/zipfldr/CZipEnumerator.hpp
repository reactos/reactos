/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CZipEnumerator
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

struct CZipEnumerator
{
private:
    CComPtr<IZip> m_Zip;
    bool m_First;
    CAtlList<CStringW> m_Returned;
public:
    CZipEnumerator()
        :m_First(true)
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
            if (!_wcsnicmp(tmp, prefix, len))
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
        if (err == UNZ_OK)
        {
            CStringA nameA;
            PSTR buf = nameA.GetBuffer(info.size_filename);
            err = unzGetCurrentFileInfo64(uf, NULL, buf, nameA.GetAllocLength(), NULL, 0, NULL, 0);
            nameA.ReleaseBuffer(info.size_filename);
            nameA.Replace('\\', '/');

            if (info.flag & MINIZIP_UTF8_FLAG)
                Utf8ToWide(nameA, name);
            else
                name = CStringW(nameA);
        }
        return err == UNZ_OK;
    }
};

