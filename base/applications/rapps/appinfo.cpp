/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with available applications
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2020 He Yang (1160386205@qq.com)
 *              Copyright 2021-2023 Mark Jansen <mark.jansen@reactos.org>
 */

#include "rapps.h"
#include "appview.h"

CAppInfo::CAppInfo(const CStringW &Identifier, AppsCategories Category)
    : szIdentifier(Identifier), iCategory(Category)
{
}

CAppInfo::~CAppInfo()
{
}

CAvailableApplicationInfo::CAvailableApplicationInfo(
    CConfigParser *Parser,
    const CStringW &PkgName,
    AppsCategories Category,
    const CPathW &BasePath)
    : CAppInfo(PkgName, Category), m_Parser(Parser), m_ScrnshotRetrieved(false), m_LanguagesLoaded(false)
{
    m_Parser->GetString(L"Name", szDisplayName);
    m_Parser->GetString(L"Version", szDisplayVersion);
    m_Parser->GetString(L"URLDownload", m_szUrlDownload);
    m_Parser->GetString(L"Description", szComments);

    CPathW IconPath = BasePath;
    IconPath += L"icons";

    CStringW IconName;
    if (m_Parser->GetString(L"Icon", IconName))
    {
        IconPath += IconName;
    }
    else
    {
        // inifile.ico
        IconPath += (szIdentifier + L".ico");
    }

    if (PathFileExistsW(IconPath))
    {
        szDisplayIcon = (LPCWSTR)IconPath;
    }

    INT iSizeBytes;

    if (m_Parser->GetInt(L"SizeBytes", iSizeBytes))
    {
        StrFormatByteSizeW(iSizeBytes, m_szSize.GetBuffer(MAX_PATH), MAX_PATH);
        m_szSize.ReleaseBuffer();
    }

    m_Parser->GetString(L"URLSite", m_szUrlSite);
}

CAvailableApplicationInfo::~CAvailableApplicationInfo()
{
    delete m_Parser;
}

VOID
CAvailableApplicationInfo::ShowAppInfo(CAppRichEdit *RichEdit)
{
    RichEdit->SetText(szDisplayName, CFE_BOLD);
    InsertVersionInfo(RichEdit);
    RichEdit->LoadAndInsertText(IDS_AINFO_LICENSE, LicenseString(), 0);
    InsertLanguageInfo(RichEdit);

    RichEdit->LoadAndInsertText(IDS_AINFO_SIZE, m_szSize, 0);
    RichEdit->LoadAndInsertText(IDS_AINFO_URLSITE, m_szUrlSite, CFE_LINK);
    RichEdit->LoadAndInsertText(IDS_AINFO_DESCRIPTION, szComments, 0);
    RichEdit->LoadAndInsertText(IDS_AINFO_URLDOWNLOAD, m_szUrlDownload, CFE_LINK);
    RichEdit->LoadAndInsertText(IDS_AINFO_PACKAGE_NAME, szIdentifier, 0);
}

int
CompareVersion(const CStringW &left, const CStringW &right)
{
    int nLeft = 0, nRight = 0;

    while (true)
    {
        CStringW leftPart = left.Tokenize(L".", nLeft);
        CStringW rightPart = right.Tokenize(L".", nRight);

        if (leftPart.IsEmpty() && rightPart.IsEmpty())
            return 0;
        if (leftPart.IsEmpty())
            return -1;
        if (rightPart.IsEmpty())
            return 1;

        int leftVal, rightVal;

        if (!StrToIntExW(leftPart, STIF_DEFAULT, &leftVal))
            leftVal = 0;
        if (!StrToIntExW(rightPart, STIF_DEFAULT, &rightVal))
            rightVal = 0;

        if (leftVal > rightVal)
            return 1;
        if (rightVal < leftVal)
            return -1;
    }
}

VOID
CAvailableApplicationInfo::InsertVersionInfo(CAppRichEdit *RichEdit)
{
    CStringW szRegName;
    m_Parser->GetString(DB_REGNAME, szRegName);

    BOOL bIsInstalled = ::GetInstalledVersion(NULL, szRegName) || ::GetInstalledVersion(NULL, szDisplayName);
    if (bIsInstalled)
    {
        CStringW szInstalledVersion;
        CStringW szNameVersion = szDisplayName + L" " + szDisplayVersion;
        BOOL bHasInstalledVersion = ::GetInstalledVersion(&szInstalledVersion, szRegName) ||
                                    ::GetInstalledVersion(&szInstalledVersion, szDisplayName) ||
                                    ::GetInstalledVersion(&szInstalledVersion, szNameVersion);

        if (bHasInstalledVersion)
        {
            BOOL bHasUpdate = CompareVersion(szInstalledVersion, szDisplayVersion) < 0;
            if (bHasUpdate)
            {
                RichEdit->LoadAndInsertText(IDS_STATUS_UPDATE_AVAILABLE, CFE_ITALIC);
                RichEdit->LoadAndInsertText(IDS_AINFO_VERSION, szInstalledVersion, 0);
            }
            else
            {
                RichEdit->LoadAndInsertText(IDS_STATUS_INSTALLED, CFE_ITALIC);
            }
        }
        else
        {
            RichEdit->LoadAndInsertText(IDS_STATUS_INSTALLED, CFE_ITALIC);
        }
    }
    else
    {
        RichEdit->LoadAndInsertText(IDS_STATUS_NOTINSTALLED, CFE_ITALIC);
    }

    RichEdit->LoadAndInsertText(IDS_AINFO_AVAILABLEVERSION, szDisplayVersion, 0);
}

CStringW
CAvailableApplicationInfo::LicenseString()
{
    INT IntBuffer;
    m_Parser->GetInt(L"LicenseType", IntBuffer);
    CStringW szLicenseString;
    m_Parser->GetString(L"License", szLicenseString);
    LicenseType licenseType;

    if (IsKnownLicenseType(IntBuffer))
        licenseType = static_cast<LicenseType>(IntBuffer);
    else
        licenseType = LICENSE_NONE;

    if (licenseType == LICENSE_NONE || licenseType == LICENSE_FREEWARE)
    {
        if (szLicenseString.CompareNoCase(L"Freeware") == 0)
        {
            licenseType = LICENSE_FREEWARE;
            szLicenseString = L""; // Don't display as "Freeware (Freeware)"
        }
    }

    CStringW szLicense;
    switch (licenseType)
    {
        case LICENSE_OPENSOURCE:
            szLicense.LoadStringW(IDS_LICENSE_OPENSOURCE);
            break;
        case LICENSE_FREEWARE:
            szLicense.LoadStringW(IDS_LICENSE_FREEWARE);
            break;
        case LICENSE_TRIAL:
            szLicense.LoadStringW(IDS_LICENSE_TRIAL);
            break;
        default:
            return szLicenseString;
    }

    if (!szLicenseString.IsEmpty())
        szLicense += L" (" + szLicenseString + L")";
    return szLicense;
}

VOID
CAvailableApplicationInfo::InsertLanguageInfo(CAppRichEdit *RichEdit)
{
    if (!m_LanguagesLoaded)
    {
        RetrieveLanguages();
    }

    if (m_LanguageLCIDs.GetSize() == 0)
    {
        return;
    }

    const INT nTranslations = m_LanguageLCIDs.GetSize();
    CStringW szLangInfo;
    CStringW szLoadedTextAvailability;
    CStringW szLoadedAInfoText;

    szLoadedAInfoText.LoadStringW(IDS_AINFO_LANGUAGES);

    const LCID lcEnglish = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT);
    if (m_LanguageLCIDs.Find(GetUserDefaultLCID()) >= 0)
    {
        szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_AVAILABLE_TRANSLATION);
        if (nTranslations > 1)
        {
            CStringW buf;
            buf.LoadStringW(IDS_LANGUAGE_MORE_PLACEHOLDER);
            szLangInfo.Format(buf, nTranslations - 1);
        }
        else
        {
            szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
            szLangInfo = L" (" + szLangInfo + L")";
        }
    }
    else if (m_LanguageLCIDs.Find(lcEnglish) >= 0)
    {
        szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_ENGLISH_TRANSLATION);
        if (nTranslations > 1)
        {
            CStringW buf;
            buf.LoadStringW(IDS_LANGUAGE_AVAILABLE_PLACEHOLDER);
            szLangInfo.Format(buf, nTranslations - 1);
        }
        else
        {
            szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
            szLangInfo = L" (" + szLangInfo + L")";
        }
    }
    else
    {
        szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_NO_TRANSLATION);
    }

    RichEdit->InsertText(szLoadedAInfoText, CFE_BOLD);
    RichEdit->InsertText(szLoadedTextAvailability, NULL);
    RichEdit->InsertText(szLangInfo, CFE_ITALIC);
}

VOID
CAvailableApplicationInfo::RetrieveLanguages()
{
    m_LanguagesLoaded = true;

    CStringW szBuffer;
    if (!m_Parser->GetString(L"Languages", szBuffer))
    {
        return;
    }

    // Parse parameter string
    int iIndex = 0;
    while (true)
    {
        CStringW szLocale = szBuffer.Tokenize(L"|", iIndex);
        if (szLocale.IsEmpty())
            break;

        szLocale = L"0x" + szLocale;

        INT iLCID;
        if (StrToIntExW(szLocale, STIF_SUPPORT_HEX, &iLCID))
        {
            m_LanguageLCIDs.Add(static_cast<LCID>(iLCID));
        }
    }
}

BOOL
CAvailableApplicationInfo::Valid() const
{
    return !szDisplayName.IsEmpty() && !m_szUrlDownload.IsEmpty();
}

BOOL
CAvailableApplicationInfo::CanModify()
{
    return FALSE;
}

BOOL
CAvailableApplicationInfo::RetrieveIcon(CStringW &Path) const
{
    Path = szDisplayIcon;
    return !Path.IsEmpty();
}

#define MAX_SCRNSHOT_NUM 16
BOOL
CAvailableApplicationInfo::RetrieveScreenshot(CStringW &Path)
{
    if (!m_ScrnshotRetrieved)
    {
        static_assert(MAX_SCRNSHOT_NUM < 10000, "MAX_SCRNSHOT_NUM is too big");
        for (int i = 0; i < MAX_SCRNSHOT_NUM; i++)
        {
            CStringW ScrnshotField;
            ScrnshotField.Format(L"Screenshot%d", i + 1);
            CStringW ScrnshotLocation;
            if (!m_Parser->GetString(ScrnshotField, ScrnshotLocation))
            {
                // We stop at the first screenshot not found,
                // so screenshots _have_ to be consecutive
                break;
            }

            if (PathIsURLW(ScrnshotLocation.GetString()))
            {
                m_szScrnshotLocation.Add(ScrnshotLocation);
            }
        }
        m_ScrnshotRetrieved = true;
    }

    if (m_szScrnshotLocation.GetSize() > 0)
    {
        Path = m_szScrnshotLocation[0];
    }

    return !Path.IsEmpty();
}

VOID
CAvailableApplicationInfo::GetDownloadInfo(CStringW &Url, CStringW &Sha1, ULONG &SizeInBytes) const
{
    Url = m_szUrlDownload;
    m_Parser->GetString(L"SHA1", Sha1);
    INT iSizeBytes;

    if (m_Parser->GetInt(L"SizeBytes", iSizeBytes))
    {
        SizeInBytes = (ULONG)iSizeBytes;
    }
    else
    {
        SizeInBytes = 0;
    }
}

VOID
CAvailableApplicationInfo::GetDisplayInfo(CStringW &License, CStringW &Size, CStringW &UrlSite, CStringW &UrlDownload)
{
    License = LicenseString();
    Size = m_szSize;
    UrlSite = m_szUrlSite;
    UrlDownload = m_szUrlDownload;
}

InstallerType
CAvailableApplicationInfo::GetInstallerType() const
{
    CStringW str;
    m_Parser->GetString(DB_INSTALLER, str);
    if (str.CompareNoCase(DB_GENINSTSECTION) == 0)
        return INSTALLER_GENERATE;
    else
        return INSTALLER_UNKNOWN;
}

BOOL
CAvailableApplicationInfo::UninstallApplication(UninstallCommandFlags Flags)
{
    ATLASSERT(FALSE && "Should not be called");
    return FALSE;
}

CInstalledApplicationInfo::CInstalledApplicationInfo(
    HKEY Key,
    const CStringW &KeyName,
    AppsCategories Category, UINT KeyInfo)
    : CAppInfo(KeyName, Category), m_hKey(Key), m_KeyInfo(KeyInfo)
{
    if (GetApplicationRegString(L"DisplayName", szDisplayName))
    {
        GetApplicationRegString(L"DisplayIcon", szDisplayIcon);
        GetApplicationRegString(L"DisplayVersion", szDisplayVersion);
        GetApplicationRegString(L"Comments", szComments);
    }
}

CInstalledApplicationInfo::~CInstalledApplicationInfo()
{
}

VOID
CInstalledApplicationInfo::AddApplicationRegString(
    CAppRichEdit *RichEdit,
    UINT StringID,
    const CStringW &String,
    DWORD TextFlags)
{
    CStringW Tmp;
    if (GetApplicationRegString(String, Tmp))
    {
        RichEdit->InsertTextWithString(StringID, Tmp, TextFlags);
    }
}

VOID
CInstalledApplicationInfo::ShowAppInfo(CAppRichEdit *RichEdit)
{
    RichEdit->SetText(szDisplayName, CFE_BOLD);
    RichEdit->InsertText(L"\n", 0);

    RichEdit->InsertTextWithString(IDS_INFO_VERSION, szDisplayVersion, 0);
    AddApplicationRegString(RichEdit, IDS_INFO_PUBLISHER, L"Publisher", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_REGOWNER, L"RegOwner", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_PRODUCTID, L"ProductID", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_HELPLINK, L"HelpLink", CFM_LINK);
    AddApplicationRegString(RichEdit, IDS_INFO_HELPPHONE, L"HelpTelephone", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_README, L"Readme", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_CONTACT, L"Contact", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_UPDATEINFO, L"URLUpdateInfo", CFM_LINK);
    AddApplicationRegString(RichEdit, IDS_INFO_INFOABOUT, L"URLInfoAbout", CFM_LINK);
    RichEdit->InsertTextWithString(IDS_INFO_COMMENTS, szComments, 0);

    if (m_szInstallDate.IsEmpty())
    {
        RetrieveInstallDate();
    }

    RichEdit->InsertTextWithString(IDS_INFO_INSTALLDATE, m_szInstallDate, 0);
    AddApplicationRegString(RichEdit, IDS_INFO_INSTLOCATION, L"InstallLocation", 0);
    AddApplicationRegString(RichEdit, IDS_INFO_INSTALLSRC, L"InstallSource", 0);

    if (m_szUninstallString.IsEmpty())
    {
        RetrieveUninstallStrings();
    }

    RichEdit->InsertTextWithString(IDS_INFO_UNINSTALLSTR, m_szUninstallString, 0);
    RichEdit->InsertTextWithString(IDS_INFO_MODIFYPATH, m_szModifyString, 0);
}

VOID
CInstalledApplicationInfo::RetrieveInstallDate()
{
    DWORD dwInstallTimeStamp;
    SYSTEMTIME InstallLocalTime;
    if (GetApplicationRegString(L"InstallDate", m_szInstallDate))
    {
        ZeroMemory(&InstallLocalTime, sizeof(InstallLocalTime));
        // Check if we have 8 characters to parse the datetime.
        // Maybe other formats exist as well?
        m_szInstallDate = m_szInstallDate.Trim();
        if (m_szInstallDate.GetLength() == 8)
        {
            InstallLocalTime.wYear = wcstol(m_szInstallDate.Left(4).GetString(), NULL, 10);
            InstallLocalTime.wMonth = wcstol(m_szInstallDate.Mid(4, 2).GetString(), NULL, 10);
            InstallLocalTime.wDay = wcstol(m_szInstallDate.Mid(6, 2).GetString(), NULL, 10);
        }
    }
    // It might be a DWORD (Unix timestamp). try again.
    else if (GetApplicationRegDword(L"InstallDate", &dwInstallTimeStamp))
    {
        FILETIME InstallFileTime;
        SYSTEMTIME InstallSystemTime;

        UnixTimeToFileTime(dwInstallTimeStamp, &InstallFileTime);
        FileTimeToSystemTime(&InstallFileTime, &InstallSystemTime);

        // convert to localtime
        SystemTimeToTzSpecificLocalTime(NULL, &InstallSystemTime, &InstallLocalTime);
    }

    // convert to readable date string
    int cchTimeStrLen = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &InstallLocalTime, NULL, 0, 0);

    GetDateFormatW(
        LOCALE_USER_DEFAULT, // use default locale for current user
        0, &InstallLocalTime, NULL, m_szInstallDate.GetBuffer(cchTimeStrLen), cchTimeStrLen);
    m_szInstallDate.ReleaseBuffer();
}

VOID
CInstalledApplicationInfo::RetrieveUninstallStrings()
{
    DWORD dwWindowsInstaller = 0;
    if (GetApplicationRegDword(L"WindowsInstaller", &dwWindowsInstaller) && dwWindowsInstaller)
    {
        // MSI has the same info in Uninstall / modify, so manually build it
        m_szUninstallString.Format(L"msiexec /x%s", szIdentifier.GetString());
    }
    else
    {
        GetApplicationRegString(L"UninstallString", m_szUninstallString);
    }
    DWORD dwNoModify = 0;
    if (!GetApplicationRegDword(L"NoModify", &dwNoModify))
    {
        CStringW Tmp;
        if (GetApplicationRegString(L"NoModify", Tmp))
        {
            dwNoModify = Tmp.GetLength() > 0 ? (Tmp[0] == '1') : 0;
        }
        else
        {
            dwNoModify = 0;
        }
    }
    if (!dwNoModify)
    {
        if (dwWindowsInstaller)
        {
            m_szModifyString.Format(L"msiexec /i%s", szIdentifier.GetString());
        }
        else
        {
            GetApplicationRegString(L"ModifyPath", m_szModifyString);
        }
    }
}

BOOL
CInstalledApplicationInfo::Valid() const
{
    return !szDisplayName.IsEmpty();
}

BOOL
CInstalledApplicationInfo::CanModify()
{
    if (m_szUninstallString.IsEmpty())
    {
        RetrieveUninstallStrings();
    }

    return !m_szModifyString.IsEmpty();
}

BOOL
CInstalledApplicationInfo::RetrieveIcon(CStringW &Path) const
{
    Path = szDisplayIcon;
    return !Path.IsEmpty();
}

BOOL
CInstalledApplicationInfo::RetrieveScreenshot(CStringW & /*Path*/)
{
    return FALSE;
}

VOID
CInstalledApplicationInfo::GetDownloadInfo(CStringW &Url, CStringW &Sha1, ULONG &SizeInBytes) const
{
    ATLASSERT(FALSE && "Should not be called");
}

VOID
CInstalledApplicationInfo::GetDisplayInfo(CStringW &License, CStringW &Size, CStringW &UrlSite, CStringW &UrlDownload)
{
    ATLASSERT(FALSE && "Should not be called");
}

InstallerType
CInstalledApplicationInfo::GetInstallerType() const
{
    CRegKey reg;
    if (reg.Open(m_hKey, GENERATE_ARPSUBKEY, KEY_READ) == ERROR_SUCCESS)
    {
        return INSTALLER_GENERATE;
    }
    return INSTALLER_UNKNOWN;
}

BOOL
CInstalledApplicationInfo::UninstallApplication(UninstallCommandFlags Flags)
{
    if (GetInstallerType() == INSTALLER_GENERATE)
    {
        return UninstallGenerated(*this, Flags);
    }

    BOOL bModify = Flags & UCF_MODIFY;
    if (m_szUninstallString.IsEmpty())
    {
        RetrieveUninstallStrings();
    }

    CStringW cmd = bModify ? m_szModifyString : m_szUninstallString;
    if ((Flags & (UCF_MODIFY | UCF_SILENT)) == UCF_SILENT)
    {
        DWORD msi = 0;
        msi = GetApplicationRegDword(L"WindowsInstaller", &msi) && msi;
        if (msi)
        {
            cmd += L" /qn";
        }
        else
        {
            CStringW silentcmd;
            if (GetApplicationRegString(L"QuietUninstallString", silentcmd) && !silentcmd.IsEmpty())
            {
                cmd = silentcmd;
            }
        }
    }

    BOOL bSuccess = StartProcess(cmd, TRUE);

    if (bSuccess && !bModify)
        WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_REMOVE, szDisplayName);

    return bSuccess;
}

BOOL
CInstalledApplicationInfo::GetApplicationRegString(LPCWSTR lpKeyName, CStringW &String)
{
    ULONG nChars = 0;
    // Get the size
    if (m_hKey.QueryStringValue(lpKeyName, NULL, &nChars) != ERROR_SUCCESS)
    {
        String.Empty();
        return FALSE;
    }

    LPWSTR Buffer = String.GetBuffer(nChars);
    LONG lResult = m_hKey.QueryStringValue(lpKeyName, Buffer, &nChars);
    if (nChars > 0 && Buffer[nChars - 1] == UNICODE_NULL)
        nChars--;
    String.ReleaseBuffer(nChars);

    if (lResult != ERROR_SUCCESS)
    {
        String.Empty();
        return FALSE;
    }

    if (String.Find('%') >= 0)
    {
        CStringW Tmp;
        DWORD dwLen = ExpandEnvironmentStringsW(String, NULL, 0);
        if (dwLen > 0)
        {
            BOOL bSuccess = ExpandEnvironmentStringsW(String, Tmp.GetBuffer(dwLen), dwLen) == dwLen;
            Tmp.ReleaseBuffer(dwLen - 1);
            if (bSuccess)
            {
                String = Tmp;
            }
            else
            {
                String.Empty();
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL
CInstalledApplicationInfo::GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue)
{
    DWORD dwSize = sizeof(DWORD), dwType;
    if (RegQueryValueExW(m_hKey, lpKeyName, NULL, &dwType, (LPBYTE)lpValue, &dwSize) != ERROR_SUCCESS ||
        dwType != REG_DWORD)
    {
        return FALSE;
    }

    return TRUE;
}
