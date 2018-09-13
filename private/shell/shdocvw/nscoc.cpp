// NscOc.cpp : Implementation of CShellFavoritesNameSpace
#include "priv.h"
#include "nscoc.h"
#include "favorite.h"   //for UpdateSubscription()
#include "subsmgr.h"    //for CLSID_SubscriptionMgr

// HTML displays hard scripting errors if methods on automation interfaces
// return FAILED().  This macro will fix these.
#define FIX_SCRIPTING_ERRORS(hr)        (FAILED(hr) ? S_FALSE : hr)


/////////////////////////////////////////////////////////////////////////////
// CShellFavoritesNameSpace
CShellFavoritesNameSpace::CShellFavoritesNameSpace()
{
    ASSERT(!_pidlBase); //assert that shell always zeroinits
    ASSERT(!m_dwSafety);
    ASSERT(!_pns);
    ASSERT(!_psfns);
    m_bWindowOnly = TRUE;
}

CShellFavoritesNameSpace::~CShellFavoritesNameSpace()
{
    ATOMICRELEASE(_pns);
    ATOMICRELEASE(_psfns);
    ATOMICRELEASE(_pweh);

    if (_pidlBase)
        ILFree(_pidlBase);
}

HRESULT CShellFavoritesNameSpace::OnDraw(ATL_DRAWINFO& di)
{
    //should only get called before CNscTree is initialized
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::ResetSort()
{
    ASSERT(_psfns);
    if (_psfns)
        return _psfns->ResetSort();
    return S_OK;
}


STDMETHODIMP CShellFavoritesNameSpace::MoveSelectionDown()
{
    ASSERT(_psfns);
    if (_psfns)
        return FIX_SCRIPTING_ERRORS(_psfns->MoveSelectionDown());
    return S_OK;
}


STDMETHODIMP CShellFavoritesNameSpace::MoveSelectionUp()
{
    ASSERT(_psfns);
    if (_psfns)
        return _psfns->MoveSelectionUp();
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::MoveSelectionTo()
{
    ASSERT(_psfns);
    if (_psfns)
        return _psfns->MoveSelectionTo();
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::Import()
{
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::Export()
{
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::Synchronize()
{
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::CreateSubscriptionForSelection(VARIANT_BOOL *pBool)
{
    ASSERT(pBool);
    
    if (_psfns && pBool)
        return FIX_SCRIPTING_ERRORS(_psfns->CreateSubscriptionForSelection(pBool));
        
    return S_OK;
}

STDMETHODIMP CShellFavoritesNameSpace::DeleteSubscriptionForSelection(VARIANT_BOOL *pBool)
{
    ASSERT(pBool);
    
    if (_psfns && pBool)
        return FIX_SCRIPTING_ERRORS(_psfns->DeleteSubscriptionForSelection(pBool));
        
    return S_OK;
}


STDMETHODIMP CShellFavoritesNameSpace::NewFolder()
{
    //hack to get control to be activated fully
    m_bUIActive = FALSE;
    InPlaceActivate(OLEIVERB_UIACTIVATE);
    if (_psfns)
        return _psfns->NewFolder();

    return S_OK;
}


STDMETHODIMP CShellFavoritesNameSpace::InvokeContextMenuCommand(BSTR strCommand)
{
    ASSERT(strCommand);

    if (strCommand)
    {
        //only if renaming, activate control
        if (StrStr(strCommand, L"rename") != NULL)
        {
            //hack to get control to be activated fully
            m_bUIActive = FALSE;
            InPlaceActivate(OLEIVERB_UIACTIVATE);
        }

        if (_psfns)
            return _psfns->InvokeContextMenuCommand(strCommand);
    }

    return S_OK;
}

//BUGBUG rename me to get_IsSubscriptionsEnabled
STDMETHODIMP CShellFavoritesNameSpace::get_FOfflinePackInstalled(VARIANT_BOOL * pVal)
{
    ASSERT(pVal);

    *pVal = BOOLIFY(!SHRestricted2(REST_NoAddingSubscriptions, NULL, 0));
    return S_OK;
}

LRESULT CShellFavoritesNameSpace::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    if ((HWND)wParam != _hwndTv)
        ::SendMessage(_hwndTv, uMsg, wParam, lParam);
    bHandled = TRUE;
    return 0;
}


LRESULT CShellFavoritesNameSpace::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = TRUE;

    return S_OK;
}
/*
LRESULT CShellFavoritesNameSpace::OnNotifyFormat(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (NF_QUERY == lParam)
    {
        bHandled = TRUE;
        return (DLL_IS_UNICODE ? NFR_UNICODE : NFR_ANSI);
    }
    return FALSE;
}
*/

BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL);

HRESULT CShellFavoritesNameSpace::GetEventInfo(IShellFolder *psf, LPCITEMIDLIST pidl,
                                               UINT *pcItems, LPWSTR pszUrl, DWORD cchUrl, 
                                               UINT *pcVisits, LPWSTR pszLastVisited, BOOL *pfAvailableOffline)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    TCHAR szUrl[MAX_URL_STRING];

#ifdef UNIX
    // IEUNIX: (BUGBUG) Uninitialized var causing crashes in trident.
    szPath[0] = szUrl[0] = 0;
#endif
    
    *pcItems = 1;
    
    ULONG ulAttr = SFGAO_FOLDER;    // make sure item is actually a folder
    hr = GetPathForItem(psf, pidl, szPath, &ulAttr);
    if (SUCCEEDED(hr) && (ulAttr & SFGAO_FOLDER)) 
    {
        pszUrl[0] = 0;
        pszLastVisited[0] = 0;
        
        WIN32_FIND_DATA ffdata;
        HANDLE hFile = FindFirstFile(szPath, &ffdata);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            SHFormatDateTime(&(ffdata.ftLastWriteTime), NULL, pszLastVisited, MAX_PATH);
            FindClose(hFile);
        }
        
        *pcVisits = -1;
        *pfAvailableOffline = 0;
        
        return S_OK;
    }

    if (FAILED(hr))
    {
        //GetPathForItem fails on channel folders, but the following GetDisplayNameOf 
        //succeeds.
        STRRET str;
        if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
            StrRetToStrN(szPath, MAX_PATH, &str, pidl);
    }

    hr = GetNavTargetName(psf, pidl, szUrl, ARRAYSIZE(szUrl));

    // IsChannelFolder will fixup szUrl if it's a channel
    IsChannelFolder(szPath, szUrl);

    SHTCharToUnicode(szUrl, pszUrl, cchUrl);

    //
    // Get the cache info for this item.  Note that we use GetUrlCacheEntryInfoEx instead
    // of GetUrlCacheEntryInfo because it follows any redirects that occured.  This wacky
    // api uses a variable length buffer, so we have to guess the size and retry if the
    // call fails.
    //
    BOOL fInCache = FALSE;
    TCHAR szBuf[512];
    LPINTERNET_CACHE_ENTRY_INFO pCE = (LPINTERNET_CACHE_ENTRY_INFO)szBuf;
    DWORD dwEntrySize = sizeof(szBuf);
    
    fInCache = GetUrlCacheEntryInfoEx(szUrl, pCE, &dwEntrySize, NULL, NULL, NULL, 0);
    if (!fInCache)
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // We guessed too small for the buffer so allocate the correct size & retry
            pCE = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, dwEntrySize);
            if (pCE)
            {
                fInCache = GetUrlCacheEntryInfoEx(szUrl, pCE, &dwEntrySize, NULL, NULL, NULL, 0);
            }
        }
    }

    *pfAvailableOffline = IsSubscribed(szUrl);

    if (fInCache)
    {
        *pcVisits = pCE->dwHitRate;

        SHFormatDateTime(&(pCE->LastAccessTime), NULL, pszLastVisited, MAX_PATH);
        
        if ((TCHAR*)pCE != szBuf)
        {
            LocalFree(pCE);
        }
    } 
    else
    {
        *pcVisits = 0;
        pszLastVisited[0] = 0;
    }
    
    return hr;
}

LRESULT CShellFavoritesNameSpace::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnm = (LPNMHDR)lParam;
    if (pnm)
    {
        switch (pnm->code)
        {
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGED:
            if (_pns)
            {
                IShellFolder *psf = NULL;
                LPITEMIDLIST pidl = NULL;
                UINT cItems, cVisits;
                WCHAR szTitle[MAX_PATH], szUrl[INTERNET_MAX_URL_LENGTH], szLastVisited[MAX_PATH];
                BOOL fAvailableOffline;

#ifdef UNIX
                    // IEUNIX: (BUGBUG) Uninitialized variables causing crashes.
                    szTitle[0] = szUrl[0] = szLastVisited[0] = 0;
#endif
                HRESULT hr = _pns->BindToSelectedItemParent(IID_IShellFolder, (void **)&psf, &pidl);
                if (SUCCEEDED(hr) && (SUCCEEDED(_pns->GetSelectedItemName(szTitle, ARRAYSIZE(szTitle)))))
                {
                    GetEventInfo(psf, pidl, &cItems, szUrl, ARRAYSIZE(szUrl), &cVisits, szLastVisited, &fAvailableOffline);

                    CComBSTR strName(szTitle);
                    CComBSTR strUrl(szUrl);
                    CComBSTR strDate(szLastVisited);
                
                    Fire_FavoritesSelectionChange(cItems, 0, strName, strUrl, cVisits, strDate, fAvailableOffline);
                }
                else
                    Fire_FavoritesSelectionChange(0, 0, NULL, NULL, 0, NULL, FALSE);
                
                ILFree(pidl);
                ATOMICRELEASE(psf);
            }
            break;

        default:
            break;
        }
    }

    if (_pns)
    {
        if (!_pweh)
            _pns->QueryInterface(IID_IWinEventHandler, (void **) &_pweh);

        if (_pweh)
        {
            LRESULT lResult;
            HRESULT hr = _pweh->OnWinEvent(_hwndTv, uMsg, wParam, lParam, &lResult);
            
            bHandled = (lResult ? TRUE : FALSE);
            return SUCCEEDED(hr) ? lResult : hr;
        }
    }

    return S_OK;
}


HWND CShellFavoritesNameSpace::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID)
{
    HRESULT hr = S_OK;

    CWindowImpl<CShellFavoritesNameSpace>::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);

    _pns = CNscTree_CreateInstance();
    if (!_pns)
        return NULL;

    EVAL(SUCCEEDED(_pns->QueryInterface(IID_IShellFavoritesNameSpace, (void **) &_psfns)));
    _pns->SetNscMode(MODE_FAVORITES | MODE_CONTROL);
    _hwndTv = NULL;
    _pns->CreateTree(m_hWnd, 0, &_hwndTv);

    if (NULL == _pidlBase)
        SHGetSpecialFolderLocation(NULL, LOWORD((LPCITEMIDLIST)CSIDL_FAVORITES), &_pidlBase);

    if (_pidlBase)
    {
        _pns->Initialize(_pidlBase, (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS), (NSS_DROPTARGET | NSS_BROWSERSELECT));
    }
    else
        return NULL;

    _pns->ShowWindow(TRUE);
    if (!m_spClientSite)
    {
        ASSERT(FALSE);
        return m_hWnd;
    }
        
    return m_hWnd;
}

STDMETHODIMP CShellFavoritesNameSpace::GetWindow(HWND* lphwnd)
{
    return IOleInPlaceActiveObjectImpl<CShellFavoritesNameSpace>::GetWindow(lphwnd);
}

STDMETHODIMP CShellFavoritesNameSpace::TranslateAccelerator(PMSG pMsg)
{
    // label editing edit control is taking the keystrokes, TAing them will just duplicate them
    if (_pns->InLabelEdit())
        return S_FALSE;

    //hack so that the escape can get out to the document, because TA won't do it
    // WM_KEYDOWN is because some keyup's come through that need to not close the dialog
    if ( (pMsg->wParam == VK_ESCAPE) && (pMsg->message == WM_KEYDOWN) )
    {
        Fire_FavoritesSelectionChange(-1, (long)0, NULL, NULL, 0, NULL, FALSE);
        return S_FALSE;
    }
    
    //except for tabs and sys keys, let nsctree take all the keystrokes
    if ((pMsg->wParam != VK_TAB) && (pMsg->message != WM_SYSCHAR) && (pMsg->message != WM_SYSKEYDOWN) && (pMsg->message != WM_SYSKEYUP))
    {
        // TreeView will return TRUE if it processes the key, so we return S_OK to indicate
        // the keystroke was used and prevent further processing 
        return ::SendMessage(pMsg->hwnd, TVM_TRANSLATEACCELERATOR, 0, (LPARAM)pMsg) ? S_OK : S_FALSE;
    } 
    else
    {
        CComQIPtr<IOleControlSite,&IID_IOleControlSite>spCtrlSite(m_spClientSite);
        if(spCtrlSite)
            return spCtrlSite->TranslateAccelerator(pMsg,0);       
    }        
    
    return S_FALSE;
}

HRESULT CShellFavoritesNameSpace::InPlaceActivate(LONG iVerb, const RECT* prcPosRect /*= NULL*/)
{
    HRESULT hr;

    hr = CComControl<CShellFavoritesNameSpace>::InPlaceActivate(iVerb, prcPosRect);

    if (::GetFocus() != _hwndTv)
        ::SetFocus(_hwndTv);
    return hr;
}

HRESULT CShellFavoritesNameSpace::SetObjectRects(LPCRECT prcPos, LPCRECT prcClip)
{
    HRESULT hr = IOleInPlaceObjectWindowlessImpl<CShellFavoritesNameSpace>::SetObjectRects(prcPos, prcClip);
    if (_hwndTv)
        ::SetWindowPos(_hwndTv, NULL, 0, 0, prcPos->right - prcPos->left, prcPos->bottom - prcPos->top,
                       SWP_NOZORDER | SWP_NOACTIVATE);
    return hr;
}

LRESULT CShellFavoritesNameSpace::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // when in label edit mode, don't try to activate the control or you'll get out of label editing,
    // even when you click on the label edit control
    if (_pns && !_pns->InLabelEdit())
        InPlaceActivate(OLEIVERB_UIACTIVATE);
    return S_OK;
}

LRESULT CShellFavoritesNameSpace::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    IUnknown_SetSite(_pns, NULL);

    bHandled = FALSE; //let default handler also do it's work
    return 0;
}

HRESULT CShellFavoritesNameSpace::UpdateRegistry(BOOL bRegister)
{
    //this control uses selfreg.inx, not the ATL registry goo
    return S_OK;
}

LRESULT CShellFavoritesNameSpace::OnGetIShellBrowser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LRESULT lResult = NULL; // This will be the IShellBrowser *.

    if (EVAL(_pns))
    {
        IShellBrowser * psb;

        if (EVAL(SUCCEEDED(_pns->QueryInterface(IID_IShellBrowser, (void **)&psb))))
        {
            lResult = (LRESULT) psb;
            psb->Release();
        }
    }
    
    bHandled = TRUE;
    return lResult;
}

HRESULT CShellFavoritesNameSpace::SetRoot(BSTR bstrFullPath)
{
    HRESULT hr = E_FAIL;

    if (_pidlBase)
        ILFree(_pidlBase);

    hr = IECreateFromPathW(bstrFullPath, &_pidlBase);
    if (S_OK == hr && _pns)
    {
        hr = _pns->Initialize(_pidlBase, (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS), (NSS_DROPTARGET | NSS_BROWSERSELECT));
    }
    return FIX_SCRIPTING_ERRORS(hr);
}

/*  
LRESULT CShellFavoritesNameSpace::OnKeyMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    //forward all these on to nsc
    bHandled = TRUE;
    if (_hwndTv)
        return ::SendMessage(_hwndTv, uMsg, wParam, lParam);
    return 0;
}
*/
