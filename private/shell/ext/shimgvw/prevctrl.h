// PrevCtrl.h : Declaration of the CPreview

#ifndef __PREVCTRL_H_
#define __PREVCTRL_H_

#include "resource.h"       // main symbols
#include "PrevWnd.h"
#include "Events.h"

/////////////////////////////////////////////////////////////////////////////
// CPreview
class ATL_NO_VTABLE CPreview : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CPreview, &CLSID_Preview>,
        public CComControl<CPreview>,
        public CStockPropImpl<CPreview, IPreview, &IID_IPreview, &LIBID_PREVIEWLib>,
        public IProvideClassInfo2Impl<&CLSID_Preview, NULL, &LIBID_PREVIEWLib>,
//        public IPersistStreamInitImpl<CPreview>,
        public IPersistPropertyBagImpl<CPreview>,           // So we can read <PARAM>'s from our object tag
        public IPersistStorageImpl<CPreview>,               // required for Embeddable objects
        public IOleObjectImpl<CPreview>,                    // required for Embeddable objects
        public IDataObjectImpl<CPreview>,                   // required for Embeddable objects
        public IQuickActivateImpl<CPreview>,
        public IOleControlImpl<CPreview>,   // REVIEW: will IOleControl::GetControlInfo help with my keyboard problems?
        public IOleInPlaceActiveObjectImpl<CPreview>,       // handles resizing, active state, TranslateAccelerator
        public IViewObjectExImpl<CPreview>,                 // for flicker-free drawing support
        public IOleInPlaceObjectWindowlessImpl<CPreview>,   // allow for windowless operation (we don't use windowless, should we be using this interface?)
//        public ISpecifyPropertyPagesImpl<CPreview>,
//        public IPointerInactiveImpl<CPreview>,
        public CPreviewEvents<CPreview>,                    // our event code for sending events to our container
        public IConnectionPointContainerImpl<CPreview>,     // Connection Point Container for our outgoing event hooks.
        public IObjectSafetyImpl<CPreview>                  // allows this control to be scripted
{
public:
    CPreviewWnd m_cwndPreview;

    CPreview()
    {
        // we want to be run in a window and never windowless
        m_bWindowOnly = TRUE;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_PREVIEW)

DECLARE_WND_CLASS( TEXT("ShImgVw:CPreview") );

BEGIN_COM_MAP(CPreview)
    COM_INTERFACE_ENTRY(IPreview)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
//    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
//    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
//    COM_INTERFACE_ENTRY_IMPL(IPointerInactive)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPreview)
    CONNECTION_POINT_ENTRY(DIID_DPreviewEvents)
END_CONNECTION_POINT_MAP()

BEGIN_PROPERTY_MAP(CPreview)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()

BEGIN_MSG_MAP(CPreview)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
END_MSG_MAP()

// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

// IOleInPlaceActiveObjectImpl
    STDMETHOD(TranslateAccelerator)( LPMSG lpmsg );
    STDMETHOD(OnFrameWindowActivate)( BOOL fActive );

// IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwSupportedOptions, DWORD dwEnabledOptions);

// IPersistPropertyBag
    STDMETHOD(Load)(IPropertyBag * pPropBag, IErrorLog * pErrorLog);
    STDMETHOD(Save)(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) {return S_OK;}

public:
    // Control message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HRESULT OnDrawAdvanced(ATL_DRAWINFO& di);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

// IPreview
public:
    STDMETHOD(get_printable)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_printable)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_cxImage)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_cyImage)(/*[out, retval]*/ long *pVal);
    STDMETHOD(ShowFile)(BSTR bstrFileName, int iSelectCount);
    STDMETHOD(Show)(VARIANT var);
};

#endif //__PREVCTRL_H_
