#pragma once

#include <atlstr.h>
#include <atlpath.h>
#include <atlsimpcoll.h>


enum LicenseType
{
    LICENSE_NONE,
    LICENSE_OPENSOURCE = 1,
    LICENSE_FREEWARE = 2,
    LICENSE_TRIAL = 3,
    LICENSE_MIN = LICENSE_NONE,
    LICENSE_MAX = LICENSE_TRIAL
};

inline BOOL
IsKnownLicenseType(INT x)
{
    return (x > LICENSE_NONE && x <= LICENSE_MAX);
}

enum AppsCategories
{
    ENUM_ALL_AVAILABLE,
    ENUM_CAT_AUDIO,
    ENUM_CAT_VIDEO,
    ENUM_CAT_GRAPHICS,
    ENUM_CAT_GAMES,
    ENUM_CAT_INTERNET,
    ENUM_CAT_OFFICE,
    ENUM_CAT_DEVEL,
    ENUM_CAT_EDU,
    ENUM_CAT_ENGINEER,
    ENUM_CAT_FINANCE,
    ENUM_CAT_SCIENCE,
    ENUM_CAT_TOOLS,
    ENUM_CAT_DRIVERS,
    ENUM_CAT_LIBS,
    ENUM_CAT_THEMES,
    ENUM_CAT_OTHER,
    ENUM_CAT_SELECTED,
    ENUM_ALL_INSTALLED = 30,
    ENUM_INSTALLED_APPLICATIONS,
    ENUM_UPDATES,
    ENUM_INVALID,
    ENUM_INSTALLED_MIN = ENUM_ALL_INSTALLED,
    ENUM_INSTALLED_MAX = ENUM_UPDATES,
    ENUM_AVAILABLE_MIN = ENUM_ALL_AVAILABLE,
    ENUM_AVAILABLE_MAX = ENUM_CAT_SELECTED,
};

inline BOOL
IsAvailableEnum(INT x)
{
    return (x >= ENUM_AVAILABLE_MIN && x <= ENUM_AVAILABLE_MAX);
}

inline BOOL
IsInstalledEnum(INT x)
{
    return (x >= ENUM_INSTALLED_MIN && x <= ENUM_INSTALLED_MAX);
}

enum UninstallCommandFlags
{
    UCF_NONE   = 0x00,
    UCF_MODIFY = 0x01,
    UCF_SILENT = 0x02,
};

enum InstallerType
{
    INSTALLER_UNKNOWN,
    INSTALLER_GENERATE, // .zip file automatically converted to installer by rapps
};

#define DB_VERSION L"Version"
#define DB_CATEGORY L"Category"
#define DB_PUBLISHER L"Publisher"
#define DB_REGNAME L"RegName"
#define DB_INSTALLER L"Installer"
#define DB_SCOPE L"Scope" // User or Machine
#define DB_SAVEAS L"SaveAs"

#define DB_GENINSTSECTION L"Generate"
#define GENERATE_ARPSUBKEY L"RApps" // Our uninstall data is stored here

class CAppRichEdit;
class CConfigParser;

class CAppInfo
{
  public:
    CAppInfo(const CStringW &Identifier, AppsCategories Category);
    virtual ~CAppInfo();

    const CStringW szIdentifier; // PkgName or KeyName
    const AppsCategories iCategory;

    CStringW szDisplayIcon;
    CStringW szDisplayName;
    CStringW szDisplayVersion;
    CStringW szComments;

    virtual BOOL
    Valid() const = 0;
    virtual BOOL
    CanModify() = 0;
    virtual BOOL
    RetrieveIcon(CStringW &Path) const = 0;
    virtual BOOL
    RetrieveScreenshot(CStringW &Path) = 0;
    virtual VOID
    ShowAppInfo(CAppRichEdit *RichEdit) = 0;
    virtual VOID
    GetDownloadInfo(CStringW &Url, CStringW &Sha1, ULONG &SizeInBytes) const = 0;
    virtual VOID
    GetDisplayInfo(CStringW &License, CStringW &Size, CStringW &UrlSite, CStringW &UrlDownload) = 0;
    virtual InstallerType
    GetInstallerType() const { return INSTALLER_UNKNOWN; }
    virtual BOOL
    UninstallApplication(UninstallCommandFlags Flags) = 0;
};

class CAvailableApplicationInfo : public CAppInfo
{
    CConfigParser *m_Parser;
    CSimpleArray<CStringW> m_szScrnshotLocation;
    bool m_ScrnshotRetrieved;
    CStringW m_szUrlDownload;
    CStringW m_szSize;
    CStringW m_szUrlSite;
    CSimpleArray<LCID> m_LanguageLCIDs;
    bool m_LanguagesLoaded;

    VOID
    InsertVersionInfo(CAppRichEdit *RichEdit);
    VOID
    InsertLanguageInfo(CAppRichEdit *RichEdit);
    VOID
    RetrieveLanguages();
    CStringW
    LicenseString();

  public:
    CAvailableApplicationInfo(
        CConfigParser *Parser,
        const CStringW &PkgName,
        AppsCategories Category,
        const CPathW &BasePath);
    ~CAvailableApplicationInfo();

    CConfigParser *
    GetConfigParser() const { return m_Parser; }

    virtual BOOL
    Valid() const override;
    virtual BOOL
    CanModify() override;
    virtual BOOL
    RetrieveIcon(CStringW &Path) const override;
    virtual BOOL
    RetrieveScreenshot(CStringW &Path) override;
    virtual VOID
    ShowAppInfo(CAppRichEdit *RichEdit) override;
    virtual VOID
    GetDownloadInfo(CStringW &Url, CStringW &Sha1, ULONG &SizeInBytes) const override;
    virtual VOID
    GetDisplayInfo(CStringW &License, CStringW &Size, CStringW &UrlSite, CStringW &UrlDownload) override;
    virtual InstallerType
    GetInstallerType() const override;
    virtual BOOL
    UninstallApplication(UninstallCommandFlags Flags) override;
};

class CInstalledApplicationInfo : public CAppInfo
{
    CRegKey m_hKey;
    CStringW m_szInstallDate;
    CStringW m_szUninstallString;
    CStringW m_szModifyString;

    BOOL
    GetApplicationRegString(LPCWSTR lpKeyName, CStringW &String);
    BOOL
    GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue);
    VOID
    AddApplicationRegString(CAppRichEdit *RichEdit, UINT StringID, const CStringW &String, DWORD TextFlags);

    VOID
    RetrieveInstallDate();
    VOID
    RetrieveUninstallStrings();

  public:
    UINT m_KeyInfo;
    CInstalledApplicationInfo(HKEY Key, const CStringW &KeyName, AppsCategories Category, UINT KeyInfo);
    ~CInstalledApplicationInfo();

    CRegKey& GetRegKey() { return m_hKey; }

    virtual BOOL
    Valid() const override;
    virtual BOOL
    CanModify() override;
    virtual BOOL
    RetrieveIcon(CStringW &Path) const override;
    virtual BOOL
    RetrieveScreenshot(CStringW &Path) override;
    virtual VOID
    ShowAppInfo(CAppRichEdit *RichEdit) override;
    virtual VOID
    GetDownloadInfo(CStringW &Url, CStringW &Sha1, ULONG &SizeInBytes) const override;
    virtual VOID
    GetDisplayInfo(CStringW &License, CStringW &Size, CStringW &UrlSite, CStringW &UrlDownload) override;
    virtual InstallerType
    GetInstallerType() const override;
    virtual BOOL
    UninstallApplication(UninstallCommandFlags Flags) override;
};

BOOL
UninstallGenerated(CInstalledApplicationInfo &AppInfo, UninstallCommandFlags Flags);
BOOL
ExtractAndRunGeneratedInstaller(const CAvailableApplicationInfo &AppInfo, LPCWSTR Archive);
