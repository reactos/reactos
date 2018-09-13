// Author:  Karim Farouki
// Date:    24 June 1998

#include "priv.h"
#include "util.h"
#include <emptyvc.h>
#include <mluisupp.h>   // for MLLoadString
#include "resource.h"   // for the string ID's

typedef HRESULT (* LPFCALLBACK )(LPINTERNET_CACHE_ENTRY_INFO, void *);

typedef struct tagRTSCBSTRUCT   
{
    IEmptyVolumeCacheCallBack   * picb;
    DWORDLONG                   * pdwlSpaceUsed;
} RTSCBSTRUCT;  // RunningTotalSizeCallBack Struct

typedef struct tagDECBSTRUCT
{
    IEmptyVolumeCacheCallBack   * picb;
    DWORDLONG                   dwlSpaceFreed;
    DWORDLONG                   dwlTotalSpace;
} DECBSTRUCT;   // DeleteEntryCallBack Struct

class COfflinePagesCacheCleaner : public IEmptyVolumeCache2
{
    private:
        // Data
        ULONG       m_cRef;
        DWORDLONG   m_dwlSpaceUsed;
        TCHAR       m_szCacheDir[MAX_PATH + 1];

        // Functions
        HRESULT     WalkOfflineCache(
                        LPFCALLBACK     lpfCallBack,
                        void            * pv
                        );

        static HRESULT CALLBACK RunningTotalSizeCallback(
                        LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo, 
                        void                        * pv
                        );
        
        static HRESULT CALLBACK DeleteEntryCallback(
                        LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo, 
                        void                        * pv
                        );
        static VOID IncrementFileSize(
                        LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo, 
                        DWORDLONG                   * pdwlSize
                        );

        ~COfflinePagesCacheCleaner(void);

    public:
        // Constructor/Destructor
        COfflinePagesCacheCleaner(void);

        // IUnknown Interface members
        STDMETHODIMP            QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG)    AddRef(void);
        STDMETHODIMP_(ULONG)    Release(void);

        // IEmptyVolumeCache interface methods
        STDMETHODIMP    Initialize(
                            HKEY    hkRegKey,
                            LPCWSTR pcwszVolume,
                            LPWSTR  * ppwszDisplayName,
                            LPWSTR  * ppwszDescription,
                            DWORD   * pdwFlags
                            );

        STDMETHODIMP    GetSpaceUsed(
                            DWORDLONG                   * pdwlSpaceUsed,
                            IEmptyVolumeCacheCallBack   * picb
                            );

        STDMETHODIMP    Purge(
                            DWORDLONG                   dwlSpaceToFree,
                            IEmptyVolumeCacheCallBack   * picb
                            );

        STDMETHODIMP    ShowProperties(
                            HWND    hwnd
                            );

        STDMETHODIMP    Deactivate(
                            DWORD   * pdwFlags
                            );

        // IEmptyVolumeCache2 interface methods
        STDMETHODIMP    InitializeEx(
                            HKEY hkRegKey,
                            LPCWSTR pcwszVolume,
                            LPCWSTR pcwszKeyName,
                            LPWSTR *ppwszDisplayName,
                            LPWSTR *ppwszDescription,
                            LPWSTR *ppwszBtnText,
                            DWORD *pdwFlags
                            );
};

STDAPI COfflinePagesCacheCleaner_CreateInstance(
    IUnknown        * punkOuter,
    IUnknown        ** ppunk,
    LPCOBJECTINFO   poi
    )
{
    HRESULT hr = S_OK;
    
    *ppunk = NULL;

    COfflinePagesCacheCleaner * lpOPCC = new COfflinePagesCacheCleaner();

    if (lpOPCC == NULL)
        hr = E_OUTOFMEMORY;
    else
        *ppunk = SAFECAST(lpOPCC, IEmptyVolumeCache *);

    return hr;
}

COfflinePagesCacheCleaner::COfflinePagesCacheCleaner() : m_cRef(1)
{
    DllAddRef();
}

COfflinePagesCacheCleaner::~COfflinePagesCacheCleaner()
{
    DllRelease();
}

STDMETHODIMP COfflinePagesCacheCleaner::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEmptyVolumeCache) || IsEqualIID(riid, IID_IEmptyVolumeCache2))
    {
        *ppv = SAFECAST(this, IEmptyVolumeCache2 *);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

STDMETHODIMP_(ULONG) COfflinePagesCacheCleaner::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) COfflinePagesCacheCleaner::Release()
{
    ULONG uRefCount = --m_cRef;
    
    if (!uRefCount)
        delete this;

    return uRefCount;
}

STDMETHODIMP COfflinePagesCacheCleaner::InitializeEx(
    HKEY hkRegKey,
    LPCWSTR pcwszVolume,
    LPCWSTR pcwszKeyName,
    LPWSTR *ppwszDisplayName,
    LPWSTR *ppwszDescription,
    LPWSTR *ppwszBtnText,
    DWORD *pdwFlags
    )
{
    *ppwszBtnText = (LPWSTR)CoTaskMemAlloc( 128*sizeof(WCHAR) );
    if ( !*ppwszBtnText )
        return E_OUTOFMEMORY;

    MLLoadString( IDS_CACHEOFF_BTNTEXT, *ppwszBtnText, 512 );

    return Initialize(hkRegKey, pcwszVolume, ppwszDisplayName, ppwszDescription, pdwFlags );
}

STDMETHODIMP COfflinePagesCacheCleaner::Initialize(
    HKEY    hkRegkey,
    LPCWSTR pcwszVolume,
    LPWSTR  * ppwszDisplayName,
    LPWSTR  * ppwszDescription,
    DWORD   * pdwFlags
    )
{
    HRESULT         hr;
    uCLSSPEC        ucs;          // Used to see if Webcheck is installed
    QUERYCONTEXT    qc = { 0 };   // Used to see if Webcheck is installed
    DWORDLONG       dwlSize;      // Amount of offline cachespace


#ifdef UNICODE
    // We can't use the registry values on NT because they can't be multi-local localized.
    // Instead we must use strings loaded from resources.
    *ppwszDisplayName = (LPWSTR)CoTaskMemAlloc( 512*sizeof(WCHAR) );
    if ( !*ppwszDisplayName )
        return E_OUTOFMEMORY;

    *ppwszDescription = (LPWSTR)CoTaskMemAlloc( 512*sizeof(WCHAR) );
    if ( !*ppwszDescription )
        return E_OUTOFMEMORY;

    MLLoadString( IDS_CACHEOFF_DISPLAY, *ppwszDisplayName, 512 );
    MLLoadString( IDS_CACHEOFF_DESCRIPTION, *ppwszDescription, 512 );

#else
    // We can use the default registry DisplayName and Description
    *ppwszDisplayName = NULL;
    *ppwszDescription = NULL;
#endif

    // Intentionally am not turning on cleanup by default; turning on *view pages* button
    *pdwFlags = EVCF_HASSETTINGS;

    // Let's check if the Internet Cache Folder is in pcwzVolume
    GetCacheLocation(m_szCacheDir, sizeof(m_szCacheDir));
    if (StrCmpNI(pcwszVolume, m_szCacheDir, 3))
    {
        // If the cache is on a different drive return S_FALSE so that we don't show up in UI
        return S_FALSE;
    }

    // Determine if offline browsing pack is intalled.
    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = CLSID_SubscriptionMgr;

    hr = FaultInIEFeature(NULL, &ucs, &qc, FIEF_FLAG_PEEK | FIEF_FLAG_FORCE_JITUI);
    
    if (SUCCEEDED(hr))  // (if offline pack installed)
    {
        GetSpaceUsed(&dwlSize, NULL);  
        
        if (dwlSize)        // If there is something in offline cache to delete
            return S_OK;    // load cleaner/
    }

    return S_FALSE;
}

STDMETHODIMP COfflinePagesCacheCleaner::GetSpaceUsed(
    DWORDLONG                   * pdwlSpaceUsed,
    IEmptyVolumeCacheCallBack   * picb
    )
{
    HRESULT hr;

    ASSERT(NULL != pdwlSpaceUsed);

    if (NULL != pdwlSpaceUsed)
    {
        RTSCBSTRUCT * prtscbStruct = new RTSCBSTRUCT;
    
        if (NULL != prtscbStruct)
        {
            // Initialize GetSpazeUsed Structure
            prtscbStruct->pdwlSpaceUsed = pdwlSpaceUsed;
            *(prtscbStruct->pdwlSpaceUsed) = 0;
            prtscbStruct->picb = picb;

            // Get Offline Cache Space Usage    
            hr = WalkOfflineCache(RunningTotalSizeCallback, (void *)(prtscbStruct));
            m_dwlSpaceUsed = *(prtscbStruct->pdwlSpaceUsed);

            // Send the last notification to the cleanup manager
            if (picb != NULL)
                picb->ScanProgress(*(prtscbStruct->pdwlSpaceUsed), EVCCBF_LASTNOTIFICATION, NULL);

            delete prtscbStruct;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP COfflinePagesCacheCleaner::Purge(
    DWORDLONG                   dwlSpaceToFree,   // Spec makes this irrelevent!
    IEmptyVolumeCacheCallBack   * picb
    )
{
    HRESULT hr;

    DECBSTRUCT  * pdecbStruct = new DECBSTRUCT;

    if (NULL != pdecbStruct)
    {
        // Initialize DeleteEntry Structure
        pdecbStruct->picb = picb;
        pdecbStruct->dwlSpaceFreed = 0;
        pdecbStruct->dwlTotalSpace = m_dwlSpaceUsed;

        //  Delete Offline Cache Entries
        hr = WalkOfflineCache(DeleteEntryCallback, (void *)(pdecbStruct));

        // Send the last notification to the cleanup manager
        if (picb != NULL)
            picb->PurgeProgress(m_dwlSpaceUsed, 0, EVCCBF_LASTNOTIFICATION, NULL);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

STDMETHODIMP COfflinePagesCacheCleaner::ShowProperties(HWND hwnd)
{
    TCHAR szOfflinePath[MAX_PATH];
    DWORD dwSize = SIZEOF(szOfflinePath);

    if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SUBSCRIPTION,
                       REGSTR_VAL_DIRECTORY, NULL, (LPBYTE)szOfflinePath, &dwSize) != ERROR_SUCCESS)
    {
        TCHAR szWindows[MAX_PATH];

        GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));
        PathCombine(szOfflinePath, szWindows, TEXT("Offline Web Pages"));
    }

    SHELLEXECUTEINFO shei;
    ZeroMemory(&shei, sizeof(shei));
    shei.cbSize     = sizeof(shei);
    shei.lpFile     = szOfflinePath;
    shei.nShow      = SW_SHOWNORMAL;
    ShellExecuteEx(&shei);
    
    // Returning S_OK insures that GetSpaceUsed is recalled (to recalc) the size being
    // used (in case someone deletes some MAO stuff).
    return S_OK;
}

STDMETHODIMP COfflinePagesCacheCleaner::Deactivate(DWORD * pdwFlags)
{
    // We don't implement this.
    *pdwFlags = 0;

    return S_OK;
}

HRESULT COfflinePagesCacheCleaner::WalkOfflineCache(
    LPFCALLBACK     lpfCallBack,
    void *          pv
    )
{
    HRESULT hr = S_OK;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;
    HANDLE hCacheEntryInfo;
    DWORD dwCacheEntryInfoSize;
    
    if ((lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, 
                                        MAX_CACHE_ENTRY_INFO_SIZE)) == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        dwCacheEntryInfoSize = MAX_CACHE_ENTRY_INFO_SIZE;
        if ((hCacheEntryInfo = FindFirstUrlCacheEntry(NULL, lpCacheEntryInfo,
                                        &dwCacheEntryInfoSize)) == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    
        if (SUCCEEDED(hr))
        {
            do
            {   
                if (lpCacheEntryInfo->CacheEntryType & STICKY_CACHE_ENTRY)
                {
                    hr = lpfCallBack(lpCacheEntryInfo, pv);
                }

                dwCacheEntryInfoSize = MAX_CACHE_ENTRY_INFO_SIZE;
            } while ((E_ABORT != hr) &&
                     FindNextUrlCacheEntry(hCacheEntryInfo, lpCacheEntryInfo,
                                           &dwCacheEntryInfoSize));
            
            if (hr != E_ABORT) 
            {
                DWORD dwLastErr = GetLastError();

                if (dwLastErr != ERROR_NO_MORE_ITEMS)
                {
                    hr = HRESULT_FROM_WIN32(dwLastErr);
                }
            }
        }
    
        LocalFree(lpCacheEntryInfo);
    }

    return hr;
}

HRESULT CALLBACK COfflinePagesCacheCleaner::RunningTotalSizeCallback(
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,  // Name of the CacheEntry to sum
    void                        * pv               // A RTSCBSTRUCT
    )
{
    HRESULT hr = S_OK;
    RTSCBSTRUCT * prtscbStruct = (RTSCBSTRUCT *)pv;

    // Add current file size to total    
    IncrementFileSize(lpCacheEntryInfo, prtscbStruct->pdwlSpaceUsed);

    // Update the progressbar!
    if (prtscbStruct->picb != NULL)
        hr = prtscbStruct->picb->ScanProgress(*(prtscbStruct->pdwlSpaceUsed), 0, NULL);

    return hr;
}

HRESULT CALLBACK COfflinePagesCacheCleaner::DeleteEntryCallback(
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo, // Name of the CacheEntry to delete
    void                        * pv              // Pointer to DECBSTRUCT
    )
{
    HRESULT     hr = S_OK;
    DECBSTRUCT  * pdecbStruct = (DECBSTRUCT *)pv;
    
    // Add current file size to total deleted
    IncrementFileSize(lpCacheEntryInfo, &(pdecbStruct->dwlSpaceFreed));

    DeleteUrlCacheEntry(lpCacheEntryInfo->lpszSourceUrlName);   

    // Update the progress bar!
    if (pdecbStruct->picb != NULL)
    {
        hr =  pdecbStruct->picb->PurgeProgress(pdecbStruct->dwlSpaceFreed, 
            pdecbStruct->dwlTotalSpace - pdecbStruct->dwlSpaceFreed, NULL, NULL);
    }

    return hr;
}

VOID COfflinePagesCacheCleaner::IncrementFileSize(
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo, 
    DWORDLONG                   * pdwlSize
    )
{
    ULARGE_INTEGER uliFileSize;
    
    uliFileSize.HighPart = lpCacheEntryInfo->dwSizeHigh;
    uliFileSize.LowPart = lpCacheEntryInfo->dwSizeLow;

    *pdwlSize += QUAD_PART(uliFileSize);
}
