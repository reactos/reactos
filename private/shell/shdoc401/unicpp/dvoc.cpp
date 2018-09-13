#include "stdafx.h"
#pragma hdrstop
#include "dvoc.h"

LCID g_lcidLocaleUnicpp = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);

// Helper function that can be used here and in main shell folder view window to get the
// current view options...
DWORD GetViewOptionsForDispatch()
{
    SHELLSTATE ss = {0};
    DWORD dwSetting = 0;

    // Get the view options to return...

    SHGetSetSettings(&ss, 
        SSF_SHOWALLOBJECTS|SSF_SHOWEXTENSIONS|SSF_SHOWCOMPCOLOR|
            SSF_SHOWSYSFILES|SSF_DOUBLECLICKINWEBVIEW|SSF_DESKTOPHTML|SSF_WIN95CLASSIC,
        FALSE);

    // Aarg: mnuch the Bool:1 fields into a dword...
    if (ss.fShowAllObjects) dwSetting |= SFVVO_SHOWALLOBJECTS;
    if (ss.fShowExtensions) dwSetting |= SFVVO_SHOWEXTENSIONS;
    if (ss.fShowCompColor) dwSetting |= SFVVO_SHOWCOMPCOLOR;
    if (ss.fShowSysFiles) dwSetting |= SFVVO_SHOWSYSFILES;
    if (ss.fDoubleClickInWebView) dwSetting |= SFVVO_DOUBLECLICKINWEBVIEW;
    if (ss.fDesktopHTML) dwSetting |= SFVVO_DESKTOPHTML;
    if (ss.fWin95Classic) dwSetting |= SFVVO_WIN95CLASSIC;
    
    return dwSetting;
}



CWebViewFolderContents::CWebViewFolderContents() : _psdf(NULL), _psfv(NULL)
{
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_pdvf);
    ASSERT(!_pdvf2);
    ASSERT(!_psfv);
    ASSERT(!_hwndLV);
    ASSERT(!_hwndLVParent);
    ASSERT(!_fSetDefViewAutomationObject);
    ASSERT(!_fClientEdge);
    ASSERT(!_pClassTypeInfo);
    ASSERT(!_psdf);
    ASSERT(!_dwAdviseCount);
    ASSERT(!_fReArrangeListView);
    ASSERT(!_dsaCookies);

    m_bWindowOnly = TRUE;
    m_bEnabled = TRUE;
    m_bRecomposeOnResize = TRUE;
    m_bResizeNatural = TRUE;
}

CWebViewFolderContents::~CWebViewFolderContents()
{
    UnadviseAll();
    ASSERT(NULL==_pdvf);
    ASSERT(NULL==_pdvf2);
    ASSERT(NULL==_psfv);
    ASSERT(NULL==_hwndLV);

    if (_pClassTypeInfo)
        _pClassTypeInfo->Release();

    if (_psdf) {
        _psdf->SetSite(NULL);
        _psdf->Release();
    }

    ATOMICRELEASE(_pdvf);
    ATOMICRELEASE(_pdvf2);
    ATOMICRELEASE(_psfv);

    // m_pdisp doesn't have a ref so it's OK if it's not NULL.

    if (_pmyiecp)
        delete _pmyiecp;

    DllRelease();
}


// ATL maintainence functions
LRESULT CWebViewFolderContents::_OnMessageForwarder(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (_hwndLVParent)
    {
        bHandled = TRUE;
        HWND hwnd = NULL;

        // Forward these messages directly to DefView (don't let MSHTML eat them)
        return ::SendMessage(_hwndLVParent, uMsg, wParam, lParam);
    }
    else
        return 0;
}


LRESULT CWebViewFolderContents::_OnEraseBkgndMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    // This function will just tell the default handler not to do anything and we
    // will handle it.

    // This is done in the case of WM_ERASEBKGND to...
    // Avoid flicker by not erasing the background. This OC doesn't care
    // about design-time issues - just usage on a Web View page.
    bHandled = TRUE;
    return 1;
}


LRESULT CWebViewFolderContents::_OnSizeMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    // Now resize the DefView ListView window because ATL isn't very reliable at it.
    if (_hwndLV)
    {
        ::SetWindowPos(_hwndLV, 0, 0, 0, m_rcPos.right - m_rcPos.left, m_rcPos.bottom - m_rcPos.top, SWP_NOZORDER);

        // We need to force a rearrange on IE401SHELL32 when the proper
        // size is set.  This is a timing dependent hack, but it works right now.
        if (_fReArrangeListView)
        {
            ListView_Arrange(_hwndLV, LVA_DEFAULT);
            _fReArrangeListView = FALSE;
        }
    }

    bHandled = FALSE;
    return 0;
}


LRESULT CWebViewFolderContents::_OnReArrangeListView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    _fReArrangeListView = TRUE;
    bHandled = TRUE;
    return 1;
}


HRESULT CWebViewFolderContents::DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent)
{
    HRESULT hr = IOleObjectImpl<CWebViewFolderContents>::DoVerbUIActivate(prcPosRect, hwndParent);
    
    if (SUCCEEDED(hr))
    {
        hr = _OnInPlaceActivate();
    }

    if (_hwndLV)
    {
        ::SetFocus(_hwndLV);
    }

    return hr;
}


void CWebViewFolderContents::UnadviseAll()
{
    if (_dsaCookies)
    {
        DWORD dw;
        for (int i = 0; DSA_GetItem(_dsaCookies, i, &dw); i++)
        {
            Unadvise(dw);
        }
        DSA_Destroy(_dsaCookies);
        _dsaCookies = NULL;
    }
}
    
HRESULT CWebViewFolderContents::Close(DWORD dwSaveOption)
{
    UnadviseAll();
    ATOMICRELEASE(_psfv);
    HRESULT hr = IOleObjectImpl<CWebViewFolderContents>::Close(dwSaveOption);
    return hr;
}

// move from de-active to in-place-active
HRESULT CWebViewFolderContents::_OnInPlaceActivate(void)
{
    HRESULT hr = S_OK;

    if (_pdvf == NULL)
    {
        hr = IUnknown_QueryService(m_spClientSite, SID_DefView, IID_IDefViewFrame, (void **)&_pdvf);
        if (EVAL(SUCCEEDED(hr)))
        {
            // Use the IDefViewFrame2 if we can get one; else fall back on IDefViewFrame.
            if (_pdvf2 == NULL)
            {
                hr = _pdvf->QueryInterface(IID_IDefViewFrame2, (LPVOID *)&_pdvf2);
            }
            if (_pdvf2 ? EVAL(SUCCEEDED(hr = _pdvf2->GetWindowLV2(&_hwndLV, SAFECAST(this, IWebViewOCWinMan *))))
                       : EVAL(SUCCEEDED(hr = _pdvf ->GetWindowLV (&_hwndLV))))
            {
                // we got it -- show the listview
                //
                _ShowWindowLV(_hwndLV);
                
                // IE 401 shell32 posts itself a message during GetWindowLV and it assumes
                // the last SetObjectRects call will occur before that message gets processed
                // This is a timing issue and is no longer the case.  Mimick what shell32
                // does so we can do the rearrange when the correct SetObjectRects comes through
                ::PostMessage(m_hWnd, WM_DVOC_REARRANGELISTVIEW, 0, 0);
           }
        }
    }
    return hr;
}

HRESULT CWebViewFolderContents::DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND hwndParent)
{
    HRESULT hr = IOleObjectImpl<CWebViewFolderContents>::DoVerbInPlaceActivate(prcPosRect, hwndParent);
    if (EVAL(SUCCEEDED(hr)))
    {
        hr = _OnInPlaceActivate();
    }
    return hr;
}

HRESULT CWebViewFolderContents::InPlaceDeactivate(void)
{
    _ReleaseWindow();
    ATOMICRELEASE(_pdvf);
    ATOMICRELEASE(_pdvf2);

    _ClearAutomationObject();

    ATOMICRELEASE(_psfv);
    
    return IOleInPlaceObject_InPlaceDeactivate();
}

HRESULT CWebViewFolderContents::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    HRESULT hres = IOleInPlaceObject_SetObjectRects(lprcPosRect, lprcClipRect);

    if (_hwndLV && _pdvf2)
    {
        _pdvf2->AutoAutoArrange(0);
    }

    return hres;
}

// *** IOleInPlaceActiveObject ***
HRESULT CWebViewFolderContents::TranslateAccelerator(LPMSG pMsg)
{
    HRESULT hres = S_OK;
    if (!_fTabRecieved)
    {
        hres = IOleInPlaceActiveObjectImpl<CWebViewFolderContents>::TranslateAccelerator(pMsg);

        // If we did not handle this and if it is a tab (and we are not getting it in a cycle), forward it to trident, if present.
        if (hres != S_OK && pMsg && (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6) && m_spClientSite)
        {
            IOleControlSite* pocs = NULL;
            if (SUCCEEDED(m_spClientSite->QueryInterface(IID_IOleControlSite, (void **)&pocs)))
            {
                DWORD grfModifiers = 0;
                if (GetKeyState(VK_SHIFT) & 0x8000)
                {
                    grfModifiers |= 0x1;    //KEYMOD_SHIFT
                }
                if (GetKeyState(VK_CONTROL) & 0x8000)
                {
                    grfModifiers |= 0x2;    //KEYMOD_CONTROL;
                }
                if (GetKeyState(VK_MENU) & 0x8000)
                {
                    grfModifiers |= 0x4;    //KEYMOD_ALT;
                }
                _fTabRecieved = TRUE;
                hres = pocs->TranslateAccelerator(pMsg, grfModifiers);
                _fTabRecieved = FALSE;
            }
        }
    }
    return hres;
}

// *** IProvideClassInfo ***
HRESULT CWebViewFolderContents::GetClassInfo(ITypeInfo ** ppTI)
{
    if (!_pClassTypeInfo) 
        Shell32GetTypeInfo(LANGIDFROMLCID(g_lcidLocaleUnicpp), CLSID_WebViewFolderContents, &_pClassTypeInfo);

    if (EVAL(_pClassTypeInfo))
    {
        _pClassTypeInfo->AddRef();
        *ppTI = _pClassTypeInfo;
        return S_OK;
    }

    *ppTI = NULL;
    return E_FAIL;
}



// *** IDispatch ***
HRESULT CWebViewFolderContents::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo)
{
    HRESULT hr = S_OK;

    *ppITypeInfo = NULL;

    if (0 != itinfo)
        return(TYPE_E_ELEMENTNOTFOUND);

    /*
#if 1
    // docs say we can ignore lcid if we support only one LCID
    // we don't have to return DISP_E_UNKNOWNLCID if we're *ignoring* it
    ppITI = &m_pInfo;
#else
    //
    // Since we returned one from GetTypeInfoCount, this function
    // can be called for a specific locale.  We support English
    // and neutral (defaults to English) locales.  Anything
    // else is an error.
    //
    // After this switch statement, ppITI will point to the proper
    // member pITypeInfo. If *ppITI is NULL, we know we need to
    // load type information, retrieve the ITypeInfo we want, and
    // then store it in *ppITI.
    //
    switch (PRIMARYLANGID(lcid))
    {
    case LANG_NEUTRAL:
    case LANG_ENGLISH:
        ppITI=&m_pInfo;
        break;

    default:
        return(DISP_E_UNKNOWNLCID);
    }
#endif
*/
    //Load a type lib if we don't have the information already.
    if (NULL == *ppITypeInfo)
    {
        ITypeInfo * pITIDisp;

        hr = Shell32GetTypeInfo(lcid, IID_IShellFolderViewDual, &pITIDisp);

        if (SUCCEEDED(hr))
        {
            HRESULT hrT;
            HREFTYPE hrefType;

            // All our IDispatch implementations are DUAL. GetTypeInfoOfGuid
            // returns the ITypeInfo of the IDispatch-part only. We need to
            // find the ITypeInfo for the dual interface-part.
            //
            hrT = pITIDisp->GetRefTypeOfImplType(0xffffffff, &hrefType);
            if (SUCCEEDED(hrT))
                hrT = pITIDisp->GetRefTypeInfo(hrefType, ppITypeInfo);

            ASSERT(SUCCEEDED(hrT));
            pITIDisp->Release();
        }
    }

    return hr;
}


HRESULT CWebViewFolderContents::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgdispid)
{
    ITypeInfo* pInfo;
    HRESULT hr = GetTypeInfo(0, lcid, &pInfo);

    if (pInfo != NULL)
    {
        hr = pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
        pInfo->Release();
    }

    TraceMsg(TF_DEFVIEW, "CWebViewFolderContents::GetIDsOfNames(DISPID=%ls, lcid=%d, cNames=%d) returned hr=%#08lx", *rgszNames, lcid, cNames, hr);
    return hr;
}

HRESULT CWebViewFolderContents::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr = E_FAIL;
    IDispatch * pdisp;
    DISPPARAMS dispparams = {0};

    if (!pdispparams)
        pdispparams = &dispparams;  // otherwise OLE Fails when passed NULL.

#ifdef CROSS_FRAME_SECURITY
    switch (dispidMember)
    {
    // this 
    case DISPID_SECURITYCTX:
        ASSERT(pvarResult);
        V_VT(pvarResult) = VT_BSTR;
        hr = _GetFolderSecurity(&V_BSTR(pvarResult));
        break;

    default:
#endif

    if (dispidMember == DISPID_WINDOWOBJECT)
    {
        if (SUCCEEDED(get_Script(&pdisp)))
        {
            hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            pdisp->Release();
            return hr;
        }
        else
            return DISP_E_MEMBERNOTFOUND;
    }
    // make sure we have an interface to hand off to Invoke
    if (NULL == m_pdisp)
    {
        hr = _InternalQueryInterface(IID_IShellFolderViewDual, (LPVOID*)&m_pdisp);
        ASSERT(SUCCEEDED(hr));

        // don't hold a refcount on ourself
        m_pdisp->Release();
    }


    ITypeInfo * pITypeInfo;

    hr = GetTypeInfo(0, lcid, &pITypeInfo);
    if (EVAL(SUCCEEDED(hr)))
    {
        //Clear exceptions
        SetErrorInfo(0L, NULL);

        hr = pITypeInfo->Invoke(m_pdisp, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        pITypeInfo->Release();
    }

    TraceMsg(TF_DEFVIEW, "CWebViewFolderContents::Invoke(DISPID=%#08lx, lcid=%d, wFlags=%d, pvarResult=%#08lx) returned hr=%#08lx", dispidMember, lcid, wFlags, pvarResult, hr);
#ifdef CROSS_FRAME_SECURITY
    }
#endif

    return hr;
}

#define DW_MISC_STATUS (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_RECOMPOSEONRESIZE|OLEMISC_CANTLINKINSIDE|OLEMISC_INSIDEOUT)
HRESULT CWebViewFolderContents::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    *pdwStatus = DW_MISC_STATUS;

    return S_OK;
}


// IExpDispSupport
HRESULT CWebViewFolderContents::FindCIE4ConnectionPoint(REFIID riid, CIE4ConnectionPoint **ppccp)
{
    HRESULT hr = E_FAIL;

    if (!ppccp)
        return E_INVALIDARG;

    *ppccp = NULL;
    if (IsEqualIID(DIID_DShellFolderViewEvents, riid))
    {
        if (!_pmyiecp)
            _pmyiecp = new CMyIE4ConnectionPoint(SAFECAST(this, IExpDispSupport *));

        if (EVAL(_pmyiecp))
        {
            // Don't give ref because _pmyiecp doesn't do refs, we
            // are assuming we will out live the one caller.  That's
            // okay because we assume with good reason that it will only be one specific
            // caller.
            *ppccp = _pmyiecp;
            hr = S_OK;
        }
    }

    return hr;
}

 
// IOleInPlaceActiveObject
HRESULT CWebViewFolderContents::SwapWindow(HWND hwndLV, IWebViewOCWinMan **pocWinMan)
{
    HRESULT hres = S_OK;

    ASSERT(pocWinMan);
    *pocWinMan = NULL;
    _ReleaseWindow();

    if (!hwndLV)
        return hres;

    _ShowWindowLV(hwndLV);

    if (FAILED(_InternalQueryInterface(IID_IWebViewOCWinMan, (LPVOID*)pocWinMan)))   
        hres = S_FALSE;

    return hres;
}


void CWebViewFolderContents::_ShowWindowLV(HWND hwndLV)
{
    if (!hwndLV)
        return;
    _hwndLV = hwndLV;
    _hwndLVParent = ::GetParent(_hwndLV);

    SHSetParentHwnd(_hwndLV, m_hWnd);

    LONG lExStyle = ::GetWindowLong(_hwndLV, GWL_EXSTYLE);
    _fClientEdge = lExStyle & WS_EX_CLIENTEDGE ? TRUE : FALSE;
    if (_fClientEdge)
    {
        lExStyle &= ~WS_EX_CLIENTEDGE;
        ::SetWindowLong(_hwndLV, GWL_EXSTYLE, lExStyle);
    }

    lExStyle = ListView_GetExtendedListViewStyle(_hwndLV);
    // Switch off infotips for webview
    lExStyle &= ~LVS_EX_INFOTIP;

    ListView_SetExtendedListViewStyle(_hwndLV, lExStyle);

    ::SetWindowPos(_hwndLV, 0, 0, 0, m_rcPos.right - m_rcPos.left, m_rcPos.bottom - m_rcPos.top, SWP_NOZORDER);

    _pdvf->QueryInterface(IID_IShellFolderView, (void **)&_psfv);

    // Incase We have been advised before we got here...
    if (_fSetDefViewAutomationObject)
        _SetAutomationObject();

    return;
}

void CWebViewFolderContents::_ReleaseWindow()
{
    if (_hwndLV)
    {
        HWND hwndFocusPrev = GetFocus();
        if (_fClientEdge)
            SetWindowBits(_hwndLV, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);

        SHSetParentHwnd(_hwndLV, _hwndLVParent);
        ::SetWindowPos(_hwndLV, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (hwndFocusPrev == _hwndLV)
            ::SetFocus(_hwndLV);
        _pdvf->ReleaseWindowLV();
        _hwndLV = NULL;
    }
    return;
}


// *** IConnectionPoint ***
HRESULT CWebViewFolderContents::Advise(IUnknown * pUnkSink, DWORD * pdwCookie)
{
    HRESULT hr = S_OK;

    if (!_dsaCookies)
    {
        _dsaCookies = DSA_Create(sizeof(*pdwCookie), 4);
        if (!_dsaCookies)
        {
            *pdwCookie = 0;
            hr = E_OUTOFMEMORY;
        }
    }

    hr = IConnectionPointImpl<CWebViewFolderContents, &DIID_DShellFolderViewEvents>::Advise(pUnkSink, pdwCookie);
    _dwAdviseCount++;
    _SetAutomationObject();

    if (SUCCEEDED(hr))
    {
        if (-1 == DSA_AppendItem(_dsaCookies, pdwCookie))
        {
            IConnectionPointImpl<CWebViewFolderContents, &DIID_DShellFolderViewEvents>::Unadvise(*pdwCookie);
            *pdwCookie = 0;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT CWebViewFolderContents::Unadvise(DWORD dwCookie)
{
    HRESULT hr = E_FAIL;

    if (_dsaCookies)
    {
        int i = 0;
        DWORD dw;

        while (DSA_GetItem(_dsaCookies, i++, &dw))
        {
            if (dw == dwCookie)
            {
                DSA_DeleteItem(_dsaCookies, --i);
                hr = IConnectionPointImpl<CWebViewFolderContents, &DIID_DShellFolderViewEvents>::Unadvise(dwCookie);
                _dwAdviseCount--;
                _ClearAutomationObject();
                break;
            }
        }
    }
    return hr;
}


HRESULT CWebViewFolderContents::_SetAutomationObject(void)
{
    HRESULT hr = S_OK;

    if (1 == _dwAdviseCount)
    {
        if (_psfv)
        {
            // Let Defview know about us...
            hr = _psfv->SetAutomationObject(SAFECAST(this, IDispatch*));
        }
        // Set the flag anyway so that we know to do it again later...
        _fSetDefViewAutomationObject = TRUE;
    }

    return hr;
}


HRESULT CWebViewFolderContents::_ClearAutomationObject(void)
{
    HRESULT hr = S_OK;

    if ((0 == _dwAdviseCount) && _fSetDefViewAutomationObject)
    {
        if (_psfv)
            hr = _psfv->SetAutomationObject(NULL);

        _fSetDefViewAutomationObject = FALSE;
    }

    return hr;
}

// IShellFolderViewDual
HRESULT CWebViewFolderContents::get_Application(IDispatch **ppid)
{
    // We will let the folder object get created and have it maintain that we only have one
    // application object (with the site) set properly...
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psdf->get_Application(ppid);
    return hr;
}

HRESULT CWebViewFolderContents::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    ASSERT(0);  // Error returns from script cause bad dialogs
    return E_FAIL;
}

HRESULT CWebViewFolderContents::_GetFolderIDList(LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    IShellFolder *psf;
    if (SUCCEEDED(_pdvf->GetShellFolder(&psf)))
    {
        IPersistFolder2 *ppf;
        if (SUCCEEDED(psf->QueryInterface(IID_IPersistFolder2, (void **)&ppf)))
        {
            ppf->GetCurFolder(ppidl);
            ppf->Release();
        }
        psf->Release();
    }

    if (*ppidl == NULL)
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(_psfv->GetObject(&pidl, (UINT)-42)) && pidl)
        {
            *ppidl = ILClone(pidl);
        }
    }

    ASSERT(*ppidl);  // Error returns from script cause bad dialogs
    return *ppidl ? S_OK : E_FAIL;
}
    
#ifdef CROSS_FRAME_SECURITY
HRESULT CWebViewFolderContents::_GetFolderSecurity(BSTR *pbstr)
{
    *pbstr = NULL;

    LPITEMIDLIST pidl;
    HRESULT hres = _GetFolderIDList(&pidl);
    if (SUCCEEDED(hres))
    {
        TCHAR szURL[MAX_URL_STRING];

        if (SHGetPathFromIDList(pidl, szURL))
        {
            DWORD dwChar = ARRAYSIZE(szURL);
            hres = UrlCreateFromPath(szURL, szURL, &dwChar, 0);    // in place!
            if (SUCCEEDED(hres))
            {
                *pbstr = TCharSysAllocString(szURL);
                hres = *pbstr ? S_OK : E_OUTOFMEMORY;
            }
        }
        else
            hres = E_FAIL;
    }

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}
#endif

HRESULT CWebViewFolderContents::_GetFolder()
{
    HRESULT hres = E_FAIL;

    if (_pdvf)
    {
        if (_psdf)
            return NOERROR;

        LPITEMIDLIST pidl;
        if (EVAL(SUCCEEDED(_GetFolderIDList(&pidl))))
        {
            IShellFolder *psf = NULL;
            // For objects that cheat and not have a unique pidl to folder mapping
            _pdvf->GetShellFolder(&psf);

            // We assume we need the parent window to the defview...
            hres = CSDFolder_Create(::GetParent(_hwndLVParent), pidl, psf, &_psdf);
            SAFERELEASE(psf);
            if (SUCCEEDED(hres))
            {
                _psdf->SetSite(m_spClientSite);
                hres = MakeSafeForScripting((IUnknown**)&_psdf);
            }
            ILFree(pidl);
        }
    }

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}

HRESULT CWebViewFolderContents::get_Folder(Folder **ppid)
{
    HRESULT hres;
    *ppid = NULL;

    hres = _GetFolder();
    if (SUCCEEDED(hres))
        hres = _psdf->QueryInterface(IID_Folder, (void **)ppid);

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}

HRESULT CWebViewFolderContents::SelectedItems(FolderItems **ppid)
{
    // We need to talk to the actual window under us
    *ppid = NULL;

    HRESULT hres = _GetFolder();
    if (EVAL(SUCCEEDED(hres)))
    {
        hres = CSDFldrItems_Create(_psdf, TRUE, ppid);
        if (EVAL(SUCCEEDED(hres)))
            hres = MakeSafeForScripting((IUnknown**)ppid);
    }

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}

HRESULT CWebViewFolderContents::get_FocusedItem(FolderItem **ppid)
{
    *ppid = NULL;

    HRESULT hres = _GetFolder();
    if (EVAL(SUCCEEDED(hres)))
    {
        hres = S_FALSE;

        if (_psfv)
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(_psfv->GetObject(&pidl, (UINT)-2)) && pidl)
            {
                hres = CSDFldrItem_Create(_psdf, pidl, ppid);
                if (SUCCEEDED(hres))
                {
                    hres = MakeSafeForScripting((IUnknown**)ppid);
                }
                // Don't free pidl as we did not copy it...
                // ILFree(pidl);
            }
        }
        else
            hres = E_FAIL;
    }

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}

HRESULT CWebViewFolderContents::SelectItem(VARIANT *pvfi, int dwFlags)
{
    HRESULT hres = E_FAIL;
    if (_pdvf && _psfv)
    {
        LPCITEMIDLIST pidl = VariantToConstIDList(pvfi);
        if (pidl)
        {
            IShellView *psv;
            hres = _psfv->QueryInterface(IID_IShellView, (void **)&psv);
            if (SUCCEEDED(hres))
            {
                hres = psv->SelectItem(pidl, dwFlags);
                psv->Release();
            }
        }
    }

    ASSERT(SUCCEEDED(hres));  // Error returns from script cause bad dialogs
    return hres;
}

HRESULT CWebViewFolderContents::PopupItemMenu(FolderItem *pfi, VARIANT vx, VARIANT vy, BSTR * pbs)
{
    ASSERT(0);  // Error returns from script cause bad dialogs
    return E_NOTIMPL;
}

HRESULT CWebViewFolderContents::get_Script(IDispatch **ppid)
{
    HRESULT hr;
    IHTMLDocument * phtmld;
    IShellView  * psv;

    *ppid = NULL;
    if (_psfv) {
        hr = _psfv->QueryInterface(IID_IShellView, (void **)&psv);
        if (SUCCEEDED(hr)) {
            // lets see if there is a IHTMLDocument that is below us now...    
            hr = psv->GetItemObject(SVGIO_BACKGROUND, IID_IHTMLDocument, (void **)&phtmld);
            if (SUCCEEDED(hr)) {
                hr = MakeSafeForScripting((IUnknown **)&phtmld);
                if (SUCCEEDED(hr)) {
                    hr = phtmld->get_Script(ppid);
                    phtmld->Release();
                }
            }
            psv->Release();
        }
    } else {
        hr = E_FAIL;
    }

   ASSERT(SUCCEEDED(hr));  // Error returns from script cause bad dialogs
   return hr;
}
HRESULT CWebViewFolderContents::get_ViewOptions(long *plSetting)
{
    *plSetting = (LONG)GetViewOptionsForDispatch();
    return S_OK;
}






HRESULT CMyIE4ConnectionPoint::DoInvokeIE4(LPBOOL pf, LPVOID *ppv, DISPID dispid, DISPPARAMS *pdispparams)
{
    // Shell32 never asks for a cancel
    ASSERT(pf == NULL && ppv == NULL);

    // Forward the invoke to our parent WebViewFolderContents.
    return IUnknown_CPContainerInvokeParam(m_punk, DIID_DShellFolderViewEvents,
                        dispid, NULL, 0);
}
