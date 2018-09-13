//=--------------------------------------------------------------------------=
// PropPage.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of CPropertyPage object.
//
#include "IPServer.H"
#include "PropPage.H"
#include "Util.H"
#include "Globals.H"

// for ASSERT and FAIL
//
SZTHISFILE

// this variable is used to pass the pointer to the object to the hwnd.
//
static CPropertyPage *s_pLastPageCreated;

//=--------------------------------------------------------------------------=
// CPropertyPage::CPropertyPage
//=--------------------------------------------------------------------------=
// constructor.
//
// Parameters:
//    IUnknown *          - [in] controlling unknown
//    int                 - [in] object type.
//
// Notes:
//
#pragma warning(disable:4355)  // using 'this' in constructor
CPropertyPage::CPropertyPage
(
    IUnknown         *pUnkOuter,
    int               iObjectType
)
: CUnknownObject(pUnkOuter, this), m_ObjectType(iObjectType)
{
    // initialize various dudes.
    //
    m_pPropertyPageSite = NULL;
    m_hwnd = NULL;
    m_fDirty = FALSE;
    m_fActivated = FALSE;
    m_cObjects = 0;
}
#pragma warning(default:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CPropertyPage::~CPropertyPage
//=--------------------------------------------------------------------------=
// destructor.
//
// Notes:
//
CPropertyPage::~CPropertyPage()
{
    // clean up our window.
    //
    if (m_hwnd) {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)0xffffffff);
        DestroyWindow(m_hwnd);
    }

    // release all the objects we're holding on to.
    //
    m_ReleaseAllObjects();

    // release the site
    //
    QUICK_RELEASE(m_pPropertyPageSite);
}

//=--------------------------------------------------------------------------=
// CPropertyPage::InternalQueryInterface
//=--------------------------------------------------------------------------=
// we support IPP and IPP2.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CPropertyPage::InternalQueryInterface
(
    REFIID  riid,
    void  **ppvObjOut
)
{
    IUnknown *pUnk;

    *ppvObjOut = NULL;

    if (DO_GUIDS_MATCH(IID_IPropertyPage, riid)) {
        pUnk = (IUnknown *)this;
    } else if (DO_GUIDS_MATCH(IID_IPropertyPage2, riid)) {
        pUnk = (IUnknown *)this;
    } else {
        return CUnknownObject::InternalQueryInterface(riid, ppvObjOut);
    }

    pUnk->AddRef();
    *ppvObjOut = (void *)pUnk;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::SetPageSite    [IPropertyPage]
//=--------------------------------------------------------------------------=
// the initialization function for a property page through which the page
// receives an IPropertyPageSite pointer.
//
// Parameters:
//    IPropertyPageSite *        - [in] new site.
//
// Output:
//    HRESULT
//
// Notes;
//
STDMETHODIMP CPropertyPage::SetPageSite
(
    IPropertyPageSite *pPropertyPageSite
)
{
    RELEASE_OBJECT(m_pPropertyPageSite);
    m_pPropertyPageSite = pPropertyPageSite;
    ADDREF_OBJECT(pPropertyPageSite);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Activate    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to create it's display window as a child of hwndparent
// and to position it according to prc.
//
// Parameters:
//    HWND                - [in]  parent window
//    LPCRECT             - [in]  where to position ourselves
//    BOOL                - [in]  whether we're modal or not.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Activate
(
    HWND    hwndParent,
    LPCRECT prcBounds,
    BOOL    fModal
)
{
    HRESULT hr;

    // first make sure the dialog window is loaded and created.
    //
    hr = m_EnsureLoaded();
    RETURN_ON_FAILURE(hr);

    // set our parent window if we haven't done so yet.
    //
    if (!m_fActivated) {
        SetParent(m_hwnd, hwndParent);
        m_fActivated = TRUE;
    }

    // now move ourselves to where we're told to be and show ourselves
    //
    Move(prcBounds);
    ShowWindow(m_hwnd, SW_SHOW);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Deactivate    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to destroy the window created in activate
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Deactivate
(
    void
)
{
    // blow away yon window.
    //
    if (m_hwnd)
        DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    m_fActivated = FALSE;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::GetPageInfo    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to fill a PROPPAGEINFO structure
//
// Parameters:
//    PROPPAGEINFO *    - [out] where to put info.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::GetPageInfo
(
    PROPPAGEINFO *pPropPageInfo
)
{
    RECT rect;

    CHECK_POINTER(pPropPageInfo);

    m_EnsureLoaded();

    // clear it out first.
    //
    memset(pPropPageInfo, 0, sizeof(PROPPAGEINFO));

    pPropPageInfo->pszTitle = OLESTRFROMRESID(TITLEIDOFPROPPAGE(m_ObjectType));
    pPropPageInfo->pszDocString = OLESTRFROMRESID(DOCSTRINGIDOFPROPPAGE(m_ObjectType));
    pPropPageInfo->pszHelpFile = OLESTRFROMANSI(HELPFILEOFPROPPAGE(m_ObjectType));
    pPropPageInfo->dwHelpContext = HELPCONTEXTOFPROPPAGE(m_ObjectType);

    if (!(pPropPageInfo->pszTitle && pPropPageInfo->pszDocString && pPropPageInfo->pszHelpFile))
        goto CleanUp;

    // if we've got a window yet, go and set up the size information they want.
    //
    if (m_hwnd) {
        GetWindowRect(m_hwnd, &rect);

        pPropPageInfo->size.cx = rect.right - rect.left;
        pPropPageInfo->size.cy = rect.bottom - rect.top;
    }

    return S_OK;

  CleanUp:
    if (pPropPageInfo->pszDocString) CoTaskMemFree(pPropPageInfo->pszDocString);
    if (pPropPageInfo->pszHelpFile) CoTaskMemFree(pPropPageInfo->pszHelpFile);
    if (pPropPageInfo->pszTitle) CoTaskMemFree(pPropPageInfo->pszTitle);

    return E_OUTOFMEMORY;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::SetObjects    [IPropertyPage]
//=--------------------------------------------------------------------------=
// provides the page with the objects being affected by the changes.
//
// Parameters:
//    ULONG            - [in] count of objects.
//    IUnknown **      - [in] objects.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::SetObjects
(
    ULONG      cObjects,
    IUnknown **ppUnkObjects
)
{
    HRESULT hr;
    ULONG   x;

    // free up all the old objects first.
    //
    m_ReleaseAllObjects();

    if (!cObjects)
        return S_OK;

    // now go and set up the new ones.
    //
    m_ppUnkObjects = (IUnknown **)HeapAlloc(g_hHeap, 0, cObjects * sizeof(IUnknown *));
    RETURN_ON_NULLALLOC(m_ppUnkObjects);

    // loop through and copy over all the objects.
    //
    for (x = 0; x < cObjects; x++) {
        m_ppUnkObjects[x] = ppUnkObjects[x];
        ADDREF_OBJECT(m_ppUnkObjects[x]);
    }

    // go and tell the object that there are new objects
    //
    hr = S_OK;
    m_cObjects = cObjects;
    // if we've got a window, go and notify it that we've got new objects.
    //
    if (m_hwnd)
        SendMessage(m_hwnd, PPM_NEWOBJECTS, 0, (LPARAM)&hr);
    if (SUCCEEDED(hr)) m_fDirty = FALSE;

    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Show    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to show or hide its window
//
// Parameters:
//    UINT             - [in] whether to show or hide
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Show
(
    UINT nCmdShow
)
{
    if (m_hwnd)
        ShowWindow(m_hwnd, nCmdShow);
    else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Move    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to relocate and resize itself to a position other than what
// was specified through Activate
//
// Parameters:
//    LPCRECT        - [in] new position and size
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Move
(
    LPCRECT prcBounds
)
{
    // do what they sez
    //
    if (m_hwnd)
        SetWindowPos(m_hwnd, NULL, prcBounds->left, prcBounds->top,
                     prcBounds->right - prcBounds->left,
                     prcBounds->bottom - prcBounds->top,
                     SWP_NOZORDER);
    else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::IsPageDirty    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page whether it has changed its state
//
// Output
//    S_OK            - yep
//    S_FALSE         - nope
//
// Notes:
//
STDMETHODIMP CPropertyPage::IsPageDirty
(
    void
)
{
    return m_fDirty ? S_OK : S_FALSE;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Apply    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to send its changes to all the objects passed through
// SetObjects()
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Apply
(
    void
)
{
    HRESULT hr = S_OK;

    if (m_hwnd) {
        SendMessage(m_hwnd, PPM_APPLY, 0, (LPARAM)&hr);
        RETURN_ON_FAILURE(hr);

        if (m_fDirty) {
            m_fDirty = FALSE;
            if (m_pPropertyPageSite)
                m_pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    } else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Help    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page that the help button was clicked.
//
// Parameters:
//    LPCOLESTR        - [in] help directory
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Help
(
    LPCOLESTR pszHelpDir
)
{
    BOOL f;

    ASSERT(m_hwnd, "How can somebody have clicked Help, but we don't have an hwnd?");

    // oblige them and show the help.
    //
    MAKE_ANSIPTR_FROMWIDE(psz, pszHelpDir);
    f = WinHelp(m_hwnd, psz, HELP_CONTEXT, HELPCONTEXTOFPROPPAGE(m_ObjectType));

    return f ? S_OK : E_FAIL;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::TranslateAccelerator    [IPropertyPage]
//=--------------------------------------------------------------------------=
// informs the page of keyboard events, allowing it to implement it's own
// keyboard interface.
//
// Parameters:
//    LPMSG            - [in] message that triggered this
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::TranslateAccelerator
(
    LPMSG pmsg
)
{
    ASSERT(m_hwnd, "How can we get a TranslateAccelerator call if we're not visible?");

    // just pass this message on to the dialog proc and see if they want it.
    //
    return IsDialogMessage(m_hwnd, pmsg) ? S_OK : S_FALSE;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::EditProperty    [IPropertyPage2]
//=--------------------------------------------------------------------------=
// instructs the page to set the focus to the property matching the dispid.
//
// Parameters:
//    DISPID            - [in] dispid of property to set focus to.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::EditProperty
(
    DISPID dispid
)
{
    HRESULT hr = E_NOTIMPL;

    // send the message on to the control, and see what they want to do with it.
    //
    SendMessage(m_hwnd, PPM_EDITPROPERTY, (WPARAM)dispid, (LPARAM)&hr);

    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::m_EnsureLoaded
//=--------------------------------------------------------------------------=
// makes sure the dialog is actually loaded
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CPropertyPage::m_EnsureLoaded
(
    void
)
{
    HRESULT hr = S_OK;

    // duh
    //
    if (m_hwnd)
        return S_OK;

    // set up the global variable so that when we're in the dialog proc, we can
    // stuff this in the hwnd
    //
    // crit sect this whole creation process for apartment threading support.
    //
    EnterCriticalSection(&g_CriticalSection);
    s_pLastPageCreated = this;

    // create the dialog window
    //
    CreateDialog(GetResourceHandle(), TEMPLATENAMEOFPROPPAGE(m_ObjectType), GetParkingWindow(),
                          (DLGPROC)CPropertyPage::PropPageDlgProc);
    ASSERT(m_hwnd, "Couldn't load Dialog Resource!!!");
    if (!m_hwnd) {
        LeaveCriticalSection(&g_CriticalSection);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // clean up variables and leave the critical section
    //
    s_pLastPageCreated = NULL;
    LeaveCriticalSection(&g_CriticalSection);

    // go and notify the window that it should pick up any objects that are
    // available
    //
    SendMessage(m_hwnd, PPM_NEWOBJECTS, 0, (LPARAM)&hr);

    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::m_ReleaseAllObjects
//=--------------------------------------------------------------------------=
// releases all the objects that we're working with
//
// Notes:
//
void CPropertyPage::m_ReleaseAllObjects
(
    void
)
{
    HRESULT hr;
    UINT x;

    if (!m_cObjects)
        return;

    // some people will want to stash pointers in the PPM_INITOBJECTS case, so
    // we want to tell them to release them now.
    //
    SendMessage(m_hwnd, PPM_FREEOBJECTS, 0, (LPARAM)&hr);

    // loop through and blow them all away.
    //
    for (x = 0; x < m_cObjects; x++)
        QUICK_RELEASE(m_ppUnkObjects[x]);

    HeapFree(g_hHeap, 0, m_ppUnkObjects);
    m_ppUnkObjects = NULL;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::PropPageDlgProc
//=--------------------------------------------------------------------------=
// static global helper dialog proc that gets called before we pass the message
// on to anybody ..
//
// Parameters:
//    - see win32sdk docs on DialogProc
//
// Notes:
//
BOOL CALLBACK CPropertyPage::PropPageDlgProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CPropertyPage *pPropertyPage;

    // get the window long, and see if it's been set to the object this hwnd
    // is operating against.  if not, go and set it now.
    //
    pPropertyPage = (CPropertyPage *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if ((LONG_PTR)pPropertyPage == (LONG_PTR)0xffffffff)
        return FALSE;
    if (!pPropertyPage) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)s_pLastPageCreated);
        pPropertyPage = s_pLastPageCreated;
        pPropertyPage->m_hwnd = hwnd;
    }

    ASSERT(pPropertyPage, "Uh oh.  Got a window, but no CpropertyPage for it!");

    // just call the user dialog proc and see if they want to do anything.
    //
    return pPropertyPage->DialogProc(hwnd, msg, wParam, lParam);
}


//=--------------------------------------------------------------------------=
// CPropertyPage::FirstControl
//=--------------------------------------------------------------------------=
// returns the first controlish object that we are showing ourselves for.
// returns a cookie that must be passed in for Next ...
//
// Parameters:
//    DWORD *    - [out] cookie to be used for Next
//
// Output:
//    IUnknown *
//
// Notes:
//
IUnknown *CPropertyPage::FirstControl
(
    DWORD *pdwCookie
)
{
    // just use the implementation of NEXT.
    //
    *pdwCookie = 0;
    return NextControl(pdwCookie);
}

//=--------------------------------------------------------------------------=
// CPropertyPage::NextControl
//=--------------------------------------------------------------------------=
// returns the next control in the chain of people to work with given a cookie
//
// Parameters:
//    DWORD *            - [in/out] cookie to get next from, and new cookie.
//
// Output:
//    IUnknown *
//
// Notes:
//
IUnknown *CPropertyPage::NextControl
(
    DWORD *pdwCookie
)
{
    UINT      i;

    // go looking through all the objects that we've got, and find the
    // first non-null one.
    //
    for (i = *pdwCookie; i < m_cObjects; i++) {
        if (!m_ppUnkObjects[i]) continue;

        *pdwCookie = i + 1;                // + 1 so we start at next item next time
        return m_ppUnkObjects[i];
    }

    // couldn't find it .
    //
    *pdwCookie = 0xffffffff;
    return NULL;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::MakeDirty    [helper, callable]
//=--------------------------------------------------------------------------=
// marks a page as dirty.
//
// Notes:
//
void CPropertyPage::MakeDirty
(
    void
)
{
    m_fDirty = TRUE;
    if (m_pPropertyPageSite)
        m_pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY|PROPPAGESTATUS_VALIDATE);
}


// from Globals.C
//
extern HINSTANCE g_hInstResources;


//=--------------------------------------------------------------------------=
// CPropertyPage::GetResourceHandle    [helper, callable]
//=--------------------------------------------------------------------------=
// returns current resource handle, based on pagesites ambient LCID.
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE CPropertyPage::GetResourceHandle
(
    void
)
{
    if (!g_fSatelliteLocalization)
        return g_hInstance;

    // if we've already got it, then there's not all that much to do.
    // don't need to crit sect this one right here since even if they do fall
    // into the ::GetResourceHandle call, it'll properly deal with things.
    //
    if (g_hInstResources)
        return g_hInstResources;

    // we'll get the ambient localeid from the host, and pass that on to the
    // automation object.
    //
    // enter a critical section for g_lcidLocale and g_fHavelocale
    //
    EnterCriticalSection(&g_CriticalSection);
    if (!g_fHaveLocale) {
        if (m_pPropertyPageSite) {
            m_pPropertyPageSite->GetLocaleID(&g_lcidLocale);
            g_fHaveLocale = TRUE;
        }
    }
    LeaveCriticalSection(&g_CriticalSection);

    return ::GetResourceHandle();
}
