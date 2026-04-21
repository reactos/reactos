#include "precomp.h"
#include "CShellBagCache.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <propvarutil.h>

#define SHELLBAG_SUBKEY L"Software\\Microsoft\\Windows\\Shell\\Bags"

CShellBagCache::CShellBagCache()
    : m_hRegistryRoot(NULL), m_bInitialized(FALSE), m_cRefCount(1), m_cacheEntries(NULL)
{
    ZeroMemory(&m_viewState, sizeof(m_viewState));
    InitializeCriticalSection(&m_csCache);
    m_cacheEntries = DPA_Create(8);
}

CShellBagCache::~CShellBagCache()
{
    ClearCache();
    if (m_hRegistryRoot)
        RegCloseKey(m_hRegistryRoot);
    DeleteCriticalSection(&m_csCache);
}

HRESULT CShellBagCache::Initialize()
{
    if (m_bInitialized)
        return S_OK;

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csCache);

    LONG lRet = RegOpenKeyExW(HKEY_CURRENT_USER, SHELLBAG_SUBKEY, 0,
                              KEY_READ | KEY_WRITE, &m_hRegistryRoot);
    if (lRet != ERROR_SUCCESS)
    {
        lRet = RegCreateKeyExW(HKEY_CURRENT_USER, SHELLBAG_SUBKEY, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                               NULL, &m_hRegistryRoot, NULL);
    }

    if (lRet == ERROR_SUCCESS)
    {
        m_bInitialized = TRUE;
        ZeroMemory(&m_viewState, sizeof(m_viewState));
        m_viewState.uViewMode = FVM_LIST;
        m_viewState.cxIcon = 0;
        m_viewState.cyIcon = 0;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(lRet);
    }

    LeaveCriticalSection(&m_csCache);
    return hr;
}

HRESULT CShellBagCache::GetShellBagData(LPCWSTR pszPath, SHELLBAG_DATA* pBagData)
{
    if (!pszPath || !pBagData)
        return E_INVALIDARG;
    if (!m_bInitialized)
        return E_FAIL;

    HRESULT hr = S_OK;
    WCHAR szSubKeyPath[MAX_PATH];
    HKEY hBagKey = NULL;

    EnterCriticalSection(&m_csCache);

    ShellBagCacheEntry* pEntry = FindInCache(pszPath);
    if (pEntry)
    {
        *pBagData = pEntry->data;
        LeaveCriticalSection(&m_csCache);
        return S_OK;
    }

    hr = BuildRegistryPath(pszPath, szSubKeyPath, _countof(szSubKeyPath));
    if (SUCCEEDED(hr))
    {
        LONG lRet = RegOpenKeyExW(m_hRegistryRoot, szSubKeyPath, 0, KEY_READ, &hBagKey);
        if (lRet == ERROR_FILE_NOT_FOUND)
        {
            hr = GetDefaultViewState(pBagData);
        }
        else if (lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
        }
        else
        {
            hr = ReadBagDataFromRegistry(hBagKey, pBagData);
            if (SUCCEEDED(hr))
                AddToCache(pszPath, pBagData);
        }
    }

    if (hBagKey)
        RegCloseKey(hBagKey);

    LeaveCriticalSection(&m_csCache);
    return hr;
}

HRESULT CShellBagCache::SetShellBagData(LPCWSTR pszPath, const SHELLBAG_DATA* pBagData)
{
    if (!pszPath || !pBagData)
        return E_INVALIDARG;
    if (!m_bInitialized)
        return E_FAIL;

    HRESULT hr = S_OK;
    WCHAR szSubKeyPath[MAX_PATH];
    HKEY hBagKey = NULL;
    DWORD dwDisposition;

    EnterCriticalSection(&m_csCache);

    hr = BuildRegistryPath(pszPath, szSubKeyPath, _countof(szSubKeyPath));
    if (SUCCEEDED(hr))
    {
        LONG lRet = RegCreateKeyExW(m_hRegistryRoot, szSubKeyPath, 0, NULL,
                                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                    NULL, &hBagKey, &dwDisposition);
        if (lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
        }
        else
        {
            hr = WriteBagDataToRegistry(hBagKey, pBagData);
            if (SUCCEEDED(hr))
                AddToCache(pszPath, pBagData);
        }
    }

    if (hBagKey)
        RegCloseKey(hBagKey);

    LeaveCriticalSection(&m_csCache);
    return hr;
}

HRESULT CShellBagCache::BuildRegistryPath(LPCWSTR pszPath, LPWSTR pszRegPath, UINT cchRegPath)
{
    if (!pszPath || !pszRegPath || cchRegPath == 0)
        return E_INVALIDARG;
    
    StringCchCopyW(pszRegPath, cchRegPath, pszPath);
    
    for (LPWSTR p = pszRegPath; *p; p++)
    {
        switch (*p)
        {
            case L'\\':
            case L'/':
                *p = L'_';
                break;
            case L':':
                *p = L'_';
                break;
        }
    }
    
    return S_OK;
}

HRESULT CShellBagCache::ReadBagDataFromRegistry(HKEY hKey, SHELLBAG_DATA* pBagData)
{
    if (!hKey || !pBagData)
        return E_INVALIDARG;
    
    LONG lRet;
    DWORD dwType, dwSize;
    
    ZeroMemory(pBagData, sizeof(*pBagData));
    
    dwSize = sizeof(pBagData->uViewMode);
    lRet = RegQueryValueExW(hKey, L"ViewMode", NULL, &dwType, 
                            (LPBYTE)&pBagData->uViewMode, &dwSize);
    if (lRet != ERROR_SUCCESS)
        pBagData->uViewMode = FVM_LIST;
    
    dwSize = sizeof(pBagData->cxIcon);
    RegQueryValueExW(hKey, L"IconSpacingX", NULL, &dwType, 
                     (LPBYTE)&pBagData->cxIcon, &dwSize);
    
    dwSize = sizeof(pBagData->cyIcon);
    RegQueryValueExW(hKey, L"IconSpacingY", NULL, &dwType, 
                     (LPBYTE)&pBagData->cyIcon, &dwSize);
    
    dwSize = sizeof(pBagData->rgViewOptions);
    RegQueryValueExW(hKey, L"ViewOptions", NULL, &dwType,
                     (LPBYTE)pBagData->rgViewOptions, &dwSize);
    
    dwSize = sizeof(pBagData->uSort);
    RegQueryValueExW(hKey, L"SortOrder", NULL, &dwType,
                     (LPBYTE)&pBagData->uSort, &dwSize);
    
    dwSize = sizeof(pBagData->ptScroll);
    RegQueryValueExW(hKey, L"ScrollPos", NULL, &dwType,
                     (LPBYTE)&pBagData->ptScroll, &dwSize);
    
    dwSize = sizeof(pBagData->rcWindow);
    RegQueryValueExW(hKey, L"WindowRect", NULL, &dwType,
                     (LPBYTE)&pBagData->rcWindow, &dwSize);
    
    return S_OK;
}

HRESULT CShellBagCache::WriteBagDataToRegistry(HKEY hKey, const SHELLBAG_DATA* pBagData)
{
    if (!hKey || !pBagData)
        return E_INVALIDARG;
    
    LONG lRet;
    
    lRet = RegSetValueExW(hKey, L"ViewMode", 0, REG_DWORD,
                          (LPBYTE)&pBagData->uViewMode, sizeof(pBagData->uViewMode));
    if (lRet != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRet);
    
    if (pBagData->cxIcon > 0)
    {
        RegSetValueExW(hKey, L"IconSpacingX", 0, REG_DWORD,
                       (LPBYTE)&pBagData->cxIcon, sizeof(pBagData->cxIcon));
    }
    
    if (pBagData->cyIcon > 0)
    {
        RegSetValueExW(hKey, L"IconSpacingY", 0, REG_DWORD,
                       (LPBYTE)&pBagData->cyIcon, sizeof(pBagData->cyIcon));
    }
    
    RegSetValueExW(hKey, L"ViewOptions", 0, REG_BINARY,
                   (LPBYTE)pBagData->rgViewOptions, sizeof(pBagData->rgViewOptions));
    
    RegSetValueExW(hKey, L"SortOrder", 0, REG_DWORD,
                   (LPBYTE)&pBagData->uSort, sizeof(pBagData->uSort));
    
    RegSetValueExW(hKey, L"ScrollPos", 0, REG_BINARY,
                   (LPBYTE)&pBagData->ptScroll, sizeof(pBagData->ptScroll));
    
    RegSetValueExW(hKey, L"WindowRect", 0, REG_BINARY,
                   (LPBYTE)&pBagData->rcWindow, sizeof(pBagData->rcWindow));
    
    return S_OK;
}

HRESULT CShellBagCache::GetDefaultViewState(SHELLBAG_DATA* pBagData)
{
    if (!pBagData)
        return E_INVALIDARG;
    
    ZeroMemory(pBagData, sizeof(*pBagData));
    pBagData->uViewMode = FVM_LIST;
    pBagData->uSort = 0;
    SetRect(&pBagData->rcWindow, 0, 0, 800, 600);
    pBagData->ptScroll.x = 0;
    pBagData->ptScroll.y = 0;
    
    return S_OK;
}

ShellBagCacheEntry* CShellBagCache::FindInCache(LPCWSTR pszPath)
{
    if (!m_cacheEntries) return NULL;

    for (INT i = 0; i < DPA_GetPtrCount(m_cacheEntries); i++)
    {
        ShellBagCacheEntry* pEntry = (ShellBagCacheEntry*)DPA_FastGetPtr(m_cacheEntries, i);
        if (pEntry && wcscmp(pEntry->szPath, pszPath) == 0)
            return pEntry;
    }
    return NULL;
}

HRESULT CShellBagCache::AddToCache(LPCWSTR pszPath, const SHELLBAG_DATA* pBagData)
{
    if (!pszPath || !pBagData || !m_cacheEntries)
        return E_INVALIDARG;

    ShellBagCacheEntry* pEntry = FindInCache(pszPath);
    if (pEntry)
    {
        pEntry->data = *pBagData;
        return S_OK;
    }

    if (DPA_GetPtrCount(m_cacheEntries) >= MAX_CACHE_ENTRIES)
    {
        ShellBagCacheEntry* pOld = (ShellBagCacheEntry*)DPA_DeletePtr(m_cacheEntries, 0);
        delete pOld;
    }

    ShellBagCacheEntry* pNew = new ShellBagCacheEntry;
    if (!pNew) return E_OUTOFMEMORY;

    StringCchCopyW(pNew->szPath, _countof(pNew->szPath), pszPath);
    pNew->data = *pBagData;

    if (DPA_AppendPtr(m_cacheEntries, pNew) == -1)
    {
        delete pNew;
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static INT CALLBACK DeleteCacheEntry(LPVOID p, LPVOID)
{
    delete static_cast<ShellBagCacheEntry*>(p);
    return 0;
}

void CShellBagCache::ClearCache()
{
    EnterCriticalSection(&m_csCache);
    if (m_cacheEntries)
    {
        DPA_DestroyCallback(m_cacheEntries, DeleteCacheEntry, NULL);
        m_cacheEntries = NULL;
    }
    LeaveCriticalSection(&m_csCache);
}

// IUnknown implementation
STDMETHODIMP_(ULONG) CShellBagCache::AddRef()
{
    return InterlockedIncrement(&m_cRefCount);
}

STDMETHODIMP_(ULONG) CShellBagCache::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRefCount);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP CShellBagCache::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_INVALIDARG;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown*)this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}
