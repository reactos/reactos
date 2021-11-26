#pragma once

#include <windef.h>
#include <atlstr.h>

class CConfigParser
{
    const ATL::CStringW szConfigPath;
    CSimpleMap<CStringW, CStringW> m_Keys;

    void CacheINI();
    void ReadSection(ATL::CStringW& Buffer, const ATL::CStringW& Section, BOOL isArch);

public:
    CConfigParser(const ATL::CStringW& FileName);

    BOOL GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    BOOL GetInt(const ATL::CStringW& KeyName, INT& iResult);
};

