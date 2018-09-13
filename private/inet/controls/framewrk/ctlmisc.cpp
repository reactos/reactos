//=--------------------------------------------------------------------------=
// CtlMisc.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// things that aren't elsewhere, such as property pages, and connection
// points.

#include "IPServer.H"
#include "CtrlObj.H"
#include "CtlHelp.H"
#include "Globals.H"
#include "StdEnum.H"
#include "Util.H"

#include <stdarg.h>

// for ASSERT and FAIL
//
SZTHISFILE

// this is used in our window proc so that we can find out who was last created
//
static COleControl *s_pLastControlCreated;

//=--------------------------------------------------------------------------=
// COleControl::COleControl
//=--------------------------------------------------------------------------=
// constructor
//
// Parameters:
//    IUnknown *          - [in] controlling Unknown
//    int                 - [in] type of primary dispatch interface OBJECT_TYPE_*
//    void *              - [in] pointer to entire object
//	  BOOL                - [in] whether to enable IDispatchEx functionality
//							to allow dynamic adding of properties
// Notes:
//
COleControl::COleControl
(
    IUnknown *pUnkOuter,
    int       iPrimaryDispatch,
    void     *pMainInterface,
	BOOL     fExpandoEnabled
)
: CAutomationObject(pUnkOuter, iPrimaryDispatch, pMainInterface, fExpandoEnabled),
  m_cpEvents(SINK_TYPE_EVENT),
  m_cpPropNotify(SINK_TYPE_PROPNOTIFY)
{
    // initialize all our variables -- we decided against using a memory-zeroing
    // memory allocator, so we sort of have to do this work now ...
    //
    m_nFreezeEvents = 0;

    m_pClientSite = NULL;
    m_pControlSite = NULL;
    m_pInPlaceSite = NULL;
    m_pInPlaceFrame = NULL;
    m_pInPlaceUIWindow = NULL;


    m_pInPlaceSiteWndless = NULL;

    // certain hosts don't like 0,0 as your initial size, so we're going to set
    // our initial size to 100,50 [so it's at least sort of visible on the screen]
    //
    m_Size.cx = 0;
    m_Size.cy = 0;
    memset(&m_rcLocation, 0, sizeof(m_rcLocation));

    m_hRgn = NULL;
    m_hwnd = NULL;
    m_hwndParent = NULL;
    m_hwndReflect = NULL;
    m_fHostReflects = TRUE;
    m_fCheckedReflecting = FALSE;

    m_nFreezeEvents = 0;
    m_pSimpleFrameSite = NULL;
    m_pOleAdviseHolder = NULL;
    m_pViewAdviseSink = NULL;
    m_pDispAmbient = NULL;

    m_fDirty = FALSE;
    m_fModeFlagValid = FALSE;
    m_fInPlaceActive = FALSE;
    m_fInPlaceVisible = FALSE;
    m_fUIActive = FALSE;
    m_fSaveSucceeded = FALSE;
    m_fViewAdvisePrimeFirst = FALSE;
    m_fViewAdviseOnlyOnce = FALSE;
    m_fRunMode = FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::~COleControl
//=--------------------------------------------------------------------------=
// "We are all of us resigned to death; it's life we aren't resigned to."
//    - Graham Greene (1904-91)
//
// Notes:
//
COleControl::~COleControl()
{
    // if we've still got a window, go and kill it now.
    //
    if (m_hwnd) {
        // so our window proc doesn't crash.
        //
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)0xFFFFFFFF);
        DestroyWindow(m_hwnd);
    }

    if (m_hwndReflect) {
        SetWindowLongPtr(m_hwndReflect, GWLP_USERDATA, 0);
        DestroyWindow(m_hwndReflect);
    }

    if (m_hRgn != NULL)
    {
       DeleteObject(m_hRgn);
       m_hRgn = NULL;
    }

    // clean up all the pointers we're holding around.
    //
    QUICK_RELEASE(m_pClientSite);
    QUICK_RELEASE(m_pControlSite);
    QUICK_RELEASE(m_pInPlaceSite);
    QUICK_RELEASE(m_pInPlaceFrame);
    QUICK_RELEASE(m_pInPlaceUIWindow);
    QUICK_RELEASE(m_pSimpleFrameSite);
    QUICK_RELEASE(m_pOleAdviseHolder);
    QUICK_RELEASE(m_pViewAdviseSink);
    QUICK_RELEASE(m_pDispAmbient);

    QUICK_RELEASE(m_pInPlaceSiteWndless);
}

#ifndef DEBUG
#pragma optimize("t", on)
#endif // DEBUG

//=--------------------------------------------------------------------------=
// COleControl::InternalQueryInterface
//=--------------------------------------------------------------------------=
// derived-controls should delegate back to this when they decide to support
// additional interfaces
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//    - NOTE: this function is speed critical!!!!
//
HRESULT COleControl::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    switch (riid.Data1) {
        // private interface for prop page support
        case Data1_IControlPrv:
          if(DO_GUIDS_MATCH(riid, IID_IControlPrv)) {
            *ppvObjOut = (void *)this;
            ExternalAddRef();
            return S_OK;
          }
          goto NoInterface;
        QI_INHERITS(this, IOleControl);
        QI_INHERITS(this, IPointerInactive);
        QI_INHERITS(this, IQuickActivate);
        QI_INHERITS(this, IOleObject);
        QI_INHERITS((IPersistStorage *)this, IPersist);
        QI_INHERITS(this, IPersistStreamInit);
        QI_INHERITS(this, IOleInPlaceObject);
        QI_INHERITS(this, IOleInPlaceObjectWindowless);
        QI_INHERITS((IOleInPlaceActiveObject *)this, IOleWindow);
        QI_INHERITS(this, IOleInPlaceActiveObject);
        QI_INHERITS(this, IViewObject);
        QI_INHERITS(this, IViewObject2);
        QI_INHERITS(this, IViewObjectEx);
        QI_INHERITS(this, IConnectionPointContainer);
        QI_INHERITS(this, ISpecifyPropertyPages);
        QI_INHERITS(this, IPersistStorage);
        QI_INHERITS(this, IPersistPropertyBag);
        QI_INHERITS(this, IProvideClassInfo);
        default:
            goto NoInterface;
    }

    // we like the interface, so addref and return
    //
    ((IUnknown *)(*ppvObjOut))->AddRef();
    return S_OK;

  NoInterface:
    // delegate to super-class for automation interfaces, etc ...
    //
    return CAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

#ifndef DEBUG
#pragma optimize("s", on)
#endif // DEBUG

//=--------------------------------------------------------------------------=
// COleControl::FindConnectionPoint    [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// given an IID, find a connection point sink for it.
//
// Parameters:
//    REFIID              - [in]  interfaces they want
//    IConnectionPoint ** - [out] where the cp should go
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::FindConnectionPoint
(
    REFIID             riid,
    IConnectionPoint **ppConnectionPoint
)
{
    CHECK_POINTER(ppConnectionPoint);

    // we support the event interface, and IDispatch for it, and we also
    // support IPropertyNotifySink.
    //
    if (DO_GUIDS_MATCH(riid, EVENTIIDOFCONTROL(m_ObjectType)) || DO_GUIDS_MATCH(riid, IID_IDispatch))
        *ppConnectionPoint = &m_cpEvents;
    else if (DO_GUIDS_MATCH(riid, IID_IPropertyNotifySink))
        *ppConnectionPoint = &m_cpPropNotify;
    else
        return E_NOINTERFACE;

    // generic post-processing.
    //
    (*ppConnectionPoint)->AddRef();
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::EnumConnectionPoints    [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// creates an enumerator for connection points.
//
// Parameters:
//    IEnumConnectionPoints **    - [out]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::EnumConnectionPoints
(
    IEnumConnectionPoints **ppEnumConnectionPoints
)
{
    IConnectionPoint **rgConnectionPoints;

    CHECK_POINTER(ppEnumConnectionPoints);

    // HeapAlloc an array of connection points [since our standard enum
    // assumes this and HeapFree's it later ]
    //
    rgConnectionPoints = (IConnectionPoint **)HeapAlloc(g_hHeap, 0, sizeof(IConnectionPoint *) * 2);
    RETURN_ON_NULLALLOC(rgConnectionPoints);

    // we support the event interface for this dude as well as IPropertyNotifySink
    //
    rgConnectionPoints[0] = &m_cpEvents;
    rgConnectionPoints[1] = &m_cpPropNotify;

    *ppEnumConnectionPoints = (IEnumConnectionPoints *)(IEnumGeneric *) new CStandardEnum(IID_IEnumConnectionPoints,
                                2, sizeof(IConnectionPoint *), (void *)rgConnectionPoints,
                                CopyAndAddRefObject);
    if (!*ppEnumConnectionPoints) {
        HeapFree(g_hHeap, 0, rgConnectionPoints);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetPages    [ISpecifyPropertyPages]
//=--------------------------------------------------------------------------=
// returns a counted array with the guids for our property pages.
//
// parameters:
//    CAUUID *    - [out] where to put the counted array.
//
// Output:
//    HRESULT
//
// NOtes:
//
STDMETHODIMP COleControl::GetPages
(
    CAUUID *pPages
)
{
    const GUID **pElems;
    void *pv;
    WORD  x;

    // if there are no property pages, this is actually pretty easy.
    //
    if (!CPROPPAGESOFCONTROL(m_ObjectType)) {
        pPages->cElems = 0;
        pPages->pElems = NULL;
        return S_OK;
    }

    // fill out the Counted array, using IMalloc'd memory.
    //
    pPages->cElems = CPROPPAGESOFCONTROL(m_ObjectType);
    pv = CoTaskMemAlloc(sizeof(GUID) * (pPages->cElems));
    RETURN_ON_NULLALLOC(pv);
    pPages->pElems = (GUID *)pv;

    // loop through our array of pages and get 'em.
    //
    pElems = PPROPPAGESOFCONTROL(m_ObjectType);
    for (x = 0; x < pPages->cElems; x++)
        pPages->pElems[x] = *(pElems[x]);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::m_pOleControl
//=--------------------------------------------------------------------------=
// returns a pointer to the control in which we are nested.
//
// Output:
//    COleControl *
//
// Notes:
//
inline COleControl *COleControl::CConnectionPoint::m_pOleControl
(
    void
)
{
    return (COleControl *)((BYTE *)this - ((m_bType == SINK_TYPE_EVENT)
                                          ? offsetof(COleControl, m_cpEvents)
                                          : offsetof(COleControl, m_cpPropNotify)));
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::QueryInterface
//=--------------------------------------------------------------------------=
// standard qi
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
STDMETHODIMP COleControl::CConnectionPoint::QueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    if (DO_GUIDS_MATCH(riid, IID_IConnectionPoint) || DO_GUIDS_MATCH(riid, IID_IUnknown)) {
        *ppvObjOut = (IConnectionPoint *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::AddRef
//=--------------------------------------------------------------------------=
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG COleControl::CConnectionPoint::AddRef
(
    void
)
{
    return m_pOleControl()->ExternalAddRef();
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::Release
//=--------------------------------------------------------------------------=
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG COleControl::CConnectionPoint::Release
(
    void
)
{
    return m_pOleControl()->ExternalRelease();
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::GetConnectionInterface
//=--------------------------------------------------------------------------=
// returns the interface we support connections on.
//
// Parameters:
//    IID *        - [out] interface we support.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::CConnectionPoint::GetConnectionInterface
(
    IID *piid
)
{
    if (m_bType == SINK_TYPE_EVENT)
        *piid = EVENTIIDOFCONTROL(m_pOleControl()->m_ObjectType);
    else
        *piid = IID_IPropertyNotifySink;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::GetConnectionPointContainer
//=--------------------------------------------------------------------------=
// returns the connection point container
//
// Parameters:
//    IConnectionPointContainer **ppCPC
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::CConnectionPoint::GetConnectionPointContainer
(
    IConnectionPointContainer **ppCPC
)
{
    return m_pOleControl()->ExternalQueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
}


//=--------------------------------------------------------------------------=
// COleControl::CConnectiontPoint::Advise
//=--------------------------------------------------------------------------=
// someboyd wants to be advised when something happens.
//
// Parameters:
//    IUnknown *        - [in]  guy who wants to be advised.
//    DWORD *           - [out] cookie
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::CConnectionPoint::Advise
(
    IUnknown *pUnk,
    DWORD    *pdwCookie
)
{
    HRESULT    hr;
    void      *pv;

    CHECK_POINTER(pdwCookie);

    // first, make sure everybody's got what they thinks they got
    //
    if (m_bType == SINK_TYPE_EVENT) {

        // CONSIDER: 12.95 -- this theoretically is broken -- if they do a find
        // connection point on IDispatch, and they just happened to also support
        // the Event IID, we'd advise on that.  this is not awesome, but will
        // prove entirely acceptable short term.
        //
        hr = pUnk->QueryInterface(EVENTIIDOFCONTROL(m_pOleControl()->m_ObjectType), &pv);
        if (FAILED(hr))
            hr = pUnk->QueryInterface(IID_IDispatch, &pv);
    }
    else
        hr = pUnk->QueryInterface(IID_IPropertyNotifySink, &pv);
    RETURN_ON_FAILURE(hr);

    // finally, add the sink.  it's now been cast to the correct type and has
    // been AddRef'd.
    //
    return AddSink(pv, pdwCookie);
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::AddSink
//=--------------------------------------------------------------------------=
// in some cases, we'll already have done the QI, and won't need to do the
// work that is done in the Advise routine above.  thus, these people can
// just call this instead. [this stems really from IQuickActivate]
//
// Parameters:
//    void *        - [in]  the sink to add. it's already been addref'd
//    DWORD *       - [out] cookie
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::CConnectionPoint::AddSink
(
    void  *pv,
    DWORD *pdwCookie
)
{
    IUnknown **rgUnkNew;
    int        i;

    // we optimize the case where there is only one sink to not allocate
    // any storage.  turns out very rarely is there more than one.
    //

    if (!m_cSinks) {
        m_SingleSink = (IUnknown *) pv;
        *pdwCookie = (DWORD)-1;
        m_cSinks = 1;
        return S_OK;
    }

    // Allocated the array yet?
    if (!m_rgSinks) {
        rgUnkNew = (IUnknown **)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, 8 * sizeof(IUnknown *));
        RETURN_ON_NULLALLOC(rgUnkNew);
        m_cAllocatedSinks = 8;
        m_rgSinks = rgUnkNew;
        m_rgSinks[0] = m_SingleSink;
        m_rgSinks[1] = (IUnknown *)pv;
        m_SingleSink = NULL;
        m_cSinks = 2;
        *pdwCookie = 2;
        return S_OK;
    }

    // Find an empty slot.
    for (i = 0; i < m_cAllocatedSinks; i++) {
        if (!m_rgSinks[i]) {
            m_rgSinks[i] = (IUnknown *)pv;
            i++;
            *pdwCookie = i;
            m_cSinks++;
            return S_OK;
        }
    }

    // Didn't find one, grow the array.
    rgUnkNew = (IUnknown **)HeapReAlloc(g_hHeap, HEAP_ZERO_MEMORY, m_rgSinks, (m_cAllocatedSinks + 8) * sizeof(IUnknown *));
    RETURN_ON_NULLALLOC(rgUnkNew);
    m_rgSinks = rgUnkNew;
    m_rgSinks[m_cAllocatedSinks] = (IUnknown *)pv;
    i = m_cAllocatedSinks+1;
    m_cAllocatedSinks += 8;

    *pdwCookie = i;
    m_cSinks++;
    return S_OK;
}


//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::Unadvise
//=--------------------------------------------------------------------------=
// they don't want to be told any more.
//
// Parameters:
//    DWORD        - [in]  the cookie we gave 'em.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::CConnectionPoint::Unadvise
(
    DWORD dwCookie
)
{
    IUnknown *pUnk;

    if (!dwCookie)
        return S_OK;

    if (dwCookie == (DWORD) -1) {
        pUnk = m_SingleSink;
        m_SingleSink = NULL;
        if (m_rgSinks) {
            m_rgSinks[0] = NULL;
        }
    } else {
        if (dwCookie <= (DWORD)m_cAllocatedSinks) {
            if (m_rgSinks) {
                pUnk = m_rgSinks[dwCookie-1];
                m_rgSinks[dwCookie-1] = NULL;
            } else {
                return CONNECT_E_NOCONNECTION;
            }
        } else {
            return CONNECT_E_NOCONNECTION;
        }
    }

    m_cSinks--;

    if (!m_cSinks && m_rgSinks) {
        HeapFree(g_hHeap, 0, m_rgSinks);
        m_cAllocatedSinks = 0;
        m_rgSinks = NULL;
    }

    if (pUnk) {
        pUnk->Release();
        return S_OK;
    } else {
        return CONNECT_E_NOCONNECTION;
    }
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::EnumConnections
//=--------------------------------------------------------------------------=
// enumerates all current connections
//
// Paramters:
//    IEnumConnections ** - [out] new enumerator object
//
// Output:
//    HRESULT
//
// NOtes:
//
STDMETHODIMP COleControl::CConnectionPoint::EnumConnections
(
    IEnumConnections **ppEnumOut
)
{
    CONNECTDATA *rgConnectData = NULL;
    unsigned int i;

    if (m_cSinks) {
        // allocate some memory big enough to hold all of the sinks.
        //
        rgConnectData = (CONNECTDATA *)HeapAlloc(g_hHeap, 0, m_cSinks * sizeof(CONNECTDATA));
        RETURN_ON_NULLALLOC(rgConnectData);

        if ((m_cSinks == 1) && !m_rgSinks) {
            rgConnectData[0].pUnk = m_SingleSink;
            rgConnectData[0].dwCookie = (DWORD)-1;
        } else {
            for (unsigned int x = 0, i=0; x < m_cAllocatedSinks; x++) {
                if (m_rgSinks[x]) {
                    rgConnectData[i].pUnk = m_rgSinks[x];
                    rgConnectData[i].dwCookie = x+1;
                    i++;
                }
            }
        }
    }

    // create yon enumerator object.
    //
    *ppEnumOut = (IEnumConnections *)(IEnumGeneric *)new CStandardEnum(IID_IEnumConnections,
                        m_cSinks, sizeof(CONNECTDATA), rgConnectData, CopyAndAddRefObject);
    if (!*ppEnumOut) {
        HeapFree(g_hHeap, 0, rgConnectData);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::~CConnectionPoint
//=--------------------------------------------------------------------------=
// cleans up
//
// Notes:
//
COleControl::CConnectionPoint::~CConnectionPoint ()
{
    int x;

    // clean up some memory stuff
    //
    if (!m_cSinks)
        return;
    else if (m_SingleSink)
        ((IUnknown *)m_SingleSink)->Release();
    else {
        for (x = 0; x < m_cAllocatedSinks; x++) {
    	    if (m_rgSinks[x]) {
                QUICK_RELEASE(m_rgSinks[x]);
	        }
        }
        HeapFree(g_hHeap, 0, m_rgSinks);
    }
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPiont::DoInvoke
//=--------------------------------------------------------------------------=
// fires an event to all listening on our event interface.
//
// Parameters:
//    DISPID            - [in] event to fire.
//    DISPPARAMS        - [in]
//
// Notes:
//
void COleControl::CConnectionPoint::DoInvoke
(
    DISPID      dispid,
    DISPPARAMS *pdispparams
)
{
    int iConnection;

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.
    //
    if (m_cSinks == 0)
        return;
    else if (m_SingleSink) {
        ((IDispatch *)m_SingleSink)->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
    } else {
        for (iConnection = 0; iConnection < m_cAllocatedSinks; iConnection++) {
    	    if (m_rgSinks[iConnection]) {
                ((IDispatch *)m_rgSinks[iConnection])->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
	        }
    	}
    }
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::DoOnChanged
//=--------------------------------------------------------------------------=
// fires the OnChanged event for IPropertyNotifySink listeners.
//
// Parameters:
//    DISPID            - [in] dude that changed.
//
// Output:
//    none
//
// Notes:
//
void COleControl::CConnectionPoint::DoOnChanged
(
    DISPID dispid
)
{
    int iConnection;

    // if we don't have any sinks, then there's nothing to do.
    //
    if (m_cSinks == 0)
        return;
    else if (m_SingleSink) {
        ((IPropertyNotifySink *)m_SingleSink)->OnChanged(dispid);
    } else {
        for (iConnection = 0; iConnection < m_cAllocatedSinks; iConnection++) {
    	    if (m_rgSinks[iConnection]) {
                ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnChanged(dispid);
            }
	    }
    }
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::DoOnRequestEdit
//=--------------------------------------------------------------------------=
// fires the OnRequestEdit for IPropertyNotifySinkListeners
//
// Parameters:
//    DISPID             - [in] dispid user wants to change.
//
// Output:
//    BOOL               - false means you cant
//
// Notes:
//
BOOL COleControl::CConnectionPoint::DoOnRequestEdit
(
    DISPID dispid
)
{
    HRESULT hr;
    int     iConnection;

    // if we don't have any sinks, then there's nothing to do.
    //
    if (m_cSinks == 0)
        hr = S_OK;
    else if (m_SingleSink) {
        hr =((IPropertyNotifySink *)m_SingleSink)->OnRequestEdit(dispid);
    } else {
        for (iConnection = 0; iConnection < m_cAllocatedSinks; iConnection++) {
    	    if (m_rgSinks[iConnection]) {
                hr = ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnRequestEdit(dispid);
                if (hr != S_OK)
	        	    break;
    	    }
        }
    }

    return (hr == S_OK) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::CreateInPlaceWindow
//=--------------------------------------------------------------------------=
// creates the window with which we will be working.
// yay.
//
// Parameters:
//    int            - [in] left
//    int            - [in] top
//    BOOL           - [in] can we skip redrawing?
//
// Output:
//    HWND
//
// Notes:
//    - DANGER! DANGER!  this function is protected so that anybody can call it
//      from their control.  however, people should be extremely careful of when
//      and why they do this.  preferably, this function would only need to be
//      called by an end-control writer in design mode to take care of some
//      hosting/painting issues.  otherwise, the framework should be left to
//      call it when it wants.
//
HWND COleControl::CreateInPlaceWindow
(
    int  x,
    int  y,
    BOOL fNoRedraw
)
{
    BOOL    fVisible;
    HRESULT hr;
    DWORD   dwWindowStyle, dwExWindowStyle;
    char    szWindowTitle[128];

    // if we've already got a window, do nothing.
    //
    if (m_hwnd)
        return m_hwnd;

    // get the user to register the class if it's not already
    // been done.  we have to critical section this since more than one thread
    // can be trying to create this control
    //
    EnterCriticalSection(&g_CriticalSection);
    if (!CTLWNDCLASSREGISTERED(m_ObjectType)) {
        if (!RegisterClassData()) {
            LeaveCriticalSection(&g_CriticalSection);
            return NULL;
        } else
            CTLWNDCLASSREGISTERED(m_ObjectType) = TRUE;
    }
    LeaveCriticalSection(&g_CriticalSection);

    // let the user set up things like the window title, the
    // style, and anything else they feel interested in fiddling
    // with.
    //
    dwWindowStyle = dwExWindowStyle = 0;
    szWindowTitle[0] = '\0';
    if (!BeforeCreateWindow(&dwWindowStyle, &dwExWindowStyle, szWindowTitle))
        return NULL;

    dwWindowStyle |= (WS_CHILD | WS_CLIPSIBLINGS);

    // create window visible if parent hidden (common case)
    // otherwise, create hidden, then shown.  this is a little subtle, but
    // it makes sense eventually.
    //
    if (!m_hwndParent)
        m_hwndParent = GetParkingWindow();

    fVisible = IsWindowVisible(m_hwndParent);

    // This one kinda sucks -- if a control is subclassed, and we're in
    // a host that doesn't support Message Reflecting, we have to create
    // the user window in another window which will do all the reflecting.
    // VERY blech. [don't however, bother in design mode]
    //
    if (SUBCLASSWNDPROCOFCONTROL(m_ObjectType) && (m_hwndParent != GetParkingWindow())) {
        // determine if the host supports message reflecting.
        //
        if (!m_fCheckedReflecting) {
            VARIANT_BOOL f;
            hr = GetAmbientProperty(DISPID_AMBIENT_MESSAGEREFLECT, VT_BOOL, &f);
            if (FAILED(hr) || !f)
                m_fHostReflects = FALSE;
            m_fCheckedReflecting = TRUE;
        }

        // if the host doesn't support reflecting, then we have to create
        // an extra window around the control window, and then parent it
        // off that.
        //
        if (!m_fHostReflects) {
            ASSERT(m_hwndReflect == NULL, "Where'd this come from?");
            m_hwndReflect = CreateReflectWindow(!fVisible, m_hwndParent, x, y, &m_Size);
            if (!m_hwndReflect)
                return NULL;
            SetWindowLongPtr(m_hwndReflect, GWLP_USERDATA, (LONG_PTR)this);
            dwWindowStyle |= WS_VISIBLE;
        }
    } else {
        if (!fVisible)
            dwWindowStyle |= WS_VISIBLE;
    }

    // we have to mutex the entire create window process since we need to use
    // the s_pLastControlCreated to pass in the object pointer.  nothing too
    // serious
    //
    EnterCriticalSection(&g_CriticalSection);
    s_pLastControlCreated = this;
    m_fCreatingWindow = TRUE;

    // finally, go create the window, parenting it as appropriate.
    //
    m_hwnd = CreateWindowEx(dwExWindowStyle,
                            WNDCLASSNAMEOFCONTROL(m_ObjectType),
                            szWindowTitle,
                            dwWindowStyle,
                            (m_hwndReflect) ? 0 : x,
                            (m_hwndReflect) ? 0 : y,
                            m_Size.cx, m_Size.cy,
                            (m_hwndReflect) ? m_hwndReflect : m_hwndParent,
                            NULL, g_hInstance, NULL);

    // clean up some variables, and leave the critical section
    //
    m_fCreatingWindow = FALSE;
    s_pLastControlCreated = NULL;
    LeaveCriticalSection(&g_CriticalSection);

    if (m_hwnd) {
        // let the derived-control do something if they so desire
        //
        if (!AfterCreateWindow()) {
            BeforeDestroyWindow();
            SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)0xFFFFFFFF);
            DestroyWindow(m_hwnd);
            m_hwnd = NULL;
            return m_hwnd;
        }

        // if we didn't create the window visible, show it now.
        //
		
        if (fVisible)
		{
			if (GetParent(m_hwnd) != m_hwndParent)
				// SetWindowPos fails if parent hwnd is passed in so keep
				// this behaviour in those cases without ripping.
				SetWindowPos(m_hwnd, m_hwndParent, 0, 0, 0, 0,
							 SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW | ((fNoRedraw) ? SWP_NOREDRAW : 0));
		}
    }

    // finally, tell the host of this
    //
    if (m_pClientSite)
        m_pClientSite->ShowObject();

    return m_hwnd;
}

//=--------------------------------------------------------------------------=
// COleControl::SetInPlaceParent    [helper]
//=--------------------------------------------------------------------------=
// sets up the parent window for our control.
//
// Parameters:
//    HWND            - [in] new parent window
//
// Notes:
//
void COleControl::SetInPlaceParent
(
    HWND hwndParent
)
{
    ASSERT(!m_pInPlaceSiteWndless, "This routine should only get called for windowed OLE controls");

    if (m_hwndParent == hwndParent)
        return;

    m_hwndParent = hwndParent;
    if (m_hwnd)
        SetParent(GetOuterWindow(), hwndParent);
}

//=--------------------------------------------------------------------------=
// COleControl::ControlWindowProc
//=--------------------------------------------------------------------------=
// default window proc for an OLE Control.   controls will have their own
// window proc called from this one, after some processing is done.
//
// Parameters:
//    - see win32sdk docs.
//
// Notes:
//
LRESULT CALLBACK COleControl::ControlWindowProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    COleControl *pCtl = ControlFromHwnd(hwnd);
    HRESULT hr;
    LRESULT lResult;
    DWORD   dwCookie;

    // if the value isn't a positive value, then it's in some special
    // state [creation or destruction]  this is safe because under win32,
    // the upper 2GB of an address space aren't available.
    //
    if (!pCtl) {
        pCtl = s_pLastControlCreated;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pCtl);
            // This test and else clause, which you would resonably expect to never
            // ever happen, is needed for the plugin control when hosting the SGI
            // COSMO plugin.  That plugin queries GWL_USERDATA and uses it as a pointer.
            // In IE4.0 we in-place deactivate it when IE3 did not, causing the window
            // to be destroyed, and the SGI COSMO plugin to fault.  The fix is for the
            // Plugin control to never use the GWL_USERDATA == -1 mechanism.
            // -Tomsn, 1/2/97 (happy new year), IE4 bug 13292.
        if( pCtl != NULL ) {
            pCtl->m_hwnd = hwnd;
        }
        else {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } else if ((LONG_PTR)pCtl == (LONG_PTR)0xffffffff) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // message preprocessing
    //
    if (pCtl->m_pSimpleFrameSite) {
        hr = pCtl->m_pSimpleFrameSite->PreMessageFilter(hwnd, msg, wParam, lParam, &lResult, &dwCookie);
        if (hr == S_FALSE) return lResult;
    }

    // for certain messages, do not call the user window proc. instead,
    // we have something else we'd like to do.
    //
    switch (msg) {
      case WM_PAINT:
        {
        // call the user's OnDraw routine.
        //
        PAINTSTRUCT ps;
        RECT        rc;
        HDC         hdc;

        // if we're given an HDC, then use it
        //
        if (!wParam)
            hdc = BeginPaint(hwnd, &ps);
        else
            hdc = (HDC)wParam;

        GetClientRect(hwnd, &rc);
        pCtl->OnDraw(DVASPECT_CONTENT, hdc, (RECTL *)&rc, NULL, NULL, TRUE);

        if (!wParam)
            EndPaint(hwnd, &ps);
        }
        break;

      default:
        // call the derived-control's window proc
        //
        lResult = pCtl->WindowProc(msg, wParam, lParam);
        break;
    }

    // message postprocessing
    //
    switch (msg) {

      case WM_NCDESTROY:
        // after this point, the window doesn't exist any more
        //
        pCtl->m_hwnd = NULL;
        break;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:
        // give the control site focus notification
        //
        if (pCtl->m_fInPlaceActive && pCtl->m_pControlSite)
            pCtl->m_pControlSite->OnFocus(msg == WM_SETFOCUS);
        break;

      case WM_SIZE:
        // a change in size is a change in view
        //
        if (!pCtl->m_fCreatingWindow)
            pCtl->ViewChanged();
        break;
    }

    // lastly, simple frame postmessage processing
    //
    if (pCtl->m_pSimpleFrameSite)
        pCtl->m_pSimpleFrameSite->PostMessageFilter(hwnd, msg, wParam, lParam, &lResult, dwCookie);

    return lResult;
}

//=--------------------------------------------------------------------------=
// COleControl::SetFocus
//=--------------------------------------------------------------------------=
// we have to override this routine to get UI Activation correct.
//
// Parameters:
//    BOOL              - [in] true means take, false release
//
// Output:
//    BOOL
//
// Notes:
//    - CONSIDER: this is pretty messy, and it's still not entirely clear
//      what the ole control/focus story is.
//
BOOL COleControl::SetFocus
(
    BOOL fGrab
)
{
    HRESULT hr;
    HWND    hwnd;

    // first thing to do is check out UI Activation state, and then set
    // focus [either with windows api, or via the host for windowless
    // controls]
    //
    if (m_pInPlaceSiteWndless) {
        if (!m_fUIActive && fGrab)
            if (FAILED(InPlaceActivate(OLEIVERB_UIACTIVATE))) return FALSE;

        hr = m_pInPlaceSiteWndless->SetFocus(fGrab);
        return (hr == S_OK) ? TRUE : FALSE;
    } else {

        // we've got a window.
        //
        if (m_fInPlaceActive) {
            hwnd = (fGrab) ? m_hwnd : m_hwndParent;
            if (!m_fUIActive && fGrab)
                return SUCCEEDED(InPlaceActivate(OLEIVERB_UIACTIVATE));
            else
                return SetGUIFocus(hwnd);
        } else
            return FALSE;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::SetGUIFocus
//=--------------------------------------------------------------------------=
// does the work of setting the Windows GUI focus to the specified window
//
// Parameters:
//    HWND              - [in] window that should get focus
//
// Output:
//    BOOL              - [out] whether setting focus succeeded
//
// Notes:
//    we do this because some controls host non-ole window hierarchies, like
// the Netscape plugin ocx.  in such cases, the control may need to be UIActive
// to function properly in the document, but cannot take the windows focus
// away from one of its child windows.  such controls may override this method
// and respond as appropriate.
//
BOOL COleControl::SetGUIFocus
(
    HWND hwndSet
)
{
    return (::SetFocus(hwndSet) == hwndSet);
}


//=--------------------------------------------------------------------------=
// COleControl::ReflectWindowProc
//=--------------------------------------------------------------------------=
// reflects window messages on to the child window.  very lame.
//
// Parameters and Output:
//    - see win32 sdk docs
//
// Notes:
//
LRESULT CALLBACK COleControl::ReflectWindowProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    COleControl *pCtl;

    switch (msg) {
        case WM_COMMAND:
        case WM_NOTIFY:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_DELETEITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_COMPAREITEM:
        case WM_HSCROLL:
        case WM_VSCROLL:
        case WM_PARENTNOTIFY:
        case WM_SETFOCUS:
        case WM_SIZE:
            pCtl = (COleControl *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pCtl)
                return SendMessage(pCtl->m_hwnd, OCM__BASE + msg, wParam, lParam);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientProperty    [callable]
//=--------------------------------------------------------------------------=
// returns the value of an ambient property
//
// Parameters:
//    DISPID        - [in]  property to get
//    VARTYPE       - [in]  type of desired data
//    void *        - [out] where to put the data
//
// Output:
//    BOOL          - FALSE means didn't work.
//
// Notes:
//
BOOL COleControl::GetAmbientProperty
(
    DISPID  dispid,
    VARTYPE vt,
    void   *pData
)
{
    DISPPARAMS dispparams;
    VARIANT v, v2;
    HRESULT hr;

    v.vt = VT_EMPTY;
    v.lVal = 0;
    v2.vt = VT_EMPTY;
    v2.lVal = 0;

    // get a pointer to the source of ambient properties.
    //
    if (!m_pDispAmbient) {
        if (m_pClientSite)
            m_pClientSite->QueryInterface(IID_IDispatch, (void **)&m_pDispAmbient);

        if (!m_pDispAmbient)
            return FALSE;
    }

    // now go and get the property into a variant.
    //
    memset(&dispparams, 0, sizeof(DISPPARAMS));
    hr = m_pDispAmbient->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams,
                                &v, NULL, NULL);
    if (FAILED(hr)) return FALSE;

    // we've got the variant, so now go an coerce it to the type that the user
    // wants.  if the types are the same, then this will copy the stuff to
    // do appropriate ref counting ...
    //
    hr = VariantChangeType(&v2, &v, 0, vt);
    if (FAILED(hr)) {
        VariantClear(&v);
        return FALSE;
    }

    // copy the data to where the user wants it
    //
    CopyMemory(pData, &(v2.lVal), g_rgcbDataTypeSize[vt]);
    VariantClear(&v);
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientFont    [callable]
//=--------------------------------------------------------------------------=
// gets the current font for the user.
//
// Parameters:
//    IFont **         - [out] where to put the font.
//
// Output:
//    BOOL             - FALSE means couldn't get it.
//
// Notes:
//
BOOL COleControl::GetAmbientFont
(
    IFont **ppFont
)
{
    IDispatch *pFontDisp;

    // we don't have to do much here except get the ambient property and QI
    // it for the user.
    //
    *ppFont = NULL;
    if (!GetAmbientProperty(DISPID_AMBIENT_FONT, VT_DISPATCH, &pFontDisp))
        return FALSE;

    pFontDisp->QueryInterface(IID_IFont, (void **)ppFont);
    pFontDisp->Release();
    return (*ppFont) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::DesignMode
//=--------------------------------------------------------------------------=
// returns TRUE if we're in Design mode.
//
// Output:
//    BOOL            - true is design mode, false is run mode
//
// Notes:
//
BOOL COleControl::DesignMode
(
    void
)
{
    VARIANT_BOOL f;

    // if we don't already know our run mode, go and get it.  we'll assume
    // it's true unless told otherwise [or if the operation fails ...]
    //
    if (!m_fModeFlagValid) {
        f = TRUE;
        m_fModeFlagValid = TRUE;
        GetAmbientProperty(DISPID_AMBIENT_USERMODE, VT_BOOL, &f);
        m_fRunMode = f;
    }

    return !m_fRunMode;
}


//=--------------------------------------------------------------------------=
// COleControl::FireEvent
//=--------------------------------------------------------------------------=
// fires an event.  handles arbitrary number of arguments.
//
// Parameters:
//    EVENTINFO *        - [in] struct that describes the event.
//    ...                - arguments to the event
//
// Output:
//    none
//
// Notes:
//    - use stdarg's va_* macros.
//
void __cdecl COleControl::FireEvent
(
    EVENTINFO *pEventInfo,
    ...
)
{
    va_list    valist;
    DISPPARAMS dispparams;
    VARIANT    rgvParameters[MAX_ARGS];
    VARIANT   *pv;
    VARTYPE    vt;
    int        iParameter;
    int        cbSize;

    ASSERT(pEventInfo->cParameters <= MAX_ARGS, "Don't support more than MAX_ARGS params.  sorry.");

    va_start(valist, pEventInfo);

    // copy the Parameters into the rgvParameters array.  make sure we reverse
    // them for automation
    //
    pv = &(rgvParameters[pEventInfo->cParameters - 1]);
    for (iParameter = 0; iParameter < pEventInfo->cParameters; iParameter++) {

        vt = pEventInfo->rgTypes[iParameter];

        // if it's a by value variant, then just copy the whole
        // dang thing
        //
        if (vt == VT_VARIANT)
            *pv = va_arg(valist, VARIANT);
        else {
            // copy the vt and the data value.
            //
            pv->vt = vt;
            if (vt & VT_BYREF)
                cbSize = sizeof(void *);
            else
                cbSize = g_rgcbDataTypeSize[vt];

            // small optimization -- we can copy 2/4 bytes over quite
            // quickly.
            //
            if (cbSize == sizeof(short))
                V_I2(pv) = va_arg(valist, short);
            else if (cbSize == 4)
                V_I4(pv) = va_arg(valist, long);
            else {
                // copy over 8 bytes
                //
                ASSERT(cbSize == 8, "don't recognize the type!!");
                V_CY(pv) = va_arg(valist, CURRENCY);
            }
        }

        pv--;
    }

    // fire the event
    //
    dispparams.rgvarg = rgvParameters;
    dispparams.cArgs = pEventInfo->cParameters;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cNamedArgs = 0;

    m_cpEvents.DoInvoke(pEventInfo->dispid, &dispparams);

    va_end(valist);
}

//=--------------------------------------------------------------------------=
// COleControl::AfterCreateWindow    [overridable]
//=--------------------------------------------------------------------------=
// something the user can pay attention to
//
// Output:
//    BOOL             - false means fatal error, can't continue
// Notes:
//
BOOL COleControl::AfterCreateWindow
(
    void
)
{
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::BeforeCreateWindow    [overridable]
//=--------------------------------------------------------------------------=
// called just before we create a window.  the user should register their
// window class here, and set up any other things, such as the title of
// the window, and/or sytle bits, etc ...
//
// Parameters:
//    DWORD *            - [out] dwWindowFlags
//    DWORD *            - [out] dwExWindowFlags
//    LPSTR              - [out] name of window to create
//
// Output:
//    BOOL               - false means fatal error, can't continue
//
// Notes:
//
BOOL COleControl::BeforeCreateWindow
(
    DWORD *pdwWindowStyle,
    DWORD *pdwExWindowStyle,
    LPSTR  pszWindowTitle
)
{
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::InvalidateControl    [callable]
//=--------------------------------------------------------------------------=
void COleControl::InvalidateControl
(
    LPCRECT lpRect
)
{
    if (m_fInPlaceActive)
        InvalidateRect(m_hwnd, lpRect, TRUE);
    else
        ViewChanged();

    // CONSIDER: one might want to call pOleAdviseHolder->OnDataChanged() here
    // if there was support for IDataObject
}

//=--------------------------------------------------------------------------=
// COleControl::SetControlSize    [callable]
//=--------------------------------------------------------------------------=
// sets the control size. they'll give us the size in pixels.  we've got to
// convert them back to HIMETRIC before passing them on!
//
// Parameters:
//    SIZEL *        - [in] new size
//
// Output:
//    BOOL
//
// Notes:
//
BOOL COleControl::SetControlSize
(
    SIZEL *pSize
)
{
    HRESULT hr;
    SIZEL slHiMetric;

    PixelToHiMetric(pSize, &slHiMetric);
    hr = SetExtent(DVASPECT_CONTENT, &slHiMetric);
    return (FAILED(hr)) ? FALSE : TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::RecreateControlWindow    [callable]
//=--------------------------------------------------------------------------=
// called by a [subclassed, typically] control to recreate it's control
// window.
//
// Parameters:
//    none
//
// Output:
//    HRESULT
//
// Notes:
//    - NOTE: USE ME EXTREMELY SPARINGLY! THIS IS AN EXTREMELY EXPENSIVE
//      OPERATION!
//
HRESULT COleControl::RecreateControlWindow
(
    void
)
{
    HRESULT hr;
    HWND    hwndPrev;

    // we need to correctly preserve the control's position within the
    // z-order here.
    //
    if (m_hwnd)
        hwndPrev = ::GetWindow(m_hwnd, GW_HWNDPREV);

    // if we're in place active, then we have to deactivate, and reactivate
    // ourselves with the new window ...
    //
    if (m_fInPlaceActive) {

        hr = InPlaceDeactivate();
        RETURN_ON_FAILURE(hr);
        hr = InPlaceActivate((m_fUIActive) ? OLEIVERB_UIACTIVATE : OLEIVERB_INPLACEACTIVATE);
        RETURN_ON_FAILURE(hr);

    } else if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
        if (m_hwndReflect) {
            DestroyWindow(m_hwndReflect);
            m_hwndReflect = NULL;
        }

        CreateInPlaceWindow(0, 0, FALSE);
    }

    // restore z-order position
    //
    if (m_hwnd)
        SetWindowPos(m_hwnd, hwndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    return m_hwnd ? S_OK : E_FAIL;
}

// from Globals.C. don't need to mutex it here since we only read it.
//
extern HINSTANCE g_hInstResources;

//=--------------------------------------------------------------------------=
// COleControl::GetResourceHandle    [callable]
//=--------------------------------------------------------------------------=
// gets the HINSTANCE of the DLL where the control should get resources
// from.  implemented in such a way to support satellite DLLs.
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE COleControl::GetResourceHandle
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
    // crit sect this for apartment threading support.
    //
    EnterCriticalSection(&g_CriticalSection);
    if (!g_fHaveLocale)
        // if we can't get the ambient locale id, then we'll just continue
        // with the globally set up value.
        //
        if (!GetAmbientProperty(DISPID_AMBIENT_LOCALEID, VT_I4, &g_lcidLocale))
            goto Done;

    g_fHaveLocale = TRUE;

  Done:
    LeaveCriticalSection(&g_CriticalSection);
    return ::GetResourceHandle();
}
