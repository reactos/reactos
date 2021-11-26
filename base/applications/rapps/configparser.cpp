/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Config parser
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */
#include "rapps.h"
#include <debug.h>

struct CLocaleSections
{
    CStringW Locale;
    CStringW LocaleNeutral;
    CStringW Section;
};

struct CSectionNames
{
    CLocaleSections ArchSpecific;
    CLocaleSections ArchNeutral;
};
static CSectionNames g_Names;


static
ATL::CStringW GetINIFullPath(const ATL::CStringW& FileName)
{
    ATL::CStringW szDir;
    ATL::CStringW szBuffer;

    GetStorageDirectory(szDir);
    szBuffer.Format(L"%ls\\rapps\\%ls", szDir.GetString(), FileName.GetString());

    return szBuffer;
}

CConfigParser::CConfigParser(const ATL::CStringW& FileName)
    : szConfigPath(GetINIFullPath(FileName))
{
    CacheINI();
}

void CConfigParser::ReadSection(ATL::CStringW& Buffer, const ATL::CStringW& Section, BOOL isArch)
{
    DWORD len = 512;
    DWORD result;

    do
    {
        len *= 2;

        result = GetPrivateProfileSectionW(Section, Buffer.GetBuffer(len), len, szConfigPath);
        Buffer.ReleaseBuffer(result);
    } while (result == len - 2);

    len = 0;
    while (len < result)
    {
        // Explicitly use the null terminator!
        CString tmp = Buffer.GetBuffer() + len;
        if (tmp.GetLength() > 0)
        {
            len += tmp.GetLength() + 1;

            int idx = tmp.Find('=');
            if (idx >= 0)
            {
                CString key = tmp.Left(idx);

#ifndef _M_IX86
                // On non-x86 architecture we need the architecture specific URL
                if (!isArch && key == "URLDownload")
                {
                    continue;
                }
#endif

                // Is this key already present from a more specific translation?
                if (m_Keys.FindKey(key) >= 0)
                {
                    continue;
                }

                CString value = tmp.Mid(idx+1);
                m_Keys.Add(key, value);
            }
            else
            {
                DPRINT1("ERROR: invalid key/value pair: '%S'\n", tmp.GetString());
            }
        }
        else
        {
            break;
        }
    }
}

VOID CConfigParser::CacheINI()
{
    // Cache section names
    if (g_Names.ArchSpecific.Locale.IsEmpty())
    {
        CString szLocaleID;
        const INT cchLocaleSize = 5;

        GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_ILANGUAGE, szLocaleID.GetBuffer(cchLocaleSize), cchLocaleSize);
        szLocaleID.ReleaseBuffer();
        CString INISectionLocale = L"Section." + szLocaleID;

        g_Names.ArchSpecific.Locale = INISectionLocale + L"." CurrentArchitecture;
        g_Names.ArchNeutral.Locale = INISectionLocale;

        // turn "Section.0c0a" into "Section.0a", keeping just the neutral lang part
        if (szLocaleID.GetLength() >= 2)
        {
            g_Names.ArchSpecific.LocaleNeutral = L"Section." + szLocaleID.Right(2) + L"." CurrentArchitecture;
            g_Names.ArchNeutral.LocaleNeutral = L"Section." + szLocaleID.Right(2);
        }

        g_Names.ArchSpecific.Section = L"Section." CurrentArchitecture;
        g_Names.ArchNeutral.Section = L"Section";
    }

    // Use a shared buffer so that we don't have to re-allocate it every time
    CStringW Buffer;

    ReadSection(Buffer, g_Names.ArchSpecific.Locale, TRUE);
    if (!g_Names.ArchSpecific.LocaleNeutral.IsEmpty())
    {
        ReadSection(Buffer, g_Names.ArchSpecific.LocaleNeutral, TRUE);
    }
    ReadSection(Buffer, g_Names.ArchSpecific.Section, TRUE);


    ReadSection(Buffer, g_Names.ArchNeutral.Locale, FALSE);
    if (!g_Names.ArchNeutral.LocaleNeutral.IsEmpty())
    {
        ReadSection(Buffer, g_Names.ArchNeutral.LocaleNeutral, FALSE);
    }
    ReadSection(Buffer, g_Names.ArchNeutral.Section, FALSE);
}

BOOL CConfigParser::GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString)
{
    int nIndex = m_Keys.FindKey(KeyName);
    if (nIndex >= 0)
    {
        ResultString = m_Keys.GetValueAt(nIndex);
        return TRUE;
    }

    ResultString.Empty();
    return FALSE;
}

BOOL CConfigParser::GetInt(const ATL::CStringW& KeyName, INT& iResult)
{
    ATL::CStringW Buffer;

    iResult = 0;

    // grab the text version of our entry
    if (!GetString(KeyName, Buffer))
        return FALSE;

    if (Buffer.IsEmpty())
        return FALSE;

    // convert it to an actual integer
    iResult = StrToIntW(Buffer.GetString());

    // we only care about values > 0
    return (iResult > 0);
}
