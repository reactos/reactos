// Author:  Pritvinath Obla
// Date:    10 July 1997

#include "priv.h"
#include "util.h"
#include <emptyvc.h>
#include <mluisupp.h>   // for MLLoadString
#include "resource.h"   // for the string ID's

class CInternetCacheCleaner : public IEmptyVolumeCache2
{
private:
    //
    // Data
    //
    ULONG                   m_cRef;             // reference count
    DWORDLONG               m_dwlSpaceUsed;
    TCHAR                   m_szCacheDir[MAX_PATH + 1];

    //
    // Functions
    //
    HRESULT                 GetInternetCacheSize(
                                DWORDLONG                   *pdwlSpaceUsed,
                                IEmptyVolumeCacheCallBack   *picb
                                );

    HRESULT                 DelInternetCacheFiles(
                                DWORD                       dwPercentToFree,
                                IEmptyVolumeCacheCallBack   *picb
                                );

public:
    //
    // Constructor and Destructor
    //
    CInternetCacheCleaner(void);
    ~CInternetCacheCleaner(void);

    //
    // IUnknown interface members
    //
    STDMETHODIMP            QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    //
    // IEmptyVolumeCache interface methods
    //
    STDMETHODIMP            Initialize(
                                HKEY    hkRegKey,
                                LPCWSTR pcwszVolume,
                                LPWSTR  *ppwszDisplayName,
                                LPWSTR  *ppwszDescription,
                                DWORD   *pdwFlags
                                );

    STDMETHODIMP            GetSpaceUsed(
                                DWORDLONG                   *pdwlSpaceUsed,
                                IEmptyVolumeCacheCallBack   *picb
                                );

    STDMETHODIMP            Purge(
                                DWORDLONG                   dwlSpaceToFree,
                                IEmptyVolumeCacheCallBack   *picb
                                );

    STDMETHODIMP            ShowProperties(
                                HWND    hwnd
                                );

    STDMETHODIMP            Deactivate(
                                DWORD   *pdwFlags
                                );

    //
    // IEmptyVolumeCache2 interface methods
    //
    STDMETHODIMP            InitializeEx(
                                HKEY hkRegKey,
                                LPCWSTR pcwszVolume,
                                LPCWSTR pcwszKeyName,
                                LPWSTR *ppwszDisplayName,
                                LPWSTR *ppwszDescription,
                                LPWSTR *ppwszBtnText,
                                DWORD *pdwFlags
                                );
};

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner_CreateInstance
//
// Purpose:     CreateInstance function for IClassFactory
//------------------------------------------------------------------------------
//
STDAPI CInternetCacheCleaner_CreateInstance(
    IUnknown        *punkOuter,
    IUnknown        **ppunk,
    LPCOBJECTINFO   poi
    )
{
    *ppunk = NULL;

    CInternetCacheCleaner *lpICC = new CInternetCacheCleaner();

    if (lpICC == NULL)
        return E_OUTOFMEMORY;

    *ppunk = SAFECAST(lpICC, IEmptyVolumeCache *);

    return S_OK;
}

CInternetCacheCleaner::CInternetCacheCleaner() : m_cRef(1)
{
    DllAddRef();

    m_dwlSpaceUsed = 0;
    *m_szCacheDir = '\0';
}

CInternetCacheCleaner::~CInternetCacheCleaner()
{
    DllRelease();
}

STDMETHODIMP CInternetCacheCleaner::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (IsEqualIID(riid, IID_IUnknown)  ||
        IsEqualIID(riid, IID_IEmptyVolumeCache2) ||
        IsEqualIID(riid, IID_IEmptyVolumeCache))
    {
        *ppv = SAFECAST(this, IEmptyVolumeCache2 *);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CInternetCacheCleaner::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CInternetCacheCleaner::Release()
{
    //  
    // Decrement and check
    //
    if (--m_cRef)
        return m_cRef;

    //
    // No references left to this object
    //
    delete this;

    return 0;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::InitializeEx
//
// Purpose:     Initializes the Internet Cache Cleaner and returns the 
//                  specified IEmptyVolumeCache flags to the cache manager
//------------------------------------------------------------------------------
//

STDMETHODIMP CInternetCacheCleaner::InitializeEx(
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

    MLLoadString( IDS_CACHECLN_BTNTEXT, *ppwszBtnText, 128 );

    return Initialize(hkRegKey, pcwszVolume, ppwszDisplayName, ppwszDescription, pdwFlags );
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::Initialize
//
// Purpose:     Initializes the Internet Cache Cleaner and returns the 
//                  specified IEmptyVolumeCache flags to the cache manager
//------------------------------------------------------------------------------
//
STDMETHODIMP CInternetCacheCleaner::Initialize(
    HKEY    hkRegKey,
    LPCWSTR pcwszVolume,
    LPWSTR  *ppwszDisplayName,
    LPWSTR  *ppwszDescription,
    DWORD   *pdwFlags
    )
{
#ifdef UNICODE
    // We can't use the registry values on NT because they can't be multi-local localized.  As
    // a result we must set the out pointers with values read from resources.
    *ppwszDisplayName = (LPWSTR)CoTaskMemAlloc( 512*sizeof(WCHAR) );
    if ( !*ppwszDisplayName )
        return E_OUTOFMEMORY;

    *ppwszDescription = (LPWSTR)CoTaskMemAlloc( 512*sizeof(WCHAR) );
    if ( !*ppwszDescription )
        return E_OUTOFMEMORY;

    MLLoadString( IDS_CACHECLN_DISPLAY, *ppwszDisplayName, 512 );
    MLLoadString( IDS_CACHECLN_DESCRIPTION, *ppwszDescription, 512 );
#else
    //
    // Let cleanmgr read the default DisplayName and Description
    //      from hkRegKey and use them
    //
    *ppwszDisplayName = NULL;
    *ppwszDescription = NULL;
#endif

    *pdwFlags = 0;              // initialize the [out] parameter

    //
    // Check if the Internet Cache Folder is in pcwzVolume
    //
    GetCacheLocation(m_szCacheDir, sizeof(m_szCacheDir));
    if (StrCmpNI(pcwszVolume, m_szCacheDir, 3))
    {
        //
        // Different drives; return S_FALSE so that this cleaner
        //      doesn't show up in cleanmgr's UI
        //
        return S_FALSE;
    }

    //
    // Enable this cleaner by default both in cleanup and tuneup modes
    //
    *pdwFlags = EVCF_ENABLEBYDEFAULT |
                EVCF_ENABLEBYDEFAULT_AUTO |
                EVCF_HASSETTINGS;

#if 0
    /***
    // BUGBUG: Since GetInternetCacheSize returns only an approx. size,
    //      we would never get a value of 0 even if the cache is empty
    // Should enable this check once wininet.dll exports a GetCacheSize API

    //
    // Check if there is any disk space to free at all
    // If not, return S_FALSE so that this cleaner doesn't show up in
    //      cleanmgr's UI
    //
    DWORDLONG dwlSpaceUsed;
    if (SUCCEEDED(GetInternetCacheSize(&dwlSpaceUsed, NULL))  &&
        dwlSpaceUsed == 0)
    {
        return S_FALSE;
    }
    ***/
#endif

    return S_OK;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::GetSpaceUsed
//
// Purpose:     Return the total amount of space this internet cache cleaner
//                  can free up
//------------------------------------------------------------------------------
//
STDMETHODIMP CInternetCacheCleaner::GetSpaceUsed(
    DWORDLONG                   *pdwlSpaceUsed,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    HRESULT hr;

    hr = GetInternetCacheSize(pdwlSpaceUsed, picb);
    m_dwlSpaceUsed = *pdwlSpaceUsed;

    //
    // Send the last notification to the cleanup manager
    //
    if (picb != NULL)
    {
        picb->ScanProgress(*pdwlSpaceUsed, EVCCBF_LASTNOTIFICATION, NULL);
    }

    if (hr != E_ABORT)
    {
        if (FAILED(hr))
        {
            //
            // *pdwlSpaceUsed is only a guesstimate; so return S_FALSE
            //
            hr = S_FALSE;
        }
        else
        {
            //
            // BUGBUG: Return S_OK once wininet exports a GetCacheSize API;
            //      till then use FindFirstUrlCacheEntry/FindNextUrlCacheEntry
            //      to get approx. size of the cache
            //
            hr = S_FALSE;
        }
    }

    return hr;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::Purge
//
// Purpose:     Delete the internet cache files
//------------------------------------------------------------------------------
//
STDMETHODIMP CInternetCacheCleaner::Purge(
    DWORDLONG                   dwlSpaceToFree,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    HRESULT hr;
    DWORD dwPercentToFree = 100;    // Optimize the most common scenario:
                                    // In most cases, dwlSpaceToFree will be
                                    //      equal to m_dwlSpaceUsed

    if (dwlSpaceToFree != m_dwlSpaceUsed)
    {
        dwPercentToFree = m_dwlSpaceUsed ?
                                DWORD((dwlSpaceToFree * 100) / m_dwlSpaceUsed) :
                                100;
    }

    hr = DelInternetCacheFiles(dwPercentToFree, picb);

    //
    // Send the last notification to the cleanup manager
    //
    if (picb != NULL)
    {
        picb->PurgeProgress(dwlSpaceToFree, 0,
                                EVCCBF_LASTNOTIFICATION, NULL);
    }

    if (hr != E_ABORT)
    {
        hr = S_OK;          // cannot return anything else
    }

    return hr;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::ShowProperties
//
// Purpose:     Launch the cache viewer to list the internet cache files
//------------------------------------------------------------------------------
//
STDMETHODIMP CInternetCacheCleaner::ShowProperties(
    HWND    hwnd
    )
{
    DWORD dwAttrib;

    if (*m_szCacheDir == '\0')      // Internet cache dir is not yet initialized
    {
        GetCacheLocation(m_szCacheDir, sizeof(m_szCacheDir));
    }

    dwAttrib = GetFileAttributes(m_szCacheDir);
    if (dwAttrib != 0xffffffff  &&  (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        SHELLEXECUTEINFO sei;

        //
        // Launch the cache viewer
        //
        sei.cbSize          = sizeof(SHELLEXECUTEINFO);
        sei.hwnd            = hwnd;
        sei.lpVerb          = NULL;
        sei.lpFile          = m_szCacheDir;
        sei.lpParameters    = NULL;
        sei.lpDirectory     = NULL;
        sei.nShow           = SW_SHOWNORMAL;
        sei.fMask           = 0;

        ShellExecuteEx(&sei);
    }

    //
    // The user may or may not delete files directly from the cachevu folder
    // Since there is no way of knowing this, don't return S_OK which would
    //      trigger cleanmgr to call GetSpaceUsed again
    //
    return S_OK;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::Deactivate
//
// Purpose:     Deactivates the Internet Cache Cleaner...Not implemented
//------------------------------------------------------------------------------
//
STDMETHODIMP CInternetCacheCleaner::Deactivate(
    DWORD   *pdwFlags
    )
{
    *pdwFlags = 0;

    return S_OK;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::GetInternetCacheSize
//
// Purpose:     Find the size of the internet cache by calling into wininet APIs
//
// Notes:       The current implementation is temporary; once wininet exports
//                  a real API for getting the cache size, use that
//------------------------------------------------------------------------------
//
HRESULT CInternetCacheCleaner::GetInternetCacheSize(
    DWORDLONG                   *pdwlSpaceUsed,
    IEmptyVolumeCacheCallBack   *picb           // not used
    )
{
    HRESULT hr = S_OK;
    DWORD dwLastErr;
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo;
    HANDLE hCacheEntryInfo;
    DWORD dwCacheEntryInfoSize;

    *pdwlSpaceUsed = 0;

    if ((lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFOA) LocalAlloc(LPTR,
                                        MAX_CACHE_ENTRY_INFO_SIZE)) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    dwCacheEntryInfoSize = MAX_CACHE_ENTRY_INFO_SIZE;
    if ((hCacheEntryInfo = FindFirstUrlCacheEntryA(NULL, lpCacheEntryInfo,
                                            &dwCacheEntryInfoSize)) == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        do
        {
            if (!(lpCacheEntryInfo->CacheEntryType & (STICKY_CACHE_ENTRY | COOKIE_CACHE_ENTRY)))
            {
                ULARGE_INTEGER uliFileSize;

                uliFileSize.HighPart = lpCacheEntryInfo->dwSizeHigh;
                uliFileSize.LowPart = lpCacheEntryInfo->dwSizeLow;

                *pdwlSpaceUsed += QUAD_PART(uliFileSize);
            }
            
            dwCacheEntryInfoSize = MAX_CACHE_ENTRY_INFO_SIZE;

        } while (FindNextUrlCacheEntryA(hCacheEntryInfo, lpCacheEntryInfo,
                                                    &dwCacheEntryInfoSize));

        if ((dwLastErr = GetLastError()) != ERROR_NO_MORE_ITEMS)
        {
            hr = HRESULT_FROM_WIN32(dwLastErr);
        }
    }

    if (lpCacheEntryInfo != NULL)
        LocalFree(lpCacheEntryInfo);

    return hr;
}

//
//------------------------------------------------------------------------------
// CInternetCacheCleaner::DelInternetCacheFiles
//
// Purpose:     Delete the internet cache files
//------------------------------------------------------------------------------
//
HRESULT CInternetCacheCleaner::DelInternetCacheFiles(
    DWORD                       dwPercentToFree,
    IEmptyVolumeCacheCallBack   *picb           // not used
    )
{
    HRESULT hr = S_OK;

    if (*m_szCacheDir == '\0')      // Internet cache dir is not yet initialized
    {
        hr = GetCacheLocation(m_szCacheDir, sizeof(m_szCacheDir));
    }

    if (SUCCEEDED(hr))
    {
        FreeUrlCacheSpace(m_szCacheDir, dwPercentToFree, STICKY_CACHE_ENTRY);
    }

    return hr;
}
