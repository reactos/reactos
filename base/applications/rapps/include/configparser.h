#pragma once

#include <atlstr.h>

class CConfigParser
{
    const CStringW szConfigPath;
    CSimpleMap<CStringW, CStringW> m_Keys;

    void
    CacheINI();
    void
    ReadSection(CStringW &Buffer, const CStringW &Section, BOOL isArch);

  public:
    CConfigParser(const CStringW &FilePath);

    BOOL
    GetString(const CStringW &KeyName, CStringW &ResultString);
    BOOL
    GetInt(const CStringW &KeyName, INT &iResult);

    UINT
    GetSectionString(LPCWSTR Section, LPCWSTR Name, CStringW &Result);
};

HRESULT
ReadIniValue(LPCWSTR File, LPCWSTR Section, LPCWSTR Name, CStringW &Output);
