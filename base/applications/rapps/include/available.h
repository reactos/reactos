#pragma once

#include <windef.h>
#include <atlstr.h> 
#include <atlsimpcoll.h>
#include <atlcoll.h>

#include "misc.h"

enum LicenseType
{
    LICENSE_NONE,
    LICENSE_OPENSOURCE,
    LICENSE_FREEWARE,
    LICENSE_TRIAL,
    LICENSE_MIN = LICENSE_NONE,
    LICENSE_MAX = LICENSE_TRIAL
};

inline BOOL IsLicenseType(INT x)
{
    return (x >= LICENSE_MIN && x <= LICENSE_MAX);
}

struct CAvailableApplicationInfo
{
    INT m_Category;
    BOOL m_IsSelected;
    LicenseType m_LicenseType;
    ATL::CStringW m_szName;
    ATL::CStringW m_szRegName;
    ATL::CStringW m_szVersion;
    ATL::CStringW m_szLicense;
    ATL::CStringW m_szDesc;
    ATL::CStringW m_szSize;
    ATL::CStringW m_szUrlSite;
    ATL::CStringW m_szUrlDownload;
    ATL::CStringW m_szCDPath;
    ATL::CSimpleArray<LCID> m_LanguageLCIDs;

    // Caching mechanism related entries
    ATL::CStringW m_sFileName;
    FILETIME m_ftCacheStamp;

    // Optional integrity checks (SHA-1 digests are 160 bit = 40 characters in hex string form)
    ATL::CStringW m_szSHA1;
    ATL::CStringW m_szInstalledVersion;

    // Create an object from file
    CAvailableApplicationInfo(const ATL::CStringW& sFileNameParam);

    // Load all info from the file
    VOID RefreshAppInfo();
    BOOL HasLanguageInfo() const;
    BOOL HasNativeLanguage() const;
    BOOL HasEnglishLanguage() const;
    BOOL IsInstalled() const;
    BOOL HasInstalledVersion() const;
    BOOL HasUpdate() const;

    // Set a timestamp
    VOID SetLastWriteTime(FILETIME* ftTime);

private:
    BOOL m_IsInstalled;
    BOOL m_HasLanguageInfo;
    BOOL m_HasInstalledVersion;
    CConfigParser* m_Parser;

    inline BOOL GetString(LPCWSTR lpKeyName, ATL::CStringW& ReturnedString);

    // Lazily load general info from the file
    VOID RetrieveGeneralInfo();
    VOID RetrieveInstalledStatus();
    VOID RetrieveInstalledVersion();
    VOID RetrieveLanguages();
    VOID RetrieveLicenseType();
    inline BOOL FindInLanguages(LCID what) const;
};

typedef BOOL(CALLBACK *AVAILENUMPROC)(CAvailableApplicationInfo *Info, LPCWSTR szFolderPath);

struct AvailableStrings
{
    ATL::CStringW szPath;
    ATL::CStringW szCabPath;
    ATL::CStringW szAppsPath;
    ATL::CStringW szSearchPath;

    AvailableStrings();
};

class CAvailableApps
{
    static AvailableStrings m_Strings;
    ATL::CAtlList<CAvailableApplicationInfo*> m_InfoList;

public:
    CAvailableApps();

    static BOOL UpdateAppsDB();
    static BOOL ForceUpdateAppsDB();
    static VOID DeleteCurrentAppsDB();

    VOID FreeCachedEntries();
    BOOL Enum(INT EnumType, AVAILENUMPROC lpEnumProc);

    CAvailableApplicationInfo* FindInfo(const ATL::CStringW& szAppName) const;
    ATL::CSimpleArray<CAvailableApplicationInfo> FindInfoList(const ATL::CSimpleArray<ATL::CStringW> &arrAppsNames) const;
    ATL::CSimpleArray<CAvailableApplicationInfo> GetSelected() const;

    const ATL::CStringW& GetFolderPath() const;
    const ATL::CStringW& GetAppPath() const;
    const ATL::CStringW& GetCabPath() const;
};
