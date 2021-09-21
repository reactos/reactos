#pragma once

#include <windef.h>
#include <atlstr.h>

class CConfigParser
{
    // Locale names cache
    const static INT m_cchLocaleSize = 5;

    ATL::CStringW m_szLocaleID;
    ATL::CStringW m_szCachedINISectionLocale;
    ATL::CStringW m_szCachedINISectionLocaleNeutral;

    const ATL::CStringW szConfigPath;

    ATL::CStringW GetINIFullPath(const ATL::CStringW& FileName);
    VOID CacheINILocale();
    BOOL GetStringWorker(const ATL::CStringW& KeyName, PCWSTR Suffix, ATL::CStringW& ResultString);

public:
    CConfigParser(const ATL::CStringW& FileName);

    BOOL GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    BOOL GetInt(const ATL::CStringW& KeyName, INT& iResult);
};

