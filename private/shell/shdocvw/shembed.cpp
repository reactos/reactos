#include "priv.h"
#include "sccls.h"

#include <mluisupp.h>

#define IPSMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IPS::%s called", psz)
#define IPSMSG2(psz, hres)      TraceMsg(TF_SHDCONTROL, "she TR-IPS::%s %x", psz, hres)
#define IPSMSG3(pszName, psz)   TraceMsg(TF_SHDCONTROL, "she TR-IPS::%s:%s called", pszName,psz)
#define IOOMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IOO::%s called", psz)
#define IOOMSGX(psz, hres)      TraceMsg(TF_SHDCONTROL, "she TR-IOO::%s returning %x", psz, hres)
#define IOOMSG2(psz, i)         TraceMsg(TF_SHDCONTROL, "she TR-IOO::%s called with (%d)", psz, i)
#define IOOMSG3(psz, i, j)      TraceMsg(TF_SHDCONTROL, "she TR-IOO::%s called with (%d, %d)", psz, i, j)
#define IVOMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IVO::%s called", psz)
#define IVOMSG2(psz, i)         TraceMsg(TF_SHDCONTROL, "she TR-IVO::%s called with (%d)", psz, i)
#define IVOMSG3(psz, i, j)      TraceMsg(TF_SHDCONTROL, "she TR-IVO::%s with (%d, %d)", psz, i, j)
#define CCDMSG(psz, punk)       TraceMsg(TF_SHDCONTROL, "she TR-CSV::%s called punk=%x", psz, punk)
#define IDTMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IDT::%s called", psz)
#define IDTMSG2(psz, i)         TraceMsg(TF_SHDCONTROL, "she TR-IDT::%s called with %d", psz, i)
#define IDTMSG3(psz, x)         TraceMsg(TF_SHDCONTROL, "she TR-IDT::%s %x", psz, x)
#define IDTMSG4(psz, i, j)      TraceMsg(TF_SHDCONTROL, "she TR-IDT::%s called with %x,%x", psz, i, j)
#define IIPMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IOIPO::%s called", psz)
#define IIAMSG(psz)             TraceMsg(TF_SHDCONTROL, "she TR-IOIPAO::%s called", psz)
#define IEVMSG(psz, i, j, ps)   TraceMsg(TF_SHDCONTROL, "she TR-IEV::%s called celt=%d, _iCur=%d, %x", psz, i, j, ps)

const TCHAR c_szShellEmbedding[] = TEXT("Shell Embedding");

//
// A special lindex value to be passed to ::Draw member indicating
// that it is an internal call from ::GetData
//
#define LINDEX_INTERNAL 12345

// REVIEW: We may want to use the functions in UTIL.C -- they look more efficient...
//
//=========================================================================
// Helper functions
//=========================================================================

#define HIM_PER_IN 2540

int g_iXppli = 0;
int g_iYppli = 0;

void GetLogPixels()
{
    HDC hdc = GetDC(NULL);
    g_iXppli = GetDeviceCaps(hdc, LOGPIXELSX);
    g_iYppli = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
}

// Scalar conversion of MM_HIMETRIC to MM_TEXT
void MetricToPixels(SIZEL* psize)
{
    ASSERT(g_iXppli);

    psize->cx = MulDiv(psize->cx, g_iXppli, HIM_PER_IN);
    psize->cy = MulDiv(psize->cy, g_iYppli, HIM_PER_IN);
}

// Scalar conversion of MM_TEXT to MM_HIMETRIC
void PixelsToMetric(SIZEL* psize)
{
    ASSERT(g_iYppli);

    psize->cx = MulDiv(psize->cx, HIM_PER_IN, g_iXppli);
    psize->cy = MulDiv(psize->cy, HIM_PER_IN, g_iYppli);
}


//=========================================================================
// CShellEmbedding implementaiton
//=========================================================================
HRESULT CShellEmbedding::v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellEmbedding, IPersist),
        QITABENT(CShellEmbedding, IOleObject),
        QITABENT(CShellEmbedding, IViewObject2),
        QITABENTMULTI(CShellEmbedding, IViewObject, IViewObject2),
        QITABENT(CShellEmbedding, IDataObject),
        QITABENT(CShellEmbedding, IOleInPlaceObject),
        QITABENTMULTI(CShellEmbedding, IOleWindow, IOleInPlaceObject),
        QITABENT(CShellEmbedding, IOleInPlaceActiveObject),
        QITABENT(CShellEmbedding, IInternetSecurityMgrSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

CShellEmbedding::CShellEmbedding(IUnknown* punkOuter, LPCOBJECTINFO poi, const OLEVERB* pverbs)
    : _pverbs(pverbs)
    , _nActivate(OC_DEACTIVE)
    , CAggregatedUnknown(punkOuter)
{
    TraceMsg(TF_SHDCONTROL, "ctor CShellEmbedding %x", this);

    DllAddRef();
    _RegisterWindowClass();
    _pObjectInfo = poi;
    _size.cx = 50;
    _size.cy = 20;

    // make sure some globals are set
    GetLogPixels();

    // let our logical size match our physical size
    _sizeHIM = _size;
    PixelsToMetric(&_sizeHIM);
}

CShellEmbedding::~CShellEmbedding()
{
    ASSERT(_hwnd==NULL);
    // IE v 4.1 bug 44541.  In an Office 97 user form, we were seeing this destructor get entered
    // with a non-null hwnd, which would cause a fault the next time the hwnd received a message.
    // 
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
    ASSERT(_hwndChild==NULL);
    ASSERT(_pcli==NULL);
    ASSERT(_pipsite==NULL);
    ASSERT(_pipframe==NULL);
    ASSERT(_pipui==NULL);

    //
    // WARNING: Don't call any of virtual functions of this object
    //  itself for clean-up purpose. The Vtable is alreadly adjusted
    //  and we won't be able to perform any full clean up. Do it
    //  right before you delete in CShellEmbedding::CSVInner::Release.
    //
    TraceMsg(TF_SHDCONTROL, "dtor CShellEmbedding %x", this);

    // Warning: if the client site has not been released do not release the advise
    // object as some applications like VC5 will fault on this...
    if (_padv) {
        _padv->OnClose();
        if (!_pcli)
            ATOMICRELEASE(_padv);
    }

    if (!_pcli)
    {
        ATOMICRELEASE(_pdah);
        ATOMICRELEASE(_poah);
    }
    ATOMICRELEASE(_pstg);
    ATOMICRELEASE(_pcliHold);


    DllRelease();
}

// **************** IPersist ****************
HRESULT CShellEmbedding::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSIDOFOBJECT(this);

    return S_OK;
}

BOOL CShellEmbedding::_ShouldDraw(LONG lindex)
{
    // Don't draw if the window is visible.
    return ! (_pipsite && lindex!=LINDEX_INTERNAL);
}

// **************** IViewObject ****************
HRESULT CShellEmbedding::Draw(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hdcTargetDev,
    HDC hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    BOOL ( __stdcall *pfnContinue )(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    IVOMSG3(TEXT("Draw called"), lprcBounds->top, lprcBounds->bottom);

    // BUGBUG: this looks wrong to me -- I think we should always respond
    // to a Draw request, as the hdcDraw may not be the screen!
    //
    // Don't draw if the window is visible.
    if (!_ShouldDraw(lindex)) {
        return S_OK;
    }

    if (_hwnd) {
        int iDC = SaveDC(hdcDraw);
          RECTL rcBounds = *lprcBounds;
          ::LPtoDP(hdcDraw, (LPPOINT)&rcBounds, 2);
          IVOMSG3(TEXT("Draw DP=="), rcBounds.top, rcBounds.bottom);
          TraceMsg(TF_SHDCONTROL, "she Draw cx=%d cy=%d", rcBounds.right-rcBounds.left, rcBounds.bottom-rcBounds.top);

          SetMapMode(hdcDraw, MM_TEXT);         // make it 1:1
          SetMapMode(hdcDraw, MM_ANISOTROPIC);  // inherit call from MM_TEXT
          POINT pt;
          SetViewportOrgEx(hdcDraw, rcBounds.left, rcBounds.top, &pt);

          // BUGBUG: WordPad does a GetExtent to get the size and passes that in as lprcBounds
          // *without* doing a SetExtent, so when we resize larger (due to a BROWSE verb) _hwnd
          // is still the old size. So we IntersectClipRect to _hwnd but WordPad draws the border
          // to rcBounds. Ugly.

           RECT rc;
           GetClientRect(_hwnd, &rc);
           IntersectClipRect(hdcDraw, 0, 0, rc.right, rc.bottom);
           SendMessage(_hwnd, WM_PRINT, (WPARAM)hdcDraw,
                       PRF_NONCLIENT|PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);

         SetViewportOrgEx(hdcDraw, pt.x, pt.y, NULL);
        RestoreDC(hdcDraw, iDC);
        return S_OK;
    }

    return OLE_E_BLANK;
}

HRESULT CShellEmbedding::GetColorSet(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    IVOMSG(TEXT("GetColorSet"));
    return S_FALSE;     // Indicating that the object doesn't care
}

HRESULT CShellEmbedding::Freeze(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DWORD *pdwFreeze)
{
    IVOMSG(TEXT("Freeze"));
    *pdwFreeze = 0;
    return S_OK;
}

HRESULT CShellEmbedding::Unfreeze(DWORD dwFreeze)
{
    IVOMSG(TEXT("Unfreeze"));
    return S_OK;
}

HRESULT CShellEmbedding::SetAdvise(
    DWORD aspects,
    DWORD advf,
    IAdviseSink *pAdvSink)
{
    IVOMSG2(TEXT("SetAdvise"), pAdvSink);

    if (advf & ~(ADVF_ONLYONCE | ADVF_PRIMEFIRST))
        return E_INVALIDARG;

    if (pAdvSink != _padv)
    {
        ATOMICRELEASE(_padv);

        if (pAdvSink)
        {
            _padv = pAdvSink;
            _padv->AddRef();
        }
    }

    _asp  = aspects;
    _advf = advf;

    if (advf & ADVF_PRIMEFIRST)
        _SendAdvise(OBJECTCODE_VIEWCHANGED);

    return S_OK;
}

HRESULT CShellEmbedding::GetAdvise(
    DWORD *pAspects,
    DWORD *pAdvf,
    IAdviseSink **ppAdvSink)
{
    IVOMSG(TEXT("GetAdvise"));
    if (pAspects) {
        *pAspects = _asp;
    }

    if (pAdvf) {
        *pAdvf = _advf;
    }

    if (ppAdvSink) {
        *ppAdvSink = _padv;
        if (_padv) {
            _padv->AddRef();
        }
    }

    return S_OK;
}

// **************** IViewObject2 ****************
HRESULT CShellEmbedding::GetExtent(
    DWORD dwDrawAspect,
    LONG lindex,
    DVTARGETDEVICE *ptd,
    LPSIZEL lpsizel)
{
    TraceMsg(TF_SHDCONTROL, "she GetExtent cx=%d cy=%d", _size.cx, _size.cy);
    lpsizel->cx = _size.cx;
    lpsizel->cy = _size.cy;
    PixelsToMetric(lpsizel);
    return S_OK;
}

//
// **************** IOleObject ****************
//

void CShellEmbedding::_OnSetClientSite()
{
    if (_pcli)
    {
        IOleInPlaceSite* pipsite;
        if (SUCCEEDED(_pcli->QueryInterface(IID_IOleInPlaceSite, (LPVOID*)&pipsite)))
        {
            _CreateWindowOrSetParent(pipsite);
            pipsite->Release();
        }
    }
    else if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
}

HRESULT CShellEmbedding::SetClientSite(IOleClientSite *pClientSite)
{
    IOOMSG2(TEXT("SetClientSite"), pClientSite);

    // If I have a client site on hold, get rid of it.
    //
    ATOMICRELEASE(_pcliHold);

    if (_pcli == pClientSite)
    {
        // mshtml is hitting their initialization code twice
        // no need for us to do anything here.
    }
    else
    {
        ATOMICRELEASE(_pcli);
        ATOMICRELEASE(_pipsite);
        ATOMICRELEASE(_pipframe);
        ATOMICRELEASE(_pipui);
    
        _pcli = pClientSite;
    
        if (_pcli)
            _pcli->AddRef();

        _OnSetClientSite();
    }

    return S_OK;
}


//
//  This function create _hwnd (the parent of this embedding) if it not
// created yet. Otherwise, it simply SetParent appropriately.
//
// BUGBUG: When this object is embedded in PowerPoint 95, the first
//  CreateWindowEx fails  when this function if called from SetClientSite
//  for some unknown reason.
//   It, however, succeeds when it is called from DoVerb. We should find
//  it out.
//
HRESULT CShellEmbedding::_CreateWindowOrSetParent(IOleWindow* pwin)
{
    HWND hwndParent = NULL;
    HRESULT hres = S_OK;

    //
    // NOTES: Unlike IE3.0, we don't fail even if pwin->GetWindow fails.
    //  It allows Trident to SetClientSite (and Navigate) before they
    //  are In-place Activated. In that case (hwndParent==NULL), we
    //  create a top-level window hidden and use it for navigation.
    //  When we are being InPlaceActivated, we hit this function again
    //  and set the parent (and window styles) correctly. Notice that
    //  we need to set WS_POPUP to avoid the Window Manager automatically
    //  add other random styles for verlapped window. 
    //
    pwin->GetWindow(&hwndParent);
#ifdef DEBUG
    // Pretend that GetWindow failed here.
    if (_hwnd==NULL && (g_dwPrototype & 0x00000200)) {
        TraceMsg(DM_TRACE, "CSE::_CreateWindowOrSetParent pretend unsuccessful GetWindow");
        hwndParent = NULL;
    }
#endif

    _fOpen = TRUE;
    
    if (_hwnd) {
        SetParentHwnd(_hwnd, hwndParent);

     } else {
        _hwnd = CreateWindowEx(
            WS_EX_WINDOWEDGE,
            c_szShellEmbedding, NULL,
            (hwndParent ?
                (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP)
                : (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP)),
            0, 0, _rcPos.right - _rcPos.left, _rcPos.bottom - _rcPos.top,
            hwndParent,
            (HMENU)0,
            HINST_THISDLL,
            (LPVOID)SAFECAST(this, CImpWndProc*));

        if (!_hwnd) {
            hres = E_FAIL;
            TraceMsg(TF_SHDCONTROL, "sdv TR-IOO::_CreateWindowOrSetParent CreateWindowEx failed (%d)", GetLastError());
        }
    }

    return hres;
}
HRESULT CShellEmbedding::GetClientSite(IOleClientSite **ppClientSite)
{
    IOOMSG(TEXT("GetClientSite"));
    *ppClientSite = _pcli;

    if (_pcli) {
        _pcli->AddRef();
    }

    return S_OK;
}

HRESULT CShellEmbedding::SetHostNames(
    LPCOLESTR szContainerApp,
    LPCOLESTR szContainerObj)
{
    IOOMSG(TEXT("SetHostNames"));
    // We are not interested in host name
    return S_OK;
}


// A container application calls IOleObject::Close when it wants
// to move the object from a running to a loaded state. Following
// such a call, the object still appears in its container but is
// not open for editing. Calling IOleObject::Close on an object
// that is loaded but not running has no effect.
//
HRESULT CShellEmbedding::Close(DWORD dwSaveOption)
{
    IOOMSG2(TEXT("Close"), dwSaveOption);
    // Change the state of object back to TEXT("loaded") state.

    BOOL fSave = FALSE;
    if (_fDirty &&
        ((OLECLOSE_SAVEIFDIRTY==dwSaveOption)
         || (dwSaveOption==OLECLOSE_PROMPTSAVE))) {
        fSave = TRUE;
    }

    if (fSave) {
        _SendAdvise(OBJECTCODE_SAVEOBJECT);
        _SendAdvise(OBJECTCODE_SAVED);
    }

    _SendAdvise(OBJECTCODE_CLOSED);
    _fOpen = FALSE;

    // "loaded but not running" is confusing wording... If you look
    // at the OLEIVERB_HIDE comment in _OnActivateChange, it mentions
    // that OLEIVERB_HIDE puts it in the state "just after loading"
    // and puts us in OC_DEACTIVE state. Let's do that here as well.
    //
    // BUGBUG: it just came to my awareness that OCs UIDeactivate,
    //         not IPDeactivate...
    _DoActivateChange(NULL, OC_DEACTIVE, FALSE);

    // It seems like some containers (Trident frame set) don't
    // do a SetClientSite(NULL) on us, so do it here. Old code here
    // did a DestroyWindow(_hwnd), which SetClientSite(NULL) will do.
    // NOTE: VB does call SetClientSite, but they do it after Close.

    // If we already have one on hold, release it.
    //
    ATOMICRELEASE(_pcliHold);

    // Hold onto our client site.  We may need it if we're DoVerbed, as Office tends to do.
    //

    IOleClientSite  *pOleClientSite = _pcli;
    if (pOleClientSite)
    {
        pOleClientSite->AddRef();
    }

    SetClientSite(NULL);

    _pcliHold = pOleClientSite;

    return S_OK;
}

HRESULT CShellEmbedding::SetMoniker(
    DWORD dwWhichMoniker,
    IMoniker *pmk)
{
    IOOMSG(TEXT("SetMoniker"));
    // We are not interested in moniker.
    return S_OK;
}

HRESULT CShellEmbedding::GetMoniker(
    DWORD dwAssign,
    DWORD dwWhichMoniker,
    IMoniker **ppmk)
{
    IOOMSG(TEXT("GetMoniker"));
    return E_NOTIMPL;
}

HRESULT CShellEmbedding::InitFromData(
    IDataObject *pDataObject,
    BOOL fCreation,
    DWORD dwReserved)
{
    IOOMSG(TEXT("InitFromData"));
    // LATER: We may want to implement this later.
    return E_FAIL;
}

HRESULT CShellEmbedding::GetClipboardData(
    DWORD dwReserved,
    IDataObject **ppDataObject)
{
    IOOMSG(TEXT("GetClipboardData"));
    return E_FAIL;
}

HRESULT CShellEmbedding::DoVerb(
    LONG iVerb,
    LPMSG lpmsg,
    IOleClientSite *pActiveSite,
    LONG lindex,
    HWND hwndParent,
    LPCRECT lprcPosRect)
{
    IOOMSG2(TEXT("DoVerb"), iVerb);
    HRESULT hres = S_OK;

    // If I don't have a client site, but I have one on "hold", I need to set it up again.
    //
    if (_pcli == NULL
        && _pcliHold)
    {
        IOleClientSite *pOleClientSite = _pcliHold;
        _pcliHold = NULL;
        SetClientSite(pOleClientSite);
        pOleClientSite->Release();
    }

    switch(iVerb)
    {
    case OLEIVERB_HIDE:
        hres = _DoActivateChange(NULL, OC_DEACTIVE, FALSE);
        break;

    case OLEIVERB_OPEN:
        hres = E_FAIL;
        break;

    case OLEIVERB_PRIMARY:
    case OLEIVERB_SHOW:
        if (_pipsite) {
            return S_OK;
        }
        // Fall through
    case OLEIVERB_UIACTIVATE:
        hres = _DoActivateChange(pActiveSite, OC_UIACTIVE, TRUE); //TRUE => We want to force UIACTIVE even if we are already active.
        break;

    case OLEIVERB_INPLACEACTIVATE:
        hres = _DoActivateChange(pActiveSite, OC_INPLACEACTIVE, FALSE);
        break;

    default:
        hres = E_FAIL; // OLEOBJ_S_INVALDVERB;
        break;
    }

    IOOMSGX(TEXT("DoVerb"), hres);
    return hres;
}

//
// fForce == TRUE indicates that we need to call _OnActivateChange even if we
// are already in OC_UIACITVE state.
//
HRESULT CShellEmbedding::_DoActivateChange(IOleClientSite* pActiveSite, UINT uState, BOOL fForce)
{
    if (uState == _nActivate)
    {
        // in general, we have nothing to do if we're already in
        // the correct state. HOWEVER, OLEIVERB_UIACTIVATE is supposed
        // to set focus if we (or our children?) don't currently have it.
        // Fall into _OnActivateChange so CWebBrowserOC can tell the
        // base browser to go uiactive.
        //
        if ((uState != OC_UIACTIVE) || !fForce)
            return S_OK;
    }

    #define STATETOSTRING(n) (n==OC_DEACTIVE ? TEXT("OC_DEACTIVE") : (n==OC_INPLACEACTIVE ? TEXT("OC_INPLACEACTIVE") : (n== OC_UIACTIVE ? TEXT("OC_UIACTIVE") : TEXT("ERROR"))))
    TraceMsg(TF_SHDCONTROL, "she _DoActivateChange from %s to %s", STATETOSTRING(_nActivate), STATETOSTRING(uState));

    return _OnActivateChange(pActiveSite, uState);
}

HRESULT CShellEmbedding::_OnActivateChange(IOleClientSite* pActiveSite, UINT uState)
{
    if (uState != _nActivate)
    {
        // mark us in our new state immediately. this avoids recursion death with bad containers (ipstool)
        UINT uOldState = _nActivate;
        _nActivate = uState;
    
        if (uOldState == OC_DEACTIVE) // going from deactive to IP or UI active
        {
            if (pActiveSite==NULL)
            {
                _nActivate = uOldState;
                return E_INVALIDARG;
            }
    
            ASSERT(!_pipsite); // always true, so why check below?
            if (!_pipsite)
            {
                HRESULT hres = pActiveSite->QueryInterface(IID_IOleInPlaceSite, (LPVOID*)&_pipsite);
        
                if (FAILED(hres))
                {
                    _nActivate = uOldState;
                    return hres;
                }
                
                hres = _pipsite->CanInPlaceActivate();
                if (hres != S_OK) {
                    ATOMICRELEASE(_pipsite);
                    TraceMsg(TF_SHDCONTROL, "she - CanInPlaceActivate returned %x", hres);
                    _nActivate = uOldState;
                    return E_FAIL;
                }
        
                _OnInPlaceActivate(); // do it
            }
        }
        else if (uOldState == OC_UIACTIVE) // going from UIActive to IPActive or deactive
        {
            _OnUIDeactivate();
        }
    
        if (uState == OC_UIACTIVE) // going to UIActive
        {
            _OnUIActivate();
        }
        else if (uState == OC_DEACTIVE) // going to Deactive
        {
            // We fail creation (OLEIVERB_PRIMARY, OLEIVERB_SHOW,
            // OLEIVERB_UIACTIVATE, or OLEIVERB_INPLACEACTIVATE) if we don't
            // get a pipsite, so we should never hit this case.
            ASSERT(_pipsite);
            // OLEIVERB_HIDE should ... return it to the visual state just after
            // initial creation or reloading, before OLEIVERB_SHOW or OLEIVERB_OPEN
            // is sent. Which is what _InPlaceDeactivate does. What's the point of this?
            // htmlobj calls OLEIVERB_HIDE and then ::InPlaceDeactivate
            _OnInPlaceDeactivate();
        }
    }

    return S_OK;
}

// move from de-active to in-place-active
void CShellEmbedding::_OnInPlaceActivate()
{
    //
    // Set the appropriate parent window.
    //
    _CreateWindowOrSetParent(_pipsite);

    _pipsite->OnInPlaceActivate();
    ASSERT(_pipframe == NULL);
    ASSERT(_pipui == NULL);
    _finfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    _pipsite->GetWindowContext(&_pipframe, &_pipui,
                               &_rcPos, &_rcClip, &_finfo);

    TraceMsg(TF_SHDCONTROL, "she::_OnInPlaceActivate x=%d y=%d cx=%d cy=%d (_cx=%d _cy=%d)", _rcPos.left, _rcPos.top, _rcPos.right-_rcPos.left, _rcPos.bottom-_rcPos.top, _size.cx, _size.cy);
    SetWindowPos(_hwnd, 0,
                 _rcPos.left, _rcPos.top,
                 _rcPos.right-_rcPos.left,
                 _rcPos.bottom-_rcPos.top,
                 SWP_SHOWWINDOW | SWP_NOZORDER);

    _SendAdvise(OBJECTCODE_SHOWOBJECT); // just like OLE 2nd ed (p.1074)
}

// Move from in-place-active to de-active
void CShellEmbedding::_OnInPlaceDeactivate(void)
{
    if (_hwnd) {
        ShowWindow(_hwnd, SW_HIDE);

        // re-parent our _hwnd... when we're not active we can't rely on
        // what our parent window is doing. The container can even destroy it!
        //
        // BUGBUG: the standard thing to do here is DESTROY our HWND and
        // recreate it if/when we are reactivated. This may break our hosted
        // IShellView and draw code. Investigate this.
        //

        // BUGBUG: this has been taken out by CDturner, MikeSH assures me we don't need it, and 
        // BUGBUG: this is causing our app to lose activation and regain it which causes the 
        // BUGBUG: palette to flash on 256 colour machines...y
        // SetParentHwnd(_hwnd, NULL);
    }

    if (_pipsite) {
        _pipsite->OnInPlaceDeactivate();
        ATOMICRELEASE(_pipsite);
    }

    ATOMICRELEASE(_pipframe);
    ATOMICRELEASE(_pipui);

    //
    // We need to tell the container to update the cached metafile, if any.
    //
    _SendAdvise(OBJECTCODE_DATACHANGED);

}

// move from in-place-active to ui-active
void CShellEmbedding::_OnUIActivate(void)
{
    if (_pipsite) {
        _pipsite->OnUIActivate();
    }

    //
    // HACK: When we are in Excel, _pipui->SetActiveObject sets the focus
    //  to us (for some unknown reason -- trying to be nice?). Since _hwnd
    //  simply forward the focus to the _hwndChild, setting focus to _hwnd
    //  twice causes this:
    //
    //   1. SetFocus(_hwnd)             by us (if we call SetFocus(_hwnd))
    //   2. SetFocus(_hwndChild)        in _hwnd's wndproc
    //   3. SetFocus(_hwnd)             by Excel
    //   4. SetFocus(_hwndChild)        in _hwnd's wndproc
    //
    //   If _hwndChild is a control, it notifies to the parent that it
    //  lost the focus. Then, we think "oh, we lost the focus. We should
    //  deactivate this object". To avoid it, we don't call SetFocus before
    //  we call _pipui->SetActiveObject and do some tricky thing below.
    //
    // SetFocus(_hwnd);

    //
    // RDuke suggest us to change the second parameter to NULL (instead of
    // "FOO" in IE3, but we don't know the side effect of it. I'm changing
    // it to "item" for IE4. (SatoNa)
    //
    if (_pipframe) {
        _pipframe->SetActiveObject(SAFECAST(this, IOleInPlaceActiveObject*), L"item");
    }

    if (_pipui) {
        _pipui->SetActiveObject(SAFECAST(this, IOleInPlaceActiveObject*), L"item");
    }

    //
    // We don't have any menu, so tell the container to use its own menu.
    //
    if (_pipframe) {
        _pipframe->SetMenu(NULL, NULL, _hwnd);
    }

    // Find-out if one of our child window has the input focus.
    for (HWND hwndFocus = GetFocus();
         hwndFocus && hwndFocus!=_hwnd;
         hwndFocus = GetParent(hwndFocus))
    {}

    // If not, set it.
    if (hwndFocus==NULL) {
         SetFocus(_hwnd);
    }

    // If this UIActivate came from below (i.e., our hosted DocObject), then we need to inform
    // our container.  We do this by calling IOleControlSite::OnFocus.  VB5 and Visual FoxPro
    // (at least) rely on this call being made for proper focus handling.
    //
    IUnknown_OnFocusOCS(_pcli, TRUE);
}

void CShellEmbedding::_OnUIDeactivate(void)
{
    //
    // We don't have any shared menu or tools to clean up.
    //

    if (_pipframe) {
        _pipframe->SetActiveObject(NULL, NULL);
    }

    if (_pipui) {
        _pipui->SetActiveObject(NULL, NULL);
    }

    if (_pipsite) {
        _pipsite->OnUIDeactivate(FALSE);
    }
    // If this UIDeactivate came from below (i.e., our hosted DocObject), then we need to inform
    // our container.  We do this by calling IOleControlSite::OnFocus.  VB5 and Visual FoxPro
    // (at least) rely on this call being made for proper focus handling.
    //
    IUnknown_OnFocusOCS(_pcli, FALSE);
}



HRESULT CShellEmbedding::EnumVerbs(
    IEnumOLEVERB **ppEnumOleVerb)
{
    IOOMSG(TEXT("EnumVerbs"));
    *ppEnumOleVerb = new CSVVerb(_pverbs);
    return *ppEnumOleVerb ? S_OK : E_OUTOFMEMORY;
}

HRESULT CShellEmbedding::Update( void)
{
    IOOMSG(TEXT("Update"));
    // Always up-to-date

    return S_OK;
}

HRESULT CShellEmbedding::IsUpToDate( void)
{
    IOOMSG(TEXT("IsUpToDate"));
    // Always up-to-date
    return S_OK;
}

HRESULT CShellEmbedding::GetUserClassID(CLSID *pClsid)
{
    IOOMSG(TEXT("GetUserClassID"));
    return GetClassID(pClsid);
}

HRESULT CShellEmbedding::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    return OleRegGetUserType(CLSIDOFOBJECT(this), dwFormOfType, pszUserType);
}

HRESULT CShellEmbedding::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    // SetExtent sets the LOGICAL size of an object. SetObjectRects determins
    // the size of the object on the screen. If we cared about zooming, we'd
    // keep track of this and do some sort of scaling. But we don't.
    // We still need to remember this value so we return it on GetExtent.
    //
    _sizeHIM = *psizel;

    // HOWEVER, IE3 shipped a SetExtent that changed the physical size of the
    // object. For compat (AOL uses SetExtent to change the size), if we're the
    // old WebBrowser, continue to resize.
    //
    if (_pObjectInfo->pclsid == &CLSID_WebBrowser_V1)
    {
        RECT rc;
        HDC   hdc;
        int   mmOld;
        POINT pt;

        // Make sure a container doesn't do anything stupid like
        // make us negative size
        //
        // BUGBUG: this breaks Trident because it sizes us negative
        // and we fail that sizing and they get confused...
        //
        //ASSERT(psizel->cx >= 0 && psizel->cy <= 0);
        //if (psizel->cx < 0 || psizel->cy > 0)
        //    return E_FAIL;
    
        // We only support DVASPECT_CONTENT
        if (dwDrawAspect != DVASPECT_CONTENT)
            return E_NOTIMPL;
    
        // Map this to a SetObjectRects call -- that way superclasses
        // only have to watch one function for size changes
        //

        int nScaleFactorX = 1, nScaleFactorY = 1;

        pt.x = psizel->cx;
        pt.y = psizel->cy;

        hdc = GetDC(NULL);
        mmOld = SetMapMode(hdc, MM_HIMETRIC);

        if (!g_fRunningOnNT)  // if running on Win95
        {
            // Win95 doesn't like coordinates over 32K
            const int SHRT_MIN = -32768, SHRT_MAX = 32767;

            while (pt.x > SHRT_MAX || pt.x < SHRT_MIN)
            {
                pt.x >>= 1;
                nScaleFactorX <<= 1;
            }
            while (pt.y > SHRT_MAX || pt.y < SHRT_MIN)
            {
                pt.y >>= 1;
                nScaleFactorY <<= 1;
            }
        }

        LPtoDP(hdc, &pt, 1);

        if (!g_fRunningOnNT)
        {
            pt.x *= nScaleFactorX;
            pt.y *= nScaleFactorY;
        }

        pt.y = -pt.y;
        SetMapMode(hdc, mmOld);
        ReleaseDC(NULL, hdc);
    
        rc.left = _rcPos.left;
        rc.right = rc.left + pt.x;
        rc.top = _rcPos.top;
        rc.bottom = rc.top + pt.y;
    
        // Assume that using SetExtent adjusts both the pos and clip rects
        return SetObjectRects(&rc, NULL);
    }
    else
    {
        return S_OK;
    }
}

HRESULT CShellEmbedding::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    *psizel = _sizeHIM;
    return S_OK;
}

HRESULT CShellEmbedding::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    IOOMSG2(TEXT("Advise"), pAdvSink);
    HRESULT hr = E_INVALIDARG;

    if (!pdwConnection)
        return hr;

    *pdwConnection = NULL;              // set out params to NULL

    if (!_poah)
        hr = ::CreateOleAdviseHolder(&_poah);
    else
        hr = NOERROR;

    if( SUCCEEDED(hr) )
        hr = _poah->Advise(pAdvSink, pdwConnection);

    return(hr);
}

HRESULT CShellEmbedding::Unadvise(DWORD dwConnection)
{
    IOOMSG(TEXT("Unadvise"));
    HRESULT     hr;

    if (!_poah)
        return(OLE_E_NOCONNECTION);

    hr = _poah->Unadvise(dwConnection);

    return(hr);
}

HRESULT CShellEmbedding::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
{
    IOOMSG(TEXT("EnumAdvise"));
    HRESULT     hr;

    if (!ppenumAdvise)
        return(E_INVALIDARG);

    if (!_poah)
    {
        *ppenumAdvise = NULL;
        hr = S_OK;
    }
    else
    {
        hr = _poah->EnumAdvise(ppenumAdvise);
    }

    return(hr);
}

HRESULT CShellEmbedding::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    IOOMSG(TEXT("GetMiscStatus"));

    *pdwStatus = OLEMISCFLAGSOFCONTROL(this);

    return S_OK;
}

HRESULT CShellEmbedding::SetColorScheme(LOGPALETTE *pLogpal)
{
    IOOMSG(TEXT("GetColorScheme"));
    return S_OK;
}

//
//  Helper function to create an HDC from an OLE DVTARGETDEVICE structure.
//  Very useful for metafile drawing, where the Metafile DC will be the 
//  actual "draw to" dc, and the TargetDC, if present, will describe the ultimate output device.
//
HDC CShellEmbedding::_OleStdCreateDC(DVTARGETDEVICE *ptd)
{
    HDC        hdc = NULL;
    LPDEVNAMES lpDevNames = NULL;
    LPDEVMODEA lpDevMode = NULL;
    LPSTR      lpszDriverName = NULL;
    LPSTR      lpszDeviceName = NULL;
    LPSTR      lpszPortName = NULL;

    if (ptd)
    {
        lpDevNames = (LPDEVNAMES) ptd;
        if (ptd->tdExtDevmodeOffset)
        {
            lpDevMode = (LPDEVMODEA) ( (LPSTR) ptd + ptd->tdExtDevmodeOffset);
        }

        lpszDriverName = (LPSTR) lpDevNames + ptd->tdDriverNameOffset;
        lpszDeviceName = (LPSTR) lpDevNames + ptd->tdDeviceNameOffset;
        lpszPortName   = (LPSTR) lpDevNames + ptd->tdPortNameOffset;

        hdc = CreateDCA(lpszDriverName, lpszDeviceName, lpszPortName, lpDevMode);
    }
    return hdc;
}

// *** IDataObject ***
//
// WARNING:
//   It is well-known fact that Word and Excel (in Office95) does not call
//  IViewObject::Draw to draw embedding. Instead, they GetData(CF_METAFILEPICT).
//  If we don't offer it, Word will fail to embed it and Excel will draw
//  white rectangle when our object is deactivated. To be embedded correctly
//  on those apps, we must support CF_METAFILEPICT. (SatoNa)
//
HRESULT CShellEmbedding::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    IDTMSG4(TEXT("GetData"), pformatetcIn->cfFormat, pformatetcIn->tymed);
    HRESULT hres = DV_E_FORMATETC;
    HDC hdcTargetDevice = NULL;
    HENHMETAFILE hemf = NULL;

    // If a Target device is specified in the FORMATETC structure, create a DC for it.
    // This gets passed to CreateEnhMetaFile and IViewObject::Draw.
    //
    if (pformatetcIn->ptd) 
    {
        hdcTargetDevice = _OleStdCreateDC(pformatetcIn->ptd);
        if (!hdcTargetDevice)
        {
            return E_FAIL;
        }
    }

    // Enhanced metafiles need special processing.
    //
    if (pformatetcIn->cfFormat == CF_ENHMETAFILE
        && (pformatetcIn->tymed & TYMED_ENHMF))
    {
        if (_hwnd)
        {
            RECTL rectBounds = { 0, 0, _sizeHIM.cx, _sizeHIM.cy };

            //
            // Call the "A" version since we're not passing in strings and
            // this needs to work on W95.
            HDC hdc = CreateEnhMetaFileA(hdcTargetDevice, NULL, (RECT*)&rectBounds, NULL);
            IDTMSG3(TEXT("_EnhMetafileFromWindow CreateEnhMetaFile returned"), hdc);
            if (hdc)
            {
                SetMapMode(hdc, MM_HIMETRIC);
                rectBounds.bottom = -rectBounds.bottom;

                Draw(DVASPECT_CONTENT, LINDEX_INTERNAL, NULL, pformatetcIn->ptd,
                     hdcTargetDevice, hdc, &rectBounds, NULL, NULL, 0);

                hemf = CloseEnhMetaFile(hdc);
                IDTMSG3(TEXT("_EnhMetafileFromWindow CloseEnhMetaFile returned"), hemf);
            }
        }

        pmedium->hEnhMetaFile = hemf;
        if (pmedium->hEnhMetaFile) 
        {
            pmedium->tymed = TYMED_ENHMF;
            pmedium->pUnkForRelease = NULL;
            hres = S_OK;
        } 
        else 
        {
            hres = E_FAIL;
        }
    }

    // Create a standard metafile
    //
    else if (pformatetcIn->cfFormat == CF_METAFILEPICT
        && (pformatetcIn->tymed & TYMED_MFPICT))
    {
        hres = E_OUTOFMEMORY;
        HGLOBAL hmem = GlobalAlloc(GPTR, sizeof(METAFILEPICT));
        if (hmem)
        {
            LPMETAFILEPICT pmf = (LPMETAFILEPICT) hmem;
            RECTL rectBounds = { 0, 0, _sizeHIM.cx, _sizeHIM.cy };

            HDC hdc = CreateMetaFile(NULL);
            if (hdc)
            {
                SetMapMode(hdc, MM_HIMETRIC);
                rectBounds.bottom = -rectBounds.bottom;

                SetWindowOrgEx(hdc, 0, 0, NULL);
                SetWindowExtEx(hdc, _sizeHIM.cx, _sizeHIM.cy, NULL);

                Draw(DVASPECT_CONTENT, LINDEX_INTERNAL, NULL, 
                    pformatetcIn->ptd, hdcTargetDevice,
                    hdc, &rectBounds, &rectBounds, NULL, 0);

                pmf->hMF = CloseMetaFile(hdc);

                if (pmf->hMF)
                {
                    pmf->mm = MM_ANISOTROPIC;
                    pmf->xExt = _sizeHIM.cx;
                    pmf->yExt = _sizeHIM.cy;
                    TraceMsg(TF_SHDCONTROL, "sdv TR ::GetData (%d,%d)-(%d,%d)",
                             _size.cx, _size.cy, _sizeHIM.cx, _sizeHIM.cy);

                    pmedium->tymed = TYMED_MFPICT;
                    pmedium->hMetaFilePict = hmem;
                    pmedium->pUnkForRelease = NULL;
                    hres = S_OK;
                }
            }

            if (FAILED(hres))
            {
                GlobalFree(hmem);
            }
        }
    }

    return hres;
}

HRESULT CShellEmbedding::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    IDTMSG2(TEXT("GetDataHere"), pformatetc->cfFormat);
    return E_NOTIMPL;
}

HRESULT CShellEmbedding::QueryGetData(FORMATETC *pformatetc)
{
    IDTMSG2(TEXT("QueryGetData"), pformatetc->cfFormat);
    HRESULT hres = S_FALSE;
    if (pformatetc->cfFormat == CF_ENHMETAFILE
        && (pformatetc->tymed & TYMED_ENHMF))
    {
        hres = S_OK;
    }
    else if (pformatetc->cfFormat == CF_METAFILEPICT
        && (pformatetc->tymed & TYMED_MFPICT))
    {
        hres = S_OK;
    }

    return hres;
}

HRESULT CShellEmbedding::GetCanonicalFormatEtc(FORMATETC *pformatetcIn, FORMATETC *pformatetcOut)
{
    IDTMSG2(TEXT("GetCanonicalFormatEtc"), pformatetcIn->cfFormat);
    *pformatetcOut = *pformatetcIn;
    return DATA_S_SAMEFORMATETC;
}

HRESULT CShellEmbedding::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    IDTMSG(TEXT("SetData"));
    return E_NOTIMPL;
}

HRESULT CShellEmbedding::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    IDTMSG(TEXT("EnumFormatEtc"));
    return E_NOTIMPL;
}

HRESULT CShellEmbedding::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    IDTMSG(TEXT("DAdvise"));
    HRESULT hr = E_INVALIDARG;

    if (!pdwConnection)
        return hr;

    *pdwConnection = NULL;              // set out params to NULL

    if (!_pdah)
        hr = ::CreateDataAdviseHolder(&_pdah);
    else
        hr = NOERROR;

    if( SUCCEEDED(hr) )
        hr = _pdah->Advise(this, pformatetc, advf, pAdvSink, pdwConnection);

    return(hr);
}

HRESULT CShellEmbedding::DUnadvise(DWORD dwConnection)
{
    IDTMSG(TEXT("DUnadvise"));
    HRESULT     hr;

    if (!_pdah)
        return(OLE_E_NOCONNECTION);

    hr = _pdah->Unadvise(dwConnection);

    return(hr);
}

HRESULT CShellEmbedding::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    IDTMSG(TEXT("EnumDAdvise"));
    HRESULT     hr;

    if (!ppenumAdvise)
        return(E_INVALIDARG);

    if (!_pdah)
    {
        *ppenumAdvise = NULL;
        hr = S_OK;
    }
    else
    {
        hr = _pdah->EnumAdvise(ppenumAdvise);
    }

    return(hr);
}

// *** IOleWindow ***
HRESULT CShellEmbedding::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;
    return S_OK;
}

HRESULT CShellEmbedding::ContextSensitiveHelp(BOOL fEnterMode)
{
    return S_OK;
}


// *** IOleInPlaceObject ***
HRESULT CShellEmbedding::InPlaceDeactivate(void)
{
    IIPMSG(TEXT("InPlaceDeactivate"));
    return _DoActivateChange(NULL, OC_DEACTIVE, FALSE);
}

HRESULT CShellEmbedding::UIDeactivate(void)
{
    IIPMSG(TEXT("UIDeactivate"));
    return _DoActivateChange(NULL, OC_INPLACEACTIVE, FALSE);
}

HRESULT CShellEmbedding::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    RECT  rcVisible;

    _rcPos = *lprcPosRect;

    if (lprcClipRect)
    {
        _rcClip = *lprcClipRect;
    }
    else
    {
        _rcClip = _rcPos;
    }

    IntersectRect(&rcVisible, &_rcPos, &_rcClip);    
    if (EqualRect(&rcVisible, &_rcPos))
    {
        if (_fUsingWindowRgn)
        {
            SetWindowRgn(_hwnd, NULL, TRUE);
            _fUsingWindowRgn = FALSE;
        }
    }
    else 
    {
        _fUsingWindowRgn = TRUE;
        OffsetRect(&rcVisible, -_rcPos.left, -_rcPos.top);
        SetWindowRgn(_hwnd,
                CreateRectRgnIndirect(&rcVisible),
                TRUE);
    }

    // We should consider this equivalent to a SetExtent as well...
    // But only for valid sizes (html viewer gives us invalid
    // sizes during it's reformat routine). Note: we still need
    // the SetWindowPos because we may move off the window.
    int cx = _rcPos.right - _rcPos.left;
    int cy = _rcPos.bottom - _rcPos.top;
    TraceMsg(TF_SHDCONTROL, "she SetObjectRects to x=%d y=%d cx=%d cy=%d (from cx=%d cy=%d)", _rcPos.left, _rcPos.top, cx, cy, _size.cx, _size.cy);
    if (cx >= 0 && cy >= 0)
    {
        _size.cx = cx;
        _size.cy = cy;
    }

    if (_hwnd)
    {
        SetWindowPos(_hwnd, NULL,
                     _rcPos.left, _rcPos.top,
                     _size.cx,
                     _size.cy,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return S_OK;
}

HRESULT CShellEmbedding::ReactivateAndUndo(void)
{
    IIPMSG(TEXT("ReactivateAndUndo"));
    return INPLACE_E_NOTUNDOABLE;
}

// *** IOleInPlaceActiveObject ***
HRESULT CShellEmbedding::TranslateAccelerator(LPMSG lpmsg)
{
    extern BOOL IsVK_TABCycler(MSG * pMsg);
    HRESULT hr = S_FALSE;

    // IIAMSG(TEXT("TranslateAccelerator"));
    // We have no accelerators (other than TAB, which we must pass up
    // to IOCS::TA to move on to the next control if any)

    if (IsVK_TABCycler(lpmsg)) {
        // BUGBUG grfMods?
        hr = IUnknown_TranslateAcceleratorOCS(_pcli, lpmsg, /*grfMods*/ 0);
    }

    return hr;
}

HRESULT CShellEmbedding::OnFrameWindowActivate(BOOL fActivate)
{
    IIAMSG(TEXT("OnFrameWindowActivate"));

    if (fActivate)
    {
        // our frame has been activated and we are the active object
        // make sure we have focus
        SetFocus(_hwnd);
    }

    return S_OK;
}

HRESULT CShellEmbedding::OnDocWindowActivate(BOOL fActivate)
{
    IIAMSG(TEXT("OnDocWindowActivate"));
    // We don't care
    return S_OK;
}

HRESULT CShellEmbedding::ResizeBorder(LPCRECT prcBorder,
                    IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    IIAMSG(TEXT("ResizeBorder"));
    // We have no toolbars.
    return S_OK;
}

HRESULT CShellEmbedding::EnableModeless(BOOL fEnable)
{
    IIAMSG(TEXT("EnableModeless"));
    // We have no dialogs.
    return S_OK;
}

void CShellEmbedding::_RegisterWindowClass(void)
{
    WNDCLASS wc = {0};
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = s_WndProc ;
    //wc.cbClsExtra    = 0;
    wc.cbWndExtra    = SIZEOF(CShellEmbedding*) * 2;
    wc.hInstance     = g_hinst ;
    //wc.hIcon         = NULL ;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    //wc.lpszMenuName  = NULL ;
    wc.lpszClassName = c_szShellEmbedding;

    SHRegisterClass(&wc);
}


LRESULT CShellEmbedding::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_NCCREATE:
        DWORD dwExStyles;
        if ((dwExStyles = GetWindowLong(hwnd, GWL_EXSTYLE)) & RTL_MIRRORED_WINDOW)
        {
             SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyles &~ RTL_MIRRORED_WINDOW);
        }
        goto DoDefault;

    case WM_SETFOCUS:
        if (_hwndChild)
            SetFocus(_hwndChild);
        // If this SETFOCUS came from TABbing onto the control, VB5 expects us to call its
        // IOleControlSite::OnFocus.  Then it will UIActivate us.
        //
        IUnknown_OnFocusOCS(_pcli, TRUE);
        break;

    case WM_KILLFOCUS:
        // If this KILLFOCUS came from TABbing off the control, VB5 expects us to call its
        // IOleControlSite::OnFocus.  Then it will UIDeactivate us.
        //
        IUnknown_OnFocusOCS(_pcli, FALSE);
        break;

    case WM_WINDOWPOSCHANGED:
        if (_hwndChild)
        {
            LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;

            if (!(lpwp->flags & SWP_NOSIZE))
            {
                SetWindowPos(_hwndChild, NULL,
                    0, 0, lpwp->cx, lpwp->cy,
                    SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE|
                    (lpwp->flags&(SWP_NOREDRAW|SWP_NOCOPYBITS)));
            }
        }
        goto DoDefault;

#ifdef DEBUG
    // BUGBUG: we'll never get this with ShellExplorer OC, but if we did,
    // we'd need to call _DoActivateChange(OC_UIACTIVE, FALSE);
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        TraceMsg(TF_SHDCONTROL, "she ::v_WndProc(WM_xBUTTONDOWN) - we need to UIActivate");
        goto DoDefault;
#endif

    default:
DoDefault:
        return DefWindowProc(_hwnd, uMsg, wParam, lParam);
    }

    return 0L;
}

void CShellEmbedding::_ViewChange(DWORD dwAspect, LONG lindex)
{
    dwAspect &= _asp;

    if (dwAspect && _padv)
    {
        IAdviseSink *padv = _padv;
        IUnknown *punkRelease;

        if (_advf & ADVF_ONLYONCE)
        {
            _padv = NULL;
            punkRelease = padv;
        }
        else
            punkRelease = NULL;

        padv->OnViewChange(dwAspect, lindex);

        if (punkRelease)
            punkRelease->Release();
    }
}

void CShellEmbedding::_SendAdvise(UINT uCode)
{
    DWORD       dwAspect=DVASPECT_CONTENT | DVASPECT_THUMBNAIL;

    switch (uCode)
    {
    case OBJECTCODE_SAVED:
        if (NULL!=_poah)
            _poah->SendOnSave();
        break;

    case OBJECTCODE_CLOSED:
        if (NULL!=_poah)
            _poah->SendOnClose();
        break;

    case OBJECTCODE_RENAMED:
        //Call IOleAdviseHolder::SendOnRename (later)
        break;

    case OBJECTCODE_SAVEOBJECT:
        if (_fDirty && NULL!=_pcli)
            _pcli->SaveObject();

        _fDirty=FALSE;
        break;

    case OBJECTCODE_DATACHANGED:
        // _fDirty=TRUE;

        //No flags are necessary here.
        if (NULL!=_pdah)
            _pdah->SendOnDataChange(this, 0, 0);
        //
        // fall through
        //
    case OBJECTCODE_VIEWCHANGED:
        _ViewChange(dwAspect, -1);
        break;

    case OBJECTCODE_SHOWOBJECT:
        if (NULL!=_pcli)
            _pcli->ShowObject();
        break;
    }
}

HRESULT CSVVerb::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IEnumOLEVERB) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IEnumOLEVERB*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    _cRef++;
    return S_OK;
}

ULONG CSVVerb::AddRef()
{
    return ++_cRef;
}

ULONG CSVVerb::Release()
{
    if (--_cRef > 0) {
        return _cRef;
    }

    delete this;
    return 0;
}

HRESULT CSVVerb::Next(
    /* [in] */ ULONG celt,
    /* [out] */ LPOLEVERB rgelt,
    /* [out] */ ULONG *pceltFetched)
{
    HRESULT hres = S_FALSE;
    ULONG celtFetched = 0;


    // We need to enumerate the predefined verbs we support,
    // or some containers will never call them. This list
    // of verbs comes from our ::DoVerb function
    //
    static const OLEVERB rgVerbs[5] =
    {
        {OLEIVERB_PRIMARY, NULL, 0, 0},
        {OLEIVERB_INPLACEACTIVATE, NULL, 0, 0},
        {OLEIVERB_UIACTIVATE, NULL, 0, 0},
        {OLEIVERB_SHOW, NULL, 0, 0},
        {OLEIVERB_HIDE, NULL, 0, 0}
    };
    if (_iCur < ARRAYSIZE(rgVerbs))
    {
        IEVMSG(TEXT("Next"), celt, _iCur, TEXT("OLEIVERB_..."));

        *rgelt = rgVerbs[_iCur++];
        hres = S_OK;
    }
    else if (_pverbs)
    {
        int iCur = _iCur - ARRAYSIZE(rgVerbs);

        IEVMSG(TEXT("Next"), celt, _iCur, _pverbs[iCur].lpszVerbName);

        //
        // BUGBUG: Should we do while(celt--)?
        //
        if (_pverbs[iCur].lpszVerbName)
        {
            *rgelt = _pverbs[_iCur++];
            WCHAR* pwszVerb = (WCHAR *)CoTaskMemAlloc(128 * sizeof(WCHAR));
            if (pwszVerb)
            {
                MLLoadStringW(PtrToUint(_pverbs[iCur].lpszVerbName), pwszVerb, 128);
                rgelt->lpszVerbName = pwszVerb;
                celtFetched++;
                hres = S_OK;
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    if (pceltFetched) {
        *pceltFetched = celtFetched;
    }
    return hres;
}

HRESULT CSVVerb::Skip(ULONG celt)
{
    return S_OK;
}

HRESULT CSVVerb::Reset( void)
{
    _iCur = 0;
    return S_OK;
}

HRESULT CSVVerb::Clone(IEnumOLEVERB **ppenum)
{
    *ppenum = new CSVVerb(_pverbs);
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}
