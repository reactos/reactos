#ifndef __DATETIMEPICKEROC_H__
#define __DATETIMEPICKEROC_H__

#include "unicpp/stdafx.h"
#include <mshtmdid.h>
#include <mshtml.h>
#include "atldisp.h"

#define DTPICKER_WINDOWCLASS          TEXT("Date Time Picker ActiveX Control Window")
#define DTPICKER_CONTROL_ID           0x00929ba6

class CDateTimePickerOC;

class ATL_NO_VTABLE CDateTimePickerOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CDateTimePickerOC, &CLSID_DateTimePickerOC>
                    , public CComControl<CDateTimePickerOC>
                    , public IDispatchImpl<IDateTimePickerOC, &IID_IDateTimePickerOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IProvideClassInfo2Impl<&CLSID_DateTimePickerOC, NULL, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IPersistImpl<CDateTimePickerOC>
                    , public IOleControlImpl<CDateTimePickerOC>
                    , public IViewObjectExImpl<CDateTimePickerOC>
                    , public IOleInPlaceActiveObjectImpl<CDateTimePickerOC>
                    , public IObjectSafetyImpl<CDateTimePickerOC>
                    , public IPersistPropertyBagImpl<CDateTimePickerOC>
                    , public IQuickActivateImpl<CDateTimePickerOC>
                    , public IOleInPlaceObject
                    , public CShell32AtlIDispatch<CDateTimePickerOC, &CLSID_DateTimePickerOC, &IID_IDateTimePickerOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
{
public:
    CDateTimePickerOC();
    ~CDateTimePickerOC();

    DECLARE_POLY_AGGREGATABLE(CDateTimePickerOC);
    DECLARE_NO_REGISTRY();
    DECLARE_WND_CLASS(DTPICKER_WINDOWCLASS)

BEGIN_COM_MAP(CDateTimePickerOC)
    // ATL Uses these in IUnknown::QueryInterface()
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IDateTimePickerOC)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
END_COM_MAP()

BEGIN_MSG_MAP(CDateTimePickerOC)
    MESSAGE_HANDLER(WM_SIZE, _OnSetSize) 
    MESSAGE_HANDLER(WM_SETFOCUS, _OnSetFocusControl)

    // REVIEW: How many of the following message handlers are TRUELY needed?  All?  None?  Some?
    MESSAGE_HANDLER(WM_KEYDOWN, _ForwardMessage) 
    MESSAGE_HANDLER(WM_SYSKEYDOWN, _ForwardMessage) 
    MESSAGE_HANDLER(WM_CHAR, _ForwardMessage) 
    MESSAGE_HANDLER(WM_KILLFOCUS, _ForwardMessage) 
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CDateTimePickerOC)
END_PROPERTY_MAP()

    // *** IDateTimePickerOC ***
    virtual STDMETHODIMP get_DateTime(OUT DATE * pdatetime);
    virtual STDMETHODIMP put_DateTime(IN DATE datetime);
    virtual STDMETHODIMP get_Enabled(OUT VARIANT_BOOL * pfEnabled);
    virtual STDMETHODIMP put_Enabled(IN VARIANT_BOOL fEnabled);
    virtual STDMETHODIMP STDMETHODCALLTYPE Reset(VARIANT vDelta);

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CDateTimePickerOC>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CDateTimePickerOC>::ContextSensitiveHelp(fEnterMode); };

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP InPlaceDeactivate(void) {return IOleInPlaceObject_InPlaceDeactivate();};
    virtual STDMETHODIMP UIDeactivate(void) { return IOleInPlaceObject_UIDeactivate(); };
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect) {return IOleInPlaceObject_SetObjectRects(lprcPosRect, lprcClipRect);};
    virtual STDMETHODIMP ReactivateAndUndo(void)  { return E_NOTIMPL; };

    // *** IPersistPropertyBag ***
    virtual STDMETHODIMP Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog);
    virtual STDMETHODIMP Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) {return S_OK;};

    // *** IObjectSafety ***
    virtual STDMETHODIMP SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) {return S_OK;};

    // *** IOleInPlaceActiveObject ***
    virtual STDMETHODIMP TranslateAccelerator(MSG *pMsg) { return CShell32AtlIDispatch<CDateTimePickerOC, &CLSID_DateTimePickerOC, &IID_IDateTimePickerOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::TranslateAcceleratorPriv(this, pMsg, m_spClientSite); };

    // Over ride ATL functions.
    LRESULT _ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnSetSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName = NULL, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, DWORD dwExStyle = 0, UINT_PTR nID = 0);
    virtual HRESULT TranslateAcceleratorInternal(MSG *pMsg, IOleClientSite * pocs) { return E_NOTIMPL; };

    LRESULT _OnSetFocusControl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnSetFocusDatePicker(UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent) { return CShell32AtlIDispatch<CDateTimePickerOC, &CLSID_DateTimePickerOC, &IID_IDateTimePickerOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::DoVerbUIActivate(prcPosRect, hwndParent, _hwndDTPicker); };
    static LRESULT CALLBACK SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData);

    virtual STDMETHODIMP PrivateQI(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject);};

protected:
    // Helper functions;
    HWND                _hwndDTPicker;               // HWND of comctl32.dll DateTime Picker control
    VARIANT_BOOL        _fEnabled;                   // Is the control enabled (not grayed out)?
    SYSTEMTIME          _st;                         // The system time to initialize to...
};


#endif // __DATETIMEPICKEROC_H__
