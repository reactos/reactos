#include "init.h"
#include <emptyvc.h>
#include <regstr.h>
#include "general.h"
#include "dlg.h"
#include "emptyvol.h"
#include "parseinf.h"

#define MAX_DRIVES                 26   // there are 26 letters only

// {8369AB20-56C9-11d0-94E8-00AA0059CE02}
const CLSID CLSID_EmptyControlVolumeCache = {
                            0x8369ab20, 0x56c9, 0x11d0, 
                            0x94, 0xe8, 0x0, 0xaa, 0x0,
                            0x59, 0xce, 0x2};

/******************************************************************************
    class CEmptyControlVolumeCache
******************************************************************************/

class CEmptyControlVolumeCache : public IEmptyVolumeCache
{
public:
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEmptyVolumeCache Methods
    STDMETHODIMP Initialize(HKEY hRegKey, LPCWSTR pszVolume,
        LPWSTR *ppszDisplayName, LPWSTR *ppszDescription, DWORD *pdwFlags);
    STDMETHODIMP GetSpaceUsed(DWORDLONG *pdwSpaceUsed,
        IEmptyVolumeCacheCallBack *picb);
    STDMETHODIMP Purge(DWORDLONG dwSpaceToFree,
        IEmptyVolumeCacheCallBack *picb);
    STDMETHODIMP ShowProperties(HWND hwnd);
    STDMETHODIMP Deactivate(DWORD *pdwFlags);

// Attributes
public:
    static HRESULT IsControlExpired(HANDLE hControl, BOOL fUseCache = TRUE);

// Implementation
public:
    // Constructor and destructor
    CEmptyControlVolumeCache();
    virtual ~CEmptyControlVolumeCache();

protected:
	// implementation data helpers

    // Note. Write operations are only perfomed by the private functions
    //       prefixed cpl_XXX. Read access is not restricted.
    LPCACHE_PATH_NODE m_pPathsHead,
                      m_pPathsTail;

    // Note. Write operations are only perfomed by the private functions
    //       prefixed chl_XXX. Read access is not restricted.
    LPCONTROL_HANDLE_NODE m_pControlsHead,
                          m_pControlsTail;

    WCHAR     m_szVol[4];
    DWORDLONG m_dwTotalSize;
    ULONG     m_cRef;

	// implementation helper routines

    // cpl prefix stands for CachePathsList
    HRESULT cpl_Add(LPCTSTR pszCachePath);
    void    cpl_Remove();
    HRESULT cpl_CreateForVolume(LPCWSTR pszVolume = NULL);

    // chl prefix stands for ControlHandlesList
    HRESULT chl_Find(HANDLE hControl,
        LPCONTROL_HANDLE_NODE *rgp = NULL, UINT nSize = 1) const;
    HRESULT chl_Add(HANDLE hControl);
    void    chl_Remove(LPCONTROL_HANDLE_NODE rgp[2]);
    HRESULT chl_Remove(HANDLE hControl = NULL);
    HRESULT chl_CreateForPath(LPCTSTR pszCachePath,
        DWORDLONG *pdwUsedInFolder = NULL);

    friend HRESULT _stdcall EmptyControl_CreateInstance(IUnknown *pUnkOuter,
        REFIID riid, LPVOID* ppv);

//  friend BOOL CALLBACK EmptyControl_PropertiesDlgProc(HWND hDlg,
//      UINT msg, WPARAM wp, LPARAM lp);
};


STDAPI EmptyControl_CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID* ppv)
{
    *ppv = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    CEmptyControlVolumeCache *pCRC = new CEmptyControlVolumeCache;
    if (pCRC == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = pCRC->QueryInterface(riid, ppv);
    pCRC->Release();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CEmptyControlVolumeCache constructor and destructor

CEmptyControlVolumeCache::CEmptyControlVolumeCache()
{
    DllAddRef();

    m_pPathsHead = m_pPathsTail = NULL;
    m_pControlsHead = m_pControlsTail = NULL;

    m_szVol[0] = L'\0';
    m_dwTotalSize = 0;
    m_cRef = 1;
}

CEmptyControlVolumeCache::~CEmptyControlVolumeCache()
{
    ASSERT(m_cRef == 0);
    cpl_Remove();
    chl_Remove();

    DllRelease();
}


/////////////////////////////////////////////////////////////////////////////
// CEmptyControlVolumeCache attributes


// CEmptyControlVolumeCache::IsControlExpired
// Check if a control has not been accessed for more than N days. If there is
// no registry entry, default is DEFAULT_DAYS_BEFORE_EXPIRE.
//
// Parameters: fUseCache can be used to not go to the registry for the value
// of N above.
//
// Returns: either the Win32 error converted to HRESULT or
//          S_OK if control is expired and S_FALSE if not;
//
// Used by: only by CEmptyControlVolumeCache::chl_CreateForPath
//
HRESULT CEmptyControlVolumeCache::IsControlExpired(HANDLE hControl,
    BOOL fUseCache /*= TRUE*/)
{
    SYSTEMTIME    stNow;
    FILETIME      ftNow;
    FILETIME      ftLastAccess;
    LARGE_INTEGER timeExpire;
    HRESULT       hr = S_OK;

    ASSERT(hControl != NULL && hControl != INVALID_HANDLE_VALUE);

    // don't expire controls with uncertain access time.
    if (FAILED(GetLastAccessTime(hControl, &ftLastAccess)))
        return S_FALSE;
 
    //----- Time calculations (wierd looking) -----
    // Add to last access date the length of time before a control expires
    timeExpire.LowPart  = ftLastAccess.dwLowDateTime;
    timeExpire.HighPart = ftLastAccess.dwHighDateTime;
    timeExpire.QuadPart += (((CCacheItem*)hControl)->GetExpireDays() * 864000000000L); //24*3600*10^7

    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);

    return CompareFileTime((FILETIME*)&timeExpire, &ftNow) <= 0 ?
        S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CEmptyControlVolumeCache CachePathsList routines

// CEmptyControlVolumeCache::cpl_Add
// Check if a control has not been accessed for more than N days. If there is
// no registry entry, default is DEFAULT_DAYS_BEFORE_EXPIRE.
//
// Parameters: a cache folder path to add.
//
// Returns: E_OUTOFMEMORY or
//          S_FALSE if path is already in the list or S_OK if added.
//
// Used by: only by CEmptyControlVolumeCache::cpl_CreateForVolume
//
HRESULT CEmptyControlVolumeCache::cpl_Add(LPCTSTR pszCachePath)
{
    LPCACHE_PATH_NODE pNode;

    ASSERT(pszCachePath != NULL);

    for (pNode = m_pPathsHead; pNode != NULL; pNode = pNode->pNext)
        if (lstrcmpi(pNode->szCachePath, pszCachePath) == 0)
            break;
    if (pNode != NULL)
        return S_FALSE;

    pNode = new CACHE_PATH_NODE;
    if (pNode == NULL)
        return E_OUTOFMEMORY;

    lstrcpyn(pNode->szCachePath, pszCachePath, MAX_PATH);
    pNode->pNext = NULL;
    if (m_pPathsHead == NULL)
        m_pPathsHead = pNode;
    else
        m_pPathsTail->pNext = pNode;
    m_pPathsTail = pNode;

    return S_OK;
}

// CEmptyControlVolumeCache::cpl_Remove
// Remove all paths from the internal list.
//
// Parameters: none;
//
// Returns: void;
//
// Used by: several obvious places
//
void CEmptyControlVolumeCache::cpl_Remove()
{
    // remove cache path list
    for (LPCACHE_PATH_NODE pCur = m_pPathsHead;
         m_pPathsHead != NULL;
         pCur = m_pPathsHead) {

        m_pPathsHead = m_pPathsHead->pNext;
        delete[] pCur;
    }
    m_pPathsTail = NULL;
}

// CEmptyControlVolumeCache::cpl_CreateForVolume
// Build a list of paths to cache folders.
//
// Parameters: volume (or drive) where these folders are;
//
// Returns: S_OK or one out of the bunch of obvious errors;
//
// Used by: only by IEmptyVolumeCache::GetSpaceUsed
//
HRESULT CEmptyControlVolumeCache::cpl_CreateForVolume(LPCWSTR pszVolume)
{
    HKEY    hkey = NULL;
    HRESULT hr   = E_FAIL;
    int     iDriveNum;

    ASSERT(pszVolume != NULL);
    iDriveNum = PathGetDriveNumberW(pszVolume);
    if (iDriveNum < 0)
        return E_INVALIDARG;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_ACTIVEX_CACHE, 0,
            KEY_READ, &hkey) != ERROR_SUCCESS)
        return E_FAIL;

    TCHAR szCachePath[MAX_PATH],
          szValue[MAX_PATH];
    DWORD dwIndex    = 0,
          dwValueLen = MAX_PATH, dwLen = MAX_PATH;

    cpl_Remove();
    while (RegEnumValue(hkey, dwIndex++, szValue, &dwValueLen, NULL, NULL,
               (LPBYTE)szCachePath, &dwLen) == ERROR_SUCCESS) {
        dwLen = dwValueLen = MAX_PATH;

        if (PathGetDriveNumber(szCachePath) != iDriveNum)
            continue;

        // we must have added at least one successfully to get a success code..
        hr = cpl_Add(szCachePath);
        if (FAILED(hr))
            break;
    }
    RegCloseKey(hkey);

    if (FAILED(hr))
        cpl_Remove();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CEmptyControlVolumeCache ControlHandlesList routines

// CEmptyControlVolumeCache::chl_Find
// Find and return a location for the specified handle in the internal list.
// if (rgp == NULL), only result matters;
// if (rgp != NULL),
//     if (nSize == 1), *rgp is going to have found item (if it's there)
//     if (nSize >= 2), *rgp[0] = prev to the found item, and *rgp[1] is the
//                      item.
//
// Parameters: explained above;
//
// Returns: S_OK if the item is found, S_FALSE otherwise or
//          one out of the bunch of obvious errors;
//
// Used by: CEmptyControlVolumeCache::chl_Add and
//          CEmptyControlVolumeCache::chl_Remove
//
HRESULT CEmptyControlVolumeCache::chl_Find(HANDLE hControl,
    LPCONTROL_HANDLE_NODE *rgp /*= NULL*/, UINT nSize /*= 1*/) const
{
    LPCONTROL_HANDLE_NODE pCur,
                          pPrev = NULL;

    ASSERT(hControl != NULL && hControl != INVALID_HANDLE_VALUE);
    for (pCur = m_pControlsHead; pCur != NULL; pCur = pCur->pNext) {
        if (pCur->hControl == hControl)
            break;
        pPrev = pCur;
    }
    if (pCur == NULL)
        pPrev = NULL;                           // zero out possible return

    if (rgp != NULL && nSize > 0)
        if (nSize == 1)
            *rgp = pCur;
        else { /* if (nSize >= 2) */
            rgp[0] = pPrev;
            rgp[1] = pCur;
        }

    return (pCur != NULL) ? S_OK : E_FAIL;
}

HRESULT CEmptyControlVolumeCache::chl_Add(HANDLE hControl)
{
    LPCONTROL_HANDLE_NODE pNode;
    DWORD                 dwSize;

    // Note. Retail build assumes that handle is not in the list.
    ASSERT(hControl != NULL && hControl != INVALID_HANDLE_VALUE);
    ASSERT(FAILED(chl_Find(hControl)));

    pNode = new CONTROL_HANDLE_NODE;
    if (pNode == NULL)
        return E_OUTOFMEMORY;

    GetControlInfo(hControl, GCI_SIZESAVED, &dwSize, NULL, 0);

    pNode->hControl = hControl;
    pNode->pNext    = NULL;

    if (m_pControlsHead == NULL)
        m_pControlsHead = pNode;
    else {
        ASSERT(m_pControlsHead != NULL);
        m_pControlsTail->pNext = pNode;
    }
    m_pControlsTail = pNode;

    m_dwTotalSize += dwSize;
    return S_OK;
}

void CEmptyControlVolumeCache::chl_Remove(LPCONTROL_HANDLE_NODE rgp[2])
{
    DWORD dwSize;

    if (m_pControlsHead == NULL || (rgp[0] != NULL && rgp[1] == NULL))
        return;

    if (rgp[0] != NULL)
        rgp[0]->pNext = rgp[1]->pNext;
    else {
        rgp[1] = m_pControlsHead;
        m_pControlsHead = m_pControlsHead->pNext;
    }

    if (rgp[1] == m_pControlsTail)
        m_pControlsTail = rgp[0];

    GetControlInfo(rgp[1]->hControl, GCI_SIZESAVED, &dwSize, NULL, 0);

    // Note. This code assumes that the size of a control didn't change since
    //       it was added.
    m_dwTotalSize -= dwSize;
    ASSERT(m_dwTotalSize >= 0);

    ReleaseControlHandle(rgp[1]->hControl);
    delete rgp[1];
}

HRESULT CEmptyControlVolumeCache::chl_Remove(HANDLE hControl /*= NULL*/)
{
    LPCONTROL_HANDLE_NODE rgp[2] = { NULL, NULL };
    HRESULT hr;

    ASSERT(hControl != INVALID_HANDLE_VALUE);
    if (hControl != NULL) {
        hr = chl_Find(hControl, rgp, 2);
        if (FAILED(hr))
            return hr;

        chl_Remove(rgp);
        return S_OK;
    }

    while (m_pControlsHead != NULL)
        chl_Remove(rgp);

    ASSERT(m_pControlsHead == NULL && m_pControlsTail == NULL);
    return S_OK;
}

// CEmptyControlVolumeCache::chl_CreateForPath
// Calculate the size in bytes taken up by controls in the control cache
// folder specified.
//
// Parameters: pszCachePath is a path to the controls cache folder;
//             pdwSpaceUsed is the result
//
// Used by: only by IEmptyVolumeCache::GetSpaceUsed
//
HRESULT CEmptyControlVolumeCache::chl_CreateForPath(LPCTSTR pszCachePath,
    DWORDLONG *pdwUsedInFolder /*= NULL*/)
{
    DWORDLONG dwCopy;
    HANDLE    hFind    = NULL,
              hControl = NULL;
    LONG      lResult;
    BOOL      fCache   = FALSE;

    dwCopy = m_dwTotalSize;
    for (lResult = FindFirstControl(hFind, hControl, pszCachePath);
         lResult == ERROR_SUCCESS;
         lResult = FindNextControl(hFind, hControl)) {

        lResult = HRESULT_CODE(IsControlExpired(hControl, fCache));
        fCache  = TRUE;
        if (lResult != ERROR_SUCCESS)
            continue;

        lResult = HRESULT_CODE(chl_Add(hControl));
        if (lResult != ERROR_SUCCESS)
            break;
    }
    FindControlClose(hFind);

    if (lResult == ERROR_NO_MORE_ITEMS)
        lResult = ERROR_SUCCESS;

    if (pdwUsedInFolder != NULL) {
        *pdwUsedInFolder = m_dwTotalSize - dwCopy;
        ASSERT(*pdwUsedInFolder >= 0);
    }
    return HRESULT_FROM_WIN32(lResult);
}


/******************************************************************************
    IUnknown Methods
******************************************************************************/

STDMETHODIMP CEmptyControlVolumeCache::QueryInterface(REFIID iid, void** ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;

    if (iid != IID_IUnknown && iid != IID_IEmptyVolumeCache)
        return E_NOINTERFACE;

    *ppv = (void *)this;
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CEmptyControlVolumeCache::AddRef()
{
    return (++m_cRef);
}

STDMETHODIMP_(ULONG) CEmptyControlVolumeCache::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;   
}


/******************************************************************************
    IEmptyVolumeCache Methods
******************************************************************************/

STDMETHODIMP CEmptyControlVolumeCache::Initialize(HKEY hRegKey,
    LPCWSTR pszVolume, LPWSTR *ppszDisplayName, LPWSTR *ppszDescription,
    DWORD *pdwFlags)
{
    if (pszVolume == NULL)
        return E_POINTER;

    if (ppszDisplayName == NULL || ppszDescription == NULL)
        return E_POINTER;

    if (pdwFlags == NULL)
        return E_POINTER;

    StrCpyNW(m_szVol, pszVolume, ARRAYSIZE(m_szVol));
    cpl_Remove();
    chl_Remove();
    
    if (lstrlenW(m_szVol) == 0) {
        return E_UNEXPECTED;
    }

    if (FAILED(cpl_CreateForVolume(m_szVol))) {
        return E_FAIL;
    }

    *ppszDisplayName = *ppszDescription = NULL;
    *pdwFlags = EVCF_HASSETTINGS | EVCF_ENABLEBYDEFAULT |
        EVCF_ENABLEBYDEFAULT_AUTO;
    return S_OK;
}

STDMETHODIMP CEmptyControlVolumeCache::GetSpaceUsed(DWORDLONG *pdwSpaceUsed,
    IEmptyVolumeCacheCallBack *picb)
{
    LPCACHE_PATH_NODE pCur;
    HRESULT hr = S_OK;

    if (pdwSpaceUsed == NULL) {
        hr = E_POINTER;
        goto LastNotification;
    }
    *pdwSpaceUsed = 0;

    if (lstrlenW(m_szVol) == 0) {
        hr = E_UNEXPECTED;
        goto LastNotification;
    }

    for (pCur = m_pPathsHead; pCur != NULL; pCur = pCur->pNext) {
        DWORDLONG dwlThisItem = 0;
        if (FAILED(chl_CreateForPath(pCur->szCachePath, &dwlThisItem)))
            hr = S_FALSE;                       // at least one failed

        m_dwTotalSize += dwlThisItem;
        
        if (picb != NULL)
            picb->ScanProgress(m_dwTotalSize, 0, NULL);
    }
//  cpl_Remove();                               // because of ShowProperties

    *pdwSpaceUsed = m_dwTotalSize;

LastNotification:
    if (picb != NULL)
        picb->ScanProgress(m_dwTotalSize, EVCCBF_LASTNOTIFICATION, NULL);
    return hr;
}

STDMETHODIMP CEmptyControlVolumeCache::Purge(DWORDLONG dwSpaceToFree,
    IEmptyVolumeCacheCallBack *picb)
{
    LPCONTROL_HANDLE_NODE rgp[2] = { NULL, NULL };
    DWORDLONG dwSpaceFreed;
    HANDLE    hControl;
    DWORD     dwSize;
    HRESULT   hr;

    if (m_pControlsHead == NULL) {
        DWORDLONG dwSpaceUsed;

        hr = GetSpaceUsed(&dwSpaceUsed, picb);
        if (FAILED(hr) || m_pControlsHead == NULL)
            hr = FAILED(hr) ? hr : STG_E_NOMOREFILES;

        if (picb != NULL)
            picb->PurgeProgress(0, dwSpaceToFree, EVCCBF_LASTNOTIFICATION,
                NULL);

        return hr;
    }

    dwSpaceFreed = 0;
    ASSERT(m_pControlsHead != NULL);
    while (m_pControlsHead != NULL) {
        hControl = m_pControlsHead->hControl;
        ASSERT(hControl != NULL && hControl != INVALID_HANDLE_VALUE);

        GetControlInfo(hControl, GCI_SIZESAVED, &dwSize, NULL, 0);

        hr = RemoveControlByHandle2(hControl, FALSE, TRUE);
        if (SUCCEEDED(hr)) {
            dwSpaceFreed += dwSize;

            if (picb != NULL)
                picb->PurgeProgress(dwSpaceFreed, dwSpaceToFree, 0, NULL);
        }
        chl_Remove(rgp);

        if (dwSpaceFreed >= dwSpaceToFree)
            break;
    }

    if (picb != NULL)
        picb->PurgeProgress(dwSpaceFreed, dwSpaceToFree, 0, NULL);

    return S_OK;
}

// Note. This function opens the last cache folder in the internal list.
STDMETHODIMP CEmptyControlVolumeCache::ShowProperties(HWND hwnd)
{
    // Note. (According to SeanF) The codedownload engine will query
    //       ActiveXCache key under HKLM\SOFTWARE\Microsoft\Windows\
    //       CurrentVersion\Internet Settings. The value of this key should
    //       be equal to the last item in the CachePathsList which is why
    //       navigation below is done for the tail.
    if (m_pPathsTail == NULL || m_pPathsTail->szCachePath == NULL)
        return E_UNEXPECTED;

    ShellExecute(hwnd, NULL, m_pPathsTail->szCachePath, NULL, NULL, SW_SHOW);
    return S_OK;
/*
    int iDlgResult;

    iDlgResult = MLDialogBoxWrap(MLGetHinst(), MAKEINTRESOURCE(IDD_PROP_EXPIRE), hwnd,
        EmptyControl_PropertiesDlgProc);

    return iDlgResult == IDOK ? S_OK : S_FALSE;
*/
}

STDMETHODIMP CEmptyControlVolumeCache::Deactivate(DWORD *pdwFlags)
{
    if (pdwFlags == NULL)
        return E_INVALIDARG;
    *pdwFlags = 0;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

/*
static void msg_OnInitDialog(HWND hDlg);
static BOOL msg_OnCommand(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

static BOOL cmd_OnOK(HWND hDlg);

INT_PTR CALLBACK EmptyControl_PropertiesDlgProc(HWND hDlg,
    UINT msg, WPARAM wp, LPARAM lp)
{
    static MSD rgmsd[] = {
        { WM_INITDIALOG, ms_vh,    (PFN)msg_OnInitDialog },
        { WM_COMMAND,    ms_bwwwl, (PFN)msg_OnCommand    },
        { WM_NULL,       ms_end,   (PFN)NULL             }
    };

    return Dlg_MsgProc(rgmsd, hDlg, msg, wp, lp);
}

void msg_OnInitDialog(HWND hDlg)
{
    UINT nDays;

    CEmptyControlVolumeCache::GetDaysBeforeExpire(&nDays);
    SetDlgItemInt(hDlg, IDC_EDIT_EXPIRE, nDays, FALSE);
}

BOOL msg_OnCommand(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CMD rgcmd[] = {
        { IDOK, 0, ms_bh,  (PFN)cmd_OnOK },
        { 0,    0, ms_end, (PFN)NULL     }
    };

    return Msg_OnCmd(rgcmd, hDlg, msg, wp, lp);
}

BOOL cmd_OnOK(HWND hDlg)
{
    UINT nDays;
    BOOL fWorked;

    nDays = GetDlgItemInt(hDlg, IDC_EDIT_EXPIRE, &fWorked, FALSE);
    if (!fWorked) {
        MessageBeep(-1);
        SetFocus(GetDlgItem(hDlg, IDC_EDIT_EXPIRE));
        return FALSE;
    }

    CEmptyControlVolumeCache::SetDaysBeforeExpire(nDays);
    return TRUE;
}
*/
