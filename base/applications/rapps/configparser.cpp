/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Config parser
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev           (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov     (sanchaez@reactos.org)
 */
#include "rapps.h"

CConfigParser::CConfigParser(const ATL::CStringW& FileName) : szConfigPath(GetINIFullPath(FileName))
{
    CacheINILocale();
}

ATL::CStringW CConfigParser::GetINIFullPath(const ATL::CStringW& FileName)
{
    ATL::CStringW szDir;
    ATL::CStringW szBuffer;

    GetStorageDirectory(szDir);
    szBuffer.Format(L"%ls\\rapps\\%ls", szDir.GetString(), FileName.GetString());

    return szBuffer;
}

VOID CConfigParser::CacheINILocale()
{
    // TODO: Set default locale if call fails
    // find out what is the current system lang code (e.g. "0a") and append it to SectionLocale
    GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_ILANGUAGE,
                    m_szLocaleID.GetBuffer(m_cchLocaleSize), m_cchLocaleSize);

    m_szLocaleID.ReleaseBuffer();
    m_szCachedINISectionLocale = L"Section." + m_szLocaleID;

    // turn "Section.0c0a" into "Section.0a", keeping just the neutral lang part
    if (m_szLocaleID.GetLength() >= 2)
        m_szCachedINISectionLocaleNeutral = L"Section." + m_szLocaleID.Right(2);
    else
        m_szCachedINISectionLocaleNeutral = m_szCachedINISectionLocale;
}

BOOL CConfigParser::GetStringWorker(const ATL::CStringW& KeyName, PCWSTR Suffix, ATL::CStringW& ResultString)
{
    DWORD dwResult;

    LPWSTR ResultStringBuffer = ResultString.GetBuffer(MAX_PATH);
    // 1st - find localized strings (e.g. "Section.0c0a")
    dwResult = GetPrivateProfileStringW((m_szCachedINISectionLocale + Suffix).GetString(),
                                        KeyName.GetString(),
                                        NULL,
                                        ResultStringBuffer,
                                        MAX_PATH,
                                        szConfigPath.GetString());

    if (!dwResult)
    {
        // 2nd - if they weren't present check for neutral sub-langs/ generic translations (e.g. "Section.0a")
        dwResult = GetPrivateProfileStringW((m_szCachedINISectionLocaleNeutral + Suffix).GetString(),
                                            KeyName.GetString(),
                                            NULL,
                                            ResultStringBuffer,
                                            MAX_PATH,
                                            szConfigPath.GetString());
        if (!dwResult)
        {
            // 3rd - if they weren't present fallback to standard english strings (just "Section")
            dwResult = GetPrivateProfileStringW((ATL::CStringW(L"Section") + Suffix).GetString(),
                                                KeyName.GetString(),
                                                NULL,
                                                ResultStringBuffer,
                                                MAX_PATH,
                                                szConfigPath.GetString());
        }
    }

    ResultString.ReleaseBuffer();
    return (dwResult != 0 ? TRUE : FALSE);
}

BOOL CConfigParser::GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString)
{
    /* First try */
    if (GetStringWorker(KeyName, L"." CurrentArchitecture, ResultString))
    {
        return TRUE;
    }

#ifndef _M_IX86
    /* On non-x86 architecture we need the architecture specific URL */
    if (KeyName == L"URLDownload")
    {
        return FALSE;
    }
#endif

    /* Fall back to default */
    return GetStringWorker(KeyName, L"", ResultString);
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
