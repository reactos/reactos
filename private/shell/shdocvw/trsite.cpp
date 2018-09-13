#include "priv.h"
#include "basesb.h"

#define DISPID_ONTRANSITIONFINISH     1
const GUID DIID_ICSSTransitionEvents = {0x8E64AA50L,0xDC42,0x11D0,0x99,0x49,0x00,0xA0,0xC9,0x0A,0x8F,0xF2};

//
// Notes:
//  - CTransitionSite object is always contained in a CBaseBrowser object,
//   therefore, it does not have its own reference count.
//  - The macro CONTAINERMAP maps the pointer to this object up to the
//   containing object.
//  - CTransitionSite is a friend class of CBaseBrowser class.
//
// Notes:
//  - Having anything be a friend of CBaseBrowser is lame.
//  - This object is completely contained by it's parent object,
//    so to avoid a reference loop we keep our pContainer pointer
//    with no reference. Do not QI off it or we will leak!
//
#define TF_TRSITE           0
#define TF_TRDRAW           TF_ALWAYS
#define TF_TREXTDRAW        0
#define TF_TRLIFE           0
#define TF_ADDREFRELEASE    0
#define TF_TRSPB            0
#define TF_DEBUGQI          0

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define MIN_TRANSITION_DURATION     0.0
#define MAX_TRANSITION_DURATION     100.0

#define MIN_ONVIEWCHANGE_DURATION   1500    // ms

#define TSPB_CREATE_INCR            4
    // CTransitionSitePropertyBag property list create size

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
static const TCHAR      c_szTransitionsKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CSSFilters");
static const OLECHAR    c_szDurationProp[] = OLESTR("Duration");
static OLECHAR *        s_szApplyMethod = OLESTR("Apply");
static OLECHAR *        s_szPlayMethod = OLESTR("Play");
static OLECHAR *        s_szStopMethod = OLESTR("Stop");

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite
/////////////////////////////////////////////////////////////////////////////
#undef THISCLASS

CTransitionSite::CTransitionSite
(
    IShellBrowser * pcont
) : _pContainer(pcont) // DO NOT ADDREF
{
    TraceMsg(TF_TRLIFE, "TRS::ctor called");

    ASSERT(pcont != NULL);

    _uState = TRSTATE_NONE;
    _ptiCurrent = NULL;
}

CTransitionSite::~CTransitionSite
(
)
{
    TraceMsg(TF_TRLIFE, "TRS::dtor called");

#ifdef DEBUG
    ASSERT(_pSite == NULL);
    ASSERT(_psvNew == NULL);
    ASSERT(_pvoNew == NULL);
    ASSERT(_hwndViewNew == NULL);
    ASSERT(_pTransition == NULL);
    ASSERT(_pDispTransition == NULL);
    ASSERT(_dwTransitionSink == NULL);
#endif  // DEBUG

    for (int te = teFirstEvent; te < teNumEvents; te++)
        SAFERELEASE(_tiEventInfo[te].pPropBag);
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_ApplyTransition
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_ApplyTransition
(
    BOOL bSiteChanging
)
{
    HRESULT hrResult = S_OK;

    TraceMsg(TF_TRSITE, "TRS::_ApplyTransition(%d) called", bSiteChanging);

    if (_ptiCurrent != NULL)
        _OnComplete();

    ASSERT(_ptiCurrent == NULL);

    if (bSiteChanging && (_tiEventInfo[teSiteEnter].clsid != CLSID_NULL))
        _ptiCurrent = &_tiEventInfo[teSiteEnter];
    else if (_tiEventInfo[tePageEnter].clsid != CLSID_NULL)
        _ptiCurrent = &_tiEventInfo[tePageEnter];
    else
        _ptiCurrent = NULL;

    // Allow the contatiner a chance to set a transition if we don't have one.
    if (_ptiCurrent == NULL)
    {
        // vaIn.vt = VT_BOOL; vaIn.boolVal = Site is changing;
        VARIANTARG  vaIn = { VT_BOOL, bSiteChanging };

        // vaOut[0].vt = VT_I4; vaOut[0].lVal = TransitionEvent
        // vaOut[1].vt = VT_BSTR; vaOut[1].bstrVal = Transition String.
        VARIANTARG  vaOut[2] = { 0 };

        if (SUCCEEDED(hrResult = IUnknown_Exec(_pContainer,
                                                    &CGID_ShellDocView,
                                                    SHDVID_GETTRANSITION,
                                                    OLECMDEXECOPT_DODEFAULT,
                                                    &vaIn,
                                                    vaOut)))
        {
            TRANSITIONINFO ti = { 0 };

            if  (
                (vaOut[0].vt == VT_I4)
                &&
                ((vaOut[0].lVal >= teFirstEvent) && (vaOut[0].lVal < teNumEvents))
                &&
                (vaOut[1].vt == VT_BSTR)
                &&
                (vaOut[1].bstrVal != NULL)
                &&
                ParseTransitionInfo((LPWSTR)vaOut[1].bstrVal, &ti)
                )
            {
                hrResult = _SetTransitionInfo((TransitionEvent)vaOut[0].lVal, &ti);
                _ptiCurrent = &_tiEventInfo[vaOut[0].lVal];
            }
            else
                hrResult = E_UNEXPECTED;

            SAFERELEASE(ti.pPropBag);
        }

        TraceMsg(TF_TRSITE, "hrResult = 0x%.8X after _pContainer->Exec()", hrResult);

        VariantClear(&vaOut[0]);
        VariantClear(&vaOut[1]);
    }

    if (SUCCEEDED(hrResult))
    {
        if (SUCCEEDED(hrResult = _LoadTransition()))
        {
            ASSERT(_hwndViewNew != NULL);

            // We need to hide the view window to draw on our own DC.
            _fViewIsVisible = ::IsWindowVisible(_hwndViewNew);
            if (_fViewIsVisible)
                ::ShowWindow(_hwndViewNew, SW_HIDE);

            _pContainer->EnableModelessSB(FALSE);
        }
    }
    else
    {
        // Make sure we don't fiddle with the window visibility
        // in _OnComplete if we didn't succeed.
        _hwndViewNew = NULL;

        _OnComplete();
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_LoadTransition
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_LoadTransition
(
)
{
    HRESULT hrResult;

    TraceMsg(TF_TRSITE, "TRS::_LoadTransition called");

    if ((_ptiCurrent == NULL) || (_ptiCurrent->clsid == CLSID_NULL))
        return E_INVALIDARG;

    // Create the transition.
    for (;;)
    {
        ASSERT(_pTransition == NULL);
        if (FAILED(hrResult = CoCreateInstance( _ptiCurrent->clsid,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IHTMLViewFilter,
                                                (void **)&_pTransition)))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition CoCreateInstance failed 0x%X", hrResult);
            break;
        }

        ASSERT(_pDispTransition == NULL);
        if (FAILED(hrResult = _pTransition->QueryInterface( IID_IDispatch,
                                                            (void **)&_pDispTransition)))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition QI::IDispatch failed 0x%X", hrResult);
            break;
        }

        ASSERT(_dwTransitionSink == 0);
        if (FAILED(hrResult = ConnectToConnectionPoint( SAFECAST(this, IDispatch *),
                                                        DIID_ICSSTransitionEvents,
                                                        TRUE,
                                                        _pTransition,
                                                        &_dwTransitionSink,
                                                        NULL)))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition ConnectToConnectionPoint failed 0x%X", hrResult);
            break;
        }

        // Supply the property bag to the transition (if needed)
        IPersistPropertyBag * pPersistPropBag;
        if  (
            (_ptiCurrent->pPropBag != NULL)
            &&
            SUCCEEDED(_pTransition->QueryInterface(IID_IPersistPropertyBag, (void **)&pPersistPropBag))
            )
        {
            EVAL(SUCCEEDED(pPersistPropBag->InitNew()));
            EVAL(SUCCEEDED(pPersistPropBag->Load(SAFECAST(_ptiCurrent->pPropBag, IPropertyBag *), NULL)));
            ATOMICRELEASE(pPersistPropBag);
        }

        if (FAILED(hrResult = _pTransition->SetSite(SAFECAST(this, IHTMLViewFilterSite *))))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition _pT->SetSite failed 0x%X", hrResult);
            break;
        }

        if (FAILED(hrResult = _pTransition->SetSource(SAFECAST(this, IHTMLViewFilter *))))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition _pT->SetSource failed 0x%X", hrResult);
            break;
        }

        if (FAILED(hrResult = _InitWait()))
        {
            TraceMsg(TF_ERROR, "TRS::_LoadTransition _InitWait failed 0x%X", hrResult);
            break;
        }

        hrResult = S_OK;
        break;
    }

    if (FAILED(hrResult))
        _OnComplete();

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_SetTransitionInfo
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_SetTransitionInfo
(
    TransitionEvent     te,
    TRANSITIONINFO *    pti
)
{
    TraceMsg(TF_TRSITE, "TRS::_SetTransitionInfo(%d)", te);
    
    ASSERT((te >= teFirstEvent) && (te < teNumEvents));

    SAFERELEASE(_tiEventInfo[te].pPropBag);        

    _tiEventInfo[te] = *pti;

    if (_tiEventInfo[te].pPropBag != NULL)
        _tiEventInfo[te].pPropBag->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_InitWait
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_InitWait()
{
    HRESULT hrResult;

    TraceMsg(TF_TRSITE, "TRS::_InitWait called");

    ASSERT(_pTransition != NULL);
    ASSERT(_pDispTransition != NULL);
    ASSERT(_uState == TRSTATE_NONE);

    for (;;)
    {
        RECT rc;
        IBrowserService2 *pbs2;
        if (SUCCEEDED(_pContainer->QueryInterface(IID_IBrowserService2, (void **)&pbs2)))
        {
            pbs2->GetViewRect(&rc);
            pbs2->Release();
        }

        EVAL(SUCCEEDED(hrResult = _pTransition->SetPosition(&rc)));
        TraceMsg(TF_TRSITE, "TRS::_InitWait called _pTransition->SetPosition 0x%X", hrResult);

        DISPID dispid;
        if (FAILED(hrResult = _pDispTransition->GetIDsOfNames(  IID_NULL,
                                                                &s_szApplyMethod,
                                                                1,
                                                                LOCALE_USER_DEFAULT,
                                                                &dispid)))
        {
            TraceMsg(TF_ERROR, "TRS::_InitWait _pDispTransition->GetIDsOfNames(Apply) failed 0x%X", hrResult);
            break;
        }

        unsigned int uArgError = (unsigned int)-1;
        DISPPARAMS dispparamsNoArgs = { NULL, NULL, 0, 0 };
        if (FAILED(hrResult = _pDispTransition->Invoke( dispid,
                                                        IID_NULL,
                                                        LOCALE_USER_DEFAULT,
                                                        DISPATCH_METHOD,
                                                        &dispparamsNoArgs,
                                                        NULL,
                                                        NULL,
                                                        &uArgError)))
        {
            TraceMsg(TF_ERROR, "TRS::_InitWait _pDispTransition->Invoke(Apply) failed 0x%X", hrResult);
            break;
        }

        break;
    }

    if (SUCCEEDED(hrResult))
        _uState = TRSTATE_INITIALIZING;

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_StartTransition
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_StartTransition
(
)
{
    HRESULT hrResult;
    
    TraceMsg(TF_TRSITE, "TRS::_StartTransition");

    ASSERT(_uState == TRSTATE_INITIALIZING);

    for (;;)
    {
        if (_pDispTransition == NULL)
        {
            hrResult = E_UNEXPECTED;
            break;
        }

        if  (
            FAILED(hrResult = _psvNew->QueryInterface(IID_IViewObject, (void **)&_pvoNew))
            ||
            FAILED(hrResult = _pvoNew->SetAdvise(   DVASPECT_CONTENT,
                                                    0,
                                                    SAFECAST(this, IAdviseSink *)))
            )
        {
            TraceMsg(TF_ERROR, "TRS::_StartTransition QI for IViewObject failed 0x%X", hrResult);
            break;
        }

        _uState = TRSTATE_STARTPAINTING;

        DISPID dispid;
        if (FAILED(hrResult = _pDispTransition->GetIDsOfNames(  IID_NULL,
                                                                &s_szPlayMethod,
                                                                1,
                                                                LOCALE_USER_DEFAULT,
                                                                &dispid)))
        {
            TraceMsg(TF_ERROR, "TRS::_StartTransition _pDispTransition->GetIDsOfNames(Play) failed 0x%X", hrResult);
            break;
        }

        unsigned int uArgError = (unsigned int)-1;
        DISPPARAMS dispparamsNoArgs = { NULL, NULL, 0, 0 };
        if (FAILED(hrResult = _pDispTransition->Invoke( dispid,
                                                        IID_NULL,
		                                                LOCALE_USER_DEFAULT,
                                                        DISPATCH_METHOD,
                                                        &dispparamsNoArgs,
                                                        NULL,
                                                        NULL,
                                                        &uArgError)))
        {
            TraceMsg(TF_ERROR, "TRS::_StartTransition _pDispTransition->Invoke(Play) failed 0x%X", hrResult);
            break;
        }

        _uState = TRSTATE_PAINTING;
        break;
    }

    if (FAILED(hrResult))
        _OnComplete();

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_StopTransition
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_StopTransition
(
)
{
    HRESULT hrResult;

    for (;;)
    {
        if (_pDispTransition == NULL)
        {
            hrResult = E_UNEXPECTED;
            break;
        }

        DISPID dispid;
        if (FAILED(hrResult = _pDispTransition->GetIDsOfNames(  IID_NULL,
                                                                &s_szStopMethod,
                                                                1,
                                                                LOCALE_USER_DEFAULT,
                                                                &dispid)))
        {
            TraceMsg(TF_ERROR, "TRS::_StopTransition _pDispTransition->GetIDsOfNames(Stop) failed 0x%X", hrResult);
            break;
        }

        unsigned int uArgError = (unsigned int)-1;
        DISPPARAMS dispparamsNoArgs = { NULL, NULL, 0, 0 };
        if (FAILED(hrResult = _pDispTransition->Invoke( dispid,
                                                        IID_NULL,
		                                                LOCALE_USER_DEFAULT,
                                                        DISPATCH_METHOD,
                                                        &dispparamsNoArgs,
                                                        NULL,
                                                        NULL,
                                                        &uArgError)))
        {
            TraceMsg(TF_ERROR, "TRS::_StopTransition _pDispTransition->Invoke(Stop) failed 0x%X", hrResult);
            break;
        }

        break;
    }

    _OnComplete();

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_UpdateEventList
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_UpdateEventList
(
)
{
    SAFERELEASE(_tiEventInfo[teSiteEnter].pPropBag);
    _tiEventInfo[teSiteEnter] = _tiEventInfo[teSiteExit];
    _tiEventInfo[teSiteExit].clsid = CLSID_NULL;
    _tiEventInfo[teSiteExit].pPropBag = NULL;

    SAFERELEASE(_tiEventInfo[tePageEnter].pPropBag);
    _tiEventInfo[tePageEnter] = _tiEventInfo[tePageExit];
    _tiEventInfo[tePageExit].clsid = CLSID_NULL;
    _tiEventInfo[tePageExit].pPropBag = NULL;

    _uState     = TRSTATE_NONE;
    _ptiCurrent = NULL;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::_OnComplete
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::_OnComplete
(
)
{
    TraceMsg(TF_TRSITE, "TRS::_OnComplete() called");

    if (_dwTransitionSink != 0)
    {
        EVAL(SUCCEEDED(ConnectToConnectionPoint(SAFECAST(this, IDispatch *),
                                                DIID_ICSSTransitionEvents,
                                                FALSE,
                                                _pTransition,
                                                &_dwTransitionSink,
                                                NULL)));
        _dwTransitionSink = 0;
    }
    
    if (_pvoNew != NULL)
    {
        _pvoNew->SetAdvise(DVASPECT_CONTENT, 0, 0);
        ATOMICRELEASE(_pvoNew);
    }

    ATOMICRELEASE(_pDispTransition);

    if (_pTransition != NULL)
    {
        _pTransition->SetSource(NULL);
        _pTransition->SetSite(NULL);

        ASSERT(_pSite == NULL);

        ATOMICRELEASE(_pTransition);
    }

    if (_hwndViewNew != NULL)
    {
        if (_fViewIsVisible)
            ::ShowWindow(_hwndViewNew, SW_SHOW);

        _pContainer->EnableModelessSB(TRUE);
    }

    ATOMICRELEASE(_psvNew);
    _hwndViewNew = NULL;

    return _UpdateEventList();
}

/////////////////////////////////////////////////////////////////////////////
// IUnknown interface
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IHTMLViewFilter))
    {
        *ppvObj = SAFECAST(this, IHTMLViewFilter *);
    }
    else if (IsEqualIID(riid, IID_IHTMLViewFilterSite))
    {
        *ppvObj = SAFECAST(this, IHTMLViewFilterSite *);
    }
    else if (IsEqualIID(riid, DIID_ICSSTransitionEvents) ||
            IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = SAFECAST(this, IDispatch *);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        return _pContainer->QueryInterface(IID_IServiceProvider, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG CTransitionSite::AddRef()
{
    TraceMsg(TF_ADDREFRELEASE, "TRS::AddRef()");
    return _pContainer->AddRef();
}

ULONG CTransitionSite::Release()
{
    TraceMsg(TF_ADDREFRELEASE, "TRS::Release()");
    return _pContainer->Release();
}


/////////////////////////////////////////////////////////////////////////////
// IHTMLViewFilter interface
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::SetSource(IHTMLViewFilter * pFilter)
{
    ASSERT(0);
    return E_FAIL;
}

HRESULT CTransitionSite::GetSource
(
    IHTMLViewFilter ** ppFilter
)
{
    ASSERT(0);
    return E_FAIL;
}

HRESULT CTransitionSite::SetSite
(
    IHTMLViewFilterSite * pFilterSite
)
{
    TraceMsg(TF_TRSITE, "TRS::SetSite called with %x", pFilterSite);

    ATOMICRELEASE(_pSite);

    _pSite = pFilterSite;
    if (pFilterSite != NULL)
        pFilterSite->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::GetSite
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::GetSite
(
    IHTMLViewFilterSite ** ppFilterSite
)
{
    TraceMsg(TF_TRSITE, "TRS::GetSite called _pSite=%x", _pSite);
    *ppFilterSite = _pSite;

    if (_pSite != NULL)
    {
        _pSite->AddRef();
        return S_OK;
    }

    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::SetPosition
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::SetPosition
(
    LPCRECT prc
)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::Draw
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::Draw
(
    HDC     hDC,
    LPCRECT prc
)
{
    HRESULT hrResult;
    HWND    hwndView = NULL;    // init to suppress bogus C4701 warning

    TraceMsg(TF_TREXTDRAW,
             "TRS::Draw called with hdc=%x (%d,%d)-(%d,%d)",
             hDC, prc->left, prc->top, prc->right, prc->bottom);

    if ((_uState == TRSTATE_STARTPAINTING) || (_uState == TRSTATE_PAINTING))
    {
        ASSERT(_pvoNew != NULL);

        RECTL rcl = { prc->left, prc->top, prc->right, prc->bottom };

        if (FAILED(hrResult = _pvoNew->Draw(DVASPECT_CONTENT,
                                            -1,
                                            NULL,
                                            NULL,
                                            NULL,
                                            hDC,
                                            &rcl,
                                            NULL,
                                            NULL,
                                            0)))
        {
            TraceMsg(TF_ERROR, "TRS::Draw IVO::Draw failed 0x%X", hrResult);
            hwndView = _hwndViewNew;
        }
    }
    else
    {
        IShellView * psv;
        hrResult = _pContainer->QueryActiveShellView(&psv);
        if (SUCCEEDED(hrResult))
        {
            hrResult = OleDraw(psv, DVASPECT_CONTENT, hDC, prc);
            psv->Release();
        }
        if (FAILED(hrResult))
        {
            IBrowserService2 *pbs;
            
            TraceMsg(TF_WARNING, "TRS:Draw OleDraw failed 0x%X", hrResult);
            
            if (SUCCEEDED(_pContainer->QueryInterface(IID_IBrowserService2, (void **)&pbs)))
            {
                pbs->GetViewWindow(&hwndView);
                pbs->Release();
            }
        }
    }

    // As a last resort, if IViewObject::Draw fails, try WM_PRINT
    if (FAILED(hrResult))
    {
        ASSERT((hwndView != NULL) && IsWindow(hwndView));

        // DrawEdge(..., EDGE_SUNKEN, BF_ADJUST|BF_RECT|BF_SOFT);

        ::SendMessage(hwndView, WM_PRINT, (WPARAM)hDC, PRF_NONCLIENT | PRF_CLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
        hrResult = S_OK;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::GetStatusBits
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::GetStatusBits
(
    DWORD * pdwFlags
)
{
    *pdwFlags = FILTER_STATUS_OPAQUE;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IHTMLViewFilterSite interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::GetDC
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::GetDC
(
    LPCRECT prc,
    DWORD   dwFlags,
    HDC *   phDC
)
{
    HWND hwnd;
    _pContainer->GetWindow(&hwnd);
    *phDC = ::GetDC(hwnd);
    
    TraceMsg(TF_TRSITE, "TRS::GetDC returning *phDC=%x", *phDC);

    return ((*phDC != NULL) ? S_OK : E_FAIL);
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::ReleaseDC
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::ReleaseDC
(
    HDC hDC
)
{
    TraceMsg(TF_TRSITE, "TRS::ReleaseDC called with %x", hDC);
    
    HWND hwnd;
    _pContainer->GetWindow(&hwnd);
    ::ReleaseDC(hwnd, hDC);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::InvalidateRect
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::InvalidateRect
(
    LPCRECT prc,
    BOOL    fErase
)
{
#ifdef DEBUG
    if (prc) {
        TraceMsg(TF_TREXTDRAW, "TRS::InvalidateRect called with (%x, %x)-(%x, %x) fErase=%d",
             prc->left, prc->top, prc->right, prc->bottom, fErase);
    } else {
        TraceMsg(TF_TREXTDRAW, "TRS::InvalidateRect called prc=NULL, fErase=%d", fErase);
    }
#endif

    DWORD dwFlags = RDW_INVALIDATE | RDW_UPDATENOW;
    dwFlags |= (fErase ? RDW_ERASE : 0);

    HWND hwnd;
    _pContainer->GetWindow(&hwnd);
    ::RedrawWindow(hwnd, prc, NULL, dwFlags);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::InvalidateRgn
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::InvalidateRgn
(
    HRGN hrgn,
    BOOL fErase
)
{
    TraceMsg(TF_TRSITE, "TRS::InvalidateRgn called");

    DWORD dwFlags = RDW_INVALIDATE | RDW_UPDATENOW;
    dwFlags |= (fErase ? RDW_ERASE : 0);

    HWND hwnd;
    _pContainer->GetWindow(&hwnd);
    ::RedrawWindow(hwnd, NULL, hrgn, dwFlags);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::OnStatusBitsChange
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::OnStatusBitsChange
(
    DWORD dwFlags
)
{
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IAdviseSink interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::OnViewChange
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(void) CTransitionSite::OnViewChange(DWORD dwAspect, LONG lindex)
{
    static DWORD dwLastUpdate = 0;

    if  (
        ((GetTickCount() - dwLastUpdate) > MIN_ONVIEWCHANGE_DURATION)
        &&
        (_uState == TRSTATE_PAINTING)
        &&
        (dwAspect & DVASPECT_CONTENT)
        )
    {
        TraceMsg(TF_TRDRAW, "TRS::OnViewChange(%d)", lindex);

        _pSite->InvalidateRect(NULL, FALSE);
    }

    dwLastUpdate = GetTickCount();
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTransitionSite::Invoke
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSite::Invoke
(
    DISPID          dispidMember,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr
)
{
    ASSERT(pdispparams != NULL);
    if (pdispparams == NULL)
        return E_INVALIDARG;

    if (!(wFlags & DISPATCH_METHOD))
        return E_INVALIDARG;

    switch(dispidMember)
    {
        case DISPID_ONTRANSITIONFINISH:
        {
            _OnComplete();
            break;
        }

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTransitionSitePropertyBag
/////////////////////////////////////////////////////////////////////////////
#undef CTransitionSite

CTransitionSitePropertyBag::CTransitionSitePropertyBag
(
) : _cRef(1)
{
    TraceMsg(TF_TRLIFE, "TRSPropBag::ctor called");

    // Implicit: _hdpaProperties = NULL;
}

CTransitionSitePropertyBag::~CTransitionSitePropertyBag()
{
    TraceMsg(TF_TRLIFE, "TRSPropBag::dtor called");

    if (_hdpaProperties != NULL)
        DPA_DestroyCallback(_hdpaProperties, _DPA_FreeProperties, 0);
}

/////////////////////////////////////////////////////////////////////////////
// _DPA_FreeProperties
/////////////////////////////////////////////////////////////////////////////
int CTransitionSitePropertyBag::_DPA_FreeProperties(void *pv, void *pData)
{
    NAMEVALUE * pnv = (NAMEVALUE *)pv;

    ASSERT(pnv != NULL);
    ASSERT(pnv->pwszName != NULL);

    LocalFree(pnv->pwszName);
    VariantClear(&pnv->varValue);
    LocalFree(pnv);

    return 1;
}

/////////////////////////////////////////////////////////////////////////////
// _AddProperty
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSitePropertyBag::_AddProperty(WCHAR *wszPropName, VARIANT *pvarValue)
{
    NAMEVALUE * pnv = NULL;
    HRESULT     hrResult = E_FAIL;

    // Create the list if needed
    if (_hdpaProperties == NULL)
        _hdpaProperties = DPA_Create(TSPB_CREATE_INCR);

    for (;;)
    {
        if (_hdpaProperties == NULL)
            break;

        // Alloc a new name value pair
        pnv = (NAMEVALUE *)LocalAlloc(LPTR, SIZEOF(NAMEVALUE));
        if (pnv == NULL)
        {
            TraceMsg(TF_WARNING, "TRSPB Unable to alloc memory for property");
            break;
        }

        // Copy the name
        UINT cb = (lstrlenW(wszPropName)+1) * SIZEOF(wszPropName[0]);
        pnv->pwszName = (LPWSTR)LocalAlloc(LPTR, cb);
        if (pnv->pwszName == NULL)
        {
            TraceMsg(TF_WARNING, "TRSPB Unable to alloc memory for property");
            break;
        }

        StrCpyNW(pnv->pwszName, wszPropName, cb / sizeof(wszPropName[0]));

        // Copy the value
        if (FAILED(hrResult = VariantCopy(&pnv->varValue, pvarValue)))
        {
            TraceMsg(TF_WARNING, "TRSPB VariantCopy failed");
            break;
        }

        // Add the name value pair to the list
        if (DPA_AppendPtr(_hdpaProperties, pnv) == DPA_ERR)
            break;

#ifdef DEBUG
        TCHAR szPropName[80];
        SHUnicodeToTChar(wszPropName, szPropName, SIZEOF(szPropName));

        VARIANT vVal = { 0 };
        TCHAR   szPropVal[80];

        EVAL(SUCCEEDED(VariantChangeType(&vVal, pvarValue, 0, VT_BSTR)));
        SHUnicodeToTChar(V_BSTR(&vVal), szPropVal, SIZEOF(szPropVal));
        EVAL(SUCCEEDED(VariantClear(&vVal)));

        TraceMsg(TF_TRSPB, "TRSPB::_AddProperty added '%s = %s'", szPropName, szPropVal);
#endif  // DEBUG        

        hrResult = S_OK;
        break;
    }

    // Cleanup on error
    if (FAILED(hrResult))
    {
        if (pnv != NULL)
        {
            if (pnv->pwszName != NULL)
                LocalFree(pnv->pwszName);

            VariantClear(&pnv->varValue);

            LocalFree(pnv);
        }
    }

    return hrResult;
}

ULONG CTransitionSitePropertyBag::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTransitionSitePropertyBag::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTransitionSitePropertyBag::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyBag))
    {
        *ppvObj = SAFECAST(this, IPropertyBag *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPropertyBag interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTransitionSitePropertyBag::Read
/////////////////////////////////////////////////////////////////////////////
HRESULT CTransitionSitePropertyBag::Read
(
    LPCOLESTR   pszPropName,
    VARIANT *   pVar,
    IErrorLog * pErrorLog
)
{
    HRESULT hrResult = E_INVALIDARG;

    if ((pszPropName == NULL) || (pVar == NULL))
        return E_POINTER;

#ifdef DEBUG
    TCHAR szPropName[80];
    SHUnicodeToTChar(pszPropName, szPropName, SIZEOF(szPropName));
    TraceMsg(TF_TRSPB, "TRSPB::Read(%s)", szPropName);
#endif  // DEBUG

    if (_hdpaProperties != NULL)
    {
        // Search for the property in the list.
        for (int i = 0; i < DPA_GetPtrCount(_hdpaProperties); i++)
        {
            NAMEVALUE * pnv = (NAMEVALUE *)DPA_GetPtr(_hdpaProperties, i);

            if (StrCmpIW(pszPropName, pnv->pwszName) == 0)
            {
                // Copy the variant property.
                hrResult = VariantChangeType(pVar, &pnv->varValue, 0, pVar->vt);
                break;
            }
        }
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLSIDFromTransitionName
/////////////////////////////////////////////////////////////////////////////
HRESULT CLSIDFromTransitionName
(
    LPCTSTR pszName,
    LPCLSID clsidName
)
{
    HKEY    hkeyTransitions = NULL;
    HRESULT hrResult;

    for (;;)
    {
        // Check for "{...CLSID...}"
        if (*pszName == TEXT('{'))
        {
            hrResult = SHCLSIDFromString(pszName, clsidName);
            break;
        }

        // Check for Transition Name
        if (RegCreateKey(   HKEY_LOCAL_MACHINE,
                            c_szTransitionsKey,
                            &hkeyTransitions) == ERROR_SUCCESS)
        {
            TCHAR   szCLSID[GUIDSTR_MAX];
            DWORD   cbBytes = SIZEOF(szCLSID);
            DWORD   dwType;

            if  (
                (RegQueryValueEx(   hkeyTransitions,
                                    pszName,
                                    NULL,
                                    &dwType,
                                    (BYTE *)szCLSID,
                                    &cbBytes) == ERROR_SUCCESS)
                &&
                (dwType == REG_SZ)
                )
            {
                hrResult = SHCLSIDFromString(szCLSID, clsidName);
                break;
            }
        }

        hrResult = E_FAIL;
        break;
    }

    if (hkeyTransitions != NULL)
        RegCloseKey(hkeyTransitions);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ParseTransitionInfo
//
// Purpose: Parse <META HTTP-EQUIV...> of the form:
//
//      <META
//          HTTP-EQUIV = transition-event
//          CONTENT = transition-description    
//      >
//
// where:
//
//      transition-event -> PAGE-ENTER
//                              | PAGE-EXIT
//                              | SITE-ENTER
//                              | SITE-EXIT
//
//      transition-description -> transition-name ( transition-properties ) 
//
//      transition-name -> IDENTIFIER
//                              | CLSID 
//                              | PROGID
//
//      transition-properties -> transition-property , transition-property
//      transition-properties -> transition-property
//
//      name-value-assignment -> = | :
//
//      transition-property -> NAME name-value-assignment VALUE
//
// examples:
//
//      <META
//          HTTP-EQUIV = "Page-Enter"
//          CONTENT = "Reveal(duration=500)"
//      >
//
//      <META
//          HTTP-EQUIV = "Page-Leave"
//          CONTENT = "Reveal(duration=500, type=checkerboard, size=10)"
//      >
//
/////////////////////////////////////////////////////////////////////////////
BOOL ParseTransitionInfo
(
    WCHAR *             pwz,
    TRANSITIONINFO *    pti
)
{
    enum
    {
        PTE_PARSE_TRANSITION_NAME,
        PTE_PARSE_NAME,
        PTE_PARSE_VALUE,
        PTE_FINISHED
    };

    #define MAX_TRANSITION_NAME_LEN GUIDSTR_MAX
    WCHAR wszTransitionName[MAX_TRANSITION_NAME_LEN];
    WCHAR * pwzTransitionName = wszTransitionName;

    #define MAX_PTINAME_LEN     32
    WCHAR   wszName[MAX_PTINAME_LEN];
    WCHAR * pwzName = wszName;

    #define MAX_PTIVALUE_LEN    32
    WCHAR   wszValue[MAX_PTIVALUE_LEN];
    WCHAR * pwzValue = wszValue;

    WCHAR   wch;
    UINT    cch         = 0;
    UINT    uiState     = PTE_PARSE_TRANSITION_NAME;
    BOOL    bSucceeded  = FALSE;

    ASSERT(pti != NULL);

    do
    {
        wch = *pwz;

        switch (uiState)
        {
            case PTE_PARSE_TRANSITION_NAME:
            {
                if (wch == TEXT('('))  // Open paren
                {
                    cch     = 0;
                    uiState = PTE_PARSE_NAME;

                    *pwzTransitionName = '\0';
                    pwzTransitionName = wszTransitionName;

                    TCHAR szTransitionName[MAX_TRANSITION_NAME_LEN];
                    OleStrToStrN(szTransitionName, ARRAYSIZE(szTransitionName), wszTransitionName, (UINT)-1);

                    TraceMsg(DM_TRACE, "ParseTransitionInfo(%s)", szTransitionName);

                    // Resolve the Transition Name
                    EVAL(SUCCEEDED(CLSIDFromTransitionName(szTransitionName, &(pti->clsid))));
                }
                else if (
                        !ISSPACE(wch)
                        &&
                        (cch++ < (MAX_TRANSITION_NAME_LEN-1))
                        )
                {
                    *pwzTransitionName++ = wch;
                }
                else
                    ;   // Ignore
                    
                break;
            }

            case PTE_PARSE_NAME:
            {
                if  (
                    (wch == TEXT('='))  // Equal
                    ||
                    (wch == TEXT(':'))  // Semicolon
                    )
                {
                    cch     = 0;
                    uiState = PTE_PARSE_VALUE;

                    *pwzName = '\0';
                    pwzName = wszName;
                }
                else if (
                        !ISSPACE(wch)
                        &&
                        (cch++ < (MAX_PTINAME_LEN-1))
                        )
                {
                    *pwzName++ = wch;
                }
                else
                    ;   // Ignore

                break;
            }

            case PTE_PARSE_VALUE:
            {
                if  (
                    (wch == TEXT(','))  // Comma
                    ||
                    (wch == TEXT(')'))  // Close paren
                    )
                {
                    cch = 0;
                    if (wch == TEXT(','))
                        uiState = PTE_PARSE_NAME;
                    else
                        uiState = PTE_FINISHED;

                    *pwzValue = '\0';
                    pwzValue = wszValue;

                    VARIANT v; V_VT(&v) = VT_BSTR; V_BSTR(&v) = SysAllocString(wszValue);

                    // Initialize the property bag class
                    if (pti->pPropBag == NULL)
                        pti->pPropBag = new CTransitionSitePropertyBag;

                    if (pti->pPropBag)
                    {
                        // Limit the duration of the transition.
                        if (StrCmpIW(wszName, c_szDurationProp) == 0)
                        {
                            VARIANT vDuration = { 0 };

                            if (SUCCEEDED(VariantChangeTypeEx(&vDuration, &v, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 0, VT_R4)))
                            {
                                ASSERT(V_VT(&vDuration) == VT_R4);

                                if (V_R4(&vDuration) < MIN_TRANSITION_DURATION)
                                    V_R4(&vDuration) = MIN_TRANSITION_DURATION;
                                else if (V_R4(&vDuration) > MAX_TRANSITION_DURATION)
                                    V_R4(&vDuration) = MAX_TRANSITION_DURATION;

                                EVAL(SUCCEEDED(VariantClear(&v)));
                                EVAL(SUCCEEDED(VariantCopy(&v, &vDuration)));
                                EVAL(SUCCEEDED(VariantClear(&vDuration)));
                            }
                        }

                        // Add the property to the property bag
                        EVAL(SUCCEEDED(pti->pPropBag->_AddProperty(wszName, &v)));
                    }

                    EVAL(SUCCEEDED(VariantClear(&v)));
                }
                else if (
                        !ISSPACE(wch)
                        &&
                        (cch++ < (MAX_PTIVALUE_LEN-1))
                        )
                {
                    *pwzValue++ = wch;
                }
                else
                    ;   // Ignore

                break;
            }

            case PTE_FINISHED:
            {
                bSucceeded = TRUE;
                break;
            }

            default:
                break;
        }

        pwz++;
    } while (wch && !bSucceeded);

    return bSucceeded;
} // ParseTransitionInfo
