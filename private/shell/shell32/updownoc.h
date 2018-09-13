#ifndef __UPDOWNOC_H__
#define __UPDOWNOC_H__

#include "unicpp/stdafx.h"
#include <mshtmdid.h>
#include <mshtml.h>
#include "atldisp.h"

#define UPDOWN_WINDOWCLASS          TEXT("UpDown ActiveX Control Window")
#define UPDOWN_CONTROL_ID           0x0023fac9

class CUpDownOC;

class ATL_NO_VTABLE CUpDownOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CUpDownOC, &CLSID_UpDownOC>
                    , public CComControl<CUpDownOC>
                    , public IDispatchImpl<IUpDownOC, &IID_IUpDownOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IProvideClassInfo2Impl<&CLSID_UpDownOC, NULL, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IPersistImpl<CUpDownOC>
                    , public IOleControlImpl<CUpDownOC>
                    , public IViewObjectExImpl<CUpDownOC>
                    , public IOleInPlaceActiveObjectImpl<CUpDownOC>
                    , public IObjectSafetyImpl<CUpDownOC>
                    , public IPersistPropertyBagImpl<CUpDownOC>
                    , public IQuickActivateImpl<CUpDownOC>
                    , public IOleInPlaceObject
                    , public CShell32AtlIDispatch<CUpDownOC, &CLSID_UpDownOC, &IID_IUpDownOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
{
public:
    CUpDownOC();
    ~CUpDownOC();
    static CWndClassInfo& GetWndClassInfo( );

    DECLARE_POLY_AGGREGATABLE(CUpDownOC);
    DECLARE_NO_REGISTRY();

BEGIN_COM_MAP(CUpDownOC)
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
    COM_INTERFACE_ENTRY(IUpDownOC)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
END_COM_MAP()

// Declare the default message map
// UpDown fires UDN_DELTAPOS before the change to give us a chance to cancel the event.
// We synch WM_VSCROLL/WM_HSCROLL because that's after the change.
BEGIN_MSG_MAP(CUpDownOC)
    MESSAGE_HANDLER(WM_SIZE, _ForwardMessage) 
    MESSAGE_HANDLER(WM_SETFOCUS, _OnFocus) 
    MESSAGE_HANDLER(WM_VSCROLL, _OnPosChange) 
    MESSAGE_HANDLER(WM_HSCROLL, _OnPosChange) 
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CUpDownOC)
END_PROPERTY_MAP()

    // *** IUpDownOC ***
    virtual STDMETHODIMP get_Value(OUT BSTR * pbstrValue);
    virtual STDMETHODIMP put_Value(IN BSTR bstrValue);
    virtual STDMETHODIMP Range(IN LONG lMinValue, IN LONG lMaxValue);
    virtual STDMETHODIMP get_Enabled(OUT VARIANT_BOOL * pfEnabled);
    virtual STDMETHODIMP put_Enabled(IN VARIANT_BOOL fEnabled);

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CUpDownOC>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CUpDownOC>::ContextSensitiveHelp(fEnterMode); };

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
    virtual STDMETHODIMP TranslateAccelerator(MSG *pMsg) { return CShell32AtlIDispatch<CUpDownOC, &CLSID_UpDownOC, &IID_IUpDownOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::TranslateAcceleratorPriv(this, pMsg, m_spClientSite); };

    // Over ride ATL functions.
    LRESULT _OnPosChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    virtual HRESULT TranslateAcceleratorInternal(MSG *pMsg, IOleClientSite * pocs) {return E_NOTIMPL;};

    virtual STDMETHODIMP IOleObject_SetClientSite(IOleClientSite *pClientSite);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName = NULL, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, DWORD dwExStyle = 0, UINT_PTR nID = 0);

    LRESULT _OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent) { return CShell32AtlIDispatch<CUpDownOC, &CLSID_UpDownOC, &IID_IUpDownOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::DoVerbUIActivate(prcPosRect, hwndParent, _hwndUpDown); };
    static LRESULT CALLBACK SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData);

    virtual STDMETHODIMP PrivateQI(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject);};

protected:
    // Helper functions;
    HRESULT _CreateUpDownWindow(void);
    HRESULT _GetEditboxBuddy(void);
    LONG _GetBuddiesValue(void);
    HRESULT _SetBuddiesValue(LONG lNewValue);
    HRESULT _GetRange(void);
    LONG _Filter(LONG lValue);
    HRESULT _getValue(OUT LONG * plValue);
    HRESULT _putValue(IN LONG lValue);

    // Control State
    BITBOOL             _fHorizontal    : 1;         // Does the caller want the arrows to go right-left instead of the default up-down?
    BITBOOL             _fWrap          : 1;         // Does the caller want the scrolling to wrap when hitting a boundry?
    BITBOOL             _fArrowKeys     : 1;         // Does the caller want the scrolling to wrap when hitting a boundry?
    BITBOOL             _fAlignLeft     : 1;         // Does the caller want the scroll itme to wrap when hitting a boundry?
    BITBOOL             _fNoPropagation : 1;         // _fEditBox is on and the caller does not want the editbox to be changed on up/down events
    BITBOOL             _fInRecursion   : 1;
    VARIANT_BOOL        _fEnabled;                   // Is the control enabled (not grayed out)?

    HWND                _hwndUpDown;                 // HWND of UpDown control
    LONG                _lMinValue;                  // Low range
    LONG                _lMaxValue;                  // High range
    LONG                _lValue;                     // Current range
    LPWSTR              _pwzBuddyID;                 // ID of Buddy Control (normally Editbox)
    CComPtr<IHTMLInputTextElement> _spBuddyEditbox;  // pointer to Buddy Control (normally Editbox)
};

inline CWndClassInfo& CUpDownOC::GetWndClassInfo( )
{
    static CWndClassInfo wc =
    {
        { sizeof(WNDCLASSEX), 0, StartWindowProc,
          0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, UPDOWN_WINDOWCLASS, 0 },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

#endif // __UPDOWNOC_H__
