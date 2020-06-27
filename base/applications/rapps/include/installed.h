#pragma once

#include <windef.h>
#include <atlstr.h>

class CInstalledApplicationInfo
{
public:
    HKEY hSubKey = NULL;
    BOOL bIsUpdate = FALSE;

    CInstalledApplicationInfo(BOOL bIsUserKey, HKEY hSubKey);
    BOOL GetApplicationString(LPCWSTR lpKeyName, ATL::CStringW& String);
    BOOL UninstallApplication(BOOL bModify);

    ~CInstalledApplicationInfo();
};

typedef BOOL(CALLBACK *APPENUMPROC)(INT ItemIndex, ATL::CStringW &Name, CInstalledApplicationInfo * Info, PVOID param);

//BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);

class CInstalledApps
{
    ATL::CAtlList<CInstalledApplicationInfo *> m_InfoList;

public:
    BOOL Enum(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);

    VOID CInstalledApps::FreeCachedEntries();
};

