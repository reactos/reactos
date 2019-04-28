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
typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, PINSTALLED_INFO Info);

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc);
BOOL GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString);

BOOL UninstallApplication(INT Index, BOOL bModify);
VOID RemoveAppFromRegistry(INT Index);
