#pragma once

#include <windef.h>
#include <atlstr.h>

struct INSTALLED_INFO
{
    HKEY hRootKey;
    HKEY hSubKey;
    ATL::CStringW szKeyName;

    BOOL GetApplicationString(LPCWSTR lpKeyName, ATL::CStringW& String);
};

typedef INSTALLED_INFO *PINSTALLED_INFO;
typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, PINSTALLED_INFO Info, PVOID param);

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);
BOOL GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString);

BOOL UninstallApplication(PINSTALLED_INFO ItemInfo, BOOL bModify);
