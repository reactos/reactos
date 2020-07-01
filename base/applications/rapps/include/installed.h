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

    ATL::CStringW szDisplayName;
    ATL::CStringW szDisplayVersion;
    ATL::CStringW szPublisher;
    ATL::CStringW szRegOwner;
    ATL::CStringW szProductID;
    ATL::CStringW szHelpLink;
    ATL::CStringW szHelpTelephone;
    ATL::CStringW szReadme;
    ATL::CStringW szContact;
    ATL::CStringW szURLUpdateInfo;
    ATL::CStringW szURLInfoAbout;
    ATL::CStringW szComments;
    ATL::CStringW szInstallDate;
    ATL::CStringW szInstallLocation;
    ATL::CStringW szInstallSource;
    ATL::CStringW szUninstallString;
    ATL::CStringW szModifyPath;

    ~CInstalledApplicationInfo();
};

typedef BOOL(CALLBACK *APPENUMPROC)(CInstalledApplicationInfo * Info, PVOID param);

//BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param);

class CInstalledApps
{
    ATL::CAtlList<CInstalledApplicationInfo *> m_InfoList;

public:
    BOOL Enum(INT EnumType, APPENUMPROC lpEnumProc, PVOID param);

    VOID FreeCachedEntries();
};

