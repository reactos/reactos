#include "local.h"
#include "hsfolder.h"
#include "../security.h"
#include "../favorite.h"
#include "resource.h"

#include <mluisupp.h>

#define DM_HSFOLDER 0

STDAPI  AddToFavorites(HWND hwnd, LPCITEMIDLIST pidlCur, LPCTSTR pszTitle,
                       BOOL fDisplayUI, IOleCommandTarget *pCommandTarget, IHTMLDocument2 *pDoc);

STDAPI HlinkFrameNavigateNHL(DWORD grfHLNF, LPBC pbc,
                           IBindStatusCallback *pibsc,
                           LPCWSTR pszTargetFrame,
                           LPCWSTR pszUrl,
                           LPCWSTR pszLocation);

INT_PTR CALLBACK CacheItem_PropDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK HistItem_PropDlgProc(HWND, UINT, WPARAM, LPARAM);

void MakeLegalFilenameA(LPSTR pszFilename);
void MakeLegalFilenameW(LPWSTR pszFilename);

#define MAX_ITEM_OPEN 10

//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheItem Object
//
//////////////////////////////////////////////////////////////////////////////


CHistCacheItem::CHistCacheItem() 
{
    DllAddRef();
    InitClipboardFormats();
    _cRef = 1;
    _dwDelCookie = DEL_COOKIE_WARN;
}        

CHistCacheItem::~CHistCacheItem()
{
    if (_pHCFolder)
        _pHCFolder->Release();          // release the pointer to the sf

    if (_ppcei)
    {
        for (UINT i = 0; i < _cItems; i++) 
        {
            if (_ppcei[i])
                ILFree((LPITEMIDLIST)_ppcei[i]);
        }
        LocalFree((HLOCAL)_ppcei);
    }
    
    DllRelease();
}

HRESULT CHistCacheItem::Initalize(CHistCacheFolder *pHCFolder, HWND hwnd, UINT cidl, LPCITEMIDLIST *ppidl)
{
    HRESULT hres;
    _ppcei = (LPCEIPIDL *)LocalAlloc(LPTR, cidl * sizeof(LPCEIPIDL));
    if (_ppcei)
    {
        _hwndOwner = hwnd;
        _cItems     = cidl;

        hres = S_OK;
        for (UINT i = 0; i < cidl; i++)
        {
            _ppcei[i] = (LPCEIPIDL)ILClone(ppidl[i]);
            if (!_ppcei[i])
            {
                hres = E_OUTOFMEMORY;
                break;
            }
        }

        if (SUCCEEDED(hres))
        {
            _pHCFolder = pHCFolder;
            _pHCFolder->AddRef();      // we're going to hold onto this pointer, so
        }
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}        

HRESULT CHistCacheItem_CreateInstance(CHistCacheFolder *pHCFolder, HWND hwnd,
    UINT cidl, LPCITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;                 // null the out param

    if (!_ValidateIDListArray(cidl, ppidl))
        return E_FAIL;

    CHistCacheItem *pHCItem = new CHistCacheItem;
    if (pHCItem)
    {
        hr = pHCItem->Initalize(pHCFolder, hwnd, cidl, ppidl);
        if (SUCCEEDED(hr))
            hr = pHCItem->QueryInterface(riid, ppv);
        pHCItem->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CHistCacheItem::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CHistCacheItem, IContextMenu),
        QITABENT(CHistCacheItem, IDataObject),
        QITABENT(CHistCacheItem, IExtractIconA),
        QITABENT(CHistCacheItem, IExtractIconW),
        QITABENT(CHistCacheItem, IQueryInfo),
         { 0 },
    };
    hres = QISearch(this, qit, iid, ppv);

    if (FAILED(hres) && iid == IID_IHistCache) 
    {
        *ppv = (LPVOID)this;    // for our friends
        AddRef();
        hres = S_OK;
    }
    return hres;
}

ULONG CHistCacheItem::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistCacheItem::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;   
}


//////////////////////////////////
//
// IQueryInfo Methods
//
HRESULT CHistCacheItem::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    return _pHCFolder->_GetInfoTip((LPCITEMIDLIST)_ppcei[0], dwFlags, ppwszTip);
}


HRESULT CHistCacheItem::GetInfoFlags(DWORD *pdwFlags)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST)_ppcei[0];
    LPCTSTR pszUrl = HCPidlToSourceUrl(pidl);

    *pdwFlags = QIF_CACHED; 

    if (pszUrl)
    {
        pszUrl = _StripHistoryUrlToUrl(pszUrl);

        BOOL fCached = TRUE;

        if (UrlHitsNet(pszUrl) && !UrlIsMappedOrInCache(pszUrl))
        {
            fCached = FALSE;
        }
            
        if (!fCached)
            *pdwFlags &= ~QIF_CACHED;
    }

    return S_OK;
}

//////////////////////////////////
//
// IExtractIconA Methods...
//
HRESULT CHistCacheItem::GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags)
{
    int cbIcon;

    if (_pHCFolder->_uViewType) {
        switch (_pHCFolder->_uViewType) {
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ:
        case VIEWPIDL_ORDER_TODAY:
            cbIcon = IDI_HISTURL;
            break;
        case VIEWPIDL_ORDER_SITE:
            switch(_pHCFolder->_uViewDepth) {
            case 0: cbIcon = (uFlags & GIL_OPENICON) ? IDI_HISTOPEN:IDI_HISTFOLDER; break;
            case 1: cbIcon = IDI_HISTURL; break;
            }
            break;
        }
    }
    else {
        switch (_pHCFolder->_foldertype)
        {
        case FOLDER_TYPE_Cache:
            if (ucchMax < 2) return E_FAIL;
            
            *puFlags = GIL_NOTFILENAME;
            pszIconFile[0] = '*';
            pszIconFile[1] = '\0';
            
            // "*" as the file name means iIndex is already a system icon index.
            return _pHCFolder->GetIconOf((LPCITEMIDLIST)_ppcei[0], uFlags, pniIcon);
            break;
            
        case FOLDER_TYPE_Hist:
            cbIcon = IDI_HISTWEEK;
            break;
        case FOLDER_TYPE_HistInterval:
            cbIcon = (uFlags & GIL_OPENICON) ? IDI_HISTOPEN:IDI_HISTFOLDER;
            break;
        case FOLDER_TYPE_HistDomain:
            cbIcon = IDI_HISTURL;
            break;
        default:
            return E_FAIL;
        }
    }
    *puFlags = 0;
    *pniIcon = -cbIcon;
    StrCpyNA(pszIconFile, "shdocvw.dll", ucchMax);
    return S_OK;
}

HRESULT CHistCacheItem::Extract(LPCSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize)
{
    return S_FALSE;
}

//////////////////////////////////
//
// IExtractIconW Methods...
//
HRESULT CHistCacheItem::GetIconLocation(UINT uFlags, LPWSTR pwzIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags)
{
    CHAR szIconFile[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, szIconFile, ARRAYSIZE(szIconFile), pniIcon, puFlags);
    if (SUCCEEDED(hr))
        AnsiToUnicode(szIconFile, pwzIconFile, ucchMax);
    return hr;
}

HRESULT CHistCacheItem::Extract(LPCWSTR pcwzFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize)
{
    CHAR szFile[MAX_PATH];
    UnicodeToAnsi(pcwzFile, szFile, ARRAYSIZE(szFile));
    return Extract(szFile, uIconIndex, phiconLarge, phiconSmall, ucIconSize);
}

//////////////////////////////////
//
// IContextMenu Methods
//
HRESULT CHistCacheItem::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,UINT idCmdLast, UINT uFlags)
{
    USHORT cItems;

    TraceMsg(DM_HSFOLDER, "hci - cm - QueryContextMenu() called.");
    
    if ((uFlags & CMF_VERBSONLY) || (uFlags & CMF_DVFILE))
    {
        cItems = MergePopupMenu(&hmenu, POPUP_CONTEXT_URL_VERBSONLY, 0, indexMenu, 
            idCmdFirst, idCmdLast);
    
    }
    else  // (uFlags & CMF_NORMAL)
    {
        UINT idResource = POPUP_CACHECONTEXT_URL;

        // always use the cachecontext menu unless:
        if ( ((_pHCFolder->_uViewType == VIEWPIDL_ORDER_SITE) &&
              (_pHCFolder->_uViewDepth == 0))                      ||
             (!IsLeaf(_pHCFolder->_foldertype)) )
            idResource = POPUP_HISTORYCONTEXT_URL;

        cItems = MergePopupMenu(&hmenu, idResource, 0, indexMenu, idCmdFirst, idCmdLast);

        if (IsInetcplRestricted(L"History"))
        {
            DeleteMenu(hmenu, RSVIDM_DELCACHE + idCmdFirst, MF_BYCOMMAND);
            _SHPrettyMenu(hmenu);
        }
    }
    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    return ResultFromShort(cItems);    // number of menu items    
}


LPCTSTR CHistCacheItem::_GetUrl(int nIndex)
{
    LPCTSTR pszUrl = NULL;

    if (IsHistory(_pHCFolder->_foldertype))
        pszUrl = _StripHistoryUrlToUrl(HCPidlToSourceUrl((LPCITEMIDLIST)_ppcei[nIndex]));
    else
        pszUrl = HCPidlToSourceUrl((LPCITEMIDLIST)_ppcei[nIndex]);

    return pszUrl;
}


// Return value:
//               TRUE - URL is Safe.
//               FALSE - URL is questionable and needs to be re-zone checked w/o PUAF_NOUI.
BOOL CHistCacheItem::_ZoneCheck(int nIndex, DWORD dwUrlAction)
{
    LPCTSTR pszUrl = _GetUrl(nIndex);

    // Are we dealing with the history folder?
    if (IsHistory(_pHCFolder->_foldertype))
    {
        // Yes, then consider anything that is not
        // a FILE URL safe.

        int nScheme = GetUrlScheme(pszUrl);
        if (URL_SCHEME_FILE != nScheme)
            return TRUE;        // It's safe because it's not a file URL.
    }

    if (S_OK != ZoneCheckUrl(pszUrl, dwUrlAction, PUAF_NOUI, NULL))
        return FALSE;

    return TRUE;
}


HRESULT CHistCacheItem::_AddToFavorites(int nIndex)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlUrl = NULL;
    TCHAR szParsedUrl[MAX_URL_STRING];

    // NOTE: This URL came from the user, so we need to clean it up.
    //       If the user entered "yahoo.com" or "Search Get Rich Quick",
    //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
    DWORD cchParsedUrl = ARRAYSIZE(szParsedUrl);
    if (!ParseURLFromOutsideSource(_GetUrl(nIndex), szParsedUrl, &cchParsedUrl, NULL))
    {
        StrCpyN(szParsedUrl, _GetUrl(nIndex), ARRAYSIZE(szParsedUrl));
    } 

    hr = IEParseDisplayName(CP_ACP, szParsedUrl, &pidlUrl);
    if (SUCCEEDED(hr))
    {
        LPCTSTR pszTitle = _GetURLTitle(_ppcei[nIndex]);
        if ((pszTitle == NULL) || (lstrlen(pszTitle) == 0))
            pszTitle = _GetUrl(nIndex);
        AddToFavorites(_hwndOwner, pidlUrl, pszTitle, TRUE, NULL, NULL);
        ILFree(pidlUrl);
        hr = S_OK;
    }

    return hr;
}


STDMETHODIMP CHistCacheItem::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT i;
    int idCmd = _GetCmdID(pici->lpVerb);
    HRESULT hres = S_OK;
    DWORD dwAction;
    BOOL fCancelCopyAndOpen = FALSE;
    BOOL fZonesUI = FALSE;
    BOOL fMustFlushNotify = FALSE;
    BOOL fBulkDelete;

    TraceMsg(DM_HSFOLDER, "hci - cm - InvokeCommand() called.");

    if (idCmd == RSVIDM_DELCACHE)
    {
        TCHAR szBuff[INTERNET_MAX_URL_LENGTH+MAX_PATH];
        TCHAR szFormat[MAX_PATH];
                
        if (IsHistory(_pHCFolder->_foldertype))
        {
            if (_cItems == 1)          
            {
                TCHAR szTitle[MAX_URL_STRING];

                if (_pHCFolder->_foldertype != FOLDER_TYPE_Hist)
                {
                    _GetURLDispName(_ppcei[0], szTitle, ARRAYSIZE(szTitle));
                }
                else
                {
                    FILETIME ftStart, ftEnd;
                    LPCTSTR pszIntervalName = _GetURLTitle(_ppcei[0]);

                    if (SUCCEEDED(_ValueToIntervalW(pszIntervalName, &ftStart, &ftEnd)))
                    {
                        GetDisplayNameForTimeInterval(&ftStart, &ftEnd, szTitle, ARRAYSIZE(szTitle));
                    }
                }

                MLLoadString(IDS_WARN_DELETE_HISTORYITEM, szFormat, ARRAYSIZE(szFormat));
                wnsprintf(szBuff, ARRAYSIZE(szBuff), szFormat, szTitle);
            }
            else
            {
                MLLoadString(IDS_WARN_DELETE_MULTIHISTORY, szFormat, ARRAYSIZE(szFormat));
                wnsprintf(szBuff, ARRAYSIZE(szBuff), szFormat, _cItems);
            }
            if (DialogBoxParam(MLGetHinst(),
                                 MAKEINTRESOURCE(DLG_HISTCACHE_WARNING),
                                 pici->hwnd,
                                 HistoryConfirmDeleteDlgProc,
                                 (LPARAM)szBuff) != IDYES)
            {
                return S_FALSE;
            }
            return _pHCFolder->_DeleteItems((LPCITEMIDLIST *)_ppcei, _cItems);
        }
    }

    // ZONES SECURITY CHECK.
    //
    // We need to cycle through each action and Zone Check the URLs.
    // We pass NOUI when zone checking the URLs because we don't want info
    // displayed to the user.  We will stop when we find the first questionable
    // URL.  We will then 
    for (i = 0; (i < _cItems) && !fZonesUI; i++)
    {
        if (_ppcei[i]) 
        {
            switch (idCmd)
            {
            case RSVIDM_OPEN:
                if ((i < MAX_ITEM_OPEN) && _pHCFolder->_IsLeaf())
                {
                    if (!_ZoneCheck(i, URLACTION_SHELL_VERB))
                    {
                        fZonesUI = TRUE;
                        dwAction = URLACTION_SHELL_VERB;
                    }
                }
                break;

            case RSVIDM_COPY:
                if (_pHCFolder->_IsLeaf())
                {
                    if (!_ZoneCheck(i, URLACTION_SHELL_MOVE_OR_COPY))
                    {
                        fZonesUI = TRUE;
                        dwAction = URLACTION_SHELL_MOVE_OR_COPY;
                    }
                }
                break;
            }
        }
    }

    if (fZonesUI)
    {
        LPCTSTR pszUrl = _GetUrl(i-1);  // Sub 1 because of for loop above.
        if (S_OK != ZoneCheckUrl(pszUrl, dwAction, PUAF_DEFAULT|PUAF_WARN_IF_DENIED, NULL))
        {
            // The user cannot do this or does not want to do this.
            fCancelCopyAndOpen = TRUE;
        }
    }

    i = _cItems;
    fBulkDelete = i > LOTS_OF_FILES;

    // fCancelCopyAndOpen happens if the user cannot or chose not to proceed.
    while (i && !fCancelCopyAndOpen)
    {
        i--;
        if (_ppcei[i]) 
        {

            switch (idCmd)
            {
            case RSVIDM_OPEN:
                ASSERT(!_pHCFolder->_uViewType);
                if (i >= MAX_ITEM_OPEN)
                {
                    hres = S_FALSE;
                    goto Done;
                }

                if (!IsLeaf(_pHCFolder->_foldertype))
                {
                    LPITEMIDLIST pidlOpen;
                    
                    hres = S_FALSE;
                    pidlOpen = ILCombine(_pHCFolder->_pidl,(LPITEMIDLIST)_ppcei[i]);
                    if (pidlOpen)
                    {
                        IShellBrowser *psb = FileCabinet_GetIShellBrowser(_hwndOwner);
                        if (psb)
                        {
                            psb->AddRef();
                            psb->BrowseObject(pidlOpen, 
                                        (i==_cItems-1) ? SBSP_DEFBROWSER:SBSP_NEWBROWSER);
                            psb->Release();
                            hres = S_OK;
                        }
                        else
                        {
                            hres = _LaunchAppForPidl(pici->hwnd, pidlOpen);
                        }
                        ILFree(pidlOpen);
                    }
                }
                else
                {
                    if (!IsHistory(_pHCFolder->_foldertype) && 
                        (CEI_CACHEENTRYTYPE(_ppcei[i]) & COOKIE_CACHE_ENTRY))
                    {
                        ASSERT(PathFindExtension(CEI_LOCALFILENAME(_ppcei[i])) && \
                            !StrCmpI(PathFindExtension(CEI_LOCALFILENAME(_ppcei[i])),TEXT(".txt")));
                        hres = _LaunchApp(pici->hwnd, CEI_LOCALFILENAME(_ppcei[i]));
                    }
                    else
                    {
                        TCHAR szDecoded[MAX_URL_STRING];
                        ConditionallyDecodeUTF8(_GetUrl(i), szDecoded, ARRAYSIZE(szDecoded));

                        hres = _LaunchApp(pici->hwnd, szDecoded);
                    }
                }
                break;

            case RSVIDM_ADDTOFAVORITES:
                hres = _AddToFavorites(i);
                goto Done;
            case RSVIDM_OPEN_NEWWINDOW:
                {
                    TCHAR szDecoded[MAX_URL_STRING];
                    ConditionallyDecodeUTF8(_GetUrl(i), szDecoded, ARRAYSIZE(szDecoded));
                    LPWSTR pwszTarget;
                    
                    if (SUCCEEDED((hres = SHStrDup(szDecoded, &pwszTarget)))) {
                        hres = NavToUrlUsingIEW(pwszTarget, TRUE);
                        CoTaskMemFree(pwszTarget);
                    }
                    goto Done;
                }
            case RSVIDM_COPY:
                if (!_pHCFolder->_IsLeaf())
                {
                    hres = E_FAIL;
                }
                else
                {
                    OleSetClipboard((IDataObject *)this);
                }
                goto Done;

            case RSVIDM_DELCACHE:
                ASSERT(!_pHCFolder->_uViewType);                
                if (IsHistory(_pHCFolder->_foldertype))
                {
                    hres = E_FAIL;
                }
                else
                {
                    // pop warning msg for cookie only once
                    if ((CEI_CACHEENTRYTYPE(_ppcei[i]) & COOKIE_CACHE_ENTRY) &&     
                        (_dwDelCookie == DEL_COOKIE_WARN ))
                    {
                        if(CachevuWarningDlg(_ppcei[i], IDS_WARN_DELETE_CACHE, pici->hwnd))
                            _dwDelCookie = DEL_COOKIE_YES;
                        else
                            _dwDelCookie = DEL_COOKIE_NO;
                    }

                    if ((CEI_CACHEENTRYTYPE(_ppcei[i]) & COOKIE_CACHE_ENTRY) &&     
                        (_dwDelCookie == DEL_COOKIE_NO ))
                        continue;
              
                    if (DeleteUrlCacheEntry(HCPidlToSourceUrl((LPCITEMIDLIST)_ppcei[i])))
                    {
                        if (!fBulkDelete)
                        {
                            _GenerateEvent(SHCNE_DELETE, _pHCFolder->_pidl, (LPITEMIDLIST)(_ppcei[i]), NULL);
                        }
                        fMustFlushNotify = TRUE;
                    }
                    else 
                        hres = E_FAIL;
                }
                break;

            case RSVIDM_PROPERTIES:
                // NOTE: We'll probably want to split this into two cases
                // and call a function in each case
                //
                if (IsLeaf(_pHCFolder->_foldertype))
                {
                    if (IsHistory(_pHCFolder->_foldertype)) {
                        // this was a bug in IE4, too:
                        //   the pidl is re-created so that it has the most up-to-date information
                        //   possible -- this way we can avoid assuming that the NSC has cached the
                        //   most up-to-date pidl (which, in most cases, it hasn't)
                        LPHEIPIDL pidlTemp =
                            _pHCFolder->_CreateHCacheFolderPidlFromUrl(FALSE,
                                                                       HCPidlToSourceUrl((LPITEMIDLIST)_ppcei[i]));
                        if (pidlTemp) {
                            _CreatePropSheet(pici->hwnd, (LPCEIPIDL)pidlTemp,
                                             DLG_HISTITEMPROP, HistItem_PropDlgProc);
                            LocalFree(pidlTemp);
                        }
                    }
                    else
                        _CreatePropSheet(pici->hwnd, _ppcei[i], DLG_CACHEITEMPROP, CacheItem_PropDlgProc);
                }
                else
                {
                    hres = E_FAIL;
                }
                goto Done;

            default:
                hres = E_FAIL;
                break;
            }
            
            ASSERT(SUCCEEDED(hres));
            if (FAILED(hres))
                TraceMsg(DM_HSFOLDER, "Cachevu failed the command at: %s", HCPidlToSourceUrl((LPCITEMIDLIST)_ppcei[i]));
        }
    }
Done:
    if (fMustFlushNotify)
    {
        if (fBulkDelete)
        {
            ASSERT(!_pHCFolder->_uViewType);
            _GenerateEvent(SHCNE_UPDATEDIR, _pHCFolder->_pidl, NULL, NULL);
        }

        SHChangeNotifyHandleEvents();
    }
    return hres;
}


STDMETHODIMP CHistCacheItem::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved,
                                LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_FAIL;

    TraceMsg(DM_HSFOLDER, "hci - cm - GetCommandString() called.");

    if ((uFlags == GCS_VERBA) || (uFlags == GCS_VERBW))
    {
        LPCSTR pszSrc = NULL;

        switch(idCmd)
        {
            case RSVIDM_OPEN:
                pszSrc = c_szOpen;
                break;

            case RSVIDM_COPY:
                pszSrc = c_szCopy;
                break;

            case RSVIDM_DELCACHE:
                pszSrc = c_szDelcache;
                break;

            case RSVIDM_PROPERTIES:
                pszSrc = c_szProperties;
                break;
        }
        
        if (pszSrc)
        {
            if (uFlags == GCS_VERBA)
                StrCpyNA(pszName, pszSrc, cchMax);
            else if (uFlags == GCS_VERBW) // GCS_VERB === GCS_VERBW
                SHAnsiToUnicode(pszSrc, (LPWSTR)pszName, cchMax);
            else
                ASSERT(0);
            hres = S_OK;
        }
    }
    
    else if (uFlags == GCS_HELPTEXTA || uFlags == GCS_HELPTEXTW)
    {
        switch(idCmd)
        {
            case RSVIDM_OPEN:
            case RSVIDM_COPY:
            case RSVIDM_DELCACHE:
            case RSVIDM_PROPERTIES:
                if (uFlags == GCS_HELPTEXTA)
                {
                    MLLoadStringA(IDS_SB_FIRST+ (UINT)idCmd, pszName, cchMax);
                }
                else
                {
                    MLLoadStringW(IDS_SB_FIRST+ (UINT)idCmd, (LPWSTR)pszName, cchMax);
                }
                hres = NOERROR;
                break;

            default:
                break;
        }
    }
    return hres;
}


//////////////////////////////////
//
// IDataObject Methods...
//

HRESULT CHistCacheItem::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    HRESULT hres;

#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wnsprintf(szName, ARRAYSIZE(szName), TEXT("#%d"), pFEIn->cfFormat);

    TraceMsg(DM_HSFOLDER, "hci - do - GetData(%s)", szName);
#endif

    pSTM->hGlobal = NULL;
    pSTM->pUnkForRelease = NULL;

    if (IsHistory(_pHCFolder->_foldertype))
    {

        if ((pFEIn->cfFormat == g_cfFileDescW) && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateFileDescriptorW(pSTM);

        else if ((pFEIn->cfFormat == g_cfFileDescA) && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateFileDescriptorA(pSTM);

        else if ((pFEIn->cfFormat == g_cfFileContents) && (pFEIn->tymed & TYMED_ISTREAM))
            hres = _CreateFileContents(pSTM, pFEIn->lindex);

        else if (pFEIn->cfFormat == CF_UNICODETEXT && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateUnicodeTEXT(pSTM);

        else if (pFEIn->cfFormat == CF_TEXT && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateHTEXT(pSTM);

        else if (pFEIn->cfFormat == g_cfURL && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateURL(pSTM);

        else if ((pFEIn->cfFormat == g_cfPreferedEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreatePrefDropEffect(pSTM);
   
        else
            hres = DATA_E_FORMATETC;
    }
    else
    {   
        if (pFEIn->cfFormat == CF_HDROP && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreateHDROP(pSTM);

        else if ((pFEIn->cfFormat == g_cfPreferedEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
            hres = _CreatePrefDropEffect(pSTM);
   
        else
            hres = DATA_E_FORMATETC;
    }
    
    return hres;

}

HRESULT CHistCacheItem::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    TraceMsg(DM_HSFOLDER, "hci - do - GetDataHere() called.");
    return E_NOTIMPL;
}

HRESULT CHistCacheItem::QueryGetData(LPFORMATETC pFEIn)
{
#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wnsprintf(szName, ARRAYSIZE(szName), TEXT("#%d"), pFEIn->cfFormat);

    TraceMsg(DM_HSFOLDER, "hci - do - QueryGetData(%s)", szName);
#endif

    if (IsHistory(_pHCFolder->_foldertype))
    {
        if (pFEIn->cfFormat == g_cfFileDescW ||
            pFEIn->cfFormat == g_cfFileDescA ||
            pFEIn->cfFormat == g_cfFileContents   ||
            pFEIn->cfFormat == g_cfURL            ||
            pFEIn->cfFormat == CF_UNICODETEXT     ||
            pFEIn->cfFormat == CF_TEXT            ||
            pFEIn->cfFormat == g_cfPreferedEffect)
        {
            TraceMsg(DM_HSFOLDER, "		   format supported.");
            return NOERROR;
        }
    }

    else
    {
        if (pFEIn->cfFormat == CF_HDROP            || 
            pFEIn->cfFormat == g_cfPreferedEffect)
        {
            TraceMsg(DM_HSFOLDER, "		   format supported.");
            return NOERROR;
        }
    }
    
    return S_FALSE;
}

HRESULT CHistCacheItem::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
    TraceMsg(DM_HSFOLDER, "hci - do - GetCanonicalFormatEtc() called.");
    return DATA_S_SAMEFORMATETC;
}

HRESULT CHistCacheItem::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    TraceMsg(DM_HSFOLDER, "hci - do - SetData() called.");
    return E_NOTIMPL;
}

HRESULT CHistCacheItem::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum)
{
    if (IsHistory(_pHCFolder->_foldertype))
    {
        FORMATETC Histfmte[] = {
            {g_cfFileDescW,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {g_cfFileDescA,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {g_cfFileContents,   NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM },
            {g_cfURL,            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {CF_UNICODETEXT,     NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {CF_TEXT,            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {g_cfPreferedEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };
        return SHCreateStdEnumFmtEtc(ARRAYSIZE(Histfmte), Histfmte, ppEnum);
    }
    else
    {
        FORMATETC Cachefmte[] = {
            {CF_HDROP,                NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {g_cfPreferedEffect,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };
        return SHCreateStdEnumFmtEtc(ARRAYSIZE(Cachefmte), Cachefmte, ppEnum);
    }
}

HRESULT CHistCacheItem::DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CHistCacheItem::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CHistCacheItem::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////////
//
// Helper Routines
//
//////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK HistItem_PropDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPHEIPIDL phei = lpPropSheet ? (LPHEIPIDL)lpPropSheet->lParam : NULL;

    switch(message) {

        case WM_INITDIALOG:
        {
            SHFILEINFO sfi;
            TCHAR szBuf[80];
            TCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];

            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            phei = (LPHEIPIDL)((LPPROPSHEETPAGE)lParam)->lParam;

            SHGetFileInfo(TEXT(".url"), 0, &sfi, SIZEOF(sfi), SHGFI_ICON |
                SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_TYPENAME);

            SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0);
            
            _GetURLTitleForDisplay((LPCEIPIDL)phei, szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            SetDlgItemText(hDlg, IDD_TITLE, szDisplayUrl);
            
            SetDlgItemText(hDlg, IDD_FILETYPE, sfi.szTypeName);
            
            ConditionallyDecodeUTF8(_GetUrlForPidl((LPCITEMIDLIST)phei), szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            SetDlgItemText(hDlg, IDD_INTERNET_ADDRESS, szDisplayUrl);
            
            FileTimeToDateTimeStringInternal(&phei->ftLastVisited, szBuf, ARRAYSIZE(szBuf), FALSE);
            SetDlgItemText(hDlg, IDD_LAST_VISITED, szBuf);

            // It looks like the hitcount is double what it is supposed to be
            //  (ie - navigate to a site and hitcount += 2)
            // For now, we'll just half the hitcount before we display it:
            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%d"), (phei->dwNumHits)/2) ;
            SetDlgItemText(hDlg, IDD_NUMHITS, szBuf);

            break;            
        }
        
        
        case WM_DESTROY:
            {
                HICON hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_GETICON, 0, 0);
                if (hIcon)
                    DestroyIcon(hIcon);
            }
            break;

        case WM_COMMAND:
        case WM_HELP:
        case WM_CONTEXTMENU:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

INT_PTR CALLBACK CacheItem_PropDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPCEIPIDL pcei = lpPropSheet ? (LPCEIPIDL)lpPropSheet->lParam : NULL;

    switch(message) {

        case WM_INITDIALOG: {
            SHFILEINFO sfi;
            TCHAR szBuf[80];
            
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            pcei = (LPCEIPIDL)((LPPROPSHEETPAGE)lParam)->lParam;

            // get the icon and file type strings

            SHGetFileInfo(CEI_LOCALFILENAME(pcei), 0, &sfi, SIZEOF(sfi), SHGFI_ICON | SHGFI_TYPENAME);

            SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0);

            // set the info strings
            SetDlgItemText(hDlg, IDD_HSFURL, HCPidlToSourceUrl((LPCITEMIDLIST)pcei));
            SetDlgItemText(hDlg, IDD_FILETYPE, sfi.szTypeName);

            SetDlgItemText(hDlg, IDD_FILESIZE, StrFormatByteSize(pcei->cei.dwSizeLow, szBuf, ARRAYSIZE(szBuf)));
            SetDlgItemText(hDlg, IDD_CACHE_NAME, PathFindFileName(CEI_LOCALFILENAME(pcei)));
            FileTimeToDateTimeStringInternal(&pcei->cei.ExpireTime, szBuf, ARRAYSIZE(szBuf), FALSE);
            SetDlgItemText(hDlg, IDD_EXPIRES, szBuf);
            FileTimeToDateTimeStringInternal(&pcei->cei.LastModifiedTime, szBuf, ARRAYSIZE(szBuf), FALSE);
            SetDlgItemText(hDlg, IDD_LAST_MODIFIED, szBuf);
            FileTimeToDateTimeStringInternal(&pcei->cei.LastAccessTime, szBuf, ARRAYSIZE(szBuf), FALSE);
            SetDlgItemText(hDlg, IDD_LAST_ACCESSED, szBuf);
            
            break;
        }

        case WM_DESTROY:
            {
                HICON hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_GETICON, 0, 0);
                if (hIcon)
                    DestroyIcon(hIcon);
            }
            break;

        case WM_COMMAND:
        case WM_HELP:
        case WM_CONTEXTMENU:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

HRESULT CHistCacheItem::_CreateFileDescriptorA(LPSTGMEDIUM pSTM)
{
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;

    FILEGROUPDESCRIPTORA *pfgd = (FILEGROUPDESCRIPTORA*)GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTORA) + (_cItems-1) * sizeof(FILEDESCRIPTORA));
    if (pfgd == NULL)
    {
        TraceMsg(DM_HSFOLDER, "hci -   Couldn't alloc file descriptor");
        return E_OUTOFMEMORY;
    }
    
    pfgd->cItems = _cItems;     // set the number of items

    for (UINT i = 0; i < _cItems; i++)
    {

        FILEDESCRIPTORA *pfd = &(pfgd->fgd[i]);
        UINT cchFilename;
        
        SHTCharToAnsi(_GetURLTitle(_ppcei[i]), pfd->cFileName, ARRAYSIZE(pfd->cFileName) );
        
        MakeLegalFilenameA(pfd->cFileName);

        cchFilename = lstrlenA(pfd->cFileName);
        SHTCharToAnsi(L".URL", pfd->cFileName+cchFilename, ARRAYSIZE(pfd->cFileName)-cchFilename);

    }

    pSTM->hGlobal = pfgd;
    
    return S_OK;
}
    
HRESULT CHistCacheItem::_CreateFileDescriptorW(LPSTGMEDIUM pSTM)
{
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    FILEGROUPDESCRIPTORW *pfgd = (FILEGROUPDESCRIPTORW*)GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTORW) + (_cItems-1) * sizeof(FILEDESCRIPTORW));
    if (pfgd == NULL)
    {
        TraceMsg(DM_HSFOLDER, "hci -   Couldn't alloc file descriptor");
        return E_OUTOFMEMORY;
    }
    
    pfgd->cItems = _cItems;     // set the number of items

    for (UINT i = 0; i < _cItems; i++)
    {
        FILEDESCRIPTORW *pfd = &(pfgd->fgd[i]);
        
        _GetURLTitleForDisplay(_ppcei[i], pfd->cFileName, ARRAYSIZE(pfd->cFileName));
        
        MakeLegalFilenameW(pfd->cFileName);

        UINT cchFilename = lstrlenW(pfd->cFileName);
        SHTCharToUnicode(L".URL", pfd->cFileName+cchFilename, ARRAYSIZE(pfd->cFileName)-cchFilename);

    }

    pSTM->hGlobal = pfgd;
    
    return S_OK;
}

// this format is explicitly ANSI, hence no TCHAR stuff

HRESULT CHistCacheItem::_CreateURL(LPSTGMEDIUM pSTM)
{
    DWORD cchSize;
    LPCTSTR pszURL = _StripHistoryUrlToUrl(HCPidlToSourceUrl((LPCITEMIDLIST)_ppcei[0]));
    if (!pszURL)
        return E_FAIL;
    
    // render the url
    cchSize = lstrlen(pszURL) + 1;

    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cchSize * sizeof(CHAR));
    if (pSTM->hGlobal)
    {
        TCharToAnsi(pszURL, (LPSTR)pSTM->hGlobal, cchSize);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


HRESULT CHistCacheItem::_CreatePrefDropEffect(LPSTGMEDIUM pSTM)
{
    ASSERT(!_pHCFolder->_uViewType);    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    pSTM->hGlobal = GlobalAlloc(GPTR, sizeof(DWORD));

    if (pSTM->hGlobal)
    {
        *((LPDWORD)pSTM->hGlobal) = DROPEFFECT_COPY;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}


HRESULT CHistCacheItem::_CreateFileContents(LPSTGMEDIUM pSTM, LONG lindex)
{
    HRESULT hr;
    
    // make sure the index is in a valid range.
    ASSERT((unsigned)lindex < _cItems);
    ASSERT(lindex >= 0);

    // here's a partial fix for when ole sometimes passes in -1 for lindex
    if (lindex == -1)
    {
        if (_cItems == 1)
            lindex = 0;
        else
            return E_FAIL;
    }
    
    pSTM->tymed = TYMED_ISTREAM;
    pSTM->pUnkForRelease = NULL;
    
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pSTM->pstm);
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER li = {0L, 0L};
        IUniformResourceLocator *purl;

    	hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
	        IID_IUniformResourceLocator, (void **)&purl);
        if (SUCCEEDED(hr))
        {
            TCHAR szDecoded[MAX_URL_STRING];

            ConditionallyDecodeUTF8(_GetUrlForPidl((LPCITEMIDLIST)_ppcei[lindex]), 
                szDecoded, ARRAYSIZE(szDecoded));

            hr = purl->SetURL(szDecoded, TRUE);

            if (SUCCEEDED(hr))
            {
                IPersistStream *pps;
                hr = purl->QueryInterface(IID_IPersistStream, (LPVOID *)&pps);
                if (SUCCEEDED(hr))
                {
                    hr = pps->Save(pSTM->pstm, TRUE);
                    pps->Release();
                }
            }
            purl->Release();
        }               
        pSTM->pstm->Seek(li, STREAM_SEEK_SET, NULL);
    }

    return hr;
}


HRESULT CHistCacheItem::_CreateHTEXT(LPSTGMEDIUM pSTM)
{
    UINT i;
    UINT cbAlloc = sizeof(TCHAR);        // null terminator
    TCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];
    

    for (i = 0; i < _cItems; i++)
    {
        LPCTSTR pszUrl = _GetDisplayUrlForPidl((LPCITEMIDLIST)_ppcei[i], szDisplayUrl, ARRAYSIZE(szDisplayUrl));
        if (!pszUrl)
            return E_FAIL;

        char szAnsiUrl[MAX_URL_STRING];
        TCharToAnsi(pszUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));

        // 2 extra for carriage return and newline
        cbAlloc += sizeof(CHAR) * (lstrlenA(szAnsiUrl) + 2);  
    }

    // render the url
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cbAlloc);

    if (pSTM->hGlobal)
    {
        LPSTR  pszHTEXT = (LPSTR)pSTM->hGlobal;
        int    cchHTEXT = cbAlloc / sizeof(CHAR);

        for (i = 0; i < _cItems; i++)
        {
            if (i && cchHTEXT > 2)
            {
                *pszHTEXT++ = 0xD;
                *pszHTEXT++ = 0xA;
                cchHTEXT -= 2;
            }

            LPCTSTR pszUrl = _GetDisplayUrlForPidl((LPCITEMIDLIST)_ppcei[i], szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            int     cchUrl = lstrlen(pszUrl);

            TCharToAnsi(pszUrl, pszHTEXT, cchHTEXT);

            pszHTEXT += cchUrl;
            cchHTEXT -= cchUrl;
        }
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


HRESULT CHistCacheItem::_CreateUnicodeTEXT(LPSTGMEDIUM pSTM)
{
    UINT i;
    UINT cbAlloc = sizeof(WCHAR);        // null terminator
    WCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];

    for (i = 0; i < _cItems; i++)
    {
        ConditionallyDecodeUTF8(_GetUrlForPidl((LPCITEMIDLIST)_ppcei[i]), 
            szDisplayUrl, ARRAYSIZE(szDisplayUrl));

        if (!*szDisplayUrl)
            return E_FAIL;

        cbAlloc += sizeof(WCHAR) * (lstrlenW(szDisplayUrl) + 2);
    }

    // render the url
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cbAlloc);

    if (pSTM->hGlobal)
    {
        LPTSTR pszHTEXT = (LPTSTR)pSTM->hGlobal;
        int    cchHTEXT = cbAlloc / sizeof(WCHAR);

        for (i = 0; i < _cItems; i++)
        {
            if (i && cchHTEXT > 2)
            {
                *pszHTEXT++ = 0xD;
                *pszHTEXT++ = 0xA;
                cchHTEXT -= 2;
            }

            ConditionallyDecodeUTF8(_GetUrlForPidl((LPCITEMIDLIST)_ppcei[i]), 
                szDisplayUrl, ARRAYSIZE(szDisplayUrl));

            int     cchUrl = lstrlenW(szDisplayUrl);

            StrCpyN(pszHTEXT, szDisplayUrl, cchHTEXT);

            pszHTEXT += cchUrl;
            cchHTEXT -= cchUrl;
        }
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


// use CEI_LOCALFILENAME to get the file name for the HDROP, but map that
// to the final file name (store in the file system) through the "FileNameMap"
// data which uses _GetURLTitle() as the final name of the file.

HRESULT CHistCacheItem::_CreateHDROP(STGMEDIUM *pmedium)
{
    ASSERT(!_pHCFolder->_uViewType);
    UINT i;
    UINT cbAlloc = sizeof(DROPFILES) + sizeof(CHAR);        // header + null terminator

    for (i = 0; i < _cItems; i++)
    {
        char szAnsiUrl[MAX_URL_STRING];
        
        SHTCharToAnsi(CEI_LOCALFILENAME(_ppcei[i]), szAnsiUrl, ARRAYSIZE(szAnsiUrl));
        cbAlloc += sizeof(CHAR) * (lstrlenA(szAnsiUrl) + 1);
    }

    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->pUnkForRelease = NULL;
    pmedium->hGlobal = GlobalAlloc(GPTR, cbAlloc);
    if (pmedium->hGlobal)
    {
        LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;
        LPSTR pszFiles  = (LPSTR)(pdf + 1);
        int   cchFiles  = (cbAlloc - sizeof(DROPFILES) - sizeof(CHAR));
        pdf->pFiles = sizeof(DROPFILES);
        pdf->fWide = FALSE;

        for (i = 0; i < _cItems; i++)
        {
            LPTSTR pszPath = CEI_LOCALFILENAME(_ppcei[i]);
            int    cchPath = lstrlen(pszPath);

            SHTCharToAnsi(pszPath, pszFiles, cchFiles);

            pszFiles += cchPath + 1;
            cchFiles -= cchPath + 1;

            ASSERT((UINT)((LPBYTE)pszFiles - (LPBYTE)pdf) < cbAlloc);
        }
        ASSERT((LPSTR)pdf + cbAlloc - 1 == pszFiles);
        ASSERT(*pszFiles == 0); // zero init alloc

        return NOERROR;

    }
    return E_OUTOFMEMORY;
}

//
// These routines make a string into a legal filename by replacing
// all invalid characters with spaces.
//
// The list of invalid characters was obtained from the NT error
// message you get when you try to rename a file to an invalid name.
//

#ifndef UNICODE
#error The MakeLegalFilename code only works when it's part of a UNICODE build
#endif

//
// This function takes a string and makes it into a
// valid filename (by calling PathCleanupSpec).
//
// The PathCleanupSpec function wants to know what
// directory the file will live in.  But it's going
// on the clipboard, so we don't know.  We just
// guess the desktop.
//
// It only uses this path to decide if the filesystem
// supports long filenames or not, and to check for
// MAX_PATH overflow.
//
void MakeLegalFilenameW(LPWSTR pszFilename)
{
    WCHAR szDesktopPath[MAX_PATH];

    GetWindowsDirectoryW(szDesktopPath,MAX_PATH);
    PathCleanupSpec(szDesktopPath,pszFilename);

}

//
// ANSI wrapper for above function
//
void MakeLegalFilenameA(LPSTR pszFilename)
{
    WCHAR szFilenameW[MAX_PATH];

    SHAnsiToUnicode(pszFilename, szFilenameW, MAX_PATH);

    MakeLegalFilenameW(szFilenameW);

    SHUnicodeToAnsi(szFilenameW, pszFilename, MAX_PATH);

}
