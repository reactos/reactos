#ifndef COMMONSB_INC_
#define COMMONSB_INC_

#include "caggunk.h"
#include "fldset.h"

#define ITB_ITBAR       0               // index to the Internet Toolbar


typedef struct _ZONESICONNAMECACHE  // Cache for zones icons and display names
{
    HICON hiconZones;
    TCHAR *pszZonesName;
} ZONEICONNAMECACHE, *PZONEICONNAMECACHE;


class CCommonBrowser : 
    public CAggregatedUnknown
   ,public IShellBrowser
   ,public IBrowserService2
   ,public IServiceProvider
   ,public IOleCommandTarget
   ,public IDockingWindowSite
   ,public IDockingWindowFrame
   ,public IInputObjectSite
   ,public IDropTarget
{
public:

    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) {return CAggregatedUnknown::QueryInterface(riid, ppvObj);};
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};


    // *** IBrowserService specific methods ***
    virtual STDMETHODIMP GetParentSite(  IOleInPlaceSite** ppipsite) ;
    virtual STDMETHODIMP SetTitle( IShellView* psv, LPCWSTR pszName) ;
    virtual STDMETHODIMP GetTitle( IShellView* psv, LPWSTR pszName, DWORD cchName) ;
    virtual STDMETHODIMP GetOleObject(  IOleObject** ppobjv) ;

    // think about this one.. I'm not sure we want to expose this -- Chee
    // BUGBUG:: Yep soon we should have interface instead.
    // My impression is that we won't document this whole interface???
    virtual STDMETHODIMP GetTravelLog( ITravelLog** pptl) ;

    virtual STDMETHODIMP ShowControlWindow( UINT id, BOOL fShow) ;
    virtual STDMETHODIMP IsControlWindowShown( UINT id, BOOL *pfShown) ;
    virtual STDMETHODIMP IEGetDisplayName( LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags) ;
    virtual STDMETHODIMP IEParseDisplayName( UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut) ;
    virtual STDMETHODIMP DisplayParseError( HRESULT hres, LPCWSTR pwszPath) ;
    virtual STDMETHODIMP NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF) ;

    virtual STDMETHODIMP SetNavigateState( BNSTATE bnstate) ;
    virtual STDMETHODIMP GetNavigateState ( BNSTATE *pbnstate) ;

    virtual STDMETHODIMP NotifyRedirect (  IShellView* psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse) ;
    virtual STDMETHODIMP UpdateWindowList () ;

    virtual STDMETHODIMP UpdateBackForwardState () ;

    virtual STDMETHODIMP SetFlags( DWORD dwFlags, DWORD dwFlagMask) ;
    virtual STDMETHODIMP GetFlags( DWORD *pdwFlags) ;

    // Tells if it can navigate now or not.
    virtual STDMETHODIMP CanNavigateNow () ;

    virtual STDMETHODIMP GetPidl ( LPITEMIDLIST *ppidl) ;
    virtual STDMETHODIMP SetReferrer ( LPITEMIDLIST pidl) ;
    virtual STDMETHODIMP_(DWORD) GetBrowserIndex() ;
    virtual STDMETHODIMP GetBrowserByIndex( DWORD dwID, IUnknown **ppunk) ;
    virtual STDMETHODIMP GetHistoryObject( IOleObject **ppole, IStream **pstm, IBindCtx **ppbc) ;
    virtual STDMETHODIMP SetHistoryObject( IOleObject *pole, BOOL fIsLocalAnchor) ;

    virtual STDMETHODIMP CacheOLEServer( IOleObject *pole) ;

    virtual STDMETHODIMP GetSetCodePage( VARIANT* pvarIn, VARIANT* pvarOut) ;
    virtual STDMETHODIMP OnHttpEquiv( IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut) ;
    virtual STDMETHODIMP GetPalette(  HPALETTE * hpal ) ;
    virtual STDMETHODIMP RegisterWindow( BOOL fUnregister, int swc) ;
    virtual STDMETHODIMP_(LRESULT) WndProcBS( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) ;
    virtual STDMETHODIMP OnSize(WPARAM wParam);
    virtual STDMETHODIMP OnCreate(LPCREATESTRUCT pcs);
    virtual STDMETHODIMP_(LRESULT) OnCommand(WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP OnDestroy();
    virtual STDMETHODIMP_(LRESULT) OnNotify(NMHDR * pnm);
    virtual STDMETHODIMP OnSetFocus();
    virtual STDMETHODIMP OnFrameWindowActivateBS(BOOL fActive);
    virtual STDMETHODIMP ReleaseShellView( ) ;
    virtual STDMETHODIMP ActivatePendingView( ) ;
    virtual STDMETHODIMP CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd);
    virtual STDMETHODIMP GetBaseBrowserData( LPCBASEBROWSERDATA* ppbd );
    virtual STDMETHODIMP_(LPBASEBROWSERDATA) PutBaseBrowserData();
    virtual STDMETHODIMP SetTopBrowser();
    virtual STDMETHODIMP UpdateSecureLockIcon(int eSecureLock);
    virtual STDMETHODIMP Offline(int iCmd);
    virtual STDMETHODIMP InitializeDownloadManager();
    virtual STDMETHODIMP InitializeTransitionSite();
    virtual STDMETHODIMP GetFolderSetData(struct tagFolderSetData* pfsd) { *pfsd = _fsd; return S_OK; };
    virtual STDMETHODIMP CreateBrowserPropSheetExt(REFIID, LPVOID *);
    virtual STDMETHODIMP SetActivateState(UINT uActivate);
    virtual STDMETHODIMP AllowViewResize(BOOL f);
    virtual STDMETHODIMP _Initialize(HWND hwnd, IUnknown *pauto);
    
    // Temporarily in interface, needs to be brought local
    virtual STDMETHODIMP_(UINT) _get_itbLastFocus() {return _itbLastFocus; };
    virtual STDMETHODIMP _put_itbLastFocus(UINT itbLastFocus) {_itbLastFocus = itbLastFocus; return S_OK;};
    virtual STDMETHODIMP _UIActivateView(UINT uState) ;

    // BEGIN REVIEW:  review names and need of each.  
    // 
    // this first set could be basebrowser only members.  no one overrides
    virtual STDMETHODIMP _CancelPendingNavigationAsync() ;

    virtual STDMETHODIMP _MaySaveChanges() ; 
    virtual STDMETHODIMP _PauseOrResumeView( BOOL fPaused) ;
    virtual STDMETHODIMP _DisableModeless() ;
    
    // rethink these... are all of these necessary?
    virtual STDMETHODIMP _NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    virtual STDMETHODIMP _TryShell2Rename( IShellView* psv, LPCITEMIDLIST pidlNew);
    virtual STDMETHODIMP _SwitchActivationNow( );
    virtual STDMETHODIMP _CancelPendingView() ;

    
    virtual STDMETHODIMP v_MayTranslateAccelerator( MSG* pmsg);
    virtual STDMETHODIMP _CycleFocus( LPMSG lpMsg) ;
    virtual STDMETHODIMP v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM * pptbi, HWND * phwnd);
    virtual STDMETHODIMP _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg);
    virtual STDMETHODIMP_(BOOL) _HasToolbarFocus(void) ;
    virtual STDMETHODIMP _FixToolbarFocus(void) ;

    // this belongs with the toolbar set.
    virtual STDMETHODIMP _ExecChildren(IUnknown *punkBar, BOOL fBroadcast,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    virtual STDMETHODIMP _SendChildren(HWND hwndBar, BOOL fBroadcast,
        UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP _GetViewBorderRect(RECT* prc);

    virtual STDMETHODIMP _UpdateViewRectSize();
    virtual STDMETHODIMP _ResizeNextBorder(UINT itb);
    virtual STDMETHODIMP _ResizeView();

    virtual STDMETHODIMP _GetBorderDWHelper(IUnknown* punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor);

    virtual STDMETHODIMP _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);
    //END REVIEW:

    // for CShellBrowser split
    virtual STDMETHODIMP SetAsDefFolderSettings();
    virtual STDMETHODIMP GetViewRect(RECT* prc);
    virtual STDMETHODIMP GetViewWindow(HWND * phwndView);
    virtual STDMETHODIMP InitializeTravelLog(ITravelLog* ptl, DWORD dw);

    // Desktop needs to override these:
    virtual STDMETHODIMP_(IStream*) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName);
    
    // Desktop needs access to these:
    virtual STDMETHODIMP_(LRESULT) ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP SetAcceleratorMenu(HACCEL hacc);
 
    // Shell browser overrides this.
    virtual STDMETHODIMP v_CheckZoneCrossing(LPCITEMIDLIST pidl) {return S_OK;};

    // *** IDropTarget (delegate to basesb) ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IOleWindow
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    
    // IShellBrowser (same as IOleInPlaceFrame)
    virtual STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    virtual STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd);
    virtual STDMETHODIMP RemoveMenusSB(HMENU hmenuShared);
    virtual STDMETHODIMP SetStatusTextSB(LPCOLESTR lpszStatusText);
    virtual STDMETHODIMP EnableModelessSB(BOOL fEnable);
    virtual STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID);
    virtual STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags);
    virtual STDMETHODIMP GetViewStateStream(DWORD grfMode, LPSTREAM  *ppStrm);
    virtual STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd);
    virtual STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    virtual STDMETHODIMP QueryActiveShellView(struct IShellView ** ppshv);
    virtual STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv);
    virtual STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // IServiceProvider
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IDockingWindowFrame (also IOleWindow(?))
    virtual STDMETHODIMP AddToolbar(IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwReserved);
    virtual STDMETHODIMP RemoveToolbar(IUnknown* punkSrc, DWORD dwFlags);
    virtual STDMETHODIMP FindToolbar(LPCWSTR pwszItem, REFIID riid, LPVOID* ppvObj);

    // IDockingWindowSite (also IOleWindow(?))
    virtual STDMETHODIMP GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder);
    virtual STDMETHODIMP RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);
    virtual STDMETHODIMP SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);

    // IInputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    // This is the QueryInterface the aggregator implements
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);

protected:
    CCommonBrowser(IUnknown* punkAgg);
    virtual ~CCommonBrowser();
    
    friend HRESULT CCommonBrowser_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    //
    // Notes: 
    //  The values in the _arcBorderTools array indicates the size of
    // the border space taken by each toolbar on each side of the
    // containing rectangle.
    //
    virtual STDMETHODIMP_(LPTOOLBARITEM) _GetToolbarItem(int itb);
    virtual STDMETHODIMP_(int) _GetToolbarCount() { return FDSA_GetItemCount(&_fdsaTBar); }
    virtual STDMETHODIMP_(int) _AllocToolbarItem();
    void        _ReleaseToolbarItem(int itb, BOOL fClose);
    
    // Helper function for toolbar negotiation
    virtual STDMETHODIMP_(UINT) _FindTBar(IUnknown* punkSrc);
    virtual STDMETHODIMP _OnFocusChange(UINT itb);
    virtual STDMETHODIMP _CloseAndReleaseToolbars(BOOL fClose = TRUE);

    virtual STDMETHODIMP v_ShowHideChildWindows(BOOL fChildOnly);
    virtual STDMETHODIMP ShowToolbar(IUnknown* punkSrc, BOOL fShow) ;
    virtual STDMETHODIMP _SaveToolbars(IStream* pstm);
    virtual STDMETHODIMP _LoadToolbars(IStream* pstm);

    BOOL _TBWindowHasFocus(UINT itb);
    BOOL _ShouldTranslateAccelerator(MSG* pmsg);

    DWORD _CacheZonesIconsAndNames(BOOL fRefresh);
    IInternetZoneManager * _pizm;

    virtual STDMETHODIMP _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor);

    
    virtual BOOL _CanHandleAcceleratorNow(void) {return TRUE;}
    
    FDSA            _fdsaTBar;
    TOOLBARITEM     _aTBar[ITB_CSTATIC];
    UINT            _itbLastFocus;   // last one called OnFocusChange (can be -1)

    HRESULT _FindActiveTarget(REFIID riid, LPVOID* ppvOut);

    int _cRef;
    IUnknown* _punkInner;

    // implementations in basesb
    IBrowserService2* _pbsInner;
    IShellBrowser* _psbInner;
    IDropTarget* _pdtInner;             // TODO: non-cached?
    IServiceProvider* _pspInner;
    IOleCommandTarget* _pctInner;
    IInputObjectSite* _piosInner;

    // desktop overrides some of these methods
    IBrowserService2* _pbsOuter;
    
    LPCBASEBROWSERDATA _pbbd;

    HACCEL _hacc;
    
    // for view set information
    struct tagFolderSetData _fsd;
    
    virtual HRESULT SetInner(IUnknown* punk);
};


HRESULT     _ConvertPathToPidl(IBrowserService2* pbs, HWND hwnd, LPCTSTR pszPath, LPITEMIDLIST * ppidl);

#endif // COMMONSB_INC_
