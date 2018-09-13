#include "stdafx.h"
#pragma hdrstop

class CShellFolderViewOC;

class CDShellFolderViewEvents : public DShellFolderViewEvents
{
public:
    // *** IUnknown ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // *** IDispatch ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo,LCID lcid,ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID *rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
              DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,UINT *puArgErr);

    CDShellFolderViewEvents(CShellFolderViewOC * psfvOC) { _psfvOC = psfvOC; };
    ~CDShellFolderViewEvents() {};

protected:
    CShellFolderViewOC * _psfvOC;
};

class ATL_NO_VTABLE CShellFolderViewOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CShellFolderViewOC, &CLSID_ShellFolderViewOC>
                    , public IDispatchImpl<IFolderViewOC, &IID_IFolderViewOC, &LIBID_Shell32>
                    , public IProvideClassInfo2Impl<&CLSID_ShellFolderView, NULL, &LIBID_Shell32>
                    , public IObjectSafetyImpl<CShellFolderViewOC>
                    , public IConnectionPointContainerImpl<CShellFolderViewOC>
                    , public IConnectionPointImpl<CShellFolderViewOC, &DIID_DShellFolderViewEvents>
                    , public CComControl<CShellFolderViewOC>
                    , public IPersistStreamInitImpl<CShellFolderViewOC>
                    , public IOleControlImpl<CShellFolderViewOC>
                    , public IOleObjectImpl<CShellFolderViewOC>
                    , public IOleInPlaceActiveObjectImpl<CShellFolderViewOC>
                    , public IOleInPlaceObjectWindowlessImpl<CShellFolderViewOC>
                    , public IOleInPlaceObject
//                  , public IViewObjectExImpl<CShellFolderViewOC>
{
public:
    // DECLARE_POLY_AGGREGATABLE(CShellFolderViewOC);
    DECLARE_NO_REGISTRY();

BEGIN_COM_MAP(CShellFolderViewOC)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IFolderViewOC)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    //COM_INTERFACE_ENTRY(IViewObjectEx)
    //COM_INTERFACE_ENTRY(IViewObject2)
    //COM_INTERFACE_ENTRY(IViewObject)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CShellFolderViewOC)
    CONNECTION_POINT_ENTRY(DIID_DShellFolderViewEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CShellFolderViewOC)
    MESSAGE_HANDLER(WM_DESTROY, _ReleaseForwarderMessage) 
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CShellFolderViewOC)
END_PROPERTY_MAP()

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CShellFolderViewOC>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CShellFolderViewOC>::ContextSensitiveHelp(fEnterMode); };

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP InPlaceDeactivate(void) {return IOleInPlaceObject_InPlaceDeactivate();};
    virtual STDMETHODIMP UIDeactivate(void) { return IOleInPlaceObject_UIDeactivate(); };
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect) { return IOleInPlaceObject_SetObjectRects(lprcPosRect, lprcClipRect); };
    virtual STDMETHODIMP ReactivateAndUndo(void)  { return E_NOTIMPL; };

    // *** IFolderViewOC methods ***
    virtual STDMETHODIMP SetFolderView(IDispatch *pDisp);

    friend class CDShellFolderViewEvents;

protected:
    CShellFolderViewOC();
    ~CShellFolderViewOC();

private:
    HRESULT _SetupForwarder(void);
    void    _ReleaseForwarder(void);
    LRESULT _ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BOOL                    _fInit;             // Has the init function been called?
    DWORD                   _dwdeeCookie;       // Have we installed _dee in browser?
    IDispatch               *_pdispFolderView;  // Hold onto IDispatch passed in from put_FolderView
    CDShellFolderViewEvents *_pdee;
};

CShellFolderViewOC::CShellFolderViewOC() : _dwdeeCookie(0), _fInit(0), _pdispFolderView(NULL)
{
    // postpone initialization until InitNew is called.
    // this lets us query the container for ambient properties.
    // if we ever do any slow initialization stuff, we won't do it when not needed.
    // the cost is an if in front of every interface which references our state (robustness).
    _pdee = new CDShellFolderViewEvents(this);
}

CShellFolderViewOC::~CShellFolderViewOC()
{
    ATOMICRELEASE(_pdispFolderView);
    if (_pdee)
        delete _pdee;
}

HRESULT CShellFolderViewOC::SetFolderView(IDispatch *pDisp)
{
    HRESULT hr = S_OK;

    _ReleaseForwarder();    // cleanup previous state

    IUnknown_Set((IUnknown **)&_pdispFolderView, (IUnknown *)pDisp);
    if (_pdispFolderView)
        hr = _SetupForwarder();
    return hr;
}

HRESULT CShellFolderViewOC::_SetupForwarder()
{
    if (!_pdee)
        return E_FAIL;

    return ConnectToConnectionPoint(SAFECAST(_pdee, IDispatch *), DIID_DShellFolderViewEvents, TRUE, _pdispFolderView, &_dwdeeCookie, NULL);
}

void CShellFolderViewOC::_ReleaseForwarder()
{
    ConnectToConnectionPoint(NULL, DIID_DShellFolderViewEvents, FALSE, _pdispFolderView, &_dwdeeCookie, NULL);
}


// ATL maintainence functions
LRESULT CShellFolderViewOC::_ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = FALSE;
    _ReleaseForwarder();
    return 0;
}

STDMETHODIMP CDShellFolderViewEvents::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI2(CDShellFolderViewEvents, DIID_DShellFolderViewEvents, DShellFolderViewEvents),
        QITABENTMULTI(CDShellFolderViewEvents, IDispatch, DShellFolderViewEvents),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CDShellFolderViewEvents::AddRef()
{
    return SAFECAST(_psfvOC, IFolderViewOC*)->AddRef();
}

ULONG CDShellFolderViewEvents::Release()
{
    return SAFECAST(_psfvOC, IFolderViewOC*)->Release();
}

STDMETHODIMP CDShellFolderViewEvents::GetTypeInfoCount(UINT * pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, 
                                                    UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                             DISPPARAMS *pdispparams, VARIANT *pvarResult, 
                                             EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    SHINVOKEPARAMS inv;
    inv.flags = 0;
    inv.dispidMember = dispidMember;
    inv.piid = &riid;
    inv.lcid = lcid;
    inv.wFlags = wFlags;
    inv.pdispparams = pdispparams;
    inv.pvarResult = pvarResult;
    inv.pexcepinfo = pexcepinfo;
    inv.puArgErr = puArgErr;

    return IUnknown_CPContainerInvokeIndirect(SAFECAST(_psfvOC, IFolderViewOC *), DIID_DShellFolderViewEvents, &inv);
}

STDAPI CShellFolderViewOC_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    return CComCreator< CComObject< CShellFolderViewOC > >::CreateInstance((void *)punkOuter, riid, ppvOut);
}

