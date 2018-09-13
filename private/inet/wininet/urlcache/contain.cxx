/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    contain.cxx

Abstract:

    Abstract-for-module.

    Contents:

Author:

     16-Nov-1995

[Environment:]

    optional-environment-info (e.g. kernel mode only...)

[Notes:]

    optional-notes

Revision History:

    16-Nov-1995
        Created

    Shishir Pardikar (shishirp) added: (as of 7/6/96)

    1) Container allows any size file. The file is cleanedup at scavneging time
    2) Free 100% uses cleanupallurls, reinitializes memorymappedfile and cleansup
       all directories
    3) CurrentCacheSIze and Cache Limit in the memorymapped file itself
    4) FileCreation time and lastcheckedtime added
    5) friendly naming scheme

    25-Sep-1997

    Ahsan Kabir (akabir) made minor alterations to GetFileSizeAndTimeByName.

--*/

/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    contain.cxx

Abstract:

    Contains code that implements CONTAINER classes defined in
    contain.hxx.

Author:

    Madan Appiah (madana)  28-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/


#include <cache.hxx>

// Beta logging
#ifdef BETA_LOGGING
#define BETA_LOG(stat) \
    {DWORD dw; INET_ASSERT (IsContentContainer()); \
    IncrementHeaderData (CACHE_HEADER_DATA_##stat, &dw);}
#else
#define BETA_LOG(stat) do { } while(0)
#endif

// Typedef for GetFileAttributeEx function
typedef BOOL (WINAPI *PFNGETFILEATTREX)(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
extern PFNGETFILEATTREX gpfnGetFileAttributesEx;


// private functions
DWORD GetFileSizeAndTimeByName(
    LPCTSTR FileName,
    WIN32_FILE_ATTRIBUTE_DATA *lpFileAttrData
    )
/*++

Routine Description:

    Get the size and creation time and file attributes of the specified file.

Arguments:

    FileName : full path name of the file whose size is asked for.

    lpFindData : pointer to a WIN32_FIND_DATA structure where the size and time value is
        returned. On WinNT, only the size and time fields are valid.

Return Value:

    Windows Error Code.

--*/
{
    INET_ASSERT(lpFileAttrData != NULL);

    if (gpfnGetFileAttributesEx)
    {
        if(!gpfnGetFileAttributesEx(FileName, GetFileExInfoStandard, (LPVOID)lpFileAttrData))
            return( GetLastError() );
    }
    else
    {
        HANDLE hHandle;
        WIN32_FIND_DATA FindData;

        hHandle = FindFirstFile(FileName, &FindData);
        if( hHandle == INVALID_HANDLE_VALUE ) {
            return( GetLastError() );
        }
        memset(lpFileAttrData, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
        lpFileAttrData->dwFileAttributes = FindData.dwFileAttributes;
        lpFileAttrData->nFileSizeLow = FindData.nFileSizeLow;
        lpFileAttrData->nFileSizeHigh = FindData.nFileSizeHigh;
        lpFileAttrData->ftCreationTime = FindData.ftCreationTime;

        FindClose(hHandle);
    }

    return(ERROR_SUCCESS);
}

DWORD
GetFileSizeByName(
    LPCTSTR pszFileName,
    DWORD *pdwFileSize
    )
/*++

Routine Description:

    Get the size of the specified file.

Arguments:

    FileName : full path name of the file whose size is asked for.

    FileSize : pointer to a longlong location where the size value is
        returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD dwError = ERROR_SUCCESS;

    // get the size of the file being cached.
    //  since we do not handle 4+gb files, we can safely ignore the high dword

    INET_ASSERT(pdwFileSize!=NULL);

    if (gpfnGetFileAttributesEx)
    {
        WIN32_FILE_ATTRIBUTE_DATA FileAttrData;
        if(!gpfnGetFileAttributesEx(pszFileName, GetFileExInfoStandard, &FileAttrData))
            return( GetLastError() );

        *pdwFileSize = FileAttrData.nFileSizeLow;
    }
    else
    {
        HANDLE hfFileHandle = CreateFile(
                        pszFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if( hfFileHandle == INVALID_HANDLE_VALUE )
            return(GetLastError());

        *pdwFileSize = GetFileSize( hfFileHandle, NULL);
        if(*pdwFileSize == 0xFFFFFFFF)
            dwError = GetLastError();

        CloseHandle( hfFileHandle );
    }
    return dwError;
}

// -------------------------------URL_CONTAINER----------------------------------- //

/*-----------------------------------------------------------------------------
URL_CONTAINER::URL_CONTAINER Sets path, prefix and limit.
-----------------------------------------------------------------------------*/
URL_CONTAINER::URL_CONTAINER(LPTSTR CacheName, LPTSTR CachePath,
                             LPTSTR CachePrefix, LONGLONG CacheStartUpLimit,
                             DWORD dwOptions)
{
    _fIsInitialized = FALSE;
    _fPerUserItem = TRUE;
    _dwLastReference = GetTickCount();
    _fDeleted = FALSE;
    _fMarked = FALSE;
    _fDeletePending = FALSE;
    _fMustLaunchScavenger = FALSE;
//#ifdef CHECKLOCK_NORMAL
    _dwTaken = 0;
//#endif
    _dwRefCount = 0;
    _dwOptions = dwOptions;
    _dwBytesDownloaded = _dwItemsDownloaded = 0;

    if (!CachePath || !*CachePath || !CachePrefix || !CacheStartUpLimit)
    {
        _Status = ERROR_INVALID_PARAMETER;
        return;
    }

    _CacheName = NewString(CacheName == NULL ? TEXT(""):CacheName);

    _CachePathLen = strlen(CachePath);

    if (CachePath[_CachePathLen-1] != DIR_SEPARATOR_CHAR)
    {
        _CachePath = CatString(CachePath, DIR_SEPARATOR_STRING);
        _CachePathLen++;
    }
    else
        _CachePath = NewString(CachePath);

    _CachePrefix = NewString(CachePrefix);

    if (!_CachePath || !_CachePrefix || !_CacheName)
    {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    _CachePrefixLen = strlen(_CachePrefix);

    _CacheStartUpLimit = CacheStartUpLimit;

    _UrlObjStorage = NULL;
    _FileManager = NULL;

    if (!memcmp(_CachePrefix, CONTENT_PREFIX, sizeof(CONTENT_PREFIX)))
        _CacheEntryType = NORMAL_CACHE_ENTRY;
    else if (!memcmp(_CachePrefix, COOKIE_PREFIX, sizeof(COOKIE_PREFIX)))
        _CacheEntryType = COOKIE_CACHE_ENTRY;
    else if (!memcmp(_CachePrefix, HISTORY_PREFIX, sizeof(HISTORY_PREFIX)))
        _CacheEntryType = URLHISTORY_CACHE_ENTRY;
    else
        _CacheEntryType = 0;

    _Status = ERROR_SUCCESS;
}


#ifdef CHECKLOCK_PARANOID
void URL_CONTAINER::CheckNoLocks(DWORD dwThreadId)
{
    INET_ASSERT(_dwTaken == 0 || _dwThreadLocked != dwThreadId);
}
#endif

/*-----------------------------------------------------------------------------
URL_CONTAINER::Init
-----------------------------------------------------------------------------*/
DWORD URL_CONTAINER::Init()
{
    _Status = ERROR_SUCCESS;
    _FileMapEntrySize = NORMAL_ENTRY_SIZE;
    MemMapStatus eMMStatus;
    BOOL fMustUnlock = FALSE;
    DWORDLONG dlSize;

    // Generate the mutex name based on the cache path.
    DWORD i;
    LPSTR szPrefix;
    CHAR MutexName[MAX_PATH + 1];
    LPTSTR pCachePath, pMutexName;

    i = 0;
    pCachePath = _CachePath,
    pMutexName = (LPSTR) MutexName;
    while( *pCachePath != '\0'  && (i++ < MAX_PATH))
    {
        if( *pCachePath == DIR_SEPARATOR_CHAR )
            *pMutexName = '!';
        else
            *pMutexName = tolower(*pCachePath);

        pMutexName++;
        pCachePath++;
    }
    *pMutexName = '\0';

    // Open the existing mutex, or if first process, create a new one.
    BOOL fFirstProcess;

    _MutexHandle = OpenMutex(SYNCHRONIZE, FALSE, (LPTSTR)MutexName);
    if (_MutexHandle == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
    {
        _MutexHandle = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, (LPTSTR)MutexName );
        if (_MutexHandle != NULL)
            fFirstProcess = TRUE;
    }

    if (_MutexHandle == NULL)
    {
        _Status = GetLastError();
        goto Cleanup;
    }

    // Lock the container.
    if (!LockContainer(&fMustUnlock))
    {
        if (fMustUnlock) ReleaseMutex(_MutexHandle);
        fMustUnlock = FALSE;
        _Status = GetLastError();
        if (_MutexHandle)
        {
            CloseHandle(_MutexHandle);
            _MutexHandle = NULL;
        }
        goto Cleanup;
    }
    if ((_CachePathLen > 1) && (_CachePath[_CachePathLen-1] != PATH_CONNECT_CHAR))
    {
        lstrcat( _CachePath, PATH_CONNECT_STRING );
        _CachePathLen++;
    }

    // Initialize _ClusterSizeMinusOne and _ClusterSizeMask
    if (!GetDiskInfo(_CachePath, &_ClusterSizeMinusOne, &dlSize, NULL))
    {
        _Status = GetLastError();
        goto Cleanup;
    }
    _ClusterSizeMinusOne--;
    _ClusterSizeMask = ~_ClusterSizeMinusOne;

    // Construct and initialize the memory mapped file object.
    _UrlObjStorage = new MEMMAP_FILE;
    if( _UrlObjStorage == NULL )
    {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    eMMStatus = _UrlObjStorage->Init(_CachePath, _FileMapEntrySize);

    if((_Status = _UrlObjStorage->GetStatus()) != ERROR_SUCCESS )
        goto Cleanup;

    // for first process attach, we need to clean up the notification
    // hwnd, msg, gid and filter
    if( fFirstProcess && (_CacheEntryType == NORMAL_CACHE_ENTRY ))
    {
        RegisterCacheNotify(0, 0, 0, 0);
    }

    _UrlObjStorage->SetCacheLimit(_CacheStartUpLimit);

    // Construct and initialize the file manager.
    // Cookies and history don't use random subdirs.
    // BUGBUG - move this off to container manager.
    szPrefix = GetCachePrefix();

    if (!strcmp(szPrefix, COOKIE_PREFIX)
        || !strcmp(szPrefix, HISTORY_PREFIX)
        || (_dwOptions & INTERNET_CACHE_CONTAINER_NOSUBDIRS))
    {
        // Insecure cache -no random cache subdirs.
        _FileManager = new CFileMgr(_UrlObjStorage, GetOptions());
        if (!_FileManager)
        {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }
    else
    {
        // Secure cache - random cache subdirs.
        _FileManager = new CSecFileMgr(_UrlObjStorage, GetOptions());
        if (!_FileManager)
        {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // If first process to attach, unlock any locked entries.
    if (fFirstProcess)
        UnlockAllItems();

    // If the memory mapped file was reinitialized
    // cleanup old files.
    if (eMMStatus == MEMMAP_STATUS_REINITIALIZED)
        _FileManager->Cleanup();
    else
        eMMStatus = MEMMAP_STATUS_OPENED_EXISTING;

    _fIsInitialized = TRUE;
    _fMustLaunchScavenger = (BOOL)(dlSize <= (DWORDLONG)(4*1024*1024));

Cleanup:

    if( _Status != ERROR_SUCCESS)
    {
        INET_ASSERT(FALSE);
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "URL_CONTAINER::URL_CONTAINER() failed, %ld\n", _Status ));
        SetLastError(_Status);
    }
    if (fMustUnlock) UnlockContainer();

    if (_Status == ERROR_SUCCESS)
        return (eMMStatus == MEMMAP_STATUS_OPENED_EXISTING ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);

    return _Status;
}

void URL_CONTAINER::CloseContainerFile()
{

    // Cleanup.
    // _Filemanager holds a pointer to _UrlObjStorage and
    // must be deleted before _UrlObjStorage.
    delete _FileManager;
    delete _UrlObjStorage;

    _FileManager = NULL;
    _UrlObjStorage = NULL;

    if (!_fDeleted)
    {
        // Keep fixed container files from being deleted.
        if (!(_CacheEntryType & (NORMAL_CACHE_ENTRY | COOKIE_CACHE_ENTRY | URLHISTORY_CACHE_ENTRY)))
        {
            _fDeleted = GlobalUrlContainers->DeleteFileIfNotRegistered(this);
            if (_fDeleted) _fDeletePending = FALSE;
        }
    }
}

URL_CONTAINER::~URL_CONTAINER(
    VOID
    )
/*++

Routine Description:

    URL_CONTAINER destructor

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL fMustUnlock;

    // If not initialized, only delete path and prefix.
    if (!IsInitialized())
    {
        //  Free pending deleted container, even if someone has
        //  a enum handle open on it.  This is our last chance as we handle process
        //  detach

        LOCK_CACHE();
        TryToUnmap(0xFFFFFFFF);
        UNLOCK_CACHE();

        delete _CacheName;
        delete _CachePath;
        delete _CachePrefix;
        return;
    }

    LockContainer(&fMustUnlock);

    // Otherwise, do a full destruct.
    CloseContainerFile();

    if (fMustUnlock) UnlockContainer();

    // Delete mutex.
    if( _MutexHandle != NULL )
        CloseHandle( _MutexHandle );

    delete _CacheName;
    delete _CachePath;
    delete _CachePrefix;
}

DWORD URL_CONTAINER::GetOptions()
{
    return _dwOptions;
}

DWORD URL_CONTAINER::GetLastReference()
{
    return _dwLastReference;
}

BOOL URL_CONTAINER::IsVisible()
{
    return !(_fDeletePending || _fDeleted);
}

void URL_CONTAINER::Mark(BOOL fMarked)
{
    _fMarked = fMarked;
}

BOOL URL_CONTAINER::GetMarked()
{
    return _fMarked;
}

BOOL URL_CONTAINER::GetDeleted()
{
    return _fDeleted;
}

void URL_CONTAINER::SetDeleted(BOOL fDeleted)
{
    if (!_fIsInitialized) _fDeleted = fDeleted;
}

BOOL URL_CONTAINER::GetDeletePending()
{
    return _fDeletePending;
}

void URL_CONTAINER::SetDeletePending(BOOL fDeletePending)
{
    _fDeletePending = fDeletePending;
}

void URL_CONTAINER::AddRef()
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC.
{
    _dwRefCount++;
}

void URL_CONTAINER::TryToUnmap(DWORD dwAcceptableRefCount)
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC.
{
    BOOL fMustUnlock;

    if (_dwRefCount <= dwAcceptableRefCount)
    {
        if (_fIsInitialized)
        {
            LockContainer(&fMustUnlock);

            CloseContainerFile();

            if (fMustUnlock) UnlockContainer();

            // Delete mutex.
            if( _MutexHandle != NULL )
            {
                CloseHandle( _MutexHandle );
                _MutexHandle = NULL;
            }
            _fIsInitialized = FALSE;
        }
        else
        {
            if (!_fDeleted)
            {
                //  Never CONTENT, COOKIES or HISTORY container.
                if (!(_CacheEntryType & (NORMAL_CACHE_ENTRY | COOKIE_CACHE_ENTRY | URLHISTORY_CACHE_ENTRY)))
                {
                    _fDeleted = GlobalUrlContainers->DeleteFileIfNotRegistered(this);
                    if (_fDeleted) _fDeletePending = FALSE;
                }
            }
        }
    }
}

DWORD URL_CONTAINER::Release(BOOL fTryToUnmap)
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC.
{
    DWORD dwRefCount = 0;

    INET_ASSERT(_dwRefCount);
    if (_dwRefCount)
    {
        dwRefCount = --_dwRefCount;
        if (fTryToUnmap && _dwRefCount == 0)
        {
            //  Never CONTENT, COOKIES or HISTORY container.
            if (!(_CacheEntryType & (NORMAL_CACHE_ENTRY | COOKIE_CACHE_ENTRY | URLHISTORY_CACHE_ENTRY)))
            {
                if (_fDeletePending)
                {
                    TryToUnmap(0);
                }
            }
        }
    }
    return dwRefCount;
}


BOOL URL_CONTAINER::LockContainer(BOOL *fMustUnlock)
/*++

Routine Description:

    This function waits for the container to be free.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    _dwLastReference = GetTickCount();

    *fMustUnlock = FALSE;

    if( _MutexHandle == NULL )
    {
        // Bad mutex handle.
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "Container Mutex Handle is NULL.\n" ));
        return FALSE;
    }

    //TcpsvcsDbgPrint((DEBUG_ERRORS, "LockContainer called by thread %x\n", GetCurrentThreadId()));

    //
    // Wait the for the mutex to be signalled.
    //

    DWORD Result;

    #if DBG
        DWORD MutexTimeoutCount;
        MutexTimeoutCount = 0;

    Waitagain:

    #endif

    // Check the mutex.
    #if DBG
        Result = WaitForSingleObject(_MutexHandle, MUTEX_DBG_TIMEOUT);
    #else
        Result = WaitForSingleObject(_MutexHandle, INFINITE);
    #endif

    switch ( Result )
    {
        case WAIT_OBJECT_0:

            // Mutex is signalled (normal result). We now have ownership of the mutex.
            // Do a CheckSizeGrowAndRemapAddress.
            _dwTaken++;
#ifdef CHECKLOCK_NORMAL
            _dwThreadLocked = GetCurrentThreadId();
#endif
            *fMustUnlock = TRUE;
            if (_UrlObjStorage)
            {
                if (_UrlObjStorage->CheckSizeGrowAndRemapAddress() != ERROR_SUCCESS)
                {
                    return (FALSE);
                }
            }
            return TRUE;

    #if DBG
        case WAIT_TIMEOUT:

            // Exceeded debug timeout count. Try again.
            MutexTimeoutCount++;
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "Mutex wait time-out (count = %ld).\n", MutexTimeoutCount ));
            goto Waitagain;
    #endif

        case WAIT_ABANDONED :

            // The thread owning the mutex failed to release it before it terminated.
            // We still get ownership of the mutex.
            _dwTaken++;
#ifdef CHECKLOCK_NORMAL
            _dwThreadLocked = GetCurrentThreadId();
#endif
            *fMustUnlock = TRUE;

            TcpsvcsDbgPrint(( DEBUG_ERRORS, "Mutex ABANDONED.\n" ));
            if (_UrlObjStorage)
            {
                if (_UrlObjStorage->CheckSizeGrowAndRemapAddress() != ERROR_SUCCESS)
                    return (FALSE);
            }

            return TRUE;

        case WAIT_FAILED :

            // Failed to obtain mutex.
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "Mutex wait failed (%ld).\n", GetLastError() ));
            return FALSE;

    }

    INET_ASSERT( FALSE );
    return FALSE;
}


VOID URL_CONTAINER::UnlockContainer(VOID)
/*++

Routine Description:

    This function frees the container to be used by someone else.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    BOOL fMustLaunchScavenger = FALSE;

    //TcpsvcsDbgPrint((DEBUG_ERRORS, "UnlockContainer called by thread %x\n", GetCurrentThreadId()));
    _dwLastReference = GetTickCount();

    _dwTaken--;

#ifdef CHECKLOCK_NORMAL
    if( _MutexHandle)
    {
        INET_ASSERT(_dwThreadLocked == GetCurrentThreadId());

        if (_dwTaken == 0)
            _dwThreadLocked = 0;
#endif
        if (_dwTaken == 0)
        {
            fMustLaunchScavenger = _fMustLaunchScavenger;
            _fMustLaunchScavenger = FALSE;
        }
        if (ReleaseMutex( _MutexHandle ) == FALSE )
        {
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "ReleaseMutex failed (%ld).\n", GetLastError() ));
        }

        if (fMustLaunchScavenger)
            LaunchScavenger();

#ifdef CHECKLOCK_NORMAL
    }
#endif
    return;
}

BOOL URL_CONTAINER::UpdateOfflineRedirect
(
    DWORD dwUrlItemOffset, // offset to hash table item of URL entry
    LPCSTR pszUrl,         // URL string
    DWORD cbUrl,           // URL length
    LPCSTR pszRedir        // redirect string
)
/*++
Routine Description:
    Marks a hash table item as allowing a redirect to add trailing slash,
    or creates a new redirect hash table item and memory mapped file entry.

    Addendum: We keep track of redirects in the cache and simulate them when
    offline. Often the redirected URL is the same as the original URL plus
    trailing slash.

WARNING: this function has multiple calls which can grow and remap the
memory map file, invalidating any pointers into the file.  Be careful.

Return Value: TRUE if redirect was cached

--*/

{
    DWORD cbRedir = strlen (pszRedir);
    DWORD dwUrlHash;
    DWORD dwRedirItemOffset;

    // Ignore the redirect URL if same as original URL.
    if (cbRedir == cbUrl && !memcmp(pszUrl, pszRedir, cbRedir))
        return FALSE;

    { // limit scope of pUrlItem

        HASH_ITEM* pUrlItem = (HASH_ITEM*)
            (*_UrlObjStorage->GetHeapStart() + dwUrlItemOffset);

        // Special case trailing slash redirect.
        if (   cbRedir + 1 == cbUrl
            && pszUrl [cbRedir] == '/'
            && !memcmp (pszUrl, pszRedir, cbRedir)
           )
        {
            pUrlItem->SetSlash();
            return TRUE;
        }

        // Record high bits of target URL hash value.
        dwUrlHash = pUrlItem->GetValue();
    }

    //
    // BUGBUG: we do not handle the case of redirect that once added a
    // trailing slash to redirect to another URL altogether.  We should
    // scan the entire hash table slot and unset the trailing slash bit
    // if we find a match.
    //

    REDIR_FILEMAP_ENTRY* pEntry;

    { // limit scope of pRedirItem

        HASH_ITEM* pRedirItem = NULL;

        if (HashFindItem (pszRedir, LOOKUP_REDIR_CREATE, &pRedirItem))
        {
            // Found existing redirect entry; update it.
            pEntry = (REDIR_FILEMAP_ENTRY*) HashGetEntry (pRedirItem);
            INET_ASSERT (pEntry->dwSig == SIG_REDIR);
            pEntry->dwHashValue = dwUrlHash;
            pEntry->dwItemOffset = dwUrlItemOffset;
            INET_ASSERT (!lstrcmp (pEntry->szUrl, pszRedir));
            return TRUE;
        }

        if (!pRedirItem)
            return FALSE;

        dwRedirItemOffset = (DWORD) ((LPBYTE) pRedirItem - *_UrlObjStorage->GetHeapStart()); // 64BIT
    }

    // Created new hash table item so allocate a redir entry.
    DWORD cbEntry = sizeof(*pEntry) + cbRedir;
    pEntry = (REDIR_FILEMAP_ENTRY *) _UrlObjStorage->AllocateEntry (cbEntry);

    // Associate entry with hash table item.
    HASH_ITEM *pRedirItem =
        (HASH_ITEM*) (*_UrlObjStorage->GetHeapStart() + dwRedirItemOffset);
    if (!pEntry)
    {
        pRedirItem->MarkFree();
        return FALSE;
    }
    HashSetEntry (pRedirItem, (LPBYTE) pEntry);
    pRedirItem->SetRedir();

    // Initialize the redirect entry.
    pEntry->dwSig        = SIG_REDIR;
    pEntry->dwHashValue  = dwUrlHash;
    pEntry->dwItemOffset = dwUrlItemOffset;
    memcpy (pEntry->szUrl, pszRedir, cbRedir + 1);

    return TRUE;
}

LPSTR URL_CONTAINER::GetPrefixMap()
{
    return TEXT("");
}

LPSTR URL_CONTAINER::GetVolumeLabel()
{
    return TEXT("");
}

LPSTR URL_CONTAINER::GetVolumeTitle()
{
    return TEXT("");
}

DWORD URL_CONTAINER::AddUrl (AddUrlArg* pArgs)

/*++

Routine Description:

    This member functions adds an URL to the container and moves the
    cache file to cache path.

Arguments:

    UrlName : pointer to an URL string.

    lpszNewFileName : pointer to a cache file (full) name.

    ExpireTime : expire time of the file.

    LastModifiedTime : Last modified time of this file. if this value is
        zero, current time is set as the last modified time.

    dwCacheEntryType : type of this new entry.

    HeaderInfo : if this pointer is non-NULL, it stores the HeaderInfo
        data as part of the URL entry in the memory mapped file, otherwise
        the app may store it else where. The size of the header info is
        specified by the HeaderSize parameter.

    dwHeaderSize : size of the header info associated with this URL, this
        can be non-zero even if the HeaderInfo specified above is NULL.

    FileExtension : file extension used to create this file.

Return Value:

    Windows Error Code.

--*/
{
    DWORD dwReturn;
    BOOL fMustUnlock;
    DWORD dwCurrentOffset;
    DWORD dwUrlNameSize;
    LPTSTR FileName;
    DWORD dwFileNameSize;
    DWORD dwUrlFileExtSize;
    DWORD dwEntrySize;
    FILETIME ftCreate;
    DWORD dwFileSize;
    LONGLONG CacheSize, CacheLimit;
    DWORD dwItemOffset;
    BOOL fUpdate;
    LPURL_FILEMAP_ENTRY NewEntry;
    HASH_ITEM *pItem;
    GROUP_ENTRY* pGroupEntry;
    GroupMgr    gm;
    DWORD dwFileMapEntrySizeAligned;

    if (!LockContainer(&fMustUnlock))
    {
        dwReturn = GetLastError();
        goto exit;
    }

    // Calculate dwUrlNameSize.
    dwUrlNameSize = lstrlen(pArgs->pszUrl) + 1;

    // FileName points to the filename sans cachepath.  Calculate dwFileNameSize.
    if(pArgs->pszFilePath)
    {

        // Is this an absolute path (edited)?
        if (!(pArgs->dwEntryType & EDITED_CACHE_ENTRY))
        // No, so find the last slash in the file name path.
        // This delimits the file name.

        {
            CHAR  *pSlash = NULL,
                  *ptr = (LPSTR) pArgs->pszFilePath;
            while (*ptr)
            {
                if (*ptr == DIR_SEPARATOR_CHAR)
                    pSlash = ptr;

                ptr=CharNext(ptr);
            }
            FileName = pSlash  + 1;
            dwFileNameSize = (DWORD) (ptr - FileName + 1); // 64BIT
        }
        // Have an absolute path so use the full path
        else
        {
            FileName = (char *)pArgs->pszFilePath;
            dwFileNameSize = lstrlen(FileName)+1;
        }
    }
    else
    {
        FileName = NULL;
        dwFileNameSize = 0;
    }

    // Calculate dwUrlFileExtSize
    if (FileName)
    {
        dwUrlFileExtSize =
            (pArgs->pszFileExt? (lstrlen(pArgs->pszFileExt) + 1) : sizeof("txt"));

    }
    else
    {
        dwUrlFileExtSize = 0;
    }

    // Get the file size.
    if (!pArgs->pszFilePath)
        dwFileSize = 0;
    else
    {
        if (!pArgs->dwFileSize)
        {
            WIN32_FILE_ATTRIBUTE_DATA FileAttrData;

            dwReturn = GetFileSizeAndTimeByName(pArgs->pszFilePath, &FileAttrData);
            if (FileAttrData.nFileSizeHigh) // Accept 0 length files too ...
            {
                // Reject files of length 0 or larger than 4 GB.
                dwReturn = ERROR_INVALID_DATA;
                goto exit;
            }
            dwFileSize = FileAttrData.nFileSizeLow;
            ftCreate = FileAttrData.ftCreationTime;
        }
        else
        {
            dwFileSize = pArgs->dwFileSize;
            ftCreate = pArgs->ftCreate;
        }
    }

    dwReturn = ERROR_SUCCESS;

    { // Open a new block to limit the scope of pointer variables.

        HASH_ITEM *pItem;

        // Lookup or create a hash item for the URL entry.

        fUpdate = HashFindItem (pArgs->pszUrl, LOOKUP_URL_CREATE, &pItem);

//////////////////////////////////////////////////////////////////////////////
// WARNING: LOOKUP_URL_CREATE set,thus the file might be grown and remapped //
// so all pointers into the file must be recalculated from their offsets!   //
//////////////////////////////////////////////////////////////////////////////

        if (fUpdate)
        {
            // Check existing entry for refcount.
            URL_FILEMAP_ENTRY* pOld = HashGetEntry (pItem);
            if (pOld->NumReferences)
            {
                dwReturn = ERROR_SHARING_VIOLATION;
                goto exit;
            }

            // Validate the size of data to be copied from existing entry.
            // If the entry version is IE5, the value could be random, so
            // force it to the correct value.  If the value is otherwise
            // bogus, also set it to the IE5 size for safety.
            if (    pOld->bVerCreate == ENTRY_VERSION_IE5
                ||  pOld->CopySize > pOld->nBlocks * NORMAL_ENTRY_SIZE
                ||  pOld->CopySize & 3 // should be dword aligned
               )
            {
                pOld->CopySize = ENTRY_COPYSIZE_IE5;
            }

            dwFileMapEntrySizeAligned = pOld->CopySize;

        }
        else if (!pItem)
        {
            // Item was not found but could not allocate another hash table.
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        else // brand new entry
        {
           dwFileMapEntrySizeAligned = ENTRY_COPYSIZE_CURRENT;
        }

        // Save offsets in case the memmap file must be grown and remapped.
        dwItemOffset = (DWORD) ((LPBYTE) pItem - *_UrlObjStorage->GetHeapStart()); // 64BIT
    }

//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

    // Create/update offline redirect as necessary.
    if (pArgs->pszRedirect)
    {
        UpdateOfflineRedirect
            (dwItemOffset, pArgs->pszUrl, dwUrlNameSize - 1, pArgs->pszRedirect);
    }

    DWORD dwUrlNameSizeAligned;
    DWORD dwFileNameSizeAligned;
    DWORD dwHeaderSizeAligned;
    DWORD dwUrlFileExtSizeAligned;

    // Calculate the total size of the entry, rounding up for alignment.
    dwUrlNameSizeAligned        = ROUNDUPDWORD(dwUrlNameSize);
    dwFileNameSizeAligned       = ROUNDUPDWORD(dwFileNameSize);
    dwHeaderSizeAligned         = (pArgs->pbHeaders) ? ROUNDUPDWORD(pArgs->cbHeaders) : 0;
    dwUrlFileExtSizeAligned     = ROUNDUPDWORD(dwUrlFileExtSize);

    INET_ASSERT (dwFileMapEntrySizeAligned % sizeof(DWORD) == 0);
    INET_ASSERT (sizeof(FILEMAP_ENTRY) % sizeof(DWORD) == 0);
    dwFileMapEntrySizeAligned += sizeof(FILEMAP_ENTRY);

    dwEntrySize =         dwFileMapEntrySizeAligned
                        + dwUrlNameSizeAligned
                        + dwFileNameSizeAligned
                        + dwHeaderSizeAligned
                        + dwUrlFileExtSizeAligned;

    NewEntry = (LPURL_FILEMAP_ENTRY) _UrlObjStorage->AllocateEntry(dwEntrySize);

//////////////////////////////////////////////////////////////////
// END WARNING: The file might have grown and remapped, so all  //
// pointers into the file after this point must be recalculated //
// from offsets.                                                //
//////////////////////////////////////////////////////////////////

    // Restore pointer to hash table item.
    pItem = (HASH_ITEM*) (*_UrlObjStorage->GetHeapStart() + dwItemOffset);

    if (!NewEntry)
    {
        if (!fUpdate)
            pItem->MarkFree();
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }


/*
    Handle any differences between cache entry versions.

    IE5 inits bVerUpdate to 0 when creating an entry, but incorrectly preserves 
    the value when updating instead of forcing it to 0.  Fortunately IE6 does 
    not care since IE5 will not be able to find an identity-specific entry in 
    order to update it.

    However, we should have a safety hatch for IE7+ to realize that a downlevel 
    urlcache updated its entry and just copied over the opaque data.  Of course 
    the solution will be a total ugly hack.  One possibility is for the uplevel 
    urlcache to set dwUrlNameOffset to be dwCopySizeAligned + 4.  This will 
    leave an uninitialized dword "hole" between the fixed fields and the 4 
    variable size fields.  Once this hack is used, by IE7 for example, it 
    should correctly set bVerUpdate2 so that IE8+ can detect both IE5-6 and IE7 
    updates without creating further holes.
*/

    if (fUpdate)
    {
        URL_FILEMAP_ENTRY* pOld = HashGetEntry (pItem);

        if ((pOld->dwSig != SIG_URL)
            || (pOld->CopySize > PAGE_SIZE))
        {
            INET_ASSERT(FALSE);
            pItem->MarkFree();
            _UrlObjStorage->FreeEntry(NewEntry);
            dwReturn = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
        
        CopyMemory (((LPBYTE) NewEntry) + sizeof(FILEMAP_ENTRY),
                    ((LPBYTE) pOld) + sizeof(FILEMAP_ENTRY),
                    pOld->CopySize);
        INET_ASSERT (NewEntry->CopySize == pOld->CopySize);
        if (NewEntry->CopySize == ENTRY_COPYSIZE_IE5)
            NewEntry->bVerCreate = ENTRY_VERSION_IE5;
        INET_ASSERT (NewEntry->bVerCreate == pOld->bVerCreate);
    }
    else
    {
        NewEntry->CopySize   = ENTRY_COPYSIZE_CURRENT;
        NewEntry->bVerCreate = ENTRY_VERSION_CURRENT;
        NewEntry->bVerUpdate = ENTRY_VERSION_CURRENT;
    }

    // Invalidate the signature during the update.
    NewEntry->dwSig = 0;

    // We must set entry type, file size, and exempt delta before
    // calling SetExemptDelta.  We leave the sticky bit out of the
    // entry type in case we have no more room for sticky items.
    NewEntry->CacheEntryType = _CacheEntryType
        | (pArgs->dwEntryType & ~STICKY_CACHE_ENTRY);
    NewEntry->dwFileSize = dwFileSize;

    if (!fUpdate)
    {
        // This is a brand new entry.
        NewEntry->dwGroupOffset = 0;
        NewEntry->NumAccessed   = 1;

        if (pArgs->fImage)
        {
            NewEntry->bSyncState = SYNCSTATE_IMAGE;
            BETA_LOG (SYNCSTATE_IMAGE);
        }
        else
        {
            NewEntry->bSyncState = SYNCSTATE_VOLATILE;
            if (IsContentContainer())
                BETA_LOG (SYNCSTATE_VOLATILE);
        }

        NewEntry->dwExemptDelta = 0;
        if (pArgs->dwEntryType & STICKY_CACHE_ENTRY)
            SetExemptDelta (NewEntry, 24 * 60 * 60, dwItemOffset); // one day

    }
    else // if (fUpdate)
    {
        URL_FILEMAP_ENTRY* ExistingEntry = HashGetEntry (pItem);

        // This is an update of an existing entry.
        INET_ASSERT (NewEntry->dwGroupOffset == ExistingEntry->dwGroupOffset);

        NewEntry->NumAccessed++;  // BUGBUG: blowing off wraparound

        if (ExistingEntry->bSyncState == SYNCSTATE_STATIC)
            BETA_LOG (SYNCSTATE_STATIC_VOLATILE);

        NewEntry->bSyncState = SYNCSTATE_VOLATILE;

        if (ExistingEntry->dwExemptDelta ||
            ExistingEntry->CacheEntryType & STICKY_CACHE_ENTRY )
        {
            // If item was previously sticky, it is preserved upon update.
            NewEntry->dwExemptDelta = ExistingEntry->dwExemptDelta;
            INET_ASSERT (ExistingEntry->CacheEntryType | STICKY_CACHE_ENTRY);
            NewEntry->CacheEntryType |= STICKY_CACHE_ENTRY;
        }
        else
        {
            // If the item wasn't previously sticky, it might be made so.
            NewEntry->dwExemptDelta = 0;
            if (pArgs->dwEntryType & STICKY_CACHE_ENTRY)
                SetExemptDelta (NewEntry, 24 * 60 * 60 * 1000, dwItemOffset); // one day
        }

        // if belongs to a group, adjust the group usage
        if( ExistingEntry->dwGroupOffset &&
            ExistingEntry->dwFileSize != NewEntry->dwFileSize )
        {
            LONGLONG llDelta = RealFileSize(NewEntry->dwFileSize) -
                               RealFileSize(ExistingEntry->dwFileSize);
            if( pItem->HasMultiGroup() )
            {
                // multiple group
                if( gm.Init(this) )
                {
                    // dwGroupOffset is now offset to head of group list
                    gm.AdjustUsageOnList(ExistingEntry->dwGroupOffset, llDelta);
                }
            }
            else
            {
                // single group
                pGroupEntry = _UrlObjStorage->ValidateGroupOffset(
                    ExistingEntry->dwGroupOffset, pItem);

                // WARNING: quota can be reached by usage increase
                if( pGroupEntry )
                {
                    AdjustGroupUsage(pGroupEntry, llDelta);
                }
            }
        }

        // Delete the old entry if it's not an installed entry or an EDITED_CACHE_ENTRY
        // (Unless the new entry replacing it is also an EDITED_CACHE_ENTRY),
        // either way the hash table item is preserved. We also check the filesize if
        // we've determined the old entry was ECE but the new one is not, just in case
        // the client deletes the file but doesn't get around to deleting from the
        // cache index. The logic is optimized for the most likely case (want to del).
        if ((ExistingEntry->DirIndex != INSTALLED_DIRECTORY_KEY)
            && (!(ExistingEntry->CacheEntryType & EDITED_CACHE_ENTRY)))
        {
            // IDK=0 ECE=0 NECE=? FS=?
            DeleteUrlEntry (ExistingEntry, NULL, SIG_UPDATE);
        }
        else if (ExistingEntry->DirIndex != INSTALLED_DIRECTORY_KEY)
        {
            // IDK=0 ECE=1 NECE=? FS=?
            if (NewEntry->CacheEntryType & EDITED_CACHE_ENTRY)
            {
                // IDK=0 ECE=1 NECE=1 FS=?
                DeleteUrlEntry (ExistingEntry, NULL, SIG_UPDATE);
            }
            else
            {
                // IDK=0 ECE=1 NECE=0 FS=?
                // Only want to go out to the FS to get filesize if absolutely necessary

                WIN32_FILE_ATTRIBUTE_DATA FileAttrData;

                FileAttrData.nFileSizeLow = 0;
                dwReturn = GetFileSizeAndTimeByName(
                    (LPTSTR) OFFSET_TO_POINTER (ExistingEntry, ExistingEntry->InternalFileNameOffset),
                    &FileAttrData);
                if (!FileAttrData.nFileSizeLow)
                {
                    // IDK=0 ECE=1 NECE=0 FS=0 or not-exist
                    // if filesize is zero, might as well trounce it
                    DeleteUrlEntry (ExistingEntry, NULL, SIG_UPDATE);
                }
                else if (dwReturn == ERROR_SUCCESS)
                {
                    // IDK=0 ECE=1 NECE=0 FS>0
                    // found a non-zero length file
                    _UrlObjStorage->FreeEntry(NewEntry);
                    pItem->MarkFree();
                    dwReturn = ERROR_SHARING_VIOLATION;
                    goto exit;
                }
            }
        }
        else
        {
            // IDK=1 ECE=? NECE=? FS=?
            // Installed directory item
            _UrlObjStorage->FreeEntry(NewEntry);
            pItem->MarkFree();
            dwReturn = ERROR_SHARING_VIOLATION;
            goto exit;
        }

    } // end else if (fUpdate)

    // Record the new offset in the hash table item.
    HashSetEntry (pItem, (LPBYTE) NewEntry);

    // Initialize NewEntry fields.
    NewEntry->dwRedirHashItemOffset = 0;
    NewEntry->NumReferences        = 0;
    NewEntry->HeaderInfoSize       = pArgs->cbHeaders;
    NewEntry->LastModifiedTime     = pArgs->qwLastMod;
    GetCurrentGmtTime ((FILETIME*) &NewEntry->LastAccessedTime);

    FileTime2DosTime(
        *LONGLONG_TO_FILETIME( &(pArgs->qwExpires) ),
        &(NewEntry->dostExpireTime)   );

    FileTime2DosTime(
        *LONGLONG_TO_FILETIME( &(pArgs->qwPostCheck) ),
        &(NewEntry->dostPostCheckTime)   );

    // GetDirIndex will fail if the entry is stored at an absolute path outside
    // the cache dir, This is valid for EDITED_CACHE_ENTRYs such as offline
    // Office9 docs. Even tho the call fails in this case, NewEntry->DirIndex
    // will be set to NOT_A_CACHE_SUBDIRECTORY

    DWORD dwIndex;
    if ((!_FileManager->GetDirIndex((LPSTR) pArgs->pszFilePath, &dwIndex))
        && !(pArgs->dwEntryType & EDITED_CACHE_ENTRY))
    {
        _UrlObjStorage->FreeEntry(NewEntry);
        pItem->MarkFree();
        dwReturn = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    NewEntry->DirIndex = (BYTE) dwIndex;

    // If this entry points to the store directory, set
    // the INSTALLED_CACHE_ENTRY bit so that retrieval
    // from cache will not result in requests to the wire
    // from wininet except for refresh requests.

    if ((NewEntry->DirIndex == INSTALLED_DIRECTORY_KEY) ||
        (pArgs->dwEntryType & EDITED_CACHE_ENTRY))
    {
        NewEntry->CacheEntryType |= INSTALLED_CACHE_ENTRY;
    }

    FileTimeToDosDateTime( (FILETIME *)&(NewEntry->LastAccessedTime),
                           (LPWORD)&(NewEntry->dostLastSyncTime),
                           ((LPWORD)&(NewEntry->dostLastSyncTime)+1));


    // The URL_FILEMAP_ENTRY will point to
    //
    // [URL_FILEMAP_ENTRY][UrlName][FileName][HeaderInfo][FileExtension]
    //                    ^        ^         ^           ^
    //                    |        |         |           FileExtensionOffset
    //                    |        |         |
    //                    |        |         HeaderInfoOffset
    //                    |        |
    //                    |        FileNameOffset
    //                    |
    //                    UrlNameOffset
    //


    dwCurrentOffset = dwFileMapEntrySizeAligned;
    NewEntry->UrlNameOffset = dwCurrentOffset;

    // Copy UrlName into NewEntry
    memcpy((LPSTR) OFFSET_TO_POINTER(NewEntry, NewEntry->UrlNameOffset),
        pArgs->pszUrl, dwUrlNameSize);

    dwCurrentOffset += dwUrlNameSizeAligned;

    // Copy FileName into NewEntry
    if(FileName)
    {
        NewEntry->InternalFileNameOffset = dwCurrentOffset;
        memcpy((LPTSTR) OFFSET_TO_POINTER (NewEntry,
            NewEntry->InternalFileNameOffset), FileName, dwFileNameSize);

        // Get file creation time of cache file.
        if (!pArgs->pszFilePath)
            NewEntry->dostFileCreationTime = 0;
        else
        {
            FileTimeToDosDateTime (&ftCreate,
                (LPWORD)&(NewEntry->dostFileCreationTime),
                ((LPWORD)&(NewEntry->dostFileCreationTime)+1));
        }

        dwCurrentOffset += dwFileNameSizeAligned;
    }
    else
    {
        NewEntry->InternalFileNameOffset = 0;
        NewEntry->dostFileCreationTime = 0;
    }

    // Copy HeaderInfo into NewEntry
    if(pArgs->pbHeaders)
    {
        NewEntry->HeaderInfoOffset = dwCurrentOffset;
        memcpy((LPBYTE)NewEntry + NewEntry->HeaderInfoOffset,
            pArgs->pbHeaders, pArgs->cbHeaders);
        dwCurrentOffset += dwHeaderSizeAligned;
    }
    else
    {
        NewEntry->HeaderInfoOffset = 0;
    }

    // Copy FileExtension into NewEntry
    if(pArgs->pszFileExt)
    {
        NewEntry->FileExtensionOffset = dwCurrentOffset;
        memcpy ((LPTSTR) ((LPBYTE)NewEntry + NewEntry->FileExtensionOffset),
            pArgs->pszFileExt, dwUrlFileExtSize);
        dwCurrentOffset += dwUrlFileExtSizeAligned;
    }
    else
    {
        NewEntry->FileExtensionOffset = 0;
    }

    // Restore the signature.
    NewEntry->dwSig = SIG_URL;

    // Increment the FileManager's count
    if (FileName)
    {
        // This is a no-op for the COOKIES and HISTORY containers.
        _FileManager->NotifyCommit(NewEntry->DirIndex);

        // Adjust CacheSize if not an installed entry or stored outside of cache dir.
        // If disk quota is exceeded, initiate cleanup.
        // NOTE: this attempts to take the critical section, so we must defer it
        // until we have released the container mutex to avoid potential deadlock
        if ((NewEntry->DirIndex != INSTALLED_DIRECTORY_KEY) && (!(NewEntry->CacheEntryType & EDITED_CACHE_ENTRY)))
        {
            _dwBytesDownloaded += (DWORD)RealFileSize(dwFileSize);
            _dwItemsDownloaded++;

            _UrlObjStorage->AdjustCacheSize(RealFileSize(dwFileSize));
            CacheSize = _UrlObjStorage->GetCacheSize();
            CacheLimit = _UrlObjStorage->GetCacheLimit();
            if (CacheSize > CacheLimit)
                _fMustLaunchScavenger = TRUE;

            // We also want to scavenge if, even though there's plenty of space available in the
            // cache, the amount of total disk space available falls below a certain threshold.
            // We'll arbitrarily set the threshold to be 4MB (below which, things are likely to get
            // painful.

            if (!_fMustLaunchScavenger && ((_dwBytesDownloaded>(1024*1024)) || (_dwItemsDownloaded>100)))
            {
                DWORDLONG dlSize = 0;

                _dwBytesDownloaded = _dwItemsDownloaded = 0;
                if (GetDiskInfo(_CachePath, NULL, &dlSize, NULL))
                {
                    _fMustLaunchScavenger = (BOOL)(dlSize <= (DWORDLONG)GlobalDiskUsageLowerBound);
                }
            }
        }
    }

    // Flush index if this is an edited document or Cookie to mitigate the risk of dirty shutdown losing changes a client
    // like Office9 might have made (they store edited docs in our cache).

    if (pArgs->dwEntryType & EDITED_CACHE_ENTRY )
    {
        FlushViewOfFile((LPCVOID)NewEntry, dwCurrentOffset);
    }

    if( pArgs->dwEntryType & COOKIE_CACHE_ENTRY )
    {
        FlushViewOfFile( (void*)(*_UrlObjStorage->GetHeapStart()), 0 );
    }


    // Notification
    // If item was previously sticky, it is preserved upon update.
    // only need to report non sticky -> sticky
    if( !fUpdate )
    {
        NotifyCacheChange(CACHE_NOTIFY_ADD_URL, dwItemOffset);
    }
    else
    {
        NotifyCacheChange(CACHE_NOTIFY_UPDATE_URL, dwItemOffset);
    }

exit:

    if (fMustUnlock) UnlockContainer();
    return dwReturn;
}



DWORD URL_CONTAINER::AddLeakFile (LPCSTR pszFilePath)
{
    DWORD dwReturn;

    BOOL fUnlock;
    if (!LockContainer(&fUnlock))
    {
        if(fUnlock)
            ReleaseMutex(_MutexHandle);

        return GetLastError();
    }

    //
    // Calculate the size of the filename, after the last slash.
    //

    LPSTR pszSlash = NULL;
    LPSTR pszScan = (LPSTR) pszFilePath;
    while (*pszScan)
    {
        if (*pszScan == DIR_SEPARATOR_CHAR)
            pszSlash = pszScan;

        pszScan = CharNext(pszScan);
    }
    LPSTR pszFileName = pszSlash + 1;
    DWORD cbFileName = (DWORD) (pszScan - pszFileName + 1); // 64BIT

    //
    // Determine the directory bucket.
    //

    DWORD nDirIndex;
    if (!_FileManager->GetDirIndex((LPSTR) pszFilePath, &nDirIndex))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Get the file size and create time.
    //

    WIN32_FILE_ATTRIBUTE_DATA FileAttrData;
    dwReturn = GetFileSizeAndTimeByName (pszFilePath, &FileAttrData);
    if (!FileAttrData.nFileSizeLow || FileAttrData.nFileSizeHigh)
    {
        // Reject files of length 0 or larger than 4 GB.
        dwReturn = ERROR_INVALID_DATA;
        goto exit;
    }

    //
    // Allocate a leaked file entry.
    //

//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////


    URL_FILEMAP_ENTRY* NewEntry;
    NewEntry = (URL_FILEMAP_ENTRY*) _UrlObjStorage->AllocateEntry
        (sizeof(URL_FILEMAP_ENTRY) + cbFileName);
    if (!NewEntry)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

//////////////////////////////////////////////////////////////////
// END WARNING: The file might have grown and remapped, so all  //
// pointers into the file after this point must be recalculated //
// from offsets.                                                //
//////////////////////////////////////////////////////////////////


    //
    // Fill only the fields important to WalkLeakList method.
    //

    NewEntry->dwSig = SIG_LEAK;
    NewEntry->dwFileSize = FileAttrData.nFileSizeLow;
    NewEntry->InternalFileNameOffset = sizeof(URL_FILEMAP_ENTRY);
    memcpy ((LPBYTE) (NewEntry + 1), pszFileName, cbFileName);
    NewEntry->DirIndex = (BYTE) nDirIndex;
    FileTimeToDosDateTime( &FileAttrData.ftCreationTime,
                           (LPWORD)&(NewEntry->dostFileCreationTime),
                           ((LPWORD)&(NewEntry->dostFileCreationTime)+1));
    NewEntry->NumReferences = 0;

    //
    // Add this entry on to the head of the scavenger leak list.
    //
    _UrlObjStorage->GetHeaderData
        (CACHE_HEADER_DATA_ROOT_LEAK_OFFSET, &NewEntry->dwNextLeak);
    _UrlObjStorage->SetHeaderData
        (CACHE_HEADER_DATA_ROOT_LEAK_OFFSET,
         OffsetFromPointer (NewEntry));

    //
    // Update the cache usage and directory count.
    //

    _UrlObjStorage->AdjustCacheSize(RealFileSize(NewEntry->dwFileSize));
    _FileManager->NotifyCommit(NewEntry->DirIndex);

    //
    // We could check here if usage exceeds quota then launch scavenger,
    // but it will probably happen soon enough anyway upon AddUrl.
    //

    dwReturn = ERROR_SUCCESS;

exit:
    if (fUnlock)
        UnlockContainer();

    return dwReturn;
}

DWORD URL_CONTAINER::RetrieveUrl
(
    LPCSTR UrlName,
    LPCACHE_ENTRY_INFO* ppInfo,
    LPDWORD pcbInfo,
    DWORD dwLookupFlags, // e.g. redirect
    DWORD dwRetrievalFlags
)

/*++

Routine Description:

    This member function retrives an url from the cache. The url is marked
    as referenced, so that caller should call UnlockUrl when he is done
    using it.

Arguments:

    UrlName : pointer to the url name.

    ppInfo :  ptr to ptr to an entry info buffer, where the url entry info
        is returned.

    pcbInfo : pointer to a DWORD location containing the size of the
        above buffer, on return it has the size of the buffer consumed or
        size of the buffer required for successful retrieval.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPURL_FILEMAP_ENTRY UrlEntry;
    BOOL fMustUnlock;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto exit;
    }

    // Find the item.
    HASH_ITEM *pItem;
    if (!HashFindItem (UrlName, dwLookupFlags, &pItem))
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // Get the hash entry.
    UrlEntry = HashGetEntry (pItem);
    if (UrlEntry->InternalFileNameOffset == 0)
    {
        Error = ERROR_INVALID_DATA;
        goto exit;
    }
    
    // For content container, check that username matches.
    if (IsContentContainer())
    {
        LPSTR pszHeaders = ((LPSTR) UrlEntry) + UrlEntry->HeaderInfoOffset;
        TcpsvcsDbgPrint((DEBUG_CONTAINER,
            "RetrieveUrl (contain.cxx): IsContentContainer() = TRUE; IsCorrectUser() = %s \r\n",
            (IsCorrectUser(pszHeaders, UrlEntry->HeaderInfoSize))? "TRUE" : "FALSE"
            ));

        if (!IsCorrectUser(pszHeaders, UrlEntry->HeaderInfoSize))
        {
            Error = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
    }
    else
    {
        TcpsvcsDbgPrint((DEBUG_CONTAINER,
            "RetrieveUrl (contain.cxx): IsContentContainer() = FALSE\r\n"));
    }

    // Hide sparse cache entries from non-wininet clients.
    if (UrlEntry->CacheEntryType & SPARSE_CACHE_ENTRY
        && !(dwLookupFlags & LOOKUP_BIT_SPARSE))
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // Found the entry. Copy URL info in the return buffer first.
    Error = CopyUrlInfo( UrlEntry, ppInfo, pcbInfo, dwRetrievalFlags );
    if( Error != ERROR_SUCCESS )
        goto Cleanup;

    if ((*ppInfo)->CacheEntryType & SPARSE_CACHE_ENTRY)
    {
        // Delete the item and entry but not the file.
        UrlEntry->InternalFileNameOffset = 0;
        DeleteUrlEntry (UrlEntry, pItem, SIG_DELETE);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Verify the file size is what it is supposed to be.
    if (dwRetrievalFlags & RETRIEVE_WITH_CHECKS)
    {
        DWORD dwFileSize;
        Error = GetFileSizeByName
            ((LPCSTR)((*ppInfo)->lpszLocalFileName), &dwFileSize);
        if (Error != ERROR_SUCCESS)
        {
            Error = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        if (dwFileSize != UrlEntry->dwFileSize)
        {
            Error = ERROR_INVALID_DATA;
            goto Cleanup;
        }
    }

    // Hack to keep track of if any entries have locks.
    GlobalRetrieveUrlCacheEntryFileCount++;

    // Increment the reference count before returning.
    if (UrlEntry->NumReferences)
    {
        if( !pItem->IsLocked() )
        {
            //
            // corrupted index file
            // entry says it's locked, hash table say it's not
            // believe the hash table by fixing up the entry
            //
            INET_ASSERT (FALSE);
            UrlEntry->NumReferences = 0;
        }
    }
    else
        pItem->SetLocked();
    UrlEntry->NumReferences++;

    // Update last accessed time.
    GetCurrentGmtTime ((FILETIME*) &UrlEntry->LastAccessedTime);

    // And the number of times this was accessed
    UrlEntry->NumAccessed++;

Cleanup:

    if (Error != ERROR_SUCCESS)
    {
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "RetrieveUrl call failed, %ld.\n",
            Error ));
        SetLastError(Error);
    }
exit:
    if (fMustUnlock) UnlockContainer();
    return Error;
}


DWORD URL_CONTAINER::DeleteUrl(LPCSTR  UrlName)
/*++

Routine Description:

    This member function deletes the specified url from the cache.

Arguments:

    UrlName : pointer to the url name.

Return Value:

    Windows Error Code.

--*/
{
    BOOL fMustUnlock;
    DWORD Error;
    URL_FILEMAP_ENTRY *pEntry;
    HASH_ITEM *pItem;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto exit;
    }

    // Find (and validate) the entry.
    if (!HashFindItem (UrlName, LOOKUP_URL_NOCREATE, &pItem))
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    pEntry = HashGetEntry(pItem);

    // Delete the hash table item and entry from the index.
    Error = DeleteUrlEntry (pEntry, pItem, SIG_DELETE);

    TcpsvcsDbgPrint((DEBUG_CONTAINER,
        "DeleteUrl: RefCount=%d, DeletePending=%d \r\n",
        pEntry->NumReferences,
        (pEntry->CacheEntryType & PENDING_DELETE_CACHE_ENTRY)? 1 : 0
        ));

    // Notify
    NotifyCacheChange(
        CACHE_NOTIFY_DELETE_URL,
        (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
    );

exit:
    if (fMustUnlock) UnlockContainer();
    return Error;
}


/*++
Routine Description:
    This member functions deletes an URL from the container and also
    deletes the cache file from cache path.

    dwSig - if we must put an uplevel entry on the async fixup list,
       this param distinguishes between updates and deletions.

Return Value:
    Windows Error Code.
--*/

DWORD URL_CONTAINER::DeleteUrlEntry
    (URL_FILEMAP_ENTRY* pEntry, HASH_ITEM *pItem, DWORD dwSig)
{
    INET_ASSERT (pItem? dwSig == SIG_DELETE : dwSig == SIG_UPDATE);

    if (pEntry->dwSig != SIG_URL)
    {
        INET_ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    DWORD Error;
    GROUP_ENTRY* pGroupEntry = NULL;
    GroupMgr     gm;

    // Check for locked entry.
    if (pEntry->NumReferences)
    {
        // Mark the entry for pending delete.
        pEntry->CacheEntryType |= PENDING_DELETE_CACHE_ENTRY;
        Error = ERROR_SHARING_VIOLATION;
        goto Cleanup;
    }

    // If the entry version is beyond our understanding...
    if (pEntry->bVerCreate & ENTRY_VERSION_NONCOMPAT_MASK)
    {
        INET_ASSERT (!(pEntry->bVerCreate & ENTRY_VERSION_NONCOMPAT_MASK));

        pEntry->dwSig = dwSig; // mark as either updated or deleted

        // Add entry to head of fixup list.
        _UrlObjStorage->GetHeaderData
            (CACHE_HEADER_DATA_ROOT_FIXUP_OFFSET, &pEntry->dwNextLeak);
        _UrlObjStorage->SetHeaderData
            (CACHE_HEADER_DATA_ROOT_FIXUP_OFFSET,
             OffsetFromPointer (pEntry));

        // Increment count of items on fixup list, maybe trigger fixup.
        DWORD dwCount, dwTrigger;
        _UrlObjStorage->GetHeaderData
            (CACHE_HEADER_DATA_ROOT_FIXUP_COUNT, &dwCount);
        _UrlObjStorage->GetHeaderData
            (CACHE_HEADER_DATA_ROOT_FIXUP_TRIGGER, &dwTrigger);
        if (++dwCount > dwTrigger)
            _fMustLaunchScavenger = TRUE;
        _UrlObjStorage->SetHeaderData
            (CACHE_HEADER_DATA_ROOT_FIXUP_COUNT, dwCount);

        goto delete_hash_item;
    }

    // If group associated, Adjust Group's disk Usage
    // if pItem == NULL, we should perserve all the group info
    if( pEntry->dwGroupOffset && pItem )
    {
        if( pItem->HasMultiGroup() )
        {
            // multiple group
            if( gm.Init(this) )
            {
                // dwGroupOffset now offset to head of group list
                gm.AdjustUsageOnList(
                    pEntry->dwGroupOffset, -RealFileSize(pEntry->dwFileSize) );
            }
        }
        else
        {
            // single group
            pGroupEntry = _UrlObjStorage->ValidateGroupOffset(
                pEntry->dwGroupOffset, pItem);

            if( pGroupEntry )
            {
                AdjustGroupUsage(pGroupEntry, -RealFileSize(pEntry->dwFileSize) );
            }
        }
    }

    // Delete any associated file.

    if (pEntry->InternalFileNameOffset
        && (pEntry->DirIndex != INSTALLED_DIRECTORY_KEY)
        && (!(pEntry->CacheEntryType & EDITED_CACHE_ENTRY)))
    {
        // Reduce the exempt usage for sticky item (could be an update)
        if ( pEntry->dwExemptDelta ||
             (pEntry->CacheEntryType & STICKY_CACHE_ENTRY) )
            _UrlObjStorage->AdjustExemptUsage(-RealFileSize(pEntry->dwFileSize));

        // Get the absolute path to the file.
        DWORD cb;
        TCHAR szFile[MAX_PATH];
        if (_FileManager->GetFilePathFromEntry(pEntry, szFile, &(cb = MAX_PATH))
            && _FileManager->DeleteOneCachedFile
            (szFile, pEntry->dostFileCreationTime, pEntry->DirIndex))
        {
            // Adjust cache usage.
            _UrlObjStorage->AdjustCacheSize(-RealFileSize(pEntry->dwFileSize));
            _UrlObjStorage->FreeEntry(pEntry);
        }
        else
        {
            // Link the entry at the head of leaked files list.
            INET_ASSERT(pEntry->NumReferences==0);

            pEntry->dwSig = SIG_LEAK;
            _UrlObjStorage->GetHeaderData
                (CACHE_HEADER_DATA_ROOT_LEAK_OFFSET, &pEntry->dwNextLeak);
            _UrlObjStorage->SetHeaderData
                (CACHE_HEADER_DATA_ROOT_LEAK_OFFSET,
                 OffsetFromPointer (pEntry));
        }
    }
    else
    {
        // NOTE: In the case that the entry is in a store (INSTALLED_DIRECTORY_KEY)
        // we do allow the cache entry to be deleted, but we do NOT allow the associated
        // file to be deleted. This at least allows us to delete these entries from
        // the cache without affecting their (permanent) backing store files.

        _UrlObjStorage->FreeEntry(pEntry);
    }

delete_hash_item:

    // Delete this item from the hash table.
    if (pItem)
        pItem->MarkFree();

    Error = ERROR_SUCCESS;

Cleanup:

    TcpsvcsDbgPrint ((DEBUG_ERRORS,
        "URL_CONTAINER::DeleteUrlEntry() returning %ld\n", Error));
    return Error;
}


DWORD URL_CONTAINER::UnlockUrl(LPCSTR UrlName)
/*++

Routine Description:

    This member function unreferences the url entry, so that it can be
    freed up when used no one.

Arguments:

    Url : pointer to an URL name.

Return Value:

    Windows Error Code.

--*/
 {
    DWORD Error;
    BOOL fMustUnlock;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto exit;
    }

    HASH_ITEM* pItem;
    URL_FILEMAP_ENTRY* pEntry;

    // Look up the entry.
    if (HashFindItem (UrlName, LOOKUP_URL_NOCREATE, &pItem))
        pEntry = HashGetEntry (pItem);
    else
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    TcpsvcsDbgPrint((DEBUG_CONTAINER,
        "RefCount=%d, DeletePending=%d \r\n",
        pEntry->NumReferences,
        (pEntry->CacheEntryType & PENDING_DELETE_CACHE_ENTRY)? 1 : 0
        ));

    UnlockItem (pEntry, pItem);

    // Hack to keep track of if any entries have locks.
    GlobalRetrieveUrlCacheEntryFileCount--;
    Error = ERROR_SUCCESS;

exit:
    if (fMustUnlock) UnlockContainer();
    return Error;
}

DWORD URL_CONTAINER::GetUrlInfo
(
    LPCSTR UrlName,
    LPCACHE_ENTRY_INFO* ppUrlInfo,
    LPDWORD UrlInfoLength,
    DWORD dwLookupFlags,
    DWORD dwEntryFlags,
    DWORD dwRetrievalFlags
)
/*++

Routine Description:

    This member function retrieves the url info.

Arguments:

    UrlName : name of the url file (unused now).

    ppUrlInfo : pointer to the pointer to the url info structure that receives the url
        info.

    UrlInfoLength : pointer to a location where length of
        the above buffer is passed in. On return, this contains the length
        of the above buffer that is fulled in.

    dwLookupFlags: flags, e.g. translate through redirects

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    BOOL fMustUnlock;
    LPURL_FILEMAP_ENTRY UrlEntry;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto exit;
    }

    // Look up the entry.
    UrlEntry = HashFindEntry (UrlName, dwLookupFlags);
    if (!UrlEntry)
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // For content container, check that username matches.
    if (IsContentContainer())
    {
        LPSTR pszHeaders = ((LPSTR) UrlEntry) + UrlEntry->HeaderInfoOffset;
        if (!IsCorrectUser(pszHeaders, UrlEntry->HeaderInfoSize))
        {
            Error = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
    }

    // Hide sparse cache entries from non-wininet clients.
    if (UrlEntry->CacheEntryType & SPARSE_CACHE_ENTRY
        && !(dwLookupFlags & LOOKUP_BIT_SPARSE))
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // Find only installed entry types.
    if ((dwEntryFlags & INTERNET_CACHE_FLAG_INSTALLED_ENTRY)
        && (!(UrlEntry->CacheEntryType & INSTALLED_CACHE_ENTRY)))
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    if (UrlInfoLength)
    {
        if (!ppUrlInfo || !*ppUrlInfo)
            *UrlInfoLength = 0;

       Error = CopyUrlInfo( UrlEntry, ppUrlInfo, UrlInfoLength,
                            (dwEntryFlags & INTERNET_CACHE_FLAG_ADD_FILENAME_ONLY ?
                                RETRIEVE_ONLY_FILENAME : 0) |
                            (dwEntryFlags & INTERNET_CACHE_FLAG_GET_STRUCT_ONLY ? 
                                RETRIEVE_ONLY_STRUCT_INFO : 0) |
                            dwRetrievalFlags);
    }
    else
       Error = ERROR_SUCCESS;

exit:
   if (fMustUnlock) UnlockContainer();
   return( Error );
}


DWORD URL_CONTAINER::SetExemptDelta
    (URL_FILEMAP_ENTRY* UrlEntry, DWORD dwExemptDelta, DWORD dwItemOffset)
{
    // Expanded history calls with STICKY_CACHE_ENTRY for no good reason.
    // INET_ASSERT (UrlEntry->FileSize);

    DWORD dwError = ERROR_SUCCESS;

    if (dwExemptDelta)
    {
        if (!UrlEntry->dwExemptDelta)
        {
            // Entry is changing from non-exempt to exempt.
            // (exempt limit check should be done at UpdateStickness
            dwError = UpdateStickness(UrlEntry, URLCACHE_OP_SET_STICKY, dwItemOffset);
            if( dwError != ERROR_SUCCESS )
                goto End;
        }
    }
    else // if (!dwExemptDelta)
    {
        if (UrlEntry->dwExemptDelta)
        {
            // Entry is changing from exempt to non-exempt.
            dwError = UpdateStickness(UrlEntry, URLCACHE_OP_UNSET_STICKY, dwItemOffset);
            if( dwError != ERROR_SUCCESS )
                goto End;
        }
    }

    UrlEntry->dwExemptDelta = dwExemptDelta;
End:
    return dwError;
}



DWORD URL_CONTAINER::SetUrlInfo(LPCSTR UrlName,
                                LPCACHE_ENTRY_INFO UrlInfo, DWORD FieldControl)
/*++

Routine Description:

Arguments:

    UrlName : name of the url file (unused now).

    UrlInfo : pointer to the url info structure that has the url info to
        be set.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPURL_FILEMAP_ENTRY UrlEntry;
    BOOL fMustUnlock;
    HASH_ITEM *pItem;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto Cleanup;
    }

    // Look up the entry.
    if (HashFindItem (UrlName, 0, &pItem))
    {
        UrlEntry = HashGetEntry (pItem);
    }
    else
    {
        UrlEntry = NULL;
    }

    if (!UrlEntry)
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    // Set cache entry ATTRIBUTE.
    if(FieldControl & CACHE_ENTRY_ATTRIBUTE_FC)
        UrlEntry->CacheEntryType = UrlInfo->CacheEntryType;

    // Reset cache entry HITRATE.
    if(FieldControl & CACHE_ENTRY_HITRATE_FC)
        UrlEntry->NumAccessed = UrlInfo->dwHitRate;

    // Set last modified time.
    if(FieldControl & CACHE_ENTRY_MODTIME_FC)
         UrlEntry->LastModifiedTime = FT2LL(UrlInfo->LastModifiedTime);

    // Set expire time.
    if( FieldControl & CACHE_ENTRY_EXPTIME_FC)
    {
        FileTime2DosTime(UrlInfo->ExpireTime, &(UrlEntry->dostExpireTime) );
    }

    // Set last access time.
    if(FieldControl & CACHE_ENTRY_ACCTIME_FC)
        UrlEntry->LastAccessedTime = FT2LL(UrlInfo->LastAccessTime);

    // Set last sync time.
    if(FieldControl & CACHE_ENTRY_SYNCTIME_FC)
    {
        FileTimeToDosDateTime( &(UrlInfo->LastSyncTime),
                                (LPWORD)&(UrlEntry->dostLastSyncTime),
                               ((LPWORD)&(UrlEntry->dostLastSyncTime)+1));

        if (   UrlEntry->bSyncState != SYNCSTATE_VOLATILE
            && UrlEntry->bSyncState < SYNCSTATE_STATIC)
        {
            // See if we should transition to SYNCSTATE_STATIC.
            if (UrlEntry->bSyncState == SYNCSTATE_IMAGE)
            {
                // We have not had the image long enough to
                // conclude it is static.  See if it is older
                // than MIN_AGESYNC.

                LONGLONG qwCreate;
                INET_ASSERT (UrlEntry->dostFileCreationTime);
                DosDateTimeToFileTime(
                    * (LPWORD)&(UrlEntry->dostFileCreationTime),
                    *((LPWORD)&(UrlEntry->dostFileCreationTime)+1),
                    (FILETIME*) &qwCreate);

                if (FT2LL(UrlInfo->LastSyncTime) > qwCreate + MIN_AGESYNC)
                {
                    UrlEntry->bSyncState++;
                }
            }
            else
            {
                if (++UrlEntry->bSyncState == SYNCSTATE_STATIC)
                    BETA_LOG (SYNCSTATE_IMAGE_STATIC);
            }

        }
    }

    // Set exemption delta.
    if (FieldControl & CACHE_ENTRY_EXEMPT_DELTA_FC)
    {
        Error = SetExemptDelta (
            UrlEntry,
            UrlInfo->dwExemptDelta,
            (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
        );

        if (Error != ERROR_SUCCESS)
            goto Cleanup;
    }

    Error = ERROR_SUCCESS;

    NotifyCacheChange(CACHE_NOTIFY_UPDATE_URL,
            (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
        );

Cleanup:
    if (fMustUnlock) UnlockContainer();
    return Error;
}

/*++
Adds or removes a URL from a group.  If adding, may set exemption time.
--*/
DWORD URL_CONTAINER::SetUrlGroup (LPCSTR lpszUrl, DWORD dwFlags, GROUPID GroupId)
{
    DWORD Error;
    BOOL fMustUnlock;
    LPURL_FILEMAP_ENTRY pEntry;
    GROUP_ENTRY* pGroupEntry = NULL;
    GROUP_ENTRY* pOldGroupEntry = NULL;
    GROUPID      gid = 0;
    HASH_ITEM    *pItem = NULL;
    GroupMgr gm;

    if (dwFlags & INTERNET_CACHE_GROUP_NONE)
        return ERROR_SUCCESS;

    if (!GroupId)
        return ERROR_INVALID_PARAMETER;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto exit;
    }

    //
    // HashFindEntry will do the same thing, however, we need pItem
    // here so that we can set/clear the group bit
    //
    if (HashFindItem (lpszUrl, 0, &pItem))
    {
        pEntry = HashGetEntry (pItem);
    }
    else
    {
        pEntry = NULL;
    }

    if (!pEntry)
    {
        Error = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    if( !gm.Init(this) )
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;
    }

    if (dwFlags & INTERNET_CACHE_GROUP_REMOVE)
    {
        // offset to GROUP_ENTRY*
        DWORD dwGEOffset = 0;

        // find the group via GroupOffset
        if( !pEntry->dwGroupOffset )
        {
            Error = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

        // Get GroupEntry Offset
        if( pItem->HasMultiGroup() )
        {
            // multiple group, get from list
            Error = gm.GetOffsetFromList(
                pEntry->dwGroupOffset, GroupId, &dwGEOffset);
            if( Error != ERROR_SUCCESS )
                goto exit;
        }
        else
        {
            dwGEOffset = pEntry->dwGroupOffset;
        }

        // get group entry from the offset
        pGroupEntry = _UrlObjStorage->ValidateGroupOffset(dwGEOffset, pItem);

        if( !pGroupEntry )
        {
            Error = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

        // Remove the group from list
        if( pItem->HasMultiGroup() )
        {
            // remove it from list
            DWORD dwNewHeaderOffset = pEntry->dwGroupOffset;

            Error = gm.RemoveFromGroupList(
                pEntry->dwGroupOffset, dwGEOffset, &dwNewHeaderOffset );

            if( Error != ERROR_SUCCESS )
            {
                goto exit;
            }

            //
            // header may have been changed (if head is the one we want)
            // newHeaderOffset = 0 means last group has been removed
            //
            // NOTE: even we may have one item left on the list, we
            //       are not changing the multiGroup flag on this
            //       entry, so the dwGroupOffset are still points to
            //       the list
            pEntry->dwGroupOffset = dwNewHeaderOffset;

        }
        else
        {
            // set offset to 0 (single group)
            pEntry->dwGroupOffset = 0;

        }

        // if dwExamptDelta is set, we should leave the stick bit
        if(!pEntry->dwExemptDelta)
        {
            //
            // if the unassociated group is sticky, we are remove
            // the sticky bit of this url
            //
            // For multiple groups, we will have to make sure all
            // the remaining groups are non-sticky
            //

            if( IsStickyGroup(pGroupEntry->gid ) &&
                ( !pItem->HasMultiGroup()  ||
                  gm.NoMoreStickyEntryOnList(pEntry->dwGroupOffset) )
            )
            {
                Error = UpdateStickness(
                    pEntry,
                    URLCACHE_OP_UNSET_STICKY,
                    (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
                );
                if( Error != ERROR_SUCCESS )
                    goto exit;
            }
        }

        // update the usage
        if( pItem->HasMultiGroup() )
        {
            // dwGroupOffset now offset to head of group list
            gm.AdjustUsageOnList(
                    pEntry->dwGroupOffset, -RealFileSize(pEntry->dwFileSize) );
        }
        else
        {
            AdjustGroupUsage(pGroupEntry, -RealFileSize(pEntry->dwFileSize));
        }

        //
        // update hash bit indicating no group associate with this url
        // we won't clear the multiple group flag even if there is single
        // group left on the group list.
        //
        if( !pEntry->dwGroupOffset )
        {
            pItem->ClearGroup();
            pItem->ClearMultGroup();
        }

    }
    else
    {

        // Find Group via gid
        Error = gm.FindEntry(GroupId, &pGroupEntry, FALSE);
        if( Error != ERROR_SUCCESS )
        {
            goto exit;
        }

        if( pItem->HasGroup() )
        {
            // multiple group

            LPBYTE  lpBase;
            DWORD dwGroupEntryOffset = 0;

            lpBase = *_UrlObjStorage->GetHeapStart();
            dwGroupEntryOffset = PtrDiff32(pGroupEntry, lpBase);

            DWORD dwListEntryOffset = 0;
            DWORD dwEntryOffset = 0;
            DWORD dwItemOffset = 0;
            DWORD dwOldGroupEntryOffset = 0;

            if( !pItem->HasMultiGroup() )
            {
                //
                // switch from a single group to multiple
                // group, need to
                //  1) create a new group list
                //  2) add the existing single group to the newly created list
                //


//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

                // save offset
                dwEntryOffset = PtrDiff32(pEntry, lpBase);
                dwItemOffset = PtrDiff32(pItem, lpBase);
                if( pOldGroupEntry )
                {
                    dwOldGroupEntryOffset = PtrDiff32(pOldGroupEntry, lpBase);
                }

                //
                // get a new List (memfile may grown)
                //
                Error = gm.CreateNewGroupList(&dwListEntryOffset);
                if( Error != ERROR_SUCCESS )
                {
                    goto exit;
                }

                // restore pointers based on (possible) new base addr
                lpBase = *_UrlObjStorage->GetHeapStart();

                pEntry =  (URL_FILEMAP_ENTRY*)(lpBase + dwEntryOffset);
                pGroupEntry = (GROUP_ENTRY*)(lpBase + dwGroupEntryOffset);
                pItem = (HASH_ITEM*) (lpBase + dwItemOffset);
                if( pOldGroupEntry )
                {
                    pOldGroupEntry
                        = (GROUP_ENTRY*) (lpBase + dwOldGroupEntryOffset);
                }

//////////////////////////////////////////////////////////////////
// END WARNING: The file might have grown and remapped, so all  //
// pointers into the file after this point must be recalculated //
// from offsets.                                                //
//////////////////////////////////////////////////////////////////

                //
                // add the original group (whose offset is indicated
                // with dwGroupOffset of the url entry)
                // to the newly created list
                //
//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

                // save offset
                lpBase = *_UrlObjStorage->GetHeapStart();
                dwEntryOffset = PtrDiff32(pEntry, lpBase);
                dwItemOffset = PtrDiff32(pItem, lpBase);
                dwGroupEntryOffset = PtrDiff32(pGroupEntry, lpBase);
                if( pOldGroupEntry )
                {
                    dwOldGroupEntryOffset = PtrDiff32(pOldGroupEntry, lpBase);
                }

                Error = gm.AddToGroupList(
                        dwListEntryOffset, pEntry->dwGroupOffset);

                // restore offset
                lpBase = *_UrlObjStorage->GetHeapStart();
                pEntry =  (URL_FILEMAP_ENTRY*)(lpBase + dwEntryOffset);
                pGroupEntry = (GROUP_ENTRY*)(lpBase + dwGroupEntryOffset);
                pItem = (HASH_ITEM*) (lpBase + dwItemOffset);
                if( pOldGroupEntry )
                {
                    pOldGroupEntry
                        = (GROUP_ENTRY*) (lpBase + dwOldGroupEntryOffset);
                }


                
//////////////////////////////////////////////////////////////////
// END WARNING: The file might have grown and remapped, so all  //
// pointers into the file after this point must be recalculated //
// from offsets.                                                //
//////////////////////////////////////////////////////////////////
                if( Error != ERROR_SUCCESS )
                {
                    goto exit;
                }

                //
                // the dwGroupOffset of the url entry now
                // points the the head of a group list
                //
                pEntry->dwGroupOffset = dwListEntryOffset;
                pItem->MarkMultGroup();

            }

            //
            // Multiple group, just add the new group to the list
            //
//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
            // save offset
            lpBase = *_UrlObjStorage->GetHeapStart();
            dwEntryOffset = PtrDiff32(pEntry, lpBase);
            dwItemOffset = PtrDiff32(pItem, lpBase);
            dwGroupEntryOffset = PtrDiff32(pGroupEntry, lpBase);

            Error = gm.AddToGroupList(
                pEntry->dwGroupOffset, dwGroupEntryOffset);


            if( Error != ERROR_SUCCESS )
            {
                goto exit;
            }

            // remap since multiple group may cause memfile grow
            lpBase = *_UrlObjStorage->GetHeapStart();
            pEntry =  (URL_FILEMAP_ENTRY*)(lpBase + dwEntryOffset);
            pGroupEntry = (GROUP_ENTRY*)(lpBase + dwGroupEntryOffset);
            pItem = (HASH_ITEM*) (lpBase + dwItemOffset);

//////////////////////////////////////////////////////////////////
// END WARNING: The file might have grown and remapped, so all  //
// pointers into the file after this point must be recalculated //
// from offsets.                                                //
//////////////////////////////////////////////////////////////////
        }
        else
        {
            // single group, dwGroupOffset points the real group
            pEntry->dwGroupOffset = PtrDiff32(pGroupEntry, *_UrlObjStorage->GetHeapStart());
        }



        // update hash bit indicating group associate with this url
        pItem->MarkGroup();

        // if group is sticky, mark the entry to sticky as well
        if( IsStickyGroup(pGroupEntry->gid) )
        {
            Error = UpdateStickness(
                pEntry,
                URLCACHE_OP_SET_STICKY,
                (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
            );
            if( Error != ERROR_SUCCESS )
                goto exit;
        }

        // update the usage
        if( pItem->HasMultiGroup() )
        {
            // dwGroupOffset now offset to head of group list
            gm.AdjustUsageOnList(
                    pEntry->dwGroupOffset, RealFileSize(pEntry->dwFileSize) );
        }
        else
        {
            AdjustGroupUsage(pGroupEntry, RealFileSize(pEntry->dwFileSize) );
        }

        //
        // track the usage and quota
        // NOTE: we still allow this url to be added to the group
        //       even if usage > quota, DISK_FULL error will be
        //       returned, so the client is responsible to take
        //       futher action
        //
        if( pGroupEntry->llDiskUsage > (pGroupEntry->dwDiskQuota * 1024) )
        {
            Error = ERROR_NOT_ENOUGH_QUOTA;
            goto  exit;
        }
    }

    Error = ERROR_SUCCESS;

    NotifyCacheChange(CACHE_NOTIFY_UPDATE_URL,
            (DWORD)( ((LPBYTE) pItem) - *_UrlObjStorage->GetHeapStart())
        );

exit:
    if (fMustUnlock) UnlockContainer();
    return Error;
}

/*++
Gets group ID and exemption time for a particular URL.
--*/
DWORD URL_CONTAINER::GetUrlInGroup
    (LPCSTR lpszUrl, GROUPID* pGroupId, LPDWORD pdwExemptDelta)
{
    DWORD dwError;
    BOOL fMustUnlock;
    URL_FILEMAP_ENTRY* pEntry;
    GROUP_ENTRY*       pGroupEntry = NULL;
    HASH_ITEM*          pItem = NULL;

    if (!LockContainer(&fMustUnlock))
    {
        dwError = GetLastError();
        goto exit;
    }


    // Look up the entry.
    if (HashFindItem (lpszUrl, 0, &pItem))
    {
        pEntry = HashGetEntry (pItem);
    }
    else
    {
        pEntry = NULL;
    }

    if (!pEntry)
        dwError = ERROR_FILE_NOT_FOUND;
    else
    {
        if( pEntry->dwGroupOffset )
        {
            pGroupEntry = _UrlObjStorage->ValidateGroupOffset(
                pEntry->dwGroupOffset, pItem);
            if( pGroupEntry )
            {

                INET_ASSERT(pGroupEntry->gid);
                *((LONGLONG*) pGroupId) = pGroupEntry->gid;
            }
            else
            {
                dwError = ERROR_FILE_NOT_FOUND;
            }
        }
        else
        {
            *((LONGLONG*) pGroupId) = 0;
        }

        *pdwExemptDelta = pEntry->dwExemptDelta;
        dwError = ERROR_SUCCESS;
    }

exit:
    if (fMustUnlock) UnlockContainer();
    return dwError;
}


DWORD URL_CONTAINER::CreateUniqueFile(LPCSTR UrlName, DWORD ExpectedSize,
                                   LPCSTR lpszFileExtension, LPTSTR FileName,
                                   HANDLE *phfHandle)
/*++

Routine Description:

    This function creates a temperary file in the cache storage. This call
    is called by the application when it receives a url file from a
    server. When the receive is completed it caches this file to url cache
    management, which will move the file to permanent cache file. The idea
    is the cache file is written only once directly into the cache store.

Arguments:

    UrlName : name of the url file (unused now).

    ExpectedSize : expected size of the incoming file. If it is unknown
        this value is set to null.

    lpszFileExtension: extension for the filename created

    FileName : pointer to a buffer that receives the full path name of the
        the temp file.

    phfHandle : pointer to a handle that receives the handle of the file
        being create; pass null if we don't care (the file will be closed).

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    BOOL fMustUnlock;

    // BUGBUG - adding LockContainer here.
    if (!LockContainer(&fMustUnlock))
    {
        Error = (GetLastError());
        goto exit;
    }

    Error = _FileManager->CreateUniqueFile((LPSTR) UrlName, (LPSTR) FileName,
        (LPSTR) lpszFileExtension, (HANDLE*) phfHandle);

exit:
    if (fMustUnlock) UnlockContainer();
    return( Error );
}



DWORD URL_CONTAINER::FindNextEntry
    (LPDWORD lpdwEnum, LPCACHE_ENTRY_INFO *ppCEI, LPDWORD lpdwCEI, DWORD dwFilter, GROUPID GroupId, DWORD dwFlags, DWORD dwRetrievalFlags)
{

    DWORD Error;
    URL_FILEMAP_ENTRY* pEntry;
    DWORD dwEnumSave;
    BOOL fMustUnlock;
    DWORD dwCopyFlags;

    if (!LockContainer(&fMustUnlock))
    {
        Error = GetLastError();
        goto Cleanup;
    }

    BOOL fCheckUser;
    fCheckUser = IsContentContainer() && !(dwFilter & OTHER_USER_CACHE_ENTRY);
    dwCopyFlags = 0;
    if (dwFlags & FIND_FLAGS_RETRIEVE_ONLY_STRUCT_INFO)
    {
        dwCopyFlags = RETRIEVE_ONLY_STRUCT_INFO;
    }
    else if (dwFlags & FIND_FLAGS_RETRIEVE_ONLY_FIXED_AND_FILENAME)
    {
        dwCopyFlags = RETRIEVE_ONLY_FILENAME;
    }
    dwCopyFlags |= dwRetrievalFlags;
    while (1)
    {
        dwEnumSave = *lpdwEnum;
        pEntry = (URL_FILEMAP_ENTRY*) _UrlObjStorage->FindNextEntry(lpdwEnum, dwFilter, GroupId);

        if(!pEntry)
        {
            Error = ERROR_NO_MORE_ITEMS;
            goto Cleanup;
        }

        // For content container, skip items marked for another user.
        if (fCheckUser)
        {
            LPSTR pszHeaders = ((LPSTR) pEntry) + pEntry->HeaderInfoOffset;
            if (!IsCorrectUser(pszHeaders, pEntry->HeaderInfoSize))
                continue;
        }

        // Copy the data
        Error = CopyUrlInfo (pEntry, ppCEI, lpdwCEI, dwCopyFlags);
        switch (Error)
        {
            case ERROR_INSUFFICIENT_BUFFER:
                // Restore current enum position.
                *lpdwEnum = dwEnumSave;
                goto Cleanup;

            case ERROR_FILE_NOT_FOUND:
                continue;

            default:
                INET_ASSERT (FALSE);
            // intentional fall through
            case ERROR_SUCCESS:
                goto Cleanup;
        }
    } // end while(1)

Cleanup:
    if (fMustUnlock) UnlockContainer();
    return Error;
}



/*------------------------------------------------------------------------------
    CopyUrlInfo

Routine Description:

    Copy URL info data from an URL_FILEMAP_ENTRY in the memory mapped file
    to CACHE_ENTRY_INFO output buffer. If the buffer given is sufficient,
    it returns ERROR_INSUFFICIENT_BUFFER, and pcbInfo will contain
    buffer size required.

Arguments:

    pEntry     : pointer to the source of the URL info.

    ppInfo   : ptr to ptr to an entry info buffer, where the url entry info
               is returned.

    pcbInfo : pointer to a DWORD location containing the size of the
               above buffer, on return it has the size of the buffer consumed or
               size of the buffer required for successful retrieval.

Return Value:

    Windows Error Code.

------------------------------------------------------------------------------*/
DWORD URL_CONTAINER::CopyUrlInfo(LPURL_FILEMAP_ENTRY   pEntry,
                                 LPCACHE_ENTRY_INFO*   ppInfo,
                                 LPDWORD               pcbInfo,
                                 DWORD                 dwFlags)
{
    DWORD cbRequired;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cbSourceUrlName;
    DWORD cbLocalFileName;
    DWORD cbHeaderInfo;
    DWORD cbFileExt;

    INET_ASSERT(!((dwFlags & RETRIEVE_WITH_ALLOCATION) &&
                  (dwFlags & RETRIEVE_ONLY_FILENAME
                    || dwFlags & RETRIEVE_ONLY_STRUCT_INFO)));
    // Check signature
    if (pEntry->dwSig != SIG_URL)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // Verify url string exists.
    if (!pEntry->UrlNameOffset)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // Hate using goto's but, don't want to clutter anymore than I have to.
    // We assume that anything functions that pass these flags will have allocated
    // enough memory before hand.
    if ((dwFlags & RETRIEVE_ONLY_FILENAME) || (dwFlags & RETRIEVE_ONLY_STRUCT_INFO))
    {
        if (ppInfo && *ppInfo)
        {
            memset(*ppInfo, 0, sizeof(INTERNET_CACHE_ENTRY_INFO));
        }
        goto ShortCircuit;
    }

    // -----------------  Calculate embedded data sizes ------------------------
    // All byte counts are sizes.

    // SourceUrlName length;
    cbSourceUrlName = strlen((LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->UrlNameOffset)) + 1;

    // LocalFileName length.
    if(pEntry->InternalFileNameOffset)
    {
        cbLocalFileName =
            _FileManager->GetDirLen(pEntry->DirIndex)
            + strlen((LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->InternalFileNameOffset))
            + 1;
    }
    else
        cbLocalFileName = 0;

    // HeaderInfo length.
    cbHeaderInfo = (pEntry->HeaderInfoOffset) ? pEntry->HeaderInfoSize + 1 : 0;

    // File extension length.
    if (pEntry->FileExtensionOffset)
    {
        cbFileExt =
              strlen((LPSTR) OFFSET_TO_POINTER(pEntry, pEntry->FileExtensionOffset)) + 1;
    }
    else
        cbFileExt = 0;

    // Alignment - these quantities are already aligned in
    // URL_FILEMAP_ENTRY and should be reflected in its size.
    cbSourceUrlName  = ROUNDUPDWORD(cbSourceUrlName);
    cbLocalFileName  = ROUNDUPDWORD(cbLocalFileName);
    cbHeaderInfo     = ROUNDUPDWORD(cbHeaderInfo);
    cbFileExt        = ROUNDUPDWORD(cbFileExt);

    cbRequired = *pcbInfo;
    *pcbInfo = sizeof(CACHE_ENTRY_INFO)
                   + cbSourceUrlName
                   + cbLocalFileName
                   + cbHeaderInfo
                   + cbFileExt;

    if (dwFlags & RETRIEVE_WITH_ALLOCATION)
    {
        // If we are allocating entry info, use the ex version.
        *pcbInfo += sizeof(CACHE_ENTRY_INFOEX) - sizeof(CACHE_ENTRY_INFO);
        *ppInfo = (LPCACHE_ENTRY_INFO)ALLOCATE_FIXED_MEMORY(*pcbInfo);
        if (!*ppInfo)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
    }
    else
    {
        // Second check for required buffer size.
        if (cbRequired < *pcbInfo )
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
            goto exit;
        }
    }

    // ----------------------  Copy embedded data --------------------------------

    // A Typical CACHE_ENTRY_INFO will look like
    //
    // [CACHE_ENTRY_INFO][UrlName][FileName][Headers][FileExtension]
    //
    //                   ^        ^         ^        ^
    //                   |        |         |        |
    //                   |        |         |        lpszFileExtension
    //                   |        |         |
    //                   |        |         lpHeaderInfo
    //                   |        |
    //                   |        lpszLocalFileName
    //                   |
    //                   lpszSourceUrlName
    //

    // Pointer walks through CACHE_ENTRY_INFO appended data.
    LPBYTE pCur;
    pCur = (LPBYTE) *ppInfo + sizeof(CACHE_ENTRY_INFO);
    if (dwFlags & RETRIEVE_WITH_ALLOCATION)
    {
        // If we are creating the -ex version, skip over those fields.
        pCur += sizeof(CACHE_ENTRY_INFOEX) - sizeof(CACHE_ENTRY_INFO);
    }

    // UrlName.
    memcpy(pCur, OFFSET_TO_POINTER(pEntry, pEntry->UrlNameOffset), cbSourceUrlName);
    (*ppInfo)->lpszSourceUrlName = (LPSTR) pCur;
    pCur += cbSourceUrlName;

    // FileName
    if (cbLocalFileName)
    {
        DWORD cb;
        if (!_FileManager->GetFilePathFromEntry(pEntry, (LPSTR) pCur, &(cb = MAX_PATH)))
        {
            dwError = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
        
        (*ppInfo)->lpszLocalFileName = (LPTSTR) pCur;
        pCur += cbLocalFileName;
    }
    else
        (*ppInfo)->lpszLocalFileName = NULL;


    // HeaderInfo
    if (cbHeaderInfo)
    {
        memcpy (pCur, OFFSET_TO_POINTER(pEntry, pEntry->HeaderInfoOffset),
            pEntry->HeaderInfoSize);
        pCur[pEntry->HeaderInfoSize] = 0;
        (*ppInfo)->lpHeaderInfo = (LPTSTR)pCur;
        pCur += cbHeaderInfo;
    }
    else
        (*ppInfo)->lpHeaderInfo = NULL;


    // FileExt
    if (cbFileExt)
    {
        memcpy(pCur, OFFSET_TO_POINTER(pEntry, pEntry->FileExtensionOffset), cbFileExt);
        (*ppInfo)->lpszFileExtension = (LPTSTR) pCur;
        pCur += cbFileExt;
    }
    else
        (*ppInfo)->lpszFileExtension = NULL;


    // ------------  Set remaining CACHE_ENTRY_INFO members -------------
ShortCircuit:
    // Struct size, entry type, use count and hit rate.
    (*ppInfo)->dwStructSize = URL_CACHE_VERSION_NUM;
    (*ppInfo)->CacheEntryType = pEntry->CacheEntryType;
    if (pEntry->bSyncState == SYNCSTATE_STATIC)
        (*ppInfo)->CacheEntryType |= STATIC_CACHE_ENTRY;
    (*ppInfo)->dwUseCount     = pEntry->NumReferences;
    (*ppInfo)->dwHitRate      = pEntry->NumAccessed;

    // File size.
    (*ppInfo)->dwSizeLow      = pEntry->dwFileSize;
    (*ppInfo)->dwSizeHigh     = 0;

    // Last modified, expire, last access and last sync times, maybe download time.
    (*ppInfo)->LastModifiedTime   = *LONGLONG_TO_FILETIME(&pEntry->LastModifiedTime);

    // expire time may be 0

    DosTime2FileTime(pEntry->dostExpireTime, &((*ppInfo)->ExpireTime));

    (*ppInfo)->LastAccessTime     = *LONGLONG_TO_FILETIME(&pEntry->LastAccessedTime);
    if (dwFlags & RETRIEVE_WITH_ALLOCATION)
    {
        CACHE_ENTRY_INFOEX* pCEI = (CACHE_ENTRY_INFOEX*) *ppInfo;
        DosDateTimeToFileTime(*(LPWORD)&(pEntry->dostFileCreationTime),
                             *((LPWORD)&(pEntry->dostFileCreationTime)+1),
                             &pCEI->ftDownload);

        DosTime2FileTime(pEntry->dostPostCheckTime, &pCEI->ftPostCheck);

    }

    DosDateTimeToFileTime(*(LPWORD)&(pEntry->dostLastSyncTime),
                          *((LPWORD)&(pEntry->dostLastSyncTime)+1),
                          &((*ppInfo)->LastSyncTime));


    // Header info size and exempt delta.
    (*ppInfo)->dwHeaderInfoSize   = pEntry->HeaderInfoSize;
    (*ppInfo)->dwExemptDelta      = pEntry->dwExemptDelta;

    // If we want only struct info and filename, we'll assume that we've preallocated
    // enough memory.
    if (dwFlags & RETRIEVE_ONLY_FILENAME)
    {
        DWORD cb;
        if (!_FileManager->GetFilePathFromEntry(pEntry, (LPSTR) (*ppInfo) + sizeof(INTERNET_CACHE_ENTRY_INFO),
            &(cb = MAX_PATH)))
        {
            dwError = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
        (*ppInfo)->lpszLocalFileName = (LPTSTR) (*ppInfo) + sizeof(INTERNET_CACHE_ENTRY_INFO);
    }

exit:
    return dwError;
}


void URL_CONTAINER::UnlockItem (URL_FILEMAP_ENTRY* pEntry, HASH_ITEM* pItem)
{
    INET_ASSERT (pEntry->NumReferences);


    if (pEntry->NumReferences)
    {
        if (--pEntry->NumReferences)
        {
            if( !pItem->IsLocked() )
            {
                // corrupted index file, we have to believe the hash table
                // to fixup the cache entry
                INET_ASSERT (FALSE);
                pEntry->NumReferences = 0;
            }
        }
        else
        {
            pItem->ClearLocked();

            // If the item is marked for pending delete, do it now.
            if (pEntry->CacheEntryType & PENDING_DELETE_CACHE_ENTRY)
                DeleteUrlEntry (pEntry, pItem, SIG_DELETE);
        }
    }
}


void URL_CONTAINER::UnlockAllItems (void)
{
    DWORD dwEnum = *(_UrlObjStorage->GetPtrToHashTableOffset());

    // Enumerate hash table items.
    while (dwEnum)
    {
        HASH_ITEM *pItem = HashGetNextItem
            (_UrlObjStorage, *(_UrlObjStorage->GetHeapStart()), &dwEnum, 0);

        if (pItem && (pItem->IsLocked()))
        {
            // Validate and unlock the entry.
            URL_FILEMAP_ENTRY *pEntry =
                _UrlObjStorage->ValidateUrlOffset (pItem->dwOffset);
            if (!pEntry)
                pItem->MarkFree(); // invalid item
            else
            {
                // Clear the lockcount.
                pEntry->NumReferences = 1;
                UnlockItem (pEntry, pItem);
            }
        }
    }
}


DWORD URL_CONTAINER::RegisterCacheNotify( HWND      hWnd,
                                          UINT      uMsg,
                                          GROUPID   gid,
                                          DWORD     dwFilter)
{
    BOOL fUnlock;
    LockContainer(&fUnlock);

    _UrlObjStorage->SetHeaderData(
            CACHE_HEADER_DATA_NOTIFICATION_HWND,    GuardedCast((DWORD_PTR)hWnd));
    _UrlObjStorage->SetHeaderData(
            CACHE_HEADER_DATA_NOTIFICATION_MESG,    (DWORD)uMsg);
    _UrlObjStorage->SetHeaderData(
            CACHE_HEADER_DATA_NOTIFICATION_FILTER,  (DWORD)dwFilter);

    if (fUnlock) UnlockContainer();

    return ERROR_SUCCESS;
}


// update stickness will do:
//      1. flip the bit
//      2. update the exempt usage
//      3. send notification
DWORD URL_CONTAINER::UpdateStickness( URL_FILEMAP_ENTRY* pEntry,
                                      DWORD              dwOp,
                                      DWORD              dwItemOffset)
{
    DWORD dwError = ERROR_SUCCESS;

    if( dwOp == URLCACHE_OP_SET_STICKY )
    {
        if( !( pEntry->CacheEntryType & STICKY_CACHE_ENTRY ) )
        {
            // Ensure that exempt items do not crowd the cache.
            LONGLONG FileUsage = RealFileSize(pEntry->dwFileSize);
            LONGLONG ExemptUsage = _UrlObjStorage->GetExemptUsage();
            LONGLONG CacheLimit  = _UrlObjStorage->GetCacheLimit();
            LONGLONG MaxExempt = (CacheLimit * MAX_EXEMPT_PERCENTAGE) / 100;
            if (ExemptUsage + FileUsage > MaxExempt)
                return ERROR_DISK_FULL;

            pEntry->CacheEntryType |= STICKY_CACHE_ENTRY;
            _UrlObjStorage->AdjustExemptUsage(RealFileSize(pEntry->dwFileSize));
            NotifyCacheChange(CACHE_NOTIFY_URL_SET_STICKY, dwItemOffset);
        }
    }

    else
    if( dwOp == URLCACHE_OP_UNSET_STICKY )
    {
        if( pEntry->CacheEntryType & STICKY_CACHE_ENTRY )
        {
            pEntry->CacheEntryType &= ~STICKY_CACHE_ENTRY;
            _UrlObjStorage->AdjustExemptUsage(-RealFileSize(pEntry->dwFileSize));
            NotifyCacheChange(CACHE_NOTIFY_URL_UNSET_STICKY, dwItemOffset);
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}

VOID FileTime2DosTime(FILETIME ft, DWORD* pdt)
{
    INET_ASSERT(pdt);

    *pdt = 0;
    if( FT2LL(ft) != LONGLONG_ZERO )
    {
        if( FT2LL(ft) == MAX_FILETIME)
        {
            *pdt = MAX_DOSTIME;
        }
        else
        {
            FileTimeToDosDateTime(
                &ft,
                ((LPWORD)(pdt)    ),
                ((LPWORD)(pdt) + 1)
            );
        }
    }
}

VOID DosTime2FileTime(DWORD dt, FILETIME* pft)
{
    INET_ASSERT(pft);

    LONGLONG llZero = LONGLONG_ZERO;
    LONGLONG llMax = MAX_FILETIME;
    if( dt )
    {
        if( dt == MAX_DOSTIME )
        {
            *pft = *LONGLONG_TO_FILETIME(&llMax);
        }
        else
        {
            DosDateTimeToFileTime(
                *((LPWORD)&(dt)    ),
                *((LPWORD)&(dt) + 1),
                pft
            );
        }
    }
    else
    {
        *pft = *LONGLONG_TO_FILETIME(&llZero);
    }
}
