/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cachecfg.cxx

Abstract:

    This module contains the functions to get and set disk cache
    configuration parameters.

    Contents:
        GetCacheConfigInfoA
        SetCacheConfigInfoA

Author:

    Sophia Chung (sophiac)  1-May-1995
    
Environment:

    User Mode - Win32

Revision History:
    Mucho rewritten by Akabir   1Q 98

    To understand how the new registration code works, it might be better for you to start with ConfigureCache, 
    GetCacheConfigInfo, etc. for a high level acquaintance; _then_ start poring over the actual registry sets code.
--*/

#include <cache.hxx>
#include <conmgr.hxx>
#include <time.h>

#include <shlobj.h>

#define CACHE_TAG           "Cache"

// Cache path keys.
CHAR* g_szSubKey[] = {CONTENT_PATH_KEY, COOKIE_PATH_KEY, HISTORY_PATH_KEY};
CHAR* g_szOldSubKey[] = {CACHE_TAG, COOKIE_PATH_KEY, HISTORY_PATH_KEY};

INT g_iContainerCSIDL[] = { CSIDL_INTERNET_CACHE, CSIDL_COOKIES, CSIDL_HISTORY };

// Top level cache paths resource IDs
#ifndef UNIX
DWORD g_dwCachePathResourceID[] = {IDS_CACHE_DEFAULT_SUBDIR, IDS_COOKIES_DEFAULT_SUBDIR, IDS_HISTORY_DEFAULT_SUBDIR};
#else
DWORD g_dwCachePathResourceID[] = {IDS_CACHE_DEFAULT_SUBDIR_UNIX, IDS_COOKIES_DEFAULT_SUBDIR, IDS_HISTORY_DEFAULT_SUBDIR};
#endif /* UNIX */

// Cache prefixes.
CHAR* g_szCachePrefix[] = {CONTENT_PREFIX, COOKIE_PREFIX, HISTORY_PREFIX};
CHAR* g_szVersionName[] = { CONTENT_VERSION_SUBDIR, "", "History.IE5" };

#define OLD_CACHE_PATH      "Path1"
#define OLD_CACHE_SUBKEY    DIR_SEPARATOR_STRING##"Cache1"

typedef BOOL (WINAPI *PFNGETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

void FlushShellFolderCache()
{
    SHFlushSFCacheWrap( );
    return;
}

typedef HRESULT (*PFNSHGETFOLDERPATH)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);

//#define DEBUG_CACHE_UPGRADE

#ifdef DEBUG_CACHE_UPGRADE
VOID LOG_UPGRADE_DATA(PTSTR pszData)
{
    CHAR szFileName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    CHAR szComputerName[MAX_PATH];
    HANDLE hResultsFile = NULL;
    strcpy(szFileName, "\\\\BANYAN\\IPTD\\AKABIR\\cacheupgrade\\");
    if (!GetComputerNameA(szComputerName, &dwSize))
    {
        goto exit;
    }
    lstrcatA(szFileName, szComputerName);
    hResultsFile = CreateFileA( szFileName,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        0,
                        NULL);
    if (hResultsFile != INVALID_HANDLE_VALUE)
    {
        if (SetFilePointer(hResultsFile, 0, NULL, FILE_END)==0xFFFFFFFF)
        {
            goto exit;
        }
        DWORD dwFoo;
        if (0==WriteFile(hResultsFile, (PVOID)pszData, lstrlenA(pszData), &dwFoo, NULL))
        {
            DWORD dwE = GetLastError();
        }
    }
exit:
    if (hResultsFile)
    {
        CloseHandle(hResultsFile);
    }
}
#else
#define LOG_UPGRADE_DATA(x)
#endif

#undef SHGetFolderPath

HRESULT SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    if (!g_HMODSHFolder)
    {
        // if we are on NT5, then skip shfolder.dll, go straight to shell32.dll
        g_HMODSHFolder = LoadLibrary(GlobalPlatformVersion5 ? "shell32.dll" : "shfolder.dll");
    }
    
    HRESULT hr = E_POINTER;
    if (g_HMODSHFolder) 
    {
        PFNSHGETFOLDERPATH pfn = (PFNSHGETFOLDERPATH)GetProcAddress(g_HMODSHFolder, "SHGetFolderPathA");
        if (pfn)
        {
            hr = pfn(hwnd, csidl, hToken, dwFlags, pszPath);
        }
    }
    return hr;
}

#define CACHE_SIZE_CAP 32000000

DWORD 
GetDefaultCacheQuota(
    LPSTR pszCachePath, 
    DWORD dwFraction
    )
{ 
    DWORDLONG cKBLimit = 0, cbTotal;

    if (GetDiskInfo(pszCachePath, NULL, NULL, &cbTotal))
    {
        cKBLimit = (cbTotal / (DWORDLONG)(1024*dwFraction));
    }
    if (cKBLimit<1024)
    {
        cKBLimit = DEF_CACHE_LIMIT;
    }
    else if (cKBLimit > CACHE_SIZE_CAP)
    {
        cKBLimit = CACHE_SIZE_CAP;
    }

    return (DWORD)cKBLimit;
}
        
VOID CleanPath(PTSTR pszPath);

/*-----------------------------------------------------------------------------
NormalisePath
 (code swiped from shell32\folder.c: UnexpandEnvironmentstring)

  Collapses paths of the form C:\foobar\dir1\...\dirn to
  %FOOBAR%\dir1\...\dirn
  where %FOOBAR% = "C:\foobar".
  storing result in pszOut.

  If collapse is not possible, returns FALSE and path is unchanged.
  If the given environment variable exists as the first part of the path,
  then the environment variable is inserted into the output buffer.

  Returns TRUE if pszResult is filled in.
  Example:  Input  -- C:\WINNT\SYSTEM32\FOO.TXT -and- lpEnvVar = %SystemRoot%
            Output -- %SystemRoot%\SYSTEM32\FOO.TXT

---------------------------------------------------------------------------*/


BOOL NormalisePath(LPCTSTR pszPath, LPCTSTR pszEnvVar, LPTSTR pszResult, UINT cbResult)
{
    TCHAR szEnvVar[MAX_PATH];
//     DWORD dwEnvVar = ExpandEnvironmentStrings(pszEnvVar, szEnvVar, sizeof(szEnvVar)) - 1; // don't count the NULL
//    akabir: a curious bug? causes ExpandEnvironmentStrings to return twice the number of characters.

    ExpandEnvironmentStrings(pszEnvVar, szEnvVar, sizeof(szEnvVar)-1); // don't count the NULL
    DWORD dwEnvVar = lstrlen(szEnvVar);

    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar, dwEnvVar, pszPath, dwEnvVar) == 2) 
    {
        if (lstrlen(pszPath) + dwEnvVar < cbResult)
        {
            strncpy(pszResult, pszEnvVar, MAX_PATH);
            strncat(pszResult, pszPath + dwEnvVar, MAX_PATH);
            return TRUE;
        }
    }
    return FALSE;
}



// ------------------------------------------------------------------------------------------------
// IE 3, 4 and 5 have different registry settings. These classes help ensure they all stay in sync.


// -- IE5's registry set ------------------------------------------------------------------------------------------------
// *** If using multiple registry sets, use InitialiseKeys for IE5 first.***
// This is to ensure that profiles-capabilities are noted.

class IE5_REGISTRYSET
{
protected:
    REGISTRY_OBJ m_roHKLMCache, m_roHKCUCache, m_roShellFolder, m_roUserShellFolder, m_roWorking;
    BOOL m_fProfiles;
    BOOL m_fWorkingPerUser;
    TCHAR m_szSharedPath[MAX_PATH];
    TCHAR m_szProfilePath[MAX_PATH];
    DWORD cbP, cbS, m_dwWorking;
    BOOL m_fInitialised;
    
    DWORD InitCommonKeys(BOOL fProfilesCapable, LPSTR pszReg)
    {
        DWORD dwError, dwFlag = CREATE_KEY_IF_NOT_EXISTS;

        m_fProfiles = fProfilesCapable;
        // Shared item info are located in HKLM/[...]/Internet Settings/5.0/Cache/*
        m_roHKLMCache.WorkWith(HKEY_LOCAL_MACHINE, pszReg, dwFlag);
        if (dwError = m_roHKLMCache.GetStatus()!=ERROR_SUCCESS)
        {
            m_roHKLMCache.WorkWith(HKEY_LOCAL_MACHINE, pszReg, dwFlag, BASIC_ACCESS);
            if (dwError = m_roHKLMCache.GetStatus()!=ERROR_SUCCESS)
                goto exit;
        }

        m_roShellFolder.WorkWith(HKEY_CURRENT_USER, SHELL_FOLDER_KEY, dwFlag);
        if (dwError = m_roShellFolder.GetStatus()!=ERROR_SUCCESS)
                goto exit;

        if (fProfilesCapable)
        {
            m_roHKCUCache.WorkWith(HKEY_CURRENT_USER, pszReg, dwFlag);
            if (dwError = m_roHKCUCache.GetStatus()!=ERROR_SUCCESS)
                goto exit;
        }

        m_roUserShellFolder.WorkWith(HKEY_CURRENT_USER, USER_SHELL_FOLDER_KEY, dwFlag);
        dwError = m_roUserShellFolder.GetStatus();
        if (dwError==ERROR_SUCCESS)
        {
            m_fInitialised = TRUE;
        }
        // Per-user items are located in HKCU/[...]/Explorer/Shell Folders and /Internet Settings/[5.0/]Cache/*
    exit:
        return dwError;
    }

    virtual BOOL DetermineKeyPlacing(DWORD dwWhich)
    {
        // Determine if this is a per-user item
        // HKCU overrides HKLM
        // If any of the following fail, for content, we'll default to shared.

        if (!m_fProfiles)
        {
            return FALSE;    
        }
        
        DWORD dwTemp;
        REGISTRY_OBJ roCUContainer(&m_roHKCUCache, g_szSubKey[dwWhich], CREATE_KEY_IF_NOT_EXISTS);
        if ((roCUContainer.GetStatus()==ERROR_SUCCESS)
            &&
            (roCUContainer.GetValue(PER_USER_KEY, &dwTemp)==ERROR_SUCCESS))
        {
            return dwTemp;
        }
        
        REGISTRY_OBJ roLMContainer(&m_roHKLMCache, g_szSubKey[dwWhich], CREATE_KEY_IF_NOT_EXISTS);
        BOOL fPerUser = FALSE;

        if ((roLMContainer.GetStatus()==ERROR_SUCCESS)
            &&
            (roLMContainer.GetValue(PER_USER_KEY, &dwTemp)==ERROR_SUCCESS))
        {
            return dwTemp;
        }  

        // On NT, the default will be a per-user container.
#ifndef UNIX
        dwTemp = (GlobalPlatformType == PLATFORM_TYPE_WINNT) ? TRUE : (dwWhich!=CONTENT);
#else
        dwTemp = (GlobalPlatformType == PLATFORM_TYPE_UNIX) ? TRUE : (dwWhich!=CONTENT);
#endif /* UNIX */

        roLMContainer.SetValue(PER_USER_KEY, &dwTemp);
        roCUContainer.SetValue(PER_USER_KEY, &dwTemp);
        return (BOOL)dwTemp;
    }


    // -- ValidatePath ------
    // We always assume we've been given a valid path, but we have to test that it's there
    // and available.
    BOOL ValidatePath(PSTR pszPath)
    {
        DWORD dwAttribute = GetFileAttributes(pszPath);
        if (dwAttribute==0xFFFFFFFF)
        {
            // We assume that the directory just isn't there. So we create it.
            hConstructSubDirs(pszPath);
            dwAttribute = GetFileAttributes(pszPath);
        }
        if ((dwAttribute==0xFFFFFFFF)
            ||
            (dwAttribute & FILE_ATTRIBUTE_READONLY)
            ||
            (!(dwAttribute & FILE_ATTRIBUTE_DIRECTORY)))
        {
            // BUG BUG BUG We probably want to make sure that the old path gets deleted on other machines....
            // We'll use the system path
            // BUG BUG BUG BUG BUG We are *NOT* recording this default location in the registry. Thus, on another
            // machine, the user might still be able to use the set cache location.
            memcpy(pszPath, m_szSharedPath, cbS);
            LoadString(GlobalDllHandle, g_dwCachePathResourceID[m_dwWorking], pszPath+cbS, MAX_PATH - cbS);
            SetPerUserStatus(FALSE);
        }
        return ERROR_SUCCESS;
    }
    
public:
    IE5_REGISTRYSET()
    {
        m_fInitialised = FALSE;
    }

    virtual DWORD InitialiseKeys(BOOL& fProfilesCapable)
    {
        if (m_fInitialised)
        {
            fProfilesCapable = m_fProfiles;
            return ERROR_SUCCESS;
        }

        DWORD dwError = ERROR_SUCCESS;

        fProfilesCapable = TRUE;
#ifndef UNIX
        cbS = GetWindowsDirectory(m_szSharedPath, sizeof(m_szSharedPath));
#else
        /* On Unix, GetWindowsDirectory points to <install dir>/common
         * And, we don't want to put the cache here.
         */
        lstrcpy(m_szSharedPath, UNIX_SHARED_CACHE_PATH);
        cbS = lstrlen(m_szSharedPath);
#endif /* UNIX */
        if (!cbS || (cbS>sizeof(m_szSharedPath)))
            return ERROR_PATH_NOT_FOUND;
            
        AppendSlashIfNecessary(m_szSharedPath, cbS);

        cbP = 0;
        // We think that profiles are enabled, so we want to get some info before
        // proceeding. If any of this fails, though, we'll default to no profiles.
        switch (GlobalPlatformType)
        {
#ifndef UNIX
        case PLATFORM_TYPE_WIN95:
        {
            REGISTRY_OBJ roProfilesEnabled(HKEY_LOCAL_MACHINE, PROFILES_ENABLED_VALUE);
            DWORD dwProfilesEnabled = 0;
            if (  (roProfilesEnabled.GetStatus() == ERROR_SUCCESS)
                && 
                  ((roProfilesEnabled.GetValue(PROFILES_ENABLED, &dwProfilesEnabled))==ERROR_SUCCESS)
                &&
                  dwProfilesEnabled)
            {                        
                  // Windows 95 sets the profiles path in the registry.
                CHAR szProfilesRegValue[MAX_PATH];
                memcpy(szProfilesRegValue, PROFILES_PATH_VALUE, sizeof(PROFILES_PATH_VALUE)-1);
                cbP = sizeof(PROFILES_PATH_VALUE)-1;
                AppendSlashIfNecessary(szProfilesRegValue, cbP);
                cbP = MAX_PATH-sizeof(PROFILES_PATH_VALUE);
                if (GetUserName(szProfilesRegValue + sizeof(PROFILES_PATH_VALUE), &cbP))
                {
                    cbP = MAX_PATH;
                    REGISTRY_OBJ roProfilesDirKey(HKEY_LOCAL_MACHINE, szProfilesRegValue);

                    if (!(((dwError = roProfilesDirKey.GetStatus()) != ERROR_SUCCESS)
                        || 
                        ((dwError = roProfilesDirKey.GetValue(PROFILES_PATH, (LPBYTE) m_szProfilePath, 
                                &cbP)) != ERROR_SUCCESS)))
                    {
                        m_szProfilePath[cbP-1] = DIR_SEPARATOR_CHAR;
                        m_szProfilePath[cbP] = '\0';
                        break;
                    }
                }
            }
            // Either
            // (a) Couldn't get the profiles path from the registry.
            // (b) Couldn't get the user name! 
            // Make the directory the windows directory
            fProfilesCapable = FALSE;
            break;
        }
            
        case PLATFORM_TYPE_WINNT:
            // Windows NT sets the USERPROFILE environment
            // string which contains the user's profile path
            if (cbP = GetEnvironmentVariable("USERPROFILE", m_szProfilePath, MAX_PATH))
            {
                m_szProfilePath[cbP++] = DIR_SEPARATOR_CHAR;
                m_szProfilePath[cbP] = '\0';
            }
            else
            {
                INET_ASSERT(FALSE);
                // Getting the user profiles dir from the environment
                // failed. Set the profiles directory to default.
                memcpy(m_szProfilePath, m_szSharedPath, cbS);
                memcpy(m_szProfilePath + cbS, DEFAULT_PROFILES_DIRECTORY, sizeof(DEFAULT_PROFILES_DIRECTORY));
                cbP = cbS + sizeof(DEFAULT_PROFILES_DIRECTORY) - 1;

                DWORD cbUser = MAX_PATH - cbP;;
                GetUserName(m_szProfilePath + cbP, &cbUser);
                cbP += cbUser;
            }
            break;

#else /* UNIX */
        case PLATFORM_TYPE_UNIX:
            lstrcpy(m_szProfilePath,TEXT("%USERPROFILE%"));
            lstrcat(m_szProfilePath,DIR_SEPARATOR_STRING);
            cbP = lstrlen(m_szProfilePath);
            break;
#endif /* UNIX */

        default:
            // This should never happen.
            INET_ASSERT(FALSE);
        }

        if (dwError==ERROR_SUCCESS)
        {
            dwError = InitCommonKeys(fProfilesCapable, CACHE5_KEY);
        }
        return dwError;
    }

    DWORD SetWorkingContainer(DWORD dwWhich)
    {
        m_dwWorking = dwWhich;
        m_fWorkingPerUser = DetermineKeyPlacing(dwWhich);
        return m_roWorking.WorkWith((m_fWorkingPerUser ? &m_roHKCUCache : &m_roHKLMCache), g_szSubKey[dwWhich], CREATE_KEY_IF_NOT_EXISTS);
    }

    VOID AttemptToUseSharedCache(PTSTR pszPath, DWORD ckbLimit);

    // Path --------------------------------------------------------------------------
    virtual DWORD GetPath(PTSTR pszPath)
    {
        if ((S_OK==SHGetFolderPath(NULL, g_iContainerCSIDL[m_dwWorking] | CSIDL_FLAG_CREATE, NULL, 0, pszPath))
            && (*pszPath!='\0'))
        {
            DWORD dwErr = ValidatePath(pszPath);
            if (dwErr==ERROR_SUCCESS)
            {
                DWORD ccPath = lstrlen(pszPath);
                // We check the lengths of the strings only when we're moving the containers. No need to do the check every
                // time (assume a valid path)
                if (m_dwWorking!=COOKIE)
                {
                    EnableCacheVu(pszPath, m_dwWorking);
                }
                AppendSlashIfNecessary(pszPath, ccPath);

#ifdef UNIX
               /* On Unix, it is possible that IE4 and IE5 co-exist on a user's
                * installation. So, we need to keep the IE4 cookies which are
                * different from the IE5 cookies. For IE5, we have the following
                * configuration for the caches -
                *
                * cookies - %HOME%/.microsoft/ie5/Cookies
                * content - %HOME%/.microsoft/ie5/TempInternetFiles/Content.IE5
                * history - %HOME%/.microsoft/ie5/History/History.IE5
                */
               CHAR szIE5Dir[] = "ie5/";
               int  index = ccPath-2; // skip the last slash
               int  lenIE5Dir = lstrlen(szIE5Dir);

               while(index >= 0 && pszPath[index] != FILENAME_SEPARATOR) 
                    index--;

               index++;
               memmove(&pszPath[index+lenIE5Dir],&pszPath[index],ccPath-index+2);
               memcpy(&pszPath[index],szIE5Dir,lenIE5Dir);
               ccPath += lenIE5Dir;
#endif /* UNIX */

                memcpy(pszPath+ccPath, g_szVersionName[m_dwWorking], lstrlen(g_szVersionName[m_dwWorking])+1);
            }
            return dwErr;
        }
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    virtual DWORD SetPath(PTSTR pszPath)
    {
        INET_ASSERT(m_roWorking.GetStatus()==ERROR_SUCCESS);
        DWORD dwError;

        /* Try to preserve the environment variables on Unix */
        UNIX_NORMALIZE_PATH_ALWAYS(pszPath, TEXT("%USERPROFILE%"));
        if (m_fProfiles)
        {
            CHAR szScratch[MAX_PATH];
            if (!NormalisePath(pszPath, TEXT("%USERPROFILE%"), szScratch, sizeof(szScratch)))
            {
                if (!NormalisePath(pszPath, TEXT("%SystemRoot%"), szScratch, sizeof(szScratch)))
                {
                    strncpy(szScratch, pszPath, MAX_PATH);
                }
            }
            if ((dwError = m_roUserShellFolder.SetValue(g_szOldSubKey[m_dwWorking], szScratch, REG_EXPAND_SZ))==ERROR_SUCCESS)
            {
#ifndef UNIX
               DWORD dwType = REG_SZ;
                dwError = m_roShellFolder.SetValue(g_szOldSubKey[m_dwWorking],pszPath, dwType);
#else
                dwError = m_roShellFolder.SetValue(g_szOldSubKey[m_dwWorking],szScratch, REG_EXPAND_SZ);
#endif /* UNIX */
            }
            // Possible BUG: If we move from the profiled path to the shared path, we still record it as a peruseritem.
            SetPerUserStatus(TRUE);            
        }
        // Non-profiles-enabled machine
        // On Win9x machines, with profiles disabled, we need to write the path to the 
        // HKEY_USERS/.default/blah blah/Explorer/User Shell Folders to ensure that SHGetFolderPath returns
        // the proper value for other users. 
        else
        {
            REGISTRY_OBJ roProfilesLessPath(HKEY_USERS, PROFILELESS_USF_KEY);
            dwError = roProfilesLessPath.GetStatus();
            if (dwError==ERROR_SUCCESS)
            {
                if ((dwError = roProfilesLessPath.SetValue(g_szOldSubKey[m_dwWorking],pszPath, REG_EXPAND_SZ))==ERROR_SUCCESS)
                {
#ifndef UNIX
                    DWORD dwType = REG_SZ;
#else
                    DWORD dwType = REG_EXPAND_SZ;
#endif /* UNIX */
                    dwError = m_roUserShellFolder.SetValue(g_szOldSubKey[m_dwWorking],pszPath, dwType);
                }
            }
            // For IE4 compatibility, we might have to adjust the old cache location here, as well.
        }
        return dwError;
    }


    // Prefix ------------------------------------------------------------------------
    virtual DWORD GetPrefix(LPSTR szPrefix)
    {
        DWORD dwError, cbKeyLen = MAX_PATH;
        if ((dwError = m_roWorking.GetValue(CACHE_PREFIX_VALUE, (LPBYTE) szPrefix, &cbKeyLen))==ERROR_SUCCESS)
        {
            if (cbKeyLen > 0)
            {
                // Strip trailing whitespace.
                cbKeyLen--;
                StripTrailingWhiteSpace(szPrefix, &cbKeyLen);
            }
        }
        else
        {
            // If no prefix found in registry create via
            // defaults and write back to registry.
            strncpy(szPrefix, g_szCachePrefix[m_dwWorking], MAX_PATH);
            SetPrefix(szPrefix);
            dwError = ERROR_SUCCESS;
        }
        
        return dwError;
    }

    virtual DWORD SetPrefix(PTSTR pszPrefix)
    {
        INET_ASSERT(m_roWorking.GetStatus()==ERROR_SUCCESS);
        return m_roWorking.SetValue(CACHE_PREFIX_VALUE, (pszPrefix) ? pszPrefix : g_szCachePrefix[m_dwWorking], REG_SZ);
    }

    // Limit -------------------------------------------------------------------------
    virtual DWORD GetLimit(PTSTR pszCachePath, DWORD& cbLimit)
    {
        if ((m_roWorking.GetValue(CACHE_LIMIT_VALUE, &cbLimit)!=ERROR_SUCCESS) || (cbLimit < 512))
        {
            cbLimit = 0;
            return SetLimit(pszCachePath, cbLimit);
        }
        return ERROR_SUCCESS;
    }

    virtual DWORD SetLimit(PTSTR pszCachePath, DWORD& cbLimit);

    // Use IsFirstTime* to figure out if this is the first time for this install of wininet and for marking it so ------------
private:
    BOOL IsFirstTimeFor(HKEY hKey)
    {
        DWORD cb = MAX_PATH;
        CHAR szSigKey[MAX_PATH];
        REGISTRY_OBJ roSig(hKey, CACHE5_KEY);

        return roSig.GetValue(CACHE_SIGNATURE_VALUE, (LPBYTE) szSigKey, &cb)==ERROR_SUCCESS ? 
                        strcmp(szSigKey, CACHE_SIGNATURE) : TRUE;
    }

public:
    BOOL IsFirstTimeForUser()
    {
        return IsFirstTimeFor((m_fProfiles ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE));
    }

    BOOL IsFirstTimeForMachine()
    {
        return IsFirstTimeFor(HKEY_LOCAL_MACHINE);
    }

    VOID SetIfFirstTime()
    {
        DWORD cb = MAX_PATH;
        CHAR szSigKey[MAX_PATH];

        // On a profiles-not-enabled machine, store the signature in HKLM so we don't have to research for values
        // On a profiles-enabled machine, store there to notify IE of previous installation of IE5.
        REGISTRY_OBJ roSig(HKEY_LOCAL_MACHINE, CACHE5_KEY);
        roSig.SetValue(CACHE_SIGNATURE_VALUE, CACHE_SIGNATURE, REG_SZ);

        // On profiles-enabled machines,  we store a signature in HKCU so that we don't have to do 
        // much hunting for registry values
        if (m_fProfiles)
        {
            REGISTRY_OBJ roSig(HKEY_CURRENT_USER, CACHE5_KEY);
            roSig.SetValue(CACHE_SIGNATURE_VALUE, CACHE_SIGNATURE, REG_SZ);
        }        
    }

    // PerUserItem ---------------------------------------------------------------------
    virtual VOID SetPerUserStatus(BOOL fState)
    {
        DWORD flState = fState;
        if (m_fProfiles && fState!=m_fWorkingPerUser)
        {
            REGISTRY_OBJ roTemp(&m_roHKCUCache, g_szSubKey[m_dwWorking], CREATE_KEY_IF_NOT_EXISTS);            
            if (roTemp.GetStatus()==ERROR_SUCCESS)
            {
                roTemp.SetValue(PER_USER_KEY, &flState);
                m_roWorking.WorkWith((fState ? &m_roHKCUCache
                                             : &m_roHKLMCache), g_szSubKey[m_dwWorking], CREATE_KEY_IF_NOT_EXISTS);
                m_fWorkingPerUser = fState;
            }
        }
    }

    virtual DWORD GetPerUserStatus()
    {
        return m_fWorkingPerUser;
    }

    DWORD UpdateContentPath(PSTR pszNewPath)
    {
        TCHAR szOldPath[MAX_PATH];
        DWORD dwError;

        dwError = ERROR_SUCCESS;

        if ((dwError=SetWorkingContainer(CONTENT))==ERROR_SUCCESS)
        {
            INTERNET_CACHE_CONFIG_INFOA icci;
            icci.dwContainer = CONTENT;
            GetUrlCacheConfigInfoA(&icci, NULL, CACHE_CONFIG_DISK_CACHE_PATHS_FC);
            strncpy(szOldPath, icci.CachePath, ARRAY_ELEMENTS(szOldPath));

            if (((dwError=MoveCachedFiles(szOldPath, pszNewPath))==ERROR_SUCCESS)
                &&
                ((dwError=SetPath(pszNewPath))==ERROR_SUCCESS))
            {
                EnableCacheVu(pszNewPath);

            // Right now, we're adding entries so that once we restart, we'll delete any
            // stray files.

            // BUT, there's a case that Move will be interrupted; in that case, we ought
            // to finish the move on start up -- pop up a dialog notifying user of such
            // and then delete.
            // Also, if the Move's interrupted, then this info never will get written. OTOH,
            // we can argue that the user can just move from the old location to the new.
    
                CHAR szRunOnce          [2 * MAX_PATH];
                CHAR szSystemPath       [MAX_PATH];

            // Add a RunOnce entry to be run on reboot.
                REGISTRY_OBJ roRunOnce((m_fWorkingPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE), RUN_ONCE_KEY, CREATE_KEY_IF_NOT_EXISTS);
                if ((dwError=roRunOnce.GetStatus())!=ERROR_SUCCESS)
                    return dwError;

            // create RunOnce string in the form:
            // "rundll32.exe <system dir>\wininet.dll,RunOnceUrlCache C:\Windows\NewCacheLocation"
                if (!GetSystemDirectory(szSystemPath, MAX_PATH))
                    return ERROR_INTERNAL_ERROR;
       
                DisableCacheVu(szOldPath);
                // Get rid of content.ie5.
                PathRemoveBackslash(szOldPath);
                PathRemoveFileSpec(szOldPath);
                DisableCacheVu(szOldPath);
                GetShortPathName(szOldPath, szOldPath, ARRAY_ELEMENTS(szOldPath));
                wnsprintf(szRunOnce, sizeof(szRunOnce),
                            "rundll32.exe %s\\wininet.dll,RunOnceUrlCache %s", 
                            szSystemPath, szOldPath);
                
            // Set the RunOnce command in registry for wininet.
                roRunOnce.SetValue(TEXT("MovingCacheA Wininet Settings"), (LPSTR)szRunOnce, REG_SZ);
            }
        }
        return dwError;
    }
};

#define m_roPaths           m_roShellFolder
#define m_roSpecialPaths    m_roHKCUCache

class IE3_REGISTRYSET : public IE5_REGISTRYSET
{
    // Registry keys shipped with IE 3:
    //
    // Software\Microsoft\Windows\CurrentVersion\Internet Settings\Cache\Paths
    //
    // Software\Microsoft\Windows\CurrentVersion\Internet Settings\Cache\Paths\path1
    //                                                                        \path2
    //                                                                        \path3
    //                                                                        \path4
    //
    // Software\Microsoft\Windows\CurrentVersion\Internet Settings\Cache\Special Paths
    //
    // Software\Microsoft\Windows\CurrentVersion\Internet Settings\Cache\Special Paths\Cookies
    //                                                                                \History
    //
    // Software\Microsoft\Windows\CurrentVersion\Internet Settings\Cache\Url History

private:
    REGISTRY_OBJ m_roPath[DEF_NUM_PATHS]; 
    
public:
    // Initialise the IE3 keys that we might work with.
    
    DWORD InitialiseKeys()
    {
        DWORD dwError, i;
        TCHAR szScratch[MAX_PATH];
        TCHAR pszBase[MAX_PATH];
        DWORD dwBaseLen;

        if (m_fInitialised)
        {
            return ERROR_SUCCESS;
        }
        m_roHKLMCache.WorkWith(HKEY_LOCAL_MACHINE, OLD_CACHE_KEY, CREATE_KEY_IF_NOT_EXISTS);
        if ((dwError=m_roHKLMCache.GetStatus())!=ERROR_SUCCESS)
                goto exit;

        m_roPaths.WorkWith(&m_roHKLMCache, CACHE_PATHS_KEY, CREATE_KEY_IF_NOT_EXISTS);
        if ((dwError=m_roPaths.GetStatus())!=ERROR_SUCCESS)
            goto exit;

        memcpy(pszBase, OLD_CACHE_PATH, sizeof(OLD_CACHE_PATH));
        dwBaseLen = sizeof(OLD_CACHE_PATH) - 1;
        for (i = 0; i < DEF_NUM_PATHS; i++)
        {
            pszBase[dwBaseLen-1] = (TCHAR)('1' + i);
            m_roPath[i].WorkWith(&m_roPaths, pszBase, CREATE_KEY_IF_NOT_EXISTS);
            if ((dwError=m_roPath[i].GetStatus())!=ERROR_SUCCESS)
                goto exit;
        }

        m_roSpecialPaths.WorkWith(&m_roHKLMCache, CACHE_SPECIAL_PATHS_KEY);
        m_fInitialised = TRUE;
    exit:
        return dwError;
    }

    BOOL GetContentDetails(LPSTR szPath, DWORD& cbLimit)
    {
        DWORD cbKey = MAX_PATH;
        if (m_roPaths.GetValue(CACHE_DIRECTORY_VALUE, (LPBYTE)szPath, &cbKey)!=ERROR_SUCCESS)
            return FALSE;

        cbLimit = 0;
        for (int i=0; i<DEF_NUM_PATHS; i++)
        {
            if (m_roPath[i].GetValue(CACHE_LIMIT_VALUE, &cbKey)!=ERROR_SUCCESS)
            {
                cbLimit = GetDefaultCacheQuota(szPath, NEW_CONTENT_QUOTA_DEFAULT_DISK_FRACTION);
                break;
            }
            cbLimit += cbKey;
        }
        return TRUE;
    }

    DWORD SetPath(PTSTR pszPath)
    {
        DWORD i, nPaths, dwError;
        DWORD cb = strlen((LPSTR)pszPath);
        TCHAR szBase[MAX_PATH];
#ifndef UNIX
        DWORD dwType = REG_SZ;
#else
        DWORD dwType = REG_EXPAND_SZ;
#endif /* UNIX */

        /* On Unix, try to preserve the Environment variables if possible */
        UNIX_NORMALIZE_PATH_ALWAYS(pszPath, TEXT("%USERPROFILE%"));


        // Cache content path.
        if ((dwError = m_roPaths.SetValue(CACHE_DIRECTORY_VALUE, (LPSTR)pszPath, dwType)) != ERROR_SUCCESS)
            goto exit;

        // Number of subdirectories (optional).
        nPaths = DEF_NUM_PATHS;
        if ((dwError = m_roPaths.SetValue(CACHE_PATHS_KEY, &nPaths)) != ERROR_SUCCESS)
            goto exit;
    
        memcpy(szBase, pszPath, cb);
        memcpy(szBase + cb, OLD_CACHE_SUBKEY, sizeof(OLD_CACHE_SUBKEY));
        cb += sizeof(OLD_CACHE_SUBKEY) - 2;
        // Subdirectories' paths and limits from CONTENT.
        for (i = 0; i < DEF_NUM_PATHS; i++)
        {
            szBase[cb] = (TCHAR)('1' + i);
            if ((dwError = m_roPath[i].SetValue(CACHE_PATH_VALUE, szBase, REG_SZ)) != ERROR_SUCCESS)
                goto exit;    
        }

    exit:    
        INET_ASSERT(dwError == ERROR_SUCCESS);
        return dwError;
    }

    DWORD SetLimit(DWORD dwLimit)
    {
        DWORD i, nPaths, dwError;

        for (i = 0; i < DEF_NUM_PATHS; i++)
        {
            DWORD cbCacheLimitPerSubCache = (DWORD) (dwLimit/ DEF_NUM_PATHS);
            if ((dwError = m_roPath[i].SetValue(CACHE_LIMIT_VALUE, &cbCacheLimitPerSubCache)) != ERROR_SUCCESS)
                goto exit;
        }
    exit:    
        INET_ASSERT(dwError == ERROR_SUCCESS);
        return dwError;
    }

    // Restore key IE3 values. *snicker* --------------------------------------------
    VOID FixLegacySettings(PTSTR pszPath, DWORD cbLimit)
    {
        if (InitialiseKeys()==ERROR_SUCCESS)
        {
            SetPath(pszPath);
            SetLimit(cbLimit);
        }
    }
};

class IE4_REGISTRYSET : public IE5_REGISTRYSET
{
private:
    BOOL DetermineKeyPlacing(DWORD dwWhich)
    {
        DWORD dwValue;
        if (m_fProfiles && (dwWhich==CONTENT))
        {
            if (m_roHKLMCache.GetValue(PROFILES_ENABLED, &dwValue)==ERROR_SUCCESS)
            {
                return dwValue;
            }

#ifndef UNIX
            if (GlobalPlatformType == PLATFORM_TYPE_WINNT)
#else
            if (GlobalPlatformType == PLATFORM_TYPE_UNIX)
#endif /* !UNIX */
            {
               return m_fProfiles;
            }

            // On Win9x we have to go through the following contortions to decide whether or not the
            // user is using a per-user cache or a shared cache.
         
            TCHAR szPath[MAX_PATH];
            DWORD cbPath = sizeof(szPath);
            if (m_roShellFolder.GetValue(g_szOldSubKey[m_dwWorking],(LPBYTE)szPath, &cbPath)==ERROR_SUCCESS)
            {
                cbPath = sizeof(szPath);
                return (m_roUserShellFolder.GetValue(g_szOldSubKey[m_dwWorking],(LPBYTE)szPath, &cbPath)==ERROR_SUCCESS);
            }
        }
        return m_fProfiles;
    }

public:
    DWORD InitialiseKeys(BOOL& fProfiles)
    {
        if (m_fInitialised)
        {
            return ERROR_SUCCESS;
        }
        return InitCommonKeys(fProfiles, OLD_CACHE_KEY);
    }

    DWORD GetPath(PTSTR pszCachePath)
    {
        DWORD cbKeyLen = MAX_PATH;
        LOG_UPGRADE_DATA("Getting IE4 cache location...\n");

        DWORD dwError = m_fProfiles ? m_roShellFolder.GetValue(g_szOldSubKey[m_dwWorking],(LPBYTE)pszCachePath, &cbKeyLen) 
                                          : m_roWorking.GetValue(CACHE_PATH_VALUE, (LPBYTE)pszCachePath, &cbKeyLen);
#ifndef UNIX
        if (m_fProfiles && (GlobalPlatformType == PLATFORM_TYPE_WINNT) && (dwError==ERROR_SUCCESS))
#else
        if (m_fProfiles && (GlobalPlatformType == PLATFORM_TYPE_UNIX) && (dwError==ERROR_SUCCESS))
#endif /* UNIX */
        {
           LOG_UPGRADE_DATA("Correcting IE4 cache location...\n");
           LOG_UPGRADE_DATA(pszCachePath);
           LOG_UPGRADE_DATA("\n");
           TCHAR szPath[MAX_PATH];
           DWORD cbPath = ARRAY_ELEMENTS(szPath);
            if (m_roUserShellFolder.GetValue(g_szOldSubKey[m_dwWorking],(LPBYTE)szPath, &cbPath)!=ERROR_SUCCESS)
            {
                if (!NormalisePath(pszCachePath, TEXT("%USERPROFILE%"), szPath, sizeof(szPath)))
                {
                    if (!NormalisePath(pszCachePath, TEXT("%SystemRoot%"), szPath, sizeof(szPath)))
                    {
                        strncpy(szPath, pszCachePath, MAX_PATH);
                    }
                }
                dwError = m_roUserShellFolder.SetValue(g_szOldSubKey[m_dwWorking], szPath, REG_EXPAND_SZ);
            }
        }
        return dwError;
    }

    BOOL WasIE4Present(BOOL& fProfilesCapable)
    {
        DWORD cb = MAX_PATH;
        CHAR szSigKey[MAX_PATH];
        REGISTRY_OBJ roSig((fProfilesCapable ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE), OLD_CACHE_KEY);

        return (roSig.GetValue(CACHE_SIGNATURE_VALUE, (LPBYTE) szSigKey, &cb)==ERROR_SUCCESS);
    }

    DWORD SetLimit(PTSTR pszCachePath, DWORD& cbLimit)
    {
        INET_ASSERT(m_roWorking.GetStatus()==ERROR_SUCCESS);
        // If no limit found in registry create via
        // defaults and write back to registry.
        // Cache limit - for the content cache we calculate the cache limit
        // as being max(DEF_CACHE_LIMIT, 1/32 of the disk size) All others caches
        // are set to DEF_CACHE_LIMIT.
        if (cbLimit==0)
        {
            cbLimit = (m_dwWorking==CONTENT) 
                            ? GetDefaultCacheQuota(pszCachePath, NEW_CONTENT_QUOTA_DEFAULT_DISK_FRACTION)
                            : DEF_CACHE_LIMIT;
        }

        // Dumb hack for back compat. *sigh*
        if (m_dwWorking==CONTENT)
        {
            REGISTRY_OBJ roLimit(&m_roHKLMCache, g_szSubKey[CONTENT]);
            if (roLimit.GetStatus()==ERROR_SUCCESS)
            {
                roLimit.SetValue(CACHE_LIMIT_VALUE, &cbLimit);
            }
        }
        return m_roWorking.SetValue(CACHE_LIMIT_VALUE, &cbLimit);
    }
};


DWORD IE5_REGISTRYSET::SetLimit(PTSTR pszCachePath, DWORD& cbLimit)
{
    INET_ASSERT(m_roWorking.GetStatus()==ERROR_SUCCESS);
    // If no limit found in registry create via
    // defaults and write back to registry.
    // Cache limit - for the content cache we calculate the cache limit
    // as being max(DEF_CACHE_LIMIT, 1/32 of the disk size) All others caches
    // are set to DEF_CACHE_LIMIT.
    if (cbLimit==0)
    {
        cbLimit = (m_dwWorking==CONTENT) 
                        ? GetDefaultCacheQuota(pszCachePath, NEW_CONTENT_QUOTA_DEFAULT_DISK_FRACTION)
                        : DEF_CACHE_LIMIT;
    }
    DWORD dwError = m_roWorking.SetValue(CACHE_LIMIT_VALUE, &cbLimit);
    if (dwError==ERROR_SUCCESS)
    {
        // Hack so that apps that read the cache quota from the registry are
        // still able to do so.
        IE4_REGISTRYSET ie4;
        dwError = ie4.InitialiseKeys(m_fProfiles);
        if (dwError==ERROR_SUCCESS)
        {
            ie4.SetWorkingContainer(m_dwWorking);
            ie4.SetLimit(pszCachePath, cbLimit);
        }
    }
    return dwError;
}


#define IsFieldSet(fc, bitFlag) (((fc) & (bitFlag)) != 0)

#define FAILSAFE_TIMEOUT (60000)
#define UNMAP_TIME (120000)

// ----------------------------------------------------------------------------
// The following functions deal with keeping the cache containers all up and ready

// -- ConfigureCache() --------------------------------------------------------
// Get the cache info from registry and try to init.

// In general, GetCacheConfigInfo should only rarely fail -- mostly whenever HKCU
// is expected but not available. In that case, we use the system root cache. 
// If _that_ fails, we panic.

DWORD CConMgr::ConfigureCache()
{
    for (DWORD iter = 0; ; iter++)
    {
        DWORD dwError;
        switch (iter)
        {
        case 0:
            dwError = GetCacheConfigInfo();
            break;

        case 1:
            dwError = GetSysRootCacheConfigInfo();
            break;

        default:
            INET_ASSERT(FALSE);
            return dwError;
        }

        if (dwError==ERROR_SUCCESS && (dwError=InitFixedContainers())==ERROR_SUCCESS)
            break;

        // If InitFixedContainers has failed, it is possible that the container list 
        // (ConList) is not empty. Make sure it has no entries.
        LOCK_CACHE();
        if (ConList.Size() != 0)
            ConList.Free();
        UNLOCK_CACHE();
    }

    return ERROR_SUCCESS;
}

VOID CheckCacheLocationConsistency();

/*-----------------------------------------------------------------------------
DWORD CConMgr::GetCacheConfigInfo
  ----------------------------------------------------------------------------*/
DWORD CConMgr::GetCacheConfigInfo()
{
    DWORD dwError, i;

    // Prepare and initialise a registry set for every version of IE available.
    // IE5 must be initialised first because it determines whether profiles are
    // enabled on this machine and set ConMgr's _fProfilesCapable for future
    // reference. Then IE4 and IE3 can be called in whatever order.
    IE5_REGISTRYSET ie5rs;
    if ((dwError=ie5rs.InitialiseKeys(_fProfilesCapable))!=ERROR_SUCCESS)
    {
        goto exit;
    }

    // Look for a signature to indicate that this cache has been placed before.
    if (ie5rs.IsFirstTimeForUser())
    {
        LOG_UPGRADE_DATA("Install 1st time for user\n");
        DiscoverRegistrySettings(&ie5rs);
    } else {
        CheckCacheLocationConsistency();
    }

    // -----------------------------------------------------------------------------------
    // Get the container paths, prefixes (if any) and default limit values.
    for (i = CONTENT; i < NCONTAINERS; i++)
    {
        CHAR szCachePath[MAX_PATH];
        CHAR szCachePrefix[MAX_PATH];
        DWORD cbCacheLimit;
        BOOL fPerUser;
        // This should only rarely fail.

        if ((dwError=ie5rs.SetWorkingContainer(i))!=ERROR_SUCCESS)
        {
            goto exit;
        }
        
        fPerUser = ie5rs.GetPerUserStatus();
        dwError = ie5rs.GetPath(szCachePath);
        LOG_UPGRADE_DATA("GetCacheConfigInfo/ie5rs.GetPath for user: ");
        LOG_UPGRADE_DATA(szCachePath);
        LOG_UPGRADE_DATA("\n");

        if (dwError==ERROR_SUCCESS) 
        {
            ie5rs.GetPrefix(szCachePrefix);
            ie5rs.GetLimit(szCachePath, cbCacheLimit);
        }
        else
        {
            break;
        }
                
        // Got info, now create the container --------------------------------------
        _coContainer[i] = new URL_CONTAINER(g_szSubKey[i], 
                                            szCachePath, 
                                            szCachePrefix, 
                                            (LONGLONG)cbCacheLimit*1024,
                                            0);
                                            
        if (_coContainer[i])
        {
            dwError = _coContainer[i]->GetStatus();
            if (dwError!=ERROR_SUCCESS)
            {
                delete _coContainer[i];
                break;
            }
            ConList.Add(_coContainer[i]);
            _coContainer[i]->SetPerUserItem(fPerUser);
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Maintain values for backwards compatibility
        if (i==CONTENT)
        {
            // If repairing IE3's settings fails, well, who cares? IE5 is still going.
            IE3_REGISTRYSET ie3rs;
            ie3rs.FixLegacySettings(szCachePath, cbCacheLimit);
        }
    }

exit:
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::GetSysRootCacheConfigInfo
----------------------------------------------------------------------------*/
DWORD CConMgr::GetSysRootCacheConfigInfo()
{
    CHAR szParentPath[MAX_PATH];
    DWORD cb = MAX_PATH;

#ifndef UNIX
    cb = GetWindowsDirectory(szParentPath, sizeof(szParentPath));
#else
    /* On Unix, GetWindowsDirectory will point to <install dir>/common
     * and the cache should not be created here in any case
     */
    lstrcpy(szParentPath,UNIX_SHARED_CACHE_PATH);
    cb = lstrlen(szParentPath);
#endif /* UNIX */
    if (!cb || (cb>sizeof(szParentPath)))
    {
        return ERROR_PATH_NOT_FOUND;
    }
    AppendSlashIfNecessary(szParentPath, cb);
    
    for (DWORD idx = CONTENT; idx < NCONTAINERS; idx++)
    {
        CHAR szCachePath[MAX_PATH];
        CHAR szCachePrefix[MAX_PATH];
        LONGLONG cbCacheLimit;

        // Get cache paths out of dll resource and form absolute
        // paths to top level cache directories.
        memcpy(szCachePath, szParentPath, cb);
        if (!LoadString(GlobalDllHandle, g_dwCachePathResourceID[idx], szCachePath + cb, MAX_PATH - cb))
        {
            return GetLastError();
        }

        DWORD ccPath = lstrlen(szCachePath);
        AppendSlashIfNecessary(szCachePath, ccPath);
        memcpy(szCachePath+ccPath, g_szVersionName[idx], lstrlen(g_szVersionName[idx])+1);

        // Cache prefix.        
        memcpy(szCachePrefix, g_szCachePrefix[idx], strlen(g_szCachePrefix[idx]) + 1);

        // Cache limit - for the content cache we calculate the cache limit
        // as being max(DEF_CACHE_LIMIT, 1/32 of the disk size) All others caches
        // are set to DEF_CACHE_LIMIT.
        if (idx == CONTENT)
        {
            
            REGISTRY_OBJ roCache(HKEY_LOCAL_MACHINE, CACHE5_KEY);
            BOOL fResult = (roCache.GetStatus()==ERROR_SUCCESS);
            if (fResult)
            {
                REGISTRY_OBJ roLimit(&roCache, CONTENT_PATH_KEY);
                fResult = FALSE;
                if (roLimit.GetStatus()==ERROR_SUCCESS)
                {
                    DWORD cKBLimit;
                    if (roLimit.GetValue(CACHE_LIMIT_VALUE, &cKBLimit)==ERROR_SUCCESS)
                    {
                        cbCacheLimit = cKBLimit * (LONGLONG)1024;
                        fResult = TRUE;
                    }
                }
            }
            if (!fResult)
            {
                cbCacheLimit = 
                        (DWORDLONG)GetDefaultCacheQuota(szCachePath, NEW_CONTENT_QUOTA_DEFAULT_DISK_FRACTION) 
                        * (DWORDLONG)1024;
            }
        }
        else
        {
            // Non-CONTENT cache; use default.
            cbCacheLimit = DEF_CACHE_LIMIT * (LONGLONG)1024;
        }

        _coContainer[idx] = new URL_CONTAINER(g_szSubKey[idx], szCachePath, szCachePrefix, cbCacheLimit, 0);

        if (_coContainer[idx])
        {
            DWORD dwError = _coContainer[idx]->GetStatus();
            if (dwError!=ERROR_SUCCESS)
            {
                delete _coContainer[idx];
                return dwError;
            }
            ConList.Add(_coContainer[idx]);
            _coContainer[idx]->SetPerUserItem(FALSE);
        }
        else
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }    

    _fUsingBackupContainers = TRUE;
    return ERROR_SUCCESS;
}


/*-----------------------------------------------------------------------------
BOOL CConMgr::GetUrlCacheConfigInfo
  ----------------------------------------------------------------------------*/
BOOL CConMgr::GetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO lpCacheConfigInfo,
    LPDWORD lpdwCacheConfigInfoBufferSize, DWORD dwFieldControl)
{
    LOCK_CACHE();

    BOOL fIE5Struct = (lpCacheConfigInfo->dwStructSize == sizeof(INTERNET_CACHE_CONFIG_INFO));
    
    if(IsFieldSet( dwFieldControl, CACHE_CONFIG_SYNC_MODE_FC))
    {
        lpCacheConfigInfo->dwSyncMode = GlobalUrlCacheSyncMode;
    }
    
    if (IsFieldSet(dwFieldControl, CACHE_CONFIG_QUOTA_FC))
    {
        lpCacheConfigInfo->dwQuota = (DWORD) (_coContainer[lpCacheConfigInfo->dwContainer]->GetCacheLimit()/1024L);
    }

    if (fIE5Struct && IsFieldSet(dwFieldControl, CACHE_CONFIG_CONTENT_USAGE_FC))
    {
        lpCacheConfigInfo->dwNormalUsage = (DWORD) (_coContainer[lpCacheConfigInfo->dwContainer]->GetCacheSize()/1024L);
    }
    
    if (fIE5Struct && IsFieldSet(dwFieldControl, CACHE_CONFIG_STICKY_CONTENT_USAGE_FC) && (lpCacheConfigInfo->dwContainer==CONTENT))
    {
        lpCacheConfigInfo->dwExemptUsage = (DWORD) (_coContainer[CONTENT]->GetExemptUsage()/1024L);
    }
    
    lpCacheConfigInfo->fPerUser = IsFieldSet( dwFieldControl, CACHE_CONFIG_USER_MODE_FC)
                                    ? _coContainer[lpCacheConfigInfo->dwContainer]->IsPerUserItem()
                                    : _coContent->IsPerUserItem();

    if (IsFieldSet(dwFieldControl, CACHE_CONFIG_CONTENT_PATHS_FC))
    {
        lpCacheConfigInfo->dwContainer = CONTENT;
    }
    else if (IsFieldSet(dwFieldControl, CACHE_CONFIG_HISTORY_PATHS_FC))
    {
        lpCacheConfigInfo->dwContainer = HISTORY;
    }
    else if (IsFieldSet(dwFieldControl, CACHE_CONFIG_COOKIES_PATHS_FC))
    {
        lpCacheConfigInfo->dwContainer = COOKIE;
    }
    // These are the actual field codes that should be sent for cache paths.
    // Note that the path returned *does not* contain subdirs (cache1..N).
    if ((lpCacheConfigInfo->dwContainer <= HISTORY) && (lpCacheConfigInfo->dwContainer >= CONTENT))
    {
        memcpy(lpCacheConfigInfo->CachePath, 
                _coContainer[lpCacheConfigInfo->dwContainer]->GetCachePath(), 
                _coContainer[lpCacheConfigInfo->dwContainer]->GetCachePathLen() + 1);
        lpCacheConfigInfo->dwQuota = (DWORD)
               (_coContainer[lpCacheConfigInfo->dwContainer]->GetCacheLimit() / 1024);        

        lpCacheConfigInfo->dwNumCachePaths = (DWORD) 1;    
    }

    UNLOCK_CACHE();
    return TRUE;
}


/*-----------------------------------------------------------------------------
BOOL CConMgr::SetUrlCacheConfigInfo
  ----------------------------------------------------------------------------*/
BOOL CConMgr::SetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO pConfig, 
                                              DWORD dwFieldControl)
{         
    DWORD i, dwError = ERROR_SUCCESS;

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);
    
    LOCK_CACHE();
    
    //  Check FieldControl bits and set the values for set fields
    if( IsFieldSet( dwFieldControl, CACHE_CONFIG_SYNC_MODE_FC ))
    {

        INET_ASSERT((pConfig->dwSyncMode >= WININET_SYNC_MODE_NEVER)
                        &&(pConfig->dwSyncMode <= WININET_SYNC_MODE_AUTOMATIC));

        InternetWriteRegistryDword(vszSyncMode, pConfig->dwSyncMode);

        GlobalUrlCacheSyncMode = pConfig->dwSyncMode;

        // set a new version and simultaneously
        // increment copy for this process, so we don't
        // read registry for this process
        IncrementHeaderData(CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION, 
                            &GlobalSettingsVersion);
    }

    if ( IsFieldSet( dwFieldControl, CACHE_CONFIG_DISK_CACHE_PATHS_FC ))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if ( IsFieldSet( dwFieldControl, CACHE_CONFIG_QUOTA_FC ) && pConfig->dwContainer==CONTENT)
    {
        DWORD cbSize = pConfig->dwQuota;
        INET_ASSERT(cbSize);

        if (!_fUsingBackupContainers)
        {
            IE5_REGISTRYSET ie5;
            IE3_REGISTRYSET ie3;
            
            if ((dwError=ie5.InitialiseKeys(_fProfilesCapable))!=ERROR_SUCCESS)
            {
                goto exit;
            }
            ie5.SetWorkingContainer(CONTENT);
            TCHAR szTemp[MAX_PATH];
            ie5.GetPath(szTemp);
            ie5.SetLimit(szTemp, cbSize);

            if (ie3.InitialiseKeys()==ERROR_SUCCESS)
            {
                ie3.FixLegacySettings(szTemp, cbSize);
            }
        }
        else
        {
            REGISTRY_OBJ roCache(HKEY_LOCAL_MACHINE, CACHE5_KEY, CREATE_KEY_IF_NOT_EXISTS);
            BOOL fResult = (roCache.GetStatus()==ERROR_SUCCESS);
            if (fResult)
            {
                REGISTRY_OBJ roLimit(&roCache, CONTENT_PATH_KEY, CREATE_KEY_IF_NOT_EXISTS);
                if (roLimit.GetStatus()==ERROR_SUCCESS)
                {
                    roLimit.SetValue(CACHE_LIMIT_VALUE, &cbSize);
                }
            }
        }

        if ((((LONGLONG)cbSize * 1024) < _coContent->GetCacheSize()))
            _coContent->CleanupUrls (DEFAULT_CLEANUP_FACTOR, 0);
        _coContent->SetCacheLimit(cbSize* (LONGLONG)1024);
    }

exit:
    UNLOCK_CACHE();
    BOOL fRet = (dwError==ERROR_SUCCESS);
    if (!fRet)
    {
        SetLastError(dwError);
        DEBUG_ERROR(INET, dwError);
    }
    return fRet;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::SetContentPath

UpdateUrlCacheContentPath leads to this function.
This initiates the cache move. Should be called just before shutdown.

----------------------------------------------------------------------------*/
BOOL CConMgr::SetContentPath(PTSTR pszNewPath)
{
    IE5_REGISTRYSET ie5rs;
    DWORD dwError;
    BOOL fLock;

    _coContent->LockContainer(&fLock);
    
    if  ((dwError=ie5rs.InitialiseKeys(_fProfilesCapable))==ERROR_SUCCESS)
    {
        dwError = ie5rs.UpdateContentPath(pszNewPath);
    }

    if (fLock)
    {
        _coContent->UnlockContainer();
    }

    if (dwError==ERROR_SUCCESS)
    {
        return TRUE;
    }
    SetLastError(dwError);
    return FALSE;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::GetExtensibleCacheConfigInfo
----------------------------------------------------------------------------*/
DWORD CConMgr::GetExtensibleCacheConfigInfo(BOOL fAlways)
{
    CHAR szCachePath[MAX_PATH];
    CHAR szCachePrefix[MAX_PATH];
    CHAR szPrefixMap[MAX_PATH];
    CHAR szVolumeLabel[MAX_PATH];
    CHAR szVolumeTitle[MAX_PATH];

    LONGLONG cbCacheLimit;

    HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
    DWORD cbKeyLen, cbKBLimit, dwError = ERROR_SUCCESS;
    CHAR szVendorKey[MAX_PATH];
    DWORD dwWaitResult = ERROR_TIMEOUT;

    URL_CONTAINER* pNewContainer;
    URL_CONTAINER* co;

    DWORD idx;
    DWORD idxPrefix;
    DWORD dwNow;
    DWORD dwOptions;
    BOOL fModified;
    BOOL fCDContainer;

    LOCK_CACHE();
    fModified = WasModified(TRUE);

    hKey = _fProfilesCapable ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    REGISTRY_OBJ roCache;

    //  WasModified MUST come first, so that we update our cached count!
    if (!fModified && !fAlways)
    {
         //  Unmap every container that hasn't be referenced in UNMAP_TIME
        dwNow = GetTickCount();
        if ((dwNow - _dwLastUnmap) > UNMAP_TIME)
        {
            for (idx = ConList.Size()-1; idx >= NCONTAINERS; idx--)
            {
                co = ConList.Get(idx);
                if (co)
                {
                    if (co->GetDeletePending() ||
                        ((dwNow - co->GetLastReference()) > UNMAP_TIME))
                    {
                        co->TryToUnmap(1);    //  RefCount should be 1 == Us
                    }
                    co->Release(FALSE);
                }
            }

            _dwLastUnmap = dwNow;
        }
        goto exit;
    }

    roCache.WorkWith(hKey, CACHE5_KEY);
    if ((dwError = roCache.GetStatus())== ERROR_SUCCESS)
    {
        // Create registry object and entry.
        REGISTRY_OBJ roExtensibleCache(&roCache, EXTENSIBLE_CACHE_PATH_KEY);
        if ((dwError = roExtensibleCache.GetStatus()) != ERROR_SUCCESS)
            goto exit;

        for (idx = NCONTAINERS; idx < ConList.Size(); idx++)
        {
            URL_CONTAINER *co = ConList.Get(idx);
            if (co)
            {
                co->Mark(FALSE);
                co->Release(FALSE);
            }
        }

        dwWaitResult = WaitForSingleObject(_hMutexExtensible, FAILSAFE_TIMEOUT);

        idx = NCONTAINERS;
        // Get the container paths, prefixes (if any) and default limit values.
        while (roExtensibleCache.FindNextKey(szVendorKey, MAX_PATH) == ERROR_SUCCESS)
        {
            REGISTRY_OBJ roVendor(&roExtensibleCache, szVendorKey);
            if (roVendor.GetStatus()==ERROR_SUCCESS)
            {
                    // Path.
                cbKeyLen = MAX_PATH;
                if (roVendor.GetValue(CACHE_PATH_VALUE, (LPBYTE) szCachePath, &cbKeyLen) != ERROR_SUCCESS)
                    continue;
                
                // Prefix.
                cbKeyLen = MAX_PATH;
                if (roVendor.GetValue(CACHE_PREFIX_VALUE, (LPBYTE) szCachePrefix, &cbKeyLen) != ERROR_SUCCESS)
                    continue;
                
                // Limit.
                if (roVendor.GetValue(CACHE_LIMIT_VALUE, &cbKBLimit) != ERROR_SUCCESS)
                    continue;    

                // Options.
                if (roVendor.GetValue(CACHE_OPTIONS_VALUE, &dwOptions) != ERROR_SUCCESS)
                    continue;
            
                if (dwOptions & INTERNET_CACHE_CONTAINER_MAP_ENABLED)
                {
                    fCDContainer = TRUE;

                    // PrefixMap
                    cbKeyLen = MAX_PATH;
                    if ((roVendor.GetValue(CACHE_PREFIX_MAP_VALUE, (LPBYTE) szPrefixMap, &cbKeyLen) != ERROR_SUCCESS)
                        || (*szPrefixMap == '\0'))
                        continue;

                    // Volume label.
                    cbKeyLen = MAX_PATH;
                    if ((roVendor.GetValue(CACHE_VOLUME_LABLE_VALUE, (LPBYTE) szVolumeLabel, &cbKeyLen) != ERROR_SUCCESS)
                        || (*szVolumeLabel == '\0'))
                        continue;
            
                    // Volume title.
                    cbKeyLen = MAX_PATH;
                    if ((roVendor.GetValue(CACHE_VOLUME_TITLE_VALUE, (LPBYTE) szVolumeTitle, &cbKeyLen) != ERROR_SUCCESS)
                        || (*szVolumeTitle == '\0'))
                        continue;
                }
                else
                {
                    fCDContainer = FALSE;
                    *szPrefixMap = '\0';
                    dwOptions &= ~INTERNET_CACHE_CONTAINER_PREFIXMAP;
                }
    
                cbCacheLimit = ((LONGLONG) cbKBLimit) * 1024;
                
                idxPrefix = FindExtensibleContainer(szVendorKey);
                if (idxPrefix != NOT_AN_INDEX)
                {
                    co = ConList.Get(idxPrefix);

                    if (co)
                    {
                        //  what if the container has been added
                        //  with the same name but a different path, prefix, or options!
                        if (stricmp(co->GetCachePath(), szCachePath) ||
                            stricmp(co->GetCachePrefix(), szCachePrefix) ||
                            co->GetOptions() != dwOptions)
                        {
                            idxPrefix = NOT_AN_INDEX;
                        }
                        else if (fCDContainer && stricmp(co->GetPrefixMap(), szPrefixMap))
                        {
                            idxPrefix = NOT_AN_INDEX;
                        }
                        else
                        {
                            co->Mark(TRUE);
                        }
                        co->Release(FALSE);
                    }
                }
            
                if (idxPrefix == NOT_AN_INDEX)
                {
                    // Construct either a normal container, or a CD container.
                    if (!fCDContainer)                        
                    {
                        pNewContainer = new URL_CONTAINER(szVendorKey, szCachePath, szCachePrefix,
                                                cbCacheLimit, dwOptions);
                    }
                    else
                    {
                        pNewContainer = new CInstCon(szVendorKey, szVolumeLabel, szVolumeTitle,
                                                 szCachePath, szCachePrefix, szPrefixMap, 
                                                 cbCacheLimit, dwOptions);
                    }
                    if (pNewContainer)
                    {
                        dwError = pNewContainer->GetStatus();
                        if (dwError!=ERROR_SUCCESS)
                        {
                            delete pNewContainer;
                            pNewContainer = NULL;
                        }
                        else
                        {
                            pNewContainer->Mark(TRUE);
                            ConList.Add(pNewContainer);
                        }
                    }
                    else
                    {
                        dwError = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                idx++;
            }
        }
        if (dwWaitResult == WAIT_OBJECT_0) 
        {
            ReleaseMutex(_hMutexExtensible);
            dwWaitResult = ERROR_TIMEOUT;
        }


        //  Mark every container that's no longer in the registry for pending delete
        //  Unmap every container that hasn't be referenced in UNMAP_TIME
        dwNow = GetTickCount();

        idx = ConList.Size() - 1;

        while (idx >= NCONTAINERS)
        {
            co = ConList.Get(idx);
            if (co)
            {
                if (!co->GetMarked() && !co->GetDeleted())
                {
                    co->SetDeletePending(TRUE);
                }
                if (co->GetDeletePending() ||
                    ((dwNow - co->GetLastReference()) > UNMAP_TIME))
                {
                    co->TryToUnmap(1);    //  RefCount should be 1 == Us, unless enumerator
                                          //  is still open
                }
                co->Release(FALSE);
            }
            idx--;
        }

        _dwLastUnmap = dwNow;
    }
    
exit:  
    if (dwWaitResult == WAIT_OBJECT_0) 
    {
        ReleaseMutex(_hMutexExtensible);
        dwWaitResult = ERROR_TIMEOUT;
    }

    UNLOCK_CACHE();
    return dwError;
}

//
//  Mixed environment of IE4 and IE5 sharing a server causes HKCU keys to get resaved as REG_SZ incorrectly
//  so we repair it here
VOID CheckCacheLocationConsistency()
{
    // Read user shell folders (necessary only in HKCU) and write back as REG_EXPAND_SZ if necessary
    REGISTRY_OBJ roUserShellFolders(HKEY_CURRENT_USER, USER_SHELL_FOLDER_KEY);
    if (roUserShellFolders.GetStatus()!=ERROR_SUCCESS)
    {
        return;
    }
    for (int i=0; i<NCONTAINERS; i++)
    {
        TCHAR szPath[MAX_PATH];
        DWORD cc = ARRAY_ELEMENTS(szPath);
        DWORD ValueSize;
        DWORD ValueType;

        //  speed things up a bit by checking if we don't need to do this
        if (roUserShellFolders.GetValueSizeAndType(g_szOldSubKey[i], &ValueSize, &ValueType ) != SUCCESS 
            || ValueType != REG_SZ)
        {
            continue;
        }
        if (roUserShellFolders.GetValue(g_szOldSubKey[i], (LPBYTE)szPath, &cc)!=ERROR_SUCCESS)
        {
            continue;
        }
        // First reconcile path to whatever it should be
        // and rename the containers accordingly.

        TCHAR szRealPath[MAX_PATH];
        
        // Expand string
        ExpandEnvironmentStrings(szPath, szRealPath, ARRAY_ELEMENTS(szRealPath));

        // Contract string
        if (!NormalisePath(szRealPath, TEXT("%USERPROFILE%"), szPath, sizeof(szPath)))
        {
            NormalisePath(szRealPath, TEXT("%SystemRoot%"), szPath, sizeof(szPath));
        }
        
        // Then write it back
        roUserShellFolders.DeleteValue(g_szOldSubKey[i]);
        roUserShellFolders.SetValue(g_szOldSubKey[i], szPath, REG_EXPAND_SZ);
    }
}

VOID MakeCacheLocationsConsistent()
{
    // Delete any 5.0 cache signatures from previous installs
    REGISTRY_OBJ roHKCU(HKEY_CURRENT_USER, CACHE5_KEY);
    if (roHKCU.GetStatus()==ERROR_SUCCESS)
    {
        roHKCU.DeleteValue(CACHE_SIGNATURE_VALUE);
    }
    
    // Read user shell folders (necessary only in HKCU) and write back as REG_EXPAND_SZ
    REGISTRY_OBJ roUserShellFolders(HKEY_CURRENT_USER, USER_SHELL_FOLDER_KEY);
    if (roUserShellFolders.GetStatus()!=ERROR_SUCCESS)
    {
        return;
    }
    roUserShellFolders.DeleteValue(TEXT("Content"));

    for (int i=0; i<NCONTAINERS; i++)
    {
        TCHAR szPath[MAX_PATH];
        DWORD cc = ARRAY_ELEMENTS(szPath);
        if (roUserShellFolders.GetValue(g_szOldSubKey[i], (LPBYTE)szPath, &cc)!=ERROR_SUCCESS)
        {
            continue;
        }
        // First reconcile path to whatever it should be
        // and rename the containers accordingly.
        // i. Get rid of all the trailing content.ie5 (History.IE5)
        // ii. Get rid of any trailing Temporary Internet Files (History)
        // iii. Append Temporary Internet Files (History)

        // We want to skip this for cookies, though.

        // PROBLEM: When we have upgrade on top of 0901+, we started appending content.ie5
        // internally. Thus, files start getting misplaced. How do I work around this?

        // Idea: We append Content.ie5 to the USF path, and test for existence. If it's there,
        // then we'll use that. (We won't bother with anymore detective work. Though we could also
        // verify that the index dat there is newer than the index.dat in the parent directory.)
        TCHAR szRealPath[MAX_PATH];
        
        // Expand string
        ExpandEnvironmentStrings(szPath, szRealPath, ARRAY_ELEMENTS(szRealPath));
        DisableCacheVu(szRealPath);

        if (i!=1)
        {
        }
        
        // Contract string
        if (!NormalisePath(szRealPath, TEXT("%USERPROFILE%"), szPath, sizeof(szPath)))
        {
            NormalisePath(szRealPath, TEXT("%SystemRoot%"), szPath, sizeof(szPath));
        }
        
        // Then write it back
        roUserShellFolders.DeleteValue(g_szOldSubKey[i]);
        roUserShellFolders.SetValue(g_szOldSubKey[i], szPath, REG_EXPAND_SZ);

        // Then append Content.IE5 and move the files to this subdirectory
        // Ideally, we should rename this to an intermediate folder,
        // delete the old location (UNLESS THIS IS THE ROOT OR SYSTEM DIRECTORY),
        // and then move intermediate folder to its new location

    }

    FlushShellFolderCache();
}

// External hooks -------------------------------------------------------------------------------------------------------

// -- RunOnceUrlCache -------------------------
// This code is called on reboot to clean up moving the cache.
// If the shutdown was successful, this will move only the few files that were open
// at that point; we assume that we'll move quickly enough to prevent collisions.
// The old index.dat is erased.

DWORD
WINAPI
RunOnceUrlCache( HWND hwnd, HINSTANCE hinst, PSTR pszCmd, int nCmdShow)
{
    // This will clean up the move, especially important if the move was interrupted. (Not too likely.)
    if (pszCmd && *pszCmd)
    {
        CFileMgr::DeleteCache(pszCmd);
    }
    return ERROR_SUCCESS;
}
      
DWORD
WINAPI
DeleteIE3Cache( HWND hwnd, HINSTANCE hinst, PSTR lpszCmd, int nCmdShow)
{
    // This will clean up the move, especially important if the move was interrupted. (Not too likely.)
    if (lpszCmd && *lpszCmd)
    {
        CFileMgr::DeleteCache(lpszCmd);
    }
    return ERROR_SUCCESS;
}
      

// -- Externally available apis

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheConfigInfoA(
    LPCACHE_CONFIG_INFO pConfig,
    DWORD dwFieldControl
    )
/*++

Routine Description:

    This function sets the cache configuration parameters.

Arguments:

    lpCacheConfigInfo - place holding cache configuration information to be set

    dwFieldControl - items to get

Return Value:

    Error Code

--*/
{
    // Initialize globals
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->SetUrlCacheConfigInfo(pConfig,dwFieldControl);
}

URLCACHEAPI
BOOL
WINAPI
GetUrlCacheConfigInfoA(
    LPCACHE_CONFIG_INFO lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    DWORD dwFieldControl
    )
/*++

Routine Description:

    This function retrieves cache configuration values from globals

Arguments:

    pConfig - pointer to a location where configuration information
                  is stored on a successful return

    lpdwCacheConfigInfoBufferSize : pointer to a location where length of
        the above buffer is passed in. On return, this contains the length
        of the above buffer that is fulled in.

    dwFieldControl - items to get

Return Value:

    Error Code

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheConfigInfoA", "%#x, %#x, %#x",
        lpCacheConfigInfo, lpdwCacheConfigInfoBufferSize, dwFieldControl ));

    BOOL fError;
    
    // Initialize globals
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        DEBUG_ERROR(API, ERROR_INTERNET_INTERNAL_ERROR);
        fError = FALSE;
    }
    else
    {
        fError = GlobalUrlContainers->GetUrlCacheConfigInfo(lpCacheConfigInfo,
            lpdwCacheConfigInfoBufferSize, dwFieldControl);
    }
    
    DEBUG_LEAVE_API (fError);
    return fError;
}


// declared in wininet\inc\urlcache.h
BOOL GetIE5ContentPath( LPSTR szPath)
{
    BOOL retVal = FALSE;
    
    IE5_REGISTRYSET ie5rs;
    BOOL fProfilesCapable;

    if( ie5rs.InitialiseKeys(fProfilesCapable) != ERROR_SUCCESS)
        goto doneGetContentPath;

    if( ie5rs.SetWorkingContainer(CONTENT) != ERROR_SUCCESS)
        goto doneGetContentPath;

    if( ie5rs.GetPath( szPath) != ERROR_SUCCESS)
        goto doneGetContentPath;

    retVal = TRUE;

doneGetContentPath:
    return retVal;
}

// SHDOCVW needs to know whether profiles are enabled, to determine whether
// or not it needs to filter out user names. This function will help keep things simple.
// And minimise perf impact.
#ifdef UNIX
extern "C"
#endif
BOOL IsProfilesEnabled()
{
    IE5_REGISTRYSET ie5rs;
    BOOL fProfilesEnabled;

    if (ie5rs.InitialiseKeys(fProfilesEnabled) != ERROR_SUCCESS)
    {
        fProfilesEnabled = FALSE;
    }
    return fProfilesEnabled;
}

BOOL CConMgr::DiscoverIE4Settings(IE5_REGISTRYSET* pie5rs)
{
    IE4_REGISTRYSET ie4rs;

    CHAR szTemp[MAX_PATH+1], szPrefix[MAX_PATH+1];
    DWORD cbLimit, dwTemp;
    BOOL fPerUser, fCaughtIE4;
        
    // Try to find IE4 settings. If any paths are found, we will not look for IE3 settings
    fCaughtIE4 = FALSE;

    if (ie4rs.WasIE4Present(_fProfilesCapable))
    {
        if (ie4rs.InitialiseKeys(_fProfilesCapable)!=ERROR_SUCCESS)
        {
           LOG_UPGRADE_DATA("IE4 initialisation failed...\n");
           return FALSE;
        }

        for (dwTemp=0;dwTemp < NCONTAINERS; dwTemp++)
        {
            ie4rs.SetWorkingContainer(dwTemp);
            if (ie4rs.GetPath(szTemp)!=ERROR_SUCCESS)
            {
                continue;
            }
            LOG_UPGRADE_DATA("DIE4Settings: ");
            LOG_UPGRADE_DATA(szTemp);
            LOG_UPGRADE_DATA("\n");
            DisableCacheVu(szTemp);

            pie5rs->SetWorkingContainer(dwTemp);

            // Because SHGetFolderPath uses shell folders to determine where the items are, we have to accomodate this
            // on no-profiles machines. 
            if (!_fProfilesCapable)
            {
                pie5rs->SetPath(szTemp);
            }
            else if (dwTemp==CONTENT)
            {
#ifndef UNIX
                if (ie4rs.GetPerUserStatus() || GlobalPlatformVersion5)
#else
                if (ie4rs.GetPerUserStatus() || GlobalPlatformType == PLATFORM_TYPE_UNIX)
#endif /* UNIX */
                {
                // If it's NT5, we want to go to a per-user, non-roaming location
                // which is the NT5 default anyway
                    LOG_UPGRADE_DATA("DIE4Settings: If NT5, ignore shared cache. Else this isn't shared anyway.");
                    pie5rs->SetPerUserStatus(TRUE);
                }
                else
                {
                // Because IE4 locates a shared cache differently from IE5, we need to
                // save the path and status.
                    LOG_UPGRADE_DATA("DIE4Settings: Will try to use shared cache");
                    pie5rs->AttemptToUseSharedCache(szTemp, 0);
                }
            }
        
            fCaughtIE4 = TRUE;

            // We don't need to check return values since we come up with
            // reasonable values on our own.
            ie4rs.GetLimit(szTemp, cbLimit);
            pie5rs->SetLimit(szTemp, cbLimit);

            ie4rs.GetPrefix(szPrefix);
            pie5rs->SetPrefix(szPrefix);
        }
    }
    if (!fCaughtIE4)
    {
        LOG_UPGRADE_DATA("No IE4 settings...\n");
    }
    else
    {
        FlushShellFolderCache();
    }
    return fCaughtIE4;
}

VOID CConMgr::DiscoverIE3Settings(IE5_REGISTRYSET* pie5rs)
{
    IE3_REGISTRYSET ie3rs;
   
    if (ie3rs.InitialiseKeys()!=ERROR_SUCCESS)
    {
        return;
    }

    TCHAR szTemp[MAX_PATH];
    DWORD cbLimit;
    
    // This fragment will look for a cache location, and test for its share-ability. If it is,
    // we'll use the location; otherwise, we'll use our own shared location.
    if (ie3rs.GetContentDetails(szTemp, cbLimit))
    {
        DeleteCachedFilesInDir(szTemp);
       // No IE4. Steal IE3's settings? We only care about content cache.
       // Is that a good idea? There's no UI for modifying the cookies/history path; 
       // if someone plumbs into the registry, do we want to support that? *sigh*
       // BUG? We're moving the cache one level deeper. We probably want to be a
       // bit more intelligent about this.
        CleanPath(szTemp);
        pie5rs->AttemptToUseSharedCache(szTemp, cbLimit);
    }

    // This fragment deletes the shared history. It should happen ONLY ONCE.
    DWORD cbKeyLen = ARRAY_ELEMENTS(szTemp);
    REGISTRY_OBJ roHist(HKEY_LOCAL_MACHINE, IE3_HISTORY_PATH_KEY);
    if ((roHist.GetStatus()==ERROR_SUCCESS)
        &&
        (roHist.GetValue(NULL,(LPBYTE)szTemp, &cbKeyLen)==ERROR_SUCCESS))
    {
        REGISTRY_OBJ roUrlHist(HKEY_LOCAL_MACHINE, szTemp);
        cbKeyLen = ARRAY_ELEMENTS(szTemp);
        if ((roUrlHist.GetStatus()==ERROR_SUCCESS)
            &&
            (roUrlHist.GetValue(CACHE_DIRECTORY_VALUE, (LPBYTE)szTemp, &cbKeyLen)==ERROR_SUCCESS))
        {
            DeleteCachedFilesInDir(szTemp);
        }
    }
}


// Logic for determining the location of the cache -------------------------------------------------------------

// PROFILES ENABLED --------------------------
// [on logon]
// If profiles are enabled, look in HKCU/Software/Microsoft/Windows/Internet Settings/5.0/Cache
//      for a signature. 
// If a signature is present, [carry on]
// Look in HKLM/Software/Microsoft/Windows/Internet Settings/5.0/Cache
//      for a signature.
// If a signature is not present, jump to [over IE4 install]
// For history and cookies, the containers will be located in the profiles directory
// For content, 
//      if it's marked per user, 
//          and there isn't a shell folder/user shell folder value, construct and put it in
//      otherwise feed the HKLM shared location into (user) shell folder.
// Insert signature and [carry on].

// [over IE4 install]
// If a signature is not present in HKCU/Software/Microsoft/Windows/Internet Settings/Cache,
//      jump to [over IE3 install]
// Determine if the content cache is per-user or not.

// [over IE3 install]
// For history and cookies, the containers will be located in the profiles directory
// Examine HKLM/Software/Microsoft/Windows/Internet Settings/Cache/Paths
// If not present, go to [clean install]
// If the cache path is located in a user's profiles directory, ignore and [clean install]
// Otherwise, adopt the values and [carry on]

// [clean install]
// Set up history/cookies to be per user.
// Set up the content cache to be shared.
// Write in default values.

// PROFILES NOT ENABLED --------------------------

// * If profiles are _not_ enabled, we'll look in HKLM/Software/Microsoft/Windows/Internet Settings/5.0/Cache
//      for a signature.
// If a signature is present, go ahead and gather information for the paths

// [carry on]
// Get info from the registry, and create the container


// -- DiscoverAnyIE5Settings
// We're going to call this function if we haven't any IE4 settings to upgrade,
// but _before_ we check for IE3,

DWORD GetIEVersion();

BOOL CConMgr::DiscoverAnyIE5Settings(IE5_REGISTRYSET* pie5rs)
{
    // Let's consider the following scenario:
    // User A logs on to the machine with IE4 installed; installs IE5, and then shuts down
    // the machine. User B comes along, but IE5 hasn't been installed yet. If User B has 
    // admin privileges, install will continue, BUT still not have any IE4/5 settings.
    // Which resulted in skipping DiscoverIE4Settings. However, we don't want to look
    // at IE3's settings.

    // If we're installing over IE4/2/5, but we don't have any settings for this user, 
    // we must avoid an IE3 upgrade. Instead, short circuit to use last-minute info-gathering
    // For IE3, use a shared cache
    DWORD dwVer = GetIEVersion();
    
    // We're going to use a shared cache for IE3 and Win9x users.
    // Upgrading over IE4 and 5 -- for users who have logged in before,
    // their signatures shoudl be in place already. In those cases, we shouldn't
    // be in this function anyway. For other users, we'll use the shared cache.
    // This is the first time for the machine.
    if ((dwVer==3) && pie5rs->IsFirstTimeForMachine())
    {
        return FALSE;
    }

    pie5rs->AttemptToUseSharedCache(NULL, 0);
    return TRUE;
}


VOID CConMgr::DiscoverRegistrySettings(IE5_REGISTRYSET* pie5rs)
{
    LOG_UPGRADE_DATA("Attempting to discover IE4 settings...\n");

    if (DiscoverIE4Settings(pie5rs))
    {
        goto exit;
    }


#ifndef UNIX
    if (GlobalPlatformType == PLATFORM_TYPE_WINNT)
#else
    if (GlobalPlatformType == PLATFORM_TYPE_UNIX)
#endif /* UNIX */
    {
        LOG_UPGRADE_DATA("This is NT. Fuhgedabout IE3 et al settings...\n");

        // This will override NT's default behaviour to use per-user containers.
        pie5rs->SetWorkingContainer(CONTENT);
        if (!pie5rs->GetPerUserStatus())
        {
            pie5rs->AttemptToUseSharedCache(NULL, 0);
        }

        // Suppose this is an install over NT. Each user should get a per-user, non-roaming
        // path by default. The values we'd pick up from SHFolderGetPath will be okay; but, we need to make
        // sure that we treat this as a per-user container.

        // Suppose we upgraded from some previous version of Win9x to NT5. There are eight scenarios:
        // nothing                  : in which case, we do as above
        // IE3                      : same; need to delete old cache
        // IE4 -- single-user machine:same as fresh install; need to delete old cache.
        //     -- shared cache      : same as fresh install
        //     -- per user cache    : preserve path
        //     -- moved cache       : preserve path
        // IE5 -- single-user machine:same as fresh install; need to delete old cache.
        //     -- shared cache      : preserve shared path
        //     -- per user cache    : preserve path
        //     -- moved cache       : preserve path

        // If an admin wants to use a shared cache under NT5, s/he will have to set
        // HKCU/Shell Folders and User Shell Folders to point to the common path
        // AND {HKCU|HKLM}/blah/PerUserItem to 0 (or delete the HKCU value).
        if ((GetIEVersion()==3) && pie5rs->IsFirstTimeForMachine())
        {
            IE3_REGISTRYSET ie3rs;
            TCHAR szTemp[MAX_PATH];
            DWORD cbLimit;
            if ((ie3rs.InitialiseKeys()==ERROR_SUCCESS)
                &&
                (ie3rs.GetContentDetails(szTemp, cbLimit)))
            {
                DeleteCachedFilesInDir(szTemp);
            }
        }
        goto exit;
    }

    LOG_UPGRADE_DATA("Attempting to discover any IE5 settings...\n");
    if (DiscoverAnyIE5Settings(pie5rs))
    {
        goto exit;
    }

    LOG_UPGRADE_DATA("Attempting to discover any IE3 settings...\n");
    DiscoverIE3Settings(pie5rs);

exit:
    LOG_UPGRADE_DATA("Flushing shell folders cache...\n");
    pie5rs->SetIfFirstTime();
    FlushShellFolderCache();
}

// -- AttemptToUseSharedCache
// Given a path (and limit) attempt to use the path for a shared location.
// If the path is null, then try to use any value if present, else invent one.

VOID IE5_REGISTRYSET::AttemptToUseSharedCache(PTSTR pszPath, DWORD ckbLimit)
{
    TCHAR szSharedPath[MAX_PATH];
    DWORD cc = ARRAY_ELEMENTS(szSharedPath);

    REGISTRY_OBJ roContent(&m_roHKLMCache, g_szSubKey[CONTENT], CREATE_KEY_IF_NOT_EXISTS);
    if ((roContent.GetStatus()==ERROR_SUCCESS)
        &&
        (roContent.GetValue(CACHE_PATH_VALUE, (LPBYTE)szSharedPath, &cc)==ERROR_SUCCESS))
    {
        LOG_UPGRADE_DATA("Found a shared cache location...\n");
        goto write_value;
    }

    if (pszPath!=NULL)
    {
        LOG_UPGRADE_DATA(pszPath);
        LOG_UPGRADE_DATA("\n is ");
        GetUserName(szSharedPath, &cc);

        // We're going to ignore just the user name, during this comparison. And
        // if it's in the profiles directory, fuhgedaboutit.
        if (m_fProfiles && !StrCmpNI(m_szProfilePath, pszPath, cbP-cc))
        {
            pszPath = NULL;
            LOG_UPGRADE_DATA("not okay \n");
            goto carryon;
        }
        LOG_UPGRADE_DATA("okay \n");
        strcpy(szSharedPath, pszPath);
    }

carryon:
    if (pszPath==NULL)
    {
        memcpy(szSharedPath, m_szSharedPath, (cbS+1)*sizeof(TCHAR));
        CleanPath(szSharedPath);
        LOG_UPGRADE_DATA("Using a constructed shared path\n");
    }

    // We've finally decided on the path. Now let's write the value into the registry.
    roContent.SetValue(CACHE_PATH_VALUE, szSharedPath, REG_SZ);
    SetWorkingContainer(CONTENT);
    SetPerUserStatus(FALSE);
    SetLimit(szSharedPath, ckbLimit);

write_value:
    LOG_UPGRADE_DATA("The shared cache will be located at ");
    LOG_UPGRADE_DATA(szSharedPath);
    LOG_UPGRADE_DATA("\n");
    
    // This will take care of HKCU
    CHAR szScratch[MAX_PATH];
#ifndef UNIX
    if (!NormalisePath(szSharedPath, TEXT("%SystemRoot%"), szScratch, sizeof(szScratch)))
#else
    if (!NormalisePath(szSharedPath, TEXT("%USERPROFILE%"), szScratch, sizeof(szScratch)))
#endif /* UNIX */
    {
        strncpy(szScratch, szSharedPath, MAX_PATH);
    }
    if (m_roUserShellFolder.SetValue(g_szOldSubKey[CONTENT], szScratch, REG_EXPAND_SZ)==ERROR_SUCCESS)
    {
#ifndef UNIX
       DWORD dwType = REG_SZ;
        m_roShellFolder.SetValue(g_szOldSubKey[CONTENT], szSharedPath, dwType);
#else
        m_roShellFolder.SetValue(g_szOldSubKey[CONTENT], szScratch, REG_EXPAND_SZ);
#endif /* UNIX */
    }
}


// -- CleanPath
// Given a path, strip away any trailing content.ie5's, and if necessary, add a trailing brand mark,
// i.e. "Temporary Internet Files" or localised version.

VOID CleanPath(PTSTR pszPath)
{
    DWORD ccPath = strlen(pszPath);
    PTSTR pszLastSep = pszPath + ccPath;

    // Now we're at the null terminator, but if the last character is also a separator, we want
    // to skip that too.
    if (*(pszLastSep-1)==DIR_SEPARATOR_CHAR)
    {
        pszLastSep--;
    }
    BOOL fSlash;
    // Strip away any "content.ie5"'s from the path
    for (;(fSlash = ScanToLastSeparator(pszPath, &pszLastSep));)
    {
        if (StrCmpNI((pszLastSep+1), TEXT("content.ie"), ARRAY_ELEMENTS(TEXT("content.ie"))-1))
        {
            break;
        }
        *pszLastSep = '\0';
    }

    // Load temp int files
    TCHAR szBrand[MAX_PATH];
    DWORD ccBrand = 0;

    ccBrand = LoadString(GlobalDllHandle, g_dwCachePathResourceID[CONTENT], szBrand, ARRAY_ELEMENTS(szBrand));

    // The following fragment should never happen, but just in case...
    if (!ccBrand)
    {
        ccBrand = sizeof(TEXT("Temporary Internet Files"));
        memcpy(szBrand, TEXT("Temporary Internet Files"), ccBrand);
        ccBrand /= sizeof(TCHAR);
        ccBrand--;
    }
    
    // If "Temporary Internet Files" doesn't trail the path, add it.
    if (!fSlash)
    {
        *pszLastSep++ = DIR_SEPARATOR_CHAR;
        *pszLastSep = '\0';
    }
    else
    {
        pszLastSep++;
    }
    if (StrCmpNI((pszLastSep), szBrand, ccBrand))
    {
        while (*pszLastSep && *pszLastSep!=DIR_SEPARATOR_CHAR)
        {
            pszLastSep++;
        }
        if (!*pszLastSep && (*(pszLastSep-1)!=DIR_SEPARATOR_CHAR))
        {
            *pszLastSep = DIR_SEPARATOR_CHAR;
            pszLastSep++;
        }
        else if (*pszLastSep)
        {
            pszLastSep++;
        }
        memcpy(pszLastSep, szBrand, ccBrand*sizeof(TCHAR));
    }
    *(pszLastSep+ccBrand)='\0';
}

DWORD GetIEVersion()
{
    DWORD dwVer = 0;

    REGISTRY_OBJ roVersion(HKEY_LOCAL_MACHINE, OLD_VERSION_KEY);
    TCHAR szKey[MAX_PATH];
    DWORD cb = ARRAY_ELEMENTS(szKey);

    if ((roVersion.GetStatus()!=ERROR_SUCCESS)
        ||
        (roVersion.GetValue(OLD_VERSION_VALUE, (LPBYTE)szKey, &cb)!=ERROR_SUCCESS))
    {
        // This should never happen during a proper setup.
        // In case it does, however, we'll just use construct IE5's default settings.
        return 0;
    }

    PTSTR psz = szKey;
    PTSTR pszFirst = szKey;
    // Get the major version number
    while (*psz!='.')
    {
        psz++;
    }
    *psz = '\0';
    dwVer = (DWORD)StrToInt(pszFirst);

    if (dwVer==4)
    {
        psz++;
        // Skip the second number
        while (*psz!='.')
        {
            psz++;
        }
        pszFirst = psz;
        psz++;
        while (*psz!='.')
        {
            psz++;
        }
        *psz = '\0';
        dwVer = ((DWORD)StrToInt(pszFirst))==0 ? 3 : 4;
    }

    return dwVer;
}

