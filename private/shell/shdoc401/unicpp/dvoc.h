#ifndef __DVOC_NEW_H__
#define __DVOC_NEW_H__

//#include "cnctnpt.h"
//#include "dspsprt.h"
#include "shlguid.h"
#include "sdspatch.h"
#include "stdenum.h"
#include "stdafx.h"
#include "dutil.h"
#include <cnctnpt.h>
#include <mshtmdid.h>

class CWebViewFolderContents;
#define SZ_ATL_SHEMBEDDING_WNDCLASS         TEXT("ATL Shell Embedding")
#define WM_DVOC_REARRANGELISTVIEW           (WM_USER + 1)

// CROSS_FRAME_SECURITY - this is disabled. this allows us to let trident do all
// of the security work for us based on the "cross frame scripting" rules. but we
// don't want to allow this since that would let script on internet pages do stuff
// with this OC, like run programs arbitrarily.

HRESULT MakeSafeForScripting(IUnknown** ppDisp);

class CMyIE4ConnectionPoint;


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
                    , public IConnectionPointContainerImpl<CWebViewFolderContents>
                    , public IConnectionPointImpl<CWebViewFolderContents, &DIID_DShellFolderViewEvents>
                    , public IOleInPlaceObject
                    , public IInternetSecurityMgrSite
                    , public IWebViewOCWinMan
                    , public IExpDispSupport
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
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IShellFolderViewDual)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY(IInternetSecurityMgrSite)
    COM_INTERFACE_ENTRY(IWebViewOCWinMan)
    COM_INTERFACE_ENTRY(IExpDispSupport)
END_COM_MAP()

 
BEGIN_CONNECTION_POINT_MAP(CWebViewFolderContents)
    // ATL Uses these to implement the Connection point code
    CONNECTION_POINT_ENTRY(DIID_DShellFolderViewEvents)
END_CONNECTION_POINT_MAP()

// Declare the default message map
BEGIN_MSG_MAP(CWebViewFolderContents)
    MESSAGE_HANDLER(WM_SIZE, _OnSizeMessage) 
    MESSAGE_HANDLER(WM_NOTIFY, _OnMessageForwarder) 
    MESSAGE_HANDLER(WM_CONTEXTMENU, _OnMessageForwarder)
    MESSAGE_HANDLER(WM_ERASEBKGND, _OnEraseBkgndMessage)
    MESSAGE_HANDLER(WM_DVOC_REARRANGELISTVIEW, _OnReArrangeListView)
END_MSG_MAP()


    // *** IDispatch ***
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);

	virtual STDMETHODIMP TranslateAccelerator(LPMSG pMsg);
	
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

    // *** IWebViewOCWinMan ***
    STDMETHODIMP SwapWindow(HWND hwndLV, IWebViewOCWinMan **pocWinMan);

    // *** IConnectionPoint ***
    STDMETHODIMP Advise(IUnknown * pUnkSink, DWORD * pdwCookie);
    STDMETHODIMP Unadvise(DWORD dwCookie);

    // *** IExpDispSupport specific methods ***
    STDMETHODIMP FindCIE4ConnectionPoint(REFIID riid, CIE4ConnectionPoint **ppccp);
    STDMETHODIMP OnTranslateAccelerator(MSG __RPC_FAR *pMsg,DWORD grfModifiers) { ASSERT(0); return E_NOTIMPL;} ;
    STDMETHODIMP OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                        VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr) { ASSERT(0); return E_NOTIMPL;} ;

    // Over ride ATL functions.
    LRESULT _OnMessageForwarder(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnEraseBkgndMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnSizeMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT _OnReArrangeListView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent);
    HRESULT DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND hwndParent);
    virtual STDMETHODIMP Close(DWORD dwSaveOption);

protected:
    // Helper functions;
    HRESULT _GetFolder();
    HRESULT _GetFolderIDList(LPITEMIDLIST *ppidl);
#ifdef CROSS_FRAME_SECURITY
    HRESULT _GetFolderSecurity(BSTR *pbstr);
#endif
    HRESULT _OnInPlaceActivate(void);
    void _ReleaseWindow(void);
    void _ShowWindowLV(HWND hwndLV);
    HRESULT _SetAutomationObject(void);
    HRESULT _ClearAutomationObject(void);
    void UnadviseAll();

    CMyIE4ConnectionPoint  * _pmyiecp;
    
    IDefViewFrame *     _pdvf;
    IDefViewFrame2 *    _pdvf2;   // alternate identity for _pdvf (optional)
    IShellFolderView *  _psfv;    // alternate identity for _pdvf
    BOOL                _fSetDefViewAutomationObject : 1;
    BOOL                _fClientEdge : 1;
    BOOL                _fTabRecieved : 1;

    HWND                _hwndLV;
    HWND                _hwndLVParent;
    DWORD               _dwAdviseCount;

    ITypeInfo *         _pClassTypeInfo; // ITypeInfo of class
    CSDFolder *         _psdf;        // The shell folder we talk to ...
    IShellFolderViewDual * m_pdisp;     // This will not contain a ref.
    BOOL                _fReArrangeListView;     // Post a "rearrange listview icons" msg to ourself
    HDSA                _dsaCookies;
};



/*************************************************************************************************

// WARNING - WARNING
// This is not a COM object and only exists in shdoc401 because the IE4 shell acts very devient by:
// 1. QIing CWebViewFolderContents for an IExpDispSupport interface.
// 2. Calling IExpDispSupport->FindCConnectionPoint(DIID_DShellFolderViewEvents, pccp) to get a CConnectionPoint pointer (pccp)
// 3. Calling (CConnectionPoint *) pccp->DoInvokeIE4();
//
// CMyIE4ConnectionPoint exists so CWebViewFolderContents can provide this support for this screwed
// up code.  This one single code path works but doing anything else will assert and do nothing.  If
// someone tries to expand the use of 'IExpDispSupport', 'CConnectionPoint', 'CMyIE4ConnectionPoint',
// I will hunt them down and hurt them.  -BryanSt

*************************************************************************************************/

class CMyIE4ConnectionPoint : public CIE4ConnectionPoint
{
public:
    CMyIE4ConnectionPoint(IUnknown *punk) {m_punk = punk;};

    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) {ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return 0; };
    virtual STDMETHODIMP_(ULONG) Release(void) { return 0; };

    // IConnectionPoint methods
    //
    virtual STDMETHODIMP GetConnectionInterface(IID FAR* pIID) {ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer FAR* FAR* ppCPC) {ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP Advise(LPUNKNOWN pUnkSink, ULONG_PTR FAR* pdwCookie) {ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP Unadvise(ULONG_PTR dwCookie) {ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP EnumConnections(LPENUMCONNECTIONS FAR* ppEnum) {ASSERT(0); return E_NOTIMPL;};

    // This is how you actually fire the events
    // Those called by shell32 are virtual
    // (Renamed to DoInvokeIE4)
    virtual HRESULT DoInvokeIE4(LPBOOL pf, LPVOID *ppv, DISPID dispid, DISPPARAMS *pdispparams);

    // This helper function does work that callers of DoInvoke often need done
    // Curiously, shell32 links to this function but never calls it
    virtual HRESULT DoInvokePIDLIE4(DISPID dispid, LPCITEMIDLIST pidl, BOOL fCanCancel) {ASSERT(0); return E_NOTIMPL;};

private:
    IUnknown *m_punk;               // Our parent who hands all the real requests
};


#endif // __DVOC_NEW_H__
