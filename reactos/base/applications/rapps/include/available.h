#pragma once
#include <windef.h>
#include <atlstr.h> 
#include <atlsimpcoll.h>
#include <atlcoll.h>

/* EnumType flags for EnumAvailableApplications */
#define ENUM_ALL_AVAILABLE     0
#define ENUM_CAT_AUDIO         1
#define ENUM_CAT_VIDEO         2
#define ENUM_CAT_GRAPHICS      3
#define ENUM_CAT_GAMES         4
#define ENUM_CAT_INTERNET      5
#define ENUM_CAT_OFFICE        6
#define ENUM_CAT_DEVEL         7
#define ENUM_CAT_EDU           8
#define ENUM_CAT_ENGINEER      9
#define ENUM_CAT_FINANCE       10
#define ENUM_CAT_SCIENCE       11
#define ENUM_CAT_TOOLS         12
#define ENUM_CAT_DRIVERS       13
#define ENUM_CAT_LIBS          14
#define ENUM_CAT_OTHER         15

#define ENUM_AVAILABLE_MIN ENUM_ALL_AVAILABLE
#define ENUM_AVAILABLE_MAX ENUM_CAT_OTHER

#define IS_AVAILABLE_ENUM(a) (a >= ENUM_AVAILABLE_MIN && a <= ENUM_AVAILABLE_MAX)

typedef enum
{
    None,
    OpenSource,
    Freeware,
    Trial,
    Max = Trial,
    Min = None
} LICENSE_TYPE, *PLICENSE_TYPE;

class CConfigParser
{
    // Locale names cache
    const static INT m_cchLocaleSize = 5;

    static ATL::CStringW m_szLocaleID;
    static ATL::CStringW m_szCachedINISectionLocale;
    static ATL::CStringW m_szCachedINISectionLocaleNeutral;

    const LPCWSTR STR_VERSION_CURRENT = L"CURRENT";
    const ATL::CStringW szConfigPath;

    static ATL::CStringW GetINIFullPath(const ATL::CStringW& FileName);
    static VOID CacheINILocaleLazy();

public:
    static const ATL::CStringW& GetLocale();
    static INT CConfigParser::GetLocaleSize();

    CConfigParser(const ATL::CStringW& FileName);

    UINT GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString);
    UINT GetInt(const ATL::CStringW& KeyName);
};

typedef struct
{
    INT Category;
    LICENSE_TYPE LicenseType;
    ATL::CStringW szName;
    ATL::CStringW szRegName;
    ATL::CStringW szVersion;
    ATL::CStringW szLicense;
    ATL::CStringW szDesc;
    ATL::CStringW szSize;
    ATL::CStringW szUrlSite;
    ATL::CStringW szUrlDownload;
    ATL::CStringW szCDPath;
    ATL::CSimpleArray<LCID> Languages;

    // Caching mechanism related entries
    ATL::CStringW sFileName;
    FILETIME ftCacheStamp;

    // Optional integrity checks (SHA-1 digests are 160 bit = 40 characters in hex string form)
    ATL::CStringW szSHA1;

} APPLICATION_INFO, *PAPPLICATION_INFO;

extern ATL::CAtlList<PAPPLICATION_INFO> InfoList;

typedef BOOL(CALLBACK *AVAILENUMPROC)(PAPPLICATION_INFO Info, LPCWSTR szFolderPath);

struct CAvailableApplicationInfo : public APPLICATION_INFO
{
    ATL::CStringW szInstalledVersion;
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
    BOOL m_IsInstalled = FALSE;
    BOOL m_HasLanguageInfo = FALSE;
    BOOL m_HasInstalledVersion = FALSE;
    CConfigParser m_Parser;

    inline BOOL GetString(LPCWSTR lpKeyName,
                          ATL::CStringW& ReturnedString);

    // Lazily load general info from the file
    VOID RetrieveGeneralInfo();
    VOID RetrieveInstalledStatus();
    VOID RetrieveInstalledVersion();
    VOID RetrieveLanguages();
    VOID RetrieveLicenseType();
    inline BOOL FindInLanguages(LCID what) const;
};

class CAvailableApps
{
    ATL::CAtlList<CAvailableApplicationInfo*> m_InfoList;
    ATL::CStringW m_szPath;
    ATL::CStringW m_szCabPath;
    ATL::CStringW m_szAppsPath;
    ATL::CStringW m_szSearchPath;

public:
    CAvailableApps();
    VOID FreeCachedEntries();
    BOOL DeleteCurrentAppsDB();
    BOOL UpdateAppsDB();
    BOOL EnumAvailableApplications(INT EnumType, AVAILENUMPROC lpEnumProc);
    const ATL::CStringW& GetFolderPath();
    const ATL::CStringW& GetAppPath();
    const ATL::CStringW& GetCabPath();
    const LPCWSTR GetFolderPathString();
    const LPCWSTR GetAppPathString();
    const LPCWSTR GetCabPathString();
};