#include "priv.h"
#include "privcpp.h"

const DWORD g_cookie = 111176;

CPackage_IViewObject2::CPackage_IViewObject2(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0);
    ASSERT(_fFrozen == FALSE);
}

CPackage_IViewObject2::~CPackage_IViewObject2()
{
    DebugMsg(DM_TRACE, "CPackage_IViewObject2 destroyed with ref count %d",_cRef);
}

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IViewObject2::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);
}

ULONG CPackage_IViewObject2::AddRef(void) 
{
    _cRef++;    // interface ref count for debugging
    return _pPackage->AddRef();
}

ULONG CPackage_IViewObject2::Release(void)
{
    _cRef--;    // interface ref count for debugging
    return _pPackage->Release();
}


//////////////////////////////////
//
// IViewObject2 Methods...
//
HRESULT CPackage_IViewObject2::Draw(DWORD dwDrawAspect, LONG lindex,
LPVOID pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
LPCRECTL lprcBounds, LPCRECTL lprcWBounds,BOOL (CALLBACK *pfnContinue)(DWORD),
DWORD dwContinue)
{
    DebugMsg(DM_TRACE,"pack vo - Draw() called.");
    
    //
    // NOTE: If we're frozen, we should use a cached represetation, but
    // the icon doesn't really change all that often, so it's kind of 
    // pointless to freeze it, but here's where we'd do it, if we 
    // wanted to.  About the only place this would ever be necessary if 
    // somebody called to freeze us while the user was in the process of 
    // editing the package or something, but I don't think it's something
    // we need to worry about right away.  (Especially since the user can't
    // change the icon right now.)
    //

    IconDraw(_pPackage->_lpic, hdcDraw, (RECT *)lprcBounds);
    return S_OK;
}

    
HRESULT CPackage_IViewObject2::GetColorSet(DWORD dwAspect, LONG lindex, 
                                LPVOID pvAspect, DVTARGETDEVICE *ptd, 
                                HDC hdcTargetDev, LPLOGPALETTE *ppColorSet)
{
    DebugMsg(DM_TRACE,"pack vo - GetColorSet() called.");
    
    if (ppColorSet == NULL)
        return E_INVALIDARG;
    
    *ppColorSet = NULL;         // null the out param
    return S_FALSE;
}

    
HRESULT CPackage_IViewObject2::Freeze(DWORD dwDrawAspect, LONG lindex, 
                                      LPVOID pvAspect, LPDWORD pdwFreeze)
{
    DebugMsg(DM_TRACE,"pack vo - Freeze() called.");

    if (pdwFreeze == NULL)
        return E_INVALIDARG;
    
    if (_fFrozen) {
        *pdwFreeze = g_cookie;
        return S_OK;
    }
    
    //
    // This is where we would take a snapshot of the icon to use as
    // the "frozen" image in subsequent routines.  For now, we just
    // return the cookie.  Draw() will use the current icon regardless 
    // of the fFrozen flag.
    //
    
    _fFrozen = TRUE;
    *pdwFreeze = g_cookie;
    
    return S_OK;
}

    
HRESULT CPackage_IViewObject2::Unfreeze(DWORD dwFreeze)
{
    DebugMsg(DM_TRACE,"pack vo - Unfreeze() called.");
    
    // If the pass us an invalid cookie or we're not frozen then bail
    if (dwFreeze != g_cookie || !_fFrozen)
        return OLE_E_NOCONNECTION;
    
    // 
    // This is where we'd get rid of the frozen presentation we saved in
    // IViewObject::Freeze().
    //
    _fFrozen = FALSE;
    return S_OK;
}

    
HRESULT CPackage_IViewObject2::SetAdvise(DWORD dwAspects, DWORD dwAdvf,
                              LPADVISESINK pAdvSink)
{
    DebugMsg(DM_TRACE,"pack vo - SetAdvise() called.");
    
    if (_pPackage->_pViewSink)
        _pPackage->_pViewSink->Release();
    
    _pPackage->_pViewSink = pAdvSink;
    _pPackage->_dwViewAspects = dwAspects;
    _pPackage->_dwViewAdvf = dwAdvf;
    
    if (_pPackage->_pViewSink) 
        _pPackage->_pViewSink->AddRef();
    
    return S_OK;
}

    
HRESULT CPackage_IViewObject2::GetAdvise(LPDWORD pdwAspects, LPDWORD pdwAdvf,
                              LPADVISESINK *ppAdvSink)
{
    DebugMsg(DM_TRACE,"pack vo - GetAdvise() called.");
    
    if (!ppAdvSink || !pdwAdvf || !pdwAspects)
        return E_INVALIDARG;
    
    *ppAdvSink = _pPackage->_pViewSink;
    _pPackage->_pViewSink->AddRef();
    
    if (pdwAspects != NULL)
        *pdwAspects = _pPackage->_dwViewAspects;
    
    if (pdwAdvf != NULL)
        *pdwAdvf = _pPackage->_dwViewAdvf;
    
    return S_OK;
}

    
HRESULT CPackage_IViewObject2::GetExtent(DWORD dwAspect, LONG lindex,
DVTARGETDEVICE *ptd, LPSIZEL pszl)
{
    DebugMsg(DM_TRACE,"pack vo - GetExtent() called.");

    if (pszl == NULL)
        return E_INVALIDARG;
    
    if (!_pPackage->_lpic)
        return OLE_E_BLANK;
            
    pszl->cx = _pPackage->_lpic->rc.right;
    pszl->cy = _pPackage->_lpic->rc.bottom;
    
    pszl->cx = MulDiv(pszl->cx,HIMETRIC_PER_INCH,DEF_LOGPIXELSX);
    pszl->cy = MulDiv(pszl->cy,HIMETRIC_PER_INCH,DEF_LOGPIXELSY);
    
    return S_OK;
}
