#include "priv.h"

#include "sccls.h"
#include "comcat.h"
#include "dvocx.h"
#define _SFVIEWP_H_ // we just want the GUIDs
#include "../shell32/sfviewp.h" // this dll should really be rolled into shell32
#include <hliface.h>
#include "shlguid.h"
#include "shvocx.h"
#include "winlist.h"
#include <mshtml.h>
#include "stdenum.h"
#include "iface.h"
#include "resource.h"

#include <mluisupp.h>

#define SUPERCLASS CShellOcx

#define MIN_HEIGHT 80   // minimum height of a ShellFolderViewOC
#define MIN_WIDTH  80   // minimum width of a ShellFolderViewOC.
#define DEF_WIDTH  300  // default width when we cannot get sizing information
#define DEF_HEIGHT 150  // default height when we cannot get sizing information

#define IPSMSG(psz)             TraceMsg(TF_SHDCONTROL, "shv IPS::%s called", (psz))
#define IPSMSG2(psz, hres)      TraceMsg(TF_SHDCONTROL, "shv IPS::%s %x", (psz), hres)
#define IPSMSG3(psz, hres, x, y) TraceMsg(TF_SHDCONTROL,"shv IPS::%s %x %d (%d)", (psz), hres, x, y)
#define IOOMSG(psz)             TraceMsg(TF_SHDCONTROL, "shv IOO::%s called", (psz))
#define IOOMSG2(psz, i)         TraceMsg(TF_SHDCONTROL, "shv IOO::%s called with (%d)", (psz), i)
#define IOOMSG3(psz, i, j)      TraceMsg(TF_SHDCONTROL, "shv IOO::%s called with (%d, %d)", (psz), i, j)
#define IVOMSG(psz)             TraceMsg(TF_SHDCONTROL, "shv IVO::%s called", (psz))
#define IVOMSG2(psz, i)         TraceMsg(TF_SHDCONTROL, "shv IVO::%s called with (%d)", (psz), i)
#define IVOMSG3(psz, i, j)      TraceMsg(TF_SHDCONTROL, "shv IVO::%s called with (%d, %d)", (psz), i, j)
#define PROPMSG(psz)            TraceMsg(TF_SHDCONTROL, "shv %s", (psz))
#define PROPMSG2(psz, pstr)     TraceMsg(TF_SHDCONTROL, "shv %s with [%s]", (psz), pstr)
#define PROPMSG3(psz, hex)      TraceMsg(TF_SHDCONTROL, "shv %s with 0x%x", (psz), hex)
#define PRIVMSG(psz)            TraceMsg(TF_SHDCONTROL, "shv %s", (psz))
#define CVOCBMSG(psz)           TraceMsg(TF_SHDCONTROL, "shv CWebBrowserSB::%s", (psz))
#define IOIPAMSG(psz)           TraceMsg(TF_SHDCONTROL, "shv IOIPA::%s", (psz));

#define ABS(i)  (((i) < 0) ? -(i) : (i))

// sizing messages are annoying, but occasionally useful:
#define TF_SIZEMSG 0 // set to zero if not wanted, TF_SHDCONTROLif wanted
#define DM_FORSEARCHBAND    0

static const OLEVERB c_averbsSV[] = {
        { 0, (LPWSTR)MAKEINTRESOURCE(IDS_VERB_EDIT), 0, OLEVERBATTRIB_ONCONTAINERMENU },
        { 0, NULL, 0, 0 }
    };
static const OLEVERB c_averbsDesignSV[] = {
        { 0, NULL, 0, 0 }
    };

#define HMODULE_NOTLOADED   ((HMODULE)-1)

/*
 * CMessageFilter - implementation of IMessageFilter
 *
 * Used to reject RPC-reentrant calls while inside AOL
 *
 */
class CMessageFilter : public IMessageFilter {
public:
    // *** IUnknown methods ***
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj)
    {
        // This interface doesn't get QI'ed.
        ASSERT(FALSE);
        return E_NOINTERFACE;
    };
    virtual ULONG __stdcall AddRef(void)    {   return ++_cRef; };
    virtual ULONG __stdcall Release(void)   {   ASSERT(_cRef > 0);
                                                _cRef--;
                                                if (_cRef > 0)
                                                    return _cRef;

                                                delete this;
                                                return 0;
                                            };

    // *** IMessageFilter specific methods ***
    virtual DWORD __stdcall HandleInComingCall(
        IN DWORD dwCallType,
        IN HTASK htaskCaller,
        IN DWORD dwTickCount,
        IN LPINTERFACEINFO lpInterfaceInfo)
    {
#ifdef DEBUG
        WCHAR   wszIID[39];

        StringFromGUID2( lpInterfaceInfo->iid, wszIID, ARRAYSIZE(wszIID) );

        TraceMsg(TF_SHDCONTROL, "shvMF HandleIncomingCall: calltype=%lx, pUnk=%lx, IID=%ws, wMethod=%hu",
                dwCallType,
                lpInterfaceInfo->pUnk,
                wszIID,
                lpInterfaceInfo->wMethod);
#endif

        //
        //  The following statement guards against RPC-reentrancy by checking if
        //  the calltype is TOPLEVEL_CALLPENDING, which means that a call has arrived with a new
        //  logical threadid and that the object is currently waiting for a reply from a previous
        //  outgoing call.  It's this type of call that has proven troublesome in the past with AOL.
        //
        //  2-Dec-97: AOL QFE: We need to allow resizing requests to pass through the message filter.
        //            These appear as IOleObject::SetExtent.  Check the IID for IOleObject, and the 
        //            wMethod for 17 (SetExtent is the 17th method in the vtable, Zero-based).
        //
        const int SetExtent = 17;

        if ( ( dwCallType == CALLTYPE_TOPLEVEL_CALLPENDING )
            && !(IsEqualIID(lpInterfaceInfo->iid, IID_IOleObject) && lpInterfaceInfo->wMethod == SetExtent) )
        {
#ifdef DEBUG
            TraceMsg(TF_SHDCONTROL, "shvMF rejected call: calltype=%lx, pUnk=%lx, IID=%ws, wMethod=%hu",
                dwCallType,
                lpInterfaceInfo->pUnk,
                wszIID,
                lpInterfaceInfo->wMethod);
#endif
            return SERVERCALL_RETRYLATER;
        }

        if (_lpMFOld)
        {
           HRESULT hr = _lpMFOld->HandleInComingCall(dwCallType, htaskCaller, dwTickCount, lpInterfaceInfo);
           TraceMsg(TF_SHDCONTROL, "shvMF HIC Previous MF returned %x", hr);
           return hr;
        }
        else
        {
            TraceMsg(TF_SHDCONTROL, "shvMF HIC returning SERVERCALL_ISHANDLED.");
            return SERVERCALL_ISHANDLED;
        }
    };

    virtual DWORD __stdcall RetryRejectedCall(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwRejectType)
    {
        TraceMsg(TF_SHDCONTROL, "shv MF RetryRejectedCall htaskCallee=%x, dwTickCount=%x, dwRejectType=%x",
            htaskCallee,
            dwTickCount,
            dwRejectType);

        if (_lpMFOld)
        {
            HRESULT hr = _lpMFOld->RetryRejectedCall(htaskCallee, dwTickCount, dwRejectType);
            TraceMsg(TF_SHDCONTROL, "shvMF RRC returned %x", hr);
            return hr;
        }
        else
        {
            TraceMsg(TF_SHDCONTROL, "shvMF RRC returning 0xffffffff");
            return 0xffffffff;
        }
    };

    virtual DWORD __stdcall MessagePending(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwPendingType)
    {
        TraceMsg(TF_SHDCONTROL, "shv MF MessagePending htaskCallee=%x, dwTickCount=%x, dwPendingType=%x",
            htaskCallee,
            dwTickCount,
            dwPendingType);

        if (_lpMFOld)
        {
            HRESULT hr = _lpMFOld->MessagePending(htaskCallee, dwTickCount, dwPendingType);
            TraceMsg(TF_SHDCONTROL, "shvMF RRC returned %x", hr);
            return hr;
        }
        else
        {
            TraceMsg(TF_SHDCONTROL, "shvMF MP returning PENDINGMSG_WAITDEFPROCESS");
            return PENDINGMSG_WAITDEFPROCESS;
        }
    };

    CMessageFilter() : _cRef(1)
    {
        ASSERT(_lpMFOld == NULL);
    };

    BOOL Initialize()
    {
        BOOL bResult = CoRegisterMessageFilter((LPMESSAGEFILTER)this, &_lpMFOld) != S_FALSE;
        TraceMsg(TF_SHDCONTROL, "shv Previous message filter is %lx", _lpMFOld);
        return bResult;
    };

    void UnInitialize()
    {
        TraceMsg(TF_SHDCONTROL, "shv MF Uninitializing, previous message filter = %x", _lpMFOld);
        CoRegisterMessageFilter(_lpMFOld, NULL);

        // we shouldn't ever get called again, but after 30 minutes
        // of automation driving we once hit a function call above
        // and we dereferenced this old pointer and page faulted.

        ATOMICRELEASE(_lpMFOld);
    };

protected:
    int _cRef;
    LPMESSAGEFILTER _lpMFOld;
};


CWebBrowserOC::CWebBrowserOC(IUnknown* punkOuter, LPCOBJECTINFO poi) :
    SUPERCLASS(punkOuter, poi, c_averbsSV, c_averbsDesignSV)
{
    TraceMsg(TF_SHDLIFE, "ctor CWebBrowserOC %x", this);

    // flag special so we only try to load browseui once
    _hBrowseUI = HMODULE_NOTLOADED;    
}

BOOL CWebBrowserOC::_InitializeOC(IUnknown *punkOuter)
{
    // we used a zero-init memory allocator, so everything else is NULL.
    // check to be sure:
    ASSERT(!_fInit);

    // By default, we're visible.  Everything else can default to FALSE.
    //
    _fVisible = 1;

    // CShellOcx holds the default event source which is DIID_DWebBrowserEvents2
    m_cpWB1Events.SetOwner(_GetInner(), &DIID_DWebBrowserEvents);

    // some stuff we want to set up now. we're a WebBrowser, so create the
    // IShellBrowser now. We need an aggregated automation object before
    // we do that.
    CIEFrameAuto_CreateInstance(SAFECAST(this, IOleControl*), &_pauto);
    if (_pauto)
    {
        // Cache some interfaces of CIEFrameAuto.
        //
        // Since we aggregate CIEFrameAuto, this will increase our refcount.
        // We cannot release this interface and expect it to work, so we
        // call release on ourself to remove the refcount cycle.
        //
        // Since we ourselves may be aggregated and we always want to get
        // CIEFrameAuto's interface and not our aggregator's, we delay
        // setting up punkOuter until below.
        //
        _pauto->QueryInterface(IID_IWebBrowser2, (void **)&_pautoWB2);
        ASSERT(_pautoWB2);
        Release();

        _pauto->QueryInterface(IID_IExpDispSupport, (void **)&_pautoEDS);
        ASSERT(_pautoEDS);
        Release();
    }

    // Now set up our aggregator's punkOuter
    if (punkOuter)
    {
        CAggregatedUnknown::_SetOuter(punkOuter);
    }

    // postpone initialization of stuff that may be persisted
    // until InitNew is called.

    // Were we successful? (we don't have to free this
    // here on failure, cuz we'll free them on delete)
    return (NULL!=_pauto);
}

CWebBrowserOC::~CWebBrowserOC()
{
    TraceMsg(TF_SHDLIFE, "dtor CWebBrowserOC %x", this);

    ASSERT(!_fDidRegisterAsBrowser);
    _UnregisterWindow();    // Last Chance - should have been done in InplaceDeactivate

    if (_psb) {
        ATOMICRELEASET(_psb, CWebBrowserSB);
    }

    ATOMICRELEASE(_plinkA);

    // We need to release these cached interface pointers.
    //
    // Since we cached them before setting up our outer-aggregation,
    // we need to un-outer-aggregate ourself first.
    //
    // Since we aggregate CIEFrameAuto (where these come from) we need
    // to AddRef ourself before releasing. Fortunately, this is done for us
    // by CAggregatedUnknown::Release (it bumps _cRef to 1000).
    //
    CAggregatedUnknown::_SetOuter(CAggregatedUnknown::_GetInner());
    ATOMICRELEASE(_pautoWB2);
    ATOMICRELEASE(_pautoEDS);

    ATOMICRELEASE(_pauto);

    if (_hmemSB) {
        GlobalFree(_hmemSB);
        _hmemSB = NULL;
    }

    if (_hBrowseUI != 0 && _hBrowseUI != HMODULE_NOTLOADED)
        FreeLibrary(_hBrowseUI);
}



IStream *CWebBrowserSB::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName)
{
    TCHAR szName[MAX_PATH];
    SHUnicodeToTChar(pwszName, szName, ARRAYSIZE(szName));
    return GetViewStream(pidl, grfMode, szName, REGSTR_PATH_EXPLORER TEXT("\\OCXStreamMRU"), TEXT("OCXStreams"));
}

void CWebBrowserOC::_InitDefault()
{
    if (_fInit)
    {
        IPSMSG(TEXT("_InitDefault already initialized"));
        return;
    }
    _fInit = TRUE;

    // Different versions of the control get different defaults.
    if (_pObjectInfo->lVersion == VERSION_1)
    {
        // AOL 3.0 compatibility: register as a browser window on InPlaceActivate
        _fShouldRegisterAsBrowser = TRUE;
    }
    else
    {
        // we use a zero-init memory allocator, so everything else is NULL.
        ASSERT(FALSE == _fShouldRegisterAsBrowser);
    }

    _size.cx = DEF_WIDTH;
    _size.cy = DEF_HEIGHT;
    _sizeHIM = _size;
    PixelsToMetric(&_sizeHIM);

    _fs.ViewMode = FVM_ICON;
    _fs.fFlags = FWF_AUTOARRANGE | FWF_NOCLIENTEDGE | FWF_SINGLECLICKACTIVATE;
    
}

void CWebBrowserOC::_RegisterWindow()
{
    if (!_fDidRegisterAsBrowser && _pipsite && _fShouldRegisterAsBrowser)
    {
        LPTARGETFRAME2 ptgf;
        HRESULT hr;
    
        if (SUCCEEDED(QueryInterface(IID_ITargetFrame2, (void **) &ptgf)))
        {
            LPUNKNOWN pUnkParent;

            hr = ptgf->GetParentFrame(&pUnkParent);
            if (SUCCEEDED(hr) && pUnkParent != NULL)
            {
                pUnkParent->Release();
            }
            else
            {
                IShellWindows* psw = WinList_GetShellWindows(TRUE);
                if (psw)
                {
                    IDispatch* pid;

                    if (SUCCEEDED(ptgf->QueryInterface(IID_IDispatch, (void **) &pid)))
                    {
                        psw->Register(pid, PtrToLong(_hwnd), SWC_3RDPARTY, &_cbCookie);
                        _fDidRegisterAsBrowser = 1;
                        pid->Release();
                    }
                    psw->Release();
                }
            }
            ptgf->Release();
        }
    }
}

void CWebBrowserOC::_UnregisterWindow()
{
    if (_fDidRegisterAsBrowser)
    {
        IShellWindows* psw = NULL;

        psw = WinList_GetShellWindows(TRUE);
        if (psw)
        {
            psw->Revoke(_cbCookie);
            _fDidRegisterAsBrowser = 0;
            psw->Release();
        }
    }
}

HRESULT CWebBrowserOC::Draw(
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
    IS_INITIALIZED;

    HRESULT hres;

    IVOMSG3(TEXT("Draw called"), lprcBounds->top, lprcBounds->bottom);
    IViewObject *pvo;
    if (_psb && _psb->GetShellView() &&
        SUCCEEDED(_psb->GetShellView()->QueryInterface(IID_IViewObject, (void **)&pvo))) 
    {
        hres = pvo->Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev, hdcDraw,
                              lprcBounds, lprcWBounds, pfnContinue, dwContinue);
        pvo->Release();
        if (SUCCEEDED(hres))
            return hres;
    }
    
    // If we don't have a shell view, or if it couldn't draw, then we draw ourselves.
    // (if we're in design mode).
    //
    if (_IsDesignMode())
    {
        FillRect(hdcDraw, (RECT*) lprcBounds, (HBRUSH)GetStockObject(BLACK_BRUSH));

        SIZE     size = { ABS( lprcBounds->right - lprcBounds->left ), 
                          ABS( lprcBounds->bottom - lprcBounds->top ) };

        HBITMAP  hImage;
        HDC      hdcTmp = CreateCompatibleDC( hdcDraw );
        HMODULE  hBrowseUI;

        hBrowseUI = _GetBrowseUI();
        if ( hBrowseUI )      
            hImage = LoadBitmap( hBrowseUI, MAKEINTRESOURCE( IDB_IEBRAND ));
        else 
            hImage = NULL;

        // hdcDraw may be a metafile, in which case the CreateCompatibleDC call will fail.
        // 
        if ( !hdcTmp )
        {
            hdcTmp = CreateCompatibleDC( hdcTargetDev );  // Okay if hdcTargetDev == NULL
        }

        if ( hImage )
        {
            BITMAP bm;
            POINT  ptOriginDest;  // origin of destination
            SIZE   sizeDest;
            POINT  ptOriginSrc = { 0, 0 };
            SIZE   sizeSrc;

            GetObject( hImage, sizeof( bm ), &bm );
            
            HGDIOBJ hPrev = SelectObject( hdcTmp, hImage );

            // Yes, this looks wrong, but it's right.  We only want the first frame
            // of the brand bitmap, and the frames are stacked vertically.
            //
            sizeSrc.cx = sizeSrc.cy = bm.bmWidth;
            
            // This code centers the bitmap while preserving its aspect ratio.
            //            
            if ( size.cx > size.cy )
            {
                // if destination is wider than tall,
                //
                ptOriginDest.x = lprcBounds->left + size.cx/2 - size.cy/2;
                ptOriginDest.y = lprcBounds->top;
                sizeDest.cx = size.cy;
                sizeDest.cy = lprcBounds->bottom - lprcBounds->top >= 0 ? size.cy : -size.cy;
            }
            else
            {
                // else destination is taller than wide
                //
                ptOriginDest.x = lprcBounds->left;
                ptOriginDest.y = lprcBounds->bottom - lprcBounds->top >= 0
                    ? ( lprcBounds->top + size.cy/2 - size.cx/2 )
                    : -( lprcBounds->top + size.cy/2 - size.cx/2 );
                sizeDest.cx = size.cx;
                sizeDest.cy = lprcBounds->bottom - lprcBounds->top >= 0 ? size.cx : -size.cx;
            }

            StretchBlt( hdcDraw,
                        ptOriginDest.x, ptOriginDest.y,
                        sizeDest.cx, sizeDest.cy,
                        hdcTmp, 
                        ptOriginSrc.x, ptOriginSrc.y,
                        sizeSrc.cx, sizeSrc.cy,
                        SRCCOPY );

            SelectObject( hdcTmp, hPrev );
            DeleteObject( hImage );
        }
            
        if ( hdcTmp )
        {
            DeleteDC( hdcTmp );
        }

        return S_OK;
    }
    
    return SUPERCLASS::Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev, hdcDraw,
                              lprcBounds, lprcWBounds, pfnContinue, dwContinue);
}

HRESULT CWebBrowserOC::GetColorSet(DWORD dwAspect, LONG lindex,
    void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    IViewObject *pvo;

    if (_psb && _psb->GetShellView() &&
        SUCCEEDED(_psb->GetShellView()->QueryInterface(IID_IViewObject,
        (void **)&pvo)))
    {
        HRESULT hres = pvo->GetColorSet(dwAspect, lindex, pvAspect, ptd,
            hicTargetDev, ppColorSet);

        pvo->Release();

        if (SUCCEEDED(hres))
            return hres;
    }

    return SUPERCLASS::GetColorSet(dwAspect, lindex, pvAspect, ptd,
        hicTargetDev, ppColorSet);
}

 HRESULT STDMETHODCALLTYPE CWebBrowserOC::SetExtent( DWORD dwDrawAspect,
            SIZEL *psizel)
{
    HRESULT hr = SUPERCLASS::SetExtent(dwDrawAspect, psizel);
    if ( FAILED( hr ))
    {
        return hr;
    }

    //
    // If oc < inplace then forward SetExtent through to docobject.
    // If docobject is already inplace active, SetExtent is meaningless.
    //
    
    if (_nActivate < OC_INPLACEACTIVE)
    {        
        IPrivateOleObject * pPrivOle = NULL;
        if ( _psb && _psb->GetShellView() && 
             SUCCEEDED(_psb->GetShellView()->QueryInterface(IID_IPrivateOleObject,
             (void **) &pPrivOle )))
        {
            // we have an ole object, delegate downwards...
            hr = pPrivOle->SetExtent( dwDrawAspect, psizel );
            pPrivOle->Release();
        }

        _dwDrawAspect = dwDrawAspect;
        // the size is already cached in _sizeHIM in our base class by the SUPERCLASS::SetExtent()
    }
    
    return hr;
}



// Called after the client site has been set so we can process
// stuff in the correct order
//
void CWebBrowserOC::_OnSetClientSite()
{
    // Until we have a client site we can't determine toplevelness.
    //
    if (_pcli)
    {
        BOOL fFoundBrowserService = FALSE;
        IBrowserService *pbsTop;

        if (SUCCEEDED(IUnknown_QueryService(_pcli, SID_STopLevelBrowser, IID_IBrowserService, (void **)&pbsTop)))
        {
            fFoundBrowserService = TRUE;
            pbsTop->Release();
        }

        // if nobody above us supports IBrowserService, we must be toplevel.
        if (!fFoundBrowserService)
            _fTopLevel = TRUE;


        // If we haven't created CWebBrowserSB, do so now.
        // We do this before our superclass OnSetClientSite
        // because shembed will create the window which
        // requires _psb's existence.
        //
        // NOTE: We do this here instead of _Initialize because
        // CBaseBrowser will QI us for interfaces during this call.
        // If we're in the middle of our CreateInstance function
        // and we've been aggregated, we pass the QI to _punkAgg
        // who faults because we haven't returned from CoCreateInstance.
        //
        // NOTE: We now destroy our window on SetClientSite(NULL)
        // which frees ths _psb, so we should create this every time.
        //
        if (EVAL(!_psb))
        {
            // Give _psb our inner unknown so we never get interfaces
            // from whoever may aggregate us. _GetInner gives us
            // first crack at QueryInterface so we get the correct
            // IWebBrowser implementation.
            //
            _psb = new CWebBrowserSB(CAggregatedUnknown::_GetInner(), this);
    
            // if we don't get _psb we're totally hosed...
            //
            if (_psb)
            {
                _psb->_fldBase._fld._fs = _fs;
                // tell _psb if it's top-level or not
                //
                if (_fTopLevel)
                {
                    _psb->SetTopBrowser();
                }

                // CBaseBrowser assumes SVUIA_ACTIVATE_FOCUS, tell it what we really are
                //
                ASSERT(OC_DEACTIVE == _nActivate); // we shouldn't be activated yet
                _psb->_UIActivateView(SVUIA_DEACTIVATE);
            }
            else
            {
                // don't let the window get created by our superclass,
                // as we can't do anything anyway...
                //
                return;
            }
        }
    }
    else
    {
        // Tell our aggregatee that their cached window is invalid
        //
        IEFrameAuto * piefa;
        if (EVAL(SUCCEEDED(_pauto->QueryInterface(IID_IEFrameAuto, (void **)&piefa))))
        {
            piefa->SetOwnerHwnd(NULL);
            piefa->Release();
        }

        if (_lpMF) 
        {
            IMessageFilter* lpMF = _lpMF;
            _lpMF = NULL;
            ((CMessageFilter *)lpMF)->UnInitialize();
            EVAL(lpMF->Release() == 0);
        }

        // Decrement the browser session count
        //
        if (_fTopLevel)
        {
            SetQueryNetSessionCount(SESSION_DECREMENT);
        }

    }

    SUPERCLASS::_OnSetClientSite();
    
    if(_pcli)
    {
        VARIANT_BOOL fAmbient;
        HWND         hwndParent = NULL;
        // We init the local properties using ambients if available
        if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, VT_BOOL, &fAmbient))
        {
            put_Offline(fAmbient);
        } 
                       
        if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_SILENT, VT_BOOL, &fAmbient))
        {
            put_Silent(fAmbient);
        }

        // After the parent window has been set, check to see if it is on the same thread as us.
        // If not, we have a cross-thread container and we need a message filter.
        //
        if ( _fTopLevel    // If we're top level
            && _hwnd       //  and we have an hwnd (we should)
            && (hwndParent = GetParent( _hwnd ) )  // and we have a parent window
                           //  and the parent window is on a different thread
            && GetWindowThreadProcessId( _hwnd, NULL ) != GetWindowThreadProcessId( hwndParent, NULL ))
        {
            if (!_lpMF)
            {
                /*
                 * Create a message filter here to reject RPC-reentrant calls.
                 */
                _lpMF = new CMessageFilter();

                if (_lpMF && !(((CMessageFilter *)_lpMF)->Initialize()))
                {
                    ATOMICRELEASE(_lpMF);
                }
                TraceMsg(TF_SHDCONTROL, "shv Registering message filter (%lx) for RPC-reentrancy", _lpMF);
            }
        }

        // Increment the browser session count.
        //
        if (_fTopLevel)
        {
            SetQueryNetSessionCount(SESSION_INCREMENT_NODEFAULTBROWSERCHECK);
        }

        // if we have a pending navigation from IPS::Load, do it now.
        //
        if (_fNavigateOnSetClientSite && _plinkA && _psb)
        {
            //
            // We hit this code if this OC is IPersistStream::Loaded before
            // the client site is set.
            //
            LPITEMIDLIST pidl;
            if (SUCCEEDED(_plinkA->GetIDList(&pidl)) && pidl)
            {
                _BrowseObject(pidl);
                ILFree(pidl);
            }

            _fNavigateOnSetClientSite = FALSE;
        }
    }
    
}

STDAPI CWebBrowserOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres;

    hres = E_OUTOFMEMORY;
    CWebBrowserOC* psvo = new CWebBrowserOC(NULL, poi);
    if (psvo)
    {
        if (!psvo->_InitializeOC(punkOuter))
        {
            psvo->Release();
        }
        else
        {
            *ppunk = psvo->_GetInner();
            hres = S_OK;
        }
    }
    return hres;
}


LRESULT CWebBrowserOC::_OnPaintPrint(HDC hdcPrint)
{
    PAINTSTRUCT ps;
    HDC hdc = hdcPrint ? hdcPrint : BeginPaint(_hwnd, &ps);
    RECT rc;
    GetClientRect(_hwnd, &rc);
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_ADJUST|BF_RECT|BF_SOFT);
    DrawText(hdc,
            hdcPrint ? TEXT("Print") : TEXT("Paint"),
            -1, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if (!hdcPrint) {
        EndPaint(_hwnd, &ps);
    }
    return 0;
}

HRESULT CWebBrowserOC::_BrowseObject(LPCITEMIDLIST pidlBrowseTo)
{
    if (_psb)
        return _psb->BrowseObject(pidlBrowseTo, SBSP_SAMEBROWSER|SBSP_ABSOLUTE);

    // if no _psb at this point, container did not
    // honor OLEMISC_SETCLIENTSITEFIRST bit, so silently
    // fail instead of trying to make this work
    ASSERT(FALSE);
    return E_FAIL;
}

LRESULT CWebBrowserOC::_OnCreate(LPCREATESTRUCT lpcs)
{
    LRESULT lres;

    ASSERT(_psb && _hwnd);

    _psb->_SetWindow(_hwnd);

    lres = _psb->OnCreate(NULL);

    //
    //  If IPersistStream::Load has stored away a block of stream for
    // toolbars, this is the time to use it.
    //
    if (_hmemSB) {
#if 0
        // only do the load if we're successfully creating the window
        if (lres != -1) {
            IStream* pstm;
            HRESULT hresT = ::CreateStreamOnHGlobal(_hmemSB, FALSE, &pstm);
            if (SUCCEEDED(hresT)) {
                _psb->_LoadToolbars(pstm);
                pstm->Release();
            }
        }
#endif

        GlobalFree(_hmemSB);
        _hmemSB = NULL;
    }

    return lres;
}

//
//  This is a virtual function defined in CShellEmbedding class, which
// is called when all but WM_NCCREATE and WM_NCDESTROY messages are
// dispatched to the our "Shell Embedding" window. It's important
// to remember that we pass this window handle to the constructor of
// CWebBrowserSB (which calls the constructor of CBaseBrowser).
// That's why we forward all messages to _psb->WndProcBS. (SatoNa)
//
LRESULT CWebBrowserOC::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0L;

    if (WM_CREATE == uMsg)
    {
        // We need first crack at this message before
        // forwarding it along to _psb (which we do
        // in this _OnCreate call)
        return _OnCreate((LPCREATESTRUCT)lParam);
    }

    // only let _psb look at it if the message is not reserved for us
    if (!IsInRange(uMsg, CWM_RESERVEDFORWEBBROWSER_FIRST, CWM_RESERVEDFORWEBBROWSER_LAST))
    {
        BOOL fDontForward = FALSE;
        
        // destroy bottom up for these 
        switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
            SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
            fDontForward = TRUE;
            break;
        }

        //
        // This AssertMsg will help debugging IE v4.1 bug 12931.
        //
        //  Comment out assert now that we don't create _psb at constructor time.  (?)
        //AssertMsg((BOOL)_psb, "WBOC::v_WndProc _psb==NULL! uMsg=%x", uMsg);

        if (_psb)
        {
            lres = _psb->WndProcBS(hwnd, uMsg, wParam, lParam);

            //  Due to aggregation of IEFrameAuto, _psb is holding references
            //  to us, so we need to break the cycle.
            //  _psb may have been freed above.  Double check it.
            //
            if (uMsg == WM_DESTROY && _psb)
            {
                _psb->ReleaseShellView();
                _psb->ReleaseShellExplorer();
                ATOMICRELEASET(_psb, CWebBrowserSB);
            }
        }
        if (uMsg >= WM_USER || fDontForward)
        {
            return lres;
        }
    }

    switch(uMsg)
    {
        /* these are handled by CBaseBrowser only */
    case WM_NOTIFY:
        return lres;

    case WM_SETCURSOR:
        if (lres) {
            return lres;
        }
        goto DoDefault;

DoDefault:
    default:
        return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
    }

    return 0L;
}

HRESULT CWebBrowserOC::Close(DWORD dwSaveOption)
{
    if (_psb)
    {
        _psb->_CancelPendingNavigation();
    }
    return SUPERCLASS::Close(dwSaveOption);
}

HRESULT CWebBrowserOC::SetHostNames(
    LPCOLESTR szContainerApp,
    LPCOLESTR szContainerObj)
{
    IOOMSG(TEXT("SetHostNames"));
    // We are not interested in host name
    // ...well ... maybe a little.  this turns out to be the best place to
    // do an apphack for VC 5.0.  VC 5.0 has a bug where it calls Release()
    // one too many times.  the only good way to detect being hosted in the
    // offending container is to check the szContainerApp in SetHostNames!
    // ...chrisfra 8/14/97, bug 30428
    // NOTE: Mike Colee of VC verified their bug and will put a Raid bug in
    // their database including how to signal that a new version works by
    // changing szContainerApp string.
    if (_fTopLevel && szContainerApp && !StrCmpW(szContainerApp, L"DevIV Package"))
    {
        AddRef();
    }
    return SUPERCLASS::SetHostNames(szContainerApp, szContainerObj);
}

HRESULT CWebBrowserOC::DoVerb(
    LONG iVerb,
    LPMSG lpmsg,
    IOleClientSite *pActiveSite,
    LONG lindex,
    HWND hwndParent,
    LPCRECT lprcPosRect)
{
    IOOMSG2(TEXT("DoVerb"), iVerb);
    IS_INITIALIZED;

    HRESULT hres;

    _pmsgDoVerb = lpmsg;

    hres = SUPERCLASS::DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);

    _pmsgDoVerb = NULL;

    return hres;
}

// *** IPersistStreamInit ***

// in order to have upgrade and downgrade compatibility in stream formats
// we can't have any size assumptions in this code. Extra data streamed
// after our PersistSVOCX structure must be referenced by a dwOffX offset
// so the downgrade case knows where to start reading from.
//
// since we always write out a stream that's downward compatible, we don't
// need to folow the "source compatible" rule of: reading an old stream
// with an old CLSID WebBrowser implies that when we save, we need to
// save using the old stream format.
//
typedef struct _PersistSVOCX
{
    struct _tagIE30 {
        DWORD cbSize;
        SIZE sizeObject;            // IE3 saves as PIXELS, IE4 saves as HIMETRIC
        FOLDERSETTINGS fs;
        long lAutoSize;             // IE3, no longer used
        DWORD fColorsSet;           // IE3, no longer used
        COLORREF clrBack;           // IE3, no longer used
        COLORREF clrFore;           // IE3, no longer used
        DWORD dwOffPersistLink;
        long lAutoSizePercentage;   // IE3, no longer used
    } ie30;
    struct _tagIE40 {
        DWORD   dwExtra;
        BOOL    bRestoreView;
        SHELLVIEWID vid;
        DWORD   fFlags;
        DWORD   dwVersion;
    } ie40;
} PersistSVOCX;

//
// Flags for dwExtra. Having a flag indicate that we have some extra
// streamed data after this structure + persisted link.
//
// NOTE: All data stored this way (instead of storing an offset to
// the data, such as dwOffPersistLink) will be lost on downgrade
// cases and cases where we have to emulate old stream formats.
//
#define SVO_EXTRA_TOOLBARS 0x00000001

// Random flags we need to persist
#define SVO_FLAGS_OFFLINE           0x00000001
#define SVO_FLAGS_SILENT            0x00000002
#define SVO_FLAGS_REGISTERASBROWSER 0x00000004
#define SVO_FLAGS_REGISTERASDROPTGT 0x00000008

#define SVO_VERSION 0 // increment for upgrade changes when size doesn't change

HRESULT CWebBrowserOC::Load(IStream *pstm)
{
    IPSMSG(TEXT("Load"));
    // Load _size
    ULONG cbRead;
    PersistSVOCX sPersist;
    HRESULT hres, hresNavigate = E_FAIL;
    DWORD dwExtra = 0;

    // It is illegal to call Load or InitNew more than once
    if (_fInit)
    {
        TraceMsg(TF_SHDCONTROL, "shv IPersistStream::Load called when ALREADY INITIALIZED!");
        ASSERT(FALSE);
        return(E_FAIL);
    }

    // we need an IShellLink to read into
    if (_plinkA == NULL)
    {
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void **)&_plinkA);
        if (FAILED(hres))
            return hres;
    }

    // remember our starting location
    ULARGE_INTEGER liStart;
    //ULARGE_INTEGER liEnd;
    LARGE_INTEGER liMove;
    liMove.LowPart = liMove.HighPart = 0;
    hres = pstm->Seek(liMove, STREAM_SEEK_CUR, &liStart);
    if (FAILED(hres))
    {
        return hres;
    }

    // Zero initialize our struct
    ZeroMemory(&sPersist, SIZEOF(sPersist));

    hres = pstm->Read(&sPersist, SIZEOF(DWORD), &cbRead);
    if (SUCCEEDED(hres))
    {
        // Validate the data
        if (cbRead != SIZEOF(DWORD) ||
            sPersist.ie30.cbSize < SIZEOF(sPersist.ie30))
        {
            TraceMsg(DM_ERROR, "Someone is asking us to read the wrong thing.");
            hres = E_FAIL;
        }
        else
        {
            DWORD cbSizeToRead = sPersist.ie30.cbSize;
            if (cbSizeToRead > SIZEOF(sPersist))
            {
                // must be a newer struct, only read what we know (ie, don't trash the stack!)
                cbSizeToRead = SIZEOF(sPersist);
            }
            cbSizeToRead -= SIZEOF(DWORD); // remove what we've already read
            hres = pstm->Read(&sPersist.ie30.sizeObject, cbSizeToRead, &cbRead);
            if (SUCCEEDED(hres))
            {
                if (cbRead != cbSizeToRead)
                {
                    hres = E_FAIL;
                }
                else
                {
                    // read ie30 data
                    //
                    if (EVAL(sPersist.ie30.cbSize >= SIZEOF(sPersist.ie30)))
                    {
                        _size = sPersist.ie30.sizeObject;
                        _fs = sPersist.ie30.fs;

                        // IE3 saved size in pixels IE4 saves size in HIM already
                        //
                        _sizeHIM = _size;
                        if (sPersist.ie30.cbSize == SIZEOF(sPersist.ie30) ||
                            sPersist.ie30.cbSize == SIZEOF(sPersist) - SIZEOF(sPersist.ie40.dwVersion)) // handle upgrade
                        {
                            // Size is in pixels.  Adjust _sizeHIM to Hi Metric.
                            PixelsToMetric(&_sizeHIM);
                        }
                        else
                        {
                            // Size is in Hi Metric.  Adjust _size to Pixels.
                            MetricToPixels(&_size);
                        }

                        if (_psb) // if no _psb then container ignored OLEMISC_SETCLIENTSITEFIRST
                            _psb->_fldBase._fld._fs = _fs;
                            
                        // Load _plinkA
                        IPersistStream* ppstm;
                        hres = _plinkA->QueryInterface(IID_IPersistStream, (void **)&ppstm);
                        if (SUCCEEDED(hres))
                        {
                            ASSERT(sPersist.ie30.dwOffPersistLink >= sPersist.ie30.cbSize);
                            liMove.LowPart = liStart.LowPart + sPersist.ie30.dwOffPersistLink;

                            hres = pstm->Seek(liMove, STREAM_SEEK_SET, NULL);
                            if (SUCCEEDED(hres))
                            {
                                hres = ppstm->Load(pstm);
                                if (SUCCEEDED(hres)) 
                                {
                                    // We always save link info last,
                                    // so remember where we are in the stream.
                                    // Since we don't have more dwOff variables
                                    // currently, don't bother remebering this...
                                    //hres = pstm->Seek(liMove, STREAM_SEEK_CUR, &liEnd);
                                    //if (SUCCEEDED(hres))
                                        _fInit = TRUE;
                                
                                    // in case the target moved invoke link tracking (ignore errors)
                                    _plinkA->Resolve(_hwnd, SLR_NO_UI);

                                    // If we already have the client site,
                                    // navigate now. Otherwise, navigate
                                    // when the client site is set.
                                    if (_pcli) 
                                    {
                                        LPITEMIDLIST pidl;
                                        if (SUCCEEDED(_plinkA->GetIDList(&pidl)) && pidl)
                                        {
                                            ASSERT(FALSE == _psb->_fAsyncNavigate);
                                            _psb->_fAsyncNavigate = TRUE;
                                            hresNavigate = _BrowseObject(pidl);
                                            _psb->_fAsyncNavigate = FALSE;
                                            ILFree(pidl);
                                        }
                                    } 
                                    else 
                                    {
                                        _fNavigateOnSetClientSite = TRUE;
                                    }
                                }
                            }

                            ppstm->Release();
                        }
                    } // read ie30 data


                    // temp upgrade hack for older struct
                    if (sPersist.ie30.cbSize == SIZEOF(sPersist) - SIZEOF(sPersist.ie40.dwVersion))
                    {
                        // dwVersion field is already correct, update cbSize
                        // to pass below size check
                        sPersist.ie30.cbSize = SIZEOF(sPersist);
                    }

                    // read ie40 data
                    if (SUCCEEDED(hres) &&
                        sPersist.ie30.cbSize >= SIZEOF(sPersist))
                    {
                        if (_psb) // if no _psb then container ignored OLEMISC_SETCLIENTSITEFIRST
                        {
                            if (sPersist.ie40.bRestoreView)
                            {
                                _psb->_fldBase._fld._vidRestore = sPersist.ie40.vid;
                                // since we read the ie40 data, this is a cache hit
                                _psb->_fldBase._fld._dwViewPriority = VIEW_PRIORITY_CACHEHIT;
                            }
                        }

                        // We let ambients take precedence over what we have persisted
                        VARIANT_BOOL fAmbient;
                        if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, VT_BOOL, &fAmbient))
                        {
                            put_Offline(fAmbient);
                        } 
                        else
                        {
                            put_Offline((sPersist.ie40.fFlags & SVO_FLAGS_OFFLINE) ? -1 : FALSE);
                        }
                        if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_SILENT, VT_BOOL, &fAmbient))
                        {
                            put_Silent(fAmbient);
                        }
                        else
                        {
                            put_Silent((sPersist.ie40.fFlags & SVO_FLAGS_SILENT) ? -1 : FALSE);
                        }

                        
                        put_RegisterAsDropTarget((sPersist.ie40.fFlags & SVO_FLAGS_REGISTERASDROPTGT) ? -1 : FALSE);

                        _fShouldRegisterAsBrowser = (sPersist.ie40.fFlags & SVO_FLAGS_REGISTERASBROWSER) ? TRUE : FALSE;

                        // remember this for later
                        dwExtra = sPersist.ie40.dwExtra;
                    }
                    else
                    {
                        // if CLSID_WebBrowser_V1 reads an old stream format,
                        // it means that we must write out an old stream format later
                        // remember this...
                        if (_pObjectInfo->lVersion == VERSION_1)
                        {
                            _fEmulateOldStream = TRUE;
                        }
                    } // read ie40 data
                }

                // if we read all the data then make sure we're at
                // the end of the stream
                if (SUCCEEDED(hres) && _fInit)
                {
                    //liMove.LowPart = liEnd.LowPart;
                    //hres = pstm->Seek(liMove, STREAM_SEEK_SET, NULL);

                    // now we can read in extra streamed data if we have it
                    if (dwExtra & SVO_EXTRA_TOOLBARS)
                    {
                        DWORD dwTotal;
                        hres = pstm->Read(&dwTotal, SIZEOF(dwTotal), NULL);
                        if (SUCCEEDED(hres)) 
                        {
                            ASSERT(dwTotal >= SIZEOF(dwTotal));
                            dwTotal -= SIZEOF(dwTotal);

                            if (_hmemSB) 
                            {
                                GlobalFree(_hmemSB);
                            }

                            _hmemSB = GlobalAlloc(GPTR, dwTotal);
                            if (_hmemSB) 
                            {
                                hres = pstm->Read((BYTE*)_hmemSB, dwTotal, NULL);
                            }
                            else 
                            {
                                hres = E_OUTOFMEMORY;
                            }
                        }
                    }
                }
            }
        }
    }

    _OnLoaded(FAILED(hresNavigate));

    if (SUCCEEDED(hres))
        hres = S_OK;    // convert S_FALSE to S_OK
 
    return hres;
}

HRESULT CWebBrowserOC_SavePersistData(IStream *pstm, SIZE* psizeObj,
    FOLDERSETTINGS* pfs, IShellLinkA* plinkA, SHELLVIEWID* pvid,
    BOOL fOffline, BOOL fSilent, BOOL fRegisterAsBrowser, BOOL fRegisterAsDropTarget,
    BOOL fEmulateOldStream, DWORD *pdwExtra)
{
    ULONG cbWritten;
    PersistSVOCX sPersist;
    HRESULT hres;

    // This means that this instance of CWebBrowserOC was created using
    // the ie30 CLSID and we were persisted from an old format stream.
    // Under this scenario, we have to write out a stream format that
    // can be read by the old ie30 object.
    if (fEmulateOldStream && pdwExtra)
    {
        // The only data we stream out that can't be read back in by
        // the old ie30 webbrowser is the dwExtra data.
        *pdwExtra = 0;
    }


    ZeroMemory(&sPersist, SIZEOF(sPersist));

    sPersist.ie30.cbSize = fEmulateOldStream ? SIZEOF(sPersist.ie30) : SIZEOF(sPersist);
    sPersist.ie30.sizeObject = *psizeObj;
    sPersist.ie30.fs = *pfs;
    sPersist.ie30.dwOffPersistLink = SIZEOF(sPersist);
    if (pvid)
    {
        sPersist.ie40.bRestoreView = TRUE;
        sPersist.ie40.vid = *pvid;
    }
    sPersist.ie40.dwExtra = pdwExtra ? *pdwExtra : 0;
    if (fOffline)
        sPersist.ie40.fFlags |= SVO_FLAGS_OFFLINE;
    if (fSilent)
        sPersist.ie40.fFlags |= SVO_FLAGS_SILENT;
    if (fRegisterAsBrowser)
        sPersist.ie40.fFlags |= SVO_FLAGS_REGISTERASBROWSER;
    if (fRegisterAsDropTarget)
        sPersist.ie40.fFlags |= SVO_FLAGS_REGISTERASDROPTGT;
    sPersist.ie40.dwVersion = SVO_VERSION;

    hres = pstm->Write(&sPersist, SIZEOF(sPersist), &cbWritten);
    IPSMSG3(TEXT("Save 1st Write(&_size) returned"), hres, cbWritten, sizeof(*psizeObj));
    if (SUCCEEDED(hres))
    {
        // Save plinkA
        ASSERT(plinkA);
        IPersistStream* ppstm;
        hres = plinkA->QueryInterface(IID_IPersistStream, (void **)&ppstm);
        if (SUCCEEDED(hres))
        {
            hres = ppstm->Save(pstm, TRUE);
            IPSMSG2(TEXT("Save plink->Save() returned"), hres);

            ppstm->Release();
        }
    }

    return hres;
}

BOOL CWebBrowserOC::_GetViewInfo(SHELLVIEWID* pvid)
{
    BOOL bGotView = FALSE;

    if (_psb)
    {
        if (_psb->GetShellView())
        {
            _psb->GetShellView()->GetCurrentInfo(&_fs);
        }
        else
        {
            _fs = _psb->_fldBase._fld._fs;
        }
        bGotView = FileCabinet_GetDefaultViewID2(&_psb->_fldBase, pvid);
    }
    
    return bGotView;
}

HRESULT CWebBrowserOC::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hres;
    LPITEMIDLIST pidl;
    SHELLVIEWID vid;
    BOOL bGotView;
    VARIANT_BOOL fOffline, fSilent, fRegDT;

    IPSMSG(TEXT("Save"));
    IS_INITIALIZED;

    pidl = NULL;
    if (_psb)
        pidl = _psb->_bbd._pidlCur;

    // we need an IShellLink to save with
    if (_plinkA == NULL)
    {
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void **)&_plinkA);
        if (FAILED(hres))
            return hres;
    }
    _plinkA->SetIDList(pidl);

    bGotView = _GetViewInfo(&vid);

    DWORD dwExtra = 0;
#if 0
    if (_psb && _psb->_SaveToolbars(NULL)==S_OK) {
        dwExtra |= SVO_EXTRA_TOOLBARS;
    }
#endif

    get_Offline(&fOffline);
    get_Silent(&fSilent);
    get_RegisterAsDropTarget(&fRegDT);

    hres = CWebBrowserOC_SavePersistData(pstm, _fEmulateOldStream ? &_size : &_sizeHIM, &_fs, _plinkA,
            (bGotView ? &vid : NULL),
            fOffline, fSilent, _fShouldRegisterAsBrowser, fRegDT,
            _fEmulateOldStream, &dwExtra);

    ASSERT(!(dwExtra & SVO_EXTRA_TOOLBARS));
#if 0
    if (SUCCEEDED(hres) && (dwExtra & SVO_EXTRA_TOOLBARS)) {

        // Remember the current location.
        ULARGE_INTEGER liStart;
        pstm->Seek(c_li0, STREAM_SEEK_CUR, &liStart);

        // Write the dummy size.
        DWORD dwTotal = 0;
        hres = pstm->Write(&dwTotal, SIZEOF(dwTotal), NULL);
        if (SUCCEEDED(hres)) {
            if (_psb)
                hres = _psb->_SaveToolbars(pstm);

            // Remember the end
            ULARGE_INTEGER liEnd;
            pstm->Seek(c_li0, STREAM_SEEK_CUR, &liEnd);

            // Seek back to the original location
            LARGE_INTEGER liT;
            liT.HighPart = 0;
            liT.LowPart = liStart.LowPart; 
            pstm->Seek(liT, STREAM_SEEK_SET, NULL);

            // Get the real dwTotal and write it
            dwTotal = liEnd.LowPart - liStart.LowPart;
            hres = pstm->Write(&dwTotal, SIZEOF(dwTotal), NULL);

            // Seek forward to the end
            liT.LowPart = liEnd.LowPart;
            pstm->Seek(liT, STREAM_SEEK_SET, NULL);
        }
    }
#endif

    if (fClearDirty)
    {
        _fDirty = FALSE;
    }

    return hres;
}

void CWebBrowserOC::_OnLoaded(BOOL fUpdateBrowserReadyState)
{
    IEFrameAuto * piefa;
    if (SUCCEEDED(_pauto->QueryInterface(IID_IEFrameAuto, (void **)&piefa)))
    {
        piefa->put_DefaultReadyState(READYSTATE_COMPLETE, fUpdateBrowserReadyState);
        piefa->Release();
    }
}

HRESULT CWebBrowserOC::InitNew(void)
{
    IPSMSG(TEXT("InitNew"));

    _InitDefault();

    // On the InitNew case, we do want to update the browser's ready state
    _OnLoaded(TRUE);


    return NOERROR;
}

// *** IPersistPropertyBag ***

static const struct tagBOOLPROPS {
    LPOLESTR pName;
    UINT     flag;
} g_boolprops[] = {
    {L"AutoArrange",    FWF_AUTOARRANGE},
    {L"NoClientEdge",   FWF_NOCLIENTEDGE},
    {L"AlignLeft",      FWF_ALIGNLEFT}
};

HRESULT CWebBrowserOC::Load(IPropertyBag *pBag, IErrorLog *pErrorLog)
{
    HRESULT hr = S_OK;

    VARIANT var;
    int i;

    BOOL fOffline = FALSE;
    BOOL fSilent = FALSE;
    BOOL fRegisterAsBrowser = FALSE;
    BOOL fRegisterAsDropTgt = FALSE;
    BOOL fUpdateBrowserReadyState = TRUE;
    VARIANT_BOOL fAmbient;

    IPSMSG(TEXT("Load PropertyBag"));

    // It is illegal to call ::Load or ::InitNew more than once
    if (_fInit)
    {
        TraceMsg(TF_SHDCONTROL, "shv IPersistPropertyBag::Load called when ALREADY INITIALIZED!");
        ASSERT(FALSE);
        return(E_FAIL);
    }

    _InitDefault();

    // grab all our DWORD-sized (VT_UI4) properties
    struct tagDWPROPS {
        LPOLESTR pName;
        DWORD*   pdw;
    } dwprops[] = {
#ifdef RECALCSIZE
        {L"AutoSize",       (LPDWORD)&_lAutoSize},
        {L"AutoSizePercentage", (LPDWORD)&_lAutoSizePercentage},
#endif
        {L"Height",         (LPDWORD)&_size.cy},
        {L"Width",          (LPDWORD)&_size.cx},
        {L"ViewMode",       (LPDWORD)&_fs.ViewMode},
        {L"Offline",        (LPDWORD)&fOffline},
        {L"Silent",         (LPDWORD)&fSilent},
        {L"RegisterAsBrowser", (LPDWORD)&fRegisterAsBrowser},
        {L"RegisterAsDropTarget", (LPDWORD)&fRegisterAsDropTgt}
    };
    VariantInit(&var);
    var.vt = VT_UI4;
    for (i=0 ; i < ARRAYSIZE(dwprops) ; i++)
    {
        if (SUCCEEDED(pBag->Read(dwprops[i].pName, &var, pErrorLog)))
        {
            *dwprops[i].pdw = var.lVal;
            // don't need to free a VT_UI4
        }
        else
        {
#ifdef DEBUG
            TraceMsg(TF_SHDCONTROL, "Load PropertyBag did not find %hs for DWORD", dwprops[i].pName);
#endif
        }
    }
    // We let ambients take precedence over what we have persisted
    if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, VT_BOOL, &fAmbient))
    {
        put_Offline(fAmbient);
    } 
    else
    {
        put_Offline(fOffline ? -1 : FALSE);
    }
    if (SUPERCLASS::_GetAmbientProperty(DISPID_AMBIENT_SILENT, VT_BOOL, &fAmbient))
    {
        put_Silent(fAmbient);
    }
    else
    {
        put_Silent(fSilent ? -1 : FALSE);
    }

    // If You use fOffline or fSilent after this, you will have to re-init them according 
    // to the returned values of fAmbient in the if statements above
    
    put_RegisterAsDropTarget(fRegisterAsDropTgt ? -1 : FALSE);
    _fShouldRegisterAsBrowser = fRegisterAsBrowser ? TRUE : FALSE;

    // IE3 saved PIXEL size, IE4 saves HIMETRIC size
    HRESULT hres = pBag->Read(L"ExtentX", &var, pErrorLog);
    if (SUCCEEDED(hres))
    {
        _sizeHIM.cx = var.lVal;
        hres = pBag->Read(L"ExtentY", &var, pErrorLog);
        if (SUCCEEDED(hres))
        {
            _sizeHIM.cy = var.lVal;
        }
    }
    if (FAILED(hres))
    {
        // convert IE3 info to HIMETRIC
        _sizeHIM = _size;
        PixelsToMetric(&_sizeHIM);
    }

#ifdef RECALCSIZE
    // if we're autosizing, we should never see the scrollbar
    if (_lAutoSize & AUTOSIZE_ON)
        _fs.fFlags |= FWF_NOSCROLL;
    else
        _fs.fFlags &= ~FWF_NOSCROLL;
#endif

    // grab all our _fs.fFlags (VT_BOOL) flags
    var.vt = VT_BOOL;
    for (i=0 ; i < ARRAYSIZE(g_boolprops) ; i++)
    {
        if (SUCCEEDED(pBag->Read(g_boolprops[i].pName, &var, pErrorLog)))
        {
            if (var.boolVal)
                _fs.fFlags |= g_boolprops[i].flag;
            else
                _fs.fFlags &= ~g_boolprops[i].flag;
            // don't need to free a VT_BOOL
        }
        else
        {
            TraceMsg(TF_SHDCONTROL, "Load PropertyBag did not find %ws for BOOL", g_boolprops[i].pName);
        }
    }

    // grab special properties

    VariantInit(&var); // because we don't VariantClear in above loop
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocStringLen(NULL, 1); // OLE Automation bug - they deref even if null!
    if (NULL != var.bstrVal)
    {
        if (SUCCEEDED(pBag->Read(L"ViewID", &var, pErrorLog)))
        {
            if (_psb) // if no _psb then container ignored OLEMISC_SETCLIENTSITEFIRST
            {
                // Buffer Overrun check.  Proper length of a GUID string is 38 characters
                //
                if (lstrlenW(var.bstrVal) < 39)
                {
                    CLSIDFromString(var.bstrVal, &_psb->_fldBase._fld._vidRestore);
                    // we sucessfully read the ViewID, so this is a cache hit
                    _psb->_fldBase._fld._dwViewPriority = VIEW_PRIORITY_CACHEHIT;
                }
            }
        }

        // do this before we read our location
        if (_psb) // if no _psb then container ignored OLEMISC_SETCLIENTSITEFIRST
        {
            _psb->_fldBase._fld._fs = _fs;

            ASSERT(VT_BSTR == var.vt);
            if (SUCCEEDED(pBag->Read(L"Location", &var, pErrorLog)))
            {
                ASSERT(FALSE == _psb->_fAsyncNavigate);

                hr = WrapSpecialUrl(&(var.bstrVal));
                if (SUCCEEDED(hr))
                {
                    _psb->_fAsyncNavigate = TRUE;

                    if (FAILED(Navigate(var.bstrVal,NULL,NULL,NULL,NULL)))
                    {
                        TraceMsg(TF_SHDCONTROL, "Load PropertyBag Navigate FAILED!");
                    }
                    else
                    {
                        // Navigate is successful (at least initially), let it update ReadyState
                        //
                        fUpdateBrowserReadyState = FALSE;
                    }
                    _psb->_fAsyncNavigate = FALSE;
                }
            }
            else
            {
                TraceMsg(TF_SHDCONTROL, "Load PropertyBag did not find Location");
            }
        }

        VariantClear(&var);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    _OnLoaded(fUpdateBrowserReadyState);
 
    return hr;
}
HRESULT CWebBrowserOC::Save(IPropertyBag *pBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    IPSMSG(TEXT("Save PropertyBag"));
    IS_INITIALIZED;

    HRESULT hres;
    VARIANT var;
    int i;
    SHELLVIEWID vid;
    BOOL bGotView = FALSE;

    VARIANT_BOOL f;
    BOOL fOffline;
    BOOL fSilent;
    BOOL fRegisterAsDropTgt;
    BOOL fRegisterAsBrowser;
    
    get_Offline(&f);
    fOffline = f ? TRUE : FALSE;
    get_Silent(&f);
    fSilent = f ? TRUE : FALSE;
    get_RegisterAsDropTarget(&f);
    fRegisterAsDropTgt = f ? TRUE : FALSE;
    fRegisterAsBrowser = _fShouldRegisterAsBrowser ? TRUE : FALSE;

    // our state may have changed
    bGotView = _GetViewInfo(&vid);

    // save all our DWORD-sized properties
    struct tagDWPROPS {
        LPOLESTR pName;
        DWORD*   pdw;
    } dwprops[] = {
#ifdef RECALCSIZE
        {L"AutoSize",       (LPDWORD)&_lAutoSize},
        {L"AutoSizePercentage", (LPDWORD)&_lAutoSizePercentage},
#endif
        {L"ExtentX",        (LPDWORD)&_sizeHIM.cx},
        {L"ExtentY",        (LPDWORD)&_sizeHIM.cy},
        {L"ViewMode",       (LPDWORD)&_fs.ViewMode},
        {L"Offline",        (LPDWORD)&fOffline},
        {L"Silent",         (LPDWORD)&fSilent},
        {L"RegisterAsBrowser", (LPDWORD)&fRegisterAsBrowser},
        {L"RegisterAsDropTarget", (LPDWORD)&fRegisterAsDropTgt},

        // IE3 OC stuff here
        {L"Height",         (LPDWORD)&_size.cy},
        {L"Width",          (LPDWORD)&_size.cx}
    };
    VariantInit(&var);
    var.vt = VT_I4; // VB doesn't understand VT_UI4! (pBag->Write succeeds,
                    // but it doesn't write anything! The load then fails!)
    int nCount = ARRAYSIZE(dwprops);
    if (_pObjectInfo->lVersion != VERSION_1)
        nCount -= 2;
    for (i=0 ; i < nCount ; i++)
    {
        var.lVal = *dwprops[i].pdw;
        hres = pBag->Write(dwprops[i].pName, &var);
        if (FAILED(hres))
        {
            TraceMsg(TF_SHDCONTROL, "Save PropertyBag could not save %ws for DWORD", dwprops[i].pName);
            return(hres);
        }
    }

    // save all our _fs.fFlags (VT_BOOL) flags
    var.vt = VT_BOOL;
    for (i=0 ; i < ARRAYSIZE(g_boolprops) ; i++)
    {
        var.boolVal = (_fs.fFlags & g_boolprops[i].flag) ? TRUE : FALSE;
        hres = pBag->Write(g_boolprops[i].pName, &var);
        if (FAILED(hres))
        {
            TraceMsg(TF_SHDCONTROL, "Load PropertyBag did not save %ws for BOOL", g_boolprops[i].pName);
        }
    }

    // save special properties

    if (bGotView)
    {
        int n;

        var.vt = VT_BSTR;
        var.bstrVal = SysAllocStringLen(NULL, GUIDSTR_MAX);

        if (var.bstrVal)
        {
            n = StringFromGUID2(vid, var.bstrVal, GUIDSTR_MAX);
            ASSERT(n);
            if (n)
            {
                pBag->Write(L"ViewID", &var);
            }
            VariantClear(&var);
        }
    }

    var.vt = VT_BSTR;
    if (SUCCEEDED(get_LocationURL(&var.bstrVal)))
    {
        hres = pBag->Write(L"Location", &var);

        VariantClear(&var);

        if (FAILED(hres))
        {
            TraceMsg(TF_SHDCONTROL, "Save PropertyBag could not save Location");
            return(hres);
        }
    }
    else
    {
        var.vt = VT_EMPTY;
        TraceMsg(TF_SHDCONTROL, "Save PropertyBag get_Location FAILED!");
    }

    return NOERROR;
}


// *** IPersistString ***

STDMETHODIMP CWebBrowserOC::Initialize(LPCWSTR pwszInit)
{
    HRESULT hres = E_OUTOFMEMORY;
    BSTR bstr = SysAllocString(pwszInit);

    if (bstr)
    {
        hres = Navigate(bstr,NULL,NULL,NULL,NULL);
        SysFreeString(bstr);
    }

    return hres;
}


#define THISCLASS CWebBrowserOC
HRESULT CWebBrowserOC::v_InternalQueryInterface(REFIID riid, void ** ppvObj)
{
    HRESULT hres;

    static const QITAB qit[] = {
        QITABENT(THISCLASS, IWebBrowser2),
        QITABENTMULTI(THISCLASS, IDispatch, IWebBrowser2), // VBA QI's IDispatch and assumes it gets the "default" automation interface, which is IWebBrowser2
        QITABENTMULTI(THISCLASS, IWebBrowser, IWebBrowser2),
        QITABENTMULTI(THISCLASS, IWebBrowserApp, IWebBrowser2),
        QITABENT(THISCLASS, IPersistString),
        QITABENT(THISCLASS, IOleCommandTarget),
        QITABENT(THISCLASS, IObjectSafety),
        QITABENT(THISCLASS, ITargetEmbedding),
        QITABENT(THISCLASS, IExpDispSupport),
        QITABENT(THISCLASS, IExpDispSupportOC),
        QITABENT(THISCLASS, IPersistHistory),
        QITABENT(THISCLASS, IPersistStorage),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
    {
        hres = SUPERCLASS::v_InternalQueryInterface(riid, ppvObj);

        // we want to expose our aggregated CIEFrameAuto's interfaces
        // in addition to ours.
        if (FAILED(hres))
        {
            hres = _pauto->QueryInterface(riid, ppvObj);
        }
    }

    return hres;
}

// *** IOleInPlaceActiveObject ***
HRESULT CWebBrowserOC::OnFrameWindowActivate(BOOL fActivate)
{
    if (_psb)
        _psb->OnFrameWindowActivateBS(fActivate);
        
    return S_OK;
}

HRESULT CWebBrowserOC::TranslateAccelerator(LPMSG lpMsg)
{
    if (_psb)
    {
        // BUGBUG REVIEW: what wID should we really pass? Is 0 ok?
        if (S_OK == _psb->_TranslateAccelerator(lpMsg, 0, DIRECTION_FORWARD_TO_CHILD))
            return S_OK;
    }
    else
    {
        IOIPAMSG(TEXT("TranslateAccelerator cannot forward to _psb"));
    }

    // SUPERCLASS has no accelerators
    return S_FALSE;
}

HRESULT CWebBrowserOC::EnableModeless(BOOL fEnable)
{
    SUPERCLASS::EnableModeless(fEnable);
    if (_psb)
    {
        return _psb->_EnableModeless(fEnable, DIRECTION_FORWARD_TO_CHILD);
    }
    else
    {
        IOIPAMSG(TEXT("EnableModeless cannot forward to _psb"));
        return S_OK;
    }
}


HRESULT CWebBrowserOC::_OnActivateChange(IOleClientSite* pActiveSite, UINT uState)
{
    HRESULT hres = SUPERCLASS::_OnActivateChange(pActiveSite, uState);

    if (SUCCEEDED(hres))
    {
        UINT uViewState;

        switch (uState)
        {
        case OC_DEACTIVE:       uViewState = SVUIA_DEACTIVATE; break;
        case OC_INPLACEACTIVE:  uViewState = SVUIA_INPLACEACTIVATE; break;
        case OC_UIACTIVE:       uViewState = SVUIA_ACTIVATE_FOCUS; break;
        default:                ASSERT(FALSE); return E_INVALIDARG;
        }

        if (_psb)
            _psb->_UIActivateView(uViewState);
    }
    else
    {
        TraceMsg(TF_SHDCONTROL, "shv _OnActivateChange failed, _psb=0x%x", _psb);
    }

    return hres;
}

void CWebBrowserOC::_OnInPlaceActivate(void)
{
    HWND       hwnd = NULL;

    SUPERCLASS::_OnInPlaceActivate();

    _RegisterWindow();

    if (_pipsite)
    {

        // we have to hold on to the target frame until we deactivate,
        // because the psbFrame will delete this thing when it shuts down
        // *before* we are actually deactivated, leaking the unadvise
        // in ieframeauto.
        ASSERT(NULL==_pTargetFramePriv);
        if (SUCCEEDED(IUnknown_QueryService(_pipsite, IID_ITargetFrame2, IID_ITargetFramePriv, (void **)&_pTargetFramePriv)))
        {
            _pTargetFramePriv->OnChildFrameActivate(SAFECAST(this, IConnectionPointContainer* ));
        }

        ASSERT(NULL==_pctContainer);
        if (FAILED(_pipsite->QueryInterface(IID_IOleCommandTarget, (void **)&_pctContainer)))
        {
            // NT 305187 discover.exe crash on exit when _pcli is NULL
            if (_pcli) 
            {
                // BUGBUG: It should be on IOleInPlaceSite,
                // but MSHTML currently has it on IOleContainer.
                LPOLECONTAINER pCont;
                if (SUCCEEDED(_pcli->GetContainer(&pCont)))
                {
                    pCont->QueryInterface(IID_IOleCommandTarget, (void **)&_pctContainer);
                    pCont->Release();
                }
            }
        }

        if (_pipframe && SUCCEEDED(_pipframe->GetWindow(&hwnd)))
        {
            // Check to see if our inplace frame is VB5.  If so, we have to hack around 
            // _pipframe->EnableModeless calls
            //
            const TCHAR VBM_THUNDER[] = TEXT("Thunder");
            TCHAR strBuf[VB_CLASSNAME_LENGTH];

            // Check to see if the inplace frame we just called is VB5's forms engine.
            //
            memset(strBuf, 0, ARRAYSIZE(strBuf));    // Clear the buffer
            GetClassName(hwnd, strBuf, (VB_CLASSNAME_LENGTH - 1));  // Get the class name of the window.
            if (StrCmpN(strBuf, VBM_THUNDER, (sizeof(VBM_THUNDER)/sizeof(TCHAR))-1) == 0)   // Is the first part "Thunder"?
            {
                _fHostedInVB5 = TRUE;
            }
        }

        {
            //
            // App compat:  Check for imagineer technical
            //

            const WCHAR STR_IMAGINEER[] = L"imagine.exe";
            WCHAR szBuff[MAX_PATH];

            if (GetModuleFileName(NULL, szBuff, ARRAYSIZE(szBuff)))
            {
                LPWSTR pszFile = PathFindFileName(szBuff);

                if (pszFile)
                {
                    _fHostedInImagineer = (0 == StrNCmpI(pszFile, STR_IMAGINEER,
                                                 ARRAYSIZE(STR_IMAGINEER) - 1));
                }
            }
        }
    }
    else
    {
        IOIPAMSG(TEXT("_OnInPlaceActivate doesn't have pipsite!"));
    }
}

void CWebBrowserOC::_OnInPlaceDeactivate(void)
{
    _UnregisterWindow();

    if (_pTargetFramePriv)
    {
        _pTargetFramePriv->OnChildFrameDeactivate(SAFECAST(this, IConnectionPointContainer*));
        ATOMICRELEASE(_pTargetFramePriv);
    }

    ATOMICRELEASE(_pctContainer);

    SUPERCLASS::_OnInPlaceDeactivate();
}


/*
 ** Ambient properties we care about
 */

// We forward IOleControl calls to the docobj.
// This helper function returns the docobj's IOleControl interface.
//
// NOTE: we may want to fail this when the WebBrowserOC was created
//       using IE3's classid just to be safe...
//
IOleControl* GetForwardingIOC(IWebBrowser* pwb)
{
    IOleControl* poc = NULL;
    IDispatch* pdisp;

    if (SUCCEEDED(pwb->get_Document(&pdisp)))
    {
        pdisp->QueryInterface(IID_IOleControl, (void **)&poc);

        pdisp->Release();
    }

    return poc;
}
STDMETHODIMP CWebBrowserOC::GetControlInfo(LPCONTROLINFO pCI)
{
    HRESULT hres = E_NOTIMPL;

    // REVIEW: What if we return some docobjects CONTROLINFO here and
    //         we navigate somewhere else. Do we have to know when this
    //         happens and tell our container that the CONTROLINFO has
    //         changed??

    IOleControl* poc = GetForwardingIOC(_pautoWB2);
    if (poc)
    {
        hres = poc->GetControlInfo(pCI);
        poc->Release();
    }

    return(hres);
}
STDMETHODIMP CWebBrowserOC::OnMnemonic(LPMSG pMsg)
{
    HRESULT hres = E_NOTIMPL;

    IOleControl* poc = GetForwardingIOC(_pautoWB2);
    if (poc)
    {
        hres = poc->OnMnemonic(pMsg);
        poc->Release();
    }

    return(hres);
}
HRESULT __stdcall CWebBrowserOC::OnAmbientPropertyChange(DISPID dispid)
{
    IS_INITIALIZED;

    // First let our base class know about the change
    //
    SUPERCLASS::OnAmbientPropertyChange(dispid);

    // Forward ambient property changes down to the docobject
    // if it is something other than offline or silent
    // for offline and silent, we call the methods to set
    // the properties so that we remember it and these 
    // methods forward downwards on their own and so we we
    // end up forwarding twice. 

    if((dispid == DISPID_AMBIENT_OFFLINEIFNOTCONNECTED) || (dispid == DISPID_AMBIENT_SILENT))
    {
        VARIANT_BOOL fAmbient;
        if (SUPERCLASS::_GetAmbientProperty(dispid, VT_BOOL, &fAmbient))
        {
            if(dispid == DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)
            {
                put_Offline(fAmbient);
            } else if(dispid == DISPID_AMBIENT_SILENT) 
            {
                put_Silent(fAmbient);
            }
        }
        // return ; // BUGBUG -- BharatS 01/20/97
        // BUGBUG --- This could be avoided if the forwarding
        // from simply calling put_offline worked fine but it does not
        // and so the second forwarding can be removed when that is fixed


    }
    IOleControl* poc = GetForwardingIOC(_pautoWB2);
    if (poc)
    {
        poc->OnAmbientPropertyChange(dispid);
        poc->Release();
    }

    return S_OK;
}
STDMETHODIMP CWebBrowserOC::FreezeEvents(BOOL bFreeze)
{
    // First let our base class know about the change
    //
    SUPERCLASS::FreezeEvents(bFreeze);

    // Forward this down to the docobject
    //
    IOleControl* poc = GetForwardingIOC(_pautoWB2);

    // delegate only if the freezeevents count is balanced when
    // you receive a FALSE. There may be FALSE calls coming in,
    // before we get a chance to delegate the pending freezeevents calls
    // TRUE is always delegated.
    if (poc && (bFreeze || (_cPendingFreezeEvents == 0)))
    {
        poc->FreezeEvents(bFreeze);
    }
    else
    {
        // keeping a total number of FreezeEvents(TRUE) calls 
        // that we have to make up for.
        if ( bFreeze )
        {
            _cPendingFreezeEvents++;
        }
        else 
        {
            // Don't let this go negative, otherwise we'll send 4 billion FreezeEvents(TRUE) to the DocObject
            // in CWebBrowserOC::_OnSetShellView. (QFE [alanau])(ferhane)
            if (EVAL(_cPendingFreezeEvents > 0))
            {
                _cPendingFreezeEvents --;
            }
        }
    }

    if (poc)
        poc->Release();

    return S_OK;
}

// CWebBrowserSB just started viewing psvNew - do our OC stuff now
//
void CWebBrowserOC::_OnSetShellView(IShellView* psvNew)
{
    _fDirty = TRUE;
    // BUGBUG: Document, Type, LocationName, LocationURL, Busy just changed...
    // PropertyChanged(DISPID_LOCATION);
    // BUGBUGBUG: that is the job of the CBaseBrowser to tell us really.
    //    ideally we would not put this here at all (or in _OnReleaseShellV...
    _SendAdvise(OBJECTCODE_DATACHANGED);    // implies OBJECTCODE_VIEWCHANGED

    // we might have received FreezeEvents(TRUE) calls from the container
    // and if the document was not ready, we might not have had a chance
    // to pass them down the stream. This is the time to make up for it.

    if (_cPendingFreezeEvents > 0)
    {
        IOleControl* poc = GetForwardingIOC(_pautoWB2);

        if (poc)
        {
            for ( ; _cPendingFreezeEvents > 0; _cPendingFreezeEvents-- )
            {
                // Forward this down to the docobject
                poc->FreezeEvents(TRUE);
            }

            poc->Release();
        }
    }
}

// CWebBrowserSB is releasing the currently viewed IShellView
void CWebBrowserOC::_OnReleaseShellView()
{
}


// *** IOleCommandTarget
//
// our version simply forwards to the object below our CWebBrowserSB
//
HRESULT CWebBrowserOC::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (_psb && _psb->_bbd._pctView)
        return _psb->_bbd._pctView->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);

    return OLECMDERR_E_UNKNOWNGROUP;
}
HRESULT CWebBrowserOC::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    if (_psb) 
    {
        if (pguidCmdGroup==NULL) 
        {
            switch(nCmdID) 
            {
            case OLECMDID_STOP:
                {
                    LPITEMIDLIST pidlIntended = (_psb->_bbd._pidlPending) ? ILClone(_psb->_bbd._pidlPending) : NULL;
                    _psb->_CancelPendingNavigation();

                    // We've just canceled the pending navigation.  We may not have a current page!  The following
                    // accomplishes two goals:
                    //
                    //  1.  Gives the user some information about the frame that couldn't navigate.
                    //  2.  Allows the browser to reach READYSTATE_COMPLETE
                    if (!_psb->_bbd._pidlCur)
                    {
                        TCHAR   szResURL[MAX_URL_STRING];

                        hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                               HINST_THISDLL,
                                               ML_CROSSCODEPAGE,
                                               TEXT("navcancl.htm"),
                                               szResURL,
                                               ARRAYSIZE(szResURL),
                                               TEXT("shdocvw.dll"));
                        if (SUCCEEDED(hr))
                        {
                            _psb->_ShowBlankPage(szResURL, pidlIntended);
                        }
                    }

                    if(pidlIntended)
                        ILFree(pidlIntended);
                    break;  // Note that we need to fall through.
                }
            case OLECMDID_ENABLE_INTERACTION:
                if (pvarargIn && pvarargIn->vt == VT_I4) 
                {
                    _psb->_fPausedByParent = BOOLIFY(pvarargIn->lVal);
                }
                break;  // Note that we need to fall through.
            }

            hr = S_OK;
            // WARNING: fall through to forward down 
        }
        if (_psb->_bbd._pctView)
            hr = _psb->_bbd._pctView->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    return hr;
}

//
//  We hit this code if the normal IOleCommandTarget chain is broken (such as
// browser band in the browser bar). In this case, we get the command target
// to the top level browser and send OLECMDID_SETDOWNLOADSTATE to it.
//
// Returns:
//      S_OK    If we found the top level browser and it processed it.
//      hresRet Otherwise
//
HRESULT CWebBrowserOC::_SetDownloadState(HRESULT hresRet, DWORD nCmdexecopt, VARIANTARG *pvarargIn)
{
    ASSERT(hresRet == OLECMDERR_E_UNKNOWNGROUP || hresRet == OLECMDERR_E_NOTSUPPORTED);

    IOleCommandTarget* pcmdtTop;
    if (_psb && SUCCEEDED(_psb->_QueryServiceParent(SID_STopLevelBrowser, IID_IOleCommandTarget, (void **)&pcmdtTop))) {
        ASSERT(pcmdtTop != _psb);
        VARIANTARG var;
        if (pvarargIn && pvarargIn->lVal) {
            ASSERT(pvarargIn->vt == VT_I4 || pvarargIn->vt == VT_BOOL || pvarargIn->vt == VT_UNKNOWN);
            //
            // Notice that we pass the pointer to this OC so that the top
            // level browser can keep track of it. This is a new behavior
            // which is different from IE 3.0. 
            //
            var.vt = VT_UNKNOWN;
            var.punkVal = SAFECAST(this, IOleCommandTarget*);
        } else {
            var.vt = VT_BOOL;
            var.lVal = FALSE;
        }
        HRESULT hresT = pcmdtTop->Exec(NULL, OLECMDID_SETDOWNLOADSTATE, nCmdexecopt, &var, NULL);

        TraceMsg(DM_FORSEARCHBAND, "WBOC::_SetDownloadState pcmdTop->Exec returned %x", hresT);

        if (SUCCEEDED(hresT)) {
            hresRet = S_OK;
        }
        pcmdtTop->Release();
    } else {
        TraceMsg(DM_FORSEARCHBAND, "WBOC::_SetDownloadState can't find the top guy");
    }

    return hresRet;
}

#if 0 // not sure if this is correct, waiting for reply from OLEPSS alias
// VB is not happy if we fail a property get, so when an IDispatch
// is not safe we return an instance of this dummy IDispatch
class CDispatchDummy : public IDispatch
{
public:
    CDispatchDummy() { m_cRef=1; DllAddRef(); };
    ~CDispatchDummy() { DllRelease(); };

    // IUnknown methods
    STDMETHOD(QueryInterface(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount) (unsigned int *pctinfo)
        { return E_FAIL; };
    STDMETHOD(GetTypeInfo) (unsigned int itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return E_FAIL; };
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, unsigned int cNames, LCID lcid, DISPID * rgdispid)
        { return E_FAIL; };
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams,VARIANT * pvarResult,EXCEPINFO * pexcepinfo,UINT * puArgErr)
        { return E_FAIL; };

protected:
    ULONG m_cRef;
};
HRESULT CDispatchDummy::QueryInterface(THIS_ REFIID riid, void ** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = SAFECAST(this, IDispatch*);
        return S_OK;
    }

    return E_NOINTERFACE;
}
ULONG CDispatchDummy::AddRef(THIS)
{
    return ++m_cRef;
}
ULONG CDispatchDummy::Release(THIS)
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CreateDummyDispatch(IDispatch** ppDisp)
{
    HRESULT hres = E_OUTOFMEMORY;

    CDispatchDummy* pdisp = new(CDispatchDummy);
    if (pdisp)
    {
        hres = pdisp->QueryInterface(IID_IDispatch, (void **)ppDisp);
        pdisp->Release();
    }

    return hres;
}
#endif


#ifdef FEATURE_FRAMES

// *** ITargetEmedding ***

HRESULT CWebBrowserOC::GetTargetFrame(ITargetFrame **ppTargetFrame)
{
    if (_psb)
    {
        return _psb->QueryServiceItsOwn(IID_ITargetFrame2, IID_ITargetFrame, (void **)ppTargetFrame);
    }
    else
    {
        *ppTargetFrame = NULL;
        return E_FAIL;
    }
}

#endif

// *** CImpIConnectionPoint override ***
CConnectionPoint* CWebBrowserOC::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    CConnectionPoint *pccp;

    // Warning: Some apps (MSDN for one) passes in IID_IDispatch and
    // expect the ole events.  So we need to be careful on which one we return.
    //
    if (fdisp && IsEqualIID(iid, IID_IDispatch))
    {
        if (_pObjectInfo->lVersion == VERSION_1)
            pccp = &m_cpWB1Events;
        else
            pccp = &m_cpEvents;
    }

    else if (IsEqualIID(iid, DIID_DWebBrowserEvents2))
    {
        pccp = &m_cpEvents;
    }
    else if (IsEqualIID(iid, DIID_DWebBrowserEvents))
    {
        pccp = &m_cpWB1Events;
    }
    else if (IsEqualIID(iid, IID_IPropertyNotifySink))
    {
        pccp = &m_cpPropNotify;
    }
    else
    {
        pccp = NULL;
    }

    return pccp;
}

STDMETHODIMP CWebBrowserOC::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 3,
                m_cpEvents.CastToIConnectionPoint(),
                m_cpWB1Events.CastToIConnectionPoint(),
                m_cpPropNotify.CastToIConnectionPoint());
}



/*
** Properties and Methods we implement
*/

// Invoke perf
//
HRESULT CWebBrowserOC::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
{
    // we should probably check that riid is IID_NULL...

    switch (dispidMember)
    {
    // handle ReadyState immediately for performance (avoid oleaut a tad longer)...
    case DISPID_READYSTATE:
        ASSERT(pdispparams && pdispparams->cArgs==0);
        if (EVAL(pvarResult) && (wFlags & DISPATCH_PROPERTYGET))
        {
            ZeroMemory(pvarResult, SIZEOF(*pvarResult));
            pvarResult->vt = VT_I4;
            return get_ReadyState((READYSTATE*)(&pvarResult->lVal));
        }
        break; // let the below invoke call give us the proper error value

    // forward these two down to our embedded object so that
    // Trident can handle cross-frame security correctly
    case DISPID_SECURITYCTX:
    case DISPID_SECURITYDOMAIN:
    {
        IDispatch* pdisp;
        if (SUCCEEDED(_pautoWB2->get_Document(&pdisp)))
        {
            HRESULT hres = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            pdisp->Release();
            return hres;
        }

        break;
    }

    default:
        // handle the default after the switch.
        break;
    }

    return CShellOcx::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

//
// Wrap around iedisp
//

#define WRAP_WB2(fn, args, nargs) \
    HRESULT CWebBrowserOC :: fn args { return _pautoWB2-> fn nargs; }
#define WRAP_WB2_DESIGN(fn, args, nargs) \
    HRESULT CWebBrowserOC :: fn args { if (_IsDesignMode()) return E_FAIL; else return _pautoWB2-> fn nargs; }

// IWebBrowser methods
//
WRAP_WB2_DESIGN(GoBack, (), ())
WRAP_WB2_DESIGN(GoForward, (), ())
WRAP_WB2_DESIGN(GoHome, (), ())
WRAP_WB2_DESIGN(GoSearch, (), ())
WRAP_WB2_DESIGN(Refresh, (), ())
WRAP_WB2_DESIGN(Refresh2, (VARIANT * Level), (Level))
WRAP_WB2_DESIGN(Stop, (), ())
WRAP_WB2(get_Type, (BSTR * pbstrType), (pbstrType))
WRAP_WB2(get_LocationName, (BSTR * pbstrLocationName), (pbstrLocationName))
WRAP_WB2(get_LocationURL, (BSTR * pbstrLocationURL), (pbstrLocationURL))
WRAP_WB2(get_Busy, (VARIANT_BOOL * pBool), (pBool))

HRESULT CWebBrowserOC::Navigate(BSTR      URL,
                        VARIANT * Flags,
                        VARIANT * TargetFrameName,
                        VARIANT * PostData,
                        VARIANT * Headers)
{
    HRESULT hr = S_OK;

    if (_dwSafetyOptions)
    {
        hr = WrapSpecialUrl(&URL);
        if (SUCCEEDED(hr))
            hr = _pautoWB2->Navigate(URL, Flags, TargetFrameName, PostData, Headers);
    }
    else
        hr = _pautoWB2->Navigate(URL, Flags, TargetFrameName, PostData, Headers);

    return hr;
}

HRESULT CWebBrowserOC::get_Application(THIS_ IDispatch **ppDisp)
{
    PROPMSG(TEXT("get_Application"));
    return QueryInterface(IID_IDispatch, (void **)ppDisp);
}
HRESULT CWebBrowserOC::get_Parent(THIS_ IDispatch **ppDisp)
{
    HRESULT hres = E_FAIL;
    PROPMSG(TEXT("get_Parent"));

    if (ppDisp)
        *ppDisp = NULL;

    if (_pcli)
    {
        IOleContainer* pContainer;

        hres = _pcli->GetContainer(&pContainer);
        if (SUCCEEDED(hres))
        {
            hres = pContainer->QueryInterface(IID_IDispatch, (void **)ppDisp);

            if (SUCCEEDED(hres) && _dwSafetyOptions)
                hres = MakeSafeForScripting((IUnknown**)ppDisp);

            pContainer->Release();
        }
        else
        {
            PROPMSG(TEXT("get_Parent couldn't find the container!"));
        }
    }
    else
    {
        PROPMSG(TEXT("get_Parent does not have _pcli!"));
    }

    // if there was an error *ppDisop is NULL, and so VB realizes 
    // this is a "nothing" dispatch -- returning failure causes 
    // error boxes to appear. ugh.

    return S_OK;
}
HRESULT CWebBrowserOC::get_Container(THIS_ IDispatch **ppDisp)
{
    // Container property is "same as parent" unless there is no parent.
    // Since we always have a parent, let get_Parent handle this.
    PROPMSG(TEXT("get_Containter passing off to get_Parent"));
    return get_Parent(ppDisp);
}
HRESULT CWebBrowserOC::get_Document(THIS_ IDispatch **ppDisp)
{
    HRESULT hres = _pautoWB2->get_Document(ppDisp);

    if (FAILED(hres) && ppDisp)
    {
        *ppDisp = NULL;
    }

    if (SUCCEEDED(hres) && _dwSafetyOptions)
        hres = MakeSafeForScripting((IUnknown**)ppDisp);


    // if there was an error *ppDisop is NULL, and so VB realizes 
    // this is a "nothing" dispatch -- returning failure causes 
    // error boxes to appear. ugh.

    return S_OK;
}
HRESULT CWebBrowserOC::get_TopLevelContainer(THIS_ VARIANT_BOOL * pBool)
{
    PROPMSG(TEXT("get_TopLevelContainer"));

    if (!pBool)
        return E_INVALIDARG;

    *pBool = FALSE;

    return S_OK;
}
HRESULT CWebBrowserOC::get_Left(THIS_ long * pl)
{
    *pl = _rcPos.left;
    return S_OK;
}
HRESULT CWebBrowserOC::put_Left(THIS_ long Left)
{
    if (_pipsite)
    {
        RECT rc = _rcPos;
        rc.left = Left;

        return(_pipsite->OnPosRectChange(&rc));
    }
    else
    {
        TraceMsg(TF_SHDCONTROL, "put_Left has no _pipsite to notify!");
        return(E_UNEXPECTED);
    }
}
HRESULT CWebBrowserOC::get_Top(THIS_ long * pl)
{
    *pl = _rcPos.top;
    return S_OK;
}
HRESULT CWebBrowserOC::put_Top(THIS_ long Top)
{
    if (_pipsite)
    {
        RECT rc = _rcPos;
        rc.top = Top;

        return(_pipsite->OnPosRectChange(&rc));
    }
    else
    {
        TraceMsg(TF_SHDCONTROL, "put_Top has no _pipsite to notify!");
        return(E_UNEXPECTED);
    }
}
HRESULT CWebBrowserOC::get_Width(THIS_ long * pl)
{
    *pl = _rcPos.right - _rcPos.left;
    return S_OK;
}
HRESULT CWebBrowserOC::put_Width(THIS_ long Width)
{
    if (_pipsite)
    {
        RECT rc = _rcPos;
        rc.right = rc.left + Width;

        return(_pipsite->OnPosRectChange(&rc));
    }
    else
    {
        TraceMsg(TF_SHDCONTROL, "put_Width has no _pipsite to notify!");
        return(E_UNEXPECTED);
    }
}
HRESULT CWebBrowserOC::get_Height(THIS_ long * pl)
{
    *pl = _rcPos.bottom - _rcPos.top;
    return S_OK;
}
HRESULT CWebBrowserOC::put_Height(THIS_ long Height)
{
    if (_pipsite)
    {
        RECT rc = _rcPos;
        rc.bottom = rc.top + Height;

        return(_pipsite->OnPosRectChange(&rc));
    }
    else
    {
        TraceMsg(TF_SHDCONTROL, "put_Height has no _pipsite to notify!");
        return(E_UNEXPECTED);
    }
}

// IWebBrowserApp methods
//
WRAP_WB2_DESIGN(PutProperty, (BSTR szProperty, VARIANT vtValue), (szProperty, vtValue))
WRAP_WB2_DESIGN(GetProperty, (BSTR szProperty, VARIANT FAR* pvtValue), (szProperty, pvtValue))
WRAP_WB2(get_FullName, (BSTR FAR* pbstrFullName), (pbstrFullName))
WRAP_WB2(get_Path, (BSTR FAR* pbstrPath), (pbstrPath))

HRESULT CWebBrowserOC::Quit()
{
    return E_FAIL;
}
HRESULT CWebBrowserOC::ClientToWindow(int FAR* pcx, int FAR* pcy)
{
    return E_FAIL;
}
HRESULT CWebBrowserOC::get_Name(BSTR FAR* pbstrName)
{
    *pbstrName = LoadBSTR(IDS_SHELLEXPLORER);
    return *pbstrName ? S_OK : E_OUTOFMEMORY;
}
HRESULT CWebBrowserOC::get_HWND(long FAR* pHWND)
{
    *pHWND = NULL;
    return E_FAIL;
}

HRESULT CWebBrowserOC::get_FullScreen(VARIANT_BOOL FAR* pBool)
{
    *pBool = _fFullScreen ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_FullScreen(VARIANT_BOOL Value)
{
    _fFullScreen = Value ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONFULLSCREEN, Value);
    return S_OK;
}
HRESULT CWebBrowserOC::get_Visible(VARIANT_BOOL FAR* pBool)
{
    *pBool = _fVisible ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_Visible(VARIANT_BOOL Value)
{
    _fVisible = Value ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONVISIBLE, Value);
    return S_OK;
}
HRESULT CWebBrowserOC::get_StatusBar(VARIANT_BOOL FAR* pBool)
{
    *pBool = (!_fNoStatusBar) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_StatusBar(VARIANT_BOOL Value)
{
    _fNoStatusBar = (!Value) ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONSTATUSBAR, Value);
    return S_OK;
}
HRESULT CWebBrowserOC::get_StatusText(BSTR FAR* pbstr)
{
    *pbstr = NULL;
    return E_FAIL;
}
HRESULT CWebBrowserOC::put_StatusText(BSTR bstr)
{
    return E_FAIL;
}
HRESULT CWebBrowserOC::get_ToolBar(int FAR* pBool)
{
    *pBool = (!_fNoToolBar) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_ToolBar(int Value)
{
    _fNoToolBar = (!Value) ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONTOOLBAR, Value);
    return S_OK;
}
HRESULT CWebBrowserOC::get_MenuBar(VARIANT_BOOL FAR* pBool)
{
    *pBool = (!_fNoMenuBar) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_MenuBar(VARIANT_BOOL Value)
{
    _fNoMenuBar = (!Value) ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONMENUBAR, Value);
    return S_OK;
}
HRESULT CWebBrowserOC::get_AddressBar(VARIANT_BOOL FAR* Value)
{
    *Value = (!_fNoAddressBar) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_AddressBar(VARIANT_BOOL Value)
{
    _fNoAddressBar = (!Value) ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONADDRESSBAR, Value);
    return S_OK;
}

// IWebBrowser2 methods
//
WRAP_WB2_DESIGN(QueryStatusWB, (OLECMDID cmdID, OLECMDF FAR* pcmdf), (cmdID, pcmdf))
WRAP_WB2_DESIGN(ShowBrowserBar, (VARIANT * pvaClsid, VARIANT FAR* pvaShow, VARIANT FAR* pvaSize), (pvaClsid, pvaShow, pvaSize))
WRAP_WB2(get_ReadyState, (READYSTATE FAR* plReadyState), (plReadyState))
WRAP_WB2(get_RegisterAsDropTarget, (VARIANT_BOOL FAR* pbRegister), (pbRegister))
WRAP_WB2(put_RegisterAsDropTarget, (VARIANT_BOOL bRegister), (bRegister))
WRAP_WB2(get_Offline, (VARIANT_BOOL FAR* pbOffline), (pbOffline))
WRAP_WB2(put_Offline, (VARIANT_BOOL bOffline), (bOffline))
WRAP_WB2(get_Silent, (VARIANT_BOOL FAR* pbSilent), (pbSilent))
WRAP_WB2(put_Silent, (VARIANT_BOOL bSilent), (bSilent))


HRESULT
CWebBrowserOC::Navigate2(VARIANT * URL,
                         VARIANT * Flags,
                         VARIANT * TargetFrameName,
                         VARIANT * PostData,
                         VARIANT * Headers)
{
    HRESULT hr = S_OK;

    if (_dwSafetyOptions && ((WORD)VT_BSTR == URL->vt) && URL->bstrVal)
    {
        hr = WrapSpecialUrl(&URL->bstrVal);
        if (SUCCEEDED(hr))
            hr = _pautoWB2->Navigate2(URL, Flags, TargetFrameName, PostData, Headers);
    }
    else
        hr = _pautoWB2->Navigate2(URL, Flags, TargetFrameName, PostData, Headers);

    return hr;
}

HRESULT 
CWebBrowserOC::ExecWB(OLECMDID      cmdID, 
                      OLECMDEXECOPT cmdexecopt, 
                      VARIANT FAR * pvaIn, 
                      VARIANT FAR * pvaOut)
{ 
    HRESULT hr = E_FAIL;

    if ( !_IsDesignMode() )
    {
        // if _dwSafetyOptions are set then we are suppossed to be 
        // running in secure mode. This means the UI-less printing should NOT
        // be honored.  This is a security issue, see ie bug 23620.
        // otherwise just let the call go through
        if ((cmdID == OLECMDID_PRINT) && _dwSafetyOptions)
        {
            // so if the UI-less- request flag is set we need to unset it.
            if (cmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER)
                cmdexecopt = OLECMDEXECOPT_DODEFAULT;
        }

        // If the optional argument pvargin is not specified make it VT_EMPTY.
        if (pvaIn && (V_VT(pvaIn) == VT_ERROR) && (V_ERROR(pvaIn) == DISP_E_PARAMNOTFOUND))
        {
            V_VT(pvaIn) = VT_EMPTY;
            V_I4(pvaIn) = 0;
        }

        // If the optional argument pvargin is not specified make it VT_EMPTY.
        if (pvaOut && (V_VT(pvaOut) == VT_ERROR) && (V_ERROR(pvaOut) == DISP_E_PARAMNOTFOUND))
        {
            V_VT(pvaOut) = VT_EMPTY;
            V_I4(pvaOut) = 0;
        }

        if (    cmdID == OLECMDID_PASTE
            ||  cmdID == OLECMDID_COPY
            ||  cmdID == OLECMDID_CUT)
        {
            BSTR bstrUrl;

            if (_dwSafetyOptions)
                return S_OK;

            if (SUCCEEDED(get_LocationURL(&bstrUrl)))
            {
                DWORD dwPolicy = 0;
                DWORD dwContext = 0;

                ZoneCheckUrlEx(bstrUrl, &dwPolicy, SIZEOF(dwPolicy), &dwContext, SIZEOF(dwContext),
                               URLACTION_SCRIPT_PASTE, 0, NULL);

                SysFreeString(bstrUrl);

                if (GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
                    return S_OK;
            }
        }

        // now pass along the call
        hr = _pautoWB2->ExecWB(cmdID, cmdexecopt, pvaIn, pvaOut); 
    }

    return hr;
}


HRESULT CWebBrowserOC::get_RegisterAsBrowser(VARIANT_BOOL FAR* pbRegister)
{
    *pbRegister = _fShouldRegisterAsBrowser ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_RegisterAsBrowser(VARIANT_BOOL bRegister)
{
    _fShouldRegisterAsBrowser = bRegister ? TRUE : FALSE;

    if (bRegister)
        _RegisterWindow();
    else
        _UnregisterWindow();

    return S_OK;
}

HRESULT CWebBrowserOC::get_TheaterMode(VARIANT_BOOL FAR* pBool)
{
    *pBool = _fTheaterMode ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}
HRESULT CWebBrowserOC::put_TheaterMode(VARIANT_BOOL Value)
{
    _fTheaterMode = Value ? TRUE : FALSE;
    FireEvent_OnAdornment(_pautoEDS, DISPID_ONTHEATERMODE, Value);
    return S_OK;
}


// IExpDispSupport
//
HRESULT CWebBrowserOC::OnTranslateAccelerator(MSG *pMsg,DWORD grfModifiers)
{
    HRESULT hr;

    hr = IUnknown_TranslateAcceleratorOCS(_pcli, pMsg, grfModifiers);
    return hr;
}

HRESULT CWebBrowserOC::OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr)
{
    HRESULT hres;

    // We get first crack at this
    hres = _pautoEDS->OnInvoke(dispidMember, iid, lcid, wFlags, pdispparams, pVarResult,pexcepinfo,puArgErr);

    // let the container get second crack at ambient properties
    //
    if(FAILED(hres))
    {
        if (!_pDispAmbient)
        {
            if (_pcli)
                _pcli->QueryInterface(IID_IDispatch, (void **)&_pDispAmbient);
        }
        if (_pDispAmbient)
        {
            hres = _pDispAmbient->Invoke(dispidMember, iid, lcid, wFlags, pdispparams, pVarResult,pexcepinfo,puArgErr);
        }
    }

   
    return(hres);
}

// IExpDispSupportOC
//
HRESULT CWebBrowserOC::OnOnControlInfoChanged()
{
    HRESULT hres = E_NOTIMPL;

    if (_pcli)
    {
        IOleControlSite* pocs;
        if (SUCCEEDED(_pcli->QueryInterface(IID_IOleControlSite, (void **)&pocs)))
        {
            hres = pocs->OnControlInfoChanged();
            pocs->Release();
        }
    }

    return(hres);
}
HRESULT CWebBrowserOC::GetDoVerbMSG(MSG *pMsg)
{
    if (_pmsgDoVerb)
    {
        *pMsg = *_pmsgDoVerb;
        return S_OK;
    }

    return E_FAIL;
}


HRESULT CWebBrowserOC::LoadHistory(IStream *pstm, IBindCtx *pbc)
{
    _InitDefault();

    ASSERT(_psb);
    HRESULT hr = _psb->LoadHistory(pstm, pbc);
    
    _OnLoaded(FAILED(hr));

    TraceMsg(TF_TRAVELLOG, "WBOC::LoadHistory pstm = %x, pbc = %x, hr = %x", pstm, pbc, hr);
    return hr;
}

HRESULT CWebBrowserOC::SaveHistory(IStream *pstm)
{
    ASSERT(_psb);
    
    TraceMsg(TF_TRAVELLOG, "WBOC::SaveHistory calling psb->SaveHistory");
    return _psb->SaveHistory(pstm);
}

HRESULT CWebBrowserOC::SetPositionCookie(DWORD dw)
{
    ASSERT(_psb);

    TraceMsg(TF_TRAVELLOG, "WBOC::SetPositionCookie calling psb->GetPositionCookie");
    return _psb->SetPositionCookie(dw);
}

HRESULT CWebBrowserOC::GetPositionCookie(DWORD *pdw)
{
    ASSERT(_psb);

    TraceMsg(TF_TRAVELLOG, "WBOC::GetPositionCookie calling psb->GetPositionCookie");
    return _psb->GetPositionCookie(pdw);
}

HMODULE CWebBrowserOC::_GetBrowseUI()
{
    if (_hBrowseUI == HMODULE_NOTLOADED)
    {
        _hBrowseUI = LoadLibrary(TEXT("browseui.dll"));
    }

    return _hBrowseUI;
}


/************************************************
**
**  CWebBrowserSB implementation
**
*/



#pragma warning(disable:4355)  // using 'this' in constructor
CWebBrowserSB::CWebBrowserSB(IUnknown* pauto, CWebBrowserOC* psvo)
        : CBASEBROWSER(NULL)
        , _psvo(psvo)
{
    TraceMsg(TF_SHDLIFE, "ctor CWebBrowserSB %x", this);
#define BUGVAL(i)   i
    _Initialize(BUGVAL(0), pauto);
}
#pragma warning(default:4355)  // using 'this' in constructor

CWebBrowserSB::~CWebBrowserSB()
{
    TraceMsg(TF_SHDLIFE, "dtor CWebBrowserSB %x", this);

   if (_ptrsite)
   {
       // Note: This isn't a normal IUnknown type of release and we don't want to
       // NULL out this pointer afterwards.  The base clase will be calling delete
       // on it.  Yes, its weird.
       _ptrsite->Release();
   }
}

HRESULT CWebBrowserSB::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = CBASEBROWSER::QueryInterface(riid, ppvObj);

    if (FAILED(hr) && (riid == IID_IIsWebBrowserSB))
        hr = CBASEBROWSER::QueryInterface(IID_IUnknown, ppvObj);

    return hr;
}


BOOL VirtualTopLevelBrowser(IOleClientSite * pcls)
{
    IOleContainer * poc;
    BOOL fNonStandard = FALSE;

    if (SUCCEEDED(pcls->GetContainer(&poc)))
    {
        ITargetContainer * ptc;

        // Is our container hosting us?
        if (SUCCEEDED(poc->QueryInterface(IID_ITargetContainer, (LPVOID*)&ptc)))
        {
            fNonStandard = TRUE;
            ptc->Release();
        }
        poc->Release();
    }

    return fNonStandard;
}

HRESULT CWebBrowserSB::SetTopBrowser()
{
    HRESULT hres = CBASEBROWSER::SetTopBrowser();

    if (_fTopBrowser && EVAL(_psvo))
        _fNoTopLevelBrowser = VirtualTopLevelBrowser(_psvo->_pcli);

    // Only create Transition Site for top level OC browsers.
    if (_ptrsite == NULL) 
        InitializeTransitionSite();
    
    return hres;
}


LRESULT CWebBrowserSB::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lret = CBASEBROWSER::WndProcBS(hwnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
    case WM_DESTROY:
        //  Because we won't get WM_NCDESTROY here, we should clean up
        // this member variable here. 
        _bbd._hwnd = NULL;
        break;
    }

    return lret;
}

// *** IOleWindow methods ***

// CBASEBROWSER maps SetStatusTextSB to SendControlMsg,
// so we don't need a CWebBrowserSB::SetStatusTextSB implementation.

HRESULT CWebBrowserSB::_EnableModeless(BOOL fEnable, BOOL fDirection)
{
    HRESULT hres = S_OK;

    CBASEBROWSER::EnableModelessSB(fEnable);

    if (fDirection == DIRECTION_FORWARD_TO_PARENT)
    {
        if (_psvo && _psvo->_pipframe)
        {
            hres = _psvo->_pipframe->EnableModeless(fEnable);
            if (!fEnable && _psvo->_fHostedInVB5)
            {
                // APPHACK: VB5 -- If we haven't put up a dialog between the EnableModelessSB(FALSE) call and this
                // call, we'll end up with a message on the queue that will disable all windows.
                // Let's peek for this message and dispatch it if it is VB5's.
                //
                #define VBM_POSTENABLEMODELESS  0x1006
                #define VBM_MAINCLASS1          TEXT("ThunderRT5Main")
                #define VBM_MAINCLASS2          TEXT("ThunderRT6Main")
                #define VBM_MAINCLASS3          TEXT("ThunderMain")

                MSG   msg;
                HWND  hwnd = NULL;
                TCHAR strBuf[15];

                if (PeekMessage( &msg, NULL, VBM_POSTENABLEMODELESS, VBM_POSTENABLEMODELESS, PM_NOREMOVE))
                {
                    /* On Win95, apparently PeekMessage can return one type of message
                     * outside the range specified: WM_QUIT.  Double-check the message we 
                     * got back.
                     */
                    if (msg.message == VBM_POSTENABLEMODELESS)
                    {
                        GetClassName(msg.hwnd, strBuf, sizeof(strBuf));
                        if (StrCmp(strBuf, VBM_MAINCLASS1) == 0 ||
                            StrCmp(strBuf, VBM_MAINCLASS2) == 0 ||
                            StrCmp(strBuf, VBM_MAINCLASS3) == 0)
                        {
                            PeekMessage( &msg, msg.hwnd, VBM_POSTENABLEMODELESS, VBM_POSTENABLEMODELESS, PM_REMOVE);
                            TraceMsg(TF_SHDCONTROL, "shv CWebBrowserSB::_EnableModeless dispatching VBM_POSTENABLEMODELESS" );
                            DispatchMessage(&msg);
                        }
                    }
                }
            }
            else
            {
                // If we're a subframe 
                //   AND the EnableModeless(FALSE) count (_cRefCannotNavigate) has become zero
                //   AND our window is disabled
                // THEN the likely scenario is that VB5 failed to reenable us.  Trace it and reenable
                //   ourselves.
                //
                if (!_fTopBrowser 
                    && _cRefCannotNavigate == 0 
                    && GetWindowLong(_bbd._hwnd, GWL_STYLE) & WS_DISABLED)
                {
                    TraceMsg(TF_WARNING, "shv Subframe was left disabled.  Reenabling ourselves.");
                    EnableWindow(_bbd._hwnd, TRUE);
                }
            }
        }
        else if (_psvo && _psvo->_pcli) 
        {
            IShellBrowser* psbParent;
            if (SUCCEEDED(IUnknown_QueryService(_psvo->_pcli, SID_SShellBrowser, IID_IShellBrowser, (LPVOID*)&psbParent))) 
            {
                psbParent->EnableModelessSB(fEnable);
                psbParent->Release();
            }
        } 
        else 
        {
            IOIPAMSG(TEXT("_EnableModeless NOT forwarding on to _pipframe"));
        }
    }
    else // DIRECTION_FORWARD_TO_CHILD
    {
        ASSERT(fDirection == DIRECTION_FORWARD_TO_CHILD);
        if (_bbd._psv)
        {
            hres = _bbd._psv->EnableModelessSV(fEnable);
        }
        else
        {
            IOIPAMSG(TEXT("_EnableModeless NOT forwarding on to _psv"));
        }
    }

    return hres;
}

HRESULT CWebBrowserSB::EnableModelessSB(BOOL fEnable)
{
    return _EnableModeless(fEnable, DIRECTION_FORWARD_TO_PARENT);
}

//
// We used to block toolbar negotiation in IE3 and IE4 beta-1, but we no
// longer do that.
//
#if 0
HRESULT CWebBrowserSB::GetBorder(LPRECT lprectBorder)
{
    // lie to them and give them lots of space
    lprectBorder->left = lprectBorder->top = 0;
    lprectBorder->right = 800;
    lprectBorder->bottom = 600;
    
    TraceMsg(TF_SHDUIACTIVATE, "UIW::GetBorder called returning=%x,%x,%x,%x",
             lprectBorder->left, lprectBorder->top, lprectBorder->right, lprectBorder->bottom);
    return S_OK;
}
#endif

HRESULT CWebBrowserSB::_TranslateAccelerator(LPMSG lpmsg, WORD wID, BOOL fDirection)
{
    HRESULT hres;

    // see if we handle it
    hres = CBASEBROWSER::TranslateAcceleratorSB(lpmsg, wID);
    if (hres == S_OK)
    {
        IOIPAMSG(TEXT("_TranslateAccelerator: CBASEBROWSER's TranslateAcceleratorSB handled it"));
    }
    else if (fDirection == DIRECTION_FORWARD_TO_PARENT)
    {
        if (_psvo && _psvo->_pipframe)
        {
            hres = _psvo->_pipframe->TranslateAccelerator(lpmsg, wID);
        }
        else
        {
            IOIPAMSG(TEXT("_TranslateAccelerator NOT forwarding on to _pipframe"));
            hres = S_FALSE;
        }
    }
    else // fDirection == DIRECTION_FORWARD_TO_CHILD
    {
        if (_bbd._psv)
        {
            hres = _bbd._psv->TranslateAccelerator(lpmsg);
        }
        else
        {
            IOIPAMSG(TEXT("_TranslateAccelerator NOT forwarding on to _psv"));
            hres = S_FALSE;
        }
    }

    return hres;
}

HRESULT CWebBrowserSB::TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)
{
    return _TranslateAccelerator(lpmsg, wID, DIRECTION_FORWARD_TO_PARENT);
}

HRESULT CWebBrowserSB::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,
            LPARAM lParam, LRESULT * pret)
{
    // let CBASEBROWSER try first so we get automation notifications working
    HRESULT hres = CBASEBROWSER::SendControlMsg(id, uMsg, wParam, lParam, pret);

    // if we're in a blocking frame, our GetControlWindow above will fail
    // causing CBASEBROWSER to fail. try to map to an IOleInPlaceFrame call.
    if (FAILED(hres) && _psvo)
    {
        // forward status bar text changes to the frame.
        if ((id == FCW_STATUS) &&
            (uMsg == SB_SETTEXT || uMsg == SB_SETTEXTW) && // trying to set status text
            (!(wParam & SBT_OWNERDRAW)) &&  // we don't own the window -- this can't work
            (((wParam & 0x00FF) == 0x00FF) || ((wParam & 0x00FF)== 0))) // simple or 0th item
        {
            WCHAR szStatusText[256];

            if (uMsg == SB_SETTEXT) {
                if (lParam)
                {
                    SHTCharToUnicode((LPTSTR)lParam, szStatusText, ARRAYSIZE(szStatusText));
                }
                else
                {
                    szStatusText[0] = L'\0';
                }

                lParam = (LPARAM) szStatusText;
            }
            else if (!lParam)
            {
                // uMsg == SB_SETTEXTW
                // Found a container that doesn't like null string pointers.  Pass an empty string instead.
                // (IE v 4.1 bug 64629)
                szStatusText[0] = 0;
                lParam = (LPARAM) szStatusText;
            }

            if (_psvo->_pipframe)
            {
                if (pret)
                {
                    *pret = 0;
                }
                hres = _psvo->_pipframe->SetStatusText((LPCOLESTR)lParam);
            }
            else
            {
                IOIPAMSG(TEXT("SetStatusTextSB NOT forwarding on to _pipframe"));
            }
        }
    }

    return hres;
}


HRESULT CWebBrowserSB::OnViewWindowActive(struct IShellView * psv)
{
    if (_psvo)
    {
        // The view is notifying us that it just went UIActive,
        // we need to update our state. OC_UIACTIVE normally
        // tells the view to UIActivate itself, but in this case
        // it already is. Avoid an infinite loop and pass FALSE.
        _psvo->_DoActivateChange(NULL, OC_UIACTIVE, FALSE);
    }

    return CBASEBROWSER::OnViewWindowActive(psv);
}


LRESULT CWebBrowserSB::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // explicitly do nothing...
    // this is just to override CBASEBROWSER's _DefWindowProc.
    return 0;
}

void CWebBrowserSB::_ViewChange(DWORD dwAspect, LONG lindex)
{
    //
    // forward this notify up to the OC's OBJECTCODE_VIEWCHANGE handler so it
    // gets forwarded on to our container.
    //
    if (_psvo)
        _psvo->_ViewChange(dwAspect, lindex);

    //
    // also let the base browser handle this view change in case nobody else is
    // handling palette messages.  most of the time this will be a nop but in
    // the case where a WM_QUERYNEWPALETTE makes it down to us this will allow
    // us to manage the palette properly.
    //
    CBASEBROWSER::_ViewChange(dwAspect, lindex);
}

HRESULT
CWebBrowserSB::ActivatePendingView()
{
    CVOCBMSG(TEXT("_ActivatePendingView"));

    // CBASEBROWSER::_ActivatePendingView will send a NavigateComplete
    // event. During this event our parent may destroy us. We then fault
    // dereferencing _psvo below. So we need to wrap this function with
    // an AddRef/Release.  (bug 15424)
    //
    AddRef();

    HRESULT hres = CBASEBROWSER::ActivatePendingView();

    if (SUCCEEDED(hres) && _psvo)
        _psvo->_OnSetShellView(_bbd._psv);

    Release();

    return hres;
}

HRESULT CWebBrowserSB::ReleaseShellView(void)
{
    CVOCBMSG(TEXT("_ReleaseShellView"));

    if (_psvo)
        _psvo->_OnReleaseShellView();

    return CBASEBROWSER::ReleaseShellView();
}


/// IBrowserService stuff
HRESULT CWebBrowserSB::GetParentSite(struct IOleInPlaceSite** ppipsite)
{
    HRESULT hres = E_FAIL;  // assume error
    *ppipsite = NULL;        // assume error

    if (_psvo)
    {
        if (_psvo->_pipsite) {
            *ppipsite = _psvo->_pipsite;
            _psvo->_pipsite->AddRef();
            hres = S_OK;
        } else if (_psvo->_pcli) {
            hres = _psvo->_pcli->QueryInterface(IID_IOleInPlaceSite, (void **)ppipsite);
        } else {
            //
            // BUGBUG: Is it expected?
            //
            TraceMsg(DM_WARNING, "CWBSB::GetParentSite called when _pcli is NULL");
        }
    }

    return hres;
}

#ifdef FEATURE_FRAMES
HRESULT CWebBrowserSB::GetOleObject(struct IOleObject** ppobjv)
{
    if (_psvo == NULL)
    {
        *ppobjv = NULL;
        return E_FAIL;
    }
    return _psvo->QueryInterface(IID_IOleObject, (void **) ppobjv);
}
#endif

HRESULT CWebBrowserSB::SetNavigateState(BNSTATE bnstate)
{
    // Do our own things (such as firing events and update _fNavigate).
    HRESULT hres = CBASEBROWSER::SetNavigateState(bnstate);

    //
    // Then, tell the container to update the download state.
    // This will start the animation if this OC is either in a frameset
    // or in a browser band.
    //
    VARIANTARG var;
    var.vt = VT_I4;
    var.lVal = _fNavigate;
    Exec(NULL, OLECMDID_SETDOWNLOADSTATE, 0, &var, NULL);

    return hres;
}


LRESULT CWebBrowserSB::OnNotify(LPNMHDR pnm)
{
    switch(pnm->code) 
    {
    case SEN_DDEEXECUTE:
    {
        IShellBrowser *psbTop;
        if (pnm->idFrom == 0 && SUCCEEDED(_QueryServiceParent(SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psbTop))) 
        {   
            HWND hwndTop;
            
            psbTop->GetWindow(&hwndTop);
            psbTop->Release();
            if (psbTop != this)
                return SendMessage(hwndTop, WM_NOTIFY, 0, (LPARAM)pnm);
        }
        break;
    }

    default:
        return CBASEBROWSER::OnNotify(pnm);
    }

    return S_OK;
}

// IServiceProvider stuff
//

HRESULT CWebBrowserSB::_QueryServiceParent(REFGUID guidService,
                                  REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;

    *ppvObj = NULL;

    // Pass it on to our parent
    if (_psvo && _psvo->_pcli)
    {
        hres = IUnknown_QueryService(_psvo->_pcli, guidService, riid, ppvObj);
    }
    else
    {
        CVOCBMSG(TEXT("QueryService doesn't have _pipsite!"));
    }

    return hres;
}

// NOTES:
//  If SID_STopLevelBrowser, go to parent (upto the top level browser)
//  If SID_SWebBrowserApp, go to parent (up to the top level browser automation)
//  If SID_SContainerDispatch, expose CWebBrowserOC (instead of _pauto)
//  Then, try CBASEBROWSER::QueryService, which will handle SID_SHlinkFrame,
//  SID_SUrlHistory, SID_SShellBrowser, etc.
//  If all fails and not SID_SShellBrowser, then we go up the parent chain.
//
HRESULT CWebBrowserSB::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL; // assume error

    //
    // If guidService is SID_STopLevelBrowser, we should not ask the super
    // class to handle. Instead, we ask the parent to handle (if any).
    //
    if (IsEqualGUID(guidService, SID_STopLevelBrowser)) {
      
//
// BUGBUG: WebBrowser is not supposed to respond to QS(SID_SInetExp) because
//  it does not support IWebBrowserApp. However, rejecting this causes
//  a stack fault when opening a frame-set. We need to find out what is causing
//  this stack fault but I'll backup this delta for now to stop GPF.
//
//   || IsEqualGUID(guidService, SID_SWebBrowserApp)
//
        goto AskParent;
    }

    if (IsEqualGUID(guidService, SID_STopFrameBrowser)) {
        BOOL fAskParent = TRUE;

        // Don't ask the parent if we are a desktop component and we are
        // not doing drag/drop.  Treat it as a top level frame.  For all other
        // cases, ask the parent.
        if (!IsEqualIID(riid, IID_IDropTarget) && _ptfrm)
        {
            LPUNKNOWN pUnkParent;
            HRESULT hres = _ptfrm->GetParentFrame(&pUnkParent);

            if (SUCCEEDED(hres))
            {
                if (pUnkParent) {
                    // Has a parent, not a desktop component
                    pUnkParent->Release();
                } else {
                    // Doesn't have a parent, must be a desktop component so
                    // fall through to call our CBASEBROWSER::QueryService
                    fAskParent = FALSE;
                }
            }
        }

        if (fAskParent)
            goto AskParent;
    }

    //
    // If the containee is asking for SID_SContainerDispatch (parent),
    // we should return the automation interface of the buddy CWebBrowserOC.
    //
    if (IsEqualGUID(guidService, SID_SContainerDispatch)) {
        if (_psvo) {
            return _psvo->QueryInterface(riid, ppvObj);
        }

        return E_UNEXPECTED;
    }

    hres = CBASEBROWSER::QueryService(guidService, riid, ppvObj);

    //
    // Notes: If guidService is SID_SShellBrowser, it indicates that
    //  the caller wants to talk to the immediate IShellBrowser.
    //  We should not try our parent in that case. Doing it so will
    //  break nested browser (frame set). In addition, notice that
    //  we don't want to go up the parent chain if hres is E_NOINTERFACE
    //  (which indicates unsuccessfull QueryInterface).
    //
    //  We don't want a shell view in a frame-set add buttons on the
    // toolbar. We should skip AskParent if guidService is SID_SExplroerToolbar. 
    //
    if (FAILED(hres) && hres!=E_NOINTERFACE
        && !IsEqualIID(guidService, SID_SShellBrowser)
        && !IsEqualIID(guidService, SID_SExplorerToolbar)
        )
    {
AskParent:
        hres = _QueryServiceParent(guidService, riid, ppvObj);

        //
        // hey, we're the top-level browser if there ain't no top-level browser
        // above us. (Such as our OC embedded in AOL/CIS/VB.) We must do this
        // or we don't get a _tlGlobal on the top frame, so navigation history
        // is screwy and the back/forward buttons are incorrect.
        //
        if (FAILED(hres) && (IsEqualGUID(guidService, SID_STopLevelBrowser) ||
              IsEqualGUID(guidService, SID_STopFrameBrowser)))
        {
            hres = CBASEBROWSER::QueryService(guidService, riid, ppvObj);
        }
    }

    return hres;
}


// *** IOleCommandTarget
//
// We simply forward to the container above our parent OC

HRESULT CWebBrowserSB::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    HRESULT hresLocal;

    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_ShellDocView))
    {
        hres = S_OK;
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            if (rgCmds[i].cmdID == SHDVID_CANGOBACK ||
                rgCmds[i].cmdID == SHDVID_CANGOFORWARD)
            {
                hresLocal = CBASEBROWSER::QueryStatus(pguidCmdGroup, 1, &rgCmds[i], pcmdtext);
            }
            else if (_psvo && _psvo->_pctContainer)
            {
                hresLocal = _psvo->_pctContainer->QueryStatus(pguidCmdGroup, 1, &rgCmds[i], pcmdtext);

                if (hresLocal == OLECMDERR_E_UNKNOWNGROUP || hresLocal == OLECMDERR_E_NOTSUPPORTED)
                {
                    hresLocal = CBASEBROWSER::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
                }
            }
            else
                hresLocal = OLECMDERR_E_NOTSUPPORTED;
            if (hresLocal != S_OK) hres = hresLocal;
        }
    } 
    else    
    {
        if (_psvo && _psvo->_pctContainer)
            hres = _psvo->_pctContainer->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);

        // if container does not support the command try base browser
        // before we only used to return an error
        if (hres == OLECMDERR_E_UNKNOWNGROUP || hres == OLECMDERR_E_NOTSUPPORTED)
        {
            hres = CBASEBROWSER::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
        }
    }

    return hres;
}

struct WBCMDLIST {
    const GUID* pguidCmdGroup;
    DWORD       nCmdID;
};

//
//  SBCMDID_CANCELNAVIGATION:                                           
//   Don't pass SBCMDID_CANCELNAVIGATION up to ROOT browser.  Canceling 
//  here doesn't need to be forwarded and will cause navigations of to  
//  initiated by javascript: navigations in this frame to be aborted.   
//                                                                      
//  SHDVID_ACTIVATEMENOW:                                               
//   The DocHost's request to activate the pending view must be handled 
//  by THIS browser, not the parent's -- if we're in a frameset page,   
//  we've probably already activated the top-level browser, now we may  
//  be trying to activate an individual frame.                          
//
const WBCMDLIST c_acmdWBSB[] = {
        { NULL, OLECMDID_HTTPEQUIV },
        { NULL, OLECMDID_HTTPEQUIV_DONE },
        { &CGID_ShellDocView, SHDVID_GETPENDINGOBJECT },
        { &CGID_ShellDocView, SHDVID_ACTIVATEMENOW },   
        { &CGID_ShellDocView, SHDVID_DOCFAMILYCHARSET },
        { &CGID_Explorer, SBCMDID_CANCELNAVIGATION },
        { &CGID_Explorer, SBCMDID_CREATESHORTCUT },     
};

HRESULT CWebBrowserSB::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    //
    //  First, test if the spcified command is supposed to processed by
    // this browser or not. If that's the case, just call CBASEBROWSER::Exec
    // and return.
    //
    for (int i=0; i<ARRAYSIZE(c_acmdWBSB); i++) {
        if (nCmdID == c_acmdWBSB[i].nCmdID) {
            if (pguidCmdGroup==NULL || c_acmdWBSB[i].pguidCmdGroup==NULL) {
                if (pguidCmdGroup==c_acmdWBSB[i].pguidCmdGroup) {
                    return CBASEBROWSER::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                }
            } else if (IsEqualGUID(*pguidCmdGroup, *c_acmdWBSB[i].pguidCmdGroup)) {
                return CBASEBROWSER::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
        }
    }

    // In some cases, we need perform some additional operations
    // before passing it to the parent. 
    //
    if (pguidCmdGroup == NULL)
    {                
        switch(nCmdID) {
        case OLECMDID_SETDOWNLOADSTATE:
            if (pvarargIn) {
                _setDescendentNavigate(pvarargIn);
            }
            break;
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CGID_ShellDocView))
    {
        switch (nCmdID) {
        /* The DocHost's request to activate the pending view must be handled
         * by THIS browser, not the parent's -- then we forward up the chain
         * so that all potentially blocked frames can attempt to deactivate
         */
        case SHDVID_DEACTIVATEMENOW:
            if (_cbScriptNesting  > 0)
                _cbScriptNesting--;
            hres = CBASEBROWSER::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            if (FAILED(hres) || _cbScriptNesting > 0)
                return hres;
            break;

        case SHDVID_NODEACTIVATENOW:
            _cbScriptNesting++;
            if (_cbScriptNesting > 1)
                return S_OK;
            break;
        case SHDVID_DELEGATEWINDOWOM:
            if (_psvo && _psvo->_pauto)
            {
                // Forward this command to the CIEFrameAuto instance for this webOC.
                return IUnknown_Exec(_psvo->_pauto, &CGID_ShellDocView, nCmdID, 0, pvarargIn, NULL);
            }
            break;
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CGID_Explorer))
    {
        //  this needs to be handled by the specific browser that 
        //  received the exec.
        switch (nCmdID)
        {
        case SBCMDID_UPDATETRAVELLOG:
        case SBCMDID_REPLACELOCATION:
            return CBASEBROWSER::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            break;
        }
    }
    else if (IsEqualGUID(CGID_ExplorerBarDoc, *pguidCmdGroup)) {
        // These are ignored so that Explorer bar changes are done once - and only in response to
        // global changes applied to top Document if frameset of browser.
        //    NOT for changes applied to frames
        //    NOT for changes applied to browserbands
        return S_OK;
    }


    //
    // Forward this EXEC to the container (if we have). 
    //
    if (_psvo && _psvo->_pctContainer)
        hres = _psvo->_pctContainer->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    //
    // if the above exec failed, we're probably in some random container
    // so let CBASEBROWSER try to simulate a top-level frame.
    // might as well be a bit paranoid and make sure it failed with "i dunno"
    //
    if (hres == OLECMDERR_E_UNKNOWNGROUP || hres == OLECMDERR_E_NOTSUPPORTED) {
        if (pguidCmdGroup==NULL && nCmdID==OLECMDID_SETDOWNLOADSTATE && _psvo) {     
            TraceMsg(DM_FORSEARCHBAND, "WBSB::QueryStatus Container does not support OLECMDID_SETDOWNLOADSTATE");
            hres = _psvo->_SetDownloadState(hres, nCmdexecopt, pvarargIn);
        }

        if (hres!=S_OK) {
            hres = CBASEBROWSER::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
    }

    return hres;
}


HRESULT CWebBrowserSB::_SwitchActivationNow()
{
    CBASEBROWSER::_SwitchActivationNow();

    if (_bbd._psv && 
        _psvo && 
        _psvo->_nActivate < OC_INPLACEACTIVE && 
        _psvo->_dwDrawAspect)
    {
        // pass on the SetExtent to the now ready browser...
        IPrivateOleObject * pPrivOle;
        if ( SUCCEEDED(_bbd._psv->QueryInterface(IID_IPrivateOleObject, (void **) &pPrivOle )))
        {
            // we have an ole object, delegate downwards...
            pPrivOle->SetExtent( _psvo->_dwDrawAspect, &_psvo->_sizeHIM );
            pPrivOle->Release();
        }
    }
    return S_OK;
}

BOOL CWebBrowserSB::_HeyMoe_IsWiseGuy()
{
    BOOL fRet;

    if (_psvo)
    {
        fRet = _psvo->_HeyMoe_IsWiseGuy();
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

