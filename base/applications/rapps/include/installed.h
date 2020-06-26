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
};

typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, CInstalledApplicationInfo * Info, PVOID param);

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);
BOOL GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString);

BOOL UninstallApplication(CInstalledApplicationInfo * ItemInfo, BOOL bModify);
