
#ifdef POSTSPLIT


#ifndef __SHFVOCX_H__
#define __SHFVOCX_H__

#include "..\util.h" // for BSTR functions
#include "dutil.h" // for BSTR functions
#include "stdafx.h"
#include "shlguid.h"
#include "sdspatch.h"


class CShellFolderViewOC;

class CDShellFolderViewEvents : public DShellFolderViewEvents
{
public:
    // *** IUnknown ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // *** IDispatch ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo,LCID lcid,ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID *rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
              DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,UINT *puArgErr);

    CDShellFolderViewEvents(CShellFolderViewOC * psfvOC, IUnknown * punk) { _psfvOC = psfvOC; _punk = punk; };
    ~CDShellFolderViewEvents() {};

protected:
    CShellFolderViewOC * _psfvOC;
    IUnknown * _punk;
};

class ATL_NO_VTABLE CShellFolderViewOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CShellFolderViewOC, &CLSID_WebViewFolderContents>
                    , public CComControl<CShellFolderViewOC>
                    , public IDispatchImpl<IFolderViewOC, &IID_IFolderViewOC, &LIBID_Shell32>
                    , public IProvideClassInfo2Impl<&CLSID_ShellFolderView, NULL, &LIBID_Shell32>
                    , public IOleControlImpl<CShellFolderViewOC>
                    , public IOleObjectImpl<CShellFolderViewOC>
                    , public IViewObjectExImpl<CShellFolderViewOC>
                    , public IOleInPlaceActiveObjectImpl<CShellFolderViewOC>
                    , public IObjectSafetyImpl<CShellFolderViewOC>
                    , public IConnectionPointContainerImpl<CShellFolderViewOC>
                    , public IConnectionPointImpl<CShellFolderViewOC, &IID_IFolderViewOC>
                    , public IPersistStreamInitImpl<CShellFolderViewOC>
                    , public IPersistPropertyBagImpl<CShellFolderViewOC>
{
public:
    // ATL Declarations
    DECLARE_POLY_AGGREGATABLE(CShellFolderViewOC);
    DECLARE_NO_REGISTRY();

BEGIN_COM_MAP(CShellFolderViewOC)
    // ATL Uses these in IUnknown::QueryInterface()
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IFolderViewOC)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CShellFolderViewOC)
    // ATL Uses these to implement the Connection point code
    CONNECTION_POINT_ENTRY(IID_IFolderViewOC)
END_CONNECTION_POINT_MAP()

// Declare the default message map
BEGIN_MSG_MAP(CShellFolderViewOC)
    // If we are being destroyed, we should release any forwarders we may have as to remove circular references
    MESSAGE_HANDLER(WM_DESTROY, _ReleaseForwarderMessage) 
    MESSAGE_HANDLER(WM_CLOSE, _ReleaseForwarderMessage)
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CShellFolderViewOC)
END_PROPERTY_MAP()

    // *** IPersistStreamInit ***
    virtual STDMETHODIMP Load(IStream *pStm) { return S_OK; };
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) { return S_OK; };
    virtual STDMETHODIMP InitNew(void);

    // *** IPersistPropertyBag ***
    virtual STDMETHODIMP Load(IPropertyBag *pBag, IErrorLog *pErrorLog);
    virtual STDMETHODIMP Save(IPropertyBag *pBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // *** IFolderViewOC methods ***
    virtual STDMETHODIMP SetFolderView(IDispatch *pDisp);

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP UIDeactivate(void) { return IOleInPlaceObject_UIDeactivate(); };

    friend class CDShellFolderViewEvents;

protected:
    CShellFolderViewOC();
    ~CShellFolderViewOC();

    // Internal functions
    HRESULT _SetupForwarder(void);
    void    _ReleaseForwarder(void);
    void    _InitDefault(void);
    LRESULT _ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    // Private member variables
    BOOL                    _fInit : 1;           // Has the init function been called?
    DWORD                   _dwdeeCookie;       // Have we installed _dee in browser?
    int                     _cSinks;            // cached count of sinks...
    IDispatch *             _pdispFolderView;  // Hold onto IDispatch passed in from put_FolderView
    CDShellFolderViewEvents * _pdee;
};

#endif // __SHFVOCX_H__

#endif // POSTSPLIT
