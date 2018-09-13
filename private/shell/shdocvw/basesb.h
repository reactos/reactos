#ifndef _BASESB2_H
#define _BASESB2_H

#include "iface.h"
#include "trsite.h"
#include "track.h"
#include "fldset.h"
#include <vervec.h>
#include <iethread.h>
#include "browsext.h"


//
//  this is used to identify the top frame browsers dwBrowserIndex
#define BID_TOPFRAMEBROWSER   ((DWORD)-1)

void IECleanUpAutomationObject(void);

#define CBASEBROWSER CBaseBrowser2
class CBaseBrowser2 : public CAggregatedUnknown 
                   , public IShellBrowser
                   , public IBrowserService2
                   , public IServiceProvider
                   , public IOleCommandTarget
                   , public IOleContainer
#ifdef LIGHT_FRAMES
                   , public IOleInPlaceFrame
#else
                   , public IOleInPlaceUIWindow
#endif
                   , public IAdviseSink
                   , public IDropTarget
                   , public IInputObjectSite
                   , public IDocNavigate
                   , public IPersistHistory
                   , public IInternetSecurityMgrSite
                   , public IVersionHost
#ifdef LIGHT_FRAMES
                   , public IOleClientSite
                   , public IOleInPlaceSite
#endif
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) {return CAggregatedUnknown::QueryInterface(riid, ppvObj);};
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};

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
    virtual STDMETHODIMP GetViewStateStream(DWORD grfMode, LPSTREAM  *ppStrm) {return E_NOTIMPL; };
    virtual STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd);
    virtual STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    virtual STDMETHODIMP QueryActiveShellView(struct IShellView ** ppshv);
    virtual STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv);
    virtual STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // IOleInPlaceUIWindow (also IOleWindow)
    virtual STDMETHODIMP GetBorder(LPRECT lprectBorder);
    virtual STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
    virtual STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
    virtual STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IOleContainer
    virtual STDMETHODIMP ParseDisplayName(IBindCtx  *pbc, LPOLESTR pszDisplayName, ULONG  *pchEaten, IMoniker  **ppmkOut);
    virtual STDMETHODIMP EnumObjects(DWORD grfFlags, IEnumUnknown **ppenum);
    virtual STDMETHODIMP LockContainer( BOOL fLock);

    // IBrowserService
    virtual STDMETHODIMP GetParentSite(struct IOleInPlaceSite** ppipsite);
    virtual STDMETHODIMP SetTitle(IShellView *psv, LPCWSTR pszName);
    virtual STDMETHODIMP GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName);
    virtual STDMETHODIMP GetOleObject(struct IOleObject** ppobjv);

    virtual STDMETHODIMP GetTravelLog(ITravelLog **pptl);
    virtual STDMETHODIMP ShowControlWindow(UINT id, BOOL fShow);
    virtual STDMETHODIMP IsControlWindowShown(UINT id, BOOL *pfShown);
    virtual STDMETHODIMP IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags);
    virtual STDMETHODIMP IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut);
    virtual STDMETHODIMP DisplayParseError(HRESULT hres, LPCWSTR pwszPath);
    virtual STDMETHODIMP NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF);
    virtual STDMETHODIMP SetNavigateState(BNSTATE bnstate);
    virtual STDMETHODIMP GetNavigateState(BNSTATE *pbnstate);
    virtual STDMETHODIMP UpdateWindowList(void);
    virtual STDMETHODIMP UpdateBackForwardState(void);
    virtual STDMETHODIMP NotifyRedirect(IShellView* psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse);
    virtual STDMETHODIMP SetFlags(DWORD dwFlags, DWORD dwFlagMask);
    virtual STDMETHODIMP GetFlags(DWORD *pdwFlags);
    virtual STDMETHODIMP CanNavigateNow(void);
    virtual STDMETHODIMP GetPidl(LPITEMIDLIST *ppidl);
    virtual STDMETHODIMP SetReferrer(LPITEMIDLIST pidl);
    virtual STDMETHODIMP_(DWORD) GetBrowserIndex(void);
    virtual STDMETHODIMP GetBrowserByIndex(DWORD dwID, IUnknown **ppunk);
    virtual STDMETHODIMP GetHistoryObject(IOleObject **ppole, IStream **ppstm, IBindCtx **ppbc);
    virtual STDMETHODIMP SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor);
    virtual STDMETHODIMP CacheOLEServer(IOleObject *pole);
    virtual STDMETHODIMP GetSetCodePage(VARIANT* pvarIn, VARIANT* pvarOut);
    virtual STDMETHODIMP OnHttpEquiv(IShellView* psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut);
    virtual STDMETHODIMP GetPalette( HPALETTE * hpal );
    virtual STDMETHODIMP RegisterWindow(BOOL fUnregister, int swc) {return E_NOTIMPL;}
    virtual STDMETHODIMP_(LRESULT) WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP OnSize(WPARAM wParam);
    virtual STDMETHODIMP OnCreate(LPCREATESTRUCT pcs);
    virtual STDMETHODIMP_(LRESULT) OnCommand(WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP OnDestroy();
    virtual STDMETHODIMP ReleaseShellView();
    virtual STDMETHODIMP ActivatePendingView();
    virtual STDMETHODIMP_(LRESULT) OnNotify(NMHDR * pnm);
    virtual STDMETHODIMP OnSetFocus();
    virtual STDMETHODIMP OnFrameWindowActivateBS(BOOL fActive);
    virtual STDMETHODIMP SetTopBrowser();
    virtual STDMETHODIMP UpdateSecureLockIcon(int eSecureLock);
    virtual STDMETHODIMP Offline(int iCmd);
    virtual STDMETHODIMP SetActivateState(UINT uActivate) { _bbd._uActivateState = uActivate; return S_OK;};
    virtual STDMETHODIMP AllowViewResize(BOOL f) { HRESULT hres = _fDontResizeView ? S_FALSE : S_OK; _fDontResizeView = !BOOLIFY(f); return hres;};
    virtual STDMETHODIMP InitializeDownloadManager();
    virtual STDMETHODIMP InitializeTransitionSite();
    virtual STDMETHODIMP CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd);
    virtual STDMETHODIMP GetFolderSetData(struct tagFolderSetData*) { ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP CreateBrowserPropSheetExt(REFIID, LPVOID *) { ASSERT(0); return E_NOTIMPL;};
    virtual STDMETHODIMP GetBaseBrowserData( LPCBASEBROWSERDATA* ppbd ) { *ppbd = &_bbd; return S_OK; };
    virtual STDMETHODIMP_(LPBASEBROWSERDATA) PutBaseBrowserData() { return &_bbd; };

    virtual STDMETHODIMP SetAsDefFolderSettings() { TraceMsg(TF_ERROR, "CBaseBrowser2::SetAsDefFolderSettings called, returned E_NOTIMPL"); return E_NOTIMPL;};
    virtual STDMETHODIMP GetViewRect(RECT* prc);
    virtual STDMETHODIMP GetViewWindow(HWND * phwndView);
    virtual STDMETHODIMP InitializeTravelLog(ITravelLog* ptl, DWORD dw);
    virtual STDMETHODIMP _Initialize(HWND hwnd, IUnknown *pauto);

#if 1 // BUGBUG split: temporary
    virtual STDMETHODIMP_(UINT) _get_itbLastFocus() { ASSERT(0); return ITB_VIEW; };
    virtual STDMETHODIMP _put_itbLastFocus(UINT itbLastFocus) { ASSERT(0); return E_NOTIMPL; };
#endif
    // see _UIActivateView, below
    
    // BEGIN REVIEW:  review names and need of each.  
    // 
    // this first set could be basebrowser only members.  no one overrides
    virtual STDMETHODIMP _CancelPendingNavigationAsync() ;
    virtual STDMETHODIMP _CancelPendingView() ;
    virtual STDMETHODIMP _MaySaveChanges() ; 
    virtual STDMETHODIMP _PauseOrResumeView( BOOL fPaused) ;
    virtual STDMETHODIMP _DisableModeless() ;
    
    // rethink these... are all of these necessary?
    virtual STDMETHODIMP _NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    virtual STDMETHODIMP _TryShell2Rename( IShellView* psv, LPCITEMIDLIST pidlNew);
    virtual STDMETHODIMP _SwitchActivationNow( );

    
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

    // Notes: Only CDesktopBrowser may sublcass this.
    virtual STDMETHODIMP _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);

    //END REVIEW:

    // CDesktopBrowser accesses CCommonBrowser implementations of these:
    virtual STDMETHODIMP_(IStream*) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName) { ASSERT(FALSE); return NULL; }
    virtual STDMETHODIMP_(LRESULT) ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { ASSERT(FALSE); return 0; }
    virtual STDMETHODIMP SetAcceleratorMenu(HACCEL hacc) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP_(int) _GetToolbarCount(THIS) { ASSERT(FALSE); return 0; }
    virtual STDMETHODIMP_(LPTOOLBARITEM) _GetToolbarItem(THIS_ int itb) { ASSERT(FALSE); return NULL; }
    virtual STDMETHODIMP _SaveToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _LoadToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _CloseAndReleaseToolbars(BOOL fClose) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM * pptbi, HWND * phwnd) { ASSERT(FALSE); return E_NOTIMPL; };
    virtual STDMETHODIMP _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP_(UINT) _FindTBar(IUnknown* punkSrc) { ASSERT(FALSE); return (UINT)-1; };
    virtual STDMETHODIMP _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP v_MayTranslateAccelerator(MSG* pmsg) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _GetBorderDWHelper(IUnknown* punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) { ASSERT(FALSE); return E_NOTIMPL; }

    // CShellBrowser overrides this.
    virtual STDMETHODIMP v_CheckZoneCrossing(LPCITEMIDLIST pidl) {return S_OK;};

    // IServiceProvider
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // IAdviseSink
    virtual STDMETHODIMP_(void) OnDataChange(FORMATETC *, STGMEDIUM *);
    virtual STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
    virtual STDMETHODIMP_(void) OnRename(IMoniker *);
    virtual STDMETHODIMP_(void) OnSave();
    virtual STDMETHODIMP_(void) OnClose();

    // *** IDropTarget ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IInputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    // IDocNavigate
    virtual STDMETHODIMP OnReadyStateChange(IShellView* psvSource, DWORD dwReadyState);
    virtual STDMETHODIMP get_ReadyState(DWORD * pdwReadyState);

    // *** IPersist
    virtual STDMETHODIMP GetClassID(CLSID *pclsid);

    // *** IPersistHistory
    virtual STDMETHODIMP LoadHistory(IStream *pStream, IBindCtx *pbc);
    virtual STDMETHODIMP SaveHistory(IStream *pStream);
    virtual STDMETHODIMP SetPositionCookie(DWORD dwPositionCookie);
    virtual STDMETHODIMP GetPositionCookie(DWORD *pdwPositioncookie);

    // *** IInternetSecurityMgrSite methods ***
    // virtual STDMETHODIMP GetWindow(HWND * lphwnd) { return IOleWindow::GetWindow(lphwnd); }
    virtual STDMETHODIMP EnableModeless(BOOL fEnable) { return EnableModelessSB(fEnable); }

    // *** IVersionHost methods ***
    virtual STDMETHODIMP QueryUseLocalVersionVector( BOOL *fUseLocal);
    virtual STDMETHODIMP QueryVersionVector( IVersionVector *pVersion);


#ifdef LIGHT_FRAMES
    // *** IOleClientSite
    virtual STDMETHODIMP SaveObject( void);
    virtual STDMETHODIMP GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    virtual STDMETHODIMP GetContainer( IOleContainer **ppContainer);
    virtual STDMETHODIMP ShowObject( void);
    virtual STDMETHODIMP OnShowWindow( BOOL fShow);
    virtual STDMETHODIMP RequestNewObjectLayout( void);

    // *** IOleInPlaceSite
    virtual STDMETHODIMP CanInPlaceActivate( void);
    virtual STDMETHODIMP OnInPlaceActivate( void);
    virtual STDMETHODIMP OnUIActivate( void);
    virtual STDMETHODIMP GetWindowContext( IOleInPlaceFrame **ppFrame, 
                                           IOleInPlaceUIWindow **ppDoc,
                                           LPRECT lprcPosRect,
                                           LPRECT lprcClipRect,
                                           LPOLEINPLACEFRAMEINFO lpFrameInfo);
    virtual STDMETHODIMP Scroll( SIZE scrollExtant);
    virtual STDMETHODIMP OnUIDeactivate( BOOL fUndoable);
    virtual STDMETHODIMP OnInPlaceDeactivate( void);
    virtual STDMETHODIMP DiscardUndoState( void);
    virtual STDMETHODIMP DeactivateAndUndo( void);
    virtual STDMETHODIMP OnPosRectChange( LPCRECT lprcPosRect);

    // *** IOleInPlaceFrame
    virtual STDMETHODIMP InsertMenus( HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
        { return InsertMenusSB(hmenuShared, lpMenuWidths); }
    virtual STDMETHODIMP SetMenu( HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
        { return SetMenuSB(hmenuShared, holemenu, hwndActiveObject); }
    virtual STDMETHODIMP RemoveMenus( HMENU hmenuShared)
        { return RemoveMenusSB(hmenuShared); }
    virtual STDMETHODIMP SetStatusText( LPCOLESTR pszStatusText)
        { return SetStatusTextSB(pszStatusText); }
//    virtual STDMETHODIMP EnableModeless( BOOL fEnable)
//        { return EnableModelessSB(fEnable); }
    virtual STDMETHODIMP TranslateAccelerator( LPMSG lpmsg, WORD wID)
        { return TranslateAcceleratorSB(lpmsg, wID); }
#endif

    // This is the QueryInterface the aggregator implements
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);

protected:

    // "protected" so derived classes can construct/destruct us too
    CBaseBrowser2(IUnknown* punkAgg);   
    virtual ~CBaseBrowser2();
    
    friend HRESULT CBaseBrowser2_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend HRESULT CBaseBrowser2_Validate(HWND hwnd, LPVOID* ppsb);

    // topmost CBaseBrowser2 in a frameset (IE3/AOL/CIS/VB)
    virtual void        _OnNavigateComplete(LPCITEMIDLIST pidl, DWORD grfHLNF);
    virtual HRESULT     _CheckZoneCrossing(LPCITEMIDLIST pidl);
    virtual void        _PositionViewWindow(HWND hwnd, LPRECT prc);
    virtual LRESULT     _DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void        _ViewChange(DWORD dwAspect, LONG lindex);
    virtual void        _UpdateBackForwardState();
    virtual BOOL        v_OnSetCursor(LPARAM lParam);
    virtual STDMETHODIMP v_ShowHideChildWindows(BOOL fChildOnly);
    virtual void        v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend);
    virtual HRESULT     _ShowBlankPage(LPCTSTR pszAboutUrl, LPCITEMIDLIST pidlIntended);
    
    // ViewStateStream related
    
    HRESULT     _CheckInCacheIfOffline(LPCITEMIDLIST pidl, BOOL fIsAPost);
    void        _CreateShortcutOnDesktop(BOOL fUI);
    void        _AddToFavorites(LPCITEMIDLIST pidl, LPCTSTR pszTitle, BOOL fDisplayUI);

    // to avoid having to pass hwnd on every message to WndProc, set it once
    void        _SetWindow(HWND hwnd) { _bbd._hwnd = hwnd; }
    void        _DoOptions(VARIANT* pvar);
    LRESULT     _OnGoto(void);
    void        _NavigateToPidlAsync(LPITEMIDLIST pidl, DWORD dwSBSP, BOOL fDontCallCancel = FALSE);
    BOOL        _CanNavigate(void);
    // inline so that lego will get the right opt.
    void        _PreActivatePendingViewAsync(void) {
        ASSERT(_bbd._psvPending);
        _StopAsyncOperation();
    };
    BOOL        _ActivatePendingViewAsync(void);
    void        _FreeQueuedPidl(LPITEMIDLIST* ppidl);
    void        _StopAsyncOperation(void);
    void        _MayUnblockAsyncOperation();
    BOOL        _PostAsyncOperation(UINT uAction);
    LRESULT     _SendAsyncOperation(UINT uAction);
    void        _SendAsyncNavigationMsg(VARIANTARG *pvarargIn);
    HRESULT     _OnCoCreateDocument(VARIANTARG *pvarargOut);
    void        _NotifyCommandStateChange();
    BOOL 		_IsViewMSHTML(IShellView *psv);

    void        _Exec_psbMixedZone();

#ifdef TEST_AMBIENTS
    BOOL        _LocalOffline(int iCmd);
    BOOL        _LocalSilent(int iCmd);
#endif // TEST_AMBIENTS
    
    #define NAVTYPE_ShellNavigate   0x01
    #define NAVTYPE_PageIsChanging  0x02
    #define NAVTYPE_SiteIsChanging  0x04

    void        _GetNavigationInfo(WORD * pwNavTypeFlags);

    virtual void _MayPlayTransition(IShellView* psvNew, HWND hwndViewNew, BOOL bSiteChanging);

    void        _EnableStop(BOOL fEnable);
    LRESULT     _OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu);
    HRESULT     _updateNavigationUI();
    HRESULT     _setDescendentNavigate(VARIANTARG *pvarargIn);
    HRESULT     _FireBeforeNavigateEvent(LPCITEMIDLIST pidl, BOOL* pfUseCache);

    HRESULT     _OpenNewFrame(LPITEMIDLIST pidlNew, UINT wFlags);
    STDMETHODIMP    _UIActivateView(UINT uState);
    HRESULT     _CancelPendingNavigation(BOOL fDontReleaseState = FALSE);
    void        _StopCurrentView(void);

    void        _MayTrackClickStream(LPITEMIDLIST pidlThis);        // (peihwal)

    virtual STDMETHODIMP _OnFocusChange(UINT itb);

    void        _RegisterAsDropTarget();
    void        _UnregisterAsDropTarget();
    
    
    enum BrowserPaletteType
    {
        BPT_DeferPaletteSupport = 0,    // we don't think we own the palette
        BPT_UnknownDisplay,             // need to decide if we need a palette
        BPT_DisplayViewChanged,         // BPT_UnknownDisplay handling notify
        BPT_UnknownPalette,             // need to decide what palette to use
        BPT_PaletteViewChanged,         // BPT_UnknownPalette handling notify
        BPT_Normal,                     // handle WM_QUERYNEWPALETTE ourselves
        BPT_ShellView,                  // forward WM_QUERYNEWPALETTE to view
        BPT_NotPalettized               // not a palettized display, do nothing
    };
    
    void            _ColorsDirty(BrowserPaletteType bptNew);
    void            _DisplayChanged(WPARAM wParam, LPARAM lParam);
    HRESULT         _UpdateBrowserPaletteInPlace(LOGPALETTE *plp);
    void            _RealizeBrowserPalette(BOOL fBackground);
    virtual void    _PaletteChanged(WPARAM wParam, LPARAM lParam);
    BOOL            _QueryNewPalette();

    /// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
    void    _CheckDisableViewWindow();
    BOOL    _SubclassDefview();
    static LRESULT DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC _pfnDefView;
    /// END-CHC- Security fix for viewing non shdocvw ishellviews
    
    void            _DLMDestroy(void);
    void            _DLMUpdate(MSOCMD* prgCmd);
    void            _DLMRegister(IUnknown* punk);

    void            CreateNewSyncShellView( void );

    void            _UpdateTravelLog(void);

    virtual BOOL    _HeyMoe_IsWiseGuy(void) {return FALSE;}

    IBrowserService2*    _pbsOuter;
    IShellBrowser*       _psbOuter;
    IServiceProvider*    _pspOuter;
    IDockingWindowSite*  _pdwsOuter;
    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //IOleCommandTarget* _pctOuter;
    //IInputObjectSite*  _piosOuter;

    BASEBROWSERDATA _bbd;
    IUnknown *_pauto;

    BrowserPaletteType  _bptBrowser;
    HPALETTE            _hpalBrowser;

    IViewObject *_pvo;  // view object implementation on the shell view
    UINT  _cRefUIActivateSV;

    DWORD  _dwBrowserIndex;
    DWORD       _dwReadyState;

    DWORD       _dwReadyStateCur;
    LPWSTR      _pszTitleCur;
    
    IDropTarget * _pdtView; // Pointer to _bbd._psv's IDropTarget interface
    

    IOleObject *_poleHistory;
    IStream    *_pstmHistory;
    IBindCtx   *_pbcHistory;

    IOleInPlaceActiveObject *_pact;     // for UIWindow

    IClassFactory* _pcfHTML;            // cached/locked class factory

    
    DWORD       _dwReadyStatePending;
    LPWSTR      _pszTitlePending;
    DWORD       _grfHLNFPending;
    HDPA        _hdpaDLM;           // downloading object (for DLM)
    BOOL        _cp;                // current codepage

    //
    // NOTES: Currently, we support only one pending navigation.
    //  If we want to support queued navigation, we need to turn
    //  following two variables into a queue. (SatoNa)
    //
    DWORD       _uActionQueued;       // queued action
    LPITEMIDLIST _pidlQueued;         // pidl to go asynchronously
    DWORD       _dwSBSPQueued;        // grfHLNF to go asynchronously

    UINT        _cRefCannotNavigate;  // Increment when we can navigate

    RECT _rcBorderDoc;                  // for UIWindow

    BITBOOL     _fDontResizeView : 1; // Don't resize _hwndView
    BITBOOL     _fNavigate:1;       // are we navigating?
    BITBOOL     _fDescendentNavigate:1; // are our descendents navigating?
    BITBOOL     _fDownloadSet:1; // did we invoke download animation?
    BITBOOL     _fNoDragDrop:1;          // TRUE iff we want to register for drops
    BITBOOL     _fRegisteredDragDrop:1;  // TRUE iff we have registered for drops
    BITBOOL     _fNavigatedToBlank: 1;  // Has called _ShowBlankPage once.
    BITBOOL     _fAsyncNavigate:1; // Ignore sync-hack-bug-fix
    BITBOOL     _fPausedByParent :1;    // Interaction paused by parent
    BITBOOL     _fDontAddTravelEntry:1;
    BITBOOL     _fIsLocalAnchor:1;
    BITBOOL     _fGeneratedPage:1;      //  trident told us that the page is generated.
    BITBOOL     _fOwnsPalette:1;        // does the browser own the palette ? (did we get QueryNewPalette ..)
    BITBOOL     _fUsesPaletteCommands : 1; // if we are using a separate communication with trident for palette commands
    BITBOOL     _fCreateViewWindowPending:1;
    BITBOOL     _fReleasingShellView:1; 
    BITBOOL     _fDeferredUIDeactivate:1;
    BITBOOL     _fDeferredSelfDestruction:1;
    BITBOOL     _fActive:1;  // remember if the frame is active or not (WM_ACTIVATE)
    BITBOOL     _fUIActivateOnActive:1; // TRUE iff we have a bending uiactivate
    BITBOOL     _fInQueryStatus:1;
    BITBOOL     _fCheckedDesktopComponentName:1;
    BITBOOL     _fInDestroy:1;          // being destroyed

    // for IDropTarget
    
    DWORD _dwDropEffect;

#ifdef DEBUG
    BOOL        _fProcessed_WM_CLOSE; // TRUE iff WM_CLOSE processed
#endif

    // friend   CIEFrameAuto;
    interface IShellHTMLWindowSupport   *_phtmlWS;  
    

    IUrlHistoryStg *_pIUrlHistoryStg;   // pointer to url history storage object
    
    ITargetFrame2 *_ptfrm;
    
    //  Cached History IShellFolder
    IUnknown *_punkSFHistory;

    //  what SSL icon to show
    int     _eSecureLockIconPending;
    

    // Support for OLECMDID_HTTPEQUIV (Client Pull, PICS, etc)

    HRESULT _HandleHttpEquiv (VARIANT *pvarargIn, VARIANT *pvarargOut, BOOL fDone);
    HRESULT _KillRefreshTimer( void );
    VOID    _OnRefreshTimer(void);
    void    _StartRefreshTimer(void);

    // equiv handlers we know about
    friend HRESULT _HandleRefresh (HWND hwnd, WCHAR *pwz, WCHAR *pwzColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam);
    friend HRESULT _HandlePICS (HWND hwnd, WCHAR *pwz, WCHAR *pwzColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam);
    friend VOID CALLBACK _RefreshTimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    friend HRESULT _HandleViewTransition(HWND hwnd, WCHAR *pwz, WCHAR *pwzColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam);

    // Client Pull values
    WCHAR *_pwzRefreshURL;
    int    _iRefreshTimeout;
    BOOL   _iRefreshTimeoutSet:1;
    INT_PTR _iRefreshTimerID;


#ifdef MESSAGEFILTER
    // COM Message filter used to help dispatch TIMER messages during OLE operations.
    LPMESSAGEFILTER _lpMF;
#endif

    friend CTransitionSite;
    CTransitionSite * _ptrsite;

    CUrlTrackingStg * _ptracking;

    // _fTopBrowser vs. _fNoTopLevelBrowser:
    // _fTopBrowser: True means we are the top most browser, or a top most browser does not exist and we are acting like the top most browser.
    //               In the latter case, the immediate childern of our host will also act like top most browsers.
    // _fNoTopLevelBrowser: This means that the top most item isn't one of our shell browsers, so it's immediate browser child
    //               will act like a top most browser.
    //
    //     In normal cases, a shell browser (CShellBrowser, CDesktopBrowser, ...) is a top most browser
    //   with TRUE==_fTopBrowser and FALSE==_fNoTopLevelBrowser.  It can have subframes that will have
    //   FALSE==_fTopBrowser and FALSE==_fNoTopLevelBrowser.
    //
    //   The only time _fNoTopLevelBrowser is TRUE is if some other object (like Athena) hosts MSHTML directly
    //   which will prevent some shell browser from being top most.  Since the HTML can have several frames,
    //   each will have TRUE==_fTopBrowser, so _fNoTopLevelBrowser will be set to TRUE to distinguish this case.
    BOOL        _fTopBrowser :1;    // Should only be set via the _SetTopBrowser method
    BOOL        _fNoTopLevelBrowser :1;         // TRUE iff the toplevel is a non-shell browser (Athena).  Shell browsers include CDesktopBrowser, CShellBrowser, ...
    BOOL        _fHaveOldStatusText :1;
    
    WCHAR       _szwOldStatusText[MAX_PATH];

    FOLDERSETDATABASE _fldBase; // cache viewset results in here (used when navigating)

    // Manages extended toolbar buttons and tools menu extensions for IE
    IToolbarExt* _pToolbarExt;

public:

    // handling for plugUI shutdown
    // need the hwnd for the lang change modal property sheet
    static HDPA         s_hdpaOptionsHwnd;

    static void         _SyncDPA();
    static int CALLBACK _OptionsPropSheetCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam);

private:
    HRESULT _AddFolderOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh);
    HRESULT _AddInternetOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh);

    // this is private!  it should only be called by _NavigateToPidl

    HRESULT     _CreateNewShellViewPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP);
    HRESULT     _CreateNewShellView(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD grfHLNF);
};

HRESULT _DisplayParseError(HWND hwnd, HRESULT hres, LPCWSTR pwszPath);

#endif // _BASESB2_H
