/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  conmgr.hxx

Abstract:

    Manages list of containers (CConList)
    
Author:
    Adriaan Canter (adriaanc) 04-02-97
    
--*/

#ifndef _CONMGR_HXX
#define _CONMGR_HXX

// Total number of containers is 3: CONTENT, COOKIE and HISTORY.

#define CONTENT     0
#define COOKIE      1
#define HISTORY     2
#define NCONTAINERS 3

// Alias for old names
#define _coContent   (_coContainer[CONTENT])
#define _coCookies   (_coContainer[COOKIE])
#define _coHistory   (_coContainer[HISTORY])

class IE5_REGISTRYSET;

/*-----------------------------------------------------------------------------
    Container manager class
  ---------------------------------------------------------------------------*/
class CConMgr
{
private:

    DWORD _dwStatus;
    BOOL _fProfilesCapable, _fUsingBackupContainers;
    BOOL _fNewCacheVersion;
    DWORD _dwModifiedCount;  // count last time we polled
    HANDLE _hMutexExtensible;// used to Mutex access to extensible containers registry
    DWORD _dwLastUnmap;      // time of last UnMap sweep

    // Array of containers.
    CConList ConList;

    // Cached (not ref-counted - not needed) special containers
    URL_CONTAINER *_coContainer[NCONTAINERS];

    // Init function called by constructor.
    DWORD Init();

    // Initializes containers.
    DWORD InitFixedContainers();

    // Config info get and set functions.

    // Utilities.
    DWORD FindIndexFromPrefix(LPCSTR);    
    BOOL PathPrefixMatch(LPCSTR, LPCSTR);
    DWORD ConfigureCache();
    DWORD GetCacheConfigInfo();
    DWORD SetLegacyCacheConfigInfo();
    DWORD GetSysRootCacheConfigInfo();
    void IncrementModifiedCount();
    DWORD ReadModifiedCount(BOOL fUpdateMemory);
    BOOL WasModified(BOOL fUpdateMemory);
    DWORD GetExtensibleCacheConfigInfo(BOOL fAlways);
    DWORD FindExtensibleContainer(LPCSTR Name);

    VOID DiscoverRegistrySettings(IE5_REGISTRYSET* pie5rs);
    VOID DiscoverIE3Settings(IE5_REGISTRYSET* pie5rs);
    BOOL DiscoverIE4Settings(IE5_REGISTRYSET* pie5rs);
    BOOL DiscoverAnyIE5Settings(IE5_REGISTRYSET* pie5rs);

public:
    CConMgr();
    ~CConMgr();

    static DWORD DeleteIE3Cache();
    
    // Methods specific to CConMgr.
    DWORD LoadContent();
    DWORD GetContainerInfo(LPSTR szUrl, LPINTERNET_CACHE_CONTAINER_INFOA pCI, LPDWORD pcbCI);
    DWORD GetStatus();
    DWORD FreeFindHandle(HANDLE hFind);
    BOOL DeleteFileIfNotRegistered(URL_CONTAINER *coDelete);
    DWORD CreateContainer(LPCSTR Name, LPCSTR CachePrefix, LPCSTR CachePath, DWORD KBCacheLimit, DWORD dwOptions);
    DWORD DeleteContainer(LPCSTR Name, DWORD dwOptions);
    HANDLE FindFirstContainer(DWORD *pdwModified, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize, DWORD dwOptions);
    BOOL FindNextContainer(HANDLE hFind, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize);
#ifdef CHECKLOCK_PARANOID
    void CheckNoLocks(DWORD dwThreadId);
#endif

    // Methods corresponding to URL_CONTAINER.
    DWORD UnlockUrl(LPCSTR);
    DWORD DeleteUrl(LPCSTR);

    BOOL GetHeaderData(DWORD, LPDWORD);
    BOOL SetHeaderData(DWORD, DWORD);
    BOOL IncrementHeaderData(DWORD, LPDWORD);
   
    DWORD SetUrlGroup(LPCSTR, DWORD, GROUPID);
    DWORD CreateUniqueFile(LPCSTR, DWORD, LPCSTR, LPTSTR, HANDLE*);
    DWORD AddUrl(AddUrlArg*);
    DWORD RetrieveUrl(LPCSTR, LPCACHE_ENTRY_INFOA*, LPDWORD, DWORD, DWORD);
    DWORD GetUrlInfo(LPCSTR, LPCACHE_ENTRY_INFOA*, LPDWORD, DWORD, DWORD, DWORD);
    DWORD GetUrlInfo(LPCSTR, LPCACHE_ENTRY_INFOA, LPDWORD, DWORD, DWORD);
    DWORD SetUrlInfo(LPCSTR, LPCACHE_ENTRY_INFOA, DWORD);
    DWORD FindNextEntry(HANDLE*, LPCSTR, LPCACHE_ENTRY_INFOA* ppNextCacheEntryInfo, LPDWORD, DWORD, GROUPID, DWORD, DWORD);
    DWORD FindNextEntry(HANDLE*, LPCSTR, LPCACHE_ENTRY_INFOA pNextCacheEntryInfo, LPDWORD, DWORD, GROUPID, DWORD);
    DWORD CleanupUrls(LPCTSTR szCachePath, DWORD dwFactor, DWORD dwFilter);
    DWORD GetUrlInGroup(LPCSTR, GROUPID*, LPDWORD);
    VOID SetCacheLimit(LONGLONG, DWORD);
    VOID GetCacheInfo(LPCSTR, LPSTR, LONGLONG*);


    // Methods corresponding to cache configuration APIs
    BOOL SetContentPath(PTSTR pszNewPath);
    BOOL GetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO, LPDWORD, DWORD);
    BOOL SetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO, DWORD);

    // Methods corresponding to cache group APIs
    DWORD CreateGroup(DWORD, GROUPID*);
    DWORD CreateDefaultGroups();
    DWORD DeleteGroup(GROUPID, DWORD);
    DWORD FindNextGroup(HANDLE*, DWORD, GROUPID*);
    DWORD GetGroupAttributes(
            GROUPID, DWORD, LPINTERNET_CACHE_GROUP_INFOA, LPDWORD);
    DWORD SetGroupAttributes(GROUPID, DWORD, LPINTERNET_CACHE_GROUP_INFOA );

    // notification api
    DWORD RegisterCacheNotify(HWND, UINT, GROUPID, DWORD);
    DWORD SendCacheNotification(DWORD);

    DWORD AddLeakFile (LPCSTR pszFile)
    {
        return _coContent->AddLeakFile (pszFile);
    }

    //  allows content import to create a directory with a known name
    BOOL CreateContentDirWithSecureName( LPSTR szDirName)
    {
        return _coContainer[CONTENT]->CreateDirWithSecureName( szDirName);
    }

    //  Creates a redirect from TargetUrl to OriginUrl
    BOOL CreateContentRedirect( LPSTR szTargetUrl, LPSTR szOriginUrl)
    {
        return _coContainer[CONTENT]->CreateRedirect( szTargetUrl, szOriginUrl);
    }

    VOID SetCacheSize(DWORD dwEnum, LONGLONG dlSize)
    {
        INET_ASSERT(dwEnum<NCONTAINERS);
        _coContainer[dwEnum]->SetCacheSize(dlSize);
    }

    VOID WalkLeakList(DWORD dwEnum)
    {
        INET_ASSERT(dwEnum<NCONTAINERS);

        if (_coContainer[dwEnum]->WalkLeakList())
        {
            _coContainer[dwEnum]->UnlockContainer();
        }
    }
};

#endif // _CONMGR_HXX
