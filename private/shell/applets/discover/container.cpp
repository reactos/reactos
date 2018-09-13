//////////////////////////////////////////////////////////////////////////
//
//  container.cpp
//
//      This file contains the complete implementation of an ActiveX
//      control container. This purpose of this container is to test
//      a single control being hosted.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlwapi.h>
#include "container.h"

/**
 *  This method is the constructor for the CContainer object. 
 */
CContainer::CContainer()
{
    m_cRefs     = 1;
    m_hwnd      = NULL;
    m_punk      = NULL;

    memset(&m_rect, 0, sizeof(m_rect));
}

/** 
 *  This method is the destructor for the CContainer object.
 */
CContainer::~CContainer()
{
    if (m_punk)
    {
        m_punk->Release();
        m_punk=NULL;
    }
}

/**
 *  This method is called when the caller wants an interface pointer.
 *
 *  @param      riid        The interface being requested.
 *  @param      ppvObject   The resultant object pointer.
 *
 *  @return     HRESULT     S_OK, E_POINTER, E_NOINTERFACE
 */
STDMETHODIMP CContainer::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IOleClientSite))
        *ppvObject = (IOleClientSite *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
        *ppvObject = (IOleInPlaceSite *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceFrame))
        *ppvObject = (IOleInPlaceFrame *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        *ppvObject = (IOleInPlaceUIWindow *)this;
    else if (IsEqualIID(riid, IID_IOleControlSite))
        *ppvObject = (IOleControlSite *)this;
    else if (IsEqualIID(riid, IID_IDocHostUIHandler))
        *ppvObject = (IDocHostUIHandler *)this;
    else if (IsEqualIID(riid, IID_IOleWindow))
        *ppvObject = this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *ppvObject = (IDispatch *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

/**
 *  This method increments the current object count.
 *
 *  @return     ULONG       The new reference count.
 */
ULONG CContainer::AddRef(void)
{
    return ++m_cRefs;
}

/**
 *  This method decrements the object count and deletes if necessary.
 *
 *  @return     ULONG       Remaining ref count.
 */
ULONG CContainer::Release(void)
{
    if (--m_cRefs)
        return m_cRefs;

    delete this;
    return 0;
}

// ***********************************************************************
//  IOleClientSite
// ***********************************************************************

HRESULT CContainer::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppMk)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetContainer(LPOLECONTAINER * ppContainer)
{
    return E_NOINTERFACE;
}

HRESULT CContainer::ShowObject()
{
    return S_OK;
}

HRESULT CContainer::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

HRESULT CContainer::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleWindow
// ***********************************************************************

HRESULT CContainer::GetWindow(HWND * lphwnd)
{
    if (!IsWindow(m_hwnd))
    {
        *lphwnd = NULL;
        return S_FALSE;
    }

    *lphwnd = m_hwnd;
    return S_OK;
}

HRESULT CContainer::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceSite
// ***********************************************************************

HRESULT CContainer::CanInPlaceActivate(void)
{
    return S_OK;
}

HRESULT CContainer::OnInPlaceActivate(void)
{
    return S_OK;
}

HRESULT CContainer::OnUIActivate(void)
{
    return S_OK;
}

HRESULT CContainer::GetWindowContext (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppIIPUIWin,
                                  LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *ppFrame = (IOleInPlaceFrame *)this;
    *ppIIPUIWin = NULL;

    RECT rect;
    GetClientRect(m_hwnd, &rect);
    lprcPosRect->left       = 0;
    lprcPosRect->top        = 0;
    lprcPosRect->right      = rect.right;
    lprcPosRect->bottom     = rect.bottom;

    CopyRect(lprcClipRect, lprcPosRect);

    lpFrameInfo->cb             = sizeof(OLEINPLACEFRAMEINFO);
    lpFrameInfo->fMDIApp        = FALSE;
    lpFrameInfo->hwndFrame      = m_hwnd;
    lpFrameInfo->haccel         = 0;
    lpFrameInfo->cAccelEntries  = 0;

    (*ppFrame)->AddRef();
    return S_OK;
}

HRESULT CContainer::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnUIDeactivate(BOOL fUndoable)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnInPlaceDeactivate(void)
{
    return S_OK;
}

HRESULT CContainer::DiscardUndoState(void)
{
    return E_NOTIMPL;
}

HRESULT CContainer::DeactivateAndUndo(void)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnPosRectChange(LPCRECT lprcPosRect)
{
    return S_OK;
}

// ***********************************************************************
//  IOleInPlaceUIWindow
// ***********************************************************************

HRESULT CContainer::GetBorder(LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

HRESULT CContainer::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceFrame
// ***********************************************************************

HRESULT CContainer::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

HRESULT CContainer::RemoveMenus(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetStatusText(LPCOLESTR pszStatusText)
{
    return S_OK;
}

HRESULT CContainer::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT CContainer::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    // BUGBUG: cache this pointer since this is called ALOT
    IOleInPlaceActiveObject * pioipao;
    if ( m_punk && SUCCEEDED(m_punk->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *)&pioipao)))
    {
        HRESULT hr;
        hr = pioipao->TranslateAccelerator( lpmsg );
        pioipao->Release();
        return hr;
    }
    return S_OK;
}

// ***********************************************************************
//  IOleControlSite
// ***********************************************************************

HRESULT CContainer::OnControlInfoChanged()
{
    return E_NOTIMPL;
}

HRESULT CContainer::LockInPlaceActive(BOOL fLock)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetExtendedControl(IDispatch **ppDisp)
{
    if (ppDisp == NULL)
        return E_INVALIDARG;

    *ppDisp = (IDispatch *)this;
    (*ppDisp)->AddRef();

    return S_OK;
}

HRESULT CContainer::TransformCoords(POINTL *pptlHimetric, POINTF *pptfContainer, DWORD dwFlags)
{
    return E_NOTIMPL;
}

HRESULT CContainer::TranslateAccelerator(LPMSG pMsg, DWORD grfModifiers)
{
    // The control will call this method on the container if the control
    // does not want to accelerate the given message.  This is called
    // after the control processes it's accelerators.
    return S_FALSE;
}

HRESULT CContainer::OnFocus(BOOL fGotFocus)
{
    return E_NOTIMPL;
}

HRESULT CContainer::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IDispatch
// ***********************************************************************

HRESULT CContainer::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)
{
    if ( 0 == StrCmpW( *rgszNames, L"Exit" ) )
    {
        *rgdispid = 1;
        return S_OK;
    }
    else
    {
        *rgdispid = DISPID_UNKNOWN;
        return DISP_E_UNKNOWNNAME;
    }
}

HRESULT CContainer::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    return E_NOTIMPL;
}

HRESULT CContainer::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr)
{
    if ( 1 == dispid )
    {
        // Exit the process by destroying our window
        // DestroyWindow(m_hwnd);

        // BUGBUG: Due to a bug in jscript (build 1983 and later) we AV when we destory the container window before
        // allowing the contained object time to shut down.  It's totally timing related.  By delaying the actual
        // DestroyWindow call we avoid the bug.  This is NOT a solution to the bug, its a work around.  It would be
        // much simpler if we could just go ahead and destroy our window.  If the browser folks get this bug fixed
        // we should switch back to using the Destroy Window call above and get rid of OnExit and OnRelease in the
        // CWebApp class.
        SendMessage(m_hwnd,WM_USER+0,0,0);              // release everything
        PostMessage(m_hwnd,WM_USER+1,0,0);              // then post a message to destroy ourselves
        return S_OK;
    }
    else
    {
        return DISP_E_MEMBERNOTFOUND;
    }
}


// ***********************************************************************
//  IDocHostUIHandler
// ***********************************************************************

HRESULT CContainer::GetHostInfo(DOCHOSTUIINFO* pInfo)
{
    pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG|DOCHOSTUIFLAG_DISABLE_HELP_MENU|DOCHOSTUIFLAG_NO3DBORDER;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

HRESULT CContainer::ShowUI(DWORD, IOleInPlaceActiveObject *, IOleCommandTarget *, IOleInPlaceFrame *, IOleInPlaceUIWindow *)
{
    // S_OK means we have shown our own UI so mshtml should not show any menus or toolbars
    return S_OK;
}

HRESULT CContainer::HideUI(void)
{
    // S_OK means we have hidden the UI elements (menus, toolbars, etc).
    // Since we don't have any of these they are already hidden.
    return S_OK;
}

HRESULT CContainer::UpdateUI(void)
{
    return S_OK;
}

//HRESULT CContainer::EnableModeless(BOOL)
//{
//    return E_NOTIMPL;
//}

HRESULT CContainer::OnDocWindowActivate(BOOL)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnFrameWindowActivate(BOOL)
{
    return E_NOTIMPL;
}

HRESULT CContainer::ResizeBorder(LPCRECT, IOleInPlaceUIWindow*, BOOL)
{
    return E_NOTIMPL;
}

HRESULT CContainer::ShowContextMenu(DWORD, POINT*, IUnknown*, IDispatch*)
{
    // S_OK means don't show context menu
    return S_OK;
}

HRESULT CContainer::TranslateAccelerator(LPMSG, const GUID __RPC_FAR *, DWORD)
{
    // S_FALSE means let mshtml do the default translation
    return S_FALSE;
}

HRESULT CContainer::GetOptionKeyPath(BSTR*, DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP CContainer::GetDropTarget(IDropTarget *, IDropTarget **)
{
    return E_NOTIMPL;
}

STDMETHODIMP CContainer::GetExternal(IDispatch **ppDispatch)
{
    // return the IDispatch we have for extending the object Model
    IDispatch* pDisp = (IDispatch*)this;
    pDisp->AddRef();
    *ppDispatch = pDisp;
    return S_OK;
}
        
STDMETHODIMP CContainer::TranslateUrl(DWORD, OLECHAR *, OLECHAR **)
{
    return E_NOTIMPL;
}
        
STDMETHODIMP CContainer::FilterDataObject( IDataObject *, IDataObject **)
{
    return E_NOTIMPL;
}


// ***********************************************************************
//  Public (non-interface) Methods
// ***********************************************************************

/**
 *  This method will add an ActiveX control to the container. Note, for
 *  now, this container can only have one control.
 *
 *  @param  bstrClsid   The CLSID or PROGID of the control.
 *
 *  @return             No return value.
 */
void CContainer::add(BSTR bstrClsid)
{
    CLSID   clsid;          // CLSID of the control object
    HRESULT hr;             // return code

    CLSIDFromString(bstrClsid, &clsid);
    CoCreateInstance(clsid, 
                     NULL, 
                     CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 
                     IID_IUnknown,
                     (PVOID *)&m_punk);

    if (!m_punk)
        return;

    IOleObject *pioo;
    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (FAILED(hr))
        return;

    pioo->SetClientSite(this);
    pioo->Release();

    IPersistStreamInit  *ppsi;
    hr = m_punk->QueryInterface(IID_IPersistStreamInit, (PVOID *)&ppsi);
    if (SUCCEEDED(hr))
    {
        ppsi->InitNew();
        ppsi->Release();
    }
}

/**
 *  This method will remove the control from the container.
 *
 *  @return             No return value.
 */
void CContainer::remove()
{
    if (!m_punk)
        return;

    HRESULT             hr;
    IOleObject          *pioo;
    IOleInPlaceObject   *pipo;

    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (SUCCEEDED(hr))
    {
        pioo->Close(OLECLOSE_NOSAVE);
        pioo->SetClientSite(NULL);
        pioo->Release();
    }

    hr = m_punk->QueryInterface(IID_IOleInPlaceObject, (PVOID *)&pipo);
    if (SUCCEEDED(hr))
    {
        pipo->UIDeactivate();
        pipo->InPlaceDeactivate();
        pipo->Release();
    }

    m_punk->Release();
    m_punk = NULL;
}

/**
 *  This method sets the parent window. This is used by the container
 *  so the control can parent itself.
 *
 *  @param  hwndParent  The parent window handle.
 *
 *  @return             No return value.
 */
void CContainer::setParent(HWND hwndParent)
{
    m_hwnd = hwndParent;
}

/**
 *  This method will set the location of the control.
 *  
 *  @param      x       The top left.
 *  @param      y       The top right.
 *  @param      width   The width of the control.
 *  @param      height  The height of the control.
 */
void CContainer::setLocation(int x, int y, int width, int height)
{
    m_rect.left     = x;
    m_rect.top      = y;
    m_rect.right    = width;
    m_rect.bottom   = height;

    if (!m_punk)
        return;

    HRESULT             hr;
    IOleInPlaceObject   *pipo;

    hr = m_punk->QueryInterface(IID_IOleInPlaceObject, (PVOID *)&pipo);
    if (FAILED(hr))
        return;

    pipo->SetObjectRects(&m_rect, &m_rect);
    pipo->Release();
}

/**
 *  Sets the visible state of the control.
 *
 *  @param  fVisible    TRUE=visible, FALSE=hidden
 *  @return             No return value.
 */
void CContainer::setVisible(BOOL fVisible)
{
    if (!m_punk)
        return;

    HRESULT     hr;
    IOleObject  *pioo;

    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (FAILED(hr))
        return;
    
    if (fVisible)
    {
        pioo->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, m_hwnd, &m_rect);
        pioo->DoVerb(OLEIVERB_SHOW, NULL, this, 0, m_hwnd, &m_rect);
    }
    else
        pioo->DoVerb(OLEIVERB_HIDE, NULL, this, 0, m_hwnd, NULL);

    pioo->Release();
}

/**
 *  This sets the focus to the control (a.k.a. UIActivate)
 *
 *  @param  fFocus      TRUE=set, FALSE=remove
 *
 *  @return             No return value.
 */
void CContainer::setFocus(BOOL fFocus)
{
    if (!m_punk)
        return;

    HRESULT     hr;
    IOleObject  *pioo;

    if (fFocus)
    {
        hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
        if (SUCCEEDED(hr))
        {
            pioo->DoVerb(OLEIVERB_UIACTIVATE, NULL, this, 0, m_hwnd, &m_rect);
            pioo->Release();
        }
    }
}

/**
 *  This method gives the control the opportunity to translate and use
 *  key strokes.
 *
 *  @param      msg     Key message.
 *
 *  @return             No return value.
 */
void CContainer::translateKey(MSG msg)
{
    if (!m_punk)
        return;

    HRESULT                 hr;
    IOleInPlaceActiveObject *pao;

    hr = m_punk->QueryInterface(IID_IOleInPlaceActiveObject, (PVOID *)&pao);
    if (FAILED(hr))
        return;

    pao->TranslateAccelerator(&msg);
    pao->Release();
}

/**
 *  Returns the IDispatch pointer of the contained control. Note, the
 *  caller is responsible for calling IDispatch::Release().
 *
 *  @return             Controls dispatch interface.
 */
IDispatch * CContainer::getDispatch()
{
    if (!m_punk)
        return NULL;

    HRESULT     hr;
    IDispatch   *pdisp;

    hr = m_punk->QueryInterface(IID_IDispatch, (PVOID *)&pdisp);
    return pdisp;
}

/**
 *  Returns the IUnknown interface pointer for the containd control. Note,
 *  the caller is responsible for calling IUnknown::Release().
 *
 *  @return             Controls unknown interface.
 */
IUnknown * CContainer::getUnknown()
{
    if (!m_punk)
        return NULL;

    m_punk->AddRef();
    return m_punk;
}