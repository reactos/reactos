#pragma once

#include <windef.h>
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
    CConfigParser(const CStringW &FileName);

    BOOL
    GetString(const CStringW &KeyName, CStringW &ResultString);
    BOOL
    GetInt(const CStringW &KeyName, INT &iResult);
};
