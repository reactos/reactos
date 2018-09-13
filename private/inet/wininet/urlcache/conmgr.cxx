/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  conmgr.cxx

Abstract:

    Manages list of containers (CConList)

Author:
    Adriaan Canter (adriaanc) 04-02-97

--*/

#include <cache.hxx>

#define FAILSAFE_TIMEOUT (60000)

/*---------------------  Private Functions -----------------------------------*/
/*-----------------------------------------------------------------------------
DWORD CConMgr::Init
----------------------------------------------------------------------------*/
DWORD CConMgr::Init()
{
    DWORD dwError = ConfigureCache();

    if (dwError==ERROR_SUCCESS)
    {
        // Get the extensible cache config info.
        // These containers are delay-initialized.
        GetExtensibleCacheConfigInfo(TRUE);
    }
    else
    {
        INET_ASSERT(FALSE);
    }
    return dwError;

}

#ifdef CHECKLOCK_PARANOID
void CConMgr::CheckNoLocks(DWORD dwThreadId)
{
    URL_CONTAINER *co;
    DWORD idx;

    LOCK_CACHE();
    for (idx = 0; idx < ConList.Size(); idx++)
    {
        URL_CONTAINER *co;

        co = ConList.Get(idx);
        if (co)
        {
            co->CheckNoLocks(dwThreadId);
            co->Release(FALSE);
        }
    }
    UNLOCK_CACHE();
}
#endif

/*-----------------------------------------------------------------------------
BOOL CConMgr::WasModified
----------------------------------------------------------------------------*/
BOOL CConMgr::WasModified(BOOL fUpdateMemory)
{
    DWORD dwOldCount = _dwModifiedCount;
    return dwOldCount != ReadModifiedCount(fUpdateMemory);
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::ReadModifiedCount
----------------------------------------------------------------------------*/
DWORD CConMgr::ReadModifiedCount(BOOL fUpdateMemory)
{
    DWORD dwChangeCount;
    DWORD *pdwChangeCount = fUpdateMemory ? &_dwModifiedCount : &dwChangeCount;

    _coContent->GetHeaderData(CACHE_HEADER_DATA_CONLIST_CHANGE_COUNT,
                             pdwChangeCount);
    return *pdwChangeCount;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::IncrementModifiedCount
----------------------------------------------------------------------------*/
void CConMgr::IncrementModifiedCount()
{
    DWORD dwLocModified;

    _coContent->IncrementHeaderData(CACHE_HEADER_DATA_CONLIST_CHANGE_COUNT,
                                   &dwLocModified);
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::InitFixedContainers
----------------------------------------------------------------------------*/
DWORD CConMgr::InitFixedContainers()
{
    DWORD idx;
    DWORD dwError = ERROR_SUCCESS;
    BOOL fInitSucceeded = TRUE;

    //  Create and init
    _hMutexExtensible = OpenMutex(SYNCHRONIZE, FALSE, TEXT("_!MSFTHISTORY!_"));
    if (_hMutexExtensible == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
    {
        _hMutexExtensible = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, TEXT("_!MSFTHISTORY!_"));
    }
    if (_hMutexExtensible == NULL)
    {
        dwError = GetLastError();
        fInitSucceeded = FALSE;
        goto exit;
    }
    _dwLastUnmap = GetTickCount();

    LOCK_CACHE();
    // Containers are configured. Attempt to initialize.
    for (idx = CONTENT; idx < ConList.Size(); idx++)
    {
        URL_CONTAINER *co;

        co = ConList.Get(idx);
        if (co)
        {
            dwError = co->Init();

            // NOTE - URL_CONTAINER::Init() returns ERROR_ALREADY_EXISTS
            // only if the the existing memory mapped file has been opened
            // successfully. If the memory mapped file was created, upgraded
            // or corrupted (in both cases the mem mapped file will be reinited)
            // the return value will be ERROR_SUCCESS.

            if(dwError != ERROR_SUCCESS && dwError != ERROR_ALREADY_EXISTS)
            {
                fInitSucceeded = FALSE;
                goto unlock_exit;
            }

            // Has the container been created, upgrade or corrupted?
            if (dwError == ERROR_SUCCESS)
            {
                if(idx==CONTENT)
                {
                    // Preload the content container.
                    LoadContent();
                }
                else if (idx==COOKIE)
                {
                    CCookieLoader cl;
                    cl.LoadCookies(co);
                }
            }
            co->Release(FALSE);
        }

    }

    // Enable cachevu for CONTENT and HISTORY.
    EnableCacheVu(_coContent->GetCachePath(), CONTENT);
    EnableCacheVu(_coHistory->GetCachePath(), HISTORY);

unlock_exit:

    UNLOCK_CACHE();

exit:
    dwError = (fInitSucceeded ? ERROR_SUCCESS : ERROR_INTERNAL_ERROR);

    INET_ASSERT(dwError == ERROR_SUCCESS);
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::LoadContent()
----------------------------------------------------------------------------*/
DWORD CConMgr::LoadContent()
{
    DWORD cbFile, dwError = ERROR_FILE_NOT_FOUND;
    CHAR  szPreloadKey[MAX_PATH],
          szUrl[MAX_PATH],
          szFile[MAX_PATH];

    // Preload registry key string.
    memcpy(szPreloadKey, OLD_CACHE_KEY, sizeof(OLD_CACHE_KEY) - 1);
    szPreloadKey[sizeof(OLD_CACHE_KEY)-1] = '\\';
    memcpy(szPreloadKey + sizeof(OLD_CACHE_KEY), "Preload", sizeof("PreLoad"));

    // Construct preload registry object.
    REGISTRY_OBJ roPreload(HKEY_CURRENT_USER, szPreloadKey);
    REGISTRY_OBJ roIE5Preload(&roPreload, "IE5Preload");
    if (roPreload.GetStatus() != ERROR_SUCCESS)
        goto exit;

    // Get the storage directory (cdf preload) to compare against to determine if we
    // need to set EDITED_CACHE_ENTRY or not. We assume any preload entry not in the
    // store dir IS and ECE.
    DWORD cb;
    CHAR szStorePath[MAX_PATH];

    // Store dir is hardwired to "%windir%\Web\"
    if ((cb = GetWindowsDirectory(szStorePath, MAX_PATH)))
    {
        AppendSlashIfNecessary(szStorePath, cb);
        memcpy(szStorePath + cb, WEBDIR_STRING, sizeof(WEBDIR_STRING));
        cb += sizeof(WEBDIR_STRING) - 1; //cb now equals size of szStorePath.
    }


    // Enum the registry url/file values and commit them
    // to the cache.
    while (roPreload.FindNextValue(szUrl, MAX_PATH,
        (LPBYTE) szFile, &(cbFile = MAX_PATH)) == ERROR_SUCCESS)
    {
        // Strip off any file:// off the file path/name.
        CHAR* ptr = szFile;

        if (!strnicmp(ptr, "file://", sizeof("file://") - 1))
            ptr += sizeof("file://") - 1;

        AddUrlArg Args;
        memset(&Args, 0, sizeof(Args));
        Args.pszUrl      = szUrl;
        Args.pszFilePath = ptr;

        // If this is a Store entry, set the type to 0 else ECE
        if (!strnicmp(ptr, szStorePath, cb))
            Args.dwEntryType = 0;
        else
            Args.dwEntryType = EDITED_CACHE_ENTRY;

        _coContent->AddUrl(&Args);
    }


    // IE5 preload.
    if (roIE5Preload.GetStatus() != ERROR_SUCCESS)
        goto exit;
        
    DWORD cbMaxUrl, cbMaxEntry, cbEntry;
    LPSTR pszUrl;
    URL_FILEMAP_ENTRY *pEntry;
    KEY_QUERY_INFO QueryInfo;

    if (ERROR_SUCCESS != roIE5Preload.GetKeyInfo(&QueryInfo))
        goto exit;
                
    cbMaxUrl = QueryInfo.MaxValueNameLen + 1;
    cbMaxEntry = QueryInfo.MaxValueLen + 1;
    
    pszUrl = new CHAR[cbMaxUrl];
    pEntry = (URL_FILEMAP_ENTRY*) new CHAR[cbMaxEntry];

    if (!(pszUrl && pEntry))
        goto exit;


    __try
    {
        
        while ((dwError = roIE5Preload.FindNextValue(pszUrl, cbMaxUrl,
            (LPBYTE) pEntry, &(cbEntry = cbMaxEntry))) == ERROR_SUCCESS)
        {
            FILETIME ft;
            AddUrlArg Args;
            memset(&Args, 0, sizeof(Args));

            // Url
            Args.pszUrl      = pEntry->UrlNameOffset ? 
                (LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->UrlNameOffset) : NULL;
    
            // File path
            Args.pszFilePath = pEntry->InternalFileNameOffset ? 
                (LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->InternalFileNameOffset) : NULL;

            // Header info
            Args.pbHeaders = pEntry->HeaderInfoOffset ? 
                (LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->HeaderInfoOffset) : NULL;
            Args.cbHeaders = pEntry->HeaderInfoSize;

            // Last modified.
            Args.qwLastMod = pEntry->LastModifiedTime;

            // Expires time.
            DosTime2FileTime(pEntry->dostExpireTime, &ft);        
            Args.qwExpires = FT2LL(ft);

            // Post check time.
            DosTime2FileTime(pEntry->dostPostCheckTime, &ft);        
            Args.qwPostCheck = FT2LL(ft);

            // File creation time.
            DosTime2FileTime(pEntry->dostFileCreationTime, &ft);        
            Args.ftCreate = ft;
        
            // File extension.
            Args.pszFileExt = pEntry->FileExtensionOffset ? 
                (LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->FileExtensionOffset) : NULL;
 
            // Entry type.
            Args.dwEntryType = pEntry->CacheEntryType;

            // File size
            Args.dwFileSize = pEntry->dwFileSize;

            // Add the url.
            _coContent->AddUrl(&Args);
        }

    } // __try

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        INET_ASSERT(FALSE);
        dwError = GetLastError();
    }
    ENDEXCEPT
     
    dwError = ERROR_SUCCESS;

    if (pszUrl)
        delete pszUrl;
    if (pEntry)
        delete pEntry;
        
exit:
    return dwError;
}


/*-----------------------------------------------------------------------------
HANDLE CConMgr::FindFirstContainer
----------------------------------------------------------------------------*/
HANDLE CConMgr::FindFirstContainer(DWORD *pdwModified, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize, DWORD dwOptions)
{
    DWORD dwError = ERROR_SUCCESS;
    CONTAINER_FIND_FIRST_HANDLE *pFind;
    DWORD dwContainers = 0;
    DWORD dwNames = 0;
    DWORD dwPrefixes = 0;
    DWORD dwLabels = 0;
    DWORD dwTitles = 0;
    DWORD dwTotal;
    HANDLE hFind = NULL;
    DWORD dwModified;

    GetExtensibleCacheConfigInfo(FALSE);

    LOCK_CACHE();
    dwModified = *pdwModified;
    *pdwModified = _dwModifiedCount;


    if ((CACHE_FIND_CONTAINER_RETURN_NOCHANGE & dwOptions) == 0 ||
        dwModified != *pdwModified)
    {
        for (DWORD i = NCONTAINERS; i < ConList.Size(); i++)
        {
            URL_CONTAINER *co = ConList.Get(i);
            if (co)
            {
                if (co->IsVisible())
                {
                    dwContainers++;
                    dwNames += strlen(co->GetCacheName()) + 1;
                    dwPrefixes += strlen(co->GetCachePrefix()) + 1;
                    dwLabels += strlen(co->GetVolumeLabel()) + 1;
                    dwTitles += strlen(co->GetVolumeTitle()) + 1;
                }
                co->Release(TRUE);
            }
        }

        dwTotal = sizeof(CONTAINER_FIND_FIRST_HANDLE)+
                         dwContainers*(4 * sizeof(LPSTR)) +
                         (dwNames+dwPrefixes+dwLabels+dwTitles) * sizeof(char);

        hFind = HandleMgr.Alloc (dwTotal);
        if (hFind)
        {
            LPSTR ps;

            pFind = (CONTAINER_FIND_FIRST_HANDLE*) HandleMgr.Map (hFind);
            pFind->dwSignature = SIGNATURE_CONTAINER_FIND;
            pFind->dwContainer = 0;
            pFind->dwNumContainers = dwContainers;
            if (dwContainers)
            {
                pFind->ppNames = (LPTSTR *) (((LPBYTE) pFind) + sizeof(CONTAINER_FIND_FIRST_HANDLE));
                pFind->ppPrefixes = pFind->ppNames + dwContainers;
                pFind->ppLabels = pFind->ppPrefixes + dwContainers;
                pFind->ppTitles = pFind->ppLabels + dwContainers;
                ps = (LPSTR) (((LPBYTE) pFind) +
                                sizeof(CONTAINER_FIND_FIRST_HANDLE)+
                                dwContainers*(4 * sizeof(LPSTR)));
                dwContainers = 0;

                for (DWORD i = NCONTAINERS; i < ConList.Size(); i++)
                {
                    URL_CONTAINER *co = ConList.Get(i);
                    if (co)
                    {
                        if (co->IsVisible())
                        {
                            pFind->ppNames[dwContainers] = ps;
                            strcpy(ps, co->GetCacheName());
                            ps += strlen(co->GetCacheName()) + 1;
                            pFind->ppPrefixes[dwContainers] = ps;
                            strcpy(ps, co->GetCachePrefix());
                            ps += strlen(co->GetCachePrefix()) + 1;
                            pFind->ppLabels[dwContainers] = ps;
                            strcpy(ps, co->GetVolumeLabel());
                            ps += strlen(co->GetVolumeLabel()) + 1;
                            pFind->ppTitles[dwContainers] = ps;
                            strcpy(ps, co->GetVolumeTitle());
                            ps += strlen(co->GetVolumeTitle()) + 1;

                            dwContainers++;
                        }
                        co->Release(TRUE);
                    }
                }

            }
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        dwError = ERROR_INTERNET_NO_NEW_CONTAINERS;
    }
    UNLOCK_CACHE();

    if (hFind)
    {
        if (FindNextContainer(hFind, lpContainerInfo, lpdwContainerInfoBufferSize))
            dwError = ERROR_SUCCESS;
        else
            dwError = GetLastError();
    }
    if( dwError != ERROR_SUCCESS )
    {
        FreeFindHandle(hFind);
        SetLastError(dwError);
        return NULL;
    }
    return hFind;
}


/*-----------------------------------------------------------------------------
BOOL CConMgr::FindNextContainer
----------------------------------------------------------------------------*/
BOOL CConMgr::FindNextContainer(HANDLE hFind, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize)
{
    // BUGBUG - this logic is borrowed from the original cachapia.cxx.

    DWORD                    dwError;
    CONTAINER_FIND_FIRST_HANDLE* pFind;

    // Map and validate the handle.
    LOCK_CACHE();
    pFind = (CONTAINER_FIND_FIRST_HANDLE*) HandleMgr.Map (hFind);
    UNLOCK_CACHE();
    if (!pFind || pFind->dwSignature != SIGNATURE_CONTAINER_FIND ||
        !lpContainerInfo ||
        *lpdwContainerInfoBufferSize < sizeof(INTERNET_CACHE_CONTAINER_INFOA))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // Continue the enumeration.
    if (pFind->dwContainer < pFind->dwNumContainers)
    {
        DWORD cbName = strlen(pFind->ppNames[pFind->dwContainer])+1;
        DWORD cbPrefix = strlen(pFind->ppPrefixes[pFind->dwContainer])+1;
        DWORD cbLabel = strlen(pFind->ppLabels[pFind->dwContainer])+1;
        DWORD cbTitle = strlen(pFind->ppTitles[pFind->dwContainer])+1;

        DWORD cbTotal = cbName+cbPrefix+cbLabel+cbTitle+sizeof(INTERNET_CACHE_CONTAINER_INFOA);
        if (cbTotal > *lpdwContainerInfoBufferSize)
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            lpContainerInfo->lpszName = (LPSTR) (((LPBYTE) lpContainerInfo) +
                sizeof(INTERNET_CACHE_CONTAINER_INFOA));
            lpContainerInfo->lpszCachePrefix = lpContainerInfo->lpszName + cbName;
            lpContainerInfo->lpszVolumeLabel = lpContainerInfo->lpszCachePrefix + cbPrefix;
            lpContainerInfo->lpszVolumeTitle = lpContainerInfo->lpszVolumeLabel + cbLabel;

            strcpy(lpContainerInfo->lpszName, pFind->ppNames[pFind->dwContainer]);
            strcpy(lpContainerInfo->lpszCachePrefix, pFind->ppPrefixes[pFind->dwContainer]);
            strcpy(lpContainerInfo->lpszVolumeLabel, pFind->ppLabels[pFind->dwContainer]);
            strcpy(lpContainerInfo->lpszVolumeTitle, pFind->ppTitles[pFind->dwContainer]);
            lpContainerInfo->dwCacheVersion = URL_CACHE_VERSION_NUM;
            pFind->dwContainer++;
            dwError = ERROR_SUCCESS;
        }
        *lpdwContainerInfoBufferSize = cbTotal;
    }
    else
    {
        dwError = ERROR_NO_MORE_ITEMS;
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
DWORD CConMgr::GetContainerInfo
----------------------------------------------------------------------------*/
DWORD CConMgr::GetContainerInfo(LPSTR szUrl,
                               LPINTERNET_CACHE_CONTAINER_INFOA pCI,
                               LPDWORD pcbCI)
{
    URL_CONTAINER *co;
    DWORD dwError;

    // Find the associated container.
    DWORD idx;

    LOCK_CACHE();

    idx = FindIndexFromPrefix(szUrl);
    co = ConList.Get(idx);

    if (co)
    {
        DWORD cbName = strlen(co->GetCacheName()) + 1;
        DWORD cbPrefix = strlen(co->GetCachePrefix()) + 1;
        DWORD cbLabel = strlen(co->GetVolumeLabel()) + 1;
        DWORD cbTitle = strlen(co->GetVolumeTitle()) + 1;
        DWORD cbReq = cbName + cbPrefix + cbLabel + cbTitle;
        if (cbReq > *pcbCI)
        {
            *pcbCI = cbReq;
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pCI->lpszName = (LPSTR) (((LPBYTE) pCI) +
                sizeof(INTERNET_CACHE_CONTAINER_INFOA));

            pCI->lpszCachePrefix = pCI->lpszName + cbName;
            pCI->lpszVolumeLabel = pCI->lpszName + cbName + cbPrefix;
            pCI->lpszVolumeTitle = pCI->lpszName + cbName + cbPrefix + cbLabel;

            memcpy(pCI->lpszName, co->GetCacheName(), cbName);
            memcpy(pCI->lpszCachePrefix, co->GetCachePrefix(), cbPrefix);
            memcpy(pCI->lpszVolumeLabel, co->GetVolumeLabel(), cbLabel);
            memcpy(pCI->lpszVolumeTitle, co->GetVolumeTitle(), cbTitle);
            pCI->dwCacheVersion = URL_CACHE_VERSION_NUM;

            *pcbCI = cbReq;
            dwError = ERROR_SUCCESS;
        }

        co->Release(TRUE);
    }
    else
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
    }

    UNLOCK_CACHE();

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::CreateContainer
----------------------------------------------------------------------------*/
DWORD CConMgr::CreateContainer(LPCSTR Name, LPCSTR CachePrefix, LPCSTR CachePath, DWORD KBCacheLimit, DWORD dwOptions)
{
    BOOL fInsertOk = TRUE;
    DWORD dwWaitResult = ERROR_TIMEOUT;
    DWORD dwError;
    CHAR szVendorKey[MAX_PATH];
    CHAR szDefaultPath[MAX_PATH];
    CHAR szCachePath[MAX_PATH];
    LONGLONG CacheStartUpLimit;
    HKEY hKey;
    DWORD cbKeyLen;

    hKey = (_fProfilesCapable ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
    REGISTRY_OBJ roCache(hKey, CACHE5_KEY);

    if (!Name || !*Name)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    if (!CachePath || !*CachePath)
    {
        LPSTR p = _coHistory->GetCachePath();
        int len = _coHistory->GetCachePathLen();
        int clen = lstrlen(Name);

        if (len + clen + sizeof(DIR_SEPARATOR_STRING) > sizeof(szDefaultPath)) {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
        memcpy(szDefaultPath, p, len);
        memcpy(&szDefaultPath[len], Name, clen);
        memcpy(&szDefaultPath[len + clen], DIR_SEPARATOR_STRING, sizeof(DIR_SEPARATOR_STRING));
        CachePath = szDefaultPath;
    }

    if (KBCacheLimit == 0)
    {
        CacheStartUpLimit = _coHistory->GetCacheStartUpLimit();
        KBCacheLimit = (DWORD) (CacheStartUpLimit / 1024);
    }


    if (!CachePrefix || !*CachePrefix || !CachePath || !*CachePath)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if ((dwError = roCache.GetStatus())==ERROR_SUCCESS)
    {
        // Create registry object and entry.
        REGISTRY_OBJ roExtensibleCache(&roCache, EXTENSIBLE_CACHE_PATH_KEY, CREATE_KEY_IF_NOT_EXISTS);
        if ((dwError = roExtensibleCache.GetStatus()) != ERROR_SUCCESS)
            goto exit;

        dwWaitResult = WaitForSingleObject(_hMutexExtensible, FAILSAFE_TIMEOUT);

        // Get the container paths, prefixes (if any) and default limit values.
        while (roExtensibleCache.FindNextKey(szVendorKey, MAX_PATH) == ERROR_SUCCESS)
        {
            REGISTRY_OBJ roVendor(&roExtensibleCache, szVendorKey);
            if (roVendor.GetStatus()==ERROR_SUCCESS)
            {
                // Path.
                cbKeyLen = MAX_PATH;
                if (roVendor.GetValue(CACHE_PATH_VALUE,(LPBYTE) szCachePath,&cbKeyLen) != ERROR_SUCCESS)
                    continue;

                if (!stricmp(szVendorKey, Name) || !stricmp(CachePath, szCachePath))
                {
                    fInsertOk = FALSE;
                    break;
                }
            }
        }   

        if (fInsertOk)
        {
            REGISTRY_OBJ roNewVendor(&roExtensibleCache, (LPSTR)Name, CREATE_KEY_IF_NOT_EXISTS);
            if (roNewVendor.GetStatus() == ERROR_SUCCESS)
            {
                    // Path.
                if ((dwError = roNewVendor.SetValue(CACHE_PATH_VALUE, (LPSTR)CachePath, REG_SZ)) != ERROR_SUCCESS)
                    goto exit;

                    // Prefix.
                if ((dwError = roNewVendor.SetValue(CACHE_PREFIX_VALUE, (LPSTR)CachePrefix, REG_SZ)) != ERROR_SUCCESS)
                    goto exit;

                    // Limit.
                if ((dwError = roNewVendor.SetValue(CACHE_LIMIT_VALUE, &KBCacheLimit)) != ERROR_SUCCESS)
                        goto exit;

                    // Options.
                if ((dwError = roNewVendor.SetValue(CACHE_OPTIONS_VALUE, &dwOptions)) != ERROR_SUCCESS)
                    goto exit;
            }
        }
        else
        {
            dwError = ERROR_ALREADY_EXISTS;
        }
    }

    if (dwWaitResult == WAIT_OBJECT_0)
    {
        ReleaseMutex(_hMutexExtensible);
        dwWaitResult = ERROR_TIMEOUT;
    }
    if (fInsertOk)
    {
        IncrementModifiedCount();
    }

exit:
    if (dwWaitResult == WAIT_OBJECT_0)
    {
        ReleaseMutex(_hMutexExtensible);
    }
    
    if (fInsertOk)
    {
        GetExtensibleCacheConfigInfo(TRUE);
    }
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::FindExtensibleContainer
----------------------------------------------------------------------------*/
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SECTION
DWORD CConMgr::FindExtensibleContainer(LPCSTR Name)
{
    DWORD n = NOT_AN_INDEX;
    DWORD i;
    URL_CONTAINER *co;

    for (i = NCONTAINERS; i < ConList.Size(); i++)
    {
        co = ConList.Get(i);
        if (co)
        {
            if (!stricmp(Name, co->GetCacheName()) && co->IsVisible())
            {
                // Found a match
                n = i;
                co->Release(FALSE);
                break;
            }
            co->Release(FALSE);
        }
    }

    return n;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::DeleteContainer
----------------------------------------------------------------------------*/
DWORD CConMgr::DeleteContainer(LPCSTR Name, DWORD dwOptions)
{
    DWORD dwWaitResult = ERROR_TIMEOUT;
    DWORD dwError = ERROR_SUCCESS;
    URL_CONTAINER *co = NULL;
    DWORD n = NOT_AN_INDEX;
    HKEY hKey;

    if (!Name || !*Name)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    LOCK_CACHE();
    n = FindExtensibleContainer(Name);
    if (n != NOT_AN_INDEX)
    {
        co = ConList.Get(n);

        if (co)
        {
            co->SetDeletePending(TRUE);
            //  Don't release here, hold it pending until we've updated registry
        }
    }
    UNLOCK_CACHE();

    if (n!= NOT_AN_INDEX)
    {
        hKey = (_fProfilesCapable ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);

        REGISTRY_OBJ roCache(hKey, CACHE5_KEY);

        if ((dwError = roCache.GetStatus()) == ERROR_SUCCESS)
        {
            REGISTRY_OBJ roExtensibleCache(&roCache, EXTENSIBLE_CACHE_PATH_KEY);
            if ((dwError = roExtensibleCache.GetStatus()) != ERROR_SUCCESS)
                goto exit;
        
            dwWaitResult = WaitForSingleObject(_hMutexExtensible, FAILSAFE_TIMEOUT);
            dwError = roExtensibleCache.DeleteKey((LPSTR)Name);
            if (dwWaitResult == WAIT_OBJECT_0)
            {   
                ReleaseMutex(_hMutexExtensible);
            }
            IncrementModifiedCount();
        }
    }

exit:
    LOCK_CACHE();
    SAFERELEASE(co, TRUE);
    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::DeleteFileIfNotRegistered
----------------------------------------------------------------------------*/
BOOL CConMgr::DeleteFileIfNotRegistered(URL_CONTAINER *coDelete)
{
    BOOL fDelete = TRUE;
    BOOL fFound = FALSE;
    CHAR szCachePath[MAX_PATH];
    CHAR szCachePrefix[MAX_PATH];
    DWORD dwOptions;
    LONGLONG cbCacheLimit;

    HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
    DWORD cbKeyLen, cbKBLimit, dwError;
    CHAR szVendorKey[MAX_PATH];
    DWORD dwWaitResult = ERROR_TIMEOUT;

    hKey = (_fProfilesCapable ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
    REGISTRY_OBJ roCache(hKey, CACHE5_KEY), roExtensibleCache;

    if (  ((dwError=roCache.GetStatus())!=ERROR_SUCCESS)
        ||
          ((dwError=roExtensibleCache.WorkWith(&roCache, EXTENSIBLE_CACHE_PATH_KEY))!=ERROR_SUCCESS))
    {
        goto exit;
    }

    if (!WasModified(FALSE))
    {
        //  If our internal cache is up to date, it can't
        //  have been deleted unless DeletePending or Deleted
        fFound = !(coDelete->GetDeletePending()||coDelete->GetDeleted());
        if (fFound)
        {
            goto exit;
        }
        //  If not found, need to look at registry to make sure we're not
        //  deleting a path that has been reused
    }

    dwWaitResult = WaitForSingleObject(_hMutexExtensible, FAILSAFE_TIMEOUT);

     // Get the container paths, prefixes.
    while (roExtensibleCache.FindNextKey(szVendorKey, MAX_PATH) == ERROR_SUCCESS)
    {
        BOOL fPathMatch;
        REGISTRY_OBJ roVendor(&roExtensibleCache, szVendorKey);
        if (roVendor.GetStatus()==ERROR_SUCCESS)
        {
            // Path.
            cbKeyLen = MAX_PATH;
            if (roVendor.GetValue(CACHE_PATH_VALUE,(LPBYTE) szCachePath, &cbKeyLen) != ERROR_SUCCESS)
                continue;

            // Prefix.
            cbKeyLen = MAX_PATH;
            if (roVendor.GetValue(CACHE_PREFIX_VALUE,(LPBYTE) szCachePrefix, &cbKeyLen) != ERROR_SUCCESS)
                continue;

            // Options.
            if (roVendor.GetValue(CACHE_OPTIONS_VALUE,&dwOptions) != ERROR_SUCCESS)
                continue;

            fPathMatch = !stricmp(coDelete->GetCachePath(), szCachePath);
            if (!stricmp(coDelete->GetCacheName(), szVendorKey) && fPathMatch &&
                !stricmp(coDelete->GetCachePrefix(), szCachePrefix) &&
                coDelete->GetOptions() != dwOptions)
            {
                fFound = TRUE;
            }
            if (fPathMatch)
                fDelete = FALSE;
        }
    }
    if (fDelete)
    {
        //  This will fail if another process still has the container mapped,
        //  that's ok.  They will check on exit if container needs to be
        //  deleted
        if (coDelete->GetOptions() & INTERNET_CACHE_CONTAINER_AUTODELETE)
        {
            CFileMgr::DeleteCache(coDelete->GetCachePath());
        }
    }
exit:
    if (dwWaitResult == WAIT_OBJECT_0)
    {
        ReleaseMutex(_hMutexExtensible);
    }

    return !fFound;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::FindIndexFromPrefix
----------------------------------------------------------------------------*/
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC
DWORD CConMgr::FindIndexFromPrefix(LPCSTR szUrl)
{
    // Unless we find a matching prefix, CONTENT is the default.
    DWORD n = szUrl[0]==EXTENSIBLE_FIRST ? NOT_AN_INDEX : CONTENT;
    URL_CONTAINER *co = NULL;

    //  NOTE: if deleting a container is supported, ConList.Get(i) can
    //  return NULL, if list shrinks after getting size.
    //  NOTE: if deleting containers is supported, it is not safe to
    //  assume CONTENT if prefix is not found.  client may be trying to
    //  insert into a container that has been deleted, but once existed.
    //  proper response is to return an error.  the simplest way to do this
    //  is to insist that all Extensible cache prefixes start with an illegal
    //  URL character, EXTENSIBLE_FIRST

    GetExtensibleCacheConfigInfo(FALSE);
    for (DWORD i = COOKIE; i < ConList.Size(); i++)
    {
        co = ConList.Get(i);
        if (co)
        {
            if (co->PrefixMatch(szUrl))
            {
                // For content container, strnicmp (szUrl, "", 0) returns nonzero
                if (co->IsVisible())
                {
                    // Found a match
                    n = i;
                    if (!co->IsInitialized())
                    {
                        // Init the container. If this fails,
                        // Mark it as DELETED and return CONTENT.
                        switch (co->Init())
                        {
                        case ERROR_SUCCESS:
                        case ERROR_ALREADY_EXISTS:
                            if (!(co->GetOptions() & INTERNET_CACHE_CONTAINER_NODESKTOPINIT))
                                EnableCacheVu(co->GetCachePath(), n);
                            break;
                        default:
                            INET_ASSERT(FALSE);
                            co->SetDeleted(TRUE);
                            n = szUrl[0]==EXTENSIBLE_FIRST ? NOT_AN_INDEX : CONTENT;
                            break;
                        }
                    }
                    co->Release(FALSE);
                    break;
                }
            }
            co->Release(FALSE);
        }
    }
    return n;
}



/*-----------------------------------------------------------------------------
BOOL CConMgr::PathPrefixMatch
----------------------------------------------------------------------------*/
BOOL CConMgr::PathPrefixMatch(LPCSTR szPath, LPCSTR szPathRef)
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
    if (szPath[len-1] == DIR_SEPARATOR_CHAR)
        --len;

    // Compare paths.
    if (!strnicmp(szPath, szPathRef, len))
        if (szPathRef[len] == DIR_SEPARATOR_CHAR || szPathRef[len] == 0)
            return TRUE;

    return FALSE;
}

/*---------------------  Public Functions -----------------------------------*/

/*-----------------------------------------------------------------------------
CConMgr::CConMgr

  Default Constructor
  ----------------------------------------------------------------------------*/
CConMgr::CConMgr()
: ConList()
{
    _coContent = NULL;
    _coCookies = NULL;
    _coHistory = NULL;

    // Assume this is a profiles-capable machine. Later on, we'll make sure this is
    // the case.
    _fProfilesCapable = TRUE;

    // Assume that we'll be using the regular containers, instead of the backup
    _fUsingBackupContainers = FALSE;
    _dwStatus = Init();
}


/*-----------------------------------------------------------------------------
CConMgr::~CConMgr

  Default Destructor
  ----------------------------------------------------------------------------*/
CConMgr::~CConMgr()
{
    ConList.Free();
    GlobalUrlContainers = NULL;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::GetStatus()
----------------------------------------------------------------------------*/
DWORD CConMgr::GetStatus()
{
    return _dwStatus;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::UnlockUrl
----------------------------------------------------------------------------*/
DWORD CConMgr::UnlockUrl(LPCSTR szUrl)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.
    DWORD idx;

    LOCK_CACHE();

    idx = FindIndexFromPrefix(szUrl);
    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call UnlockUrl on the appropriate container.
        dwError = co->UnlockUrl(szUrl); // may be expensive
        LOCK_CACHE();
        co->Release(TRUE);
    }
    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::DeleteUrl
----------------------------------------------------------------------------*/
DWORD CConMgr::DeleteUrl(LPCSTR szUrl)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.
    DWORD idx;

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    LOCK_CACHE();

    idx = FindIndexFromPrefix(szUrl);
    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call DeleteUrl on the appropriate container.
        dwError = co->DeleteUrl(szUrl); // may be expensive.
        LOCK_CACHE();

        co->Release(TRUE);

        // Update the change count for the cookies container.
        if (idx == COOKIE)
        {
            DWORD dwChange = 0;
            _coContent->IncrementHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &dwChange);
        }
    }

    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
BOOL CConMgr::GetHeaderData
----------------------------------------------------------------------------*/
BOOL CConMgr::GetHeaderData(DWORD nIdx, LPDWORD pdwData)
{
    return _coContent->GetHeaderData(nIdx, pdwData);
}

/*-----------------------------------------------------------------------------
BOOL CConMgr::SetHeaderData
----------------------------------------------------------------------------*/
BOOL CConMgr::SetHeaderData(DWORD nIdx, DWORD dwData)
{
    UNIX_RETURN_ERR_IF_READONLY_CACHE(FALSE);
    return _coContent->SetHeaderData(nIdx, dwData);
}

/*-----------------------------------------------------------------------------
BOOL CConMgr::IncrementHeaderData
----------------------------------------------------------------------------*/
BOOL CConMgr::IncrementHeaderData(DWORD nIdx, LPDWORD pdwData)
{
    UNIX_RETURN_ERR_IF_READONLY_CACHE(FALSE);
    return _coContent->IncrementHeaderData(nIdx, pdwData);
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::SetUrlGroup
----------------------------------------------------------------------------*/
DWORD CConMgr::SetUrlGroup(
    IN LPCSTR   szUrl,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId
    )
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);

    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call SetUrlInGroup on the appropriate container.
        dwError = co->SetUrlGroup
            (szUrl, dwFlags, GroupId);
        LOCK_CACHE();
        co->Release(TRUE);
    }

    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::CreateUniqueFile
----------------------------------------------------------------------------*/
DWORD CConMgr::CreateUniqueFile(LPCSTR szUrl, DWORD dwExpectedSize,
                                       LPCSTR szFileExtension, LPTSTR szFileName,
                                       HANDLE *phfHandle)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);


    co = ConList.Get(idx);


    if (co)
    {
        UNLOCK_CACHE();
        // Call CreateUniqueFile on the appropriate container.
        dwError = co->CreateUniqueFile(szUrl, dwExpectedSize,
            szFileExtension, szFileName, phfHandle); // expensive call
        LOCK_CACHE();
    }

    co->Release(TRUE);
    UNLOCK_CACHE();
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::AddUrl
----------------------------------------------------------------------------*/
DWORD CConMgr::AddUrl(AddUrlArg* pArgs)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    LOCK_CACHE();

    DWORD idx = FindIndexFromPrefix(pArgs->pszUrl);
    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call AddUrl on the appropriate container.
        dwError = co->AddUrl(pArgs); // may be expensive
        LOCK_CACHE();

        co->Release(TRUE);

        // Update the change count for the cookies container.
        if (idx == COOKIE)
        {
            DWORD dwChange = 0;
            _coContent->IncrementHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &dwChange);
        }
    }

    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::RetrieveUrl
----------------------------------------------------------------------------*/
DWORD CConMgr::RetrieveUrl( LPCSTR               szUrl,
                            LPCACHE_ENTRY_INFOA* ppCacheEntryInfo,
                            LPDWORD              pcbCacheEntryInfoBufferSize,
                            DWORD                dwLookupFlags,
                            DWORD                dwRetrievalFlags)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);


    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call RetrieveUrl on the appropriate container.
        dwError = co->RetrieveUrl(szUrl,
                                  ppCacheEntryInfo,
                                  pcbCacheEntryInfoBufferSize,
                                  dwLookupFlags, dwRetrievalFlags); // expensive?
        LOCK_CACHE();
        co->Release(TRUE);
    }

    UNLOCK_CACHE();
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::GetUrlInfo
----------------------------------------------------------------------------*/
DWORD CConMgr::GetUrlInfo( LPCSTR               szUrl,
                           LPCACHE_ENTRY_INFOA*  ppCacheEntryInfo,
                           LPDWORD              pcbCacheEntryInfoBufferSize,
                           DWORD                dwLookupFlags,
                           DWORD                dwEntryFlags,
                           DWORD                dwRetrievalFlags)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);

    co = ConList.Get(idx);

    if (co)
    {
        // Call GetUrlInfo on the appropriate container.
        dwError = co->GetUrlInfo(szUrl,
                                 ppCacheEntryInfo,
                                 pcbCacheEntryInfoBufferSize,
                                 dwLookupFlags,
                                 dwEntryFlags,
                                 dwRetrievalFlags);

        co->Release(TRUE);
    }
    UNLOCK_CACHE();
    return dwError;
}

DWORD CConMgr::GetUrlInfo(LPCSTR               szUrl,
                          LPCACHE_ENTRY_INFOA  pCacheEntryInfo,
                          LPDWORD              pcbCacheEntryInfoBufferSize,
                          DWORD                dwLookupFlags,
                          DWORD                dwEntryFlags)
{
    return GetUrlInfo(szUrl,
                     (pCacheEntryInfo) ? &pCacheEntryInfo : NULL,
                     pcbCacheEntryInfoBufferSize,
                     dwLookupFlags,
                     dwEntryFlags,
                     0);
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::SetUrlInfo
----------------------------------------------------------------------------*/
DWORD CConMgr::SetUrlInfo(LPCSTR               szUrl,
                          LPCACHE_ENTRY_INFOA  pCacheEntryInfo,
                          DWORD                dwFieldControl)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);

    co = ConList.Get(idx);

    if (co)
    {
        // Call SetUrlInfo on the appropriate container.
        dwError = co->SetUrlInfo(szUrl,
                                 pCacheEntryInfo,
                                 dwFieldControl);
        co->Release(TRUE);
    }

    UNLOCK_CACHE();
    return dwError;
}

DWORD CConMgr::FreeFindHandle(HANDLE hFind)
{
    DWORD dwError = ERROR_INVALID_HANDLE;

    if (hFind)
    {
        LOCK_CACHE();

        LPCACHE_FIND_FIRST_HANDLE pFind;
        pFind = (CACHE_FIND_FIRST_HANDLE*) HandleMgr.Map (hFind);
        if (pFind)
        {
            //  NOTHING SPECIAL TO DO FOR SIGNATURE_CONTAINER_FIND
            if (pFind->dwSig == SIG_CACHE_FIND && !pFind->fFixed)
            {
                URL_CONTAINER *co = ConList.Get(pFind->nIdx);
                if (co)
                {
                    //  It now has 2 AddRefs to balance
                    co->Release(FALSE);
                    co->Release(TRUE);
                }
            }

            HandleMgr.Free (hFind);
            dwError = ERROR_SUCCESS;
        }

        UNLOCK_CACHE();
    }
    
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::FindNextEntry
----------------------------------------------------------------------------*/
DWORD CConMgr::FindNextEntry(
      HANDLE              *phFind,
      LPCSTR               szPrefix,
      LPCACHE_ENTRY_INFOA*  ppInfo,
      LPDWORD              pcbInfo,
      DWORD                dwFilter,
      GROUPID              GroupId,
      DWORD                dwFlags,
      DWORD                dwRetrievalFlags)
{
    DWORD                      idx, dwError;
    URL_CONTAINER             *co    = NULL;
    LPCACHE_FIND_FIRST_HANDLE  pFind = NULL;

    LOCK_CACHE();

    // Null handle initiates enumeration.
    if (!*phFind)
    {
        // Allocate a handle.
        //LOCK_CACHE();
        *phFind = HandleMgr.Alloc (sizeof(CACHE_FIND_FIRST_HANDLE));
        if (*phFind)
            pFind = (CACHE_FIND_FIRST_HANDLE*) HandleMgr.Map (*phFind);
        //UNLOCK_CACHE();
        if (!*phFind)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        // Set signature and initial hash table find
        // handle in the newly allocated find handle.
        pFind->dwSig     = SIG_CACHE_FIND;
        pFind->dwHandle  = 0;
        pFind->dwFlags   = dwFlags;

        // Find the associated container. NULL prefix
        // results in enumeration over fixed containers.
        if (!szPrefix)
        {
            pFind->fFixed = TRUE;
            pFind->nIdx   = CONTENT;

        }
        else
        {

            idx = FindIndexFromPrefix(szPrefix);
            if (idx == NOT_AN_INDEX)
            {
                dwError = ERROR_NO_MORE_ITEMS;
                goto exit;
            }

            // Under old semantics prefix resolving to CONTENT
            // container implies that an enumeration over all
            // the fixed containers is desired. Enumeration then
            // begins with the CONTENT container. We do not keep
            // a refcount on any fixed containers in this case.
            if (idx == CONTENT && (dwFlags & FIND_FLAGS_OLD_SEMANTICS))
            {
                pFind->fFixed = TRUE;
                pFind->nIdx   = CONTENT;
            }
            else
            {
                // Otherwise only enumeration over the found container
                // is implied. Retrieve this container. Set fFixed to FALSE.
                //  NO RELEASE: hold RefCnt until handle is closed
                co = ConList.Get(idx);

                if (co)
                {
                    pFind->fFixed   = FALSE;
                    pFind->nIdx     = idx;
                    pFind->dwHandle = co->GetInitialFindHandle();
                }
                else
                {
                    dwError = ERROR_NO_MORE_ITEMS;
                    goto exit;
                }
            }

        }
        // Set filter and group id in handle.
        pFind->dwFilter = dwFilter;
        pFind->GroupId  = GroupId;
    }
    else
    {
        // Valid handle passed in - map it.
        //LOCK_CACHE();
        pFind = (CACHE_FIND_FIRST_HANDLE*) HandleMgr.Map (*phFind);
        //UNLOCK_CACHE();
        if (!pFind)
        {
            dwError = ERROR_INVALID_HANDLE;
            goto exit;
        }
    }

    // -------------------------------------------------------------------------
    // The handle is initialized or was created via a previous FindNextEntry.
    //--------------------------------------------------------------------------

    dwError = ERROR_NO_MORE_ITEMS;

    // Are we only enumerating over one container?
    if (!pFind->fFixed)
    {
        // Get the associated container.
        co = ConList.Get(pFind->nIdx);

        if (co)
        {
            // Enum on the container and release.
            dwError = co->FindNextEntry(&pFind->dwHandle, ppInfo, pcbInfo, pFind->dwFilter, pFind->GroupId, pFind->dwFlags, dwRetrievalFlags);
            co->Release(TRUE);
        }
        else
        {
            // Getting container failed.
            dwError = ERROR_NO_MORE_ITEMS;
            goto exit;
        }

    }
    else
    {
        // fFixed is TRUE - enumerate over the fixed containers.
        while (pFind->nIdx < NCONTAINERS)
        {
            // Get the associated container.
            co = ConList.Get(pFind->nIdx);

            if (co)
            {
                // Get the initial hash find handle if not already done so.
                if (!pFind->dwHandle)
                    pFind->dwHandle = co->GetInitialFindHandle();

                // Enum on the container and release.
                dwError = co->FindNextEntry(&pFind->dwHandle, ppInfo, pcbInfo, pFind->dwFilter, pFind->GroupId, pFind->dwFlags, dwRetrievalFlags);
                co->Release(TRUE);

                // Goto exit only if ERROR_NO_MORE_ITEMS.
                // This handles ERROR_SUCCESS correctly.
                if (dwError != ERROR_NO_MORE_ITEMS)
                    goto exit;

                // ERROR_NO_MORE_ITEMS: Go to next container
                // and begin enum anew.
                pFind->nIdx++;
                pFind->dwHandle = 0;
            }
            else
            {
                // Getting container failed.
                dwError = ERROR_NO_MORE_ITEMS;
                goto exit;
            }
        }
    }

exit:

    UNLOCK_CACHE();

    INET_ASSERT(*phFind != 0);
    INET_ASSERT(pFind != NULL);

    return dwError;
}

DWORD CConMgr::FindNextEntry(
      HANDLE              *phFind,
      LPCSTR               szPrefix,
      LPCACHE_ENTRY_INFOA  pInfo,
      LPDWORD              pcbInfo,
      DWORD                dwFilter,
      GROUPID              GroupId,
      DWORD                dwFlags)
{
    return FindNextEntry(
                phFind,
                szPrefix,
                (pInfo ? &pInfo : NULL),
                pcbInfo,
                dwFilter,
                GroupId,
                dwFlags,
                0);
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::CleanupUrls
----------------------------------------------------------------------------*/
DWORD CConMgr::CleanupUrls
    (LPCTSTR szCachePath, DWORD dwFactor, DWORD dwFilter)
{
    DWORD dwError = ERROR_SUCCESS;

    UNIX_RETURN_ERR_IF_READONLY_CACHE(dwError);

    // Bad cleanup parameter.
    if (dwFactor <= 0 || dwFactor > 100)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // For null or empty path, clean up content container.
    if (!szCachePath || !*szCachePath)
    {
        _coContent->CleanupUrls(dwFactor, dwFilter);
    }
    else
    {
        LOCK_CACHE();

        // Find the container with the matching cache path and clean it up.
        for (DWORD idx = CONTENT; idx < NCONTAINERS; idx++)
        {
            URL_CONTAINER *co = ConList.Get(idx);

            if (co)
            {
                if (PathPrefixMatch(szCachePath, co->GetCachePath()))
                {
                    UNLOCK_CACHE();
                    co->CleanupUrls(dwFactor, dwFilter); // expensive?
                    LOCK_CACHE();
                    co->Release(TRUE);
                    break;
                }
                co->Release(TRUE);
            }
        }

        UNLOCK_CACHE();
    }

exit:
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::GetUrlInGroup
----------------------------------------------------------------------------*/
DWORD CConMgr::GetUrlInGroup(LPCSTR szUrl, GROUPID* pGroupId, LPDWORD pdwExemptDelta)
{
    URL_CONTAINER *co;
    DWORD dwError = ERROR_FILE_NOT_FOUND;
    // Find the associated container.
    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szUrl);

    co = ConList.Get(idx);

    if (co)
    {
        UNLOCK_CACHE();
        // Call GetUrlInGroup on the appropriate container.
        dwError = co->GetUrlInGroup(szUrl, pGroupId, pdwExemptDelta);
        LOCK_CACHE();
        co->Release(TRUE);
    }
    UNLOCK_CACHE();
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::CreateGroup()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::CreateGroup(DWORD dwFlags, GROUPID* pGID)
{
    INET_ASSERT(_coContent);

    LOCK_CACHE();

    GroupMgr gm;
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;

    if( gm.Init(_coContent) )
    {
        dwError = gm.CreateGroup(dwFlags, pGID);
    }


    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::CreateDefaultGroups()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::CreateDefaultGroups()
{
    INET_ASSERT(_coContent); 

    LOCK_CACHE();

    GroupMgr gm;
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;

    if( gm.Init(_coContent) )
    {
        dwError = gm.CreateDefaultGroups();
    }


    UNLOCK_CACHE();
    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::DeleteGroup()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::DeleteGroup(GROUPID gid, DWORD dwFlags)
{
    INET_ASSERT(_coContent);

    LOCK_CACHE();
    GroupMgr gm;
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;

    if( gm.Init(_coContent) )
    {
        dwError = gm.DeleteGroup(gid, dwFlags);
    }

    UNLOCK_CACHE();

    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::FindNextGroup()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::FindNextGroup(
      HANDLE*                               phFind,
      DWORD                                 dwFlags,
      GROUPID*                              pGroupId
)
{
    DWORD                      dwError;
    GROUP_FIND_FIRST_HANDLE*   pFind = NULL;
    GroupMgr gm;

    INET_ASSERT(_coContent);
    LOCK_CACHE();


    // Null handle initiates enumeration.
    if (!*phFind)
    {
        // BUGBUG currently only supports SEARCH_ALL option
        if( dwFlags )
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto exit;
        }


        // Allocate a handle.
        *phFind = HandleMgr.Alloc (sizeof(GROUP_FIND_FIRST_HANDLE));
        if (*phFind)
            pFind = (GROUP_FIND_FIRST_HANDLE*) HandleMgr.Map (*phFind);

        if (!*phFind)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        // Set signature and initial hash table find
        // handle in the newly allocated find handle.
        pFind->dwSig  = SIG_GROUP_FIND;
        pFind->fFixed = TRUE;
        pFind->nIdx = CONTENT;
        pFind->dwLastItemOffset = 0;
    }
    else
    {
        // Valid handle passed in - map it.
        pFind = (GROUP_FIND_FIRST_HANDLE*) HandleMgr.Map (*phFind);
        if (!pFind)
        {
            dwError = ERROR_INVALID_HANDLE;
            goto exit;
        }
    }

    //
    // The handle is initialized or was created via a previous FindNextEntry.
    //

    dwError = ERROR_FILE_NOT_FOUND;

    // Enum on the container and release.
    if( gm.Init(_coContent) )
    {
        DWORD dwLastItemOffset = pFind->dwLastItemOffset;

        dwError = gm.GetNextGroup(&dwLastItemOffset, pGroupId);

        // update offset field of the find handle
        pFind->dwLastItemOffset = dwLastItemOffset;

    }

exit:

    UNLOCK_CACHE();
    INET_ASSERT(*phFind != 0);
    INET_ASSERT(pFind != NULL);

    return dwError;
}

/*-----------------------------------------------------------------------------
DWORD CConMgr::GetGroupAttributes()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::GetGroupAttributes(
    GROUPID                             gid,
    DWORD                               dwAttrib,
    LPINTERNET_CACHE_GROUP_INFOA        lpGroupInfo,
    LPDWORD                             lpdwGroupInfo
    )
{
    INET_ASSERT(_coContent);
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;

    LOCK_CACHE();
    GroupMgr gm;
    if( gm.Init(_coContent) )
    {
        dwError = gm.GetGroup(gid, dwAttrib, lpGroupInfo, lpdwGroupInfo);
    }

    UNLOCK_CACHE();
    return dwError;
}


/*-----------------------------------------------------------------------------
DWORD CConMgr::SetGroupAttributes()
  ----------------------------------------------------------------------------*/
DWORD CConMgr::SetGroupAttributes(
    GROUPID                             gid,
    DWORD                               dwAttrib,
    LPINTERNET_CACHE_GROUP_INFOA        lpGroupInfo
    )
{
    INET_ASSERT(_coContent);
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;

    LOCK_CACHE();
    GroupMgr gm;
    if( gm.Init(_coContent))
    {
        dwError = gm.SetGroup(gid, dwAttrib, lpGroupInfo);
    }

    UNLOCK_CACHE();
    return dwError;
}


DWORD CConMgr::RegisterCacheNotify(
    HWND    hWnd,
    UINT    uMsg,
    GROUPID gid,
    DWORD   dwFilter
    )
{
    DWORD dwError;
    INET_ASSERT(_coContent);
    dwError = _coContent->RegisterCacheNotify(hWnd, uMsg, gid, dwFilter);
    return dwError;
}

DWORD CConMgr::SendCacheNotification( DWORD  dwOp)
{
    DWORD dwError;

    INET_ASSERT(_coContent);
    _coContent->SendCacheNotification(dwOp);
    return ERROR_SUCCESS;
}

/*-----------------------------------------------------------------------------
VOID CConMgr::GetCacheInfo
----------------------------------------------------------------------------*/
VOID CConMgr::GetCacheInfo(LPCSTR szPrefix, LPSTR szCachePath, LONGLONG *cbLimit)
{
    URL_CONTAINER *co;
    // Find the associated container.

    LOCK_CACHE();
    DWORD idx = FindIndexFromPrefix(szPrefix);
    
    co = ConList.Get(idx);
    
    if (co)
    {
        // Call GetCacheInfo on the appropriate container.
        co->GetCacheInfo(szCachePath, cbLimit);
        co->Release(TRUE);
    }

    UNLOCK_CACHE();
}



/*-----------------------------------------------------------------------------
VOID CConMgr::SetCacheLimit
----------------------------------------------------------------------------*/
VOID CConMgr::SetCacheLimit(LONGLONG cbLimit, DWORD idx)
{
    URL_CONTAINER *co;
    // Find the associated container.

    UNIX_RETURN_IF_READONLY_CACHE;
    
    LOCK_CACHE();
    co = ConList.Get(idx);
    
    if (co)
    {
        // Call SetCacheLimit on the container.
        co->SetCacheLimit(cbLimit);
        co->Release(TRUE);
    }

    UNLOCK_CACHE();
}

