// Total number of containers is 3: CONTENT, COOKIE and HISTORY.
#define N_CONTAINERS 3

class CContainerManager
{

public:
    // Indicese of the respective containers.
    enum ContainerIndex
    {
        CONTENT = 0,
        COOKIE,
        HISTORY,
        MAX_CONTAINERS
    };

private:
    struct InfoArray
    {
        CHAR szCachePath[MAX_PATH];
        CHAR szCachePrefix[MAX_PATH]; // make this smaller?
        LONGLONG cbCacheLimit;
    };

    DWORD dwStatus;
    const DWORD nContainers;

    // Array of containers.
    LPURL_CONTAINER ContainerArray[N_CONTAINERS];

    // Their paths, prefixes and limits.
    InfoArray Info[N_CONTAINERS];

    // Init function called by constructor.
    DWORD Init();

    // Config info get and set functions.

    // Utilities.
    ContainerIndex FindIndexFromPrefix(LPCSTR);    
    BOOL PathPrefixMatch(LPCSTR, LPCSTR);


public:
    CContainerManager();
    ~CContainerManager();

    // Methods specific to CContainerManager.
    DWORD GetStatus();
    DWORD GetHKLMCacheConfigInfo();
    DWORD SetHKLMCacheConfigInfo();
    DWORD GetHKCUCacheConfigInfo();
    DWORD SetHKCUCacheConfigInfo();
    DWORD GetDefaultCacheConfigInfo();

    // Methods corresponding to URL_CONTAINER.
    BOOL DeleteAPendingUrl();
    DWORD UnlockUrl(LPCSTR);
    DWORD DeleteUrl(LPCSTR);
    BOOL GetHeaderFlags(LPDWORD);
    BOOL IncrementHeaderFlags(LPDWORD);
    DWORD SetUrlInGroup(LPCSTR, DWORD, GROUPID, FILETIME);
    DWORD CreateUniqueFile(LPCSTR, DWORD, LPCSTR, LPTSTR);
    DWORD AddUrl(LPCSTR, LPCSTR, FILETIME, FILETIME, DWORD, LPBYTE, DWORD, LPCSTR);
    DWORD RetrieveUrl(LPCSTR, LPCACHE_ENTRY_INFOA, LPDWORD);
    DWORD GetUrlInfo(LPCSTR, LPCACHE_ENTRY_INFOA, LPDWORD);
    DWORD SetUrlInfo(LPCSTR, LPCACHE_ENTRY_INFOA, DWORD);
    HANDLE FindFirstEntry(LPCSTR, LPCACHE_ENTRY_INFOA, LPDWORD);
    BOOL FindNextEntry(HANDLE, LPCACHE_ENTRY_INFOA pNextCacheEntryInfo, LPDWORD);
    DWORD CleanupUrls(LPCTSTR szCachePath, DWORD dwFactor);
    DWORD GetUrlInGroup(LPCSTR, GROUPID*, FILETIME*);
    VOID SetCacheLimit(LONGLONG, ContainerIndex);
    VOID GetCacheInfo(LPCSTR, LPSTR, LONGLONG*);


    // Methods corresponding to cache configuration APIs
    BOOL GetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO, LPDWORD, DWORD);
    BOOL SetUrlCacheConfigInfo(LPCACHE_CONFIG_INFO, DWORD);


};

// Postfix operator for ContainerIndex
inline CContainerManager::ContainerIndex operator++(CContainerManager::ContainerIndex &ci, int )
{
    return ci = (CContainerManager::ContainerIndex)(ci + 1);
}
