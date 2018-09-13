/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    contain.hxx

Abstract:

    contains class definitions for CONTAINER class objects.

Author:

    Madan Appiah (madana)  28-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef SAFERELEASE
#define SAFERELEASE(p,x) if ((p) != NULL) { (p)->Release(x); (p) = NULL; };
#endif
//  so as to easily recognize Extensible prefixes, require them to start with
//  a character that is illegal in URLs
#define EXTENSIBLE_FIRST (':')


struct SCORE_ITEM
{
    DWORD dwScore;       // score of this item
    DWORD dwItemOffset;  // offset to hash table item
    DWORD dwHashValue;   // masked value from hash table item
    DWORD dwHashOffset;  // offset to url entry from hash table item
};

/*++

Class Description:

    Class definition that manages the URL objects.

Private Member functions:

Public Member functions:

    URL_CONTAINER : constructs an URL container.

    ~URL_CONTAINER : destructs an URL container.

    GetStatus : Retrieves object status.

    CleanupUrls : Deletes unused URLs and frees up file system cache space.

    DeleteIndex: Deletes all files not in use and attempts to delete index file.
    
    AddUrl : Adds an URL to the container and copies the cache file.

    RetrieveUrl : Retrieves an url from the cache.

    DeleteUrl : Deletes the specified url from cache.

    UnlockUrl : Decrements the reference count.

    GetUrlInfo : Gets info of the specified url file.

    SetUrlInfo : Sets info of the specified url file.

    SetExemptDelta: Sets an exemption on the entry
    
    CreateUniqueFile : Creates a file in the cache path for the incoming url
        to store.

    GetCacheInfo : Retrieves certain container info.

    SetCacheLimit : Sets cache limit value.

    DeleteAPendingUrl : Removes and deletes an url from the pending list.
        Returns FALSE if the list is empty.

    FindNextEntry : Retrieves the next element from the container.

    GetCacheSize : return current size of the cache.

--*/

class GroupMgr;

class URL_CONTAINER {

protected:

// This class has one vtable, so there is also one dword of alignment padding.
    
    LONGLONG     _CacheStartUpLimit;

    DWORD        _Status;

    MEMMAP_FILE* _UrlObjStorage;
    CFileMgr*    _FileManager;

    LPTSTR       _CachePath;
    LPTSTR       _CachePrefix;
    DWORD        _CachePrefixLen;
    DWORD        _CacheEntryType;
    LPTSTR       _CacheName;
    DWORD        _CachePathLen;
    DWORD        _ClusterSizeMinusOne;
    DWORD        _ClusterSizeMask;
    DWORD        _FileMapEntrySize;
    HANDLE       _MutexHandle;
    BOOL         _fIsInitialized;
    DWORD        _dwRefCount;
    BOOL         _fDeletePending;
    BOOL         _fDeleted;
    BOOL         _fMarked;
    BOOL         _fMustLaunchScavenger;
    BOOL         _fPerUserItem;
    DWORD        _dwLastReference;
    DWORD        _dwOptions;
    DWORD        _dwTaken;

    DWORD        _dwBytesDownloaded;
    DWORD        _dwItemsDownloaded;
    
#ifdef CHECKLOCK_NORMAL
    DWORD        _dwThreadLocked;
#endif

    BOOL SetHeaderFlags(DWORD dwStamp);
    DWORD CopyUrlInfo(  LPURL_FILEMAP_ENTRY, LPCACHE_ENTRY_INFO*, LPDWORD, DWORD);
    void UnlockAllItems (void);
    void UnlockItem (URL_FILEMAP_ENTRY* pEntry, HASH_ITEM* pItem);
    BOOL UpdateOfflineRedirect (DWORD, LPCSTR, DWORD, LPCSTR);
    BOOL DeleteIndex (void);
    void CloseContainerFile();
    DWORD SetExemptDelta (URL_FILEMAP_ENTRY*, DWORD dwExemptDelta, DWORD dwItemOffset=0);
    DWORD DeleteUrlEntry (URL_FILEMAP_ENTRY*, HASH_ITEM* pHash, DWORD dwSig);
    
    DWORD UpdateStickness(URL_FILEMAP_ENTRY*, DWORD dwOp, DWORD dwItemOffset=0);

public:
    URL_CONTAINER( LPTSTR CacheName, LPTSTR CachePath, LPTSTR CachePrefix, LONGLONG CacheLimit, DWORD dwOptions );
    ~URL_CONTAINER( VOID );

    DWORD Init();

    DWORD GetStatus( VOID ) 
    {
        return( _Status );
    };

#ifdef CHECKLOCK_PARANOID
    void CheckNoLocks(DWORD dwThreadId);
#endif

    BOOL LockContainer( BOOL *fMustUnlock );
    VOID UnlockContainer( VOID );
    BOOL WalkLeakList (void);

    DWORD GetLastReference();
    BOOL IsVisible();
    void Mark(BOOL fMarked);
    BOOL GetMarked();
    BOOL GetDeleted();
    void SetDeleted(BOOL fDeleted);
    BOOL GetDeletePending();
    void SetDeletePending(BOOL fDeletePending);
    void AddRef();
    DWORD Release(BOOL fTryToUnmap);
    void TryToUnmap(DWORD dwAcceptableRefCount);
    DWORD GetOptions();

    //
    // Internal routines called by the cache management.
    //

    DWORD CleanupUrls ( DWORD Factor, DWORD dwFilter);

    DWORD FixupHandler (DWORD dwFactor, DWORD dwFilter);
        
    BOOL  PrefixMatch( LPCSTR   UrlName ) 
    {
        return (_CachePrefix &&
                (strnicmp(_CachePrefix, UrlName, _CachePrefixLen)==0) );
    }

#ifdef CHECKLOCK_PARANOID
    void CheckUnlocked()
    {
        INET_ASSERT(_dwTaken==0 || GetCurrentThreadId() != _dwThreadLocked);
    }
#endif

    LPSTR GetCacheName()
    {
        return _CacheName;
    }

    LPSTR GetCachePath()
    {
        return _CachePath;
    }

    LPSTR GetCachePrefix()
    {
        return _CachePrefix;
    }

    BOOL IsContentContainer()
    {
        return (_CachePrefix[0] == 0);
    }

    DWORD GetCachePathLen()
    {
        return _CachePathLen;
    }
    
    DWORD IsInitialized()
    {
        return _fIsInitialized;
    }

    BOOL IsPerUserItem()
    {
        return _fPerUserItem;
    }

    VOID SetPerUserItem(BOOL fFlag)
    {
        _fPerUserItem = fFlag;
    }

    VOID SetCacheSize(LONGLONG dlSize)
    {
        _UrlObjStorage->SetCacheSize(dlSize);
    }

    //
    // External routines called by the cache APIs.
    //

    virtual LPSTR GetPrefixMap();
    virtual LPSTR GetVolumeLabel();
    virtual LPSTR GetVolumeTitle();

    virtual DWORD AddUrl(AddUrlArg* args);

    virtual DWORD RetrieveUrl(
        LPCSTR  UrlName,
        LPCACHE_ENTRY_INFO* EntryInfo,
        LPDWORD EntryInfoSize,
        DWORD dwLookupFlags,
        DWORD dwRetrievalFlags);

    DWORD DeleteUrl( LPCSTR  UrlName );

    DWORD UnlockUrl( LPCSTR UrlName );
    virtual DWORD GetUrlInfo(
        LPCSTR UrlName,
        LPCACHE_ENTRY_INFO* ppUrlInfo,
        LPDWORD UrlInfoLength,
        DWORD dwLookupFlags,
        DWORD dwEntryFlags,
        DWORD dwRetrievalFlags);
        
    DWORD SetUrlInfo(
        LPCSTR UrlName,
        LPCACHE_ENTRY_INFO UrlInfo,
        DWORD FieldControl );

    DWORD SetUrlGroup (LPCSTR lpszUrl, DWORD dwFlags, GROUPID GroupId);

    DWORD GetUrlInGroup (LPCSTR lpszUrl, GROUPID* pGroupId, LPDWORD pdwExemptDelta);

    DWORD CreateUniqueFile(
        LPCSTR  UrlName,
        DWORD   ExpectedSize,
        LPCSTR  lpszFileExtension,
        LPTSTR  FileName,
        HANDLE *phfHandle);

    DWORD RegisterCacheNotify(
        HWND        hWnd, 
        UINT        uMsg, 
        GROUPID     gid, 
        DWORD       dwFilter);


    VOID SendCacheNotification(DWORD dwOpcode)
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        NotifyCacheChange(dwOpcode, 0);
        if (fUnlock) UnlockContainer();
    }

    VOID SetCacheLimit(LONGLONG CacheLimit ) 
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        _UrlObjStorage->SetCacheLimit(CacheLimit);
        if (fUnlock) UnlockContainer();
    }

    LONGLONG GetCacheStartUpLimit()
    {
        BOOL fUnlock;
        LONGLONG llResult;

        LockContainer(&fUnlock);
        llResult = _CacheStartUpLimit;
        if (fUnlock) UnlockContainer();
        return llResult;
    }

    LONGLONG GetCacheLimit()
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        LONGLONG cbLimit = _UrlObjStorage->GetCacheLimit();
        if (fUnlock) UnlockContainer();
        return cbLimit;
    }

    LONGLONG GetCacheSize()
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        LONGLONG CacheSize = _UrlObjStorage->GetCacheSize();
        if (fUnlock) UnlockContainer();
        return CacheSize;
    }    
    
    LONGLONG GetExemptUsage()
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        LONGLONG ExemptCacheUsage = _UrlObjStorage->GetExemptUsage();
        if (fUnlock) UnlockContainer();
        return ExemptCacheUsage;
    }    
    
    VOID GetCacheInfo( LPTSTR CachePath,
                       LONGLONG *CacheLimit ) 
    {
        BOOL fUnlock;

        LockContainer(&fUnlock);
        *CacheLimit = _UrlObjStorage->GetCacheLimit();
        memcpy(CachePath, _CachePath, _CachePathLen);
        if (fUnlock) UnlockContainer();
    }

    DWORD GetInitialFindHandle (void)
    {
        DWORD dwHandle;
        BOOL fUnlock;

        LockContainer(&fUnlock);
        dwHandle = *_UrlObjStorage->GetPtrToHashTableOffset();
        if (fUnlock) UnlockContainer();
        return dwHandle;
    }


    DWORD FindNextEntry(
        LPDWORD Handle,
        LPCACHE_ENTRY_INFO* lppCacheEntryInfo,
        LPDWORD lpCacheEntryInfoSize,
        DWORD dwFilter,
        GROUPID GroupId,
        DWORD dwFlags,
        DWORD dwRetrievalFlags);

    BOOL SetHeaderData(DWORD nIdx, DWORD dwData)
    {
        BOOL fRet, fUnlock;
        LockContainer(&fUnlock);
        fRet = _UrlObjStorage->SetHeaderData(nIdx, dwData);
        if (fUnlock) UnlockContainer();
        return fRet;
    }
        
    BOOL GetHeaderData(DWORD nIdx, LPDWORD pdwData) 
    {
        BOOL fRet, fUnlock;
        LockContainer(&fUnlock);
        fRet = _UrlObjStorage->GetHeaderData(nIdx, pdwData);
        if (fUnlock) UnlockContainer();
        return fRet;
    }

    BOOL IncrementHeaderData(DWORD nIdx, LPDWORD pdwData) 
    {
        BOOL fRet, fUnlock;
        LockContainer(&fUnlock);
        fRet = _UrlObjStorage->IncrementHeaderData(nIdx, pdwData);
        if (fUnlock) UnlockContainer();
        return fRet;
    }    

    DWORD AddLeakFile (LPCSTR pszFile);


    // Creates a cache directory with a given name to allow existing directories
    // to be copied into another cache file.
    BOOL CreateDirWithSecureName( LPSTR szDirName)
    {
        return _FileManager->CreateDirWithSecureName( szDirName);
    }

    
    //  Creates a redirect from TargetUrl to OriginUrl
    BOOL CreateRedirect( LPSTR szTargetUrl, LPSTR szOriginUrl)
    {
        BOOL retVal = FALSE;
        
        HASH_ITEM* pTargetItem;

        if( HashFindItem( szTargetUrl, 0, &pTargetItem) != TRUE)
            goto doneCreateRedirect;

        if( UpdateOfflineRedirect( OffsetFromPointer(pTargetItem),
                                   szTargetUrl, lstrlen( szTargetUrl),
                                   szOriginUrl) != TRUE)
            goto doneCreateRedirect;

        retVal = TRUE;
        
    doneCreateRedirect:
        return retVal;
    }
    

    
private:

    // hash table wrapper methods

    DWORD OffsetFromPointer (LPVOID p)
    {
        SSIZE_T llDiff = PtrDifference(p, *_UrlObjStorage->GetHeapStart());
#if defined _IA64_ || _AXP64_
        return GuardedCast(llDiff);
#else
        return llDiff;
#endif
    }

    LPBYTE PointerFromOffset (DWORD dw)
    {
        return ((LPBYTE) *_UrlObjStorage->GetHeapStart()) + dw;
    }        
   
    LPURL_FILEMAP_ENTRY HashGetEntry (HASH_ITEM *pItem)
    {
        INET_ASSERT (pItem->dwHash > HASH_END);
        return (URL_FILEMAP_ENTRY*) PointerFromOffset (pItem->dwOffset);
    }

    void HashSetEntry (HASH_ITEM* pItem, LPBYTE pEntry)
    {
        pItem->dwOffset = OffsetFromPointer (pEntry);
    }

    BOOL HashFindItem (LPCSTR pszUrl, DWORD dwLookupFlags, HASH_ITEM** ppItem);
/////////////////////////////////////////////////////////////////////////////
// WARNING: if HASH_*_CREATE set, the file might be grown and remapped     //
// so all pointers into the file must be recalculated from their offsets   //
/////////////////////////////////////////////////////////////////////////////

    PRIVATE HASH_ITEM* IsMatch (HASH_ITEM *pItem, LPCSTR pszKey, DWORD dwFlags);
    
    LPURL_FILEMAP_ENTRY HashFindEntry (LPCSTR pszUrl, DWORD dwLookupFlags = 0)
    {
        HASH_ITEM *pItem;
        if (HashFindItem (pszUrl, dwLookupFlags, &pItem))
            return HashGetEntry (pItem);
        else
            return NULL;
    }

    void AdjustGroupUsage(GROUP_ENTRY* pGroupEntry, LONGLONG llDelta)
    {
        if( pGroupEntry && !IsInvalidGroup(pGroupEntry->gid) )
        {
            if( pGroupEntry->llDiskUsage >= -llDelta ) 
            {
                pGroupEntry->llDiskUsage += llDelta;
            }
            else
            {
                INET_ASSERT(FALSE); // underflow
                pGroupEntry->llDiskUsage = 0;
            }
        }
    }

    VOID NotifyCacheChange(DWORD dwOpcode, DWORD dwHOffset)
    {
        //
        // the caller of this method has container locked, so there
        // no need to call (Un)LockContain here.
        //
        DWORD dwFilter = 0;
        _UrlObjStorage->GetHeaderData(
            CACHE_HEADER_DATA_NOTIFICATION_FILTER, &dwFilter);

        if( dwFilter & dwOpcode )
        {
            DWORD dwHWnd = 0;
            DWORD dwUMsg = 0;

            _UrlObjStorage->GetHeaderData(
                CACHE_HEADER_DATA_NOTIFICATION_HWND, &dwHWnd);
            _UrlObjStorage->GetHeaderData(
                CACHE_HEADER_DATA_NOTIFICATION_MESG, &dwUMsg);

            if( dwHWnd && dwUMsg && IsWindow((HWND)dwHWnd) )
            {
                // NOTE: we will not check for msg q overflow
                PostMessage(
                    (HWND)dwHWnd, 
                    (UINT)dwUMsg, 
                    (WPARAM)dwOpcode, 
                    (LPARAM)dwHOffset
                );
            }
        }
    }

    BOOL IsUrlEntryExemptFromScavenging
    (
        HASH_ITEM* pItem,
        URL_FILEMAP_ENTRY* pEntry,
        DWORD dwFilter,
        LONGLONG qwGmtTime,
        GroupMgr* pgm
    );

    BOOL ScavengeItem (HASH_ITEM* pItem, BOOL* pfMustUnlock);

    void ScavengerDebugSpew (SCORE_ITEM*, LONGLONG*);

friend class GroupMgr;
};



