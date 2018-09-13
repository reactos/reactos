//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: olecontrol.cpp
//
// History:
//         7-31-96  by dli
//------------------------------------------------------------------------

#include "priv.h"

class COleControlHost;

//---------------------------------------------------------------------------
// Event sink
class CEventSink : public IDispatch
//---------------------------------------------------------------------------
{
public:
    CEventSink( BOOL bAutoDelete = FALSE ) ;

    //  Connect/disconnect
    BOOL  Connect( HWND hwndOwner, HWND hwndSite, LPUNKNOWN punkOC ) ;
    BOOL  Disconnect() ;

//  IUnknown methods
    STDMETHOD (QueryInterface)( REFIID riid, void** ppvObj ) ;
    STDMETHOD_(ULONG, AddRef)() ;
    STDMETHOD_(ULONG, Release)() ;

//  IDispatch methods
    STDMETHOD (GetTypeInfoCount)( UINT *pctinfo )
        { return E_NOTIMPL ; }

    STDMETHOD (GetTypeInfo)( UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo )
        { return E_NOTIMPL ; }

    STDMETHOD (GetIDsOfNames)( REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                                LCID lcid, DISPID *rgDispId )
        { return E_NOTIMPL ; }

    STDMETHOD (Invoke)( 
        IN DISPID dispIdMember,
        IN REFIID riid,
        IN LCID lcid,
        IN WORD wFlags,
        IN OUT DISPPARAMS *pDispParams,
        OUT VARIANT *pVarResult,
        OUT EXCEPINFO *pExcepInfo,
        OUT UINT *puArgErr) ;

private:
    static HRESULT _GetDefaultEventIID( LPUNKNOWN punkOC, IID* piid ) ;
    BOOL           _Connect( HWND hwndOwner, HWND hwndSite, LPUNKNOWN punkOC, REFIID iid ) ;
    BOOL           _IsConnected( REFIID iid ) ;

    ULONG       _dwCookie ;   // connection cookie
    IID         _iid ;        // connection interface
    IID         _iidDefault ; // OC's default event dispatch interface
    LPUNKNOWN   _punkOC ;     // OC's unknown
    LONG        _cRef ;       // ref count
    HWND        _hwndSite,    // 
                _hwndOwner ;
    BOOL        _bAutoDelete ;
} ;

class CProxyUIHandler : 
    public IDocHostUIHandler
{
public:
    
    // *** IUnknown methods *** 
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // *** IDocHostUIHandler methods *** 
    virtual STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
    virtual STDMETHODIMP GetHostInfo(DOCHOSTUIINFO *pInfo);
    virtual STDMETHODIMP ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
    virtual STDMETHODIMP HideUI();
    virtual STDMETHODIMP UpdateUI();
    virtual STDMETHODIMP EnableModeless(BOOL fActivate);
    virtual STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
    virtual STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
    virtual STDMETHODIMP ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
    virtual STDMETHODIMP TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
    virtual STDMETHODIMP GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw);
    virtual STDMETHODIMP GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
    virtual STDMETHODIMP GetExternal(IDispatch **ppDispatch);
    virtual STDMETHODIMP TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    virtual STDMETHODIMP FilterDataObject( IDataObject *pDO, IDataObject **ppDORet);
};

//---------------------------------------------------------------------------
//  Ole control container object
class COleControlHost : 
        public IOleClientSite,
        public IAdviseSink,
        public IOleInPlaceSite,
        public IOleInPlaceFrame,
        public IServiceProvider,
        public IOleCommandTarget
{
friend CProxyUIHandler;

protected:
    static LRESULT CALLBACK OCHostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT _Draw(HDC hdc);
    HRESULT _PersistInit();
    HRESULT _Init();
    HRESULT _Activate();
    HRESULT _Deactivate();
    HRESULT _DoVerb(long iVerb, LPMSG lpMsg);
    HRESULT _Exit();
    HRESULT _InitOCStruct(LPOCHINITSTRUCT lpocs);
    LRESULT _OnPaint();
    LRESULT _OnSize(HWND hwnd, LPARAM lParam);
    LRESULT _OnCreate(HWND hwnd, LPCREATESTRUCT);
    LRESULT _OnDestroy();
    LRESULT _OnQueryInterface(WPARAM wParam, LPARAM lParam);
    LRESULT _SetOwner(IUnknown * punkOwner);
    LRESULT _ConnectEvents( LPUNKNOWN punkOC, BOOL bConnect ) ;
    LRESULT _SendNotify(UINT code, LPNMHDR pnmhdr);
    
    // IUnknown 
    UINT _cRef;
    
    DWORD _dwAspect;
    DWORD _dwMiscStatus;    // OLE misc status 
    DWORD _dwConnection;    // Token for Advisory connections
   
    BOOL _bInPlaceActive;   // Flag indicating if the OC is in place active
        
    HWND _hwnd;
    HWND _hwndParent;
    CLSID _clsidOC;
   
    IUnknown *_punkOC;
    IViewObject *_pIViewObject; 
    IOleObject *_pIOleObject;
    IOleInPlaceObject *_pIOleIPObject;

    IUnknown *_punkOwner;
    CEventSink  _eventSink ;
    CProxyUIHandler     _xuih;
    IDocHostUIHandler *_pIDocHostUIParent;

public:
    COleControlHost(HWND hwnd);

    static void _RegisterClass();

    // *** IUnknown methods *** 
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);    
    
    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);
    
    // *** IOleClientSite methods *** 
    STDMETHOD (SaveObject)();
    STDMETHOD (GetMoniker)(DWORD, DWORD, LPMONIKER *);
    STDMETHOD (GetContainer)(LPOLECONTAINER *);
    STDMETHOD (ShowObject)();
    STDMETHOD (OnShowWindow)(BOOL);
    STDMETHOD (RequestNewObjectLayout)();
    
    // *** IAdviseSink methods *** 
    STDMETHOD_(void,OnDataChange)(FORMATETC *, STGMEDIUM *);
    STDMETHOD_(void,OnViewChange)(DWORD, LONG);
    STDMETHOD_(void,OnRename)(LPMONIKER);
    STDMETHOD_(void,OnSave)();
    STDMETHOD_(void,OnClose)();
    
    // *** IOleWindow Methods ***
    STDMETHOD (GetWindow) (HWND * phwnd);
    STDMETHOD (ContextSensitiveHelp) (BOOL fEnterMode);
    
    // *** IOleInPlaceSite Methods *** 
    STDMETHOD (CanInPlaceActivate) (void);
    STDMETHOD (OnInPlaceActivate) (void);
    STDMETHOD (OnUIActivate) (void);
    STDMETHOD (GetWindowContext) (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD (Scroll) (SIZE scrollExtent);
    STDMETHOD (OnUIDeactivate) (BOOL fUndoable);
    STDMETHOD (OnInPlaceDeactivate) (void);
    STDMETHOD (DiscardUndoState) (void);
    STDMETHOD (DeactivateAndUndo) (void);
    STDMETHOD (OnPosRectChange) (LPCRECT lprcPosRect); 

    // IOleInPlaceUIWindow methods.
    STDMETHOD (GetBorder)(LPRECT lprectBorder);
    STDMETHOD (RequestBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD (SetBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD (SetActiveObject)(IOleInPlaceActiveObject * pActiveObject,
                                LPCOLESTR lpszObjName);

    // IOleInPlaceFrame methods
    STDMETHOD (InsertMenus)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD (SetMenu)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD (RemoveMenus)(HMENU hmenuShared);
    STDMETHOD (SetStatusText)(LPCOLESTR pszStatusText);
    STDMETHOD (EnableModeless)(BOOL fEnable);
    STDMETHOD (TranslateAccelerator)(LPMSG lpmsg, WORD wID);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguid, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguid, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
};


HRESULT COleControlHost::_Draw(HDC hdc)
{
    HRESULT hr = E_FAIL;
        
    if (_hwnd && _punkOC && !_bInPlaceActive)
    {
        RECT rc;
        GetClientRect(_hwnd, &rc);
        hr = OleDraw(_punkOC, _dwAspect, hdc, &rc);
    }
    return(hr);
}

HRESULT COleControlHost::_PersistInit()
{
    IPersistStreamInit * pIPersistStreamInit;

    if (_SendNotify(OCN_PERSISTINIT, NULL) == OCNPERSISTINIT_HANDLED)
        return S_FALSE;
    
    HRESULT hr = _punkOC->QueryInterface(IID_IPersistStreamInit, (void **)&pIPersistStreamInit);
    if (SUCCEEDED(hr))
    {
        hr = pIPersistStreamInit->InitNew();
        pIPersistStreamInit->Release();
    }
    else    
    {
        IPersistStorage * pIPersistStorage;
        hr = _punkOC->QueryInterface(IID_IPersistStorage, (void **)&pIPersistStorage);
        if (SUCCEEDED(hr))
        {
            // Create a zero sized ILockBytes.
            ILockBytes *pILockBytes;
            hr = CreateILockBytesOnHGlobal(NULL, TRUE, &pILockBytes);
            if (SUCCEEDED(hr)) {
                // Use the ILockBytes to create a storage.
                IStorage    *pIStorage;
                hr = StgCreateDocfileOnILockBytes(pILockBytes,
                                                  STGM_CREATE |
                                                  STGM_READWRITE |
                                                  STGM_SHARE_EXCLUSIVE,
                                                  0, &pIStorage);
                if (SUCCEEDED(hr)) {
                    // Call InitNew to initialize the object.
                    hr = pIPersistStorage->InitNew(pIStorage);
                    // Clean up
                    pIStorage->Release();
                } // IStorage
                pILockBytes->Release();
            } // ILockBytes
            pIPersistStorage->Release();
        }   
    }
    return hr;
}

HRESULT COleControlHost::_Init()
{
    HRESULT hr = E_FAIL;

    OCNCOCREATEMSG ocm = {0};
    ocm.clsidOC = _clsidOC;
    ocm.ppunk = &_punkOC;
    if(_SendNotify(OCN_COCREATEINSTANCE, &ocm.nmhdr) != OCNCOCREATE_HANDLED)
    {
        hr = CoCreateInstance(_clsidOC, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 
                                  IID_IUnknown, (LPVOID *)&_punkOC);
        if (FAILED(hr))
        {
            TraceMsg(TF_OCCONTROL, "_Init: Unable to CoCreateInstance this Class ID -- hr = %lX -- hr = %lX", _clsidOC, hr);
            return hr;
        }
        
    }
    
    ASSERT(_punkOC != NULL);
        
    if (_punkOC == NULL)
        return E_FAIL;
    
    hr = _punkOC->QueryInterface(IID_IOleObject, (void **)&_pIOleObject);    
    if (FAILED(hr))
    {
        TraceMsg(TF_OCCONTROL, "_Init: Unable to QueryInterface IOleObject -- hr = %s", hr);
        return hr;
    }

    hr = _pIOleObject->GetMiscStatus(_dwAspect, &_dwMiscStatus);

    // Set the inplace active flag here
    // If this fails, we will assume that we can setclientsite later

    if (_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
    {   
        hr = _pIOleObject->SetClientSite(this);
        _PersistInit();
    }
    else
    {
        _PersistInit();
        hr = _pIOleObject->SetClientSite(this);
    }
    
    if (FAILED(hr))
    {
        TraceMsg(TF_OCCONTROL, "_Init: Unable to set client site -- hr = %lX", hr);
        return hr;
    }
    
    
    if (SUCCEEDED(_punkOC->QueryInterface(IID_IViewObject, (void **)&_pIViewObject)))
    {    
        _pIViewObject->SetAdvise(_dwAspect, 0, this);
    }
    
    //BUGBUG: this is not really useful because we do not handle the cases, yet 
    _pIOleObject->Advise(this, &_dwConnection);
    
    _pIOleObject->SetHostNames(TEXTW("OC Host Window"), TEXTW("OC Host Window"));
    
    return S_OK;
}

// 
HRESULT COleControlHost::_Activate()
{
    HRESULT hr = E_FAIL;
    
    RECT rcClient;
    ASSERT(_hwnd);
    
    _SendNotify(OCN_ACTIVATE, NULL);
    
    if (!GetClientRect(_hwnd, &rcClient))
        SetRectEmpty(&rcClient);
    
    hr = _pIOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, _hwnd, &rcClient);
    
    if (SUCCEEDED(hr))
        _bInPlaceActive = TRUE;
    
    // Calling second DoVerb with OLEIVERB_SHOW because:
    // 1. If the above DoVerb fails, this is a back up activation call
    // 2. If the above DoVerb succeeds, this is also necessary because 
    //    Some embeddings needs to be explicitly told to show themselves.
    
    if (!(_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME)) 
        hr = _pIOleObject->DoVerb(OLEIVERB_SHOW, NULL, this, 0, _hwnd, &rcClient);      
    
    if (FAILED(hr))
        TraceMsg(TF_OCCONTROL, "_Activate: %d Unable to DoVerb! Error = %lX", _bInPlaceActive, hr);

    return hr;
}

HRESULT COleControlHost::_Deactivate()
{
    _SendNotify(OCN_DEACTIVATE, NULL);
    if (_pIOleIPObject)
    {
        _pIOleIPObject->InPlaceDeactivate();
        // Should be set to NULL by the above function call
        ASSERT(_pIOleIPObject == NULL);
        
        return S_OK;
    }
    
    return S_FALSE;
}

HRESULT COleControlHost::_DoVerb(long iVerb, LPMSG lpMsg)
{
    HRESULT hr = E_FAIL;
    
    RECT rcClient;
    ASSERT(_hwnd && IsWindow(_hwnd));
    // simply because others haven't been tested
    ASSERT(iVerb == OLEIVERB_UIACTIVATE || iVerb == OLEIVERB_INPLACEACTIVATE);
    
    if (!GetClientRect(_hwnd, &rcClient))
        SetRectEmpty(&rcClient);
    
    hr = _pIOleObject->DoVerb(iVerb, lpMsg, this, 0, _hwnd, &rcClient);
    
    if (SUCCEEDED(hr))
        _bInPlaceActive = TRUE;
    
#if 0 // we'll count on DocHost::DoVerb to do this if needed (or our caller?)
    // BUGBUG note that DocHost does this always (no OLEMISC_* check)
    if (iVerb == OLEIVERB_INPLACEACTIVATE) {
        // Calling second DoVerb with OLEIVERB_SHOW because:
        // 1. If the above DoVerb fails, this is a back up activation call
        // 2. If the above DoVerb succeeds, this is also necessary because 
        //    Some embeddings needs to be explicitly told to show themselves.
        
        if (!(_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME)) 
            hr = _pIOleObject->DoVerb(OLEIVERB_SHOW, lpMsg, this, 0, _hwnd, &rcClient);      
    }
#endif
    
    if (FAILED(hr))
        TraceMsg(TF_OCCONTROL, "_Activate: %d Unable to DoVerb! Error = %lX", _bInPlaceActive, hr);

    return hr;
}

// Clean up and Release all of interface pointers used in this object
HRESULT COleControlHost::_Exit()
{
    _SendNotify(OCN_EXIT, NULL);
    if (_pIViewObject)
    {
        _pIViewObject->SetAdvise(_dwAspect, 0, NULL);
        _pIViewObject->Release();
        _pIViewObject = NULL;
    }
    
    if (_pIOleObject)
    {
        if (_dwConnection)
        {
            _pIOleObject->Unadvise(_dwConnection);
            _dwConnection = 0;
        }
        
        _pIOleObject->Close(OLECLOSE_NOSAVE);
        _pIOleObject->SetClientSite(NULL);
        _pIOleObject->Release();
        _pIOleObject = NULL;
    }

    if (_punkOC)
    {
        ULONG ulRef;
        ulRef = _punkOC->Release();
        _punkOC = NULL;
        if (ulRef != 0)
            TraceMsg(TF_OCCONTROL, "OCHOST _Exit: After last release ref = %d > 0", ulRef);
    }
    
    ATOMICRELEASE(_pIDocHostUIParent);

    if (_punkOwner) {
        _punkOwner->Release();
        _punkOwner = NULL;
    }
        
        
    return S_OK;
}

COleControlHost::COleControlHost(HWND hwnd)
    : _cRef(1), _dwAspect(DVASPECT_CONTENT), _hwnd(hwnd)
{
    // These variables should be initialized to zeros automatically
    ASSERT(_dwMiscStatus == 0);
    ASSERT(_dwConnection == 0);
    ASSERT(_bInPlaceActive == FALSE);
    ASSERT(_pIDocHostUIParent == NULL);
    ASSERT(_clsidOC == CLSID_NULL);
    ASSERT(_punkOC == NULL);
    ASSERT(_pIViewObject == NULL);
    ASSERT(_pIOleIPObject == NULL);

    ASSERT(_hwnd);
    
}


#ifdef DEBUG
#define _AddRef(psz) { ++_cRef; TraceMsg(TF_OCCONTROL, "CDocObjectHost(%x)::QI(%s) is AddRefing _cRef=%lX", this, psz, _cRef); }
#else
#define _AddRef(psz)    ++_cRef
#endif

// *** IUnknown Methods ***

HRESULT COleControlHost::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);

    if (ppvObj == NULL)
        return E_INVALIDARG;
    
    *ppvObj = NULL;

    if ((IsEqualIID(riid, IID_IUnknown)) ||
        (IsEqualIID(riid, IID_IOleWindow)) || 
        (IsEqualIID(riid, IID_IOleInPlaceUIWindow)) || 
        (IsEqualIID(riid, IID_IOleInPlaceFrame)))
    {
        *ppvObj = SAFECAST(this, IOleInPlaceFrame *);
        TraceMsg(TF_OCCONTROL, "QI IOleInPlaceFrame succeeded");
    }
    else if (IsEqualIID(riid, IID_IServiceProvider)) 
    {
        *ppvObj = SAFECAST(this, IServiceProvider *);
        TraceMsg(TF_OCCONTROL, "QI IServiceProvider succeeded");
    }
        
    else if (IsEqualIID(riid, IID_IOleClientSite))
    {
        *ppvObj = SAFECAST(this, IOleClientSite *);
        TraceMsg(TF_OCCONTROL, "QI IOleClientSite succeeded");
    }
    else if (IsEqualIID(riid, IID_IAdviseSink))
    {
        *ppvObj = SAFECAST(this, IAdviseSink *);
        TraceMsg(TF_OCCONTROL, "QI IAdviseSink succeeded");
    }
    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
    {
        *ppvObj = SAFECAST(this, IOleInPlaceSite *);
        TraceMsg(TF_OCCONTROL, "QI IOleInPlaceSite succeeded");
    } 
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = SAFECAST(this, IOleCommandTarget *);
        TraceMsg(TF_OCCONTROL, "QI IOleCommandTarget succeeded");
    }
    else if (NULL != _pIDocHostUIParent  && 
            IsEqualIID(riid, IID_IDocHostUIHandler))
    {
        // only implement this if the host implements it
        *ppvObj = SAFECAST(&_xuih, IDocHostUIHandler *);
        TraceMsg(TF_OCCONTROL, "QI IDocHostUIHandler succeeded");
    }
    
    else
        return E_NOINTERFACE;  // Otherwise, don't delegate to HTMLObj!!
     
    
    _AddRef(TEXT("IOleInPlaceSite"));
    return S_OK;
}


ULONG COleControlHost::AddRef()
{
    _cRef++;
    TraceMsg(TF_OCCONTROL, "COleControlHost(%x)::AddRef called, new _cRef=%lX", this, _cRef);
    return _cRef;
}

ULONG COleControlHost::Release()
{
    _cRef--;
    TraceMsg(TF_OCCONTROL, "COleControlHost(%x)::Release called, new _cRef=%lX", this, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// ServiceProvider interfaces
HRESULT COleControlHost::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL;
    
    if (_punkOwner) {
        IServiceProvider *psp;
        
        _punkOwner->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
        if (psp) {
            hres = psp->QueryService(guidService, riid, ppvObj);
            psp->Release();
        }
    }
    
    return hres;
}

// ************************ IOleClientSite methods ****************** 

HRESULT COleControlHost::SaveObject()
{
    //BUGBUG: default set to E_NOTIMPL may not be correct
    HRESULT hr = E_NOTIMPL;
    
    IStorage * pIs;
    if (SUCCEEDED(_punkOC->QueryInterface(IID_IStorage, (void **)&pIs)))
    {
        IPersistStorage *pIps;
        if (SUCCEEDED(_punkOC->QueryInterface(IID_IPersistStorage, (void **)&pIps)))
        {
            OleSave(pIps, pIs, TRUE);
            pIps->SaveCompleted(NULL);
            pIps->Release();
            hr = S_OK;
        }
        pIs->Release();
    }
    
    return hr;   
}

HRESULT COleControlHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppMk)
{
     return E_NOTIMPL;   
}

HRESULT COleControlHost::GetContainer(LPOLECONTAINER * ppContainer)
{
    *ppContainer = NULL;       
    return E_NOINTERFACE;
}

HRESULT COleControlHost::ShowObject()
{
//    RECTL rcl;
//    POINT pt1, pt2;
    
    return S_OK;   
}

HRESULT COleControlHost::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

HRESULT COleControlHost::RequestNewObjectLayout()
{
     return E_NOTIMPL;   
}

// ************************ IAdviseSink methods ********************* 
void COleControlHost::OnDataChange(FORMATETC * pFmt, STGMEDIUM * pStgMed)
{
    // NOTES: This is optional
    return;   
}
    
void COleControlHost::OnViewChange(DWORD dwAspect, LONG lIndex)
{
    // BUGBUG: need to let the container know the colors might have changed
    // but don't want to deal with the paletts now

    // Draw only if not inplace active and this is the right aspect.  Inplace
    // active objects have their own window and are responsible for painting
    // themselves.
    
    // BUGBUG: _bInPlaceActive is not determined, yet. 
    // This funtion is called as a result of calling doverb, however, 
    // _bInPlaceActive will only be determined as DoVerb returns
    // works fine for now, but could be trouble later. 
    if ((_hwnd) && (!_bInPlaceActive) && (dwAspect == _dwAspect))
    {
        HDC hdc = GetDC(_hwnd);
        _Draw(hdc);
        ReleaseDC(_hwnd, hdc);
    }
}

void COleControlHost::OnRename(LPMONIKER pMoniker)
{
    return;   
}

void COleControlHost::OnSave()
{
    // NOTES: This is optional
    return;   
}

void COleControlHost::OnClose()
{
    // BUGBUG: need to let the container know the colors might have changed
    return;   
}

// ************************ IOleWindow Methods ********************** 
HRESULT COleControlHost::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;
    return S_OK;
}

HRESULT COleControlHost::ContextSensitiveHelp(BOOL fEnterMode)
{
    // NOTES: This is optional
    return E_NOTIMPL;   
}

// *********************** IOleInPlaceSite Methods *****************
HRESULT COleControlHost::CanInPlaceActivate(void)
{
    return S_OK;   
}

HRESULT COleControlHost::OnInPlaceActivate(void)
{
    if (!_pIOleIPObject)
        return (_punkOC->QueryInterface(IID_IOleInPlaceObject, (void **)&_pIOleIPObject));    
    else
        return S_OK;
}


HRESULT COleControlHost::OnUIActivate(void)
{
    LRESULT lres;
    OCNONUIACTIVATEMSG oam = {0};

    oam.punk = _punkOC;

    lres = _SendNotify(OCN_ONUIACTIVATE, &oam.nmhdr);
    return S_OK;
}

HRESULT COleControlHost::GetWindowContext (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppIIPUIWin, 
                                           LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *ppFrame = this;
    _AddRef("GetWindowContext");
        
    // This is set to NULL because the document window is the same as the frame 
    // window
    *ppIIPUIWin = NULL;
    
    ASSERT(_hwnd);
    if (!GetClientRect(_hwnd, lprcPosRect))
        SetRectEmpty(lprcPosRect);
    
    // Set the clip rectangle to be the same as the position rectangle
    
    CopyRect(lprcClipRect, lprcPosRect);
        
    lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
    
#ifdef MDI
    lpFrameInfo->fMDIApp = TRUE;
#else
    lpFrameInfo->fMDIApp = FALSE;
#endif
    lpFrameInfo->hwndFrame = _hwnd;
    lpFrameInfo->haccel = 0;
    lpFrameInfo->cAccelEntries = 0;
    return S_OK;
}

HRESULT COleControlHost::Scroll(SIZE scrollExtent)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::OnUIDeactivate(BOOL fUndoable)
{
    
    return E_NOTIMPL;   
}


HRESULT COleControlHost::OnInPlaceDeactivate(void)
{
    if (_pIOleIPObject)
    {
        _pIOleIPObject->Release();
        _pIOleIPObject = NULL;
    }
    
    return S_OK;
}

HRESULT COleControlHost::DiscardUndoState(void)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::DeactivateAndUndo(void)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::OnPosRectChange(LPCRECT lprcPosRect) 
{
    // We do not allow the children to change the size themselves
    OCNONPOSRECTCHANGEMSG opcm = {0};
    opcm.prcPosRect = lprcPosRect;
    _SendNotify(OCN_ONPOSRECTCHANGE, &opcm.nmhdr);
    return S_OK;
}
// ************************ IOleInPlaceUIWindow methods *************

HRESULT COleControlHost::GetBorder(LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

HRESULT COleControlHost::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT COleControlHost::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT COleControlHost::SetActiveObject(IOleInPlaceActiveObject * pActiveObject,
                                LPCOLESTR lpszObjName)
{
    return E_NOTIMPL;
}

// *********************** IOleInPlaceFrame Methods *****************
HRESULT COleControlHost::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::RemoveMenus(HMENU hmenuShared)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::SetStatusText(LPCOLESTR pszStatusText)
{
    OCNONSETSTATUSTEXTMSG osst = {0};
    osst.pwszStatusText = pszStatusText;
    _SendNotify(OCN_ONSETSTATUSTEXT, &osst.nmhdr);
    return S_OK;
}

HRESULT COleControlHost::EnableModeless(BOOL fEnable)
{
    // Should implement later
    return E_NOTIMPL;   
}

HRESULT COleControlHost::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    // Should implement later
    return E_NOTIMPL;   
}

// ************************ IOleCommandTarget Methods *************
HRESULT COleControlHost::QueryStatus(const GUID *pguid, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT *pcmdtext)
{
    return IUnknown_QueryStatus(_punkOwner, pguid, cCmds, rgCmds, pcmdtext);
}

HRESULT COleControlHost::Exec(const GUID *pguid, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return IUnknown_Exec(_punkOwner, pguid, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

HRESULT COleControlHost::_InitOCStruct(LPOCHINITSTRUCT lpocs)
{               
    HRESULT hres = E_FAIL;

    if (_punkOC)
        return S_FALSE;
    
    if (lpocs)
    {
        if (lpocs->cbSize != SIZEOF(OCHINITSTRUCT))
            return hres;

        if (lpocs->clsidOC == CLSID_NULL)
            return hres;

        _clsidOC = lpocs->clsidOC;
        _SetOwner(lpocs->punkOwner);
    }
    else 
        return hres;

    hres = _Init();
    if (SUCCEEDED(hres))
        hres = _Activate();    
    
    return hres;
}

LRESULT COleControlHost::_OnPaint()
{
    ASSERT(_hwnd);

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(_hwnd, &ps);    
    _Draw(hdc);
    EndPaint(_hwnd, &ps);
    return 0;
}

LRESULT COleControlHost::_OnSize(HWND hwnd, LPARAM lParam)
{
    if (_pIOleIPObject)
    {
        RECT rcPos, rcClip ;
        SetRect( &rcPos, 0, 0, LOWORD(lParam), HIWORD(lParam) ) ;
        rcClip = rcPos ;
        _pIOleIPObject->SetObjectRects(&rcPos, &rcClip);
    }
    return 0;
}

LRESULT COleControlHost::_OnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{   
    TCHAR szClsid[50];
    _hwndParent = GetParent(hwnd);
    SetWindowLongPtr(hwnd, 0, (LONG_PTR)this);
    
    LPOCHINITSTRUCT lpois = (LPOCHINITSTRUCT)lpcs->lpCreateParams;
    HRESULT hres = S_OK;
        
    if (lpois)
        hres = _InitOCStruct(lpois);
    else if (GetWindowText(hwnd, szClsid, ARRAYSIZE(szClsid)))
    {
        OCHINITSTRUCT ois;
        ois.cbSize = SIZEOF(OCHINITSTRUCT);
        if (FAILED(SHCLSIDFromString(szClsid, &ois.clsidOC)))
            ois.clsidOC = CLSID_NULL;
        ois.punkOwner = NULL;
        
        hres = _InitOCStruct(&ois);
    }
    
    if (FAILED(hres))
        return -1;
    return 0;
}

LRESULT COleControlHost::_OnDestroy()
{    
    ASSERT(_hwnd);
    SetWindowLongPtr(_hwnd, 0, 0);
    _ConnectEvents( _punkOC, FALSE ) ;
    _Deactivate();
    _Exit();
    Release();
   
    return 0;
}

LRESULT COleControlHost::_OnQueryInterface(WPARAM wParam, LPARAM lParam)
{
    if (lParam)
    {
        QIMSG * qiMsg = (QIMSG *)lParam;
        return _punkOC->QueryInterface(*qiMsg->qiid, qiMsg->ppvObject);
    }
    return -1;
}

LRESULT COleControlHost::_SetOwner(IUnknown * punkNewOwner)
{
    if (_punkOwner)
        _punkOwner->Release();
    _punkOwner = punkNewOwner;
    if (_punkOwner)
        _punkOwner->AddRef();

    ATOMICRELEASE(_pIDocHostUIParent);

    // Query if owner supports IDocHostUIHandler, if so then
    // we turn on our delegating wrapper
    if (punkNewOwner)
        punkNewOwner->QueryInterface(IID_IDocHostUIHandler, (LPVOID *)&_pIDocHostUIParent);

    return 0;
}

LRESULT COleControlHost::_ConnectEvents( LPUNKNOWN punkOC, BOOL bConnect )
{
    if( bConnect )
    {
        ASSERT( punkOC ) ;
        return _eventSink.Connect( _hwndParent, _hwnd, punkOC ) ;
    }
    return _eventSink.Disconnect() ;
}

LRESULT COleControlHost::_SendNotify(UINT code, LPNMHDR pnmhdr)
{
    NMHDR nmhdr;
    ASSERT(_hwnd);

    if (!_hwndParent)
        return 0;
   
    if (!pnmhdr)
        pnmhdr = &nmhdr;
    pnmhdr->hwndFrom = _hwnd;  
    pnmhdr->idFrom = GetDlgCtrlID( _hwnd ) ;
    pnmhdr->code = code;
    
    return SendMessage(_hwndParent, WM_NOTIFY, 0, (LPARAM)pnmhdr);
}



LRESULT CALLBACK COleControlHost::OCHostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COleControlHost *pcoch = (COleControlHost *)GetWindowPtr(hwnd, 0);

    if (!pcoch && (uMsg != WM_CREATE))
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);            
    
    switch(uMsg)
    {
    case WM_CREATE:
        pcoch = new COleControlHost(hwnd);
        if (pcoch)
            return pcoch->_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
        return -1;

    case WM_ERASEBKGND:
        if (pcoch->_punkOC && pcoch->_bInPlaceActive)
        {
            //  Now tell windows we don't need no stinkin'
            //  erased background because our view object
            //  is in-place active and he/she will be
            //  taking over from here.
            return TRUE;
        }
        break;
        
    case WM_PAINT:
        return pcoch->_OnPaint();
        
    case WM_SIZE:
        return pcoch->_OnSize(hwnd, lParam);
        
    case WM_DESTROY:
        return pcoch->_OnDestroy();
        
    case OCM_QUERYINTERFACE:
        return  pcoch->_OnQueryInterface(wParam, lParam);
        
    case OCM_INITIALIZE:
        return pcoch->_InitOCStruct((LPOCHINITSTRUCT)lParam);
        
    case OCM_SETOWNER:
        return pcoch->_SetOwner((IUnknown*)lParam);
    
    case OCM_DOVERB:
        return pcoch->_DoVerb((long)wParam, (LPMSG)lParam);

    case OCM_ENABLEEVENTS:
        return pcoch->_ConnectEvents( pcoch->_punkOC, (BOOL)wParam ) ;
    
    case WM_PALETTECHANGED:
        if (pcoch->_pIOleIPObject) {
            HWND hwnd;
            if (SUCCEEDED(pcoch->_pIOleIPObject->GetWindow(&hwnd))) {
                SendMessage(hwnd, WM_PALETTECHANGED, wParam, lParam);
                }
        }   
        break;

    case WM_SETFOCUS:
        
        //  OC doesn't respond to OLEIVERB_UIACTIVATE ?
        if( pcoch->_dwMiscStatus & OLEMISC_NOUIACTIVATE )
        {
            //  so explicitly assign focus
            HWND hwndObj ;
            if( pcoch->_pIOleIPObject && 
                SUCCEEDED( pcoch->_pIOleIPObject->GetWindow( &hwndObj ) ) )
                SetFocus( hwndObj ) ;
        }
        else
            pcoch->_DoVerb( OLEIVERB_UIACTIVATE, NULL ) ;

        break ;

        
    default:
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);            
    }
    
    return 0;
}

void COleControlHost::_RegisterClass()
{
    WNDCLASS wc = {0};

    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = OCHostWndProc;
    //wc.cbClsExtra    = 0;
    wc.cbWndExtra    = SIZEOF(LPVOID);
    wc.hInstance     = HINST_THISDLL;
    //wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    //wc.lpszMenuName  = NULL;
    wc.lpszClassName = OCHOST_CLASS;
    SHRegisterClass(&wc);
}


HRESULT CProxyUIHandler::QueryInterface(REFIID riid, LPVOID * ppvObj)
{   
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->QueryInterface(riid, ppvObj); 
};

ULONG CProxyUIHandler::AddRef(void)
{   
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->AddRef(); 
};

ULONG CProxyUIHandler::Release(void)
{   
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->Release(); 
};

HRESULT CProxyUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->ShowContextMenu(dwID, ppt, pcmdtReserved, pdispReserved) : E_NOTIMPL;
}

HRESULT CProxyUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->GetHostInfo(pInfo) : E_NOTIMPL;
}

HRESULT CProxyUIHandler::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->ShowUI(dwID, pActiveObject, pCommandTarget, pFrame, pDoc): E_NOTIMPL;
}

HRESULT CProxyUIHandler::HideUI()
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->HideUI(): E_NOTIMPL;
}

HRESULT CProxyUIHandler::UpdateUI()
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->UpdateUI(): E_NOTIMPL;
}

HRESULT CProxyUIHandler::EnableModeless(BOOL fActivate)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->EnableModeless(fActivate): E_NOTIMPL;
}

HRESULT CProxyUIHandler::OnDocWindowActivate(BOOL fActivate)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->OnDocWindowActivate(fActivate): E_NOTIMPL;
}

HRESULT CProxyUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->OnFrameWindowActivate(fActivate): E_NOTIMPL;
}

HRESULT CProxyUIHandler::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->ResizeBorder(prcBorder, pUIWindow, fRameWindow): E_NOTIMPL;
}

HRESULT CProxyUIHandler::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID): E_NOTIMPL;
}

HRESULT CProxyUIHandler::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->GetOptionKeyPath(pchKey, dw): E_NOTIMPL;
}

HRESULT CProxyUIHandler::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->GetDropTarget(pDropTarget, ppDropTarget) : E_NOTIMPL;
}

HRESULT CProxyUIHandler::GetExternal(IDispatch **ppDispatch)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->GetExternal(ppDispatch) : E_NOTIMPL;
}

HRESULT CProxyUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->TranslateUrl(dwTranslate, pchURLIn, ppchURLOut) : E_NOTIMPL;
}

HRESULT CProxyUIHandler::FilterDataObject( IDataObject *pDO, IDataObject **ppDORet)
{
    COleControlHost *poch = IToClass(COleControlHost, _xuih, this);

    return poch->_pIDocHostUIParent ? poch->_pIDocHostUIParent->FilterDataObject(pDO, ppDORet) : E_NOTIMPL;
}

STDAPI_(BOOL) DllRegisterWindowClasses(const SHDRC * pshdrc)
{
    if (pshdrc && pshdrc->cbSize == SIZEOF(SHDRC) && !(pshdrc->dwFlags & ~SHDRCF_ALL))
    {
        if (pshdrc->dwFlags & SHDRCF_OCHOST)
        {
            COleControlHost::_RegisterClass();
            return TRUE;
        }
    }
    return FALSE;
}

//---------------------------------------------------------------------------
//  CEventSink constructor
CEventSink::CEventSink( BOOL bAutoDelete )
    :    _hwndSite(NULL),
         _hwndOwner(NULL),
         _punkOC(NULL),
         _dwCookie(0),
         _cRef(1),
         _bAutoDelete( bAutoDelete )
{
    _iid = _iidDefault = IID_NULL ;
}

//  CEventSink IUnknown impl
STDMETHODIMP CEventSink::QueryInterface( REFIID riid, void** ppvObj )
{
    *ppvObj = NULL ;
    if( IsEqualGUID( riid, IID_IUnknown ) || 
        IsEqualGUID( riid, IID_IDispatch )||
        IsEqualGUID( riid, _iidDefault ) )
    {
        *ppvObj = this ;
        return S_OK ;
    }
    return E_NOINTERFACE ;
}

STDMETHODIMP_(ULONG) CEventSink::AddRef()
{ 
    return InterlockedIncrement( &_cRef ) ;
}

STDMETHODIMP_(ULONG) CEventSink::Release()
{
    if( InterlockedDecrement( &_cRef ) <= 0 )
    {
        if( _bAutoDelete )
            delete this ;
        return 0 ; 
    }
    return _cRef ;
}

//  Connects the sink to the OC's default event dispatch interface.
BOOL CEventSink::Connect( HWND hwndOwner, HWND hwndSite, LPUNKNOWN punkOC )
{
    ASSERT( punkOC ) ;
    IID iidDefault = IID_NULL ;

    if( SUCCEEDED( _GetDefaultEventIID( punkOC, &iidDefault ) ) )
    {
        _iidDefault = iidDefault ;
        return _Connect( hwndOwner, hwndSite, punkOC, iidDefault ) ;
    }
    return FALSE ;
}

//  Establishes advise connection on the specified interface
BOOL CEventSink::_Connect( HWND hwndOwner, HWND hwndSite, LPUNKNOWN punkOC, REFIID iid )
{
    LPCONNECTIONPOINTCONTAINER pcpc;
    ASSERT(punkOC != NULL) ;
    HRESULT hr = CONNECT_E_CANNOTCONNECT ;

    if( _IsConnected( iid ) )
        return TRUE ;

    if( _dwCookie )
        Disconnect() ;

    if( punkOC &&
        SUCCEEDED( punkOC->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pcpc )))
    {
        LPCONNECTIONPOINT pcp = NULL;
        DWORD             dwCookie = 0;
        ASSERT(pcpc != NULL);

        if( SUCCEEDED(pcpc->FindConnectionPoint( iid, &pcp )))
        {
            ASSERT(pcp != NULL);
            hr = pcp->Advise( this, &dwCookie ) ;
            
            if( SUCCEEDED( hr ) )
            {
                _iid = iid ;
                _dwCookie  = dwCookie ;
                _hwndOwner = hwndOwner ;
                _hwndSite  = hwndSite ;
                _punkOC    = punkOC ;
                _punkOC->AddRef() ;
            }
            pcp->Release();
        }
        pcpc->Release();
    }

    return SUCCEEDED( hr ) ;
}

//  Retrieves default event dispatch interface from the OC.
HRESULT CEventSink::_GetDefaultEventIID( LPUNKNOWN punkOC, IID* piid )
{
    HRESULT hr ;

    ASSERT( punkOC ) ;
    ASSERT( piid ) ;

    IProvideClassInfo  *pci ;
    IProvideClassInfo2 *pci2 ;
    *piid = IID_NULL ;

    #define IMPLTYPE_MASK \
        (IMPLTYPEFLAG_FDEFAULT|IMPLTYPEFLAG_FSOURCE|IMPLTYPEFLAG_FRESTRICTED)
    #define IMPLTYPE_DEFAULTSOURCE \
        (IMPLTYPEFLAG_FDEFAULT|IMPLTYPEFLAG_FSOURCE)

    //  Retrieve default outbound dispatch IID using OC's IProvideClassInfo2
    if( SUCCEEDED( (hr = punkOC->QueryInterface( IID_IProvideClassInfo2, (void**)&pci2 )) ) )
    {
        hr = pci2->GetGUID( GUIDKIND_DEFAULT_SOURCE_DISP_IID, piid ) ;
        pci2->Release() ;
    }
    else // no IProvideClassInfo2; try IProvideClassInfo:
    if( SUCCEEDED( (hr = punkOC->QueryInterface( IID_IProvideClassInfo, (void**)&pci )) ) )
    {
        ITypeInfo* pClassInfo = NULL;

        if( SUCCEEDED( (hr = pci->GetClassInfo( &pClassInfo )) ) )
        {
            LPTYPEATTR pClassAttr;
            ASSERT( pClassInfo );

            if( SUCCEEDED( (hr = pClassInfo->GetTypeAttr( &pClassAttr )) ) )
            {
                ASSERT( pClassAttr ) ;
                ASSERT( pClassAttr->typekind == TKIND_COCLASS ) ;

                // Enumerate implemented interfaces looking for default source IID.
                HREFTYPE hRefType;
                int      nFlags;

                for( UINT i = 0; i < pClassAttr->cImplTypes; i++ )
                {
                    if( SUCCEEDED( (hr = pClassInfo->GetImplTypeFlags( i, &nFlags )) ) &&
                        ((nFlags & IMPLTYPE_MASK) == IMPLTYPE_DEFAULTSOURCE) )
                    {
                        // Got the interface, now retrieve its IID:
                        ITypeInfo* pEventInfo = NULL ;

                        if( SUCCEEDED( (hr = pClassInfo->GetRefTypeOfImplType( i, &hRefType )) ) &&
                            SUCCEEDED( (hr = pClassInfo->GetRefTypeInfo( hRefType, &pEventInfo )) ) )
                        {
                            LPTYPEATTR pEventAttr;
                            ASSERT( pEventInfo ) ;

                            if( SUCCEEDED( (hr = pEventInfo->GetTypeAttr( &pEventAttr )) ) )
                            {
                                *piid = pEventAttr->guid ; 
                                pEventInfo->ReleaseTypeAttr(pEventAttr);
                            }
                            pEventInfo->Release();
                        }
                        break;
                    }
                }
                pClassInfo->ReleaseTypeAttr(pClassAttr);
            }
            pClassInfo->Release();
        }
        pci->Release() ;
    }

    if( SUCCEEDED( hr ) && IsEqualIID( *piid, IID_NULL ) )
        hr = E_FAIL ;

    return hr ;
}

//  reports whether the sink is connected to the indicated sink
BOOL CEventSink::_IsConnected( REFIID iid )
{
    return _dwCookie != 0L && 
           IsEqualIID( iid, _iid ) ;
}

//  disconnects the sink
BOOL CEventSink::Disconnect()
{
    LPCONNECTIONPOINTCONTAINER pcpc;

    if( _dwCookie != 0 &&
        _punkOC &&
        SUCCEEDED( _punkOC->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pcpc)))
    {
        LPCONNECTIONPOINT pcp = NULL;
        ASSERT(pcpc != NULL);

        if (SUCCEEDED(pcpc->FindConnectionPoint(_iid, &pcp)))
        {
            ASSERT(pcp != NULL);
            pcp->Unadvise(_dwCookie);
            pcp->Release();

            _iid        = IID_NULL ;
            _dwCookie   = 0L ;
            _hwndOwner = NULL ;
            _hwndSite  = NULL ;
            _punkOC->Release() ;
            _punkOC     = NULL ;
        }
        pcpc->Release();
        return TRUE ;
    }

    return FALSE ;
}

//  CEventSink IDispatch interface
STDMETHODIMP CEventSink::Invoke( 
    IN DISPID dispIdMember,
    IN REFIID riid,
    IN LCID lcid,
    IN WORD wFlags,
    IN OUT DISPPARAMS *pDispParams,
    OUT VARIANT *pVarResult,
    OUT EXCEPINFO *pExcepInfo,
    OUT UINT *puArgErr)
{
    //  Copy method args to notification block
    NMOCEVENT   event ;
    ZeroMemory( &event, sizeof(event) ) ;
    event.hdr.hwndFrom = _hwndSite;  
    event.hdr.idFrom   = GetDlgCtrlID( _hwndSite ) ;
    event.hdr.code     = OCN_OCEVENT ;
    event.dispID       = dispIdMember ;
    event.iid          = riid ;
    event.lcid         = lcid ;
    event.wFlags       = wFlags ;
    event.pDispParams  = pDispParams ;
    event.pVarResult   = pVarResult ;
    event.pExepInfo    = pExcepInfo ;
    event.puArgErr     = puArgErr ;

    //  Notify parent of event
    ::SendMessage( _hwndOwner, WM_NOTIFY, event.hdr.idFrom, (LPARAM)&event ) ;
    
    //  Cleanup args
    if (pVarResult != NULL)
        VariantClear( pVarResult ) ;

    return S_OK ;
}
