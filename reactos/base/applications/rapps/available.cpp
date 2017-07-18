/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/available.cpp
 * PURPOSE:         Classes for working with available applications
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *                  Alexander Shaposhnikov     (chaez.san@gmail.com)
 */

#include "rapps.h"

 // CAvailableApplicationInfo

CAvailableApplicationInfo::CAvailableApplicationInfo(const ATL::CStringW& sFileNameParam)
{
    LicenseType = LICENSE_TYPE::None;
    sFileName = sFileNameParam;
    RetrieveCategory();
}

VOID CAvailableApplicationInfo::RefreshAppInfo()
{
    if (RetrieveGeneralInfo())
    {
        RetrieveLicenseType();
        RetrieveLanguages();
        RetrieveInstalledStatus();
        if (m_IsInstalled)
        {
            RetrieveInstalledVersion();
        }
    }
}

BOOL CAvailableApplicationInfo::RetrieveGeneralInfo()
{
    if (szUrlDownload.IsEmpty())
    {
        if (!GetString(L"Name", szName)
            || !GetString(L"URLDownload", szUrlDownload))
        {
            return FALSE;
        }

        GetString(L"RegName", szRegName);
        GetString(L"Version", szVersion);
        GetString(L"License", szLicense);
        GetString(L"Description", szDesc);
        GetString(L"Size", szSize);
        GetString(L"URLSite", szUrlSite);
        GetString(L"CDPath", szCDPath);
        GetString(L"Language", szRegName);
        GetString(L"SHA1", szSHA1);
        return TRUE;
    }
    return FALSE;
}

VOID CAvailableApplicationInfo::RetrieveInstalledStatus()
{
    m_IsInstalled = ::GetInstalledVersion(NULL, szRegName)
        || ::GetInstalledVersion(NULL, szName);
}

VOID CAvailableApplicationInfo::RetrieveInstalledVersion()
{
    m_HasInstalledVersion = ::GetInstalledVersion(&szInstalledVersion, szRegName)
        || ::GetInstalledVersion(&szInstalledVersion, szName);
}

BOOL CAvailableApplicationInfo::RetrieveLanguages()
{
    const WCHAR cDelimiter = L'|';
    ATL::CStringW szBuffer;

    // TODO: Get multiline parameter
    if (!ParserGetString(L"Languages", sFileName, szBuffer))
        return FALSE;

    // Parse parameter string
    ATL::CStringW szLocale;
    for (INT i = 0; szBuffer[i] != UNICODE_NULL; ++i)
    {
        if (szBuffer[i] != cDelimiter)
        {
            szLocale += szBuffer[i];
        }
        else
        {
            Languages.Add(szLocale);
            szLocale.Empty();
        }
    }

    // For the text after delimiter
    if (!szLocale.IsEmpty())
    {
        Languages.Add(szLocale);
    }

    return TRUE;
}

VOID CAvailableApplicationInfo::RetrieveLicenseType()
{
    INT IntBuffer = ParserGetInt(L"LicenseType", sFileName);

    if (IntBuffer < 0 || IntBuffer > LICENSE_TYPE::Max)
    {
        LicenseType = LICENSE_TYPE::None;
    }
    else
    {
        LicenseType = (LICENSE_TYPE) IntBuffer;
    }
}

VOID CAvailableApplicationInfo::RetrieveCategory()
{
    Category = ParserGetInt(L"Category", sFileName);
}

BOOL CAvailableApplicationInfo::HasLanguageInfo() const
{
    return m_HasLanguageInfo;
}

BOOL CAvailableApplicationInfo::HasNativeLanguage() const
{
    if (!m_HasLanguageInfo)
    {
        return FALSE;
    }

    //TODO: make the actual check
    return TRUE;
}

BOOL CAvailableApplicationInfo::HasEnglishLanguage() const
{
    if (!m_HasLanguageInfo)
    {
        return FALSE;
    }

    //TODO: make the actual check
    return TRUE;
}

BOOL CAvailableApplicationInfo::IsInstalled() const
{
    return m_IsInstalled;
}

BOOL CAvailableApplicationInfo::HasInstalledVersion() const
{
    return m_HasInstalledVersion;
}

BOOL CAvailableApplicationInfo::HasUpdate() const
{
    return (szInstalledVersion.Compare(szVersion) < 0) ? TRUE : FALSE;
}

VOID CAvailableApplicationInfo::SetLastWriteTime(FILETIME* ftTime)
{
    RtlCopyMemory(&ftCacheStamp, ftTime, sizeof(FILETIME));
}

inline BOOL CAvailableApplicationInfo::GetString(LPCWSTR lpKeyName, ATL::CStringW& ReturnedString)
{
    if (!ParserGetString(lpKeyName, sFileName, ReturnedString))
    {
        ReturnedString.Empty();
        return FALSE;
    }
    return TRUE;
}

// CAvailableApps
CAvailableApps::CAvailableApps()
{
    //set all paths
    if (GetStorageDirectory(m_szPath))
    {
        m_szAppsPath = m_szPath + L"\\rapps\\";
        m_szCabPath = m_szPath + L"\\rappmgr.cab";
        m_szSearchPath = m_szAppsPath + L"*.txt";
    }
}

VOID CAvailableApps::FreeCachedEntries()
{
    POSITION InfoListPosition = m_InfoList.GetHeadPosition();

    /* loop and deallocate all the cached app infos in the list */
    while (InfoListPosition)
    {
        CAvailableApplicationInfo* Info = m_InfoList.GetAt(InfoListPosition);
        m_InfoList.RemoveHead();
        delete Info;

        InfoListPosition = m_InfoList.GetHeadPosition();
    }
}

BOOL CAvailableApps::DeleteCurrentAppsDB()
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;
    BOOL result = TRUE;

    if (m_szPath.IsEmpty())
        return FALSE;

    result = result && DeleteFileW(m_szCabPath.GetString());
    hFind = FindFirstFileW(m_szSearchPath.GetString(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return result;

    ATL::CStringW szTmp;
    do
    {
        szTmp = m_szPath + FindFileData.cFileName;
        result = result && DeleteFileW(szTmp.GetString());
    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return result;
}

BOOL CAvailableApps::UpdateAppsDB()
{
    if (!DeleteCurrentAppsDB())
        return FALSE;

    DownloadApplicationsDB(APPLICATION_DATABASE_URL);

    if (m_szPath.IsEmpty())
        return FALSE;

    if (!ExtractFilesFromCab(m_szCabPath, m_szAppsPath))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CAvailableApps::EnumAvailableApplications(INT EnumType, AVAILENUMPROC lpEnumProc)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;

    if (!CreateDirectoryW(m_szPath.GetString(), NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return FALSE;
    }

    hFind = FindFirstFileW(m_szSearchPath.GetString(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (GetFileAttributesW(m_szCabPath) == INVALID_FILE_ATTRIBUTES)
            DownloadApplicationsDB(APPLICATION_DATABASE_URL);

        ExtractFilesFromCab(m_szCabPath, m_szAppsPath);
        hFind = FindFirstFileW(m_szSearchPath.GetString(), &FindFileData);

        if (hFind == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    do
    {
        /* loop for all the cached entries */
        POSITION CurrentListPosition = m_InfoList.GetHeadPosition();
        CAvailableApplicationInfo* Info = NULL;

        while (CurrentListPosition != NULL)
        {
            POSITION LastListPosition = CurrentListPosition;
            Info = m_InfoList.GetNext(CurrentListPosition);

            /* do we already have this entry in cache? */
            if (Info->sFileName == FindFileData.cFileName)
            {
                /* is it current enough, or the file has been modified since our last time here? */
                if (CompareFileTime(&FindFileData.ftLastWriteTime, &Info->ftCacheStamp) == 1)
                {
                    /* recreate our cache, this is the slow path */
                    m_InfoList.RemoveAt(LastListPosition);

                    delete Info;
                    Info = nullptr;
                    break;
                }
                else
                {
                    /* speedy path, compare directly, we already have the data */
                    goto skip_if_cached;
                }
            }
        }

        /* create a new entry */
        Info = new CAvailableApplicationInfo(FindFileData.cFileName);

        /* set a timestamp for the next time */
        Info->SetLastWriteTime(&FindFileData.ftLastWriteTime);
        m_InfoList.AddTail(Info);

skip_if_cached:
        if (Info->Category == FALSE)
            continue;

        if (EnumType != Info->Category && EnumType != ENUM_ALL_AVAILABLE)
            continue;

        Info->RefreshAppInfo();

        if (!lpEnumProc(static_cast<PAPPLICATION_INFO>(Info), m_szAppsPath.GetString()))
            break;

    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);
    return TRUE;
}

const ATL::CStringW & CAvailableApps::GetFolderPath()
{
    return m_szPath;
}

const ATL::CStringW & CAvailableApps::GetAppPath()
{
    return m_szAppsPath;
}

const ATL::CStringW & CAvailableApps::GetCabPath()
{
    return m_szCabPath;
}

const LPCWSTR CAvailableApps::GetFolderPathString()
{
    return m_szPath.GetString();
}

const LPCWSTR CAvailableApps::GetAppPathString()
{
    return m_szPath.GetString();
}

const LPCWSTR CAvailableApps::GetCabPathString()
{
    return m_szPath.GetString();
}
