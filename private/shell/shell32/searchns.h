#ifndef __NAMESPACEOC_H__
#define __NAMESPACEOC_H__

#include "unicpp/stdafx.h"
#include <mshtmdid.h>
#include <mshtml.h>
#include "atldisp.h"
#include "combooc.h"
#include "ids.h"

class CSearchNameSpaceOC;
#define SEARCHNAMESPACEID_MAX              SEARCHNAMESPACEID_MYNETWORKPLACES
#define SEARCHNAMESPACE_WINDOWCLASS        TEXT("ActiveX Control Search Name Space ComboBoxEx")

//-------------------------------------------------------------------------//
//  class CSearchNameSpaceOC
//
//  Note: derivation from CShell32AtlIDispatch<> instead of IOleObjectImpl<> is
//  necessary when this object resides in shell32.dll
class ATL_NO_VTABLE CSearchNameSpaceOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CSearchNameSpaceOC, &CLSID_SearchNameSpaceOC>
                    , public CComControl<CSearchNameSpaceOC>
//                    , public IDispatchImpl<IComboBoxExOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IDispatchImpl<ISearchNameSpaceOC, &IID_ISearchNameSpaceOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IProvideClassInfo2Impl<&CLSID_SearchNameSpaceOC, NULL, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public IPersistImpl<CSearchNameSpaceOC>
                    , public IOleControlImpl<CSearchNameSpaceOC>
                    , public IViewObjectExImpl<CSearchNameSpaceOC>
                    , public IOleInPlaceActiveObjectImpl<CSearchNameSpaceOC>
                    , public IObjectSafetyImpl<CSearchNameSpaceOC>
                    , public IConnectionPointContainerImpl<CSearchNameSpaceOC>
                    , public IPersistPropertyBagImpl<CSearchNameSpaceOC>
                    , public IQuickActivateImpl<CSearchNameSpaceOC>
                    , public IOleInPlaceObject
                    , public CProxy_ComboBoxExEvents<CSearchNameSpaceOC>
                    , public CShell32AtlIDispatch<CSearchNameSpaceOC, &CLSID_SearchNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
                    , public CComboBoxExOC
{
public:
    CSearchNameSpaceOC();
    virtual ~CSearchNameSpaceOC();

    DECLARE_POLY_AGGREGATABLE(CSearchNameSpaceOC);
    DECLARE_NO_REGISTRY();
    DECLARE_WND_CLASS(SEARCHNAMESPACE_WINDOWCLASS)

BEGIN_COM_MAP(CSearchNameSpaceOC)
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
    COM_INTERFACE_ENTRY(ISearchNameSpaceOC)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IPersist)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CSearchNameSpaceOC)
    // ATL Uses these to implement the Connection point code
    CONNECTION_POINT_ENTRY(DIID_DComboBoxExEvents)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CSearchNameSpaceOC)
    MESSAGE_HANDLER(WM_SIZE, _ForwardMessage) 
    MESSAGE_HANDLER(WM_SETFOCUS, _OnFocus) 
    COMMAND_HANDLER(FCIDM_VIEWADDRESS, CBN_DROPDOWN, _OnDropDownMessage) 
    COMMAND_HANDLER(FCIDM_VIEWADDRESS, CBN_SELCHANGE, _OnSelectChangeMessage) 
//    NOTIFY_HANDLER(FCIDM_VIEWADDRESS, CBEN_BEGINEDIT, _DisableIconMessage) 
//    NOTIFY_HANDLER(FCIDM_VIEWADDRESS, CBEN_ENDEDIT, _EnableIconMessage) 
    NOTIFY_HANDLER(FCIDM_VIEWADDRESS, CBEN_DELETEITEM, _OnDeleteItemMessage) 
END_MSG_MAP()
 
BEGIN_PROPERTY_MAP(CSearchNameSpaceOC)
END_PROPERTY_MAP()

    // *** IComboBoxExOC ***
    virtual STDMETHODIMP get_String(OUT BSTR *pbs);
    virtual STDMETHODIMP put_String(IN BSTR bs);
    virtual STDMETHODIMP get_Enabled(OUT VARIANT_BOOL * pfEnabled) {return CComboBoxExOC::get_Enabled(pfEnabled);};
    virtual STDMETHODIMP put_Enabled(IN VARIANT_BOOL fEnabled) {return CComboBoxExOC::put_Enabled(fEnabled);};
    virtual STDMETHODIMP Reset(void) {return _SetDefaultSelect();};

    // *** ISearchNameSpaceOC ***
    virtual STDMETHODIMP get_IsValidSearch(OUT VARIANT_BOOL * pfEnabled);

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CSearchNameSpaceOC>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CSearchNameSpaceOC>::ContextSensitiveHelp(fEnterMode); };

    // *** IOleObject ***
    virtual STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);

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
    virtual STDMETHODIMP TranslateAccelerator(MSG *pMsg) { return CShell32AtlIDispatch<CSearchNameSpaceOC, &CLSID_SearchNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::TranslateAcceleratorPriv(this, pMsg, m_spClientSite); };

    // Over ride ATL functions.
    LRESULT _ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) { return CComboBoxExOC::cb_ForwardMessage(uMsg, wParam, lParam, bHandled); };
    LRESULT _OnDropDownMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled) { return CComboBoxExOC::cb_OnDropDownMessage(wNotifyCode, wID, hWndCtl, bHandled); };
    LRESULT _OnSelectChangeMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
//    LRESULT _DisableIconMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
//    LRESULT _EnableIconMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT _OnDeleteItemMessage(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName = NULL, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, DWORD dwExStyle = 0, UINT_PTR nID = 0);
    HWND _Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID) { return CWindowImpl<CSearchNameSpaceOC>::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID); };

    LRESULT _OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent) { return CShell32AtlIDispatch<CSearchNameSpaceOC, &CLSID_SearchNameSpaceOC, &IID_IComboBoxExOC, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::DoVerbUIActivate(prcPosRect, hwndParent, _hwndComboBox); };
    static LRESULT CALLBACK SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData);

    virtual STDMETHODIMP PrivateQI(REFIID iid, void ** ppvObject) { return _InternalQueryInterface(iid, ppvObject);};
    HRESULT IOleInPlaceObject_InPlaceDeactivate(void);  // Use this time to persist state.
    HRESULT _PrivateActivate(void);


protected:
    // Helper functions;

    virtual HRESULT _Populate(void);
    void Fire_EnterPressed(void) {this->CProxy_ComboBoxExEvents<CSearchNameSpaceOC>::Fire_EnterPressed();};
    BOOL _FGetStartingPidl(void);


    BOOL _IsSecure(void);
    BOOL _AddPath(LPCTSTR pszPath);
    HRESULT _GetSearchUrlW(LPWSTR pwzUrl, DWORD cchSize);
    HRESULT _AddDefaultItem(void);
    HRESULT _AddBrowseItem(void);
    HRESULT _BrowseForDirectory(LPTSTR pszPath, DWORD cchSize);
    HRESULT _SetDefaultSelect(void);

    HRESULT _AddDrives(void) { return _EnumSpecialItemIDs(CSIDL_DRIVES, (SHCONTF_FOLDERS|SHCONTF_NONFOLDERS), _AddMappedDrivesCB, this); };
    
    static HRESULT _EnumRecentAndGenPathCB( LPCTSTR pszPath, BOOL fAddEntries, LPVOID pvParam ) ;

    HRESULT _AddMyNetworkPlacesItems(void);
    HRESULT _CustomizeName(UINT idString, LPTSTR szDisplayName, DWORD dwSize);
    //HRESULT _BuildDrivesList(UINT uiFilter, LPCTSTR pszSeparator, LPCTSTR pszEnd, LPTSTR pszString, DWORD cchSize);
    HRESULT _GetSelectText(LPTSTR pszSelectText, DWORD cchSize, BOOL fDisplayName);
    HRESULT _AddDocumentFolders(void);
    HRESULT _AddMyComputer(void);
    HRESULT _AddLocalHardDrives(void);
    HRESULT _AddRecentFolderAndEntries(BOOL fAddEntries);
    HRESULT _AddMyNetworkPlaces(void);
    HRESULT _IsDefaultSelection(LPCTSTR pszLastSelection);
    HRESULT _RestoreIfUserInputValue(LPCTSTR pszLastSelection);
    void    _UpdateDeferredPathFromPathList(void);
    LRESULT _GetCurItemTextAndIndex( BOOL fPath, LPTSTR psz, int cch);

    HRESULT _AddNethoodDirs(LPITEMIDLIST pidl);
    HRESULT _AddMappedDrives(LPITEMIDLIST pidl);
    HRESULT _ReallyEnumRecentFolderAndEntries(BOOL fAddEntries);
    BOOL    _SetupBrowserCP(void);

    static HRESULT _AddNethoodDirsCB(LPITEMIDLIST pidl, LPVOID pvThis) { CSearchNameSpaceOC * pThis = (CSearchNameSpaceOC *) pvThis; return pThis->_AddNethoodDirs(pidl); };
    static HRESULT _AddMappedDrivesCB(LPITEMIDLIST pidl, LPVOID pvThis) { CSearchNameSpaceOC * pThis = (CSearchNameSpaceOC *) pvThis; return pThis->_AddMappedDrives(pidl); };

    // Private Member Variables
    BITBOOL             _fPopulated : 1;            // Have we populated yet?
    BITBOOL             _fFileSysAutoComp : 1;      // AutoComplete in the File System?
    BITBOOL             _fDeferPidlStart:1;         // Could not reliably check do it later
    BITBOOL             _fDeferPidlStartTried:1;    // We tried before and it failed don't try again
    LPTSTR              _pszPathList;               // Since this is a list of paths it will be larger than MAX_PATH at times.
    INT_PTR             _iDeferPathList;            // remember which item we added for deferred setting of strings.
    INT_PTR             _iLocalDisk;                // The index in the list of local disks...
    LPITEMIDLIST        _pidlStart;                 // Starting pidl...

    // BUGBUG:: add internal class to setup a connection point to browser to use
    // if we find out if we were loaded as part of the CreateInstance and not explicitly
    // as any args that the brower may want to pass in may not be setup yet...

    // Internal class to handle notifications from top level browser
    class CWBEvents2: public DWebBrowserEvents
    {
    public:
        STDMETHOD(QueryInterface) (REFIID riid, void **ppvObject);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
    
        // *** (DwebBrowserEvents)IDispatch methods ***
        STDMETHOD(GetTypeInfoCount)(UINT * pctinfo)
                { return E_NOTIMPL;}
        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
                { return E_NOTIMPL;}
        STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
                { return E_NOTIMPL;}
        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

        // Some helper functions...
        void SetOwner(CSearchNameSpaceOC *pcdfc)
            { _pcsns = pcdfc; }  // Don't addref as part of larger object... }
        void SetWaiting(BOOL fWait)
            {_fWaitingForNavigate = fWait;}

    protected:
        // Internal variables...
        CSearchNameSpaceOC      *_pcsns;    // pointer to top object... could cast, but...
        BOOL            _fWaitingForNavigate;   // Are we waiting for the navigate to search resluts?
    };

    friend class CWBEvents2;
    CWBEvents2              _cwbe;
    IConnectionPoint        *_pcpBrowser;   // hold onto browsers connection point;
    unsigned long           _dwCookie;      // Cookie returned by Advise
};


#endif // __NAMESPACEOC_H__
