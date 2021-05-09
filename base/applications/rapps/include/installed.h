#pragma once

#include <windef.h>
#include <atlstr.h>

class CInstalledApplicationInfo
{
public:
    BOOL IsUserKey;
    REGSAM WowKey;
    HKEY hSubKey;
    BOOL bIsUpdate = FALSE;

    ATL::CStringW szKeyName;

    CInstalledApplicationInfo(BOOL bIsUserKey, REGSAM RegWowKey, HKEY hKey);
    BOOL GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String);
    BOOL GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue);
    BOOL RetrieveIcon(ATL::CStringW& IconLocation);
    BOOL UninstallApplication(BOOL bModify);
    LSTATUS RemoveFromRegistry();

    ATL::CStringW szDisplayIcon;
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

class CInstalledApps
{
    ATL::CAtlList<CInstalledApplicationInfo *> m_InfoList;

public:
    BOOL Enum(INT EnumType, APPENUMPROC lpEnumProc, PVOID param);

    VOID FreeCachedEntries();
};

