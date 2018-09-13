#include "local.h"

#include "resource.h"
#include "hsfolder.h"
#include "cachesrch.h"
#include "shdguid.h"
#include "sfview.h"
#include <shlwapi.h>
#include <limits.h>

#include <mluisupp.h>

#define DM_HSFOLDER 0

#define DM_HISTVIEW    0x00000000
#define DM_CACHESEARCH 0x40000000

const TCHAR c_szRegKeyTopNSites[] = TEXT("HistoryTopNSitesView");
#define REGKEYTOPNSITESLEN (ARRAYSIZE(c_szRegKeyTopNSites) - 1)

const TCHAR c_szHistPrefix[] = TEXT("Visited: ");
#define HISTPREFIXLEN (ARRAYSIZE(c_szHistPrefix)-1)
const TCHAR c_szHostPrefix[] = TEXT(":Host: ");
#define HOSTPREFIXLEN (ARRAYSIZE(c_szHostPrefix)-1)
const CHAR c_szIntervalPrefix[] = "MSHist";
#define INTERVALPREFIXLEN (ARRAYSIZE(c_szIntervalPrefix)-1)
const TCHAR c_szTextHeader[] = TEXT("Content-type: text/");
#define TEXTHEADERLEN (ARRAYSIZE(c_szTextHeader) - 1)
const TCHAR c_szHTML[] = TEXT("html");
#define HTMLLEN (ARRAYSIZE(c_szHTML) - 1)
#define TYPICAL_INTERVALS (4+7)

// these are common flags to ShChangeNotify
#ifndef UNIX
#define CHANGE_FLAGS (0)
#else
#define CHANGE_FLAGS SHCNF_FLUSH
#endif


#define ALL_CHANGES (SHCNE_DELETE|SHCNE_MKDIR|SHCNE_RMDIR|SHCNE_CREATE|SHCNE_UPDATEDIR)

#define FORMAT_PARAMS (FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_MAX_WIDTH_MASK)

// this functions needs to stay here since it has cpp stuff
LPCEIPIDL _CreateCacheFolderPidl(BOOL fOleAlloc, DWORD dwSize, USHORT usSign);
LPCEIPIDL _CreateIdCacheFolderPidl(BOOL fOleAlloc, USHORT usSign, LPCTSTR szId);
LPCEIPIDL _CreateBuffCacheFolderPidl(BOOL fOleAlloc, DWORD dwSize, LPINTERNET_CACHE_ENTRY_INFO pcei);
LPHEIPIDL _CreateHCacheFolderPidl(BOOL fOleMalloc, LPCTSTR pszUrl, FILETIME ftModified, LPSTATURL lpStatURL,
                                  __int64 llPriority = 0, DWORD dwNumHits = 0);
DWORD     _DaysInInterval(HSFINTERVAL *pInterval);
void      _KeyForInterval(HSFINTERVAL *pInterval, LPTSTR pszInterval, int cchInterval);
void      _FileTimeDeltaDays(FILETIME *pftBase, FILETIME *pftNew, int Days);

void      ResizeStatusBar(HWND hwnd, BOOL fInit);



//  BEGIN OF JCORDELL CODE
#define QUANTA_IN_A_SECOND  10000000
#define SECONDS_IN_A_DAY    60 * 60 * 24
#define QUANTA_IN_A_DAY     ((__int64) QUANTA_IN_A_SECOND * SECONDS_IN_A_DAY)
#define INT64_VALUE(pFT)    ((((__int64)(pFT)->dwHighDateTime) << 32) + (__int64) (pFT)->dwLowDateTime)
#define DAYS_DIFF(s,e)      ((int) (( INT64_VALUE(s) - INT64_VALUE(e) ) / QUANTA_IN_A_DAY))

BOOL      GetDisplayNameForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                         TCHAR *szBuffer, int cbBufferLength );
BOOL      GetTooltipForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                     TCHAR *szBuffer, int cbBufferLength );
//  END OF JCORDELL CODE




class CHistFolder : public CHistCacheFolder
{
public:
    CHistFolder(FOLDER_TYPE FolderType) : CHistCacheFolder(FolderType) {}
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);

protected:
    STDMETHODIMP _GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr);
};

class CCacheFolder : public CHistCacheFolder
{
public:
    CCacheFolder(FOLDER_TYPE FolderType) : CHistCacheFolder(FolderType) {}
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);

protected:
    STDMETHODIMP _GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr);
};

class CDetailsOfFolder : public IShellDetails
{
public:
    CDetailsOfFolder(HWND hwnd, IShellFolder2 *psf) : _cRef(1), _psf(psf), _hwnd(hwnd)
    {
        _psf->AddRef();
    }

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IShellDetails
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi);
    STDMETHOD(ColumnClick)(UINT iColumn);

private:
    virtual ~CDetailsOfFolder() { _psf->Release(); }

    LONG _cRef;
    IShellFolder2 *_psf;
    HWND _hwnd;
};

STDMETHODIMP CDetailsOfFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDetailsOfFolder, IShellDetails),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDetailsOfFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDetailsOfFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDetailsOfFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    return _psf->GetDetailsOf(pidl, iColumn, pdi);
}

HRESULT CDetailsOfFolder::ColumnClick(UINT iColumn)
{
    ShellFolderView_ReArrange(_hwnd, iColumn);
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheFolderView Functions and Definitions
//
//////////////////////////////////////////////////////////////////////////////


////////////////////////
//
// Column definition for the Cache Folder DefView
//
enum {
    ICOLC_URL_SHORTNAME = 0,
    ICOLC_URL_NAME,
    ICOLC_URL_TYPE,
    ICOLC_URL_SIZE,
    ICOLC_URL_EXPIRES,
    ICOLC_URL_MODIFIED,
    ICOLC_URL_ACCESSED,
    ICOLC_URL_LASTSYNCED,
    ICOLC_URL_MAX         // Make sure this is the last enum item
};


typedef struct _COLSPEC
{
    short int iCol;
    short int ids;        // Id of string for title
    short int cchCol;     // Number of characters wide to make column
    short int iFmt;       // The format of the column;
} COLSPEC;

const COLSPEC s_CacheFolder_cols[] = {
    {ICOLC_URL_SHORTNAME,  IDS_SHORTNAME_COL,  18, LVCFMT_LEFT},
    {ICOLC_URL_NAME,       IDS_NAME_COL,       30, LVCFMT_LEFT},
    {ICOLC_URL_TYPE,       IDS_TYPE_COL,       15, LVCFMT_LEFT},
    {ICOLC_URL_SIZE,       IDS_SIZE_COL,        8, LVCFMT_RIGHT},
    {ICOLC_URL_EXPIRES,    IDS_EXPIRES_COL,    18, LVCFMT_LEFT},
    {ICOLC_URL_MODIFIED,   IDS_MODIFIED_COL,   18, LVCFMT_LEFT},
    {ICOLC_URL_ACCESSED,   IDS_ACCESSED_COL,   18, LVCFMT_LEFT},
    {ICOLC_URL_LASTSYNCED, IDS_LASTSYNCED_COL, 18, LVCFMT_LEFT}
};

const COLSPEC s_HistIntervalFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_TIMEPERIOD_COL,           30, LVCFMT_LEFT},
};

const COLSPEC s_HistHostFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_HOSTNAME_COL,           30, LVCFMT_LEFT},
};

const COLSPEC s_HistFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_NAME_COL,           30, LVCFMT_LEFT},
    {ICOLH_URL_TITLE,         IDS_TITLE_COL,          30, LVCFMT_LEFT},
    {ICOLH_URL_LASTVISITED,   IDS_LASTVISITED_COL,    18, LVCFMT_LEFT},
};

//////////////////////////////////////////////////////////////////////
#ifdef DEBUG
void DumpPidl(LPCITEMIDLIST pidl, LPTSTR opt) {
    if (!DM_HISTVIEW)
        return;
    TraceMsg(DM_HISTVIEW, "=========pidl dump: %s", opt);
    LPCITEMIDLIST pidlTemp = pidl;
    while(pidlTemp->mkid.cb) {
        DWORD cch = (pidlTemp->mkid.cb * 6) + 25;
        LPSTR ostr = (LPSTR)LocalAlloc(LPTR, cch);
        if (ostr != NULL)
        {
            wnsprintfA(ostr, cch, "  [%02X] ", pidlTemp->mkid.cb);
            UINT ostrOffset = lstrlenA(ostr);
            int i;
            for (i = 2; i <= pidlTemp->mkid.cb; ++i) {
                wnsprintfA(ostr + ostrOffset, 4, "  %c",
                          (((*((char*)pidlTemp + i)) >= 20) ?
                           ((*(((char*)pidlTemp) + i))) : '.'));
                ostrOffset += 3;
            }
            wnsprintfA(ostr + ostrOffset, 17, "\n               ");
            ostrOffset += 16;
            for (i = 2; i <= pidlTemp->mkid.cb; ++i) {
                wnsprintfA(ostr + ostrOffset, cch - ostrOffset, "%02X ", *(((char*)pidlTemp) + i));
                ostrOffset += 3;
            }
            //        TraceMsg(DM_HISTVIEW, "   [%d] %s", pidlTemp->mkid.cb, ((LPSTR)pidlTemp + 1));
            pidlTemp = _ILNext(pidlTemp);
            OutputDebugStringA(ostr);
            OutputDebugStringA("\n");
            //TraceMsg(DM_HISTVIEW, "\n%s", ostr);
            LocalFree(ostr);
        }
    }
}
#else
#define DumpPidl(x,y)
#endif


HRESULT CreateSpecialViewPidl(USHORT usViewType, LPITEMIDLIST* ppidlOut, UINT cbExtra = 0, LPBYTE *ppbExtra = NULL);

HRESULT ConvertStandardHistPidlToSpecialViewPidl(LPCITEMIDLIST pidlStandardHist,
                                                 USHORT        usViewType,
                                                 LPITEMIDLIST *ppidlOut);

UINT MergeMenuHierarchy(HMENU hmenuDst, HMENU hmenuSrc, UINT idcMin, UINT idcMax)
{
    UINT idcMaxUsed = idcMin;
    int imi = GetMenuItemCount(hmenuSrc);
    while (--imi >= 0)
    {
        MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_SUBMENU, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0 };

        if (GetMenuItemInfo(hmenuSrc, imi, TRUE, &mii))
        {
            UINT idcT = Shell_MergeMenus(GetMenuFromID(hmenuDst, mii.wID),
                    mii.hSubMenu, 0, idcMin, idcMax, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            idcMaxUsed = max(idcMaxUsed, idcT);
        }
    }
    return idcMaxUsed;
}

HRESULT HistCacheFolderView_MergeMenu(UINT idMenu, LPQCMINFO pqcm)
{
    HMENU hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(idMenu));
    if (hmenu)
    {
        MergeMenuHierarchy(pqcm->hmenu, hmenu, pqcm->idCmdFirst, pqcm->idCmdLast);

        DestroyMenu(hmenu);
    }
    return NOERROR;
}


HRESULT HistCacheFolderView_Command(HWND hwnd, UINT uID, FOLDER_TYPE FolderType)
{
    if ((!IsLeaf(FolderType)) && uID != IDM_SORTBYTITLE)
        return E_FAIL;

    switch (uID) {
    case IDM_SORTBYTITLE:
    case IDM_SORTBYADDRESS:
    case IDM_SORTBYVISITED:
    case IDM_SORTBYUPDATED:
        ShellFolderView_ReArrange(hwnd, uID - IDM_SORTBYTITLE);
        break;
    case IDM_SORTBYNAME:
    case IDM_SORTBYADDRESS2:
    case IDM_SORTBYSIZE:
    case IDM_SORTBYEXPIRES2:
    case IDM_SORTBYMODIFIED:
    case IDM_SORTBYACCESSED:
    case IDM_SORTBYCHECKED:
        ShellFolderView_ReArrange(hwnd, uID - IDM_SORTBYNAME);
        break;
    default:
        return E_FAIL;
    }
    return NOERROR;
}

HRESULT HistCacheFolderView_InitMenuPopup(HWND hwnd, UINT idCmdFirst, int nIndex, HMENU hmenu)
{
    return NOERROR;
}

HRESULT HistCacheFolderView_OnColumnClick(HWND hwnd, UINT iColumn)
{
    ShellFolderView_ReArrange(hwnd, iColumn);
    return NOERROR;
}

HRESULT HistCacheFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect)
{
    if (dwEffect & DROPEFFECT_MOVE)
    {
        CHistCacheItem *pHCItem;
        BOOL fBulkDelete;

        if (SUCCEEDED(pdo->QueryInterface(IID_IHistCache, (void **)&pHCItem)))
        {
            fBulkDelete = pHCItem->_cItems > LOTS_OF_FILES;
            for (UINT i = 0; i < pHCItem->_cItems; i++)
            {
                if (DeleteUrlCacheEntry(HCPidlToSourceUrl((LPCITEMIDLIST)pHCItem->_ppcei[i])))
                {
                    if (!fBulkDelete)
                    {
                        _GenerateEvent(SHCNE_DELETE, pHCItem->_pHCFolder->_pidl, (LPITEMIDLIST)(pHCItem->_ppcei[i]), NULL);
                    }
                }
            }
            if (fBulkDelete)
            {
                _GenerateEvent(SHCNE_UPDATEDIR, pHCItem->_pHCFolder->_pidl, NULL, NULL);
            }
            SHChangeNotifyHandleEvents();
            pHCItem->Release();
            return S_OK;
        }
    }
    return E_FAIL;
}

//BUGBUG: There are copies of exactly this function in SHELL32
// Add the File Type page
HRESULT HistCacheFolderView_OnAddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA * ppagedata)
{
    IShellPropSheetExt * pspse;
    HRESULT hres = CoCreateInstance(CLSID_FileTypes, NULL, CLSCTX_INPROC_SERVER,
                              IID_IShellPropSheetExt, (void **)&pspse);
    if (SUCCEEDED(hres))
    {
        hres = pspse->AddPages(ppagedata->pfn, ppagedata->lParam);
        pspse->Release();
    }
    return hres;
}

HRESULT HistCacheFolderView_OnGetSortDefaults(FOLDER_TYPE FolderType, int * piDirection, int * plParamSort)
{
    *plParamSort = IsHistory(FolderType) ? (int)ICOLH_URL_LASTVISITED : (int)ICOLC_URL_ACCESSED;
    *piDirection = 1;
    return S_OK;
}

//#define ZONES_PANE_WIDTH 120

HRESULT CALLBACK HistCacheFolderView_ViewCallback(
     IShellView *psvOuter,
     IShellFolder *psf,
     HWND hwnd,
     UINT uMsg,
     WPARAM wParam,
     LPARAM lParam,
     FOLDER_TYPE FolderType
     )
{
    HRESULT hres = NOERROR;

    switch (uMsg)
    {
    case DVM_GETHELPTEXT:
    {
        TCHAR szText[MAX_PATH];

        UINT id = LOWORD(wParam);
        UINT cchBuf = HIWORD(wParam);
        LPTSTR pszBuf = (LPTSTR)lParam;

        MLLoadString(id+IDS_MH_FIRST, szText, ARRAYSIZE(szText)-1);

        // we know for a fact that this parameter is really a TCHAR
        if ( IsOS( OS_NT ))
        {
            SHTCharToUnicode( szText, (LPWSTR) pszBuf, cchBuf );
        }
        else
        {
            SHTCharToAnsi( szText, (LPSTR) pszBuf, cchBuf );
        }
        break;
    }

    case SFVM_GETNOTIFY:
    {
        CHistCacheFolder *pHCFolder = NULL;
        LPCITEMIDLIST pidl;

        hres = psf->QueryInterface(CLSID_HistFolder, (void **) &pHCFolder);
        if (SUCCEEDED(hres))
        {
            pidl = pHCFolder->_pidl;
            pHCFolder->Release();
        }
        ASSERT(pidl);
        *(LPCITEMIDLIST*)wParam = pidl;
        *(LONG*)lParam = IsHistory(FolderType) ? ALL_CHANGES: SHCNE_DELETE|SHCNE_UPDATEDIR;
    }
        break;

    case DVM_DIDDRAGDROP:
        hres = HistCacheFolderView_DidDragDrop((IDataObject *)lParam, (DWORD)wParam);
        break;

    case DVM_INITMENUPOPUP:
        hres = HistCacheFolderView_InitMenuPopup(hwnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;

    case DVM_INVOKECOMMAND:
        HistCacheFolderView_Command(hwnd, (UINT)wParam, FolderType);
        break;

    case DVM_COLUMNCLICK:
        hres = HistCacheFolderView_OnColumnClick(hwnd, (UINT)wParam);
        break;

    case DVM_MERGEMENU:
        hres = HistCacheFolderView_MergeMenu(IsHistory(FolderType) ? MENU_HISTORY : MENU_CACHE, (LPQCMINFO)lParam);
        break;

    case DVM_DEFVIEWMODE:
        *(FOLDERVIEWMODE *)lParam = FVM_DETAILS;
        break;

    case SFVM_ADDPROPERTYPAGES:
        hres = HistCacheFolderView_OnAddPropertyPages((DWORD)wParam, (SFVM_PROPPAGE_DATA *)lParam);
        break;

    case SFVM_GETSORTDEFAULTS:
        hres = HistCacheFolderView_OnGetSortDefaults(FolderType, (int *)wParam, (int *)lParam);
        break;

    case SFVM_UPDATESTATUSBAR:
        ResizeStatusBar(hwnd, FALSE);
        // We did not set any text; let defview do it
        hres = E_NOTIMPL;
        break;

    case SFVM_SIZE:
        ResizeStatusBar(hwnd, FALSE);
        break;

    case SFVM_GETPANE:
        if (wParam == PANE_ZONE)
            *(DWORD*)lParam = 1;
        else
            *(DWORD*)lParam = PANE_NONE;

        break;
    case SFVM_WINDOWCREATED:
        ResizeStatusBar(hwnd, TRUE);
        break;

    case SFVM_GETZONE:
        *(DWORD*)lParam = IsHistory(FolderType) ? URLZONE_LOCAL_MACHINE : URLZONE_INTERNET; // Internet by default
        break;

    default:
        hres = E_FAIL;
    }

    return hres;
}

HRESULT CALLBACK CacheFolderView_ViewCallback(IShellView *psvOuter, IShellFolder *psf,
                                              HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HistCacheFolderView_ViewCallback(psvOuter, psf, hwnd, uMsg, wParam, lParam, FOLDER_TYPE_Cache);
}

HRESULT CALLBACK HistFolderView_ViewCallback(IShellView *psvOuter, IShellFolder *psf,
                                             HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HistCacheFolderView_ViewCallback(psvOuter, psf, hwnd, uMsg, wParam, lParam, FOLDER_TYPE_Hist);
}

HRESULT CALLBACK IntervalFolderView_ViewCallback(IShellView *psvOuter, IShellFolder *psf,
                                             HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HistCacheFolderView_ViewCallback(psvOuter, psf, hwnd, uMsg, wParam, lParam, FOLDER_TYPE_HistInterval);
}

HRESULT CALLBACK DomainFolderView_ViewCallback(IShellView *psvOuter, IShellFolder *psf,
                                             HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HistCacheFolderView_ViewCallback(psvOuter, psf, hwnd, uMsg, wParam, lParam, FOLDER_TYPE_HistDomain);
}

HRESULT HistCacheFolderView_CreateInstance(CHistCacheFolder *pHCFolder, LPCITEMIDLIST pidl, void **ppv)
{
    CSFV csfv;

    csfv.cbSize = sizeof(csfv);
    csfv.pshf = (IShellFolder *)pHCFolder;
    csfv.psvOuter = NULL;
    csfv.pidl = pidl;
    csfv.lEvents = SHCNE_DELETE; // SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_GLOBALEVENTS;
    switch (pHCFolder->_foldertype)
    {
        case FOLDER_TYPE_Cache:
            csfv.pfnCallback = CacheFolderView_ViewCallback;
            break;
        case FOLDER_TYPE_Hist:
            csfv.pfnCallback = HistFolderView_ViewCallback;
            break;

        case FOLDER_TYPE_HistInterval:
            csfv.pfnCallback = IntervalFolderView_ViewCallback;
            break;

        case FOLDER_TYPE_HistDomain:
            csfv.pfnCallback = DomainFolderView_ViewCallback;
            break;

        default:
            return E_FAIL;
    }
    csfv.fvm = (FOLDERVIEWMODE)0;         // Have defview restore the folder view mode

    return SHCreateShellFolderViewEx(&csfv, (IShellView**)ppv); // &this->psv);
}



//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////

CHistCacheFolderEnum::CHistCacheFolderEnum(DWORD grfFlags, CHistCacheFolder *pHCFolder)
{
    TraceMsg(DM_HSFOLDER, "hcfe - CHistCacheFolderEnum() called");
    _cRef = 1;
    DllAddRef();

    _grfFlags = grfFlags,
    _pHCFolder = pHCFolder;
    pHCFolder->AddRef();
    ASSERT(_hEnum             == NULL &&
           _cbCurrentInterval == 0    &&
           _cbIntervals       == 0    &&
           _pshHashTable      == NULL &&
           _polFrequentPages  == NULL &&
           _pIntervalCache    == NULL);

}

CHistCacheFolderEnum::~CHistCacheFolderEnum()
{
    ASSERT(_cRef == 0);         // we should always have a zero ref count here
    TraceMsg(DM_HSFOLDER, "hcfe - ~CHistCacheFolderEnum() called.");
    _pHCFolder->Release();
    if (_pceiWorking)
    {
        LocalFree(_pceiWorking);
    }
    if (_pIntervalCache)
    {
        LocalFree(_pIntervalCache);
    }

    if (_hEnum)
    {
        FindCloseUrlCache(_hEnum);
        _hEnum = NULL;
    }
    if (_pshHashTable)
        delete _pshHashTable;
    if (_polFrequentPages)
        delete _polFrequentPages;
    if (_pstatenum)
        _pstatenum->Release();
    DllRelease();
}


HRESULT CHistCacheFolderEnum_CreateInstance(DWORD grfFlags, CHistCacheFolder *pHCFolder, IEnumIDList **ppeidl)
{
    TraceMsg(DM_HSFOLDER, "hcfe - CreateInstance() called.");

    *ppeidl = NULL;                 // null the out param

    CHistCacheFolderEnum *pHCFE = new CHistCacheFolderEnum(grfFlags, pHCFolder);
    if (!pHCFE)
        return E_OUTOFMEMORY;

    *ppeidl = pHCFE;

    return S_OK;
}

HRESULT CHistCacheFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CHistCacheFolderEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CHistCacheFolderEnum::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistCacheFolderEnum::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistCacheFolderEnum::_NextHistInterval(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hres = S_OK;
    LPCEIPIDL pcei = NULL;
    TCHAR szCurrentInterval[INTERVAL_SIZE+1];

    //  BUGBUG chrisfra 3/27/97 on NT cache files are per user, not so on win95.  how do
    //  we manage containers on win95 if different users are specified different history
    //  intervals

    if (0 == _cbCurrentInterval)
    {
        hres = _pHCFolder->_ValidateIntervalCache();
        if (SUCCEEDED(hres))
        {
            hres = S_OK;
            ENTERCRITICAL;
            if (_pIntervalCache)
            {
                LocalFree(_pIntervalCache);
                _pIntervalCache = NULL;
            }
            if (_pHCFolder->_pIntervalCache)
            {
                _pIntervalCache = (HSFINTERVAL *)LocalAlloc(LPTR,
                                                            _pHCFolder->_cbIntervals*sizeof(HSFINTERVAL));
                if (_pIntervalCache == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                else
                {
                    _cbIntervals = _pHCFolder->_cbIntervals;
                    CopyMemory(_pIntervalCache,
                               _pHCFolder->_pIntervalCache,
                               _cbIntervals*sizeof(HSFINTERVAL));
                }
            }
            LEAVECRITICAL;
        }
    }

    if (_pIntervalCache && _cbCurrentInterval < _cbIntervals)
    {
        _KeyForInterval(&_pIntervalCache[_cbCurrentInterval], szCurrentInterval,
                        ARRAYSIZE(szCurrentInterval));
        pcei = _CreateIdCacheFolderPidl(TRUE,
                                        _pIntervalCache[_cbCurrentInterval].usSign,
                                        szCurrentInterval);
        _cbCurrentInterval++;
    }
    if (pcei)
    {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched) *pceltFetched = 1;
    }
    else
    {
        if (pceltFetched) *pceltFetched = 0;
        rgelt[0] = NULL;
        hres = S_FALSE;
    }
    return hres;
}

// This function dispatches the different "views" on History that are possible
HRESULT CHistCacheFolderEnum::_NextViewPart(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    switch(_pHCFolder->_uViewType) {
    case VIEWPIDL_SEARCH:
        return _NextViewPart_OrderSearch(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_TODAY:
        return _NextViewPart_OrderToday(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_SITE:
        return _NextViewPart_OrderSite(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_FREQ:
        return _NextViewPart_OrderFreq(celt, rgelt, pceltFetched);
    default:
        return E_NOTIMPL;
    }
}

LPITEMIDLIST _Combine_ViewPidl(USHORT usViewType, LPITEMIDLIST pidl);

// This function wraps wininet's Find(First/Next)UrlCacheEntry API
// returns DWERROR code or zero if successful
DWORD _FindURLCacheEntry(IN LPCTSTR                          pszCachePrefix,
                         IN OUT LPINTERNET_CACHE_ENTRY_INFO  pcei,
                         IN OUT HANDLE                      &hEnum,
                         IN OUT LPDWORD                      pdwBuffSize)
{
    if (!hEnum)
    {
        if (! (hEnum = FindFirstUrlCacheEntry(pszCachePrefix, pcei, pdwBuffSize)) )
            return GetLastError();
    }
    else if (!FindNextUrlCacheEntry(hEnum, pcei, pdwBuffSize))
        return GetLastError();
    return NOERROR;
}

// Thie function provides an iterator over all entries in all (MSHIST-type) buckets
//   in the cache
DWORD _FindURLFlatCacheEntry(
                             IN HSFINTERVAL *pIntervalCache,
                             IN LPTSTR       pszUserName,       // filter out cache entries owned by user
                             IN BOOL         fHostEntry,        // retrieve host entries only (FALSE), or no host entries (TRUE)
                             IN OUT int     &cbCurrentInterval, // should begin at the maximum number of intervals
                             IN OUT LPINTERNET_CACHE_ENTRY_INFO  pcei,
                             IN OUT HANDLE  &hEnum,
                             IN OUT LPDWORD  pdwBuffSize
                             )
{
    DWORD dwStoreBuffSize = *pdwBuffSize;
    DWORD dwResult        = ERROR_NO_MORE_ITEMS;
    while (cbCurrentInterval >= 0) {
        if ((dwResult = _FindURLCacheEntry(pIntervalCache[cbCurrentInterval].szPrefix,
                                           pcei, hEnum, pdwBuffSize)) != NOERROR)
        {
            if (dwResult == ERROR_NO_MORE_ITEMS) {
                // This bucket is done, now go get the next one
                FindCloseUrlCache(hEnum);
                hEnum = NULL;
                --cbCurrentInterval;
            }
            else
                break;
        }
        else
        {
            // Do requested filtering...
            BOOL fIsHost = (StrStr(pcei->lpszSourceUrlName, c_szHostPrefix) == NULL);
            if ( ((!pszUserName) ||  // if requested, filter username
                  _FilterUserName(pcei, pIntervalCache[cbCurrentInterval].szPrefix, pszUserName)) &&
                 ((!fHostEntry && !fIsHost) ||  // filter for host entries
                  (fHostEntry  && fIsHost))    )
            {
                break;
            }
        }
        // reset for next iteration
        *pdwBuffSize = dwStoreBuffSize;
    }
    return dwResult;
}

// This guy will search the flat cache (MSHist buckets) for a particular URL
//  * This function assumes that the Interval cache is good and loaded
// RETURNS: Windows Error code
DWORD CHistCacheFolder::_SearchFlatCacheForUrl(LPCTSTR pszUrl, LPINTERNET_CACHE_ENTRY_INFO pcei, LPDWORD pdwBuffSize)
{
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD dwUserNameLen = ARRAYSIZE(szUserName);

    if (FAILED(_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    UINT   uSuffixLen     = lstrlen(pszUrl) + lstrlen(szUserName) + 1; // extra 1 for '@'
    LPTSTR pszPrefixedUrl = ((LPTSTR)LocalAlloc(LPTR, (PREFIX_SIZE + uSuffixLen + 1) * sizeof(TCHAR)));
    DWORD  dwError        = ERROR_FILE_NOT_FOUND;

    if (pszPrefixedUrl != NULL)
    {
        // pszPrefixedUrl will have the format of "PREFIX username@
        wnsprintf(pszPrefixedUrl + PREFIX_SIZE, uSuffixLen + 1, TEXT("%s@%s"), szUserName, pszUrl);

        for (int i =_cbIntervals - 1; i >= 0; --i) {
            // memcpy doesn't null terminate
            memcpy(pszPrefixedUrl, _pIntervalCache[i].szPrefix, PREFIX_SIZE * sizeof(TCHAR));
            if (GetUrlCacheEntryInfo(pszPrefixedUrl, pcei, pdwBuffSize)) {
                dwError = ERROR_SUCCESS;
                break;
            }
            else if ( ((dwError = GetLastError()) != ERROR_FILE_NOT_FOUND) ) {
                break;
            }
        }
        LocalFree(pszPrefixedUrl);
    }
    else
    {
        // BUGBUG return an error indicated out of memory
    }
    
    return dwError;
}

//////////////////////////////////////////////////////////////////////
//  Most Frequently Visited Sites;

// this structure is used by the enumeration of the cache
//   to get the most frequently seen sites
class OrderList_CacheElement : public OrderedList::Element {
public:
    LPTSTR    pszUrl;
    DWORD     dwHitRate;
    __int64   llPriority;
    int       nDaysSinceLastHit;
    LPSTATURL lpSTATURL;

    static   FILETIME ftToday;
    static   BOOL     fInited;

    OrderList_CacheElement(LPTSTR pszStr, DWORD dwHR, LPSTATURL lpSU) {
        s_initToday();
        ASSERT(pszStr);
        pszUrl         = (pszStr ? StrDup(pszStr) : StrDup(TEXT("")));
        dwHitRate      = dwHR;
        lpSTATURL      = lpSU;

        nDaysSinceLastHit = DAYS_DIFF(&ftToday, &(lpSTATURL->ftLastVisited));

        // prevent division by zero
        if (nDaysSinceLastHit < 0)
            nDaysSinceLastHit = 0;
        // scale division up by a little less than half of the __int64
        llPriority  = ((((__int64)dwHitRate) * LONG_MAX) /
                       ((__int64)(nDaysSinceLastHit + 1)));
        //dPriority  = ((double)dwHitRate / (double)(nDaysSinceLastHit + 1));
    }

    virtual int compareWith(OrderedList::Element *pelt) {
        OrderList_CacheElement *polce;
        if (pelt) {
            polce = reinterpret_cast<OrderList_CacheElement *>(pelt);
            // we're cheating here a bit by returning 1 instead of testing
            //   for equality, but that's ok...
            //            return ( (dwHitRate < polce->dwHitRate) ? -1 : 1 );
            return ( (llPriority < polce->llPriority) ? -1 : 1 );
        }
        DebugBreak();
        return 0;
    }

    virtual ~OrderList_CacheElement() {
        if (pszUrl)    LocalFree(pszUrl);
        if (lpSTATURL) {
            if (lpSTATURL->pwcsUrl)
                OleFree(lpSTATURL->pwcsUrl);
            if (lpSTATURL->pwcsTitle)
                OleFree(lpSTATURL->pwcsTitle);
            delete lpSTATURL;
        }
    }

    /*
    friend ostream& operator<<(ostream& os, OrderList_CacheElement& olce) {
        os << " (" << olce.dwHitRate << "; " << olce.nDaysSinceLastHit
           << " days; pri=" << olce.llPriority << ") " << olce.pszUrl;
        return os;
    }
    */

    static void s_initToday() {
        if (!fInited) {
            SYSTEMTIME sysTime;
            GetLocalTime(&sysTime);
            SystemTimeToFileTime(&sysTime, &ftToday);
            fInited = TRUE;
        }
    }
};

FILETIME OrderList_CacheElement::ftToday;
BOOL OrderList_CacheElement::fInited = FALSE;

// caller must delete OrderedList
OrderedList* CHistCacheFolderEnum::_GetMostFrequentPages() {
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;
    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT       uUserNameLen = lstrlen(szUserName);

    // reinit the current time
    OrderList_CacheElement::fInited = FALSE;
    IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
    OrderedList     *pol         = NULL;

    if (pUrlHistStg)
    {
        IEnumSTATURL *penum = NULL;
        if (SUCCEEDED(pUrlHistStg->EnumUrls(&penum)) && penum)
        {
            DWORD dwSites = -1;
            DWORD dwType  = REG_DWORD;
            DWORD dwSize  = sizeof(DWORD);

            EVAL(SHRegGetUSValue(REGSTR_PATH_MAIN, c_szRegKeyTopNSites, &dwType,
                                 (LPVOID)&dwSites, &dwSize, FALSE,
                                 (LPVOID)&dwSites, dwSize) == ERROR_SUCCESS);

            if ( (dwType != REG_DWORD)     ||
                 (dwSize != sizeof(DWORD)) ||
                 (dwSites < 0) )
            {
                dwSites = NUM_TOP_SITES;
                SHRegSetUSValue(REGSTR_PATH_MAIN, c_szRegKeyTopNSites, REG_DWORD,
                                (LPVOID)&dwSites, dwSize, SHREGSET_HKCU);

                dwSites = NUM_TOP_SITES;
            }

            pol = new OrderedList(dwSites);
            if (pol)
            {
                STATURL *psuThis = new STATURL;
                if (psuThis)
                {
                    penum->SetFilter(NULL, STATURL_QUERYFLAG_TOPLEVEL);

                    while (pol) {
                        psuThis->cbSize    = sizeof(STATURL);
                        psuThis->pwcsUrl   = NULL;
                        psuThis->pwcsTitle = NULL;

                        ULONG   cFetched;

                        if (SUCCEEDED(penum->Next(1, psuThis, &cFetched)) && cFetched)
                        {
                            // test: the url (taken from the VISITED history bucket) is a "top-level"
                            //  url that would be in the MSHIST (displayed to user) history bucket
                            //  things ommitted will be certain error urls and frame children pages etc...
                            if ( (psuThis->dwFlags & STATURLFLAG_ISTOPLEVEL) &&
                                 (psuThis->pwcsUrl)                          &&
                                 (!IsErrorUrl(psuThis->pwcsUrl)) )
                            {
                                UINT   uUrlLen        = lstrlenW(psuThis->pwcsUrl);
                                UINT   uPrefixLen     = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                                LPTSTR pszPrefixedUrl =
                                    ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + uPrefixLen + 1) * sizeof(TCHAR)));
                                if (pszPrefixedUrl)
                                {
                                    wnsprintf(pszPrefixedUrl, uPrefixLen + 1 , TEXT("%s%s@"), c_szHistPrefix, szUserName);

                                    StrCpyN(pszPrefixedUrl + uPrefixLen, psuThis->pwcsUrl, uUrlLen + 1);

                                    PROPVARIANT vProp = {0};
                                    if (SUCCEEDED(pUrlHistStg->GetProperty(pszPrefixedUrl + uPrefixLen,
                                                                           PID_INTSITE_VISITCOUNT, &vProp)) &&
                                        (vProp.vt == VT_UI4))
                                    {
                                        pol->insert(new OrderList_CacheElement(pszPrefixedUrl,
                                                                               vProp.lVal,
                                                                               psuThis));
                                        // OrderList now owns this -- he'll free it
                                        psuThis = new STATURL;
                                        if (psuThis)
                                        {
                                            psuThis->cbSize    = sizeof(STATURL);
                                            psuThis->pwcsUrl   = NULL;
                                            psuThis->pwcsTitle = NULL;
                                        }
                                        else if (pol) {
                                            delete pol;
                                            pol = NULL;
                                        }
                                    }
                                    LocalFree(pszPrefixedUrl);
                                }
                                else if (pol) { // couldn't allocate
                                    delete pol;
                                    pol = NULL;
                                }
                            }

                            if (psuThis && psuThis->pwcsUrl)
                                OleFree(psuThis->pwcsUrl);

                            if (psuThis && psuThis->pwcsTitle)
                                OleFree(psuThis->pwcsTitle);
                        }
                        else // nothing more from the enumeration...
                            break;
                    } //while
                    if (psuThis)
                        delete psuThis;
                }
                else if (pol) { //allocation failed
                    delete pol;
                    pol = NULL;
                }
            }
            penum->Release();
        }
        /*    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;
              DWORD dwError; */

        // This commented-out code does the same thing WITHOUT going through
        //  the IUrlHistoryPriv interface, but, instead going directly
        //  to wininet
        /*
          while ((dwError = _FindURLCacheEntry(c_szHistPrefix, _pceiWorking,
          _hEnum, &dwBuffSize)) == NOERROR) {
          // if its a top-level history guy && is cache entry to valid username
          if ( (((HISTDATA *)_pceiWorking->lpHeaderInfo)->dwFlags & PIDISF_HISTORY) && //top-level
          (_FilterUserName(_pceiWorking, c_szHistPrefix, szUserName)) ) // username is good
          {
          // perf:  we can avoid needlessly creating new cache elements if we're less lazy
          pol->insert(new OrderList_CacheElement(_pceiWorking->lpszSourceUrlName,
          _pceiWorking->dwHitRate,
          _pceiWorking->LastModifiedTime));
          }
          dwBuffSize = MAX_URLCACHE_ENTRY;
          }
          ASSERT(dwError == ERROR_NO_MORE_ITEMS);
          */
        pUrlHistStg->Release();
    } // no storage

    return pol;
}

HRESULT CHistCacheFolderEnum::_NextViewPart_OrderFreq(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hRes = E_INVALIDARG;

    if ( (!_polFrequentPages) && (!(_polFrequentPages = _GetMostFrequentPages())) )
        return E_FAIL;

    if (rgelt && pceltFetched) {
        // loop to fetch as many elements as requested.
        for (*pceltFetched = 0; *pceltFetched < celt;) {
            // contruct a pidl out of the first element in the orderedlist cache
            OrderList_CacheElement *polce = reinterpret_cast<OrderList_CacheElement *>
                (_polFrequentPages->removeFirst());
            if (polce) {
                if (!(rgelt[*pceltFetched] =
                      reinterpret_cast<LPITEMIDLIST>
                      (_CreateHCacheFolderPidl(TRUE,
                                               polce->pszUrl, polce->lpSTATURL->ftLastVisited,
                                               polce->lpSTATURL,
                                               polce->llPriority,
                                               polce->dwHitRate))))
                {
                    delete polce;
                    hRes = E_OUTOFMEMORY;
                    break;
                }
                ++(*pceltFetched);
                delete polce;
                hRes = S_OK;
            }
            else {
                hRes = S_FALSE; // no more...
                break;
            }
        }
    }
    return hRes;
}

// The Next method for view -- Order by Site
HRESULT CHistCacheFolderEnum::_NextViewPart_OrderSite(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    DWORD      dwError         = NOERROR;
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen   = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    LPCTSTR    pszStrippedUrl, pszHost, pszCachePrefix = NULL;
    LPITEMIDLIST  pcei         = NULL;
    LPCTSTR    pszHostToMatch  = NULL;
    UINT       nHostToMatchLen = 0;

    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    if ((!_pceiWorking) &&
        (!(_pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY))))
        return E_OUTOFMEMORY;

    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;

    // load all the intervals and do some cache maintenance:
    if (FAILED(_pHCFolder->_ValidateIntervalCache()))
        return E_OUTOFMEMORY;

    /* To get all sites, we will search all the history buckets
       for "Host"-type entries.  These entries will be put into
       a hash table as we enumerate so that redundant results are
       not returned.                                               */

    if (!_pshHashTable)
    {
        // start a new case-insensitive hash table
        _pshHashTable = new StrHash(TRUE);
        if (_pshHashTable == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    // if we are looking for individual pages within a host,
    //  then we must find which host to match...
    if (_pHCFolder->_uViewDepth == 1) {
        LPCITEMIDLIST pidlHost = ILFindLastID(_pHCFolder->_pidl);
        ASSERT(_IsValid_IDPIDL(pidlHost) &&
               EQUIV_IDSIGN(((LPCEIPIDL)pidlHost)->usSign, IDDPIDL_SIGN));
        pszHostToMatch = _GetURLTitle((LPCEIPIDL)pidlHost);
        nHostToMatchLen = (pszHostToMatch ? lstrlen(pszHostToMatch) : 0);

    }

    // iterate backwards through containers so most recent
    //  information gets put into the final pidl
    if (!_hEnum)
        _cbCurrentInterval = (_pHCFolder->_cbIntervals - 1);

    while((dwError = _FindURLFlatCacheEntry(_pHCFolder->_pIntervalCache, szUserName,
                                            (_pHCFolder->_uViewDepth == 1),
                                            _cbCurrentInterval,
                                            _pceiWorking, _hEnum, &dwBuffSize)) == NOERROR)
    {
        // reset for next iteration
        dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

        // this guy takes out the "t-marcmi@" part of the URL
        pszStrippedUrl = _StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName);
        if (_pHCFolder->_uViewDepth == 0) {
            if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN) {
                pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
                // insertUnique returns non-NULL if this key already exists
                if (_pshHashTable->insertUnique(pszHost, TRUE, reinterpret_cast<void *>(1)))
                    continue; // already given out
                pcei = (LPITEMIDLIST)_CreateIdCacheFolderPidl(TRUE, IDDPIDL_SIGN, pszHost);
            }
            break;
        }
        else if (_pHCFolder->_uViewDepth == 1) {
            TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
            // is this entry a doc from the host we're looking for?
            _GetURLHost(_pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, _GetLocalHost());

            if ( (!StrCmpI(szHost, pszHostToMatch)) &&
                 (!_pshHashTable->insertUnique(pszStrippedUrl,
                                               TRUE, reinterpret_cast<void *>(1))) )
            {
                STATURL suThis;
                HRESULT hresLocal            = E_FAIL;
                IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();

                if (pUrlHistStg) {
                    hresLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                    pUrlHistStg->Release();
                }

                pcei = (LPITEMIDLIST)
                    _CreateHCacheFolderPidl(TRUE, _pceiWorking->lpszSourceUrlName,
                                            _pceiWorking->LastModifiedTime,
                                            (SUCCEEDED(hresLocal) ? &suThis : NULL), 0,
                                            _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName)));
                if (SUCCEEDED(hresLocal) && suThis.pwcsTitle)
                    OleFree(suThis.pwcsTitle);
                break;
            }
        }
    }

    if (pcei && rgelt) {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched)
            *pceltFetched = 1;
    }
    else {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (dwError != NOERROR) {
        if (pceltFetched)
            *pceltFetched = 0;
        if (_hEnum)
            FindCloseUrlCache(_hEnum);
        return S_FALSE;
    }
    return S_OK;
}

// "Next" method for View by "Order seen today"
HRESULT CHistCacheFolderEnum::_NextViewPart_OrderToday(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    DWORD      dwError    = NOERROR;
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    LPCTSTR    pszStrippedUrl, pszHost;
    LPCEIPIDL  pcei = NULL;

    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    if ((!_pceiWorking) &&
        (!(_pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY))))
        return E_OUTOFMEMORY;

    if (!_hEnum) {
        // load all the intervals and do some cache maintenance:
        if (FAILED(_pHCFolder->_ValidateIntervalCache()))
            return E_OUTOFMEMORY;
        // get only entries for TODAY (important part)
        SYSTEMTIME   sysTime;
        FILETIME     fileTime;
        GetLocalTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        if (FAILED(_pHCFolder->_GetInterval(&fileTime, FALSE, &_pIntervalCur)))
            return E_FAIL; // couldn't get interval for Today
    }

    DWORD dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

    while ( (dwError = _FindURLCacheEntry(_pIntervalCur->szPrefix, _pceiWorking, _hEnum,
                                          &dwBuffSize)) == NOERROR )
    {
        dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

        // Make sure that his cache entry belongs to szUserName
        if (_FilterUserName(_pceiWorking, _pIntervalCur->szPrefix, szUserName)) {
            // this guy takes out the "t-marcmi@" part of the URL
            pszStrippedUrl = _StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName);
            if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN) {
                pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
                if (StrCmpNI(c_szHostPrefix, pszStrippedUrl, HOSTPREFIXLEN) == 0)
                    continue; // this is a HOST placeholder, not a real doc
            }

            IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
            STATURL suThis;
            HRESULT hresLocal = E_FAIL;

            if (pUrlHistStg) {
                hresLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                pUrlHistStg->Release();
            }
            pcei = (LPCEIPIDL) _CreateHCacheFolderPidl(TRUE, _pceiWorking->lpszSourceUrlName,
                                                       _pceiWorking->LastModifiedTime,
                                                       (SUCCEEDED(hresLocal) ? &suThis : NULL), 0,
                                                       _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName)));
            if (SUCCEEDED(hresLocal) && suThis.pwcsTitle)
                OleFree(suThis.pwcsTitle);
            break;
        }
    }

    if (pcei && rgelt) {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched)
            *pceltFetched = 1;
    }

    if (dwError == ERROR_NO_MORE_ITEMS) {
        if (pceltFetched)
            *pceltFetched = 0;
        if (_hEnum)
            FindCloseUrlCache(_hEnum);
        return S_FALSE;
    }
    else if (dwError == NOERROR)
        return S_OK;
    else
        return E_FAIL;
}

/***********************************************************************
  Search Mamagement Stuff:

  In order to maintian state between binds to the IShellFolder from
  the desktop, we base our state information for the searches off a
  global database (linked list) that is keyed by a timestamp generated
  when the search begins.

  This FILETIME is in the pidl for the search.
  ********************************************************************/

class _CurrentSearches {
public:
    LONG      _cRef;
    FILETIME  _ftSearchKey;
    LPWSTR    _pwszSearchTarget;
    IShellFolderSearchableCallback *_psfscOnAsyncSearch;

    CacheSearchEngine::StreamSearcher _streamsearcher;

    // Currently doing async search
    BOOL      _fSearchingAsync;

    // On next pass, kill this search
    BOOL      _fKillSwitch;

    // WARNING: DO NOT access these elements without a critical section!
    _CurrentSearches  *_pcsNext;
    _CurrentSearches  *_pcsPrev;

    static _CurrentSearches* s_pcsCurrentCacheSearchThreads;

    _CurrentSearches(FILETIME &ftSearchKey, LPCWSTR pwszSrch,
                     IShellFolderSearchableCallback *psfsc,
                     _CurrentSearches *pcsNext = s_pcsCurrentCacheSearchThreads) :
        _streamsearcher(pwszSrch),
        _fSearchingAsync(FALSE), _fKillSwitch(FALSE), _cRef(1)
    {
        _ftSearchKey      = ftSearchKey;
        _pcsNext          = pcsNext;
        _pcsPrev          = NULL;

        if (psfsc)
            psfsc->AddRef();

        _psfscOnAsyncSearch = psfsc;
        SHStrDupW(pwszSrch, &_pwszSearchTarget);
    }

    ULONG AddRef() {
        return InterlockedIncrement(&_cRef);
    }

    ULONG Release() {
        if (InterlockedDecrement(&_cRef))
            return _cRef;
        delete this;
        return 0;
    }

    // this will increment the refcount to be decremented by s_RemoveSearch
    static void s_NewSearch(_CurrentSearches *pcsNew,
                            _CurrentSearches *&pcsHead = s_pcsCurrentCacheSearchThreads)
    {
        ENTERCRITICAL;
        // make sure we're inserting at the front of the list
        ASSERT(pcsNew->_pcsNext == pcsHead);
        ASSERT(pcsNew->_pcsPrev == NULL);

        pcsNew->AddRef();
        if (pcsHead)
            pcsHead->_pcsPrev = pcsNew;
        pcsHead = pcsNew;
        LEAVECRITICAL;
    }

    static void s_RemoveSearch(_CurrentSearches *pcsRemove,
                               _CurrentSearches *&pcsHead = s_pcsCurrentCacheSearchThreads);

    // This searches for the search.
    // To find this search searcher, use the search searcher searcher :)
    static _CurrentSearches *s_FindSearch(const FILETIME &ftSearchKey,
                                          _CurrentSearches *pcsHead = s_pcsCurrentCacheSearchThreads);

protected:
    ~_CurrentSearches() {
        if (_psfscOnAsyncSearch)
            _psfscOnAsyncSearch->Release();
        CoTaskMemFree(_pwszSearchTarget);
    }
};

// A linked list of current cache searchers:
//  For multiple entries to occur in this list, the user would have to be
//  searching the cache on two or more separate queries simultaneously
_CurrentSearches *_CurrentSearches::s_pcsCurrentCacheSearchThreads = NULL;

void _CurrentSearches::s_RemoveSearch(_CurrentSearches *pcsRemove, _CurrentSearches *&pcsHead)
{
    ENTERCRITICAL;
    if (pcsRemove->_pcsPrev)
        pcsRemove->_pcsPrev->_pcsNext = pcsRemove->_pcsNext;
    else
        pcsHead = pcsRemove->_pcsNext;

    if (pcsRemove->_pcsNext)
        pcsRemove->_pcsNext->_pcsPrev = pcsRemove->_pcsPrev;

    pcsRemove->Release();
    LEAVECRITICAL;
}

// Caller: Remember to Release() the returned data!!
_CurrentSearches *_CurrentSearches::s_FindSearch(const FILETIME &ftSearchKey,
                                                 _CurrentSearches *pcsHead)
{
    ENTERCRITICAL;
    _CurrentSearches *pcsTemp = pcsHead;
    _CurrentSearches *pcsRet  = NULL;
    while (pcsTemp) {
        if (((pcsTemp->_ftSearchKey).dwLowDateTime  == ftSearchKey.dwLowDateTime) &&
            ((pcsTemp->_ftSearchKey).dwHighDateTime == ftSearchKey.dwHighDateTime))
        {
            pcsRet = pcsTemp;
            break;
        }
        pcsTemp = pcsTemp->_pcsNext;
    }
    if (pcsRet)
        pcsRet->AddRef();
    LEAVECRITICAL;
    return pcsRet;
}
/**********************************************************************/

HRESULT CHistCacheFolderEnum::_NextViewPart_OrderSearch(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched) {
    HRESULT hRes      = E_FAIL;
    ULONG   uFetched  = 0;

    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;
    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT    uUserNameLen = lstrlen(szUserName);

    if (_pstatenum == NULL) {
        // This hashtable will eventually be passed off to the background
        //  cache search thread so that it doesn't return duplicates.
        ASSERT(NULL == _pshHashTable)  // don't leak a _pshHashTable
        _pshHashTable = new StrHash(TRUE);
        if (_pshHashTable) {
            IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
            if (pUrlHistStg) {
                if (SUCCEEDED((hRes = pUrlHistStg->EnumUrls(&_pstatenum))))
                    _pstatenum->SetFilter(NULL, STATURL_QUERYFLAG_TOPLEVEL);
                pUrlHistStg->Release();
            }
        }
    }
    else
        hRes = S_OK;

    if (SUCCEEDED(hRes)) {
        ASSERT(_pstatenum && _pshHashTable);

        for (uFetched; uFetched < celt;) {
            STATURL staturl = { 0 };
            staturl.cbSize = sizeof(staturl);
            ULONG   celtFetched = 0;
            if (SUCCEEDED((hRes = _pstatenum->Next(1, &staturl, &celtFetched)))) {
                if (celtFetched) {
                    ASSERT(celtFetched == 1);
                    if (staturl.pwcsUrl && (staturl.dwFlags & STATURLFLAG_ISTOPLEVEL)) {
                        BOOL fMatch = FALSE;

                        // all this streamsearcher stuff is just like a 'smart' StrStr
                        CacheSearchEngine::StringStream ssUrl(staturl.pwcsUrl);
                        if ((!(fMatch =
                               (_pHCFolder->_pcsCurrentSearch->_streamsearcher).SearchCharStream(ssUrl))) &&
                            staturl.pwcsTitle)
                        {
                            CacheSearchEngine::StringStream ssTitle(staturl.pwcsTitle);
                            fMatch = (_pHCFolder->_pcsCurrentSearch->_streamsearcher).SearchCharStream(ssTitle);
                        }

                        if (fMatch){ // MATCH!
                            // Now, we have to convert the url to a prefixed (ansi, if necessary) url
                            UINT   uUrlLen        = lstrlenW(staturl.pwcsUrl);
                            UINT   uPrefixLen     = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                            LPTSTR pszPrefixedUrl =
                                ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + uPrefixLen + 1) * sizeof(TCHAR)));
                            if (pszPrefixedUrl){
                                wnsprintf(pszPrefixedUrl, uPrefixLen + uUrlLen + 1,
                                          TEXT("%s%s@%ls"), c_szHistPrefix, szUserName,
                                          staturl.pwcsUrl);
                                LPHEIPIDL pheiTemp =
                                    _CreateHCacheFolderPidl(TRUE,
                                                            pszPrefixedUrl, staturl.ftLastVisited,
                                                            &staturl, 0,
                                                            _pHCFolder->_GetHitCount(pszPrefixedUrl + uPrefixLen));
                                if (pheiTemp) {
                                    _pshHashTable->insertUnique(pszPrefixedUrl + uPrefixLen, TRUE,
                                                                reinterpret_cast<void *>(1));
                                    rgelt[uFetched++] = (LPITEMIDLIST)pheiTemp;
                                    hRes = S_OK;
                                }
                                LocalFree(pszPrefixedUrl);
                            }
                        }
                    }
                    if (staturl.pwcsUrl)
                        OleFree(staturl.pwcsUrl);

                    if (staturl.pwcsTitle)
                        OleFree(staturl.pwcsTitle);
                }
                else {
                    hRes = S_FALSE;
                    // Addref this for the ThreadProc who then frees it...
                    AddRef();
#ifdef DEBUG
                    // The memory that goes in as an input parameter is all freed on a different thread
                    // Hence - we need to remove them from the memlist and put them on the mem list
                    // of the other thread when we get there

                    remove_from_memlist((LPVOID)this); // Make sure this is Released before exiting
                                                        // the thread proc

                    if(_pHCFolder){
                        remove_from_memlist((LPVOID)_pHCFolder);
                    }
                        
                    if(_pshHashTable){
                        _pshHashTable->_RemoveHashNodesFromMemList();
                        remove_from_memlist((LPVOID)_pshHashTable);
                    }
                    if(_polFrequentPages){
                        _polFrequentPages->_RemoveElementsFromMemlist();
                        remove_from_memlist((LPVOID)_polFrequentPages);
                    }

                    if(_pstatenum){
                        remove_from_memlist((LPVOID)_pstatenum);
                    }
                    
                    
       
#endif // DEBUG
                    SHQueueUserWorkItem((LPTHREAD_START_ROUTINE)s_CacheSearchThreadProc,
                                        (LPVOID)this,
                                        0,
                                        (DWORD_PTR)NULL,
                                        (DWORD_PTR *)NULL,
                                        "shdocvw.dll",
                                        0
                                        );
                    break;
                }
            } // succeeded getnext url
        } //for

        if (pceltFetched)
            *pceltFetched = uFetched;
    } // succeeded initalising
    return hRes;
}

// helper function for s_CacheSearchThreadProc
BOOL_PTR CHistCacheFolderEnum::s_DoCacheSearch(LPINTERNET_CACHE_ENTRY_INFO pcei,
                                           LPTSTR pszUserName, UINT uUserNameLen,
                                           CHistCacheFolderEnum *penum,
                                           _CurrentSearches *pcsThisThread, IUrlHistoryPriv *pUrlHistStg)
{
    BOOL_PTR   fFound = FALSE;
    LPTSTR pszTextHeader;

    // The header contains "Content-type: text/*"
    if (pcei->lpHeaderInfo && (pszTextHeader = StrStrI(pcei->lpHeaderInfo, c_szTextHeader)))
    {
        // in some cases, urls in the cache differ from urls in the history
        //  by only the trailing slash -- we strip it out and test both
        UINT uUrlLen = lstrlen(pcei->lpszSourceUrlName);
        if (uUrlLen && (pcei->lpszSourceUrlName[uUrlLen - 1] == TEXT('/')))
        {
            pcei->lpszSourceUrlName[uUrlLen - 1] = TEXT('\0');
            fFound = (BOOL_PTR)(penum->_pshHashTable->retrieve(pcei->lpszSourceUrlName));
            pcei->lpszSourceUrlName[uUrlLen - 1] = TEXT('/');
        }

        DWORD dwSize = MAX_URLCACHE_ENTRY;
        // see if its already been found and added...
        if ((!fFound) && !(penum->_pshHashTable->retrieve(pcei->lpszSourceUrlName)))
        {
            BOOL fIsHTML = !StrCmpNI(pszTextHeader + TEXTHEADERLEN, c_szHTML, HTMLLEN);
            // Now, try to find the url in history...

            STATURL staturl;
            HRESULT hresLocal;
            hresLocal = pUrlHistStg->QueryUrl(pcei->lpszSourceUrlName, STATFLAG_NONAME, &staturl);
            if (hresLocal == S_OK)
            {
                HANDLE hCacheStream;

                hCacheStream = RetrieveUrlCacheEntryStream(pcei->lpszSourceUrlName, pcei, &dwSize, FALSE, 0);
                if (hCacheStream)
                {
                    if (CacheSearchEngine::SearchCacheStream(pcsThisThread->_streamsearcher,
                                                             hCacheStream, fIsHTML)) {
                        EVAL(UnlockUrlCacheEntryStream(hCacheStream, 0));

                        // Prefix the url so that we can create a pidl out of it -- for now, we will
                        //  prefix it with "Visited: ", but "Bogus: " may be more appropriate.
                        UINT uUrlLen    = lstrlen(pcei->lpszSourceUrlName);
                        UINT uPrefixLen = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                        UINT uBuffSize  = uUrlLen + uPrefixLen + 1;
                        LPTSTR pszPrefixedUrl =
                            ((LPTSTR)LocalAlloc(LPTR, uBuffSize * sizeof(TCHAR)));

                        if (pszPrefixedUrl)
                        {
                            wnsprintf(pszPrefixedUrl, uBuffSize, TEXT("%s%s@%s"), c_szHistPrefix, pszUserName,
                                      pcei->lpszSourceUrlName);

                            // Create a pidl for this url
                            LPITEMIDLIST pidlFound =
                                (LPITEMIDLIST)
                                penum->_pHCFolder->_CreateHCacheFolderPidlFromUrl(FALSE, pszPrefixedUrl);

                            if (pidlFound)
                            {
                                LPITEMIDLIST pidlNotify = ILCombine(penum->_pHCFolder->_pidl, pidlFound);
                                if (pidlNotify) 
                                {
                                    // add the item to the results list...
                                    /* without the flush, the shell will coalesce these and turn
                                       them info SHChangeNotify(SHCNE_UPDATEDIR,..), which will cause nsc
                                       to do an EnumObjects(), which will start the search up again and again...
                                       */
                                    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST | SHCNF_FLUSH, pidlNotify, NULL);
                                    ILFree(pidlNotify);
                                    fFound = TRUE;
                                }

                                LocalFree(pidlFound);
                            }

                            LocalFree(pszPrefixedUrl);
                        }
                    }
                    else
                        EVAL(UnlockUrlCacheEntryStream(hCacheStream, 0));
                }
            }
            else
                TraceMsg(DM_CACHESEARCH, "In Cache -- Not In History: %s", pcei->lpszSourceUrlName);
        }
    }
    return fFound;
}

DWORD WINAPI CHistCacheFolderEnum::s_CacheSearchThreadProc(CHistCacheFolderEnum *penum)
{
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;
#ifdef DEBUG
    // The memory that goes in as an input parameter is all freed on this thread
    // Hence - we need to put them on this mem list
    ASSERT(penum);
    DbgAddToMemList((LPVOID)penum);// Make sure penum is Release'd before exiting
                                   // the thread proc
    if(penum->_pHCFolder){
        DbgAddToMemList((LPVOID)(penum->_pHCFolder));
    }
    if(penum->_pshHashTable)
    {
        DbgAddToMemList((LPVOID)(penum->_pshHashTable));
        penum->_pshHashTable->_AddHashNodesFromMemList();
    }
    if(penum->_polFrequentPages)
    {
        DbgAddToMemList((LPVOID)(penum->_polFrequentPages));   
        penum->_polFrequentPages->_AddElementsToMemlist();
    }

    if(penum->_pstatenum)
    {
        DbgAddToMemList((LPVOID)(penum->_pstatenum));
    }
#endif // DEBUG

    if (FAILED(penum->_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT    uUserNameLen = lstrlen(szUserName);

    BOOL    fNoConflictingSearch = TRUE;

    _CurrentSearches *pcsThisThread = NULL;

    IUrlHistoryPriv *pUrlHistStg = penum->_pHCFolder->_GetHistStg();

    if (pUrlHistStg)
    {

        pcsThisThread = _CurrentSearches::s_FindSearch(penum->_pHCFolder->_pcsCurrentSearch->_ftSearchKey);

        if (pcsThisThread)
        {
            // if no one else is doing the same search
            if (FALSE == InterlockedExchange((LONG *)&(pcsThisThread->_fSearchingAsync), TRUE))
            {
                if (pcsThisThread->_psfscOnAsyncSearch)
                    pcsThisThread->_psfscOnAsyncSearch->RunBegin(0);

                BYTE ab[MAX_URLCACHE_ENTRY];
                LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)(&ab);

                DWORD dwSize = MAX_URLCACHE_ENTRY;
                HANDLE hCacheEnum = FindFirstUrlCacheEntry(NULL, pcei, &dwSize);
                if (hCacheEnum)
                {
                    while(!(pcsThisThread->_fKillSwitch))
                    {
                        s_DoCacheSearch(pcei, szUserName, uUserNameLen, penum, pcsThisThread, pUrlHistStg);
                        dwSize = MAX_URLCACHE_ENTRY;
                        if (!FindNextUrlCacheEntry(hCacheEnum, pcei, &dwSize))
                        {
                            ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
                            break;
                        }
                    }
                    FindCloseUrlCache(hCacheEnum);
                }

                if (pcsThisThread->_psfscOnAsyncSearch)
                    pcsThisThread->_psfscOnAsyncSearch->RunEnd(0);

                pcsThisThread->_fSearchingAsync = FALSE; // It's been removed - no chance of
                                                         // a race condition
            }
            pcsThisThread->Release();
        }
        ATOMICRELEASE(pUrlHistStg);
    }
    ATOMICRELEASE(penum);
    return 0;
}


//
//  this gets the local host name as known by the shell
//  by default assume "My Computer" or whatever
//
void _GetLocalHost(LPTSTR psz, DWORD cch)
{
    *psz = 0;

    IShellFolder* psf;
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        WCHAR sz[GUIDSTR_MAX + 3];

        sz[0] = sz[1] = TEXT(':');
        SHStringFromGUIDW(CLSID_MyComputer, sz+2, SIZECHARS(sz)-2);

        LPITEMIDLIST pidl;
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, sz, NULL, &pidl, NULL)))
        {
            STRRET sr;
            if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_NORMAL, &sr)))
                StrRetToBuf(&sr, pidl, psz, cch);
            ILFree(pidl);
        }

        psf->Release();
    }

    if (!*psz)
        MLLoadString(IDS_NOTNETHOST, psz, cch);
}

LPCTSTR CHistCacheFolderEnum::_GetLocalHost(void)
{
    if (!*_szLocalHost)
        ::_GetLocalHost(_szLocalHost, SIZECHARS(_szLocalHost));

    return _szLocalHost;
}

//////////////////////////////////
//
// IEnumIDList Methods
//
HRESULT CHistCacheFolderEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hres             = S_FALSE;
    DWORD   dwBuffSize;
    DWORD   dwError;
    LPTSTR  pszSearchPattern = NULL;
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    TCHAR   szHistSearchPattern[PREFIX_SIZE + 1];               // search pattern for history items
    TCHAR   szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];

    TraceMsg(DM_HSFOLDER, "hcfe - Next() called.");

    if (_pHCFolder->_uViewType)
        return _NextViewPart(celt, rgelt, pceltFetched);

    if ((IsLeaf(_pHCFolder->_foldertype) && 0 == (SHCONTF_NONFOLDERS & _grfFlags)) ||
        (!IsLeaf(_pHCFolder->_foldertype) && 0 == (SHCONTF_FOLDERS & _grfFlags)))
    {
        dwError = 0xFFFFFFFF;
        goto exitPoint;
    }

    if (FOLDER_TYPE_Hist == _pHCFolder->_foldertype)
    {
        return _NextHistInterval(celt, rgelt, pceltFetched);
    }

    if (_pceiWorking == NULL)
    {
        _pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
        if (_pceiWorking == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exitPoint;
        }
    }

    // Set up things to enumerate history items, if appropriate, otherwise,
    // we'll just pass in NULL and enumerate all items as before.

    if (IsHistory(_pHCFolder->_foldertype))
    {
        if (!_hEnum)
        {
            if (FAILED(_pHCFolder->_ValidateIntervalCache()))
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto exitPoint;
            }
        }

        if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
            szUserName[0] = TEXT('\0');

        StrCpyN(szHistSearchPattern, _pHCFolder->_pszCachePrefix, ARRAYSIZE(szHistSearchPattern));

        // BUGBUG: We can't pass in the whole search pattern that we want,
        // because FindFirstUrlCacheEntry is busted.  It will only look at the
        // prefix if there is a cache container for that prefix.  So, we can
        // pass in "Visited: " and enumerate all the history items in the cache,
        // but then we need to pull out only the ones with the correct username.

        // StrCpy(szHistSearchPattern, szUserName);

        pszSearchPattern = szHistSearchPattern;
    }

TryAgain:

    dwBuffSize = MAX_URLCACHE_ENTRY;
    dwError = NOERROR;

    if (!_hEnum) // _hEnum maintains our state as we iterate over all the cache entries
    {
       _hEnum = FindFirstUrlCacheEntry(pszSearchPattern, _pceiWorking, &dwBuffSize);
       if (!_hEnum)
           dwError = GetLastError();
    }

    else if (!FindNextUrlCacheEntry(_hEnum, _pceiWorking, &dwBuffSize))
    {
        dwError = GetLastError();
    }

    if (NOERROR == dwError)
    {
        LPCEIPIDL pcei = NULL;

        if (!IsHistory(_pHCFolder->_foldertype)) // not a history-type entry
        {
            if ((_pceiWorking->CacheEntryType & URLHISTORY_CACHE_ENTRY) == URLHISTORY_CACHE_ENTRY)
                goto TryAgain;
            pcei = _CreateBuffCacheFolderPidl(TRUE, dwBuffSize, _pceiWorking);
            if (pcei != NULL)
                _GetFileTypeInternal(pcei, pcei->szTypeName, ARRAYSIZE(pcei->szTypeName));
        }
        else   // the FolderType is a History-type entry
        {
            TCHAR szTempStrippedUrl[MAX_URL_STRING];
            LPCTSTR pszStrippedUrl;
            BOOL fIsHost;
            LPCTSTR pszHost;

        //mm:  Make sure that this cache entry belongs to szUserName (relevant to Win95)
            if (!_FilterUserName(_pceiWorking, _pHCFolder->_pszCachePrefix, szUserName))
                goto TryAgain;

            StrCpyN(szTempStrippedUrl, _pceiWorking->lpszSourceUrlName, ARRAYSIZE(szTempStrippedUrl));
            pszStrippedUrl = _StripHistoryUrlToUrl(szTempStrippedUrl);
            if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN)
            {
                pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
                fIsHost = !StrCmpNI(c_szHostPrefix, pszStrippedUrl, HOSTPREFIXLEN);
            }
            else
            {
                fIsHost = FALSE;
            }
        //mm:  this is most likely domains:
            if (FOLDER_TYPE_HistInterval == _pHCFolder->_foldertype) // return unique domains
            {
                if (!fIsHost)
                    goto TryAgain;

                pcei = _CreateIdCacheFolderPidl(TRUE, IDDPIDL_SIGN, pszHost);
            }
            else if (NULL != _pHCFolder->_pszDomain) //mm: this must be docs
            {
                TCHAR szSourceUrl[MAX_URL_STRING];
                STATURL suThis;
                HRESULT hresLocal = E_FAIL;
                IUrlHistoryPriv *pUrlHistStg = NULL;

                if (fIsHost)
                    goto TryAgain;

                //  Filter domain in history view!
                _GetURLHost(_pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, _GetLocalHost());

                if (StrCmpI(szHost, _pHCFolder->_pszDomain)) //mm: is this in our domain?!
                    goto TryAgain;

                pUrlHistStg = _pHCFolder->_GetHistStg();
                if (pUrlHistStg)
                {
                    CHAR szTempUrl[MAX_URL_STRING];

                    SHTCharToAnsi(pszStrippedUrl, szTempUrl, ARRAYSIZE(szTempUrl));
                    hresLocal = pUrlHistStg->QueryUrlA(szTempUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                    pUrlHistStg->Release();
                }

                StrCpyN(szSourceUrl, _pceiWorking->lpszSourceUrlName, ARRAYSIZE(szSourceUrl));
                pcei = (LPCEIPIDL) _CreateHCacheFolderPidl(TRUE,
                                                           szSourceUrl,
                                                           _pceiWorking->LastModifiedTime,
                                                           (SUCCEEDED(hresLocal) ? &suThis : NULL), 0,
                                                           _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(szSourceUrl)));

                if (SUCCEEDED(hresLocal) && suThis.pwcsTitle)
                    OleFree(suThis.pwcsTitle);
            }
        }
        if (pcei)
        {
            rgelt[0] = (LPITEMIDLIST)pcei;
           if (pceltFetched)
               *pceltFetched = 1;
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

exitPoint:

    if (dwError != NOERROR)
    {
        if (_hEnum)
        {
            FindCloseUrlCache(_hEnum);
            _hEnum = NULL;
        }
        if (pceltFetched)
            *pceltFetched = 0;
        rgelt[0] = NULL;
        hres = S_FALSE;
    }
    else
    {
        hres = NOERROR;
    }
    return hres;
}

HRESULT CHistCacheFolderEnum::Skip(ULONG celt)
{
    TraceMsg(DM_HSFOLDER, "hcfe - Skip() called.");
    return E_NOTIMPL;
}

HRESULT CHistCacheFolderEnum::Reset()
{
    TraceMsg(DM_HSFOLDER, "hcfe - Reset() called.");
    return E_NOTIMPL;
}

HRESULT CHistCacheFolderEnum::Clone(IEnumIDList **ppenum)
{
    TraceMsg(DM_HSFOLDER, "hcfe - Clone() called.");
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheFolder Object
//
//////////////////////////////////////////////////////////////////////////////

CHistCacheFolder::CHistCacheFolder(FOLDER_TYPE FolderType)
{
    TraceMsg(DM_HSFOLDER, "hcf - CHistCacheFolder() called.");
    _cRef = 1;
    _foldertype = FolderType;
    ASSERT( _uViewType  == 0 &&
            _uViewDepth  == 0 &&
            _pszCachePrefix == NULL &&
            _pszDomain == NULL &&
            _cbIntervals == 0 &&
            _pIntervalCache == NULL &&
            _fValidatingCache == FALSE &&
            _dwIntervalCached == 0 &&
            _ftDayCached.dwHighDateTime == 0 &&
            _ftDayCached.dwLowDateTime == 0 &&
            _pidl == NULL );
    DllAddRef();
}

CHistCacheFolder::~CHistCacheFolder()
{
    ASSERT(_cRef == 0);                 // should always have zero
    TraceMsg(DM_HSFOLDER, "hcf - ~CHistCacheFolder() called.");
    if (_pIntervalCache)
    {
        LocalFree(_pIntervalCache);
    }
    if (_pszCachePrefix)
    {
        LocalFree(_pszCachePrefix);
    }
    if (_pszDomain)
    {
        LocalFree(_pszDomain);
    }
    if (_pidl)
        ILFree(_pidl);
    if (_pUrlHistStg)
    {
        _pUrlHistStg->Release();
        _pUrlHistStg = NULL;
    }
    if (_pcsCurrentSearch)
        _pcsCurrentSearch->Release();

    DllRelease();
}

LPITEMIDLIST _Combine_ViewPidl(USHORT usViewType, LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlResult = NULL;
    LPVIEWPIDL pviewpidl = (LPVIEWPIDL)SHAlloc(sizeof(VIEWPIDL) + sizeof(USHORT));
    if (pviewpidl)
    {
        memset(pviewpidl, 0, sizeof(VIEWPIDL) + sizeof(USHORT));
        pviewpidl->cb         = sizeof(VIEWPIDL);
        pviewpidl->usSign     = VIEWPIDL_SIGN;
        pviewpidl->usViewType = usViewType;
        ASSERT(pviewpidl->usExtra == 0);//pcei->usSign;
        if (pidl) 
        {
            pidlResult = ILCombine((LPITEMIDLIST)pviewpidl, pidl);
            SHFree(pviewpidl);
        }
        else
            pidlResult = (LPITEMIDLIST)pviewpidl;
    }
    return pidlResult;
}

STDMETHODIMP CHistFolder::_GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr)
{
    *pszStr = 0;

    switch (iColumn)
    {
    case ICOLH_URL_NAME:
        if (_IsLeaf())
            StrCpyN(pszStr, _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl)), cchStr);
        else
            _GetURLDispName((LPCEIPIDL)pidl, pszStr, cchStr);
        break;

    case ICOLH_URL_TITLE:
        _GetHistURLDispName((LPHEIPIDL)pidl, pszStr, cchStr);
        break;

    case ICOLH_URL_LASTVISITED:
        FileTimeToDateTimeStringInternal(&((LPHEIPIDL)pidl)->ftModified, pszStr, cchStr, TRUE);
        break;
    }
    return NOERROR;
}

HRESULT CHistFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hres;

    const COLSPEC *pcol;
    UINT nCols;

    if (_foldertype == FOLDER_TYPE_Hist)
    {
        pcol = s_HistIntervalFolder_cols;
        nCols = ARRAYSIZE(s_HistIntervalFolder_cols);
    }
    else if (_foldertype == FOLDER_TYPE_HistInterval)
    {
        pcol = s_HistHostFolder_cols;
        nCols = ARRAYSIZE(s_HistHostFolder_cols);
    }
    else
    {
        pcol = s_HistFolder_cols;
        nCols = ARRAYSIZE(s_HistFolder_cols);
    }

    if (pidl == NULL)
    {
        if (iColumn < nCols)
        {
            TCHAR szTemp[128];
            pdi->fmt = pcol[iColumn].iFmt;
            pdi->cxChar = pcol[iColumn].cchCol;
            MLLoadString(pcol[iColumn].ids, szTemp, ARRAYSIZE(szTemp));
            hres = StringToStrRet(szTemp, &pdi->str);
        }
        else
            hres = E_FAIL;  // enum done
    }
    else
    {
        // Make sure the pidl is dword aligned.

    	if(iColumn >= nCols)
    	    hres = E_FAIL;
    	else
    	{
            BOOL fRealigned;
            hres = AlignPidl(&pidl, &fRealigned);

            if (SUCCEEDED(hres) )
            {
                TCHAR szTemp[MAX_URL_STRING];
                hres = _GetDetail(pidl, iColumn, szTemp, ARRAYSIZE(szTemp));
                if (SUCCEEDED(hres))
                    hres = StringToStrRet(szTemp, &pdi->str);

            }
            if (fRealigned)
                FreeRealignedPidl(pidl);
        }
    }
    return hres;
}

STDMETHODIMP CCacheFolder::_GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr)
{
    switch (iColumn) {
    case ICOLC_URL_SHORTNAME:
        _GetCacheItemTitle((LPCEIPIDL)pidl, pszStr, cchStr);
        break;

    case ICOLC_URL_NAME:
        StrCpyN(pszStr, HCPidlToSourceUrl(pidl), cchStr);
        break;

    case ICOLC_URL_TYPE:
        StrCpyN(pszStr, ((LPCEIPIDL)pidl)->szTypeName, cchStr);
        break;

    case ICOLC_URL_SIZE:
        StrFormatKBSize(((LPCEIPIDL)pidl)->cei.dwSizeLow, pszStr, cchStr);
        break;

    case ICOLC_URL_EXPIRES:
        FileTimeToDateTimeStringInternal(&((LPCEIPIDL)pidl)->cei.ExpireTime, pszStr, cchStr, FALSE);
        break;

    case ICOLC_URL_ACCESSED:
        FileTimeToDateTimeStringInternal(&((LPCEIPIDL)pidl)->cei.LastAccessTime, pszStr, cchStr, FALSE);
        break;

    case ICOLC_URL_MODIFIED:
        FileTimeToDateTimeStringInternal(&((LPCEIPIDL)pidl)->cei.LastModifiedTime, pszStr, cchStr, FALSE);
        break;

    case ICOLC_URL_LASTSYNCED:
        FileTimeToDateTimeStringInternal(&((LPCEIPIDL)pidl)->cei.LastSyncTime, pszStr, cchStr, FALSE);
        break;
    }
    return NOERROR;
}

HRESULT CCacheFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hres;

    if (pidl == NULL)
    {
        if (iColumn < ICOLC_URL_MAX)
        {
            TCHAR szTemp[128];
            MLLoadString(s_CacheFolder_cols[iColumn].ids, szTemp, ARRAYSIZE(szTemp));
            pdi->fmt = s_CacheFolder_cols[iColumn].iFmt;
            pdi->cxChar = s_CacheFolder_cols[iColumn].cchCol;
            hres = StringToStrRet(szTemp, &pdi->str);
        }
        else
            hres = E_FAIL;  // enum done
    }
    else
    {
        TCHAR szTemp[MAX_URL_STRING];
        hres = _GetDetail(pidl, iColumn, szTemp, ARRAYSIZE(szTemp));
        if (SUCCEEDED(hres))
            hres = StringToStrRet(szTemp, &pdi->str);
    }
    return hres;
}


STDAPI HistFolder_CreateInstance(IUnknown* punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;                     // null the out param

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CHistFolder *phist = new CHistFolder(FOLDER_TYPE_Hist);
    if (!phist)
        return E_OUTOFMEMORY;

    *ppunk = SAFECAST(phist, IShellFolder2*);
    return S_OK;
}

STDAPI CacheFolder_CreateInstance(IUnknown* punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;                     // null the out param

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CCacheFolder *pcache = new CCacheFolder(FOLDER_TYPE_Cache);
    if (!pcache)
        return E_OUTOFMEMORY;

    *ppunk = SAFECAST(pcache, IShellFolder2*);
    return S_OK;
}

HRESULT CHistCacheFolder::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hres;

    static const QITAB qitCache[] = {
        QITABENT(CHistCacheFolder, IShellFolder2),
        QITABENTMULTI(CHistCacheFolder, IShellFolder, IShellFolder2),
        QITABENT(CHistCacheFolder, IShellIcon),
        QITABENT(CHistCacheFolder, IPersistFolder2),
        QITABENTMULTI(CHistCacheFolder, IPersistFolder, IPersistFolder2),
        QITABENTMULTI(CHistCacheFolder, IPersist, IPersistFolder2),
        QITABENT(CHistCacheFolder, IContextMenu),
        { 0 },
    };

    static const QITAB qitHist[] = {
        QITABENT(CHistCacheFolder, IShellFolder2),
        QITABENTMULTI(CHistCacheFolder, IShellFolder, IShellFolder2),
        QITABENT(CHistCacheFolder, IShellIcon),
        QITABENT(CHistCacheFolder, IPersistFolder),
        QITABENTMULTI(CHistCacheFolder, IPersist, IPersistFolder),
        QITABENT(CHistCacheFolder, IContextMenu),
        QITABENT(CHistCacheFolder, IHistSFPrivate),
        QITABENT(CHistCacheFolder, IShellFolderViewType),
        QITABENT(CHistCacheFolder, IShellFolderSearchable),
        { 0 },
    };


    if (IsHistoryFolder(_foldertype))
    {
        if (IID_IPersistFolder == iid && FOLDER_TYPE_Hist != _foldertype)
        {
            hres = E_NOINTERFACE;
            *ppv = NULL;
            return hres;
        }
        hres = QISearch(this, qitHist, iid, ppv);
    }
    else
    {
        hres = QISearch(this, qitCache, iid, ppv);
    }
    if (FAILED(hres))
    {
        if (iid == IID_IShellView)
        {
            // this is a total hack... return our view object from this folder
            //
            // the desktop.ini file for "Temporary Internet Files" has UICLSID={guid of this object}
            // this lets us implment only ths IShellView for this folder, leaving the IShellFolder
            // to the default file system. this enables operations on the pidls that are stored in
            // this folder that would otherwise faile since our IShellFolder is not as complete
            // as the default (this is the same thing the font folder does).
            //
            // to support this with defview we would either have to do a complete wrapper object
            // for the view implemenation, or add this hack that hands out the view object, this
            // assumes we know the order of calls that the shell makes to create this object
            // and get the IShellView implementation
            //
            ASSERT(!_uViewType);
            hres = HistCacheFolderView_CreateInstance(this, _pidl, ppv);
        }
        else if (iid == CLSID_HistFolder)
        {
            *ppv = (void *)(CHistCacheFolder *)this;
            AddRef();
            hres = S_OK;
        }
    }
    return hres;
}

ULONG CHistCacheFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistCacheFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistCacheFolder::_ExtractInfoFromPidl()
{
    LPITEMIDLIST pidlThis;
    HRESULT hres;
    LPITEMIDLIST pidlLast = NULL;
    LPITEMIDLIST pidlSecondLast = NULL;

    ASSERT(!_uViewType);

    pidlThis = _pidl;
    while (pidlThis->mkid.cb)
    {
        pidlSecondLast = pidlLast;
        pidlLast = pidlThis;
        pidlThis = _ILNext(pidlThis);
    }
    switch (_foldertype)
    {
    case FOLDER_TYPE_Hist:
        _pidlRest = pidlThis;
        break;
    case FOLDER_TYPE_HistInterval:
        _pidlRest = pidlLast;
        break;
    case FOLDER_TYPE_HistDomain:
        _pidlRest = pidlSecondLast;
        break;
    default:
        _pidlRest = NULL;
    }
    hres = NULL == _pidlRest ? E_FAIL:S_OK;

    pidlThis = _pidlRest;
    if (SUCCEEDED(hres))
    {
        while (pidlThis->mkid.cb)
        {
            if (_IsValid_IDPIDL(pidlThis))
            {
                LPCEIPIDL pcei = (LPCEIPIDL)pidlThis;

                if (EQUIV_IDSIGN(pcei->usSign,IDIPIDL_SIGN)) // This is our interval, it implies prefix
                {
                    LPCTSTR pszCachePrefix;

                    if (_foldertype == FOLDER_TYPE_Hist) _foldertype = FOLDER_TYPE_HistInterval;
                    hres = _LoadIntervalCache();
                    if (FAILED(hres)) goto exitPoint;
                    hres = _GetPrefixForInterval(_GetURLTitle((LPCEIPIDL)pidlThis), &pszCachePrefix);
                    if (FAILED(hres)) goto exitPoint;
                    hres = SetCachePrefix(pszCachePrefix);
                    if (FAILED(hres)) goto exitPoint;
                }
                else                              // This is our domain
                {
                    if (_foldertype == FOLDER_TYPE_HistInterval) _foldertype = FOLDER_TYPE_HistDomain;
                    SetDomain(_GetURLTitle((LPCEIPIDL)pidlThis));
                }
            }
            pidlThis = _ILNext(pidlThis);
        }
        switch(_foldertype)
        {
            case FOLDER_TYPE_HistDomain:
                if (_pszDomain == NULL)
                    hres = E_FAIL;
                //FALL THROUGH INTENDED
            case FOLDER_TYPE_HistInterval:
                if (_pszCachePrefix == NULL)
                    hres = E_FAIL;
                break;
        }
    }
exitPoint:
    return hres;
}

void _SetValueSign(HSFINTERVAL *pInterval, FILETIME ftNow)
{
    if (_DaysInInterval(pInterval) == 1 && !CompareFileTime(&(pInterval->ftStart), &ftNow))
    {
        pInterval->usSign = IDTPIDL_SIGN;
    }
    else
    {
        pInterval->usSign = IDIPIDL_SIGN;
    }
}

void _SetVersion(HSFINTERVAL *pInterval, LPCSTR szInterval)
{
    USHORT usVers = 0;
    int i;
    DWORD dwIntervalLen = lstrlenA(szInterval);

    //  Unknown versions are 0
    if (dwIntervalLen == INTERVAL_SIZE)
    {
        for (i = INTERVAL_PREFIX_LEN; i < INTERVAL_PREFIX_LEN+INTERVAL_VERS_LEN; i++)
        {
            if ('0' > szInterval[i] || '9' < szInterval[i])
            {
                usVers = UNK_INTERVAL_VERS;
                break;
            }
            usVers = usVers * 10 + (szInterval[i] - '0');
        }
    }
    pInterval->usVers = usVers;
}

#ifdef UNICODE
#define _ValueToInterval           _ValueToIntervalW
#else // UNICODE
#define _ValueToInterval           _ValueToIntervalA
#endif // UNICODE

HRESULT _ValueToIntervalA(LPCSTR szInterval, FILETIME *pftStart, FILETIME *pftEnd)
{
    int i;
    int iBase;
    HRESULT hres = E_FAIL;
    SYSTEMTIME sysTime;
    unsigned int digits[RANGE_LEN];

    iBase = lstrlenA(szInterval)-RANGE_LEN;
    for (i = 0; i < RANGE_LEN; i++)
    {
        digits[i] = szInterval[i+iBase] - '0';
        if (digits[i] > 9) goto exitPoint;
    }

    ZeroMemory(&sysTime, sizeof(sysTime));
    sysTime.wYear = digits[0]*1000 + digits[1]*100 + digits[2] * 10 + digits[3];
    sysTime.wMonth = digits[4] * 10 + digits[5];
    sysTime.wDay = digits[6] * 10 + digits[7];
    if (!SystemTimeToFileTime(&sysTime, pftStart)) goto exitPoint;

    ZeroMemory(&sysTime, sizeof(sysTime));
    sysTime.wYear = digits[8]*1000 + digits[9]*100 + digits[10] * 10 + digits[11];
    sysTime.wMonth = digits[12] * 10 + digits[13];
    sysTime.wDay = digits[14] * 10 + digits[15];
    if (!SystemTimeToFileTime(&sysTime, pftEnd)) goto exitPoint;

    //  Intervals are open on the end, so end should be strictly > start
    if (CompareFileTime(pftStart, pftEnd) >= 0) goto exitPoint;

    hres = S_OK;

exitPoint:
    return hres;
}

HRESULT _ValueToIntervalW(LPCWSTR wzInterval, FILETIME *pftStart, FILETIME *pftEnd)
{
    CHAR szInterval[MAX_PATH];

    ASSERT(lstrlenW(wzInterval) < ARRAYSIZE(szInterval));
    UnicodeToAnsi(wzInterval, szInterval, ARRAYSIZE(szInterval));
    return _ValueToIntervalA((LPCSTR) szInterval, pftStart, pftEnd);
}

HRESULT CHistCacheFolder::_LoadIntervalCache()
{
    HRESULT hres;
    DWORD dwLastModified;
    DWORD dwValueIndex;
    DWORD dwPrefixIndex;
    HSFINTERVAL     *pIntervalCache = NULL;
    struct {
        INTERNET_CACHE_CONTAINER_INFOA cInfo;
        char szBuffer[MAX_PATH+MAX_PATH];
    } ContainerInfo;
    DWORD dwContainerInfoSize;
    CHAR chSave;
    HANDLE hContainerEnum;
    BOOL fContinue = TRUE;
    FILETIME ftNow;
    SYSTEMTIME st;
    DWORD dwOptions;

    GetLocalTime (&st);
    SystemTimeToFileTime(&st, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);

    dwLastModified = _dwIntervalCached;
    dwContainerInfoSize = sizeof(ContainerInfo);
    if (_pIntervalCache == NULL || CompareFileTime(&ftNow, &_ftDayCached))
    {
        dwOptions = 0;
    }
    else
    {
        dwOptions = CACHE_FIND_CONTAINER_RETURN_NOCHANGE;
    }
    hContainerEnum = FindFirstUrlCacheContainerA(&dwLastModified,
                            &ContainerInfo.cInfo,
                            &dwContainerInfoSize,
                            dwOptions);
    if (hContainerEnum == NULL)
    {
        DWORD err = GetLastError();

        if (err == ERROR_NO_MORE_ITEMS)
        {
            fContinue = FALSE;
        }
        else if (err == ERROR_INTERNET_NO_NEW_CONTAINERS)
        {
            hres = S_OK;
            goto exitPoint;
        }
        else
        {
            hres = HRESULT_FROM_WIN32(err);
            goto exitPoint;
        }
    }

    //  Guarantee we return S_OK we have _pIntervalCache even if we haven't
    //  yet created the interval registry keys.
    dwPrefixIndex = 0;
    dwValueIndex = TYPICAL_INTERVALS;
    pIntervalCache = (HSFINTERVAL *) LocalAlloc(LPTR, dwValueIndex*sizeof(HSFINTERVAL));
    if (!pIntervalCache)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }

    //  All of our intervals map to cache containers starting with
    //  c_szIntervalPrefix followed by YYYYMMDDYYYYMMDD
    while (fContinue)
    {
        chSave = ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN];
        ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN] = '\0';
        if (!StrCmpIA(ContainerInfo.cInfo.lpszName, c_szIntervalPrefix))
        {
            ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN] = chSave;
            DWORD dwCNameLen;

            if (dwPrefixIndex >= dwValueIndex)
            {
                HSFINTERVAL     *pIntervalCacheNew;

                pIntervalCacheNew = (HSFINTERVAL *) LocalReAlloc(pIntervalCache,
                    (dwValueIndex*2)*sizeof(HSFINTERVAL),
                    LMEM_ZEROINIT|LMEM_MOVEABLE);
                if (pIntervalCacheNew == NULL)
                {
                    hres = E_OUTOFMEMORY;
                    goto exitPoint;
                }
                pIntervalCache = pIntervalCacheNew;
                dwValueIndex *= 2;
            }

            dwCNameLen = lstrlenA(ContainerInfo.cInfo.lpszName);
            if (dwCNameLen <= INTERVAL_SIZE && dwCNameLen >= INTERVAL_MIN_SIZE &&
                lstrlenA(ContainerInfo.cInfo.lpszCachePrefix) == PREFIX_SIZE)
            {
                _SetVersion(&pIntervalCache[dwPrefixIndex], ContainerInfo.cInfo.lpszName);
                if (pIntervalCache[dwPrefixIndex].usVers != UNK_INTERVAL_VERS)
                {
                    AnsiToTChar(ContainerInfo.cInfo.lpszCachePrefix, pIntervalCache[dwPrefixIndex].szPrefix, ARRAYSIZE(pIntervalCache[dwPrefixIndex].szPrefix));
                    hres = _ValueToIntervalA( ContainerInfo.cInfo.lpszName,
                                             &pIntervalCache[dwPrefixIndex].ftStart,
                                             &pIntervalCache[dwPrefixIndex].ftEnd);
                    if (FAILED(hres)) goto exitPoint;
                    _SetValueSign(&pIntervalCache[dwPrefixIndex], ftNow);
                    dwPrefixIndex++;
                }
                else
                {
                    pIntervalCache[dwPrefixIndex].usVers = 0;
                }
            }
            //
            // HACK! IE5 bld 807 created containers with prefix length PREFIX_SIZE - 1.
            // Delete these entries so history shows up for anyone upgrading over this
            // build.  Delete this code!  (edwardp 8/8/98)
            //
            else if (dwCNameLen <= INTERVAL_SIZE && dwCNameLen >= INTERVAL_MIN_SIZE &&
                     lstrlenA(ContainerInfo.cInfo.lpszCachePrefix) == PREFIX_SIZE - 1)
            {
                DeleteUrlCacheContainerA(ContainerInfo.cInfo.lpszName, 0);
            }
        }
        dwContainerInfoSize = sizeof(ContainerInfo);
        fContinue = FindNextUrlCacheContainerA(hContainerEnum,
                            &ContainerInfo.cInfo,
                            &dwContainerInfoSize);
    }

    hres = S_OK;
    _dwIntervalCached = dwLastModified;
    _ftDayCached = ftNow;

    {
        ENTERCRITICAL;
        if (_pIntervalCache)
        {
            LocalFree(_pIntervalCache);
            _pIntervalCache = NULL;
        }
        _pIntervalCache = pIntervalCache;
        LEAVECRITICAL;
    }
    _cbIntervals = dwPrefixIndex;
    // because it will be freed by our destructor
    pIntervalCache  = NULL;

exitPoint:
    if (hContainerEnum) FindCloseUrlCache(hContainerEnum);
    if (pIntervalCache) LocalFree(pIntervalCache);
    return hres;
}

//  Returns true if *pftItem falls in the days *pftStart..*pftEnd inclusive
BOOL _InInterval(FILETIME *pftStart, FILETIME *pftEnd, FILETIME *pftItem)
{
    return (CompareFileTime(pftStart,pftItem) <= 0 && CompareFileTime(pftItem,pftEnd) < 0);
}

//  Truncates filetime increments beyond the day and then deltas by Days and converts back
//  to FILETIME increments
void _FileTimeDeltaDays(FILETIME *pftBase, FILETIME *pftNew, int Days)
{
    _int64 i64Base;

    i64Base = (((_int64)pftBase->dwHighDateTime) << 32) | pftBase->dwLowDateTime;
    i64Base /= FILE_SEC_TICKS;
    i64Base /= DAY_SECS;
    i64Base += Days;
    i64Base *= FILE_SEC_TICKS;
    i64Base *= DAY_SECS;
    pftNew->dwHighDateTime = (DWORD) ((i64Base >> 32) & 0xFFFFFFFF);
    pftNew->dwLowDateTime = (DWORD) (i64Base & 0xFFFFFFFF);
}

DWORD _DaysInInterval(HSFINTERVAL *pInterval)
{
    _int64 i64Start;
    _int64 i64End;

    i64Start = (((_int64)pInterval->ftStart.dwHighDateTime) << 32) | pInterval->ftStart.dwLowDateTime;
    i64Start /= FILE_SEC_TICKS;
    i64Start /= DAY_SECS;
    i64End = (((_int64)pInterval->ftEnd.dwHighDateTime) << 32) | pInterval->ftEnd.dwLowDateTime;
    i64End /= FILE_SEC_TICKS;
    i64End /= DAY_SECS;
    // NOTE: the lower bound is closed, upper is open (ie first tick of next day)
    return (DWORD) (i64End - i64Start);
}

//  Returns S_OK if found, S_FALSE if not, error on error
//  finds weekly interval in preference to daily if both exist
HRESULT CHistCacheFolder::_GetInterval(FILETIME *pftItem, BOOL fWeekOnly, HSFINTERVAL **ppInterval)
{
    HRESULT hres = E_FAIL;
    HSFINTERVAL *pReturn = NULL;
    int i;
    HSFINTERVAL *pDailyInterval = NULL;

    if (NULL == _pIntervalCache) goto exitPoint;

    for (i = 0; i < _cbIntervals; i ++)
    {
        if (_pIntervalCache[i].usVers == OUR_VERS)
        {
            if (_InInterval(&_pIntervalCache[i].ftStart,
                            &_pIntervalCache[i].ftEnd,
                            pftItem))
            {
                if (7 != _DaysInInterval(&_pIntervalCache[i]))
                {
                    if (!fWeekOnly)
                    {
                        pDailyInterval = &_pIntervalCache[i];
                    }
                    continue;
                }
                else
                {
                    pReturn = &_pIntervalCache[i];
                    hres = S_OK;
                    goto exitPoint;
                }
            }
        }
    }

    pReturn = pDailyInterval;
    hres = pReturn ? S_OK : S_FALSE;

exitPoint:
    if (ppInterval) *ppInterval = pReturn;
    return hres;
}

HRESULT CHistCacheFolder::_GetPrefixForInterval(LPCTSTR pszInterval, LPCTSTR *ppszCachePrefix)
{
    HRESULT hres = E_FAIL;
    int i;
    LPCTSTR pszReturn = NULL;
    FILETIME ftStart;
    FILETIME ftEnd;

    if (NULL == _pIntervalCache) goto exitPoint;

    hres = _ValueToInterval(pszInterval, &ftStart, &ftEnd);
    if (FAILED(hres)) goto exitPoint;

    for (i = 0; i < _cbIntervals; i ++)
    {
        if(_pIntervalCache[i].usVers == OUR_VERS)
        {
            if (CompareFileTime(&_pIntervalCache[i].ftStart,&ftStart) == 0 &&
                CompareFileTime(&_pIntervalCache[i].ftEnd,&ftEnd) == 0)
            {
                pszReturn = _pIntervalCache[i].szPrefix;
                hres = S_OK;
                break;
            }
        }
    }

    hres = pszReturn ? S_OK : S_FALSE;

exitPoint:
    if (ppszCachePrefix) *ppszCachePrefix = pszReturn;
    return hres;
}

void _KeyForInterval(HSFINTERVAL *pInterval, LPTSTR pszInterval, int cchInterval)
{
    SYSTEMTIME stStart;
    SYSTEMTIME stEnd;
    CHAR szVers[3];
#ifndef UNIX
    CHAR szTempBuff[MAX_PATH];
#else
    CHAR szTempBuff[INTERVAL_SIZE+1];
#endif

    ASSERT(pInterval->usVers!=UNK_INTERVAL_VERS && pInterval->usVers < 100);

    if (pInterval->usVers)
    {
        wnsprintfA(szVers, ARRAYSIZE(szVers), "%02lu", (ULONG) (pInterval->usVers));
    }
    else
    {
        szVers[0] = '\0';
    }
    FileTimeToSystemTime(&pInterval->ftStart, &stStart);
    FileTimeToSystemTime(&pInterval->ftEnd, &stEnd);
    wnsprintfA(szTempBuff, ARRAYSIZE(szTempBuff),
             "%s%s%04lu%02lu%02lu%04lu%02lu%02lu",
             c_szIntervalPrefix,
             szVers,
             (ULONG) stStart.wYear,
             (ULONG) stStart.wMonth,
             (ULONG) stStart.wDay,
             (ULONG) stEnd.wYear,
             (ULONG) stEnd.wMonth,
             (ULONG) stEnd.wDay);

    AnsiToTChar(szTempBuff, pszInterval, cchInterval);
}

LPITEMIDLIST CHistCacheFolder::_HostPidl(LPCTSTR pszHostUrl, HSFINTERVAL *pInterval)
{
    ASSERT(!_uViewType)
    LPITEMIDLIST pidlReturn;
    LPITEMIDLIST pidl;
    struct _HOSTIDL
    {
        USHORT cb;
        USHORT usSign;
#if defined(UNIX)
        TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+4];
#else
        TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
#endif
    } HostIDL;
    struct _INTERVALIDL
    {
        USHORT cb;
        USHORT usSign;
#if defined(UNIX)
        TCHAR szInterval[INTERVAL_SIZE+4];
#else
        TCHAR szInterval[INTERVAL_SIZE+1];
#endif
        struct _HOSTIDL hostIDL;
        USHORT cbTrail;
    } IntervalIDL;
    LPBYTE pb;
    USHORT cbSave;

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ZeroMemory(&IntervalIDL, sizeof(IntervalIDL));
    IntervalIDL.usSign = pInterval->usSign;
    _KeyForInterval(pInterval, IntervalIDL.szInterval, ARRAYSIZE(IntervalIDL.szInterval));
    IntervalIDL.cb = 2*sizeof(USHORT)+ (lstrlen(IntervalIDL.szInterval) + 1) * sizeof(TCHAR);

#if defined(UNIX)
    IntervalIDL.cb = ALIGN4(IntervalIDL.cb);
#endif

    pb = ((LPBYTE) (&IntervalIDL)) + IntervalIDL.cb;
    StrCpyN((LPTSTR)(pb+2*sizeof(USHORT)), pszHostUrl,
            (sizeof(IntervalIDL) - (IntervalIDL.cb + (3 * sizeof(USHORT)))) / sizeof(TCHAR));

    HostIDL.usSign = (USHORT)IDDPIDL_SIGN;
    HostIDL.cb = 2*sizeof(USHORT)+(lstrlen((LPTSTR)(pb+2*sizeof(USHORT))) + 1) * sizeof(TCHAR);

#if defined(UNIX)
    HostIDL.cb = ALIGN4(HostIDL.cb);
#endif

    memcpy(pb, &HostIDL, 2*sizeof(USHORT));
    *(USHORT *)(&pb[HostIDL.cb]) = 0;  // terminate the HostIDL ItemID

    pidlReturn = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
    pidl->mkid.cb = cbSave;
    return pidlReturn;
}

// Notify that an event has occured that affects a specific element in
//  history for special viewtypes
HRESULT CHistCacheFolder::_ViewType_NotifyEvent(IN LPITEMIDLIST pidlRoot,
                                                IN LPITEMIDLIST pidlHost,
                                                IN LPITEMIDLIST pidlPage,
                                                IN LONG         wEventId)
{
    HRESULT hRes = S_OK;

    ASSERT(pidlRoot && pidlHost && pidlPage);

    // VIEPWIDL_ORDER_TODAY
    LPITEMIDLIST pidlToday = _Combine_ViewPidl(VIEWPIDL_ORDER_TODAY, pidlPage);
    if (pidlToday) {
        LPITEMIDLIST pidlNotify = ILCombine(pidlRoot, pidlToday);
        if (pidlNotify) {
            SHChangeNotify(wEventId, SHCNF_IDLIST | CHANGE_FLAGS, pidlNotify, NULL);
            ILFree(pidlNotify);
        }
        ILFree(pidlToday);
    }

    // VIEWPIDL_ORDER_SITE
    LPITEMIDLIST pidlSite = _Combine_ViewPidl(VIEWPIDL_ORDER_SITE, pidlHost);
    if (pidlSite) {
        LPITEMIDLIST pidlSitePage = ILCombine(pidlSite, pidlPage);
        if (pidlSitePage) {
            LPITEMIDLIST pidlNotify = ILCombine(pidlRoot, pidlSitePage);
            if (pidlNotify) {
                SHChangeNotify(wEventId, SHCNF_IDLIST | CHANGE_FLAGS, pidlNotify, NULL);
                ILFree(pidlNotify);
            }
            ILFree(pidlSitePage);
        }
        ILFree(pidlSite);
    }

    return hRes;
}

LPCTSTR CHistCacheFolder::_GetLocalHost(void)
{
    if (!*_szLocalHost)
        ::_GetLocalHost(_szLocalHost, SIZECHARS(_szLocalHost));

    return _szLocalHost;
}

//  NOTE: modifies pszUrl.
HRESULT CHistCacheFolder::_NotifyWrite(LPTSTR pszUrl, int cchUrl, FILETIME *pftModified,  LPITEMIDLIST * ppidlSelect)
{
    HRESULT hres = S_OK;
    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;
    USHORT cbSave;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNotify;
    LPITEMIDLIST pidlTemp;
    LPITEMIDLIST pidlHost;
    LPHEIPIDL    phei = NULL;
    HSFINTERVAL *pInterval;
    FILETIME ftExpires = {0,0};
    BOOL fNewHost;
    LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszUrl);
    LPCTSTR pszHostUrl = pszStrippedUrl + HOSTPREFIXLEN;
    DWORD cchFree = cchUrl - (DWORD)(pszStrippedUrl-pszUrl);
    CHAR szAnsiUrl[MAX_URL_STRING];

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ///  Should also be able to get hitcount
    STATURL suThis;
    HRESULT hResLocal = E_FAIL;
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
    if (pUrlHistStg) {
        hResLocal = pUrlHistStg->QueryUrl(_StripHistoryUrlToUrl(pszUrl),
                                          STATURL_QUERYFLAG_NOURL, &suThis);
        pUrlHistStg->Release();
    }

    phei = _CreateHCacheFolderPidl(FALSE, pszUrl, *pftModified,
                                   (SUCCEEDED(hResLocal) ? &suThis : NULL), 0,
                                   _GetHitCount(_StripHistoryUrlToUrl(pszUrl)));

    if (SUCCEEDED(hResLocal) && suThis.pwcsTitle)
        OleFree(suThis.pwcsTitle);

    if (phei == NULL)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }

    if (cchFree <= HOSTPREFIXLEN)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }

    StrCpyN((LPTSTR)pszStrippedUrl, c_szHostPrefix, cchFree);    // whack on the PIDL!
    cchFree -= HOSTPREFIXLEN;

    _GetURLHostFromUrl(HCPidlToSourceUrl((LPCITEMIDLIST)phei),
                       (LPTSTR)pszHostUrl, cchFree, _GetLocalHost());

    //  BUGBUG chrisfra 4/9/97 we could take a small performance hit here and always
    //  update host entry.  this would allow us to efficiently sort domains by most
    //  recent access.

    fNewHost = FALSE;
    dwBuffSize = MAX_URLCACHE_ENTRY;
    SHTCharToAnsi(pszUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));

    if (!GetUrlCacheEntryInfoA(szAnsiUrl, NULL, 0))
    {
        fNewHost = TRUE;
        if (!CommitUrlCacheEntryA(szAnsiUrl, NULL, ftExpires, *pftModified,
                          URLHISTORY_CACHE_ENTRY|STICKY_CACHE_ENTRY,
                          NULL, 0, NULL, 0))
        {
            hres = HRESULT_FROM_WIN32(GetLastError());
        }
        if (FAILED(hres))
            goto exitPoint;
    }


    hres = _GetInterval(pftModified, FALSE, &pInterval);
    if (FAILED(hres))
        goto exitPoint;

    pidlTemp = _HostPidl(pszHostUrl, pInterval);
    if (pidlTemp == NULL)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }

    // Get just the host part of the pidl
    pidlHost = ILFindLastID(pidlTemp);
    ASSERT(pidlHost);

    if (fNewHost)
    {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST | CHANGE_FLAGS, pidlTemp, NULL);

        // We also need to notify special history views if they are listening:
        // For now, just "View by Site" is relevant...
        LPITEMIDLIST pidlViewSuffix = _Combine_ViewPidl(VIEWPIDL_ORDER_SITE, pidlHost);
        if (pidlViewSuffix) {
            LPITEMIDLIST pidlNotify = ILCombine(_pidl, pidlViewSuffix);
            if (pidlNotify) {
                SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST | CHANGE_FLAGS, pidlNotify, NULL);
                ILFree(pidlNotify);
            }
            ILFree(pidlViewSuffix);
        }
    }

    pidlNotify = ILCombine(pidlTemp, (LPITEMIDLIST) phei);
    if (pidlNotify == NULL)
    {
        ILFree(pidlTemp);
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }
    // Create (if its not there already) and Rename (if its there)
    //  Sending both notifys will be faster than trying to figure out
    //  which one is appropriate
    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST | CHANGE_FLAGS, pidlNotify, NULL);

    // Also notify events for specail viewpidls!
    _ViewType_NotifyEvent(_pidl, pidlHost, (LPITEMIDLIST)phei, SHCNE_CREATE);

    if (ppidlSelect)
    {
        *ppidlSelect = pidlNotify;
    }
    else
    {
        ILFree(pidlNotify);
    }

    ILFree(pidlTemp);
exitPoint:
    if (phei)
        LocalFree(phei);
    pidl->mkid.cb = cbSave;
    return hres;
}

HRESULT CHistCacheFolder::_NotifyInterval(HSFINTERVAL *pInterval, LONG lEventID)
{
    // special history views are not relevant here...
    if (_uViewType)
        return S_FALSE;

    USHORT cbSave = 0;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNotify = NULL;
    LPITEMIDLIST pidlNotify2 = NULL;
    LPITEMIDLIST pidlNotify3 = NULL;
    HRESULT hres = S_OK;
    struct _INTERVALIDL
    {
        USHORT cb;
        USHORT usSign;
#if defined(UNIX)
        TCHAR szInterval[INTERVAL_SIZE+4];
#else
        TCHAR szInterval[INTERVAL_SIZE+1];
#endif
        USHORT cbTrail;
    } IntervalIDL,IntervalIDL2;

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ZeroMemory(&IntervalIDL, sizeof(IntervalIDL));
    IntervalIDL.usSign = pInterval->usSign;
    _KeyForInterval(pInterval, IntervalIDL.szInterval, ARRAYSIZE(IntervalIDL.szInterval));
    IntervalIDL.cb = 2*sizeof(USHORT) + (lstrlen(IntervalIDL.szInterval) + 1)*sizeof(TCHAR);

#if defined(UNIX)
    IntervalIDL.cb = ALIGN4(IntervalIDL.cb);
#endif

    if (lEventID&SHCNE_RENAMEFOLDER ||  // was TODAY, now is a weekday
        (lEventID&SHCNE_RMDIR && 1 == _DaysInInterval(pInterval)) ) // one day, maybe TODAY
    {
        memcpy(&IntervalIDL2, &IntervalIDL, sizeof(IntervalIDL));
        IntervalIDL2.usSign = (USHORT)IDTPIDL_SIGN;
        pidlNotify2 = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
        pidlNotify = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL2));
        if (pidlNotify2 == NULL)
        {
            hres = E_OUTOFMEMORY;
            goto exitPoint;
        }
        if (lEventID&SHCNE_RMDIR)
        {
            pidlNotify3 = pidlNotify2;
            pidlNotify2 = NULL;
        }
    }
    else
    {
        pidlNotify = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
    }
    if (pidlNotify == NULL)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }
    SHChangeNotify(lEventID, SHCNF_IDLIST|CHANGE_FLAGS, pidlNotify, pidlNotify2);
    if (pidlNotify3) SHChangeNotify(lEventID, SHCNF_IDLIST|CHANGE_FLAGS, pidlNotify3, NULL);

exitPoint:
    ILFree(pidlNotify);
    ILFree(pidlNotify2);
    ILFree(pidlNotify3);
    if (cbSave) pidl->mkid.cb = cbSave;
    return hres;
}

HRESULT CHistCacheFolder::_CreateInterval(FILETIME *pftStart, DWORD dwDays)
{
    HSFINTERVAL interval;
    TCHAR szInterval[INTERVAL_SIZE+1];
    UINT err;
    FILETIME ftNow;
    SYSTEMTIME stNow;
    CHAR szIntervalAnsi[INTERVAL_SIZE+1], szCachePrefixAnsi[INTERVAL_SIZE+1];

#define CREATE_OPTIONS (INTERNET_CACHE_CONTAINER_AUTODELETE |  \
                        INTERNET_CACHE_CONTAINER_NOSUBDIRS  |  \
                        INTERNET_CACHE_CONTAINER_NODESKTOPINIT)

    //  _FileTimeDeltaDays guarantees times just at the 0th tick of the day
    _FileTimeDeltaDays(pftStart, &interval.ftStart, 0);
    _FileTimeDeltaDays(pftStart, &interval.ftEnd, dwDays);
    interval.usVers = OUR_VERS;
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);
    _SetValueSign(&interval, ftNow);

    _KeyForInterval(&interval, szInterval, ARRAYSIZE(szInterval));

    interval.szPrefix[0] = ':';
    StrCpyN(&interval.szPrefix[1], &szInterval[INTERVAL_PREFIX_LEN+INTERVAL_VERS_LEN],
            ARRAYSIZE(interval.szPrefix) - 1);
    StrCatBuff(interval.szPrefix, TEXT(": "), ARRAYSIZE(interval.szPrefix));

    SHTCharToAnsi(szInterval, szIntervalAnsi, ARRAYSIZE(szIntervalAnsi));
    SHTCharToAnsi(interval.szPrefix, szCachePrefixAnsi, ARRAYSIZE(szCachePrefixAnsi));

    if (CreateUrlCacheContainerA(szIntervalAnsi,   // Name
                                szCachePrefixAnsi, // CachePrefix
                                NULL,              // Path
                                0,                 // Cache Limit
                                0,                 // Container Type
                                CREATE_OPTIONS,    // Create Options
                                NULL,              // Create Buffer
                                0))                // Create Buffer size
    {
        _NotifyInterval(&interval, SHCNE_MKDIR);
        err = ERROR_SUCCESS;
    }
    else
    {
        err = GetLastError();
    }
    return ERROR_SUCCESS == err ? S_OK : HRESULT_FROM_WIN32(err);
}

HRESULT CHistCacheFolder::_PrefixUrl(LPCTSTR pszStrippedUrl,
                                     FILETIME *pftLastModifiedTime,
                                     LPTSTR pszPrefixedUrl,
                                     DWORD cchPrefixedUrl)
{
    HRESULT hres;
    HSFINTERVAL *pInterval;

    hres = _GetInterval(pftLastModifiedTime, FALSE, &pInterval);
    if (S_OK == hres)
    {
        if ((DWORD)((lstrlen(pszStrippedUrl) + lstrlen(pInterval->szPrefix) + 1) * sizeof(TCHAR)) > cchPrefixedUrl)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {
            StrCpyN(pszPrefixedUrl, pInterval->szPrefix, cchPrefixedUrl);
            StrCatBuff(pszPrefixedUrl, pszStrippedUrl, cchPrefixedUrl);
        }
    }
    return hres;
}


HRESULT CHistCacheFolder::_WriteHistory(LPCTSTR pszPrefixedUrl,
                                        FILETIME ftExpires,
                                        FILETIME ftModified,
                                        BOOL fSendNotify,
                                        LPITEMIDLIST * ppidlSelect)
{
    TCHAR szNewPrefixedUrl[INTERNET_MAX_URL_LENGTH+1];
    HRESULT hres = E_INVALIDARG;
    LPCTSTR pszUrlMinusContainer;

    pszUrlMinusContainer = _StripContainerUrlUrl(pszPrefixedUrl);

    if (pszUrlMinusContainer)
    {
        hres = _PrefixUrl(pszUrlMinusContainer,
                          &ftModified,
                          szNewPrefixedUrl,
                          ARRAYSIZE(szNewPrefixedUrl));
        if (S_OK == hres)
        {
            CHAR szAnsiUrl[MAX_URL_STRING+1];

            SHTCharToAnsi(szNewPrefixedUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));
            if (!CommitUrlCacheEntryA(
                          szAnsiUrl,
                          NULL,
                          ftExpires,
                          ftModified,
                          URLHISTORY_CACHE_ENTRY|STICKY_CACHE_ENTRY,
                          NULL,
                          0,
                          NULL,
                          0))
            {
                hres = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                if (fSendNotify) _NotifyWrite(szNewPrefixedUrl,
                                              ARRAYSIZE(szNewPrefixedUrl),
                                              &ftModified, ppidlSelect);
            }
        }
    }
    return hres;
}

// This function will update any shell that might be listening to us
//  to redraw the directory.
// It will do this by generating a SHCNE_UPDATE for all possible pidl roots
//  that the shell could have.  Hopefully, this should be sufficient...
// Specifically, this is meant to be called by ClearHistory.
HRESULT CHistCacheFolder::_ViewType_NotifyUpdateAll() {
    LPITEMIDLIST pidlHistory;
    if (SUCCEEDED(SHGetHistoryPIDL(&pidlHistory)))
    {
        for (USHORT us = 1; us <= VIEWPIDL_ORDER_MAX; ++us) {
            LPITEMIDLIST pidlView;
            if (SUCCEEDED(CreateSpecialViewPidl(us, &pidlView))) {
                LPITEMIDLIST pidlTemp = ILCombine(pidlHistory, pidlView);
                if (pidlTemp) {
                    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlTemp, NULL);
                    ILFree(pidlTemp);
                }
                ILFree(pidlView);
            }
        }
        ILFree(pidlHistory);
        SHChangeNotifyHandleEvents();
    }
    return S_OK;
}

//  On a per user basis.
//  BUGBUG: chrisfra 6/11/97. _DeleteItems of a Time Interval deletes the entire interval.
//  ClearHistory should probably work the same. Pros of _DeleteEntries is on non-profile,
//  multi-user machine, other user's history is preserved.  Cons is that on profile based
//  machine, empty intervals are created.
HRESULT CHistCacheFolder::ClearHistory()
{
    HRESULT hres = S_OK;
    int i;

    hres = _ValidateIntervalCache();
    if (SUCCEEDED(hres))
    {
        for (i = 0; i < _cbIntervals; i++)
        {
#if 0
            if (_DeleteEntries(_pIntervalCache[i].szPrefix, NULL, NULL))
                hres = S_FALSE;
            _NotifyInterval(&_pIntervalCache[i], SHCNE_UPDATEDIR);
#else
            _DeleteInterval(&_pIntervalCache[i]);
#endif
        }
    }
#ifndef UNIX
    _ViewType_NotifyUpdateAll();
#endif
    return hres;
}


//  ftModified is in "User Perceived", ie local time
//  stuffed into FILETIME as if it were UNC.  ftExpires is in normal UNC time.
HRESULT CHistCacheFolder::WriteHistory(LPCTSTR pszPrefixedUrl,
                                       FILETIME ftExpires,
                                       FILETIME ftModified,
                                       LPITEMIDLIST * ppidlSelect)
{
    HRESULT hres;

    hres = _ValidateIntervalCache();
    if (SUCCEEDED(hres))
    {
        hres = _WriteHistory(pszPrefixedUrl,
                    ftExpires,
                    ftModified,
                    TRUE,
                    ppidlSelect);
    }
    return hres;
}

//  Makes best efforts attempt to copy old style history items into new containers
HRESULT CHistCacheFolder::_CopyEntries(LPCTSTR pszHistPrefix)
{
    HANDLE              hEnum = NULL;
    HRESULT             hres;
    BOOL                fNotCopied = FALSE;
    LPINTERNET_CACHE_ENTRY_INFO pceiWorking;
    DWORD               dwBuffSize;
    LPTSTR              pszSearchPattern = NULL;
    TCHAR               szHistSearchPattern[65];    // search pattern for history items


    StrCpyN(szHistSearchPattern, pszHistPrefix, ARRAYSIZE(szHistSearchPattern));

    // BUGBUG: We can't pass in the whole search pattern that we want,
    // because FindFirstUrlCacheEntry is busted.  It will only look at the
    // prefix if there is a cache container for that prefix.  So, we can
    // pass in "Visited: " and enumerate all the history items in the cache,
    // but then we need to pull out only the ones with the correct username.

    // StrCpy(szHistSearchPattern, szUserName);

    pszSearchPattern = szHistSearchPattern;

    pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
    if (NULL == pceiWorking)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }
    hres = _ValidateIntervalCache();
    if (FAILED(hres)) goto exitPoint;

    while (SUCCEEDED(hres))
    {
        dwBuffSize = MAX_URLCACHE_ENTRY;
        if (!hEnum)
        {
            hEnum = FindFirstUrlCacheEntry(pszSearchPattern, pceiWorking, &dwBuffSize);
            if (!hEnum)
            {
                goto exitPoint;
            }
        }
        else if (!FindNextUrlCacheEntry(hEnum, pceiWorking, &dwBuffSize))
        {
            //  BUGBUG chrisfra 4/3/97 should we distinquish eod vs hard errors?
            //  old code for cachevu doesn't (see above in enum code)
            hres = S_OK;
            goto exitPoint;
        }

        if (SUCCEEDED(hres) &&
            ((pceiWorking->CacheEntryType & URLHISTORY_CACHE_ENTRY) == URLHISTORY_CACHE_ENTRY) &&
            _FilterPrefix(pceiWorking, (LPTSTR) pszHistPrefix))
        {
            hres = _WriteHistory(pceiWorking->lpszSourceUrlName,
                                 pceiWorking->ExpireTime,
                                 pceiWorking->LastModifiedTime,
                                 FALSE,
                                 NULL);
            if (S_FALSE == hres) fNotCopied = TRUE;
        }
    }
exitPoint:
    if (pceiWorking) LocalFree(pceiWorking);
    if (hEnum)
    {
        FindCloseUrlCache(hEnum);
    }
    return SUCCEEDED(hres) ? (fNotCopied ? S_FALSE : S_OK) : hres;
}

HRESULT CHistCacheFolder::_GetUserName(LPTSTR pszUserName, DWORD cchUserName)
{
    HRESULT hres = _EnsureHistStg();
    if (SUCCEEDED(hres))
    {
        hres = _pUrlHistStg->GetUserName(pszUserName, cchUserName);
    }
    return hres;
}


//  Makes best efforts attempt to delete old history items in container on a per
//  user basis.  if we get rid of per user - can just empty whole container
HRESULT CHistCacheFolder::_DeleteEntries(LPCTSTR pszHistPrefix, PFNDELETECALLBACK pfnDeleteFilter, LPVOID pDelData)
{
    HANDLE              hEnum = NULL;
    HRESULT             hres = S_OK;
    BOOL                fNotDeleted = FALSE;
    LPINTERNET_CACHE_ENTRY_INFO pceiWorking;
    DWORD               dwBuffSize;
    LPTSTR   pszSearchPattern = NULL;
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;   // len of this buffer
    TCHAR    szHistSearchPattern[PREFIX_SIZE+1];                 // search pattern for history items
    LPITEMIDLIST pidlNotify;

    StrCpyN(szHistSearchPattern, pszHistPrefix, ARRAYSIZE(szHistSearchPattern));
    if (FAILED(_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    // BUGBUG: We can't pass in the whole search pattern that we want,
    // because FindFirstUrlCacheEntry is busted.  It will only look at the
    // prefix if there is a cache container for that prefix.  So, we can
    // pass in "Visited: " and enumerate all the history items in the cache,
    // but then we need to pull out only the ones with the correct username.

    // StrCpy(szHistSearchPattern, szUserName);

    pszSearchPattern = szHistSearchPattern;

    pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
    if (NULL == pceiWorking)
    {
        hres = E_OUTOFMEMORY;
        goto exitPoint;
    }

    while (SUCCEEDED(hres))
    {
        dwBuffSize = MAX_URLCACHE_ENTRY;
        if (!hEnum)
        {
            hEnum = FindFirstUrlCacheEntry(pszSearchPattern, pceiWorking, &dwBuffSize);
            if (!hEnum)
            {
                goto exitPoint;
            }
        }
        else if (!FindNextUrlCacheEntry(hEnum, pceiWorking, &dwBuffSize))
        {
            //  BUGBUG chrisfra 4/3/97 should we distinquish eod vs hard errors?
            //  old code for cachevu doesn't (see above in enum code)
            hres = S_OK;
            goto exitPoint;
        }

        pidlNotify = NULL;
        if (SUCCEEDED(hres) &&
            ((pceiWorking->CacheEntryType & URLHISTORY_CACHE_ENTRY) == URLHISTORY_CACHE_ENTRY) &&
            _FilterUserName(pceiWorking, pszHistPrefix, szUserName) &&
            (NULL == pfnDeleteFilter || pfnDeleteFilter(pceiWorking, pDelData, &pidlNotify)))
        {
            //if (!DeleteUrlCacheEntryA(pceiWorking->lpszSourceUrlName))
            if (FAILED(_DeleteUrlFromBucket(pceiWorking->lpszSourceUrlName)))
            {
                fNotDeleted = TRUE;
            }
            else if (pidlNotify)
            {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST|CHANGE_FLAGS, pidlNotify, NULL);
            }
        }
        ILFree(pidlNotify);
    }
exitPoint:
    if (pceiWorking) LocalFree(pceiWorking);
    if (hEnum)
    {
        FindCloseUrlCache(hEnum);
    }
    return SUCCEEDED(hres) ? (fNotDeleted ? S_FALSE : S_OK) : hres;
}

HRESULT CHistCacheFolder::_DeleteInterval(HSFINTERVAL *pInterval)
{
    UINT err = NOERROR;
    TCHAR szInterval[INTERVAL_SIZE+1];
    CHAR szAnsiInterval[INTERVAL_SIZE+1];

    _KeyForInterval(pInterval, szInterval, ARRAYSIZE(szInterval));

    SHTCharToAnsi(szInterval, szAnsiInterval, ARRAYSIZE(szAnsiInterval));
    if (!DeleteUrlCacheContainerA(szAnsiInterval, 0))
    {
        err = GetLastError();
    }
    else
    {
        _NotifyInterval(pInterval, SHCNE_RMDIR);
    }
    return NOERROR == err ? S_OK : HRESULT_FROM_WIN32(err);
}

//  Returns S_OK if no intervals we're deleted, S_FALSE if at least
//  one interval was deleted.
HRESULT CHistCacheFolder::_CleanUpHistory(FILETIME ftLimit, FILETIME ftTommorrow)
{
    HRESULT hres;
    BOOL fChangedRegistry = FALSE;
    int i;

    //  _CleanUpHistory does two things:
    //
    //  If we have any stale weeks destroy them and flag the change
    //
    //  If we have any days that should be in cache but not in dailies
    //  copy them to the relevant week then destroy those days
    //  and flag the change

    hres = _LoadIntervalCache();
    if (FAILED(hres)) goto exitPoint;

    for (i = 0; i < _cbIntervals; i++)
    {
        //  Delete old intervals or ones which start at a day in the future
        //  (due to fooling with the clock)
        if (CompareFileTime(&_pIntervalCache[i].ftEnd, &ftLimit) < 0 ||
            CompareFileTime(&_pIntervalCache[i].ftStart, &ftTommorrow) >= 0)
        {
            fChangedRegistry = TRUE;
            hres = _DeleteInterval(&_pIntervalCache[i]);
            if (FAILED(hres)) goto exitPoint;
        }
        else if (1 == _DaysInInterval(&_pIntervalCache[i]))
        {
            HSFINTERVAL *pWeek;

            //  NOTE: at this point we have guaranteed, we've built weeks
            //  for all days outside of current week
            if (S_OK == _GetInterval(&_pIntervalCache[i].ftStart, TRUE, &pWeek))
            {
                fChangedRegistry = TRUE;
                hres = _CopyEntries(_pIntervalCache[i].szPrefix);
                if (FAILED(hres)) goto exitPoint;
                _NotifyInterval(pWeek, SHCNE_UPDATEDIR);

                hres = _DeleteInterval(&_pIntervalCache[i]);
                if (FAILED(hres)) goto exitPoint;
            }
        }
    }

exitPoint:
    if (S_OK == hres && fChangedRegistry) hres = S_FALSE;
    return hres;
}

typedef struct _HSFDELETEDATA
{
    UINT cidl;
    LPCITEMIDLIST *ppidl;
    LPCITEMIDLIST pidlParent;
} HSFDELETEDATA,*LPHSFDELETEDATA;

//  delete if matches any host on list
BOOL fDeleteInHostList(LPINTERNET_CACHE_ENTRY_INFO pceiWorking, LPVOID pDelData, LPITEMIDLIST *ppidlNotify)
{
    LPHSFDELETEDATA phsfd = (LPHSFDELETEDATA)pDelData;
    TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
    TCHAR szLocalHost[INTERNET_MAX_HOST_NAME_LENGTH+1];

    UINT i;

    _GetLocalHost(szLocalHost, SIZECHARS(szLocalHost));
    _GetURLHost(pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, szLocalHost);
    for (i = 0; i < phsfd->cidl; i++)
    {
        if (!StrCmpI(szHost, _GetURLTitle((LPCEIPIDL)(phsfd->ppidl[i]))))
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL fDeleteInLeafList(LPINTERNET_CACHE_ENTRY_INFO pceiWorking, LPVOID pDelData, LPITEMIDLIST *ppidlNotify)
{
    LPHSFDELETEDATA phsfd = (LPHSFDELETEDATA)pDelData;
    UINT i;

    for (i = 0; i < phsfd->cidl; i++)
    {
        if (!StrCmpI(pceiWorking->lpszSourceUrlName, HCPidlToSourceUrl(phsfd->ppidl[i])))
        {
            if (ppidlNotify)
            {
                *ppidlNotify = ILCombine(phsfd->pidlParent, phsfd->ppidl[i]);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// Will attempt to hunt down all occurrances of this url in any of the
//   various history buckets...
// This is a utility function for _ViewType_DeleteItems -- it may
//  be used in other contexts providing these preconditions
//  are kept in mind:
//
//   *The URL passed in should be prefixed ONLY with the username portion
//    such that this function can prepend prefixes to these urls
//   *WARNING: This function ASSUMES that _ValidateIntervalCache
//    has been called recently!!!!  DANGER DANGER!
//
// RETURNS: S_OK if at least one entry was found and deleted
//
HRESULT CHistCacheFolder::_DeleteUrlHistoryGlobal(LPCTSTR pszUrl) {
    HRESULT hRes = E_FAIL;
    if (pszUrl) {
        LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszUrl);
        IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
        if (pUrlHistStg) {
            UINT   cchwTempUrl  = lstrlen(pszStrippedUrl) + 1;
            LPWSTR pwszTempUrl = ((LPWSTR)LocalAlloc(LPTR, cchwTempUrl * sizeof(WCHAR)));
            if (pwszTempUrl)
            {
                SHTCharToUnicode(pszStrippedUrl, pwszTempUrl, cchwTempUrl);
                hRes = pUrlHistStg->DeleteUrl(pwszTempUrl, URLFLAG_DONT_DELETE_SUBSCRIBED);
                for (int i = 0; i < _cbIntervals; ++i) {
                    // should this length be constant? (bucket sizes shouldn't vary)
                    UINT   cchTempUrl   = (PREFIX_SIZE +
                                            lstrlen(pszUrl) + 1);
                    LPTSTR pszTempUrl = ((LPTSTR)LocalAlloc(LPTR, cchTempUrl * sizeof(TCHAR)));
                    if (pszTempUrl) {
                        // StrCpy null terminates
                        StrCpyN(pszTempUrl, _pIntervalCache[i].szPrefix, cchTempUrl);
                        StrCpyN(pszTempUrl + PREFIX_SIZE, pszUrl, cchTempUrl - PREFIX_SIZE);
                        if (DeleteUrlCacheEntry(pszTempUrl))
                            hRes = S_OK;
                        LocalFree(pszTempUrl);
                    }
                    else {
                        hRes = E_OUTOFMEMORY;
                        break;
                    }
                }
                LocalFree(pwszTempUrl);
                pwszTempUrl = NULL;
            }
            else {
                hRes = E_OUTOFMEMORY;
            }
            pUrlHistStg->Release();
        }
    }
    else
        hRes = E_INVALIDARG;
    return hRes;
}

// WARNING: assumes ppidl
HRESULT CHistCacheFolder::_ViewBySite_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    HRESULT hRes = E_INVALIDARG;
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    if (FAILED(_GetUserName(szUserName, ARRAYSIZE(szUserName))))
        szUserName[0] = TEXT('\0');

    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();

    if (pUrlHistStg)
    {
        IEnumSTATURL *penum;
        if (SUCCEEDED(pUrlHistStg->EnumUrls(&penum)) &&
            penum) {

            for (UINT i = 0; i < cidl; ++i)
            {
                //ASSERT(IS_VALID_CEIPIDL(ppidl[i]));
                LPCTSTR pszHostName  = _GetURLTitle((LPCEIPIDL)ppidl[i]);
                UINT    uUserNameLen = lstrlen(szUserName);
                UINT    uBuffLen     = (HOSTPREFIXLEN + uUserNameLen +
                                        lstrlen(pszHostName) + 2); // insert '@' and '\0'
                LPTSTR  pszUrl =
                    ((LPTSTR)LocalAlloc(LPTR, (uBuffLen) * sizeof(TCHAR)));
                if (pszUrl) {
                    // get rid of ":Host: " prefixed entires in the cache
                    // Generates "username@:Host: hostname" -- wnsprintf null terminates
                    wnsprintf(pszUrl, uBuffLen, TEXT("%s@%s%s"), szUserName,
                              c_szHostPrefix, pszHostName);
                    hRes = _DeleteUrlHistoryGlobal(pszUrl);

                    // enumerate over all urls in history

                    ULONG cFetched;
                    // don't retrieve TITLE information (too much overhead)
                    penum->SetFilter(NULL, STATURL_QUERYFLAG_NOTITLE);
                    STATURL statUrl;
                    statUrl.cbSize = sizeof(STATURL);
                    while(SUCCEEDED(penum->Next(1, &statUrl, &cFetched)) && cFetched) {
                        if (statUrl.pwcsUrl) {
                            // these next few lines painfully constructs a string
                            //  that is of the form "username@url"
                            LPTSTR pszStatUrlUrl;
                            UINT uStatUrlUrlLen = lstrlenW(statUrl.pwcsUrl);
                            pszStatUrlUrl = statUrl.pwcsUrl;
                            TCHAR  szHost[INTERNET_MAX_HOST_NAME_LENGTH + 1];
                            _GetURLHostFromUrl_NoStrip(pszStatUrlUrl, szHost, INTERNET_MAX_HOST_NAME_LENGTH + 1, _GetLocalHost());

                            if (!StrCmpI(szHost, pszHostName)) {
                                LPTSTR pszDelUrl; // url to be deleted
                                UINT uUrlLen = uUserNameLen + 1 + uStatUrlUrlLen; // +1 for '@'
                                pszDelUrl = ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + 1) * sizeof(TCHAR)));
                                if (pszDelUrl) {
                                    wnsprintf(pszDelUrl, uUrlLen + 1, TEXT("%s@%s"), szUserName, pszStatUrlUrl);
                                    // finally, delete all all occurrances of that URL in all history buckets
                                    hRes =  _DeleteUrlHistoryGlobal(pszDelUrl);

                                    // BUGBUG?:
                                    //  Is is really safe to delete *during* an enumeration like this, or should
                                    //  we cache all of the URLS and delete at the end?  I'd rather do it this
                                    //  way if possible -- anyhoo, no docs say its bad to do -- 'course there are no docs ;)
                                    //  Also, there is an example of code later that deletes during an enumeration
                                    //  and seems to work...
                                    LocalFree(pszDelUrl);
                                }
                                else
                                    hRes = E_OUTOFMEMORY;
                            }
                            OleFree(statUrl.pwcsUrl);
                        }
                    }
                    penum->Reset();
                    LocalFree(pszUrl);
                }
                else
                    hRes = E_OUTOFMEMORY;

                LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                if (pidlTemp) {
                    SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST|CHANGE_FLAGS, pidlTemp, NULL);
                    ILFree(pidlTemp);
                }
                else
                    hRes = E_OUTOFMEMORY;

                if (hRes == E_OUTOFMEMORY)
                    break;
            } // for
            penum->Release();
        } // if penum
        else
            hRes = E_FAIL;
        pUrlHistStg->Release();
    } // if purlHistStg
    else
        hRes = E_FAIL;

    return hRes;
}


// This guy will delete an URL from one history (MSHIST-type) bucket
//  and then try to find it in other (MSHIST-type) buckets.
//  If it can't be found, then the URL will be removed from the main
//  history (Visited-type) bucket.
// NOTE: Only the url will be deleted and not any of its "frame-children"
//       This is probably not the a great thing...
// ASSUMES that _ValidateIntervalCache has been called recently
HRESULT CHistCacheFolder::_DeleteUrlFromBucket(LPCTSTR pszPrefixedUrl) {
    HRESULT hRes = E_FAIL;
    if (DeleteUrlCacheEntry(pszPrefixedUrl)) {
        //   check if we need to delete this url from the main Visited container, too
        //   we make sure that url exists in at least one other bucket
        LPCTSTR pszUrl = _StripHistoryUrlToUrl(pszPrefixedUrl);
        DWORD  dwError = _SearchFlatCacheForUrl(pszUrl, NULL, NULL);
        if (dwError == ERROR_FILE_NOT_FOUND) {
            IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
            if (pUrlHistStg) {
                pUrlHistStg->DeleteUrl(pszUrl, 0);
                pUrlHistStg->Release();
                hRes = S_OK;
            }
            else
                hRes = E_FAIL;
        }
        else
            hRes = S_OK;
    }
    return hRes;
}

// Tries to delete as many as possible, and returns E_FAIL if the last one could not
//   be deleted.
// <RATIONALIZATION>not usually called with more than one pidl</RATIONALIZATION>
// ASSUMES that _ValidateIntervalCache has been called recently
HRESULT CHistCacheFolder::_ViewType_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    ASSERT(_uViewType);

    HRESULT hRes = E_INVALIDARG;

    if (ppidl) {
        switch(_uViewType) {
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0) {
                hRes = _ViewBySite_DeleteItems(ppidl, cidl);
                break;
            }
            ASSERT(_uViewDepth == 1);
            // FALLTHROUGH INTENTIONAL!!
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ: {
            for (UINT i = 0; i < cidl; ++i) {
                LPCTSTR pszPrefixedUrl = HCPidlToSourceUrl(ppidl[i]);
                if (pszPrefixedUrl) {
                    if (SUCCEEDED((hRes =
                        _DeleteUrlHistoryGlobal(_StripContainerUrlUrl(pszPrefixedUrl)))))
                    {
                        LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                        if (pidlTemp) {
                            SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST | CHANGE_FLAGS, pidlTemp, NULL);
                            ILFree(pidlTemp);
                        }
                        else
                            hRes = E_OUTOFMEMORY;
                    }
                }
                else
                    hRes = E_FAIL;
            }
            break;
        }
        case VIEWPIDL_ORDER_TODAY: {
            // find the entry in the cache and delete it:
            for (UINT i = 0; i < cidl; ++i)
            {
                if (_IsValid_HEIPIDL(ppidl[i]))
                {
                    hRes = _DeleteUrlFromBucket(HCPidlToSourceUrl(ppidl[i]));
                    if (SUCCEEDED(hRes))
                    {
                        LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                        if (pidlTemp)
                        {
                            SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST | CHANGE_FLAGS, pidlTemp, NULL);
                            ILFree(pidlTemp);
                        }
                        else
                            hRes = E_OUTOFMEMORY;
                    }
                }
                else
                    hRes = E_FAIL;
            }
            break;
        }
        default:
            hRes = E_NOTIMPL;
            ASSERT(0);
            break;
        }
    }
    return hRes;
}


HRESULT CHistCacheFolder::_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    HRESULT hres;
    UINT i;
    HSFDELETEDATA hsfDeleteData = {cidl, ppidl, _pidl};
    HSFINTERVAL *pDelInterval;
    FILETIME ftStart;
    FILETIME ftEnd;
    LPCTSTR pszIntervalName;

    hres = _ValidateIntervalCache();
    if (FAILED(hres)) goto exitPoint;

    if (_uViewType) {
        hres = _ViewType_DeleteItems(ppidl, cidl);
        goto exitPoint; // when in rome...
    }

    switch(_foldertype)
    {
    case FOLDER_TYPE_Hist:
        for (i = 0; i < cidl; i++)
        {
            pszIntervalName = _GetURLTitle((LPCEIPIDL)ppidl[i]);

            hres = _ValueToInterval(pszIntervalName, &ftStart, &ftEnd);
            if (FAILED(hres)) goto exitPoint;

            if (S_OK == _GetInterval(&ftStart, FALSE, &pDelInterval))
            {
                hres = _DeleteInterval(pDelInterval);
                if (FAILED(hres)) goto exitPoint;
            }
        }
        break;
    case FOLDER_TYPE_HistInterval:
        //  last id of of _pidl is name of interval, which implies start and end
        pszIntervalName = _GetURLTitle((LPCEIPIDL)ILFindLastID(_pidl));
        hres = _ValueToInterval(pszIntervalName, &ftStart, &ftEnd);
        if (FAILED(hres)) goto exitPoint;
        if (S_OK == _GetInterval(&ftStart, FALSE, &pDelInterval))
        {
            //  It's important to delete the host: <HOSTNAME> url's first so that
            //  an interleaved _NotityWrite() will not leave us inserting a pidl
            //  but the the host: directory.  it is a conscious performance tradeoff
            //  we're making here to not MUTEX this operation (rare) with _NotifyWrite
            for (i = 0; i < cidl; i++)
            {
                LPCTSTR pszHost;
                LPITEMIDLIST pidlTemp;
                TCHAR szNewPrefixedUrl[INTERNET_MAX_URL_LENGTH+1];
                TCHAR szUrlMinusContainer[INTERNET_MAX_URL_LENGTH+1];
                DWORD cbHost;
                DWORD cbUserName;

                pszHost = _GetURLTitle((LPCEIPIDL)ppidl[i]);
                cbHost = lstrlen(pszHost);

                //  Compose the prefixed URL for the host cache entry, then
                //  use it to delete host entry
                hres = _GetUserName(szUrlMinusContainer, ARRAYSIZE(szUrlMinusContainer));
                if (FAILED(hres)) goto exitPoint;
                cbUserName = lstrlen(szUrlMinusContainer);

                if ((cbHost + cbUserName + 1)*sizeof(TCHAR) + HOSTPREFIXLEN > INTERNET_MAX_URL_LENGTH)
                {
                    hres = E_FAIL;
                    goto exitPoint;
                }
                StrCatBuff(szUrlMinusContainer, TEXT("@"), ARRAYSIZE(szUrlMinusContainer));
                StrCatBuff(szUrlMinusContainer, c_szHostPrefix, ARRAYSIZE(szUrlMinusContainer));
                StrCatBuff(szUrlMinusContainer, pszHost, ARRAYSIZE(szUrlMinusContainer));
                hres = _PrefixUrl(szUrlMinusContainer,
                      &ftStart,
                      szNewPrefixedUrl,
                      ARRAYSIZE(szNewPrefixedUrl));
                if (FAILED(hres))
                    goto exitPoint;

                if (!DeleteUrlCacheEntry(szNewPrefixedUrl))
                {
                    hres = E_FAIL;
                    goto exitPoint;
                }
                pidlTemp = _HostPidl(pszHost, pDelInterval);
                if (pidlTemp == NULL)
                {
                    hres = E_OUTOFMEMORY;
                    goto exitPoint;
                }
                SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST|CHANGE_FLAGS, pidlTemp, NULL);
                ILFree(pidlTemp);
            }
            hres = _DeleteEntries(_pszCachePrefix , fDeleteInHostList, &hsfDeleteData);
        }
        break;
    case FOLDER_TYPE_HistDomain:
        for (i = 0; i < cidl; ++i)
        {
            if (_IsValid_HEIPIDL(ppidl[i]))
            {
                hres = _DeleteUrlFromBucket(HCPidlToSourceUrl(ppidl[i]));
                if (SUCCEEDED(hres))
                {
                    LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                    if (pidlTemp)
                    {
                        SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST | CHANGE_FLAGS,
                                       pidlTemp, NULL);
                        ILFree(pidlTemp);
                    }
                }
            }
            else
                hres = E_FAIL;
        }
        //hres = _DeleteEntries(_pszCachePrefix , fDeleteInLeafList, &hsfDeleteData);
        break;
    }
exitPoint:

    if (SUCCEEDED(hres))
        SHChangeNotifyHandleEvents();

    return hres;
}

IUrlHistoryPriv *CHistCacheFolder::_GetHistStg()
{
    _EnsureHistStg();
    if (_pUrlHistStg)
    {
        _pUrlHistStg->AddRef();
    }
    return _pUrlHistStg;
}

HRESULT CHistCacheFolder::_EnsureHistStg()
{
    HRESULT hres = S_OK;

    if (_pUrlHistStg == NULL)
    {
        hres = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryPriv, (void **)&_pUrlHistStg);
    }
    return hres;
}

HRESULT CHistCacheFolder::_ValidateIntervalCache()
{
    HRESULT hres = S_OK;
    SYSTEMTIME stNow;
    SYSTEMTIME stThen;
    FILETIME ftNow;
    FILETIME ftTommorrow;
    FILETIME ftMonday;
    FILETIME ftDayOfWeek;
    FILETIME ftLimit;
    BOOL fChangedRegistry = FALSE;
    DWORD dwWaitResult = WAIT_TIMEOUT;
    HSFINTERVAL *pWeirdWeek;
    HSFINTERVAL *pPrevDay;
    long compareResult;
    BOOL fCleanupVisitedDB = FALSE;
    int i;
    int daysToKeep;

    //  Check for reentrancy
    if (_fValidatingCache) return S_OK;

    _fValidatingCache = TRUE;

    if (g_hMutexHistory == NULL)
    {
        ENTERCRITICAL;

        if (g_hMutexHistory == NULL)
        {
            //
            // Use the "A" version for W95 compatability.
            //
            g_hMutexHistory = CreateMutexA(NULL, FALSE, "_!MSFTHISTORY!_");
            if (g_hMutexHistory == NULL && GetLastError() == ERROR_ALREADY_EXISTS)
            {
                g_hMutexHistory = OpenMutexA(SYNCHRONIZE, FALSE, "_!MSFTHISTORY!_");
            }
        }
        LEAVECRITICAL;
    }

    if (g_hMutexHistory) dwWaitResult = WaitForSingleObject(g_hMutexHistory, FAILSAFE_TIMEOUT);

    hres = _LoadIntervalCache();
    if (FAILED(hres)) goto exitPoint;

    //  All history is maintained using "User Perceived Time", which is the
    //  local time when navigate was made.
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);
    _FileTimeDeltaDays(&ftNow, &ftTommorrow, 1);

    hres = _EnsureHistStg();
    if (FAILED(hres))
        goto exitPoint;

    //  Compute ftLimit as first instant of first day to keep in history
    //  _FileTimeDeltaDays truncates to first FILETIME incr of day before computing
    //  earlier/later, day.
    daysToKeep = (int)_pUrlHistStg->GetDaysToKeep();
    if (daysToKeep < 0) daysToKeep = 0;
    _FileTimeDeltaDays(&ftNow, &ftLimit, 1-daysToKeep);

    FileTimeToSystemTime(&ftNow, &stThen);
    //  We take monday as day 0 of week, and adjust it for file time
    //  tics per day (100ns per tick
    _FileTimeDeltaDays(&ftNow, &ftMonday, stThen.wDayOfWeek ? 1-stThen.wDayOfWeek: -6);

    //  Delete old version intervals so prefix matching in wininet isn't hosed

    for (i = 0; i < _cbIntervals; i++)
    {
        if (_pIntervalCache[i].usVers < OUR_VERS)
        {
            fChangedRegistry = TRUE;
            hres = _DeleteInterval(&_pIntervalCache[i]);
            if (FAILED(hres)) goto exitPoint;
        }
    }

    //  If someone set their clock forward and then back, we could have
    //  a week that shouldn't be there.  delete it.  they will lose that week
    //  of history, c'est la guerre! quel domage!
    if (S_OK == _GetInterval(&ftMonday, TRUE, &pWeirdWeek))
    {
        hres = _DeleteInterval(pWeirdWeek);
        fCleanupVisitedDB = TRUE;
        if (FAILED(hres)) goto exitPoint;
        fChangedRegistry = TRUE;
    }

    //  Create weeks as needed to house days that are within "days to keep" limit
    //  but are not in the same week at today
    for (i = 0; i < _cbIntervals; i++)
    {
        FILETIME ftThisDay = _pIntervalCache[i].ftStart;
        if (_pIntervalCache[i].usVers >= OUR_VERS &&
            1 == _DaysInInterval(&_pIntervalCache[i]) &&
            CompareFileTime(&ftThisDay, &ftLimit) >= 0 &&
            CompareFileTime(&ftThisDay, &ftMonday) < 0)
        {
            if (S_OK != _GetInterval(&ftThisDay, TRUE, NULL))
            {
                int j;
                BOOL fProcessed = FALSE;
                FILETIME ftThisMonday;
                FILETIME ftNextMonday;

                FileTimeToSystemTime(&ftThisDay, &stThen);
                //  We take monday as day 0 of week, and adjust it for file time
                //  tics per day (100ns per tick
                _FileTimeDeltaDays(&ftThisDay, &ftThisMonday, stThen.wDayOfWeek ? 1-stThen.wDayOfWeek: -6);
                _FileTimeDeltaDays(&ftThisMonday, &ftNextMonday, 7);

                //  Make sure we haven't already done this week
                for (j = 0; j < i; j++)
                {
                     if (_pIntervalCache[j].usVers >= OUR_VERS &&
                         CompareFileTime(&_pIntervalCache[j].ftStart, &ftLimit) >= 0 &&
                        _InInterval(&ftThisMonday,
                                    &ftNextMonday,
                                    &_pIntervalCache[j].ftStart))
                    {
                         fProcessed = TRUE;
                         break;
                    }
                }
                if (!fProcessed)
                {
                    hres = _CreateInterval(&ftThisMonday, 7);
                    if (FAILED(hres)) goto exitPoint;
                    fChangedRegistry = TRUE;
                }
            }
        }
    }

    //  Guarantee today is created and old TODAY is renamed to Day of Week
    ftDayOfWeek = ftMonday;
    pPrevDay = NULL;
    while ((compareResult = CompareFileTime(&ftDayOfWeek, &ftNow)) <= 0)
    {
        HSFINTERVAL *pFound;

        if (S_OK != _GetInterval(&ftDayOfWeek, FALSE, &pFound))
        {
            if (0 == compareResult)
            {
                if (pPrevDay) // old today's name changes
                {
                    _NotifyInterval(pPrevDay, SHCNE_RENAMEFOLDER);
                }
                hres = _CreateInterval(&ftDayOfWeek, 1);
                if (FAILED(hres)) goto exitPoint;
                fChangedRegistry = TRUE;
            }
        }
        else
        {
            pPrevDay = pFound;
        }
        _FileTimeDeltaDays(&ftDayOfWeek, &ftDayOfWeek, 1);
    }

    //  On the first time through, we do not migrate history, wininet
    //  changed cache file format so users going to 4.0B2 from 3.0 or B1
    //  will lose their history anyway

    //  _CleanUpHistory does two things:
    //
    //  If we have any stale weeks destroy them and flag the change
    //
    //  If we have any days that should be in cache but not in dailies
    //  copy them to the relevant week then destroy those days
    //  and flag the change

    hres = _CleanUpHistory(ftLimit, ftTommorrow);

    if (S_FALSE == hres)
    {
        hres = S_OK;
        fChangedRegistry = TRUE;
        fCleanupVisitedDB = TRUE;
    }

    if (fChangedRegistry)
        hres = _LoadIntervalCache();

exitPoint:
    if (dwWaitResult == WAIT_OBJECT_0)
        ReleaseMutex(g_hMutexHistory);

    if (fCleanupVisitedDB)
    {
        if (SUCCEEDED(_EnsureHistStg()))
        {
            HRESULT hresLocal = _pUrlHistStg->CleanupHistory();
            ASSERT(SUCCEEDED(hresLocal));
        }
    }
    _fValidatingCache = FALSE;
    return hres;
}

HRESULT CHistCacheFolder::_CopyTSTRField(LPTSTR *ppszField, LPCTSTR pszValue)
{
    if (*ppszField)
    {
        LocalFree(*ppszField);
        *ppszField = NULL;
    }
    if (pszValue)
    {
        int cchField = lstrlen(pszValue) + 1;
        *ppszField = (LPTSTR)LocalAlloc(LPTR, cchField * sizeof(TCHAR));
        if (*ppszField)
        {
            StrCpyN(*ppszField, pszValue, cchField);
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

//////////////////////////////////
//
// IHistSFPrivate methods...
//
HRESULT CHistCacheFolder::SetCachePrefix(LPCTSTR pszCachePrefix)
{
    HRESULT hres;

    hres = _CopyTSTRField(&_pszCachePrefix, pszCachePrefix);
    return hres;
}

HRESULT CHistCacheFolder::SetDomain(LPCTSTR pszDomain)
{
    return _CopyTSTRField(&_pszDomain, pszDomain);
}


//////////////////////////////////
//
// IShellFolder methods...
//
HRESULT CHistCacheFolder::ParseDisplayName(HWND hwnd, LPBC pbc,
                        LPOLESTR lpszDisplayName, ULONG *pchEaten,
                        LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    //  BUGBUG 4/1/97 chrisfra - implement this
    TraceMsg(DM_HSFOLDER, "hcf - sf - ParseDisplayName() called.");
    *ppidl = NULL;              // null the out param
    return E_FAIL;
}


HRESULT CHistCacheFolder::EnumObjects(HWND hwnd, DWORD grfFlags,
                                      IEnumIDList **ppenumIDList)
{
    TraceMsg(DM_HSFOLDER, "hcf - sf - EnumObjects() called.");
    return CHistCacheFolderEnum_CreateInstance(grfFlags, this, ppenumIDList);
}

//////////////////////////////////////////////////////////////////////
//  BindToObject Methods:

HRESULT CHistCacheFolder::_ViewPidl_BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hres = E_FAIL;

    switch(((LPVIEWPIDL)pidl)->usViewType) {
    case VIEWPIDL_SEARCH:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_ORDER_SITE:
    case VIEWPIDL_ORDER_FREQ:

        CHistFolder *phsf = new CHistFolder(FOLDER_TYPE_HistDomain);
        if (phsf)
        {
            // initialize?
            phsf->_uViewType = ((LPVIEWPIDL)pidl)->usViewType;

            LPITEMIDLIST pidlLeft = ILCloneFirst(pidl);
            if (pidlLeft)
            {
                hres = S_OK;
                if (((LPVIEWPIDL)pidl)->usViewType == VIEWPIDL_SEARCH) 
                {
                    // find this search in the global database
                    phsf->_pcsCurrentSearch =
                        _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidl)->ftSearchKey);

                    // search not found -- do not proceed
                    if (!phsf->_pcsCurrentSearch)
                        hres = E_FAIL;
                }

                if (SUCCEEDED(hres)) 
                {
                    if (phsf->_pidl)
                        ILFree(phsf->_pidl);
                    phsf->_pidl = ILCombine(_pidl, pidlLeft);

                    LPCITEMIDLIST pidlNext = _ILNext(pidl);
                    if (pidlNext->mkid.cb) 
                    {
                        CHistFolder *phsf2;
                        hres = phsf->BindToObject(pidlNext, pbc, riid, (void **)&phsf2);
                        if (SUCCEEDED(hres))
                        {
                            phsf->Release();
                            phsf = phsf2;
                        }
                        else 
                        {
                            phsf->Release();
                            phsf = NULL;
                            break;
                        }
                    }
                    hres = phsf->QueryInterface(riid, ppv);
                }

                ILFree(pidlLeft);
            }
            ASSERT(phsf);
            phsf->Release();
        }
        else
            hres = E_OUTOFMEMORY;
        break;
    }

    
                            
    return hres;
}

HRESULT CHistCacheFolder::_ViewType_BindToObject(LPCITEMIDLIST pidl, LPBC pbc,
                                                 REFIID riid, void **ppv)
{
    HRESULT hres = E_FAIL;
    switch (_uViewType) 
    {
    case VIEWPIDL_ORDER_SITE:
        if (_uViewDepth++ < 1)
        {
            LPITEMIDLIST pidlNext = _ILNext(pidl);
            if (!(ILIsEmpty(pidlNext))) 
            {
                hres = BindToObject(pidlNext, pbc, riid, ppv);
            }
            else 
            {
                *ppv = (LPVOID)this;
                LPITEMIDLIST pidlOld = _pidl;
                if (pidlOld) 
                {
                    _pidl = ILCombine(_pidl, pidl);
                    ILFree(pidlOld);
                }
                else 
                {
                    _pidl = ILClone(pidl);
                }
                AddRef();
                hres = S_OK;
            }
        }
        break;

    case VIEWPIDL_ORDER_FREQ:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_SEARCH:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}

HRESULT CHistCacheFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    //
    // Align the pidl on a dword boundry.
    //

    BOOL fRealignedPidl;
    hr = AlignPidl(&pidl, &fRealignedPidl);

    if (SUCCEEDED(hr))
    {
        if (IS_VALID_VIEWPIDL(pidl)) 
        {
            hr = _ViewPidl_BindToObject(pidl, pbc, riid, ppv);
        }
        else if (_uViewType)
        {
            hr = _ViewType_BindToObject(pidl, pbc, riid, ppv);
        }
        else
        {
            FOLDER_TYPE FolderType = _foldertype;
            LPCITEMIDLIST pidlNext = pidl;

            while (pidlNext->mkid.cb != 0 && SUCCEEDED(hr))
            {
                LPHIDPIDL phid = (LPHIDPIDL)pidlNext;
                switch (FolderType)
                {
                case FOLDER_TYPE_Hist:
                    if (phid->usSign != IDIPIDL_SIGN && phid->usSign != IDTPIDL_SIGN)
                        hr = E_FAIL;
                    else
                        FolderType = FOLDER_TYPE_HistInterval;
                    break;

                case FOLDER_TYPE_HistDomain:
                    if (phid->usSign != HEIPIDL_SIGN)
                        hr = E_FAIL;
                    break;

                case FOLDER_TYPE_HistInterval:
                    if (phid->usSign != IDDPIDL_SIGN)
                        hr = E_FAIL;
                    else
                        FolderType = FOLDER_TYPE_HistDomain;
                    break;

                default:
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr))
                    pidlNext = _ILNext(pidlNext);
            }

            if (SUCCEEDED(hr))
            {
                CHistFolder *phsf = new CHistFolder(FolderType);
                if (phsf)
                {
                    //  If we're binding to a Domain from an Interval, pidl will not contain the
                    //  interval, so we've got to do a SetCachePrefix.
                    hr = phsf->SetCachePrefix(_pszCachePrefix);
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlNew = ILCombine(_pidl, pidl);
                        if (pidlNew)
                        {
                            hr = phsf->Initialize(pidlNew);
                            if (SUCCEEDED(hr))
                            {
                                hr = phsf->QueryInterface(riid, ppv);
                            }
                            ILFree(pidlNew);
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    phsf->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (fRealignedPidl)
            FreeRealignedPidl(pidl);
    }

    return hr;
}

HRESULT CHistCacheFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc,
                        REFIID riid, void **ppv)
{
    ASSERT(!_uViewType);
    TraceMsg(DM_HSFOLDER, "hcf - sf - BindToStorage() called.");
    *ppv = NULL;         // null the out param
    return E_NOTIMPL;
}

// A Successor to the IsLeaf
BOOL CHistCacheFolder::_IsLeaf()
{
    BOOL fRet = FALSE;

    switch(_uViewType) {
    case 0:
        fRet = IsLeaf(_foldertype);
        break;
    case VIEWPIDL_ORDER_FREQ:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_SEARCH:
        fRet = TRUE;
        break;
    case VIEWPIDL_ORDER_SITE:
        fRet = (_uViewDepth == 1);
        break;
    }
    return fRet;
}

//////////////////////////////////////////////////////////////////////
// CompareIDs

// coroutine for CompaireIDs -- makes recursive call
int CHistCacheFolder::_View_ContinueCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) {
    int iRet = 0;
    if ( (pidl1 = _ILNext(pidl1)) && (pidl2 = _ILNext(pidl2)) ) {
        BOOL fEmpty1 = ILIsEmpty(pidl1);
        BOOL fEmpty2 = ILIsEmpty(pidl2);
        if (fEmpty1 || fEmpty2) {
            if (fEmpty1 && fEmpty2)
                iRet = 0;
            else
                iRet = (fEmpty1 ? -1 : 1);
        }
        else {
            IShellFolder *psf;
            if (SUCCEEDED(BindToObject(pidl1, NULL, IID_IShellFolder, (void **)&psf)))
            {
                iRet = psf->CompareIDs(0, pidl1, pidl2);
                psf->Release();
            }
        }
    }
    return iRet;
}

int _CompareTitles(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet = 0;
    LPCTSTR pszTitle1 = _GetURLTitle((LPCEIPIDL)pidl1);
    LPCTSTR pszTitle2 = _GetURLTitle((LPCEIPIDL)pidl2);
    LPCTSTR pszUrl1   = _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl1));
    LPCTSTR pszUrl2   = _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl2));

    // CompareIDs has to check for equality, also -- two URLs are only equal when
    //   they have the same URL (not title)
    int iUrlCmp;
    if (!(iUrlCmp = StrCmpI(pszUrl1, pszUrl2)))
        iRet = 0;
    else {
        iRet = StrCmpI( (pszTitle1 ? pszTitle1 : pszUrl1),
                        (pszTitle2 ? pszTitle2 : pszUrl2) );

        // this says:  if two docs have the same Title, but different URL
        //             we then sort by url -- the last thing we want to do
        //             is return that they're equal!!  Ay Caramba!
        if (iRet == 0)
            iRet = iUrlCmp;
    }
    return iRet;
}


// unalligned verison

#if defined(UNIX) || !defined(_X86_)

UINT ULCompareFileTime(UNALIGNED const FILETIME *pft1, UNALIGNED const FILETIME *pft2)
{
    FILETIME tmpFT1, tmpFT2;
    CopyMemory(&tmpFT1, pft1, sizeof(tmpFT1));
    CopyMemory(&tmpFT2, pft2, sizeof(tmpFT1));
    return CompareFileTime( &tmpFT1, &tmpFT2 );
}

#else

#define ULCompareFileTime(pft1, pft2) CompareFileTime(pft1, pft2)

#endif


HRESULT CHistCacheFolder::_ViewType_CompareIDs(LPARAM lParam,
                                               LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ASSERT(_uViewType);

    int iRet = -1;

    if (pidl1 && pidl2)
    {
        switch (_uViewType) {
        case VIEWPIDL_ORDER_FREQ:
            ASSERT(_IsValid_HEIPIDL(pidl1) && _IsValid_HEIPIDL(pidl2));
            // need to strip because freq pidls are "Visited: " and
            //  all others come from our special bucket
            if (!_CompareHCURLs(pidl1, pidl2))
                iRet = 0;
            else
                iRet = ((((LPHEIPIDL)pidl2)->llPriority < ((LPHEIPIDL)pidl1)->llPriority) ? -1 : +1);
            break;
        case VIEWPIDL_SEARCH:
            iRet = _CompareTitles(pidl1, pidl2);
            break;
        case VIEWPIDL_ORDER_TODAY:  // view by order visited today
            {
                int iNameDiff;
                ASSERT(_IsValid_HEIPIDL(pidl1) && _IsValid_HEIPIDL(pidl2));
                // must do this comparison because CompareIDs is not only called for Sorting
                //  but to see if some pidls are equal

                if ((iNameDiff = _CompareHCURLs(pidl1, pidl2)) == 0)
                    iRet = 0;
                else
                {
                    iRet = ULCompareFileTime(&(((LPHEIPIDL)pidl2)->ftModified), &(((LPHEIPIDL)pidl1)->ftModified));
                    // if the file times are equal, they're still not the same url -- so
                    //   they have to be ordered on url
                    if (iRet == 0)
                        iRet = iNameDiff;
                }
                break;
            }
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0)
            {
                TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

                _GetURLDispName((LPCEIPIDL)pidl1, szName1, ARRAYSIZE(szName1));
                _GetURLDispName((LPCEIPIDL)pidl2, szName2, ARRAYSIZE(szName2));

                iRet = StrCmpI(szName1, szName2);
            }
            else if (_uViewDepth == 1) {
                iRet = _CompareTitles(pidl1, pidl2);
            }
            break;
        }
        if (iRet == 0)
            iRet = _View_ContinueCompare(pidl1, pidl2);
    }
    else {
        iRet = -1;
    }

    return ResultFromShort((SHORT)iRet);
}

HRESULT CHistCacheFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr;
    BOOL fRealigned1;
    BOOL fRealigned2;

    //
    // Make sure pidls are dword aligned.
    //

    hr = AlignPidl(&pidl1, &fRealigned1);

    if (SUCCEEDED(hr))
    {
        hr = AlignPidl(&pidl2, &fRealigned2);

        if (SUCCEEDED(hr))
        {
            hr = _CompareAlignedIDs(lParam, pidl1, pidl2);

            if (fRealigned2)
                FreeRealignedPidl(pidl2);
        }

        if (fRealigned1)
            FreeRealignedPidl(pidl1);
    }

    return hr;
}

HRESULT CHistCacheFolder::_CompareAlignedIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet = 0;
    USHORT usSign;
    FOLDER_TYPE FolderType = _foldertype;

    if (NULL == pidl1 || NULL == pidl2)
        return E_INVALIDARG;

    if (_uViewType)
    {
        return _ViewType_CompareIDs(lParam, pidl1, pidl2);
    }

    if (IS_VALID_VIEWPIDL(pidl1) && IS_VALID_VIEWPIDL(pidl2))
    {
        if ((((LPVIEWPIDL)pidl1)->usViewType == ((LPVIEWPIDL)pidl2)->usViewType) &&
            (((LPVIEWPIDL)pidl1)->usExtra    == ((LPVIEWPIDL)pidl2)->usExtra))
        {
            iRet = _View_ContinueCompare(pidl1, pidl2);
        }
        else
        {
            iRet = ((((LPVIEWPIDL)pidl1)->usViewType < ((LPVIEWPIDL)pidl2)->usViewType) ? -1 : 1);
        }
        goto exitPoint;
    }

    if (!IsLeaf(_foldertype))
    {
        //  We try to avoid unneccessary BindToObjs to compare partial paths
        usSign = FOLDER_TYPE_Hist == FolderType  ? IDIPIDL_SIGN : IDDPIDL_SIGN;
        while (TRUE)
        {
            LPCEIPIDL pceip1 = (LPCEIPIDL) pidl1;
            LPCEIPIDL pceip2 = (LPCEIPIDL) pidl2;

            if (pidl1->mkid.cb == 0 || pidl2->mkid.cb == 0)
            {
                iRet = pidl1->mkid.cb == pidl2->mkid.cb ? 0 : 1;
                goto exitPoint;
            }

            if (usSign == CEIPIDL_SIGN)
            {
                FolderType = FOLDER_TYPE_HistDomain;    // Treat it like history leaf
                break;
            }

            if (!_IsValid_IDPIDL(pidl1) || !_IsValid_IDPIDL(pidl2))
                return E_FAIL;

            if (!EQUIV_IDSIGN(pceip1->usSign,usSign) || !EQUIV_IDSIGN(pceip2->usSign,usSign))
                return E_FAIL;

            if (_foldertype == FOLDER_TYPE_HistInterval)
            {
                TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

                _GetURLDispName((LPCEIPIDL)pidl1, szName1, ARRAYSIZE(szName1));
                _GetURLDispName((LPCEIPIDL)pidl2, szName2, ARRAYSIZE(szName2));

                iRet = StrCmpI(szName1, szName2);
                goto exitPoint;
            }
            else
            {
                iRet = StrCmpI(_GetURLTitle((LPCEIPIDL)pidl1), _GetURLTitle((LPCEIPIDL)pidl2));
                if (iRet != 0)
                    goto exitPoint;
            }

            if (pceip1->usSign != pceip2->usSign)
            {
                iRet = -1;
                goto exitPoint;
            }

            pidl1 = _ILNext(pidl1);
            pidl2 = _ILNext(pidl2);
            if (IDIPIDL_SIGN == usSign)
            {
                usSign = IDDPIDL_SIGN;
            }
            else
            {
                usSign = CEIPIDL_SIGN;
            }
        }
    }

    //  At this point, both pidls have resolved to leaf (history or cache)

    if (IsHistory(FolderType))
    {
        LPHEIPIDL phei1 = _IsValid_HEIPIDL(pidl1);
        LPHEIPIDL phei2 = _IsValid_HEIPIDL(pidl2);
        if (!phei1 || !phei2)
            return E_FAIL;

        switch (lParam & SHCIDS_COLUMNMASK) {
        case ICOLH_URL_TITLE:
            {
                TCHAR szStr1[MAX_PATH], szStr2[MAX_PATH];
                _GetHistURLDispName(phei1, szStr1, ARRAYSIZE(szStr1));
                _GetHistURLDispName(phei2, szStr2, ARRAYSIZE(szStr2));

                iRet = StrCmpI(szStr1, szStr2);
            }
            break;

        case ICOLH_URL_NAME:
            iRet = _CompareHCFolderPidl(pidl1, pidl2);
            break;

        case ICOLH_URL_LASTVISITED:
            iRet = ULCompareFileTime(&((LPHEIPIDL)pidl2)->ftModified, &((LPHEIPIDL)pidl1)->ftModified);
            break;

        default:
            // The high bit on means to compare absolutely, ie: even if only filetimes
            // are different, we rule file pidls to be different

            if (lParam & SHCIDS_ALLFIELDS)
            {
                iRet = CompareIDs(ICOLH_URL_NAME, pidl1, pidl2);
                if (iRet == 0)
                {
                    iRet = CompareIDs(ICOLH_URL_TITLE, pidl1, pidl2);
                    if (iRet == 0)
                    {
                        iRet = CompareIDs(ICOLH_URL_LASTVISITED, pidl1, pidl2);
                    }
                }
            }
            else
            {
                iRet = -1;
            }
            break;
        }
    }
    else
    {
        if (!IS_VALID_CEIPIDL(pidl1) || !IS_VALID_CEIPIDL(pidl2))
            return E_FAIL;

        switch (lParam) {
        case ICOLC_URL_SHORTNAME:
            iRet = StrCmpI(_FindURLFileName(HCPidlToSourceUrl(pidl1)),
                            _FindURLFileName(HCPidlToSourceUrl(pidl2)));
            break;

        case ICOLC_URL_NAME:
            iRet = _CompareHCFolderPidl(pidl1, pidl2);
            break;

        case ICOLC_URL_TYPE:
            iRet = StrCmp(((LPCEIPIDL)pidl1)->szTypeName, ((LPCEIPIDL)pidl2)->szTypeName);
            break;

        case ICOLC_URL_SIZE:
            iRet = _CompareSize((LPCEIPIDL)pidl1, (LPCEIPIDL)pidl2);
            break;

        case ICOLC_URL_MODIFIED:
            iRet = ULCompareFileTime(&((LPCEIPIDL)pidl1)->cei.LastModifiedTime,
                                   &((LPCEIPIDL)pidl2)->cei.LastModifiedTime);
            break;

        case ICOLC_URL_ACCESSED:
            iRet = ULCompareFileTime(&((LPCEIPIDL)pidl1)->cei.LastAccessTime,
                                   &((LPCEIPIDL)pidl2)->cei.LastAccessTime);
            break;

        case ICOLC_URL_EXPIRES:
            iRet = ULCompareFileTime(&((LPCEIPIDL)pidl1)->cei.ExpireTime,
                                   &((LPCEIPIDL)pidl2)->cei.ExpireTime);
            break;

        case ICOLC_URL_LASTSYNCED:
            iRet = ULCompareFileTime(&((LPCEIPIDL)pidl1)->cei.LastSyncTime,
                                   &((LPCEIPIDL)pidl2)->cei.LastSyncTime);
            break;

        default:
            iRet = -1;
        }
    }
exitPoint:

    return ResultFromShort((SHORT)iRet);
}


HRESULT CHistCacheFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hres;

    *ppv = NULL;

    if (riid == IID_IShellView)
    {
        ASSERT(!_uViewType);
        hres = HistCacheFolderView_CreateInstance(this, _pidl, ppv);
    }
    else if (riid == IID_IContextMenu)
    {
        hres = QueryInterface(riid, ppv);   // hack!
    }
    else if (riid == IID_IShellDetails)
    {
        CDetailsOfFolder *p = new CDetailsOfFolder(hwnd, this);
        if (p)
        {
            hres = p->QueryInterface(riid, ppv);
            p->Release();
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else
    {
        hres = E_NOINTERFACE;
    }

    return hres;
}

HRESULT CHistCacheFolder::_ViewType_GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    ASSERT(_uViewType);

    if (!prgfInOut || !apidl)
        return E_INVALIDARG;

    HRESULT hres       = S_OK;
    int     cGoodPidls = 0;

    if (*prgfInOut & SFGAO_VALIDATE) {
        for (UINT u = 0; SUCCEEDED(hres) && (u < cidl); ++u) {
            switch(_uViewType) {
            case VIEWPIDL_ORDER_TODAY: {
                _EnsureHistStg();
                if (_IsValid_HEIPIDL(apidl[u]) &&
                    SUCCEEDED(
                              _pUrlHistStg->QueryUrl
                              (_StripHistoryUrlToUrl(HCPidlToSourceUrl(apidl[u])),
                               STATURL_QUERYFLAG_NOURL,
                               NULL)
                             ))
                {
                    ++cGoodPidls;
                }
                else
                    hres = E_FAIL;
                break;
            }
            case VIEWPIDL_SEARCH:
            case VIEWPIDL_ORDER_FREQ: {
                // this is a temporary fix to get the behaviour of the namespace
                //  control correct -- the long-term fix involves cacheing a
                //  generated list of these items and validating that list
                break;
            }
            case VIEWPIDL_ORDER_SITE: {
                ASSERT(_uViewDepth == 1);
                _ValidateIntervalCache();
                if (_SearchFlatCacheForUrl(_StripHistoryUrlToUrl(HCPidlToSourceUrl(apidl[u])),
                                           NULL, NULL) == ERROR_SUCCESS)
                {
                    ++cGoodPidls;
                }
                else
                    hres = E_FAIL;
                break;
            }
            default:
                hres = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hres)) {
        if (_IsLeaf())
            *prgfInOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET;
        else
            *prgfInOut = SFGAO_FOLDER;
    }

    return hres;
}

// Right now, we will allow TIF Drag in Browser Only, even though
// it will not be Zone Checked at the Drop.
//#define BROWSERONLY_NOTIFDRAG

HRESULT CHistCacheFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                        ULONG * prgfInOut)
{
    ULONG rgfInOut;
    HRESULT hr;
    FOLDER_TYPE FolderType = _foldertype;

    // Make sure each pidl in the array is dword aligned.

    BOOL fRealigned;
    hr = AlignPidlArray(apidl, cidl, &apidl, &fRealigned);

    if (SUCCEEDED(hr))
    {

        // For view types, we'll map FolderType to do the right thing...
        if (_uViewType)
        {
            hr = _ViewType_GetAttributesOf(cidl, apidl, prgfInOut);
        }
        else
        {
            switch (FolderType)
            {
            case FOLDER_TYPE_Cache:
                rgfInOut = SFGAO_FILESYSTEM | SFGAO_CANDELETE | SFGAO_HASPROPSHEET;

        #ifdef BROWSERONLY_NOTIFDRAG
                if (PLATFORM_INTEGRATED == WhichPlatform())
        #endif // BROWSERONLY_NOTIFDRAG
                {
                    SetFlag(rgfInOut, SFGAO_CANCOPY);
                }
                break;

            case FOLDER_TYPE_Hist:
                rgfInOut = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
                break;

            case FOLDER_TYPE_HistInterval:
                rgfInOut = SFGAO_FOLDER;
                break;

            case FOLDER_TYPE_HistDomain:

                {
                    UINT cGoodPidls;

                    if (SFGAO_VALIDATE & *prgfInOut)
                    {
                        cGoodPidls = 0;
                        if (SUCCEEDED(_EnsureHistStg()))
                        {
                            for (UINT i = 0; i < cidl; i++)
                            {
                                //  NOTE: QueryUrlA checks for NULL URL and returns E_INVALIDARG
                                if (!_IsValid_HEIPIDL(apidl[i]) ||
                                    FAILED(_pUrlHistStg->QueryUrl(
                                                _StripHistoryUrlToUrl(HCPidlToSourceUrl(apidl[i])),
                                                STATURL_QUERYFLAG_NOURL,
                                                NULL))
                                    )
                                {
                                    break;
                                }
                                cGoodPidls++;
                            }
                        }
                    }
                    else
                        cGoodPidls = cidl;


                    if (cidl == cGoodPidls)
                    {
                        rgfInOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET;
                        break;
                    }
                    // FALL THROUGH INTENDED!
                }

            default:
                rgfInOut =  0;
                hr = E_FAIL;
                break;
            }

            // all items can be deleted
            if (SUCCEEDED(hr))
                rgfInOut |= SFGAO_CANDELETE;
            *prgfInOut = rgfInOut;
        }

        if (fRealigned)
            FreeRealignedPidlArray(apidl, cidl);
    }

    return hr;
}

HRESULT _GetShortcut(LPCTSTR pszUrl, REFIID riid, void **ppv)
{
    IUniformResourceLocator *purl;
    HRESULT hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator, (void **)&purl);

    if (SUCCEEDED(hr))
    {

        hr = purl->SetURL(pszUrl, TRUE);

        if (SUCCEEDED(hr))
            hr = purl->QueryInterface(riid, ppv);

        purl->Release();
    }

    return hr;
}

HRESULT CHistCacheFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                        REFIID riid, UINT * prgfInOut, void **ppv)
{
    HRESULT hr;
    *ppv = NULL;         // null the out param

    // Make sure all pidls in the array are dword aligned.

    BOOL fRealigned;
    hr = AlignPidlArray(apidl, cidl, &apidl, &fRealigned);

    if (SUCCEEDED(hr))
    {
        if ((riid == IID_IShellLinkA ||
                  riid == IID_IShellLinkW ||
                  riid == IID_IExtractIconA ||
                  riid == IID_IExtractIconW ||
                  (riid == IID_IQueryInfo && !IsHistory(_foldertype))) &&
                 _IsLeaf())
        {
            LPCTSTR pszURL = HCPidlToSourceUrl(apidl[0]);

            if (IsHistory(_foldertype))
                pszURL = _StripHistoryUrlToUrl(pszURL);

            hr = _GetShortcut(pszURL, riid, ppv);
       }
        else if ((riid == IID_IContextMenu) ||
            (riid == IID_IDataObject) ||
            (riid == IID_IExtractIconA) ||
            (riid == IID_IExtractIconW) ||
            (riid == IID_IQueryInfo && IsHistory(_foldertype)))
        {
            hr = CHistCacheItem_CreateInstance(this, hwnd, cidl, apidl, riid, ppv);
        }
        else
        {
            hr = E_FAIL;
        }

        if (fRealigned)
            FreeRealignedPidlArray(apidl, cidl);
    }

    return hr;
}

HRESULT CHistCacheFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
    {
        if (_uViewType == 0 && _foldertype == FOLDER_TYPE_HistDomain)
            *pSort = ICOLH_URL_TITLE;
        else
            *pSort = 0;
    }

    if (pDisplay)
    {
        if (_uViewType == 0 && _foldertype == FOLDER_TYPE_HistDomain)
            *pDisplay = ICOLH_URL_TITLE;
        else
            *pDisplay = 0;
    }
    return S_OK;
}

extern LPCTSTR _FindURLFileName(LPCTSTR pszURL);

LPCTSTR _GetDisplayUrlForPidl(LPCITEMIDLIST pidl, LPTSTR pszDisplayUrl, DWORD dwDisplayUrl)
{
    LPCTSTR pszUrl = _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl));
    if (pszUrl && PrepareURLForDisplay(pszUrl, pszDisplayUrl, &dwDisplayUrl))
    {
        pszUrl = pszDisplayUrl;
    }
    return pszUrl;
}

LPCTSTR _GetUrlForPidl(LPCITEMIDLIST pidl)
{
    LPCTSTR pszUrl = _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl));
    
    return pszUrl ? pszUrl : TEXT("");
}


HRESULT CHistCacheFolder::_GetInfoTip(LPCITEMIDLIST pidl, DWORD dwFlags, WCHAR **ppwszTip)
{
    HRESULT hres;
    TCHAR szTip[MAX_URL_STRING + 100], szPart2[MAX_URL_STRING];

    szTip[0] = szPart2[0] = 0;

    FOLDER_TYPE FolderType = _foldertype;

    // For special views, map FolderType to do the right thing
    if (_uViewType)
    {
        switch(_uViewType) {
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ:
        case VIEWPIDL_ORDER_TODAY:
            FolderType = FOLDER_TYPE_HistDomain;
            break;
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0)
                FolderType = FOLDER_TYPE_HistInterval;
            else
                FolderType = FOLDER_TYPE_HistDomain;
            break;
        }
    }

    switch (FolderType)
    {
    case FOLDER_TYPE_HistDomain:
        {
            _GetHistURLDispName((LPHEIPIDL)pidl, szTip, ARRAYSIZE(szTip));
            DWORD cchPart2 = ARRAYSIZE(szPart2);
            PrepareURLForDisplayUTF8(_GetUrlForPidl(pidl), szPart2, &cchPart2, TRUE);
        }
        break;


    case FOLDER_TYPE_Hist:
        {
            FILETIME ftStart, ftEnd;
            LPCTSTR pszIntervalName = _GetURLTitle((LPCEIPIDL)pidl);

            if (SUCCEEDED(_ValueToInterval(pszIntervalName, &ftStart, &ftEnd)))
            {
                GetTooltipForTimeInterval(&ftStart, &ftEnd, szTip, ARRAYSIZE(szTip));
            }
            break;
        }

    case FOLDER_TYPE_HistInterval:
        {
            TCHAR szFmt[64];

            MLLoadString(IDS_SITETOOLTIP, szFmt, ARRAYSIZE(szFmt));
            wnsprintf(szTip, ARRAYSIZE(szTip), szFmt, _GetURLTitle((LPCEIPIDL)pidl));
            break;
        }
    }

    if (szTip[0])
    {
        if (szPart2[0])
        {
            StrCatBuff(szTip, TEXT("\n\r"), ARRAYSIZE(szTip));
            StrCatBuff(szTip, szPart2, ARRAYSIZE(szTip));
        }
        hres = SHStrDup(szTip, ppwszTip);
    }
    else
    {
        hres = E_FAIL;
        *ppwszTip = NULL;
    }

    return hres;
}

BOOL _TitleIsGood(LPCWSTR psz)
{
    DWORD scheme = GetUrlScheme(psz);
    return (!PathIsFilePath(psz) && (URL_SCHEME_INVALID == scheme || URL_SCHEME_UNKNOWN == scheme));
}


void CHistCacheFolder::_GetHistURLDispName(LPHEIPIDL phei, LPTSTR pszStr, UINT cchStr)
{
    *pszStr = 0;

    if ((phei->usFlags & HISTPIDL_VALIDINFO) && phei->usTitle)
    {
        StrCpyN(pszStr, (LPTSTR)((BYTE*)phei + phei->usTitle), cchStr);
    }
    else if (SUCCEEDED(_EnsureHistStg()))
    {
        LPCTSTR pszUrl = _StripHistoryUrlToUrl(HCPidlToSourceUrl((LPCITEMIDLIST)phei));
        if (pszUrl)
        {
            STATURL suThis;
            if (SUCCEEDED(_pUrlHistStg->QueryUrl(pszUrl, STATURL_QUERYFLAG_NOURL, &suThis)) && suThis.pwcsTitle)
            {
                // sometimes the URL is stored in the title
                // avoid using those titles.
                if (_TitleIsGood(suThis.pwcsTitle))
                    SHUnicodeToTChar(suThis.pwcsTitle, pszStr, cchStr);

                OleFree(suThis.pwcsTitle);
            }

            //  if we havent got anything yet
            if (!*pszStr) 
            {
                SHELLSTATE ss;
                SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
                StrCpyN(pszStr, _FindURLFileName(pszUrl), cchStr);

                DWORD cchBuf = cchStr;
                PrepareURLForDisplayUTF8(pszStr, pszStr, &cchBuf, TRUE);

                if (!ss.fShowExtensions)
                    PathRemoveExtension(pszStr);
            }
        }
    }
}

HRESULT CHistCacheFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *lpName)
{
    TCHAR szTemp[MAX_URL_STRING];

    szTemp[0] = 0;

    // Make sure the pidl is dword aligned.

    BOOL fRealigned;

    if (SUCCEEDED(AlignPidl(&pidl, &fRealigned)))
    {
        if (IS_VALID_VIEWPIDL(pidl))
        {
            UINT idRsrc;
            switch(((LPVIEWPIDL)pidl)->usViewType) {
            case VIEWPIDL_ORDER_SITE:  idRsrc = IDS_HISTVIEW_SITE;      break;
            case VIEWPIDL_ORDER_TODAY: idRsrc = IDS_HISTVIEW_TODAY;     break;
            case VIEWPIDL_ORDER_FREQ:
            default:
                idRsrc = IDS_HISTVIEW_FREQUENCY; break;
            }

            MLLoadString(idRsrc, szTemp, ARRAYSIZE(szTemp));
        }
        else
        {
            if (_uViewType  == VIEWPIDL_ORDER_SITE &&
                _uViewDepth  == 0)
            {
                _GetURLDispName((LPCEIPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
            }
            else if (_IsLeaf())
            {
                LPCTSTR pszTitle = _GetURLTitle((LPCEIPIDL)pidl);
                BOOL fDoUnescape;  

                // _GetURLTitle could return the real title or just an URL.
                // We use _URLTitleIsURL to make sure we don't unescape any titles.

                if (pszTitle && *pszTitle)
                {
                    StrCpyN(szTemp, pszTitle, ARRAYSIZE(szTemp));
                    fDoUnescape = _URLTitleIsURL((LPCEIPIDL)pidl);
                }
                else
                {
                    LPCTSTR pszUrl = _StripHistoryUrlToUrl(HCPidlToSourceUrl(pidl));
                    if (pszUrl) 
                        StrCpyN(szTemp, pszUrl, ARRAYSIZE(szTemp));
                    fDoUnescape = TRUE;
                }
                
                if (fDoUnescape)
                {
                    DWORD cchBuf = ARRAYSIZE(szTemp);
                    PrepareURLForDisplayUTF8(szTemp, szTemp, &cchBuf, TRUE);
                    
                    SHELLSTATE ss;
                    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
                    
                    if (!ss.fShowExtensions)
                        PathRemoveExtension(szTemp);
                }
            }
            else
            {
                // for the history, we'll use the title if we have one, otherwise we'll use
                // the url filename.
                switch (_foldertype)
                {
                case FOLDER_TYPE_HistDomain:
                    _GetHistURLDispName((LPHEIPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
                    break;

                case FOLDER_TYPE_Hist:
                    {
                        FILETIME ftStart, ftEnd;

                        _ValueToInterval(_GetURLTitle((LPCEIPIDL)pidl), &ftStart, &ftEnd);
                        GetDisplayNameForTimeInterval(&ftStart, &ftEnd, szTemp, ARRAYSIZE(szTemp));
                    }
                    break;

                case FOLDER_TYPE_HistInterval:
                    _GetURLDispName((LPCEIPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
                    break;

                case FOLDER_TYPE_Cache:
                    _GetCacheItemTitle((LPCEIPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
                    break;
                }
            }
        }

        if (fRealigned)
            FreeRealignedPidl(pidl);
    }

    return StringToStrRet(szTemp, lpName);
}

HRESULT CHistCacheFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
                        LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;               // null the out param
    return E_FAIL;
}

//////////////////////////////////
//
// IShellIcon Methods...
//
HRESULT CHistCacheFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex)
{
    SHFILEINFO sFI;
    LPCTSTR pszIconFile;

    TraceMsg(DM_HSFOLDER, "hcf - si - GetIconOf() called.");

    switch (_foldertype)
    {
    case FOLDER_TYPE_Cache:
        {
            BOOL fRealigned;

            if (SUCCEEDED(AlignPidl(&pidl, &fRealigned)))
            {
                pszIconFile = CEI_LOCALFILENAME((LPCEIPIDL)pidl);

                if (fRealigned)
                    FreeRealignedPidl(pidl);
            }
        }
        break;

    default:
        //  Force Use of IExtractIcon and use special icon
        return S_FALSE;
        break;
    }

    if (SHGetFileInfo(pszIconFile, 0, &sFI, sizeof(sFI),
        SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_SMALLICON))
    {
        *lpIconIndex = sFI.iIcon;
        return NOERROR;
    }
    return E_FAIL;
}



//////////////////////////////////
//
// IPersistFolder Methods...
//
HRESULT CHistCacheFolder::GetClassID(CLSID *pclsid)
{
    TraceMsg(DM_HSFOLDER, "hcf - pf - GetClassID() called.");

    // NOTE: Need to split cases here.
    IsHistory(_foldertype) ? *pclsid = CLSID_HistFolder : *pclsid = CLSID_CacheFolder;
    return S_OK;
}

HRESULT CHistCacheFolder::Initialize(LPCITEMIDLIST pidlInit)
{
    HRESULT hres = S_OK;

    TraceMsg(DM_HSFOLDER, "hcf - pf - Initialize() called.");

    if (_pidl)
    {
        ILFree(_pidl);
    }

    _pidl = ILClone(pidlInit);
    if (!_pidl)
    {
        hres = E_OUTOFMEMORY;
    }
    else if (IsHistory(_foldertype))
    {
        hres = _ExtractInfoFromPidl();
    }
    return hres;
}

//////////////////////////////////
//
// IPersistFolder2 Methods...
//
HRESULT CHistCacheFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
    {
        *ppidl = ILClone(_pidl);
        return *ppidl ? NOERROR : E_OUTOFMEMORY;
    }

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}

//////////////////////////////////
//
// IContextMenu Methods...
//
HRESULT CHistCacheFolder::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,UINT idCmdLast, UINT uFlags)
{
    ASSERT(!_uViewType);
    USHORT cItems = 0;

    TraceMsg(DM_HSFOLDER, "hcf - cm - QueryContextMenu() called.");
    if (uFlags == CMF_NORMAL)
    {
        HMENU hmenuHist = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(IsHistory(_foldertype) ? MENU_HISTORY : MENU_CACHE));
        if (hmenuHist)
        {
            cItems = MergeMenuHierarchy(hmenu, hmenuHist, idCmdFirst, idCmdLast);

            DestroyMenu(hmenuHist);
        }
    }


    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    return ResultFromShort(cItems);    // number of menu items
}

STDMETHODIMP CHistCacheFolder::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    ASSERT(!_uViewType);
    // We don't deal with the VERBONLY case
    TraceMsg(DM_HSFOLDER, "hcf - cm - InvokeCommand() called.");
    ASSERT(HIWORD(pici->lpVerb) == 0);

    int idCmd;
    if (HIWORD(pici->lpVerb))
        idCmd = -1;
    else
        idCmd = LOWORD(pici->lpVerb);

    return HistCacheFolderView_Command(pici->hwnd, idCmd, _foldertype);
}


STDMETHODIMP CHistCacheFolder::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved,
                                LPSTR pszName, UINT cchMax)
{
    ASSERT(!_uViewType);
    HRESULT hres = E_FAIL;

    TraceMsg(DM_HSFOLDER, "hcf - cm - GetCommandString() called.");
    if (uFlags == GCS_HELPTEXTA)
    {
        MLLoadStringA((UINT)idCmd+IDS_MH_FIRST, pszName, cchMax);
        hres = NOERROR;
    }
    else if (uFlags == GCS_HELPTEXTW)
    {
        MLLoadStringW((UINT)idCmd+IDS_MH_FIRST, (LPWSTR)pszName, cchMax);
        hres = NOERROR;
    }
    return hres;

}

//////////////////////////////////////////////////
// IShellFolderViewType Methods
//
// but first, the enumerator class...
class CHistViewTypeEnum : public IEnumIDList
{
    friend class CHistCacheFolder;
public:
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { _uCurViewType += celt; return S_OK; }
    STDMETHODIMP Reset()          { _uCurViewType =     1; return S_OK; }
    STDMETHODIMP Clone(IEnumIDList **ppenum);

protected:
    ~CHistViewTypeEnum() {}
    CHistViewTypeEnum() : _cRef(1), _uCurViewType(1) {}

    LONG  _cRef;
    UINT  _uCurViewType;
private:
    // private members to prevent copying this object
    CHistViewTypeEnum(CHistViewTypeEnum& ch) {}
    CHistViewTypeEnum& operator=(CHistViewTypeEnum& ch) { return *this; }
};

STDMETHODIMP CHistViewTypeEnum::QueryInterface(REFIID iid, void **ppv) {
    if ((iid == IID_IEnumIDList) || (iid == IID_IUnknown))
    {
        *ppv = (void *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CHistViewTypeEnum::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistViewTypeEnum::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistViewTypeEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr;

    if (rgelt && (pceltFetched || 1 == celt))
    {
        ULONG i = 0;

        while (i < celt)
        {
            if (_uCurViewType <= VIEWPIDL_ORDER_MAX)
            {
                hr = CreateSpecialViewPidl(_uCurViewType, &(rgelt[i]));

                if (SUCCEEDED(hr))
                {
                    ++i;
                    ++_uCurViewType;
                }
                else
                {
                    while (i)
                        ILFree(rgelt[--i]);

                    break;
                }
            }
            else
            {
                hr = S_FALSE;
                break;
            }
        }

        if (pceltFetched)
            *pceltFetched = i;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CHistViewTypeEnum::Clone(IEnumIDList **ppenum)
{
    if (ppenum) {
        CHistViewTypeEnum* phvte = new CHistViewTypeEnum();
        if (phvte) {
            phvte->_uCurViewType = _uCurViewType;
            *ppenum = phvte;
            return S_OK;
        }
        else
            return E_OUTOFMEMORY;
    }
    return E_INVALIDARG;
}

// Continuing with the CHistCacheFolder::IShellFolderViewType
STDMETHODIMP CHistCacheFolder::EnumViews(ULONG grfFlags, IEnumIDList **ppenum) 
{
    *ppenum = new CHistViewTypeEnum();
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CHistCacheFolder::GetDefaultViewName(DWORD uFlags, LPWSTR *ppwszName) 
{
    TCHAR szName[MAX_PATH];

    MLLoadString(IDS_HISTVIEW_DEFAULT, szName, ARRAYSIZE(szName));
    return SHStrDup(szName, ppwszName);
}

// Remember that these *MUST* be in order so that
//  the array can be accessed by VIEWPIDL type
const DWORD CHistCacheFolder::_rdwFlagsTable[] = {
    SFVTFLAG_NOTIFY_CREATE,                          // Date
    SFVTFLAG_NOTIFY_CREATE,                          // site
    0,                                               // freq
    SFVTFLAG_NOTIFY_CREATE | SFVTFLAG_NOTIFY_RESORT  // today
};

STDMETHODIMP CHistCacheFolder::GetViewTypeProperties(LPCITEMIDLIST pidl, DWORD *pdwFlags) 
{
    HRESULT hr = S_OK;
    UINT    uFlagTableIndex;

    if ((pidl == NULL) || ILIsEmpty(pidl)) // default view
    {
        uFlagTableIndex = 0;
    }
    else
    {
        // Make sure the pidl is dword aligned.

        BOOL fRealigned;
        hr = AlignPidl(&pidl, &fRealigned);

        if (SUCCEEDED(hr))
        {
            if (IS_VALID_VIEWPIDL(pidl))
            {
                uFlagTableIndex = ((LPVIEWPIDL)pidl)->usViewType;
                ASSERT(uFlagTableIndex <= VIEWPIDL_ORDER_MAX);
            }
            else
            {
                hr =  E_INVALIDARG;
            }

            if (fRealigned)
                FreeRealignedPidl(pidl);
        }
    }

    *pdwFlags = _rdwFlagsTable[uFlagTableIndex];

    return hr;
}

HRESULT CHistCacheFolder::TranslateViewPidl(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlView,
                                            LPITEMIDLIST *ppidlOut)
{
    HRESULT hr;

    if (pidl && IS_VALID_VIEWPIDL(pidlView))
    {
        if (!IS_VALID_VIEWPIDL(pidl))
        {
            hr = ConvertStandardHistPidlToSpecialViewPidl(pidl,
                                 ((LPVIEWPIDL)pidlView)->usViewType,
                                 ppidlOut);
        }
        else
        {
            hr = E_NOTIMPL;
        }

    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//////////////////////////////////////////////////
//
// IShellFolderSearchable Methods
//
// For more information about how this search stuff works,
//  please see the comments for _CurrentSearches above
STDMETHODIMP CHistCacheFolder::FindString(LPCWSTR pwszTarget, LPDWORD pdwFlags,
                                          IUnknown *punkOnAsyncSearch,
                                          LPITEMIDLIST *ppidlOut)
{
    HRESULT hres = E_INVALIDARG;
    if (ppidlOut)
    {
        *ppidlOut = NULL;
        if (pwszTarget)
        {
            LPITEMIDLIST pidlView;

            SYSTEMTIME systime;
            FILETIME   ftNow;
            GetLocalTime(&systime);
            SystemTimeToFileTime(&systime, &ftNow);

            hres = CreateSpecialViewPidl(VIEWPIDL_SEARCH, &pidlView, sizeof(SEARCHVIEWPIDL) - sizeof(VIEWPIDL));
            if (SUCCEEDED(hres))
            {
                ((LPSEARCHVIEWPIDL)pidlView)->ftSearchKey = ftNow;

                IShellFolderSearchableCallback *psfscOnAsyncSearch = NULL;
                if (punkOnAsyncSearch)
                    punkOnAsyncSearch->QueryInterface(IID_IShellFolderSearchableCallback,
                                                      (void **)&psfscOnAsyncSearch);

                // Insert this search into the global database
                //  This constructor will AddRef psfscOnAsyncSearch
                _CurrentSearches *pcsNew = new _CurrentSearches(ftNow, pwszTarget, psfscOnAsyncSearch);

                if (pcsNew) {
                    if (psfscOnAsyncSearch)
                        psfscOnAsyncSearch->Release();  // _CurrentSearches now holds the ref

                    // This will AddRef pcsNew 'cause its going in the list
                    _CurrentSearches::s_NewSearch(pcsNew);
                    pcsNew->Release();
                    *ppidlOut = pidlView;
                    hres = S_OK;
                }
                else {
                    ILFree(pidlView);
                    hres = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hres;
}

STDMETHODIMP CHistCacheFolder::CancelAsyncSearch(LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (IS_VALID_VIEWPIDL(pidlSearch) &&
        (((LPVIEWPIDL)pidlSearch)->usViewType == VIEWPIDL_SEARCH))
    {
        hr = S_FALSE;
        _CurrentSearches *pcs = _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidlSearch)->ftSearchKey);
        if (pcs) {
            pcs->_fKillSwitch = TRUE;
            hr = S_OK;
            pcs->Release();
        }
    }
    return hr;
}
STDMETHODIMP CHistCacheFolder::InvalidateSearch(LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags)
{
    HRESULT hres = E_INVALIDARG;
    if (IS_VALID_VIEWPIDL(pidlSearch) &&
        (((LPVIEWPIDL)pidlSearch)->usViewType == VIEWPIDL_SEARCH))
    {
        hres = S_FALSE;
        _CurrentSearches *pcs = _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidlSearch)->ftSearchKey);
        if (pcs) {
            _CurrentSearches::s_RemoveSearch(pcs);
            pcs->Release();
        }
    }
    return hres;
}

//////////////////////////////////////////////////

DWORD CHistCacheFolder::_GetHitCount(LPCTSTR pszUrl)
{
    DWORD dwHitCount = 0;
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();

    if (pUrlHistStg)
    {
        PROPVARIANT vProp = {0};
        if (SUCCEEDED(pUrlHistStg->GetProperty(pszUrl, PID_INTSITE_VISITCOUNT, &vProp)) &&
            (vProp.vt == VT_UI4))
        {
            dwHitCount = vProp.lVal;
        }
        pUrlHistStg->Release();
    }
    return dwHitCount;
}

//  if !fOleMalloc, use LocalAlloc for speed  // ok to pass in NULL for lpStatURL
LPHEIPIDL _CreateHCacheFolderPidl(BOOL fOleMalloc, LPCTSTR pszUrl, FILETIME ftModified, LPSTATURL lpStatURL,
                                  __int64 llPriority/* = 0*/, DWORD dwNumHits/* = 0*/) // used in freqnecy view
{
    USHORT usUrlSize = (USHORT)((lstrlen(pszUrl) + 1) * sizeof(TCHAR));
    DWORD  dwSize = sizeof(HEIPIDL)- 2*sizeof(USHORT) + usUrlSize;
    USHORT usTitleSize = 0;
    BOOL fUseTitle = (lpStatURL && lpStatURL->pwcsTitle && _TitleIsGood(lpStatURL->pwcsTitle));
    if (fUseTitle)
        usTitleSize = (USHORT)((lstrlen(lpStatURL->pwcsTitle) + 1) * sizeof(WCHAR));

    dwSize += usTitleSize;

#if defined(UNIX)
    dwSize = ALIGN4(dwSize);
#endif

    LPHEIPIDL pheip = (LPHEIPIDL)_CreateCacheFolderPidl(fOleMalloc, dwSize, HEIPIDL_SIGN);

    if (pheip)
    {
        pheip->usUrl      = sizeof(HEIPIDL);
        pheip->usFlags    = lpStatURL ? HISTPIDL_VALIDINFO : 0;
        pheip->usTitle    = fUseTitle ? pheip->usUrl+usUrlSize :0;
        pheip->ftModified = ftModified;
        pheip->llPriority = llPriority;
        pheip->dwNumHits  = dwNumHits;
        if (lpStatURL)
        {
            pheip->ftLastVisited = lpStatURL->ftLastVisited;
#ifndef UNIX
            if (fUseTitle)
                StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usTitle), lpStatURL->pwcsTitle, usTitleSize / sizeof(TCHAR));
#else
            // IEUNIX : BUG BUG _CreateCacheFolderPidl() uses lstrlenA
            // while creating the pidl.
            if (fUseTitle)
                StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usTitle), lpStatURL->pwcsTitle, usTitleSize / sizeof(TCHAR));
#endif
        }
        else {
            //mm98: not so sure about the semantics on this one -- but this call
            //  with lpstaturl NULL (called from _NotifyWrite<--_WriteHistory<--WriteHistory<--CUrlHistory::_WriteToHistory
            //  makes for an uninitialised "Last Visited Member" which wreaks havoc
            //  when we want to order URLs by last visited
            pheip->ftLastVisited = ftModified;
        }
        StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usUrl), pszUrl, usUrlSize / sizeof(TCHAR));
    }
    return pheip;
}

//  if !fOleMalloc, use LocalAlloc for speed
LPCEIPIDL _CreateIdCacheFolderPidl(BOOL fOleMalloc, USHORT usSign, LPCTSTR szId)
{
    DWORD  dwSize = (lstrlen(szId) + 1) * sizeof(TCHAR);
    LPCEIPIDL pceip = _CreateCacheFolderPidl(fOleMalloc, dwSize, usSign);
    if (pceip)
    {
        // dst <- src
        // since pcei is ID type sign, _GetURLTitle points into correct place in pcei
        StrCpyN((LPTSTR)_GetURLTitle(pceip), szId, dwSize / sizeof(TCHAR));
    }
    return pceip;
}

LPCEIPIDL _CreateBuffCacheFolderPidl(BOOL fOleAlloc, DWORD dwSize, LPINTERNET_CACHE_ENTRY_INFO pcei)
{
    LPCEIPIDL pceip = _CreateCacheFolderPidl(fOleAlloc, dwSize, (USHORT)CEIPIDL_SIGN);

    if (pceip)
    {
       _CopyCEI(&pceip->cei, pcei, dwSize);
    }
    return pceip;
}

//  if !fOleAlloc, use LocalAlloc for speed
LPCEIPIDL _CreateCacheFolderPidl(BOOL fOleAlloc, DWORD dwSize, USHORT usSign)
{
    LPCEIPIDL pcei;
    DWORD dwTotalSize;

    //  Note: the buffer size returned by wininet includes INTERNET_CACHE_ENTRY_INFO
    dwTotalSize = sizeof(CEIPIDL) - sizeof(INTERNET_CACHE_ENTRY_INFO) + sizeof(USHORT) + dwSize;

#if defined(UNIX)
    dwTotalSize = ALIGN4(dwTotalSize);
#endif

    if (fOleAlloc)
    {
        pcei = (LPCEIPIDL)OleAlloc(dwTotalSize);
        if (pcei != NULL)
        {
            memset(pcei, 0, dwTotalSize);
        }
    }
    else
    {
        pcei = (LPCEIPIDL) LocalAlloc(GPTR, dwTotalSize);
        //  LocalAlloc zero inits
    }
    if (pcei)
    {
        pcei->cb = (USHORT)(dwTotalSize - sizeof(USHORT));
        pcei->usSign = usSign;
    }
    return pcei;
}

// returns a pidl (viewpidl)
//  You must free the pidl with ILFree

// cbExtra   -  count of how much to allocate at the end of the pidl
// ppbExtra  -  pointer to buffer at end of pidl that is cbExtra big
HRESULT CreateSpecialViewPidl(USHORT usViewType, LPITEMIDLIST* ppidlOut, UINT cbExtra /* = 0*/, LPBYTE *ppbExtra /* = NULL*/)
{
    HRESULT hres;

    if (ppidlOut) {
        *ppidlOut = NULL;

        ASSERT((usViewType > 0) &&
               ((usViewType <= VIEWPIDL_ORDER_MAX) ||
                (usViewType  == VIEWPIDL_SEARCH)));

        //   Tack another ITEMIDLIST on the end to be the empty "null terminating" pidl
        USHORT cbSize = sizeof(VIEWPIDL) + cbExtra + sizeof(ITEMIDLIST);
        // use the shell's allocator because folks will want to free it with ILFree
        VIEWPIDL *viewpidl = ((VIEWPIDL *)SHAlloc(cbSize));
        if (viewpidl) {
            // this should also make the "next" pidl empty
            memset(viewpidl, 0, cbSize);
            viewpidl->cb         = (USHORT)(sizeof(VIEWPIDL) + cbExtra);
            viewpidl->usSign     = VIEWPIDL_SIGN;
            viewpidl->usViewType = usViewType;
            viewpidl->usExtra    = 0;  // currently unused

            if (ppbExtra)
                *ppbExtra = ((LPBYTE)viewpidl) + sizeof(VIEWPIDL);

            *ppidlOut = (LPITEMIDLIST)viewpidl;
            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else
        hres = E_INVALIDARG;

    return hres;
}

// pidl should be freed by caller
// URL must have some sort of cache container prefix
LPHEIPIDL CHistCacheFolder::_CreateHCacheFolderPidlFromUrl(BOOL fOleMalloc, LPCTSTR pszPrefixedUrl)
{
    LPHEIPIDL pheiRet;
    HRESULT   hresLocal = E_FAIL;
    STATURL   suThis;
    LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszPrefixedUrl);
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
    if (pUrlHistStg)
    {
        hresLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
        pUrlHistStg->Release();
    }

    FILETIME ftLastVisit = { 0 };
    DWORD    dwNumHits   = 0;

    if (FAILED(hresLocal)) { // maybe the cache knows...
        BYTE ab[MAX_URLCACHE_ENTRY];
        LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)(&ab);
        DWORD dwSize = MAX_URLCACHE_ENTRY;
        if (GetUrlCacheEntryInfo(_StripHistoryUrlToUrl(pszPrefixedUrl), pcei, &dwSize)) {
            ftLastVisit = pcei->LastAccessTime;
            dwNumHits   = pcei->dwHitRate;
        }
    }

    pheiRet = _CreateHCacheFolderPidl(fOleMalloc, pszPrefixedUrl,
                                      SUCCEEDED(hresLocal) ? suThis.ftLastVisited : ftLastVisit,
                                      SUCCEEDED(hresLocal) ? &suThis : NULL, 0,
                                      SUCCEEDED(hresLocal) ? _GetHitCount(pszStrippedUrl) : dwNumHits);
    if (SUCCEEDED(hresLocal) && suThis.pwcsTitle)
        OleFree(suThis.pwcsTitle);
    return pheiRet;
}


UINT _CountPidlParts(LPCITEMIDLIST pidl) {
    LPCITEMIDLIST pidlTemp = pidl;
    UINT          uParts   = 0;

    if (pidl)
    {
        for (uParts = 0; pidlTemp->mkid.cb; pidlTemp = _ILNext(pidlTemp))
            ++uParts;
    }
    return uParts;
}

// you must dealloc (LocalFree) the ppidl returned
LPITEMIDLIST* _SplitPidl(LPCITEMIDLIST pidl, UINT& uSizeInOut) {
    LPCITEMIDLIST  pidlTemp  = pidl;
    LPITEMIDLIST*  ppidlList =
        reinterpret_cast<LPITEMIDLIST *>(LocalAlloc(LPTR,
                                                    sizeof(LPITEMIDLIST) * uSizeInOut));
    if (pidlTemp && ppidlList) {
        UINT uCount;
        for (uCount = 0; ( (uCount < uSizeInOut) && (pidlTemp->mkid.cb) );
             ++uCount, pidlTemp = _ILNext(pidlTemp))
            ppidlList[uCount] = const_cast<LPITEMIDLIST>(pidlTemp);
    }
    return ppidlList;
}

LPITEMIDLIST* _SplitPidlEasy(LPCITEMIDLIST pidl, UINT& uSizeOut) {
    uSizeOut = _CountPidlParts(pidl);
    return _SplitPidl(pidl, uSizeOut);
}

// caller LocalFree's *ppidlOut
//  returned pidl should be combined with the history folder location
HRESULT _ConvertStdPidlToViewPidl_OrderSite(LPCITEMIDLIST pidlSecondLast,
                                            LPCITEMIDLIST pidlLast,
                                            LPITEMIDLIST *ppidlOut) {
    HRESULT hres = E_FAIL;

    // painfully construct the final pidl by concatenating the little
    //   peices  [special_viewpidl, iddpidl, heipidl]
    if ( _IsValid_IDPIDL(pidlSecondLast)                                     &&
         EQUIV_IDSIGN(IDDPIDL_SIGN,
                      (reinterpret_cast<LPCEIPIDL>
                       (const_cast<LPITEMIDLIST>(pidlSecondLast)))->usSign)  &&
         (_IsValid_HEIPIDL(pidlLast)) )
    {
        LPITEMIDLIST pidlViewTemp = NULL;
        hres = CreateSpecialViewPidl(VIEWPIDL_ORDER_SITE, &pidlViewTemp);
        if (SUCCEEDED(hres) && pidlViewTemp) {
            *ppidlOut =
                ILCombine(pidlViewTemp, pidlSecondLast);
            if (*ppidlOut)
                hres = S_OK;
            else
                hres = E_OUTOFMEMORY;
            ILFree(pidlViewTemp);
        }
    }
    else
        hres = E_INVALIDARG;
    return hres;
}

// caller LocalFree's *ppidlOut
//  returned pidl should be combined with the history folder location
HRESULT _ConvertStdPidlToViewPidl_OrderToday(LPITEMIDLIST pidlLast,
                                             LPITEMIDLIST *ppidlOut,
                                             USHORT usViewType = VIEWPIDL_ORDER_TODAY)
{
    HRESULT hRes = E_FAIL;

    // painfully construct the final pidl by concatenating the little
    //   peices  [special_viewpidl, heipidl]
    if (_IsValid_HEIPIDL(pidlLast)) {
        LPHEIPIDL    phei         = reinterpret_cast<LPHEIPIDL>(pidlLast);
        LPITEMIDLIST pidlViewTemp = NULL;
        hRes = CreateSpecialViewPidl(usViewType, &pidlViewTemp);
        if (SUCCEEDED(hRes) && pidlViewTemp) {
            *ppidlOut =
                ILCombine(pidlViewTemp, reinterpret_cast<LPITEMIDLIST>(phei));
            if (*ppidlOut)
                hRes = S_OK;
            else
                hRes = E_OUTOFMEMORY;
            ILFree(pidlViewTemp);
        }
    }
    else
        hRes = E_INVALIDARG;
    return hRes;
}

// remember to ILFree pidl
HRESULT ConvertStandardHistPidlToSpecialViewPidl(LPCITEMIDLIST pidlStandardHist,
                                                 USHORT        usViewType,
                                                 LPITEMIDLIST *ppidlOut) {
    if (!pidlStandardHist || !ppidlOut) {
        return E_FAIL;
    }
    HRESULT hRes = E_FAIL;

    UINT          uPidlCount;
    LPITEMIDLIST *ppidlSplit = _SplitPidlEasy(pidlStandardHist, uPidlCount);
    /* Standard Hist Pidl should be in this form:
     *          [IDIPIDL, IDDPIDL, HEIPIDL]
     *  ex:     [Today,   foo.com, http://foo.com/bar.html]
     */
    if (ppidlSplit) {
        if (uPidlCount >= 3) {
            LPITEMIDLIST pidlTemp = NULL;
            switch(usViewType) {
            case VIEWPIDL_ORDER_FREQ:
            case VIEWPIDL_ORDER_TODAY:
                hRes = _ConvertStdPidlToViewPidl_OrderToday(ppidlSplit[uPidlCount - 1],
                                                            &pidlTemp, usViewType);
                break;
            case VIEWPIDL_ORDER_SITE:
                hRes = _ConvertStdPidlToViewPidl_OrderSite(ppidlSplit[uPidlCount - 2],
                                                           ppidlSplit[uPidlCount - 1],
                                                           &pidlTemp);
                break;
            default:
                hRes = E_INVALIDARG;
            }
            if (SUCCEEDED(hRes) && pidlTemp) {
                *ppidlOut = pidlTemp;
                hRes      = (*ppidlOut ? S_OK : E_OUTOFMEMORY);
            }
        }
        else {
            hRes = E_INVALIDARG;
        }
        LocalFree(ppidlSplit);
    }
    else
        hRes = E_OUTOFMEMORY;

    return hRes;
}

// START OF JCORDELL CODE

#ifdef DEBUG
BOOL ValidBeginningOfDay( const SYSTEMTIME *pTime )
{
    return pTime->wHour == 0 && pTime->wMinute == 0 && pTime->wSecond == 0 && pTime->wMilliseconds == 0;
}

BOOL ValidBeginningOfDay( const FILETIME *pTime )
{
    SYSTEMTIME sysTime;

    FileTimeToSystemTime( pTime, &sysTime );
    return ValidBeginningOfDay( &sysTime);
}
#endif

void _CommonTimeFormatProcessing(const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    int *pdays_delta,
                                    int *pdays_delta_from_today,
                                    TCHAR *szStartDateBuffer,
                                    DWORD dwStartDateBuffer,
                                    SYSTEMTIME *pSysStartTime,
                                    SYSTEMTIME *pSysEndTime,
                                    LCID lcidUI)
{
    SYSTEMTIME sysStartTime, sysEndTime, sysLocalTime;
    FILETIME fileLocalTime;

    // ASSERTS
    ASSERT(ValidBeginningOfDay( pStartTime ));
    ASSERT(ValidBeginningOfDay( pEndTime ));

    // Get times in SYSTEMTIME format
    FileTimeToSystemTime( pStartTime, &sysStartTime );
    FileTimeToSystemTime( pEndTime, &sysEndTime );

    // Get string date of start time
    GetDateFormat(lcidUI, DATE_SHORTDATE, &sysStartTime, NULL, szStartDateBuffer, dwStartDateBuffer);

    // Get FILETIME of the first instant of today
    GetLocalTime( &sysLocalTime );
    sysLocalTime.wHour = sysLocalTime.wMinute = sysLocalTime.wSecond = sysLocalTime.wMilliseconds = 0;
    SystemTimeToFileTime( &sysLocalTime, &fileLocalTime );

    *pdays_delta = DAYS_DIFF(pEndTime, pStartTime);
    *pdays_delta_from_today = DAYS_DIFF(&fileLocalTime, pStartTime);
    *pSysEndTime = sysEndTime;
    *pSysStartTime = sysStartTime;
}

// this wrapper allows the FormatMessage wrapper to make use of FormatMessageLite, which
// does not require a code page for correct operation on Win9x.  The original FormatMessage calls
// used the FORMAT_MESSAGE_MAX_WIDTH_MASK (which is not relevant to our strings), and used an array
// of arguments.  Now we make the call compatible with FormatMessageLite.

DWORD FormatMessageLiteWrapperW(LPCWSTR lpSource, LPWSTR lpBuffer, DWORD nSize, ...)
{
    va_list arguments;
    va_start(arguments, nSize);
    DWORD dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING, lpSource, 0, 0, lpBuffer, nSize, &arguments);
    va_end(arguments);
    return dwRet;
}

BOOL GetTooltipForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    TCHAR *szBuffer, int cbBufferLength )
{
    SYSTEMTIME sysStartTime, sysEndTime;
    int days_delta;                     // number of days between start and end time
    int days_delta_from_today;          // number of days between today and start time
    TCHAR szStartDateBuffer[64];
    TCHAR szDayBuffer[64];
    TCHAR szEndDateBuffer[64];
    TCHAR *args[2];
    TCHAR szFmt[64];
    int idFormat;
    LANGID  lidUI;
    LCID    lcidUI;

    lidUI = MLGetUILanguage();
    lcidUI = MAKELCID(lidUI, SORT_DEFAULT);

    _CommonTimeFormatProcessing(pStartTime,
                                pEndTime,
                                &days_delta,
                                &days_delta_from_today,
                                szStartDateBuffer,
                                ARRAYSIZE(szStartDateBuffer),
                                &sysStartTime,
                                &sysEndTime,
                                lcidUI);
    if ( days_delta == 1 ) {
        args[0] = &szDayBuffer[0];
        idFormat = IDS_DAYTOOLTIP;

        // day sized bucket
        if ( days_delta_from_today == 0 ) {
            // today
            szDayBuffer[0] = 0;
            idFormat = IDS_TODAYTOOLTIP;
        }
        else if  ( days_delta_from_today > 0 && days_delta_from_today < 7 )
        {
            // within the last week, put day of week
            GetDateFormat(lcidUI, 0, &sysStartTime, TEXT("dddd"), szDayBuffer, ARRAYSIZE(szDayBuffer));
        }
        else {
            // just a plain day bucket
            StrCpyN( szDayBuffer, szStartDateBuffer, ARRAYSIZE(szDayBuffer) );
        }
    }
    else if ( days_delta == 7 && sysStartTime.wDayOfWeek == 1 ) {
        // week-size bucket starting on a Monday
        args[0] = &szStartDateBuffer[0];

        // make is point to the first string for safety sake. This will be ignored by wsprintf
        args[1] = args[0];
        idFormat = IDS_WEEKTOOLTIP;
    }
    else {
        // non-standard bucket (not exactly a week and not exactly a day)

        args[0] = &szStartDateBuffer[0];
        args[1] = &szEndDateBuffer[0];
        idFormat = IDS_MISCTOOLTIP;

        GetDateFormat(lcidUI, DATE_SHORTDATE, &sysEndTime, NULL, szEndDateBuffer, ARRAYSIZE(szEndDateBuffer) );
    }

    MLLoadString(idFormat, szFmt, ARRAYSIZE(szFmt));

    // NOTE, if the second parameter is not needed by the szFMt, then it will be ignored by wnsprintf
    if (idFormat == IDS_DAYTOOLTIP)
        wnsprintf(szBuffer, cbBufferLength, szFmt, args[0]);
    else
        FormatMessageLiteWrapperW(szFmt, szBuffer, cbBufferLength, args[0], args[1]);
    return TRUE;
}

BOOL GetDisplayNameForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    LPTSTR szBuffer, int cbBufferLength)
{
    SYSTEMTIME sysStartTime, sysEndTime;
    int days_delta;                     // number of days between start and end time
    int days_delta_from_today;          // number of days between today and start time
    TCHAR szStartDateBuffer[64];
    LANGID lidUI;
    LCID lcidUI;

    lidUI = MLGetUILanguage();
    lcidUI = MAKELCID(lidUI, SORT_DEFAULT);

    _CommonTimeFormatProcessing(pStartTime,
                                pEndTime,
                                &days_delta,
                                &days_delta_from_today,
                                szStartDateBuffer,
                                ARRAYSIZE(szStartDateBuffer),
                                &sysStartTime,
                                &sysEndTime,
                                lcidUI);
    if ( days_delta == 1 ) {
        // day sized bucket
        if ( days_delta_from_today == 0 ) {
            // today
            MLLoadString(IDS_TODAY, szBuffer, cbBufferLength/sizeof(TCHAR));
        }
        else if  ( days_delta_from_today > 0 && days_delta_from_today < 7 )
        {
            // within the last week, put day of week
            int nResult = GetDateFormat(lcidUI, 0, &sysStartTime, TEXT("dddd"), szBuffer, cbBufferLength);


            ASSERT(nResult);
        }
        else {
            // just a plain day bucket
            StrCpyN( szBuffer, szStartDateBuffer, cbBufferLength );
        }
    }
    else if ( days_delta == 7 && sysStartTime.wDayOfWeek == 1 ) {
        // week-size bucket starting on a Monday
        TCHAR szFmt[64];

        int nWeeksAgo = days_delta_from_today / 7;

        if (nWeeksAgo == 1) {
            // print "Last Week"
            MLLoadString(IDS_LASTWEEK, szBuffer, cbBufferLength/sizeof(TCHAR));
        }
        else {
            // print "n Weeks Ago"
            MLLoadString(IDS_WEEKSAGO, szFmt, ARRAYSIZE(szFmt));
            wnsprintf(szBuffer, cbBufferLength, szFmt, nWeeksAgo);
        }
    }
    else {
        // non-standard bucket (not exactly a week and not exactly a day)
        TCHAR szFmt[64];
        TCHAR szEndDateBuffer[64];
        TCHAR *args[2];

        args[0] = &szStartDateBuffer[0];
        args[1] = &szEndDateBuffer[0];


        GetDateFormat(lcidUI, DATE_SHORTDATE, &sysEndTime, NULL, szEndDateBuffer, ARRAYSIZE(szEndDateBuffer) );

        MLLoadString(IDS_FROMTO, szFmt, ARRAYSIZE(szFmt));
        FormatMessageLiteWrapperW(szFmt, szBuffer, cbBufferLength, args[0], args[1]);
    }

    return TRUE;
}

#undef ZONES_PANE_WIDTH
#define ZONES_PANE_WIDTH    120

void ResizeStatusBar(HWND hwnd, BOOL fInit)
{
    HWND hwndStatus = NULL;
    RECT rc = {0};
    LPSHELLBROWSER psb = FileCabinet_GetIShellBrowser(hwnd);
    UINT cx;
    int ciParts[] = {-1, -1};

    if (!psb)
        return;

    psb->GetControlWindow(FCW_STATUS, &hwndStatus);


    if (fInit)
    {
        int nParts = 0;

        psb->SendControlMsg(FCW_STATUS, SB_GETPARTS, 0, 0L, (LRESULT*)&nParts);
        for (int n = 0; n < nParts; n ++)
        {
            psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, n, (LPARAM)TEXT(""), NULL);
            psb->SendControlMsg(FCW_STATUS, SB_SETICON, n, NULL, NULL);
        }
        psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, 0, 0L, NULL);
    }
    GetClientRect(hwndStatus, &rc);
    cx = rc.right;

    ciParts[0] = cx - ZONES_PANE_WIDTH;

    psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts, NULL);
}

//  END OF JCORDELL CODE

/*********************************************************************
                        StrHash implementation
 *********************************************************************/

//////////////////////////////////////////////////////////////////////
// StrHashNode
StrHash::StrHashNode::StrHashNode(LPCTSTR psz, void* pv, int fCopy,
                                  StrHashNode* next) {
    ASSERT(psz);
    pszKey = (fCopy ? StrDup(psz) : psz);
    pvVal  = pv;  // don't know the size -- you'll have to destroy
    this->fCopy = fCopy;
    this->next  = next;
}

StrHash::StrHashNode::~StrHashNode() {
    if (fCopy)
        LocalFree(const_cast<LPTSTR>(pszKey));
}

//////////////////////////////////////////////////////////////////////
// StrHash
const unsigned int StrHash::sc_auPrimes[] = {
    29, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593
};

const unsigned int StrHash::c_uNumPrimes     = 11;
const unsigned int StrHash::c_uFirstPrime    =  4;

// load factor is computed as (n * USHRT_MAX / t) where 'n' is #elts in table
//   and 't' is table size
const unsigned int StrHash::c_uMaxLoadFactor = ((USHRT_MAX * 100) / 95); // .95

StrHash::StrHash(int fCaseInsensitive) {
    nCurPrime = c_uFirstPrime;
    nBuckets  = sc_auPrimes[nCurPrime];

    // create an array of buckets and null out each one
    ppshnHashChain = new StrHashNode* [nBuckets];

    if (ppshnHashChain) {
        for (unsigned int i = 0; i < nBuckets; ++i)
            ppshnHashChain[i] = NULL;
    }
    nElements = 0;
    _fCaseInsensitive = fCaseInsensitive;
}

StrHash::~StrHash() {
    if (ppshnHashChain) {
        // delete all nodes first, then delete the chain
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                StrHashNode* pshnNext = pshnTemp->next;
                delete pshnTemp;
                pshnTemp = pshnNext;
            }
        }
        delete [] ppshnHashChain;
    }
}

#ifdef DEBUG
// Needed so that this stuff doesn't show 
// up as a leak when it is freed from someother thread
void
StrHash::_RemoveHashNodesFromMemList() {
    if (ppshnHashChain) {
        // remove all hasnodes from mem list first, then delete the chain
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                remove_from_memlist((LPVOID)pshnTemp);
                StrHashNode* pshnNext = pshnTemp->next;
                pshnTemp = pshnNext;
            }
        }
        remove_from_memlist((LPVOID)ppshnHashChain);
    }
}

// Needed by the thread to which this object was
// sent to add it on to the mem list to detect leaks

void
StrHash::_AddHashNodesFromMemList() {
    if (ppshnHashChain) {
        // add all nodes into mem list
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                DbgAddToMemList(((LPVOID)pshnTemp));
                StrHashNode* pshnNext = pshnTemp->next;
                pshnTemp = pshnNext;
            }
        }
        DbgAddToMemList((LPVOID)ppshnHashChain);
    }
}

#endif //DEBUG
// returns the void* value if its there and NULL if its not
void* StrHash::insertUnique(LPCTSTR pszKey, int fCopy, void* pvVal) {
    unsigned int uBucketNum = _hashValue(pszKey, nBuckets);
    StrHashNode* pshnNewElt;
    if ((pshnNewElt = _findKey(pszKey, uBucketNum)))
        return pshnNewElt->pvVal;
    if (_prepareForInsert())
        uBucketNum = _hashValue(pszKey, nBuckets);
    pshnNewElt =
        new StrHashNode(pszKey, pvVal, fCopy,
                        ppshnHashChain[uBucketNum]);
    if (pshnNewElt && ppshnHashChain)
        ppshnHashChain[uBucketNum] = pshnNewElt;
    return NULL;
}

void* StrHash::retrieve(LPCTSTR pszKey) {
    if (!pszKey) return 0;
    unsigned int uBucketNum = _hashValue(pszKey, nBuckets);
    StrHashNode* pshn = _findKey(pszKey, uBucketNum);
    return (pshn ? pshn->pvVal : NULL);
}

// dynamically grow the hash table if necessary
//   return TRUE if rehashing was done
int StrHash::_prepareForInsert() {
    ++nElements; // we'te adding an element
    if ((_loadFactor() >= c_uMaxLoadFactor) &&
        (nCurPrime++   <= c_uNumPrimes)) {
        //--- grow the hashTable by rehashing everything:
        // set up new hashTable
        unsigned int nBucketsOld = nBuckets;
        nBuckets = sc_auPrimes[nCurPrime];
        StrHashNode** ppshnHashChainOld = ppshnHashChain;
        ppshnHashChain = new StrHashNode* [nBuckets];
        if (ppshnHashChain && ppshnHashChainOld) {
            unsigned int u;
            for (u = 0; u < nBuckets; ++u)
                ppshnHashChain[u] = NULL;
            // rehash by traversing all buckets
            for (u = 0; u < nBucketsOld; ++u) {
                StrHashNode* pshnTemp = ppshnHashChainOld[u];
                while (pshnTemp) {
                    unsigned int uBucket  = _hashValue(pshnTemp->pszKey, nBuckets);
                    StrHashNode* pshnNext = pshnTemp->next;
                    pshnTemp->next = ppshnHashChain[uBucket];
                    ppshnHashChain[uBucket] = pshnTemp;
                    pshnTemp = pshnNext;
                }
            }
            delete [] ppshnHashChainOld;
        }
        return 1;
    } // if needs rehashing
    return 0;
}

/*
// this variant of Weinberger's hash algorithm was taken from
//  packager.cpp (ie source)
unsigned int _oldhashValuePJW(const char* c_pszStr, unsigned int nBuckets) {
    unsigned long h = 0L;
    while(*c_pszStr)
        h = ((h << 4) + *(c_pszStr++) + (h >> 28));
    return (h % nBuckets);
}
*/

// this variant of Weinberger's hash algorithm is adapted from
//  Aho/Sethi/Ullman (the Dragon Book) p436
// in an empircal test using hostname data, this one resulted in less
// collisions than the function listed above.
// the two constants (24 and 0xf0000000) should be recalculated for 64-bit
//   when applicable
#define DOWNCASE(x) ( (((x) >= TEXT('A')) && ((x) <= TEXT('Z')) ) ? (((x) - TEXT('A')) + TEXT('a')) : (x) )
unsigned int StrHash::_hashValue(LPCTSTR pszStr, unsigned int nBuckets) {
    if (pszStr) {
        unsigned long h = 0L, g;
        TCHAR c;
        while((c = *(pszStr++))) {
            h = (h << 4) + ((_fCaseInsensitive ? DOWNCASE(c) : c));
            if ( (g = h & 0xf0000000) )
                h ^= (g >> 24) ^ g;
        }
        return (h % nBuckets);
    }
    return 0;
}

StrHash::StrHashNode* StrHash::_findKey(LPCTSTR pszStr, unsigned int uBucketNum) {
    StrHashNode* pshnTemp = ppshnHashChain[uBucketNum];
    while(pshnTemp) {
        if (!((_fCaseInsensitive ? StrCmpI : StrCmp)(pszStr, pshnTemp->pszKey)))
            return pshnTemp;
        pshnTemp = pshnTemp->next;
    }
    return NULL;
}

unsigned int  StrHash::_loadFactor() {
    return ( (nElements * USHRT_MAX) / nBuckets );
}

/* a small driver to test the hash function
   by reading values into stdin and reporting
   if they're duplicates -- run it against this
   perl script:

   while(<>) {
        chomp;
        if ($log{$_}++) {
       ++$dups;
    }
   }

   print "$dups duplicates.\n";

void driver_to_test_strhash_module() {
    StrHash strHash;

    char  s[4096];
    int   dups = 0;

    while(cin >> s) {
        if (strHash.insertUnique(s, 1, ((void*)1)))
            ++dups;
        else
            ;//cout << s << endl;
    }
    cout << dups << " duplicates." << endl;
}
*/

/**********************************************************************
                             OrderedList
 **********************************************************************/

// pass in uSize == 0 if you want no size limit
OrderedList::OrderedList(unsigned int uSize) {
    this->uSize = uSize;
    uCount      = 0;
    peltHead    = NULL;
}

OrderedList::~OrderedList() {
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
        delete peltTemp;
    }
}

#ifdef DEBUG
// Needed to avoid bogus leak detection
void
OrderedList::_RemoveElementsFromMemlist(){
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
        remove_from_memlist((LPVOID)peltTemp);
    }
}

void
OrderedList::_AddElementsToMemlist(){
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
        DbgAddToMemList((LPVOID)peltTemp);
    }
}


#endif //DEBUG
void OrderedList::insert(OrderedList::Element *pelt) {
    // find insertion point
    OrderedList::Element* peltPrev = NULL;
    OrderedList::Element* peltTemp = peltHead;

    if (pelt)
    {
        while(peltTemp && (peltTemp->compareWith(pelt) < 0)) {
            peltPrev = peltTemp;
            peltTemp = peltTemp->next;
        }
        if (peltPrev) {
            peltPrev->next = pelt;
            pelt->next     = peltTemp;
        }
        else {
            pelt->next = peltHead;
            peltHead   = pelt;
        }

        // is list too full?  erase smallest element
        if ((++uCount > uSize) && (uSize)) {
            ASSERT(peltHead);
            peltTemp = peltHead;
            peltHead = peltHead->next;
            delete peltTemp;
            --uCount;
        }
    }
}

// YOU must delete elements that come from this one
OrderedList::Element *OrderedList::removeFirst() {
    OrderedList::Element *peltRet = peltHead;
    if (peltHead) {
        --uCount;
        peltHead = peltHead->next;
    }
    return peltRet;
}


//
// AlignPidl
//
// Check if the pidl is dword aligned.  If not reallign them by reallocating the
// pidl. If the pidls do get reallocated the caller must free them via
// FreeRealignPidl.
//

HRESULT AlignPidl(LPCITEMIDLIST* ppidl, BOOL* pfRealigned)
{
    ASSERT(ppidl);
    ASSERT(pfRealigned);

    HRESULT hr = S_OK;

    *pfRealigned = (BOOL)((ULONG_PTR)*ppidl & 3);

    if (*pfRealigned)
        hr = (*ppidl = ILClone(*ppidl)) ? S_OK : E_OUTOFMEMORY;

    return hr;
}

void inline FreeRealignedPidl(LPCITEMIDLIST pidl)
{
    ILFree((LPITEMIDLIST)pidl);
}

//
// AlignPidls
//
// AlignPidls realigns pidls for methonds that receive an array of pidls
// (i.e. GetUIObjectOf).  In this case a new array of pidl pointer needs to get
// reallocated since we don't want to stomp on the callers pointer array.
//

HRESULT AlignPidlArray(LPCITEMIDLIST* apidl, int cidl, LPCITEMIDLIST** papidl,
                   BOOL* pfRealigned)
{
    ASSERT((apidl != NULL) || (cidl==0))
    ASSERT(pfRealigned);
    ASSERT(papidl);

    HRESULT hr = S_OK;

    *pfRealigned = FALSE;

    // Check if any pidl needs to be realigned.  If anyone needs realigning
    // realign all of them.

    for (int i = 0; i < cidl && !*pfRealigned; i++)
        *pfRealigned = (BOOL)((ULONG_PTR)apidl[i] & 3);

    if (*pfRealigned)
    {
        // Use a temp pointer in case apidl and papidl are aliased (the most
        // likely case).

        LPCITEMIDLIST* apidlTemp = (LPCITEMIDLIST*)LocalAlloc(LPTR,
                                                  cidl * sizeof(LPCITEMIDLIST));

        if (apidlTemp)
        {
            for (i = 0; i < cidl && SUCCEEDED(hr); i++)
            {
                apidlTemp[i] = ILClone(apidl[i]);

                if (NULL == apidlTemp[i])
                {
                    for (int j = 0; j < i; j++)
                        ILFree((LPITEMIDLIST)apidlTemp[j]);

                    LocalFree(apidlTemp);

                    hr = E_OUTOFMEMORY;
                }
            }

            if (SUCCEEDED(hr))
                *papidl = apidlTemp;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

void FreeRealignedPidlArray(LPCITEMIDLIST* apidl, int cidl)
{
    ASSERT(apidl)
    ASSERT(cidl > 0);

    for (int i = 0; i < cidl; i++)
        ILFree((LPITEMIDLIST)apidl[i]);

    LocalFree(apidl);

    return;
}


