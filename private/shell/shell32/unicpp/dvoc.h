#ifndef __DVOC_H__
#define __DVOC_H__

#include "cnctnpt.h"
#include "dspsprt.h"
#include "shlguid.h"
#include "sdspatch.h"
#include "stdenum.h"
#include "stdafx.h"
#include "dutil.h"
#include <mshtmdid.h>

class CWebViewFolderContents;
#define SZ_ATL_SHEMBEDDING_WNDCLASS         TEXT("ATL Shell Embedding")

HRESULT MakeSafeForScripting(IUnknown** ppDisp);

class ATL_NO_VTABLE CWebViewFolderContents
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CWebViewFolderContents, &CLSID_WebViewFolderContents>
                    , public CComControl<CWebViewFolderContents>
                    , public IDispatchImpl<IShellFolderViewDual, &IID_IShellFolderViewDual, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IProvideClassInfo2Impl<&CLSID_WebViewFolderContents, NULL, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IPersistImpl<CWebViewFolderContents>
                    , public IOleControlImpl<CWebViewFolderContents>
                    , public IOleObjectImpl<CWebViewFolderContents>
                    , public IViewObjectExImpl<CWebViewFolderContents>
                    , public IOleInPlaceActiveObjectImpl<CWebViewFolderContents>
                    , public IDataObjectImpl<CWebViewFolderContents>
                    , public IObjectSafetyImpl<CWebViewFolderContents>
                    , public IConnectionPointContainer
                    , public IOleInPlaceObject
                    , public IInternetSecurityMgrSite
                    , public IWebViewOCWinMan
{
public:
    CWebViewFolderContents();
    ~CWebViewFolderContents();

    DECLARE_POLY_AGGREGATABLE(CWebViewFolderContents);
    DECLARE_NO_REGISTRY();
    DECLARE_WND_CLASS(SZ_ATL_SHEMBEDDING_WNDCLASS)

BEGIN_COM_MAP(CWebViewFolderContents)
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
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IShellFolderViewDual)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY(IInternetSecurityMgrSite)
    COM_INTERFACE_ENTRY(IWebViewOCWinMan)
END_COM_MAP()

 
// Declare the default message map
BEGIN_MSG_MAP(CWebViewFolderContents)
    MESSAGE_HANDLER(WM_SIZE, _OnSizeMessage) 
    MESSAGE_HANDLER(WM_NOTIFY, _OnMessageForwarder) 
    MESSAGE_HANDLER(WM_CONTEXTMENU, _OnMessageForwarder)
    MESSAGE_HANDLER(WM_ERASEBKGND, _OnEraseBkgndMessage)
END_MSG_MAP()


    // *** IDispatch ***
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);

    // *** IProvideClassInfo ***
    virtual STDMETHODIMP GetClassInfo(ITypeInfo** pptinfo);

    // *** IInternetSecurityMgrSite ***
    // virtual STDMETHODIMP GetWindow(HWND * lphwnd);              // Also in IOleWindow
    virtual STDMETHODIMP EnableModeless(BOOL fEnable) { return IOleInPlaceActiveObjectImpl<CWebViewFolderContents>::EnableModeless(fEnable); };     // Also in IOleInPlaceActiveObject

    // *** ShellFolderView ***
    virtual STDMETHODIMP get_Application(IDispatch **ppid);
    virtual STDMETHODIMP get_Parent (IDispatch **ppid);
    virtual STDMETHODIMP get_Folder(Folder **ppid);
    virtual STDMETHODIMP SelectedItems(FolderItems **ppid);
    virtual STDMETHODIMP get_FocusedItem(FolderItem **ppid);
    virtual STDMETHODIMP SelectItem(VARIANT *pvfi, int dwFlags);
    virtual STDMETHODIMP PopupItemMenu(FolderItem * pfi, VARIANT vx, VARIANT vy, BSTR * pbs);
    virtual STDMETHODIMP get_Script(IDispatch **ppid);
    virtual STDMETHODIMP get_ViewOptions(long *plSetting);

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) { return IOleInPlaceActiveObjectImpl<CWebViewFolderContents>::GetWindow(lphwnd); };
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CWebViewFolderContents>::ContextSensitiveHelp(fEnterMode); };

    // *** IOleObject ***
    virtual STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP InPlaceDeactivate(void);
    virtual STDMETHODIMP UIDeactivate(void) { return IOleInPlaceObject_UIDeactivate(); };
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect);
    virtual STDMETHODIMP ReactivateAndUndo(void)  { return E_NOTIMPL; };

    // *** IOleInPlaceActiveObject ***
    // our frame was activated, better do the activation thing.
    STDMETHODIMP OnFrameWindowActivate(BOOL fActivate)
    {
        if (_hwndLV && fActivate)
            ::SetFocus(_hwndLV);
        return S_OK;
    };
    virtual STDMETHODIMP TranslateAccelerator(LPMSG pMsg);

    // *** IWebViewOCWinMan ***
    STDMETHODIMP SwapWindow(HWND hwndLV, IWebViewOCWinMan **pocWinMan);

    // *** IConnectionPointContainer ***
    STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP);

    // Over ride ATL functions.
    LRESULT _OnMessageForwarder(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnEraseBkgndMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnSizeMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent);
    HRESULT DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND hwndParent);
    virtual STDMETHODIMP Close(DWORD dwSaveOption);

protected:
    // Helper functions;
    HRESULT _SetupAutomationForwarders(void);
    HRESULT _ReleaseAutomationForwarders(void);
    HRESULT _OnInPlaceActivate(void);
    void _ReleaseWindow(void);
    void _ShowWindowLV(HWND hwndLV);
    void _UnadviseAll();

    class CConnectionPointForwarder : public IConnectionPoint
    {
        // IUnknown methods
        //
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
        virtual STDMETHODIMP_(ULONG) AddRef(void);
        virtual STDMETHODIMP_(ULONG) Release(void);
        // IConnectionPoint methods
        //
        virtual STDMETHODIMP GetConnectionInterface(IID * pIID);
        virtual STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer ** ppCPC);
        virtual STDMETHODIMP Advise(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
        virtual STDMETHODIMP Unadvise(DWORD dwCookie);
        virtual STDMETHODIMP EnumConnections(LPENUMCONNECTIONS * ppEnum) { return _pcpAuto->EnumConnections(ppEnum); }

        IConnectionPoint *  _pcpAuto;
        HDSA                _dsaCookies;
        IUnknown*           _punkParent;
        friend class CWebViewFolderContents;
    };
    friend class CConnectionPointForwarder;
    CConnectionPointForwarder m_cpEvents;

    IDefViewFrame2 *    _pdvf2;   // defview
    BOOL                _fClientEdge;
    BOOL                _fTabRecieved;
    BOOL                _fCalledAutoArrange;
    HWND                _hwndLV;
    HWND                _hwndLVParent;

    ITypeInfo *         _pClassTypeInfo; // ITypeInfo of class

    // stuff added to delegate all of our work up to DefViews automation
    IShellFolderViewDual *_pdispAuto;
};


#endif // __DVOC_H__
