#ifndef __FILEASSOCNAMESPACEOC_H__
#define __FILEASSOCNAMESPACEOC_H__

#include "unicpp/stdafx.h"
#include <mshtmdid.h>
#include <mshtml.h>
#include "atldisp.h"
#include "combooc.h"
#include "ids.h"

class CFileAssocNameSpaceOC;
#define FILEASSOCIATIONSID_FILE_PATH             1   // Go parse it.
#define FILEASSOCIATIONS_WINDOWCLASS          TEXT("ActiveX Control ComboBoxEx of File Associations")

//-------------------------------------------------------------------------//
//  class CFileAssocNameSpaceOC
//
//  Note: derivation from CShell32AtlIDispatch<> instead of IOleObjectImpl<> is
//  necessary when this object resides in shell32.dll
class ATL_NO_VTABLE CFileAssocNameSpaceOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CFileAssocNameSpaceOC, &CLSID_FileAssocNameSpaceOC>
                    , public CComControl<CFileAssocNameSpaceOC>
                    , public IDispatchImpl<IComboBoxExOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IProvideClassInfo2Impl<&CLSID_FileAssocNameSpaceOC, NULL, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IPersistImpl<CFileAssocNameSpaceOC>
                    , public IOleControlImpl<CFileAssocNameSpaceOC>
                    , public IViewObjectExImpl<CFileAssocNameSpaceOC>
                    , public IOleInPlaceActiveObjectImpl<CFileAssocNameSpaceOC>
                    , public IObjectSafetyImpl<CFileAssocNameSpaceOC>
                    , public IConnectionPointContainerImpl<CFileAssocNameSpaceOC>
                    , public IPersistPropertyBagImpl<CFileAssocNameSpaceOC>
                    , public IQuickActivateImpl<CFileAssocNameSpaceOC>
                    , public IOleInPlaceObject
                    , public CProxy_ComboBoxExEvents<CFileAssocNameSpaceOC>
                    , public CShell32AtlIDispatch<CFileAssocNameSpaceOC, &CLSID_FileAssocNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public CComboBoxExOC
{
public:
    CFileAssocNameSpaceOC();
    virtual ~CFileAssocNameSpaceOC();

    DECLARE_POLY_AGGREGATABLE(CFileAssocNameSpaceOC);
    DECLARE_NO_REGISTRY();
    DECLARE_WND_CLASS(FILEASSOCIATIONS_WINDOWCLASS)

BEGIN_COM_MAP(CFileAssocNameSpaceOC)
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
    COM_INTERFACE_ENTRY(IComboBoxExOC)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CFileAssocNameSpaceOC)
    // ATL Uses these to implement the Connection point code
    CONNECTION_POINT_ENTRY(DIID_DComboBoxExEvents)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CFileAssocNameSpaceOC)
    MESSAGE_HANDLER(WM_SIZE, _ForwardMessage) 
    MESSAGE_HANDLER(WM_SETFOCUS, _OnFocus) 
    COMMAND_HANDLER(FCIDM_VIEWADDRESS, CBN_DROPDOWN, _OnDropDownMessage) 
    NOTIFY_HANDLER(FCIDM_VIEWADDRESS, CBEN_DELETEITEM, _OnDeleteItemMessage) 
END_MSG_MAP()
 
BEGIN_PROPERTY_MAP(CFileAssocNameSpaceOC)
END_PROPERTY_MAP()

    // *** IComboBoxExOC ***
    virtual STDMETHODIMP get_String(OUT BSTR *pbs);
    virtual STDMETHODIMP put_String(IN BSTR bs);
    virtual STDMETHODIMP get_Enabled(OUT VARIANT_BOOL * pfEnabled) {return CComboBoxExOC::get_Enabled(pfEnabled);};
    virtual STDMETHODIMP put_Enabled(IN VARIANT_BOOL fEnabled) {return CComboBoxExOC::put_Enabled(fEnabled);};
    virtual STDMETHODIMP Reset(void);

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CFileAssocNameSpaceOC>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CFileAssocNameSpaceOC>::ContextSensitiveHelp(fEnterMode); };

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
    virtual STDMETHODIMP TranslateAccelerator(MSG *pMsg) { return CShell32AtlIDispatch<CFileAssocNameSpaceOC, &CLSID_FileAssocNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::TranslateAcceleratorPriv(this, pMsg, m_spClientSite); };

    // Over ride ATL functions.
    LRESULT _ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) { return CComboBoxExOC::cb_ForwardMessage(uMsg, wParam, lParam, bHandled); };
    LRESULT _OnDropDownMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled) { return CComboBoxExOC::cb_OnDropDownMessage(wNotifyCode, wID, hWndCtl, bHandled); };
    LRESULT _OnDeleteItemMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName = NULL, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, DWORD dwExStyle = 0, UINT_PTR nID = 0);
    HWND _Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID) { return CWindowImpl<CFileAssocNameSpaceOC>::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID); };

    LRESULT _OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent) { return CShell32AtlIDispatch<CFileAssocNameSpaceOC, &CLSID_FileAssocNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::DoVerbUIActivate(prcPosRect, hwndParent, _hwndComboBox); };
    static LRESULT CALLBACK SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData);

    virtual STDMETHODIMP PrivateQI(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject);};
    HRESULT IOleInPlaceObject_InPlaceDeactivate(void);  // Use this time to persist state.

protected:
    // Helper functions;
    BOOL _IsSecure(void) { return TRUE; };      // We are safe because extensions are beign.
    HRESULT _AddDefaultItem(void)   { ASSERT(0); return E_NOTIMPL; };      // Should never get hit.

    virtual HRESULT _PopulateTopItem(void);
    virtual HRESULT _Populate(void);
    void Fire_EnterPressed(void) {CProxy_ComboBoxExEvents<CFileAssocNameSpaceOC>::Fire_EnterPressed();};
    HRESULT _CustomizeName(UINT idString, LPTSTR szDisplayName, DWORD dwSize) { return S_OK; };
    HRESULT _GetSelectText(LPTSTR pszSelectText, DWORD cchSize, BOOL fDisplayName);

    HRESULT _IsDefaultSelection(LPCTSTR pszLastSelection);
    HRESULT _RestoreIfUserInputValue(LPCTSTR pszLastSelection);

    // Private Member Variables
    BITBOOL             _fPopulated : 1;            // Have we populated yet?
};

#endif // __FILEASSOCNAMESPACEOC_H__
