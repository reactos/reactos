#pragma once

#include <windef.h>
#include <atlstr.h>

#define ENUM_APPLICATIONS      31
#define ENUM_UPDATES           32

#define ENUM_INSTALLED_MIN ENUM_ALL_COMPONENTS
#define ENUM_INSTALLED_MAX ENUM_UPDATES

#define IS_INSTALLED_ENUM(a) (a >= ENUM_INSTALLED_MIN && a <= ENUM_INSTALLED_MAX)

struct INSTALLED_INFO
{
    HKEY hRootKey;
    HKEY hSubKey;
    ATL::CStringW szKeyName;
};
typedef INSTALLED_INFO *PINSTALLED_INFO;
typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, PINSTALLED_INFO Info);

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc);
BOOL GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString);
BOOL GetApplicationString(HKEY hKey, LPCWSTR RegName, ATL::CStringW &String);

BOOL ShowInstalledAppInfo(INT Index);
BOOL UninstallApplication(INT Index, BOOL bModify);
VOID RemoveAppFromRegistry(INT Index);
