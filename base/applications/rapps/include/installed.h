#pragma once

#include <windef.h>
#include <atlstr.h>

class CInstalledApplicationInfo
{
public:
    HKEY hRootKey;
    HKEY hSubKey;
    ATL::CStringW szKeyName;

    BOOL GetApplicationString(LPCWSTR lpKeyName, ATL::CStringW& String);
    BOOL UninstallApplication(BOOL bModify);
};

typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, CInstalledApplicationInfo * Info, PVOID param);

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);
