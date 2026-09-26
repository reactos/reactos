#pragma once

#include <windows.h>
#include <shlobj.h>

#define MAX_CACHE_ENTRIES 64
#define SHELLBAG_SORT_DIRECTION_FLAG 0x80000000u

typedef struct
{
    UINT uViewMode; // Current view mode
    DWORD cxIcon; // Icon spacing X
    DWORD cyIcon; // Icon spacing Y
    UINT uSort; // Sort column/order
    POINT ptScroll; // Scroll position
    RECT rcWindow; // Window position and size
    BYTE rgViewOptions[256]; // Column widths and view options
} SHELLBAG_DATA;

// Internal cache entry
typedef struct
{
    WCHAR szPath[MAX_PATH]; // Folder path
    SHELLBAG_DATA data; // Associated view data
} ShellBagCacheEntry;

class CShellBagCache : public IUnknown
{
public:
    CShellBagCache();
    virtual ~CShellBagCache();

    HRESULT Initialize();

    HRESULT GetShellBagData(LPCWSTR pszPath, SHELLBAG_DATA* pBagData);
    HRESULT SetShellBagData(LPCWSTR pszPath, const SHELLBAG_DATA* pBagData);

    void ClearCache();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
private:
    HRESULT BuildRegistryPath(LPCWSTR pszPath, LPWSTR pszRegPath, UINT cchRegPath);
    HRESULT ReadBagDataFromRegistry(HKEY hKey, SHELLBAG_DATA* pBagData);
    HRESULT WriteBagDataToRegistry(HKEY hKey, const SHELLBAG_DATA* pBagData);
    HRESULT GetDefaultViewState(SHELLBAG_DATA* pBagData);

    ShellBagCacheEntry* FindInCache(LPCWSTR pszPath);
    HRESULT AddToCache(LPCWSTR pszPath, const SHELLBAG_DATA* pBagData);

    HKEY m_hRegistryRoot;
    BOOL m_bInitialized;
    LONG m_cRefCount;
    CRITICAL_SECTION m_csCache;
    SHELLBAG_DATA m_viewState;
    HDPA m_cacheEntries;
};
