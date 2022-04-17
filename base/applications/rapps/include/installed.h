#pragma once

#include <windef.h>
#include <atlstr.h>

class CInstalledApplicationInfo
{
private:
    BOOL m_IsUserKey;
    REGSAM m_WowKey;
    HKEY m_hSubKey;

    CStringW m_szKeyName;

public:
    CInstalledApplicationInfo(BOOL bIsUserKey, REGSAM RegWowKey, HKEY hKey, const CStringW& szKeyName);
    ~CInstalledApplicationInfo();

    VOID EnsureDetailsLoaded();

    BOOL GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String);
    BOOL GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue);
    BOOL RetrieveIcon(ATL::CStringW& IconLocation);
    BOOL UninstallApplication(BOOL bModify);
    LSTATUS RemoveFromRegistry();

    // These fields are always loaded
    BOOL bIsUpdate;
    CStringW szDisplayIcon;
    CStringW szDisplayName;
    CStringW szDisplayVersion;
    CStringW szComments;

    // These details are loaded on demand
    CStringW szPublisher;
    CStringW szRegOwner;
    CStringW szProductID;
    CStringW szHelpLink;
    CStringW szHelpTelephone;
    CStringW szReadme;
    CStringW szContact;
    CStringW szURLUpdateInfo;
    CStringW szURLInfoAbout;
    CStringW szInstallDate;
    CStringW szInstallLocation;
    CStringW szInstallSource;
    CStringW szUninstallString;
    CStringW szModifyPath;

};

typedef BOOL(CALLBACK *APPENUMPROC)(CInstalledApplicationInfo * Info, PVOID param);

class CInstalledApps
{
    ATL::CAtlList<CInstalledApplicationInfo *> m_InfoList;

public:
    BOOL Enum(INT EnumType, APPENUMPROC lpEnumProc, PVOID param);

    VOID FreeCachedEntries();
};

