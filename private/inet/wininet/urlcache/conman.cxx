#include <cache.hxx>

#define IsFieldSet(fc, bitFlag) (((fc) & (bitFlag)) != 0)

// Cache path keys.
CHAR* szSubKey[] = {CONTENT_PATH_KEY, COOKIE_PATH_KEY, HISTORY_PATH_KEY};

// Top level cache paths resource IDs
DWORD dwCachePathResourceID[] = {IDS_CACHE_DEFAULT_SUBDIR, 
    IDS_COOKIES_DEFAULT_SUBDIR, IDS_HISTORY_DEFAULT_SUBDIR};

// Cache prefixes.
CHAR* szCachePrefix[] = {CONTENT_PREFIX, COOKIE_PREFIX, HISTORY_PREFIX};

// Default CONTENT reg subkeys under HKLM.
const static LPCSTR  rglpDefCacheSubKeys[DEF_NUM_PATHS] = 
{
    "Path1",
    "Path2",
    "Path3",
    "Path4"
};

// Default CONTENT subdirs under HKLM.
const static LPCSTR  rglpDefCachePaths[DEF_NUM_PATHS] = 
{
    "Cache1",
    "Cache2",
    "Cache3",
    "Cache4"
};




/*---------------------  Private Functions -----------------------------------*/


/*-----------------------------------------------------------------------------
DWORD CConainerManager::Init
----------------------------------------------------------------------------*/
DWORD CContainerManager::Init()
{
    // BUGBUG: should zero init ContainerArray

    DWORD dwError;
    ContainerIndex idx; 

    // First try to get the startup settings from HKEY_LOCAL_MACHINE.
    if (GetHKLMCacheConfigInfo() != ERROR_SUCCESS)
    {
        // Getting settings from HKEY_LOCAL_MACHINE failed. Try to 
        // get the startup settings from HKEY_CURRENT_USER
        if (GetHKCUCacheConfigInfo() != ERROR_SUCCESS)
        {
            // Try to get startup settings from internal info.
            // No party if we can't even get this.
            if (dwError = GetDefaultCacheConfigInfo() != ERROR_SUCCESS)
                goto exit;
            else
            {
                // Set HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE
                // entries from the default (internal) settings.
                SetHKCUCacheConfigInfo();
                SetHKLMCacheConfigInfo();
            }
        }
        else
        {
            // Set HKEY_LOCAL_MACHINE entries from
            // the HKEY_CURRENT_USER entries.
            SetHKLMCacheConfigInfo();
        }   
    }   
    else
    {
        // Settings may have changed. Update HKCU to reflect.
        SetHKCUCacheConfigInfo();
    }

    // The Info array is now initialized. Create the containers.
    for (idx = CONTENT; idx < MAX_CONTAINERS; idx++)
    {
        ContainerArray[idx] = new URL_CONTAINER(Info[idx].szCachePath,
            Info[idx].szCachePrefix, Info[idx].cbCacheLimit);
    
        if (!ContainerArray[idx])
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        dwError = ContainerArray[idx]->GetStatus();
        if(dwError != ERROR_SUCCESS) 
            goto exit;
    }
exit:
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetHKLMCacheConfigInfo
  ----------------------------------------------------------------------------*/
DWORD CContainerManager::GetHKLMCacheConfigInfo()
{
    //
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

    DWORD i, dwError, cbKeyLen, cbKBLimit;
    LONGLONG cbLimit = 0;    

    REGISTRY_OBJ *roCache = 0, *roPaths = 0, *roSpecialPaths = 0, 
                 *roCookies = 0, *roHistory = 0, *roUrlHistory = 0,
                 *roPath[DEF_NUM_PATHS] = {0, 0, 0, 0}; 
    
    // Cache registry object.
    if (dwError = ConstructRegObj
            (roCache, HKEY_LOCAL_MACHINE, CACHE_KEY) != ERROR_SUCCESS)
        goto exit;

    // Paths registry object.
    if (dwError = ConstructRegObj
            (roPaths, roCache, CACHE_PATHS_KEY) != ERROR_SUCCESS)
        goto exit;

    // Special Paths registry object.
    if (dwError = ConstructRegObj
            (roSpecialPaths, roCache, CACHE_SPECIAL_PATHS_KEY) != ERROR_SUCCESS)
        goto exit;

    // Cookies.
    if (dwError = ConstructRegObj
            (roCookies, roSpecialPaths, COOKIE_PATH_KEY) != ERROR_SUCCESS)
        goto exit;

    // History (used ONLY to get UrlHistory key string).
    if (dwError = ConstructRegObj
            (roHistory, roSpecialPaths, HISTORY_PATH_KEY) != ERROR_SUCCESS)
        goto exit;

    // Url History
    cbKeyLen = MAX_PATH;
    CHAR szUrlHistoryPath[MAX_PATH];
    if (dwError = roHistory->GetValue
            (NULL, (LPBYTE) szUrlHistoryPath,  &cbKeyLen) != ERROR_SUCCESS)
        goto exit;
    
    if (dwError = ConstructRegObj
            (roUrlHistory, HKEY_LOCAL_MACHINE, szUrlHistoryPath) != ERROR_SUCCESS)
        goto exit;
    

    // Path1, Path2, Path3, Path4.
    for (i = 0; i < DEF_NUM_PATHS; i++)
        if (dwError = ConstructRegObj(roPath[i], roPaths, 
        (LPSTR) rglpDefCacheSubKeys[i]) != ERROR_SUCCESS)
            goto exit;
         
    
    // 1) ----- Get the CONTENT cache path, prefix and limit -----

    // CONTENT cache path.
    cbKeyLen = MAX_PATH;
    if (dwError = roPaths->GetValue(CACHE_DIRECTORY_VALUE, 
        (LPBYTE) Info[CONTENT].szCachePath, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

    // BUGBUG - when altered by inetcpl, path may have a terminating
    // slash. We don't setup with this originally and don't want it.
    if (cbKeyLen > 1 && Info[CONTENT].szCachePath[cbKeyLen-2] == '\\')
        Info[CONTENT].szCachePath[cbKeyLen-2] = '\0';
    
    
    // CONTENT cache prefix is "" and is not in old registry settings.
    strcpy(Info[CONTENT].szCachePrefix, CONTENT_PREFIX);
    
    // CONTENT cache limit: Add up the path[DEF_NUM_PATHS] cache limits for the CONTENT total.
    Info[CONTENT].cbCacheLimit = 0;
    for (i = 0; i < DEF_NUM_PATHS; i++)
    {
        // Limit in kilobytes.
        if (dwError = roPath[i]->GetValue(CACHE_LIMIT_VALUE, &cbKBLimit) != ERROR_SUCCESS)
            goto exit;
            
        // Limit in bytes.
        Info[CONTENT].cbCacheLimit += ((LONGLONG) cbKBLimit) * 1024;
    }
    
    // 2) ----- Get the COOKIE cache path, prefix and limit -----

    // COOKIE cache path.
    cbKeyLen = MAX_PATH;
    if (dwError = roCookies->GetValue(CACHE_DIRECTORY_VALUE, 
        (LPBYTE) Info[COOKIE].szCachePath, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

    // COOKIE cache prefix.
    cbKeyLen = MAX_PATH;
    if (dwError = roCookies->GetValue(CACHE_PREFIX_VALUE,
        (LPBYTE) Info[COOKIE].szCachePrefix, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

    // COOKIE cache limit in kilobytes.
    if (dwError = roCookies->GetValue(CACHE_LIMIT_VALUE, 
        &cbKBLimit) != ERROR_SUCCESS)
        goto exit;

    // Limit in bytes.
    Info[COOKIE].cbCacheLimit = ((LONGLONG) cbKBLimit) * 1024;
    
    // 3) ----- Get the HISTORY cache path, prefix and limit -----
    
    // HISTORY cache path.
    cbKeyLen = MAX_PATH;
    if (dwError = roUrlHistory->GetValue(CACHE_DIRECTORY_VALUE, 
        (LPBYTE) Info[HISTORY].szCachePath, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

    // HISTORY cache prefix.
    cbKeyLen = MAX_PATH;
    if (dwError = roUrlHistory->GetValue(CACHE_PREFIX_VALUE,
        (LPBYTE) Info[HISTORY].szCachePrefix, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

    // HISTORY cache limit.
    if (dwError = roUrlHistory->GetValue(CACHE_LIMIT_VALUE, 
        &cbKBLimit) != ERROR_SUCCESS)
        goto exit;

    // Limit in bytes.
    Info[HISTORY].cbCacheLimit = ((LONGLONG) cbKBLimit) * 1024;
   
exit:    
    delete roCache;
    delete roPaths;
    delete roSpecialPaths;
    delete roCookies;
    delete roHistory;
    delete roUrlHistory;

    for (i = 0; i < DEF_NUM_PATHS; i++)
        delete roPath[i];

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::SetHKLMCacheConfigInfo
  ----------------------------------------------------------------------------*/
DWORD CContainerManager::SetHKLMCacheConfigInfo()
{
    //
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

    DWORD i, nPaths, dwError, cbKeyLen, cbKBLimit;

    REGISTRY_OBJ *roHKLM = 0, *roCache = 0, *roPaths = 0, 
        *roSpecialPaths = 0, *roCookies = 0, *roHistory = 0, 
        *roUrlHistory = 0, *roPath[DEF_NUM_PATHS] = {0, 0, 0, 0}; 
    
    // HKLM base registry object.
    if (dwError = ConstructRegObj(roHKLM, HKEY_LOCAL_MACHINE, 
        NULL) != ERROR_SUCCESS)
        goto exit;
    
    // Create Cache entry.
    if (dwError = roHKLM->Create(CACHE_KEY,
        &roCache) != ERROR_SUCCESS)
        goto exit;
    
    // Create Paths entry.
    if (dwError = roCache->Create(CACHE_PATHS_KEY, 
        &roPaths) != ERROR_SUCCESS)
        goto exit;

    // Create Special Paths entry.
    if (dwError = roCache->Create(CACHE_SPECIAL_PATHS_KEY, 
        &roSpecialPaths) != ERROR_SUCCESS)
        goto exit;

    // Create Cookies entry.
    if (dwError = roSpecialPaths->Create(COOKIE_PATH_KEY, 
        &roCookies) != ERROR_SUCCESS)
        goto exit;

    // Create History entry
    if (dwError = roSpecialPaths->Create(HISTORY_PATH_KEY, 
        &roHistory) != ERROR_SUCCESS)
        goto exit;

    // Set the History key to point to Url History.
    CHAR szUrlHistoryPath[MAX_PATH];
    strcpy(szUrlHistoryPath, CACHE_KEY);
    strcat(szUrlHistoryPath, "\\");
    strcat(szUrlHistoryPath, URL_HISTORY_KEY);
    if (dwError = roHistory->SetValue(NULL, szUrlHistoryPath, 
        REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // Create Url History entry
    if (dwError = roHKLM->Create(szUrlHistoryPath, 
        &roUrlHistory) != ERROR_SUCCESS)
        goto exit;
        
    // Path1, Path2, Path3, Path4.
    for (i = 0; i < DEF_NUM_PATHS; i++)
    {
        if (dwError = roPaths->Create((LPSTR) rglpDefCacheSubKeys[i], 
            &roPath[i]) != ERROR_SUCCESS)
            goto exit;
            
    }

    // 1) ----- Set the CONTENT cache path, prefix and limit -----

    // Cache content path from Info[CONTENT]:
    if (dwError = roPaths->SetValue(CACHE_DIRECTORY_VALUE, 
        Info[CONTENT].szCachePath, REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // No prefix entry for CONTENT

    // Number of subdirectories (optional).
    nPaths = DEF_NUM_PATHS;
    if (dwError = roPaths->SetValue(CACHE_PATHS_KEY, 
        &nPaths) != ERROR_SUCCESS)
        goto exit;
    
    // BUGBUG - make strcpy to strncpy in all code?
    // Subdirectorie paths and limits from CONTENT.
    for (i = 0; i < DEF_NUM_PATHS; i++)
    {
        CHAR szSubDir[MAX_PATH];

        // Path1, Path2, Path3, Path3
        strcpy(szSubDir, Info[CONTENT].szCachePath);
        strcat(szSubDir, "\\");
        strcat(szSubDir, rglpDefCachePaths[i]);
        if (dwError = roPath[i]->SetValue(CACHE_PATH_VALUE, 
            szSubDir, REG_SZ) != ERROR_SUCCESS)
            goto exit;    

        // Cache limits for Path1, Path2, Path3 and Path4 are
        // the CONTENT cache limit divided by DEF_NUM_PATHS.
        DWORD cbCacheLimitPerSubCache = (DWORD) 
            (Info[CONTENT].cbCacheLimit / (1024 * DEF_NUM_PATHS));

        if (dwError = roPath[i]->SetValue(CACHE_LIMIT_VALUE, 
            &cbCacheLimitPerSubCache) != ERROR_SUCCESS)
            goto exit;    
    }

    // 2) ----- Set the COOKIE cache path, prefix and limit -----

    // COOKIE cache path.
    if (dwError = roCookies->SetValue(CACHE_DIRECTORY_VALUE, 
        Info[COOKIE].szCachePath, REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // COOKIE cache prefix.
    if (dwError = roCookies->SetValue(CACHE_PREFIX_VALUE,
        Info[COOKIE].szCachePrefix, REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // COOKIE cache limit.
    cbKBLimit = (DWORD) (Info[COOKIE].cbCacheLimit / 1024);
    if (dwError = roCookies->SetValue(CACHE_LIMIT_VALUE, 
        &cbKBLimit) != ERROR_SUCCESS)
        goto exit;
    
    // 3) ----- Set the HISTORY cache path, prefix and limit -----
    
    // HISTORY cache path.
    if (dwError = roUrlHistory->SetValue(CACHE_DIRECTORY_VALUE, 
        Info[HISTORY].szCachePath, REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // HISTORY cache prefix.
    if (dwError = roUrlHistory->SetValue(CACHE_PREFIX_VALUE,
        Info[HISTORY].szCachePrefix, REG_SZ) != ERROR_SUCCESS)
        goto exit;

    // HISTORY cache limit.
    cbKBLimit = (DWORD) (Info[HISTORY].cbCacheLimit / 1024);
    if (dwError = roUrlHistory->SetValue(CACHE_LIMIT_VALUE, 
        &cbKBLimit) != ERROR_SUCCESS)
        goto exit;
    
exit:    

    delete roHKLM;
    delete roCache;
    delete roPaths;
    delete roSpecialPaths;
    delete roCookies;
    delete roHistory;
    delete roUrlHistory;

    for (i = 0; i < DEF_NUM_PATHS; i++)
        delete roPath[i];

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetHKCUCacheConfigInfo
  ----------------------------------------------------------------------------*/
DWORD CContainerManager::GetHKCUCacheConfigInfo()
{
    DWORD cbKeyLen, dwError, nStrings, cbKBLimit;
    CHAR szWindowsPath[MAX_PATH];
    ContainerIndex i;

    REGISTRY_OBJ *roCache = 0, *roContainer[N_CONTAINERS] = {0, 0, 0};

    if (dwError = ConstructRegObj(roCache, HKEY_CURRENT_USER, CACHE_KEY) != ERROR_SUCCESS)
        goto exit;
    
    // Get the container paths, prefixes (if any) and default limit values.
    for (i = CONTENT; i < MAX_CONTAINERS; i++)
    {
        // Construct registry object.
        if (dwError = ConstructRegObj(roContainer[i], roCache, szSubKey[i]) != ERROR_SUCCESS)
            goto exit;

        // Get the registry info.

        // Path.
        cbKeyLen = MAX_PATH;
        if (dwError = roContainer[i]->GetValue(CACHE_PATH_VALUE, 
            (LPBYTE) Info[i].szCachePath, &cbKeyLen) != ERROR_SUCCESS)
            goto exit;

        // Prefix.
        cbKeyLen = MAX_PATH;
        if (dwError = roContainer[i]->GetValue(CACHE_PREFIX_VALUE, 
            (LPBYTE) Info[i].szCachePrefix, &cbKeyLen) != ERROR_SUCCESS)
            goto exit;

        // Limit.
        if (dwError = roContainer[i]->GetValue(CACHE_LIMIT_VALUE, 
            &cbKBLimit) != ERROR_SUCCESS)
            goto exit;    
        Info[i].cbCacheLimit = ((LONGLONG) cbKBLimit) * 1024;
    }

exit:    
    delete roCache;
    for (i = CONTENT; i < MAX_CONTAINERS; i++)
        delete roContainer[i];

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::SetHKCUCacheConfigInfo()
  ----------------------------------------------------------------------------*/
DWORD CContainerManager::SetHKCUCacheConfigInfo()
{
    DWORD dwError, cbKBLimit;
    ContainerIndex i;

    REGISTRY_OBJ *roHKCU = 0, *roCache = 0, 
        *roContainer[N_CONTAINERS] = {0, 0, 0};

    // Create base HKCU object.
    if (dwError = ConstructRegObj(roHKCU, HKEY_CURRENT_USER, NULL) != ERROR_SUCCESS)
        goto exit;
    
    // Create Cache object.
    if (dwError = roHKCU->Create(CACHE_KEY, &roCache) != ERROR_SUCCESS)
        goto exit;
            
    // Set cache paths, prefixes and default limits.
    for (i = CONTENT; i < MAX_CONTAINERS; i++)
    {
        // Create registry object and entry.
        if (dwError = roCache->Create(szSubKey[i], &roContainer[i]) != ERROR_SUCCESS)
            goto exit;

        // Path.
        if (dwError = roContainer[i]->SetValue(CACHE_PATH_VALUE, 
            Info[i].szCachePath, REG_SZ) != ERROR_SUCCESS)
            goto exit;

        // Prefix.
        if (dwError = roContainer[i]->SetValue(CACHE_PREFIX_VALUE, 
            Info[i].szCachePrefix, REG_SZ) != ERROR_SUCCESS)
            goto exit;

        // Limit.
        cbKBLimit = (DWORD) (Info[i].cbCacheLimit / 1024);
        if (dwError = roContainer[i]->SetValue(CACHE_LIMIT_VALUE, 
            &cbKBLimit) != ERROR_SUCCESS)
            goto exit;    
    }
        
    dwError = ERROR_SUCCESS;

exit:
    delete roHKCU;
    delete roCache;

    for (i = CONTENT; i < MAX_CONTAINERS; i++)
        delete roContainer[i];

    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetDefaultCacheConfigInfo
  ----------------------------------------------------------------------------*/
DWORD CContainerManager::GetDefaultCacheConfigInfo()
{
    DWORD cb, dwError = ERROR_SUCCESS;
    CHAR szWindowsPath[MAX_PATH];
    ContainerIndex idx;     

    // Get the Windows directory.
    cb = GetWindowsDirectory(szWindowsPath, MAX_PATH);
    if (!cb)
    {
        // Failed to get Windows directory.
        dwError = ERROR_PATH_NOT_FOUND;
        goto exit;
    }
    if (szWindowsPath[cb-1] != '\\')
    {
        szWindowsPath[cb] = '\\';
        szWindowsPath[++cb] = '\0';
    }

    for (idx = CONTENT; idx < MAX_CONTAINERS; idx++)
    {
        // Get cache paths out of dll resource and form absolute
        // paths to top level cache directories.
        strcpy(Info[idx].szCachePath, szWindowsPath);    
        if (!LoadString(GlobalCacheDllHandle, dwCachePathResourceID[idx], 
            Info[idx].szCachePath + cb, MAX_PATH - cb))
        {
            dwError = GetLastError();
            goto exit;
        }

        // Cache prefix.
        strcpy(Info[idx].szCachePrefix, szCachePrefix[idx]);

        // Cache limit.
        // BUGBUG - setting all cache limits to DEF_CACHE_LIMIT.
        Info[idx].cbCacheLimit = DEF_CACHE_LIMIT * 1024; // change to % of disk space
    }
    
exit:
    return dwError;
}



/*-----------------------------------------------------------------------------
ContainerIndex CContainerManager::FindIndexFromPrefix
----------------------------------------------------------------------------*/
CContainerManager::ContainerIndex CContainerManager::FindIndexFromPrefix(LPCSTR szUrl)
{
    // Unless we find a matching prefix, CONTENT is the default.
    ContainerIndex eMatchingIndex = CONTENT;
    for (ContainerIndex i = COOKIE; i < MAX_CONTAINERS; i++)
    {
        CHAR* szPrefix = Info[i].szCachePrefix;
        if (szPrefix)
        {
            DWORD cbLen = strlen(Info[i].szCachePrefix);
            // For content container, strnicmp (szUrl, "", 0) returns nonzero
            if (!strnicmp(szUrl, szPrefix, cbLen))
            {
                // Found a match
                eMatchingIndex = i;
                break;
            }
        }
    }
    return eMatchingIndex;
}

/*-----------------------------------------------------------------------------
BOOL CContainerManager::PathPrefixMatch
----------------------------------------------------------------------------*/
BOOL CContainerManager::PathPrefixMatch(LPCSTR szPath, LPCSTR szPathRef)
{
    // BUGBUG - logic borrowed from original cacheapi.cxx
    
    INT len;

    // TRUE if the passed in path is NULL
    if (!szPath)
        return TRUE;

    len = lstrlen(szPath);

    // TRUE if it is 0 length.
    if (!len)
        return TRUE;

    // stripout the trailing slash
    if (szPath[len-1] == '\\')
        --len;

    // Compare paths.
    if (!strnicmp(szPath, szPathRef, len)) 
        if (szPathRef[len] == '\\' || szPathRef[len] == 0)
            return TRUE;

    return FALSE;
}

/*---------------------  Public Functions -----------------------------------*/

/*-----------------------------------------------------------------------------
CContainerManager::CContainerManager

  Default Constructor
  ----------------------------------------------------------------------------*/
CContainerManager::CContainerManager() : nContainers(N_CONTAINERS)
{
    dwStatus = Init();
}


/*-----------------------------------------------------------------------------
CContainerManager::~CContainerManager

  Default Destructor
  ----------------------------------------------------------------------------*/
CContainerManager::~CContainerManager()
{
    for (ContainerIndex idx = CONTENT; idx < MAX_CONTAINERS; idx++)
        delete ContainerArray[idx];
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetStatus()
----------------------------------------------------------------------------*/
DWORD CContainerManager::GetStatus()
{
    return dwStatus;
}

/*-----------------------------------------------------------------------------
BOOL CContainerManager::DeleteAPendingUrl()
----------------------------------------------------------------------------*/
BOOL CContainerManager::DeleteAPendingUrl()
{    
    BOOL fReturn = FALSE;
    // Return TRUE if any of the containers deleted a pending URL.
    for (ContainerIndex idx = CONTENT; idx < MAX_CONTAINERS; idx++)
        fReturn = fReturn || ContainerArray[idx]->DeleteAPendingUrl();
    return fReturn;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::UnlockUrl
----------------------------------------------------------------------------*/
DWORD CContainerManager::UnlockUrl(LPCSTR szUrl)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call UnlockUrl on the appropriate container.
    DWORD dwError = ContainerArray[idx]->UnlockUrl(szUrl);
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::DeleteUrl
----------------------------------------------------------------------------*/
DWORD CContainerManager::DeleteUrl(LPCSTR szUrl)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call DeleteUrl on the appropriate container.
    DWORD dwError = ContainerArray[idx]->DeleteUrl(szUrl);
    return dwError;
}

/*-----------------------------------------------------------------------------
BOOL CContainerManager::GetHeaderFlags
----------------------------------------------------------------------------*/
BOOL CContainerManager::GetHeaderFlags(LPDWORD pdwVer)
{
    // Settings same for all containers; use CONTENT.
    return ContainerArray[CONTENT]->GetHeaderFlags(pdwVer);
}

/*-----------------------------------------------------------------------------
BOOL CContainerManager::IncrementHeaderFlags
----------------------------------------------------------------------------*/
BOOL CContainerManager::IncrementHeaderFlags(LPDWORD pdwVer)
{
    // Settings same for all containers; use CONTENT.
    return ContainerArray[CONTENT]->IncrementHeaderFlags(pdwVer);
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::SetUrlInGroup
----------------------------------------------------------------------------*/
DWORD CContainerManager::SetUrlInGroup(
    IN LPCSTR   szUrl,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN FILETIME ftExempt
    )
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call SetUrlInGroup on the appropriate container.
    DWORD dwError = ContainerArray[idx]->SetUrlInGroup(szUrl, dwFlags, 
        GroupId, ftExempt);

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::CreateUniqueFile
----------------------------------------------------------------------------*/
DWORD CContainerManager::CreateUniqueFile(LPCSTR szUrl, DWORD dwExpectedSize, 
                                       LPCSTR szFileExtension, LPTSTR szFileName)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call CreateUniqueFile on the appropriate container.
    DWORD dwError = ContainerArray[idx]->CreateUniqueFile(szUrl, dwExpectedSize,
        szFileExtension, szFileName);

    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CContainerManager::AddUrl
----------------------------------------------------------------------------*/
DWORD CContainerManager::AddUrl(
    LPCSTR   szUrl,
    LPCSTR   szLocalFileName,
    FILETIME ExpireTime,
    FILETIME LastModifiedTime,
    DWORD    CacheEntryType,
    LPBYTE   pHeaderInfo,
    DWORD    cbHeaderSize,
    LPCSTR   szFileExtension
    )
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call AddUrl on the appropriate container.
    DWORD dwError = ContainerArray[idx]->AddUrl(szUrl, 
                                                szLocalFileName, 
                                                *((LONGLONG *) &ExpireTime),
                                                *((LONGLONG *) &LastModifiedTime),
                                                CacheEntryType,
                                                pHeaderInfo,
                                                cbHeaderSize,
                                                szFileExtension);        
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::RetrieveUrl
----------------------------------------------------------------------------*/
DWORD CContainerManager::RetrieveUrl(LPCSTR               szUrl,
                                    LPCACHE_ENTRY_INFOA  pCacheEntryInfo,
                                    LPDWORD              pcbCacheEntryInfoBufferSize)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call RetrieveUrl on the appropriate container.
    DWORD dwError = ContainerArray[idx]->RetrieveUrl(szUrl, 
                                                     pCacheEntryInfo, 
                                                     pcbCacheEntryInfoBufferSize);

    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetUrlInfo
----------------------------------------------------------------------------*/
DWORD CContainerManager::GetUrlInfo(LPCSTR               szUrl,
                                   LPCACHE_ENTRY_INFOA  pCacheEntryInfo,
                                   LPDWORD              pcbCacheEntryInfoBufferSize)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call GetUrlInfo on the appropriate container.
    DWORD dwError = ContainerArray[idx]->GetUrlInfo(szUrl, 
                                                    pCacheEntryInfo, 
                                                    pcbCacheEntryInfoBufferSize);
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CContainerManager::SetUrlInfo
----------------------------------------------------------------------------*/
DWORD CContainerManager::SetUrlInfo(LPCSTR               szUrl,
                                   LPCACHE_ENTRY_INFOA  pCacheEntryInfo,
                                   DWORD                dwFieldControl)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call SetUrlInfo on the appropriate container.
    DWORD dwError = ContainerArray[idx]->SetUrlInfo(szUrl, 
                                                    pCacheEntryInfo, 
                                                    dwFieldControl);

    return dwError;
}

// BUGBUG - DON'T DO THIS SHIT.
#define FIND_HANDLE_NONE 1 

/*-----------------------------------------------------------------------------
HANDLE CContainerManager::FindFirstEntry
----------------------------------------------------------------------------*/
HANDLE CContainerManager::FindFirstEntry(
    LPCSTR              szUrlSearchPattern,
    LPCACHE_ENTRY_INFOA pFirstCacheEntryInfo,
    LPDWORD             pdwFirstCacheEntryInfoBufferSize
    )
{
    // BUGBUG - this logic is borrowed from the original cachapia.cxx.
    
    DWORD                     dwError;
    LPCACHE_FIND_FIRST_HANDLE pFind = NULL;
    HANDLE                    hFind = 0;

    // Allocate a handle.
    LOCK_CACHE();
    hFind = HandleMgr.Alloc (sizeof(CACHE_FIND_FIRST_HANDLE));
    if (hFind)
        pFind = (CACHE_FIND_FIRST_HANDLE*) HandleMgr.Map (hFind);
    UNLOCK_CACHE();

    if (!hFind)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pFind->ContainerIndex = 0;
    pFind->ContainerIndexSpecific = NO_SPECIAL_CONTAINER;
    pFind->InternelHandle = FIND_HANDLE_NONE;

    if (szUrlSearchPattern)
    {
        // Find the associated container.
        ContainerIndex idx = FindIndexFromPrefix(szUrlSearchPattern);

        // Store off the container index in the handle.
        if (idx != CONTENT)
            pFind->ContainerIndexSpecific = (DWORD) idx;
    }

    // Start the enumeration.
    if (FindNextEntry(hFind, pFirstCacheEntryInfo, pdwFirstCacheEntryInfoBufferSize))
        dwError = ERROR_SUCCESS;
    else
        dwError = GetLastError();

exit:

    if( dwError != ERROR_SUCCESS )
    {
        if (hFind)
        {
            LOCK_CACHE();
            HandleMgr.Free (hFind);
            UNLOCK_CACHE();
        }
        SetLastError(dwError);
        return NULL;
    }

    INET_ASSERT(hFind != 0);
    INET_ASSERT(pFind != NULL);
    return hFind;
}

/*-----------------------------------------------------------------------------
BOOL CContainerManager::FindNextEntry
----------------------------------------------------------------------------*/
BOOL CContainerManager::FindNextEntry(HANDLE hFind, LPCACHE_ENTRY_INFOA pNextCacheEntryInfo, 
                                              LPDWORD pdwNextCacheEntryInfoBufferSize)
{
    // BUGBUG - this logic is borrowed from the original cachapia.cxx.
    
    DWORD                    dwError;
    CACHE_FIND_FIRST_HANDLE* pFind;

    // Map and validate the handle.
    LOCK_CACHE();
    pFind = (CACHE_FIND_FIRST_HANDLE*) HandleMgr.Map (hFind);
    UNLOCK_CACHE();
    if (!pFind)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // Continue the enumeration.
    dwError = ERROR_NO_MORE_ITEMS;

    if ((pFind->ContainerIndexSpecific != NO_SPECIAL_CONTAINER))
    {
        // Find the associated container.
        ContainerIndex idx = (ContainerIndex) pFind->ContainerIndexSpecific;
     
        if (pFind->InternelHandle == FIND_HANDLE_NONE)
            pFind->InternelHandle = ContainerArray[idx]->GetInitialFindHandle();
        
        dwError = ContainerArray[idx]->FindNextEntry(&pFind->InternelHandle,
                                                pNextCacheEntryInfo,
                                                pdwNextCacheEntryInfoBufferSize);
    }
    else
    {
        while (pFind->ContainerIndex < N_CONTAINERS)
        {            
            if (pFind->InternelHandle == FIND_HANDLE_NONE)
                pFind->InternelHandle = ContainerArray[pFind->ContainerIndex]->GetInitialFindHandle();
                    
            dwError = ContainerArray[pFind->ContainerIndex]->FindNextEntry(&pFind->InternelHandle,
                                                                           pNextCacheEntryInfo,
                                                                           pdwNextCacheEntryInfoBufferSize);
            if (dwError == ERROR_SUCCESS)
                break;

            if (dwError != ERROR_NO_MORE_ITEMS)
                goto exit;

            // Go to next container and find first.
            pFind->ContainerIndex++;
            pFind->InternelHandle = FIND_HANDLE_NONE;
        }
    }

exit:

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::CleanupUrls
----------------------------------------------------------------------------*/
DWORD CContainerManager::CleanupUrls(LPCTSTR szCachePath, DWORD dwFactor)
{
    DWORD dwError;

    // Bad cleanup parameter.
    if (dwFactor <= 0 || dwFactor > 100)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    
    // Null path - clean up all containers.
    if (!szCachePath || ! *szCachePath)
        for (ContainerIndex idx = CONTENT; idx < MAX_CONTAINERS; idx++)
            ContainerArray[idx]->CleanupUrls(dwFactor, DELETE_FORCE_MINIMUM, FALSE);
    else
    {
        // Find the container with the matching cache path and clean it up.
        for (ContainerIndex idx = CONTENT; idx < MAX_CONTAINERS; idx++)
        {
            if (PathPrefixMatch(szCachePath, Info[idx].szCachePath))
            {
                ContainerArray[idx]->CleanupUrls(dwFactor, DELETE_FORCE_MINIMUM, FALSE);
                break;
            }
        }
    }

    dwError = ERROR_SUCCESS;

exit:
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CContainerManager::GetUrlInGroup
----------------------------------------------------------------------------*/
DWORD CContainerManager::GetUrlInGroup(LPCSTR szUrl, GROUPID* pGroupId, FILETIME* pftExempt)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szUrl);

    // Call GetUrlInGroup on the appropriate container.
    DWORD dwError = ContainerArray[idx]->GetUrlInGroup(szUrl, pGroupId, pftExempt);
                                         
                                         
exit:

    return dwError;
}

/*-----------------------------------------------------------------------------
VOID CContainerManager::SetCacheLimit
----------------------------------------------------------------------------*/
VOID CContainerManager::SetCacheLimit(LONGLONG cbLimit, ContainerIndex idx)
{
    // Update the container manager value
    Info[idx].cbCacheLimit = cbLimit;

    // Call SetCacheLimit on the container.
    ContainerArray[idx]->SetCacheLimit((DWORD) cbLimit);
                                                                                  
}

/*-----------------------------------------------------------------------------
VOID CContainerManager::GetCacheInfo
----------------------------------------------------------------------------*/
VOID CContainerManager::GetCacheInfo(LPCSTR szPrefix, LPSTR szCachePath, LONGLONG *cbLimit)
{
    // Find the associated container.
    ContainerIndex idx = FindIndexFromPrefix(szPrefix);

    // Call GetCacheInfo on the appropriate container.
    ContainerArray[idx]->GetCacheInfo(szCachePath, cbLimit);
                                                                                  
}


/*-----------------------------------------------------------------------------
BOOL CContainerManager::GetUrlCacheConfigInfo
  ----------------------------------------------------------------------------*/
BOOL CContainerManager::GetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO lpCacheConfigInfo,
    LPDWORD lpdwCacheConfigInfoBufferSize, DWORD dwFieldControl)
{
    DWORD i, dwError, cbKeyLen, dwBufferSizeRequired;

    REGISTRY_OBJ *roCache = 0, *roPaths = 0, *roPath[DEF_NUM_PATHS] = {0, 0, 0, 0};
    CHAR szCachePath[MAX_PATH];

    // BUGBUG - original logic from cachecfg.cxx. Not much to it.
    LOCK_CACHE();

    // Calculate required buffer size.
    dwBufferSizeRequired = sizeof(CACHE_CONFIG_INFO);
    if( IsFieldSet( dwFieldControl, CACHE_CONFIG_DISK_CACHE_PATHS_FC ))
    {
        dwBufferSizeRequired += sizeof(CACHE_CONFIG_PATH_ENTRY) * (DEF_NUM_PATHS - 1);
    }

    // Check output buffer size.
    if( *lpdwCacheConfigInfoBufferSize < dwBufferSizeRequired )
    {
        *lpdwCacheConfigInfoBufferSize = dwBufferSizeRequired;
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }

    if( IsFieldSet( dwFieldControl, CACHE_CONFIG_SYNC_MODE_FC ))
        lpCacheConfigInfo->dwSyncMode = dwSyncMode;

    if ( IsFieldSet( dwFieldControl, CACHE_CONFIG_DISK_CACHE_PATHS_FC ))
    {
        // Create Cache registry object.
        if (dwError = ConstructRegObj(roCache, HKEY_LOCAL_MACHINE, 
            CACHE_KEY) != ERROR_SUCCESS)
            goto exit;

        // Create Paths registry object.
        if (dwError = ConstructRegObj(roPaths, roCache,
            CACHE_PATHS_KEY) != ERROR_SUCCESS)
            goto exit;
                
        // Path1, Path2, Path3, Path4.
        for (i = 0; i < DEF_NUM_PATHS; i++)
        {
            // Create the subdirectory registry object.
            if (dwError = ConstructRegObj(roPath[i], roPaths, 
                (LPSTR) rglpDefCacheSubKeys[i]) != ERROR_SUCCESS)
                goto exit;

            // Get the cache path.
            cbKeyLen = MAX_PATH;
            if (dwError = roPath[i]->GetValue(CACHE_PATH_VALUE, 
                (LPBYTE) szCachePath, &cbKeyLen) != ERROR_SUCCESS)
                goto exit;
            
            // Copy the cache path into the structure.
            lstrcpy(lpCacheConfigInfo->CachePaths[i].CachePath, 
                szCachePath);

            // Copy the CONTENT cache limit into each sub-cache limit
            // (divide by number of paths and converted from bytes
            // to kilobytes
            lpCacheConfigInfo->CachePaths[i].dwCacheSize = (DWORD)
               (Info[CONTENT].cbCacheLimit / (DEF_NUM_PATHS * 1024));

        }    
        lpCacheConfigInfo->dwNumCachePaths = (DWORD) DEF_NUM_PATHS;
    }

    dwError = ERROR_SUCCESS;

exit:

    UNLOCK_CACHE();
    if( dwError != ERROR_SUCCESS )
    {
        SetLastError (dwError);
        return FALSE;
    }

    return TRUE;

}


/*-----------------------------------------------------------------------------
BOOL CContainerManager::SetUrlCacheConfigInfo
  ----------------------------------------------------------------------------*/
BOOL CContainerManager::SetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO pConfig, 
                                              DWORD dwFieldControl)
{         
    DWORD i, dwError = ERROR_SUCCESS;

    LOCK_CACHE();
    
    
    //  Check FieldControl bits and set the values for set fields
    if( IsFieldSet( dwFieldControl, CACHE_CONFIG_SYNC_MODE_FC ))
    {

        INET_ASSERT((pConfig->dwSyncMode >= WININET_SYNC_MODE_NEVER)
                        &&(pConfig->dwSyncMode <= WININET_SYNC_MODE_ALWAYS));

        InternetWriteRegistryDword(vszSyncMode, pConfig->dwSyncMode);

        dwSyncMode = pConfig->dwSyncMode;

        // set a new version and simultaneously
        // increment copy for this process, so we don't
        // read registry for this process
        IncrementHeaderFlags(&GlobalSettingsVersion);
    }

    if ( IsFieldSet( dwFieldControl, CACHE_CONFIG_DISK_CACHE_PATHS_FC ))
    {
        // Add up the quotas.
        LONGLONG CacheLimit = 0;

        for (i = 0; i < pConfig->dwNumCachePaths; i++)
        {
            DWORD dwCacheSize = pConfig->CachePaths[i].dwCacheSize;
            CacheLimit += (LONGLONG) dwCacheSize * 1024;
        }

        // Update the CONTENT cache limit.
        SetCacheLimit(CacheLimit, CONTENT);

        // Update the HKLM and HKCU registry settings.
        // BUGBUG - this will have to change with per-user.
        SetHKLMCacheConfigInfo();
        SetHKCUCacheConfigInfo();
    
    }

exit:
    
    UNLOCK_CACHE();
    
    return (dwError == ERROR_SUCCESS ? TRUE : FALSE);
}

