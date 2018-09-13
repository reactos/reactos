// stuff to fix:
//  _hwndTray
//  REVIEW, BUGBUG
//
//  questions for chee:
//      why no WinList_Register/WinList_Revoke
//              - because we don't want targeted links and stuff to find us
//                and navigate us
//      need desktop to keep rooted PIDL info (g_psCR)?
//      Desktop_GetRootPidl, used for persisting ROOTed explorers, qualifying
//          for saving view info
//

#include "stdafx.h"
#pragma hdrstop

#include "bitbuck.h"
#include <iethread.h>
#include "apithk.h"

#include "mtpt.h"

#define DM_FOCUS        0           // focus
#define DM_SHUTDOWN     DM_TRACE    // shutdown 
#define TF_SHDAUTO      0
#define DM_MISC         DM_TRACE    // misc/tmp

STDAPI_(void) CheckWinIniForAssocs();

BOOL GetOldWorkAreas(LPRECT lprc, DWORD* pdwNoOfOldWA);
void SaveOldWorkAreas(LPCRECT lprc, DWORD nOldWA);

BOOL UpdateAllDesktopSubscriptions();

//This is in deskreg.cpp
BOOL AdjustDesktopComponents(LPCRECT prcNewWorkAreas, int nNewWorkAreas, 
                             LPCRECT prcOldMonitors, LPCRECT prcOldWorkAreas, int nOldWorkAreas);

// in defview.cpp
BOOL IsFolderWindow(HWND hwnd);

// in deskfldr.cpp
STDAPI_(void) Desktop_InvalidateEnumCache(LPCITEMIDLIST pidl, LONG lEvent);


// copied from tray.c if changing here, change there also
#define GHID_FIRST 500

#define g_cxPrimaryDisplay GetSystemMetrics(SM_CXSCREEN)
#define g_cyPrimaryDisplay GetSystemMetrics(SM_CYSCREEN)
#define g_xVirtualScreen GetSystemMetrics(SM_XVIRTUALSCREEN)
#define g_yVirtualScreen GetSystemMetrics(SM_YVIRTUALSCREEN)
#define g_cxVirtualScreen GetSystemMetrics(SM_CXVIRTUALSCREEN)
#define g_cyVirtualScreen GetSystemMetrics(SM_CYVIRTUALSCREEN)
#define g_cxEdge GetSystemMetrics(SM_CXEDGE)
#define g_cyEdge GetSystemMetrics(SM_CYEDGE)

//  TOID_Desktop is the id for ShellTasks added by the desktop 
GUID TOID_Desktop = { /* 6aec6a60-b7a4-11d1-be89-0000f805ca57 */
    0x6aec6a60,
    0xb7a4,
    0x11d1,
    {0xbe, 0x89, 0x00, 0x00, 0xf8, 0x05, 0xca, 0x57}
  };

// Private interface to talk to explorer.exe
IDeskTray* g_pdtray = NULL;

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#define IDT_DISKFULL        2                   // one timer ID for each possible drive
#define IDT_DISKFULLLAST    (IDT_DISKFULL + 26)

#define DISKFULL_TIMEOUT    (3 * 1000) // 3 seconds

void FireEventSz(LPCTSTR szEvent)
{
    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEvent);
    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
}

#ifdef UNICODE // {
#define FireEventW(pszEvent) FireEventSz(pszEvent)
#else
void FireEventW(LPCWSTR pszEvent)
{
    CHAR szEvent[MAX_IEEVENTNAME];
    SHUnicodeToAnsi(pszEvent, szEvent, ARRAYSIZE(szEvent));
    FireEventSz(szEvent);
}
#endif // }

// MISC stuff duplicated in browseui {
HRESULT _ConvertPathToPidlW(IBrowserService2 *pbs, HWND hwnd, LPCWSTR pszPath, LPITEMIDLIST * ppidl)
{
    HRESULT hres = E_FAIL;
    WCHAR wszCmdLine[MAX_URL_STRING]; // must be with pszPath
    TCHAR szFixedUrl[MAX_URL_STRING];
    TCHAR szParsedUrl[MAX_URL_STRING] = {'\0'};
    DWORD dwUrlLen = ARRAYSIZE(szParsedUrl);

    // Copy the command line into a temporary buffer
    // so we can remove the surrounding quotes (if 
    // they exist)
    SHUnicodeToTChar(pszPath, szFixedUrl, ARRAYSIZE(szFixedUrl));
    PathUnquoteSpaces(szFixedUrl);
    
    if (ParseURLFromOutsideSource(szFixedUrl, szParsedUrl, &dwUrlLen, NULL))
        SHTCharToUnicode(szParsedUrl, wszCmdLine, ARRAYSIZE(wszCmdLine));
    else
        StrCpyNW(wszCmdLine, pszPath, ARRAYSIZE(wszCmdLine));
    
    hres = pbs->IEParseDisplayName(CP_ACP, wszCmdLine, ppidl);
    pbs->DisplayParseError(hres, wszCmdLine);
    return hres;
}
// END of MISC stuff duplicated in browseui }

// Several places rely on the fact that IShellBrowser is the first interface
// we inherit (and therefore is what we use as our canonical IUnknown).
// Grep for IUnknownIdentity to find them.

class CDesktopBrowser :
    public IShellBrowser
   ,public IServiceProvider
   ,public IOleCommandTarget
   ,public IDockingWindowSite
   ,public IInputObjectSite
   ,public IDropTarget
   ,public IDockingWindowFrame
   ,public IMultiMonitorDockingSite
   ,public IBrowserService2
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void * *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IShellBrowser (same as IOleInPlaceFrame)
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
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
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IDockingWindowSite (also IOleWindow)
    virtual STDMETHODIMP GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder);
    virtual STDMETHODIMP RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);
    virtual STDMETHODIMP SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);

    // IInputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    // IDropTarget
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IDockingWindowFrame (also IOleWindow)
    virtual STDMETHODIMP AddToolbar(IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwReserved);
    virtual STDMETHODIMP RemoveToolbar(IUnknown* punkSrc, DWORD dwFlags);
    virtual STDMETHODIMP FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppvObj);

    // IMultiMonitorDockingSite
    virtual STDMETHODIMP GetMonitor(IUnknown* punkSrc, HMONITOR * phMon);
    virtual STDMETHODIMP RequestMonitor(IUnknown* punkSrc, HMONITOR * phMon);
    virtual STDMETHODIMP SetMonitor(IUnknown* punkSrc, HMONITOR hMon, HMONITOR * phMonOld);
    
    static LRESULT CALLBACK DesktopWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK RaisedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void _MessageLoop();
    HWND GetTrayWindow(void) { return _hwndTray; }
    HWND GetDesktopWindow(void) { return _pbbd->_hwnd; }

    // IBrowserService
    // *** IBrowserService specific methods ***
    virtual STDMETHODIMP GetParentSite(  IOleInPlaceSite** ppipsite) ;
    virtual STDMETHODIMP SetTitle( IShellView* psv, LPCWSTR pszName) ;
    virtual STDMETHODIMP GetTitle( IShellView* psv, LPWSTR pszName, DWORD cchName) ;
    virtual STDMETHODIMP GetOleObject(  IOleObject** ppobjv) ;
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
    virtual STDMETHODIMP CanNavigateNow () ;
    virtual STDMETHODIMP GetPidl( LPITEMIDLIST *ppidl) ;
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
    virtual STDMETHODIMP SetAsDefFolderSettings() { ASSERT(FALSE); return E_NOTIMPL;}
    virtual STDMETHODIMP SetTopBrowser();
    virtual STDMETHODIMP UpdateSecureLockIcon(int eSecureLock);
    virtual STDMETHODIMP Offline(int iCmd);
    virtual STDMETHODIMP InitializeDownloadManager();
    virtual STDMETHODIMP InitializeTransitionSite();
    virtual STDMETHODIMP GetFolderSetData(struct tagFolderSetData* pfsd) { *pfsd = _fsd; return S_OK; };
    virtual STDMETHODIMP _OnFocusChange(UINT itb);
    virtual STDMETHODIMP v_ShowHideChildWindows(BOOL fChildOnly);
    virtual STDMETHODIMP CreateBrowserPropSheetExt(THIS_ REFIID riid, void **ppvObj);
    virtual STDMETHODIMP SetActivateState(UINT uActivate);
    virtual STDMETHODIMP AllowViewResize(BOOL f);
    virtual STDMETHODIMP _Initialize(HWND hwnd, IUnknown *pauto);
    virtual STDMETHODIMP_(UINT) _get_itbLastFocus();
    virtual STDMETHODIMP _put_itbLastFocus(UINT itbLastFocus);
    virtual STDMETHODIMP _UIActivateView(UINT uState) ;
    virtual STDMETHODIMP _CancelPendingNavigationAsync() ;
    virtual STDMETHODIMP _MaySaveChanges() ; 
    virtual STDMETHODIMP _PauseOrResumeView( BOOL fPaused) ;
    virtual STDMETHODIMP _DisableModeless() ;
    virtual STDMETHODIMP _NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    virtual STDMETHODIMP _TryShell2Rename( IShellView* psv, LPCITEMIDLIST pidlNew);
    virtual STDMETHODIMP _SwitchActivationNow( );
    virtual STDMETHODIMP _CancelPendingView() ;
    virtual STDMETHODIMP _ExecChildren(IUnknown *punkBar, BOOL fBroadcast,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    virtual STDMETHODIMP _SendChildren(HWND hwndBar, BOOL fBroadcast,
        UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM * pptbi, HWND * phwnd);
    virtual STDMETHODIMP _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _GetViewBorderRect(RECT* prc);
    virtual STDMETHODIMP _UpdateViewRectSize();
    virtual STDMETHODIMP _ResizeNextBorder(UINT itb);
    virtual STDMETHODIMP _ResizeView();
    virtual STDMETHODIMP _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);
    virtual STDMETHODIMP GetCurrentFolderSettings(DEFFOLDERSETTINGS *pdfs, int cbDfs) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP GetViewRect(RECT* prc);
    virtual STDMETHODIMP GetViewWindow(HWND * phwndView);
    virtual STDMETHODIMP InitializeTravelLog(ITravelLog* ptl, DWORD dw);

    // Desktop needs to override these:
    virtual STDMETHODIMP_(IStream*) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName);
    
    // Desktop needs access to these:
    virtual STDMETHODIMP_(LRESULT) ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { ASSERT(FALSE); return 0; };
    virtual STDMETHODIMP SetAcceleratorMenu(HACCEL hacc) { ASSERT(FALSE); return E_FAIL; }
    virtual STDMETHODIMP_(int) _GetToolbarCount(THIS) { ASSERT(FALSE); return 0; }
    virtual STDMETHODIMP_(LPTOOLBARITEM) _GetToolbarItem(THIS_ int itb) { ASSERT(FALSE); return NULL; }
    virtual STDMETHODIMP _SaveToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _LoadToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _CloseAndReleaseToolbars(BOOL fClose) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP_(UINT) _FindTBar(IUnknown* punkSrc) { ASSERT(FALSE); return (UINT)-1; };
    virtual STDMETHODIMP v_MayTranslateAccelerator(MSG* pmsg) { ASSERT(FALSE); return E_NOTIMPL; }
    virtual STDMETHODIMP _GetBorderDWHelper(IUnknown* punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) { ASSERT(FALSE); return E_NOTIMPL; }

    // Shell browser overrides this.
    virtual STDMETHODIMP v_CheckZoneCrossing(LPCITEMIDLIST pidl) {return S_OK;};

    // Desktop and basesb need access to these:
    virtual STDMETHODIMP _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor);

    //  it just for us of course!
    void StartBackgroundShellTasks(void);
protected:
    CDesktopBrowser();
    ~CDesktopBrowser();

    friend HRESULT CDesktopBrowser_CreateInstance(HWND hwnd, void **ppsb);
    HRESULT SetInner(IUnknown* punk);

    long _cRef;
    
    // cached pointers on inner object
    IUnknown* _punkInner;
    IBrowserService2* _pbsInner;
    IShellBrowser* _psbInner;
    IServiceProvider* _pspInner;
    IOleCommandTarget* _pctInner;
    IDockingWindowSite* _pdwsInner;
    IDockingWindowFrame* _pdwfInner;
    IInputObjectSite* _piosInner;
    IDropTarget* _pdtInner;

    LPCBASEBROWSERDATA _pbbd;

    LRESULT _RaisedWndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    void _PositionViewWindow(HWND hwnd, LPRECT prc);
    void _OnChangeNotify(LONG lNotification, LPITEMIDLIST *ppidl);
    void _SetViewArea();

    void _GetViewBorderRects(int nRects, LPRECT prc);  // does not have the tool bars
    void _SetWorkAreas(int nWorkAreas, RECT * prcWork);
    
    void _SubtractBottommostTray(LPRECT prc);
    void _SaveState();
    void _InitDeskbars();
    void _SaveDesktopToolbars();
    
    void _OnRaise(WPARAM wParam, LPARAM lParam);
    void _SetupAppRan(WPARAM wParam, LPARAM lParam);
#ifdef WINNT
    BOOL _QueryHKCRChanged(HWND hwnd, LPDWORD pdwCookie);
#endif //WINNT
    void _Lower();
    void _Raise();
    void _SwapParents(HWND hwndOldParent, HWND hwndNewParent);
    
    BOOL _OnCopyData(PCOPYDATASTRUCT pcds);
    void _OnAddToRecent( HANDLE hMem, DWORD dwProcId );
    BOOL _InitScheduler(void);
    HRESULT _AddDesktopTask(IRunnableTask *ptask, DWORD dwPriority);

    BOOL _PtOnDesktopEdge(POINTL* ppt, LPUINT puEdge);
#ifdef DEBUG
    void _CreateDeskbars();
#endif

    HRESULT _CreateDeskBarForBand(UINT uEdge, IUnknown *punk, POINTL *pptl, IBandSite **pbs);

    virtual void    v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend);
    HRESULT _OnFocusMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HRESULT _CreateDesktopView();
    HWND _GetDesktopListview();
    UINT _PeekForAMessage();
    
    void _InitMonitors();  

#ifdef ENABLE_CHANNELS
    void _MaybeLaunchChannelBand(void );
#endif
    
    virtual void    _ViewChange(DWORD dwAspect, LONG lindex);

    HWND _hwndTray;
    int _iWaitCount;
    DWORD _dwDiskFullDrives;
    ULONG _uNotifyID;
    
    DWORD _grfKeyState;
    DWORD _dwEffectOnEdge; // what's the drop effect that desktop should return on dragging over the edge
    
    BOOL _fRaised :1;
    HWND _hwndRaised;  // this is the parent of all of desktop's children when raised

    struct tagFolderSetData _fsd;

    int _nMonitors;                         // number of monitors on this desktop
    HMONITOR _hMonitors[LV_MAX_WORKAREAS]; // the order of these hmonitors need to be preserved
    RECT _rcWorkArea;       // cached work-area
    RECT _rcOldWorkAreas[LV_MAX_WORKAREAS];  // Old work areas before settings change
    DWORD _nOldWork;
    RECT _rcOldMonitors[LV_MAX_WORKAREAS];  // Old monitor sizes before settings change

    //  for _OnAddToRecent()
    IShellTaskScheduler *_psched;

#ifdef WINNT
    DWORD _cChangeEvents;
    HANDLE _rghChangeEvents[2];  // we watch HKCR and HKCR\CLSID 
    DWORD _dwChangeCookie;
    DWORD _rgdwQHKCRCookies[QHKCRID_MAX - QHKCRID_MIN];
    HKEY _hkClsid;
#endif    

    WCHAR _wzDesktopTitle[64];  // Localized Title 

    //  IUnknownIdentity - for uniformity w.r.t. aggregation
    //  We are not aggregatable, so we are our own Outer.
    IUnknown *_GetOuter() { return SAFECAST(this, IShellBrowser*); }

};

HRESULT CDesktopBrowser_CreateInstance(HWND hwnd, void **ppsb)
{
    HRESULT hr = E_OUTOFMEMORY;
    CDesktopBrowser *pdb = new CDesktopBrowser();

    if (pdb)
    {
        hr = pdb->_Initialize(hwnd, NULL);      // aggregation, etc.
        if (FAILED(hr))
            ATOMICRELEASE(pdb);
    }
    
    *ppsb = pdb;
    return hr;
}

CDesktopBrowser::CDesktopBrowser() : 
    _cRef(1)
{
    TraceMsg(TF_LIFE, "ctor CDesktopBrowser %x", this);
}

CDesktopBrowser::~CDesktopBrowser()
{
    SaveOldWorkAreas(_rcOldWorkAreas, _nOldWork);

#ifdef WINNT
    //  cleanup for QueryHKCRChanged()
    for (int i = 0; i < ARRAYSIZE(_rghChangeEvents); i++)
    {
        if (_rghChangeEvents[i])
            CloseHandle(_rghChangeEvents[i]);
    }

    if (_hkClsid)
        RegCloseKey(_hkClsid);
#endif // WINNT

    TraceMsg(TF_LIFE, "dtor CDesktopBrowser %x", this);
}


HRESULT CDesktopBrowser::_Initialize(HWND hwnd, IUnknown* pauto)
{
    IUnknown* punk;
    
    HRESULT hres = CoCreateInstance(CLSID_CCommonBrowser, _GetOuter(), CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punk);
    if (SUCCEEDED(hres))
    {
        hres = SetInner(punk); // paired w/ Release in outer (TBS::Release)
        if (SUCCEEDED(hres))
        {
            // we must initialize the inner guy BEFORE we call through any of these pointers.
            hres = _pbsInner->_Initialize(hwnd, pauto);
            if (SUCCEEDED(hres))
            {
                _pbsInner->GetBaseBrowserData(&_pbbd);
                ASSERT(_pbbd);
            
                // Restore the old settings from the registry that we persist.
                if (!GetOldWorkAreas(_rcOldWorkAreas, &_nOldWork) || _nOldWork == 0)
                {
                    // We didn't find it in the registry
                    _nOldWork = 0;  // Since this is 0, we don't have to set _rcOldWorkAreas.
                    //We will recover from this in _SetWorkAreas()
                }
            
                SetTopBrowser();
                _put_itbLastFocus(ITB_VIEW);    // focus on desktop (w95 compat)
            
                HACCEL hacc = LoadAccelerators(HINST_THISDLL, MAKEINTRESOURCE(ACCEL_DESKTOP));
                ASSERT(hacc);
                _pbsInner->SetAcceleratorMenu(hacc);
            
                // Perf: never fire events from the desktop.
                ASSERT(_pbbd->_pautoEDS);
                ATOMICRELEASE(_pbbd->_pautoEDS);
            
                _InitMonitors();
            
                // Initialise _rcOldMonitors
                for (int i = 0; i < _nMonitors; i++)
                {
                    GetMonitorRect(_hMonitors[i], &_rcOldMonitors[i]);
                }
#ifdef WINNT
                //  NOTE:  if we have any more keys to watch, then 
                //  we should create a static struct to walk
                //  so that it is easier to add more event/key pairs
                _rghChangeEvents[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
                _rghChangeEvents[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (_rghChangeEvents[0] && _rghChangeEvents[1])
                {
                    if (ERROR_SUCCESS == RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE, 
                        REG_NOTIFY_CHANGE_LAST_SET  |REG_NOTIFY_CHANGE_NAME, _rghChangeEvents[0], TRUE))
                    {
                        _cChangeEvents = 1;
                        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, MAXIMUM_ALLOWED, &_hkClsid)
                        &&  ERROR_SUCCESS == RegNotifyChangeKeyValue(_hkClsid, TRUE, REG_NOTIFY_CHANGE_LAST_SET |REG_NOTIFY_CHANGE_NAME, _rghChangeEvents[1], TRUE))
                        {
                            //  we need to leave the key open, 
                            //  or the event is signaled right away
                            _cChangeEvents++;
                        }
                    }
                }
#endif //WINNT
            }

        }
    } 
    else 
    {
        ASSERT(!"CoCreateInstance(CLSID_CCommonBrowser failed - big trouble");
    }
    
    return hres;
}


//
//  The refcount in the punk is transferred to us.  We do not need to
//  and indeed should not AddRef it.
//
//  If any of these steps fails, we will clean up in our destructor.
//
HRESULT CDesktopBrowser::SetInner(IUnknown* punk)
{
    HRESULT hres;

    ASSERT(_punkInner == NULL);

    _punkInner = punk;

#define INNERCACHE(iid, p) do { \
    hres = SHQueryInnerInterface(_GetOuter(), punk, iid, (void **)&p); \
    if (!EVAL(SUCCEEDED(hres))) return E_FAIL; \
    } while (0)

    INNERCACHE(IID_IBrowserService2, _pbsInner);
    INNERCACHE(IID_IShellBrowser, _psbInner);
    INNERCACHE(IID_IServiceProvider, _pspInner);
    INNERCACHE(IID_IOleCommandTarget, _pctInner);
    INNERCACHE(IID_IDockingWindowSite, _pdwsInner);
    INNERCACHE(IID_IDockingWindowFrame, _pdwfInner);
    INNERCACHE(IID_IInputObjectSite, _piosInner);
    INNERCACHE(IID_IDropTarget, _pdtInner);

#undef INNERCACHE

    return S_OK;
}

ULONG CDesktopBrowser::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDesktopBrowser::Release()
{
    ASSERT(_cRef > 0);
    
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    _cRef = 1000;               // guard against recursion

    RELEASEINNERINTERFACE(_GetOuter(), _pbsInner);
    RELEASEINNERINTERFACE(_GetOuter(), _psbInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pspInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pctInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pdwsInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pdwfInner);
    RELEASEINNERINTERFACE(_GetOuter(), _piosInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pdtInner);

    // this must come last
    ATOMICRELEASE(_punkInner); // paired w/ CCI aggregation
    
    ASSERT(_cRef == 1000);

    delete this;
    return 0;
}

HRESULT CDesktopBrowser::QueryInterface(REFIID riid, void * * ppvObj)
{
    // IUnknownIdentity - The interface we use for IUnknown must come first.
    static const QITAB qit[] = {
        QITABENT(CDesktopBrowser, IShellBrowser),
        QITABENT(CDesktopBrowser, IBrowserService2),
        QITABENTMULTI(CDesktopBrowser, IBrowserService, IBrowserService2),
        QITABENTMULTI(CDesktopBrowser, IOleWindow, IShellBrowser),
        QITABENTMULTI2(CDesktopBrowser, SID_SShellDesktop, IShellBrowser), // effectively an IUnknown supported only by this class
        QITABENT(CDesktopBrowser, IServiceProvider),
        QITABENT(CDesktopBrowser, IOleCommandTarget),
        QITABENT(CDesktopBrowser, IDockingWindowSite),
        QITABENT(CDesktopBrowser, IInputObjectSite),
        QITABENT(CDesktopBrowser, IMultiMonitorDockingSite),
        QITABENT(CDesktopBrowser, IDropTarget),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres)) {
        if (_punkInner)
        {
            // don't let these get through to our base class...
            // IID_IOleCommandTarget, IID_IOleInPlaceUIWindow
            // BUGBUG 970414 adp: spoke to SatoNa, these *can* go thru
            // i'll remove this week
            if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
            {
                *ppvObj = NULL;
                hres = E_NOINTERFACE;
            }
            else
            {
                hres = _punkInner->QueryInterface(riid, ppvObj);
            }
        }
    }
    
    return hres;
}


void _InitDesktopMetrics(WPARAM wParam, LPCTSTR pszSection)
{
    BOOL fForce = (!pszSection || !*pszSection);

    if (fForce || (wParam == SPI_SETNONCLIENTMETRICS) || !lstrcmpi(pszSection, TEXT("WindowMetrics")))
    {
        FileIconInit(TRUE); // Tell the shell we want to play with a full deck

        // BUGBUG do this
        // InvalidateImageIndices();
    }
}

typedef struct _EnumMonitorsData
{
    int iMonitors;
    HMONITOR * phMonitors;
} EnumMonitorsData;

BOOL CALLBACK MultiMonEnumCallBack(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData)
{
    EnumMonitorsData * pEmd = (EnumMonitorsData *)lData;
    
    if (pEmd->iMonitors > LV_MAX_WORKAREAS - 1)
        //ignore the other monitors because we can only handle up to LV_MAX_WORKAREAS
        //BUGBUG: should we dynamically allocated this?
        return FALSE;

    pEmd->phMonitors[pEmd->iMonitors++] = hMonitor;
    return TRUE;
}

// Initialize the number of monitors and the hmonitors array

void CDesktopBrowser::_InitMonitors()
{
    HMONITOR hMonPrimary = GetPrimaryMonitor();
    
    EnumMonitorsData emd;
    emd.iMonitors = 0;
    emd.phMonitors = _hMonitors;

    EnumDisplayMonitors(NULL, NULL, MultiMonEnumCallBack, (LPARAM)&emd);
    _nMonitors = GetNumberOfMonitors();
    
    // Always move the primary monitor to the first location.
    if (_hMonitors[0] != hMonPrimary)
    {
        int iMon;
        for (iMon = 1; iMon < _nMonitors; iMon++)
        {
            if (_hMonitors[iMon] == hMonPrimary)
            {
                _hMonitors[iMon] = _hMonitors[0];
                _hMonitors[0] = hMonPrimary;
                break;
            }
        }
    }
}

// Gets the persisted old work areas, from the registry
BOOL GetOldWorkAreas(LPRECT lprc, DWORD* pdwNoOfOldWA)
{
    BOOL fRet = FALSE;
    *pdwNoOfOldWA = 0;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_DESKCOMP_OLDWORKAREAS, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType, cbSize = SIZEOF(*pdwNoOfOldWA);
        // Read in the no of old work areas
        if (SHQueryValueEx(hkey, REG_VAL_OLDWORKAREAS_COUNT, NULL, &dwType, (LPBYTE)pdwNoOfOldWA, &cbSize) == ERROR_SUCCESS)
        {
            // Read in the old work area rects
            cbSize = SIZEOF(*lprc) * (*pdwNoOfOldWA);
            if (SHQueryValueEx(hkey, REG_VAL_OLDWORKAREAS_RECTS, NULL, &dwType, (LPBYTE)lprc, &cbSize) == ERROR_SUCCESS)
            {
                fRet = TRUE;
            }
        }
        RegCloseKey(hkey);
    }
    return fRet;
}
        
// Saves the old work areas into the registry
void SaveOldWorkAreas(LPCRECT lprc, DWORD nOldWA)
{
    // Recreate the registry key.
    HKEY hkey;
    if (RegCreateKey(HKEY_CURRENT_USER, REG_DESKCOMP_OLDWORKAREAS, &hkey) == ERROR_SUCCESS)
    {
        // Write out the no. of old work areas
        RegSetValueEx(hkey, REG_VAL_OLDWORKAREAS_COUNT, 0, REG_DWORD, (LPBYTE)&nOldWA, SIZEOF(nOldWA));
        // Write out the no work area rectangles
        RegSetValueEx(hkey, REG_VAL_OLDWORKAREAS_RECTS, 0, REG_BINARY, (LPBYTE)lprc, SIZEOF(*lprc) * nOldWA);
        // Close out the reg key
        RegCloseKey(hkey);
    }
}

//***   CDesktopBrowser::IOleCommandTarget::* {

STDMETHODIMP CDesktopBrowser::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDesktopBrowser::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) {
        /*NOTHING*/
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup)) {
        switch (nCmdID) {
        case SHDVID_RAISE:
            // n.b.: DTRF_RAISE/DTRF_LOWER go down; DTRF_QUERY goes up
            ASSERT(pvarargIn != NULL && pvarargIn->vt == VT_I4);
            if (pvarargIn->vt == VT_I4 && pvarargIn->lVal == DTRF_QUERY) {
                ASSERT(pvarargOut != NULL);
                pvarargOut->vt = VT_I4;
                pvarargOut->lVal = _fRaised ? DTRF_RAISE : DTRF_LOWER;
                return S_OK;
            }
            // o.w. let parent handle it
            break;

        case SHDVID_UPDATEOFFLINEDESKTOP:
            UpdateAllDesktopSubscriptions();
            return S_OK;
        }
    } else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup)) {
        switch (nCmdID)
        {
            case SBCMDID_OPTIONS:
            case SBCMDID_ADDTOFAVORITES:
                return _pctInner->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
    }

    // do *not* forward up to SUPERCLASS::Exec (see QI's cryptic commment
    // about "don't let these get thru to our base class")
    return OLECMDERR_E_NOTSUPPORTED;
}

// }

STDMETHODIMP CDesktopBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    // REVIEW: do we need this at all? does navigate come through here?

    // Force SBSP_NEWBROWSER and SBSP_ABSOLUTE;
    wFlags &= ~(SBSP_DEFBROWSER | SBSP_SAMEBROWSER | SBSP_RELATIVE | SBSP_PARENT);
    wFlags |= (SBSP_NEWBROWSER | SBSP_ABSOLUTE);
    return _psbInner->BrowseObject(pidl, wFlags);
}

IStream *GetDesktopViewStream(DWORD grfMode, LPCTSTR pszName)
{
    HKEY hkStreams;

    if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\Streams"), &hkStreams) == ERROR_SUCCESS)
    {
        IStream *pstm = OpenRegStream(hkStreams, TEXT("Desktop"), pszName, grfMode);
        RegCloseKey(hkStreams);
        return pstm;
    }
    return NULL;
}

void DeleteDesktopViewStream(LPCTSTR pszName)
{
    SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\Streams\\Desktop"), pszName);
}

#ifndef BUGBUG // export SHGetViewStream from BROWSEUI and make shdocvw,shell32 delay load it!
int CDECL MRUILIsEqual(const void *pidl1, const void *pidl2, size_t cb)
{
    // First cheap hack to see if they are 100 percent equal for performance
    int iCmp = memcmp(pidl1, pidl2, cb);

    if (iCmp == 0)
        return 0;

    if (ILIsEqual((LPITEMIDLIST)pidl1, (LPITEMIDLIST)pidl2))
        return 0;

    return iCmp;
}

IStream* GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreamMRU, LPCTSTR pszStreams)
{
    IStream *pstm = NULL;
    HANDLE hmru;
    int iFoundSlot = -1;
    UINT cbPidl;
    static DWORD s_dwMRUSize = 0;
    DWORD dwSize = sizeof(s_dwMRUSize);

    if ((0 == s_dwMRUSize) && 
        (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, pszStreamMRU, TEXT("MRU Size"), NULL, (LPVOID) &s_dwMRUSize, &dwSize)))
    {
        s_dwMRUSize = 200;          // The default.
    }

    MRUINFO mi = {
        SIZEOF(MRUINFO),
        s_dwMRUSize,                     // we store this many view streams
        MRU_BINARY,
        HKEY_CURRENT_USER,
        pszStreamMRU,
        (MRUCMPPROC)MRUILIsEqual,
    };

    ASSERT(pidl);

    // should be checked by caller - if this is not true we'll flush the
    // MRU cache with internet pidls!  FTP and other URL Shell Extension PIDLs
    // that act like a folder and need similar persistence and fine.  This
    // is especially true because recently the cache size was increased from
    // 30 or so to 200.
    //ASSERT(pidl==c_pidlURLRoot || !IsURLChild(pidl, TRUE) || IsPidlFromURLShellExt(pidl));

    cbPidl = ILGetSize(pidl);

    // Now lets try to save away the other information associated with view.
    hmru = CreateMRUListLazyEx(&mi, pidl, cbPidl, &iFoundSlot);
    if (!hmru)
        return NULL;

    // Did we find the item?
    if (iFoundSlot < 0 && ((grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_READ))
    {
        // Do not  create the stream if it does not exist and we are
        // only reading
    }
    else
    {
        HKEY hkCabStreams;


        // Note that we always create the key here, since we have
        // already checked whether we are just reading and the MRU
        // thing does not exist

        hkCabStreams = SHGetExplorerSubHkey( HKEY_CURRENT_USER, pszStreams, TRUE );
        if ( hkCabStreams )
        {
            HKEY hkValues;
            TCHAR szValue[32], szSubVal[64];

            wsprintf(szValue, TEXT("%d"), AddMRUDataEx(hmru, pidl, cbPidl));

            if (iFoundSlot < 0 && RegOpenKey(hkCabStreams, szValue, &hkValues) == ERROR_SUCCESS)
            {
                // This means that we have created a new MRU
                // item for this PIDL, so clear out any
                // information residing at this slot
                // Note that we do not just delete the key,
                // since that could fail if it has any sub-keys
                // BUGBUG NOTE: Why can't we use SHDeleteKey?
                DWORD dwType;

                while (dwSize = ARRAYSIZE(szSubVal),
                       ERROR_SUCCESS == RegEnumValue(hkValues, 0, szSubVal, &dwSize, NULL, &dwType, NULL, NULL))
                {
                    if (RegDeleteValue(hkValues, szSubVal) != ERROR_SUCCESS)
                    {
                        break;
                    }
                }

                RegCloseKey(hkValues);
            }
            pstm = SHOpenRegStream(hkCabStreams, szValue, pszName, grfMode);
            RegCloseKey(hkCabStreams);
        }
    }

    FreeMRUListEx(hmru);

    return pstm;
}
#endif // !BUGBUG

IStream *CDesktopBrowser::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName)
{
    IStream *pstm;
    TCHAR szName[MAX_PATH];
    
    SHUnicodeToTChar(pwszName, szName, ARRAYSIZE(szName));
    pstm = GetDesktopViewStream(grfMode, szName);
    
    //
    // Detect the very first boot and migrate the icon positoins from where
    // Win95 has stored (OK to execute this code for NT as well).
    //
    if (grfMode == STGM_READ && !lstrcmpi(szName, TEXT("ViewView")) &&
        (!pstm || SHIsEmptyStream(pstm)))
    {
        TCHAR szPath[MAX_PATH];
        if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, TRUE))
        {
            LPITEMIDLIST pidl=ILCreateFromPath(szPath);
            if (pidl) {
                LPCTSTR lpstrMRU;
                LPCTSTR lpstrStream;
                
                if ( pstm )
                    pstm->Release();
                
                if (g_fRunningOnNT)
                {
                    lpstrMRU = REGSTR_PATH_EXPLORER TEXT("\\DesktopStreamMRU");
                    lpstrStream = TEXT("DesktopStreams");
                }
                else
                {
                    lpstrMRU = REGSTR_PATH_EXPLORER TEXT("\\StreamMRU");
                    lpstrStream = TEXT("Streams");
                }
                pstm = ::GetViewStream(pidl, grfMode, szName, lpstrMRU, lpstrStream);
                
                if(!pstm && g_fRunningOnNT)
                {
                    LPITEMIDLIST  pidlWin95Desktop;
                    TCHAR         szWin95DesktopPath[MAX_PATH];
                    LPTSTR        pszDirName;
                    
                    // This path is hit when we upgrade from Win95 (with NO IE4.X).
                    // The path to desktop in NT5 is "c:\<win>\Profiles\<username>\Desktop";
                    // but, under win95, the path is "c:\<win>\Desktop". So, we need to 
                    // generate the pidl corresponding to what we have in Win95.
                    pszDirName = PathFindFileName(szPath); //Finds pointer to "Desktop"
                    GetWindowsDirectory(szWin95DesktopPath, ARRAYSIZE(szWin95DesktopPath));
                    PathAppend(szWin95DesktopPath, pszDirName);
                    
                    pidlWin95Desktop = ILCreateFromPath(szWin95DesktopPath);
                    if (pidlWin95Desktop) 
                    {
                        // When we upgrade from Win95 to NT5 directly, "DesktopStreamMRU" won't be present.
                        // So, we need to read from the old location in this case.
                        pstm = ::GetViewStream(pidlWin95Desktop, grfMode, szName, 
                            REGSTR_PATH_EXPLORER TEXT("\\StreamMRU"), 
                            TEXT("Streams"));
                        ILFree(pidlWin95Desktop);
                    }
                }
                ILFree(pidl);
            }
        }
    }
    
    return pstm;
}

LRESULT CDesktopBrowser::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case FCIDM_FINDFILES:
        SHFindFiles(_pbbd->_pidlCur, NULL);
        break;
        
    case FCIDM_REFRESH:
    {
        // Invalidate the enum cache to force a reenumeration
        LPITEMIDLIST pidl;
        if (S_OK == SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidl))
        {
            Desktop_InvalidateEnumCache(pidl, SHCNE_DISKEVENTS);
            ILFree(pidl);
        }

        VARIANT v = {0};
        v.vt = VT_I4;
        v.lVal = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
        // Our Exec is neutered (on purpose), so call our parent
        _pctInner->Exec(NULL, OLECMDID_REFRESH, OLECMDEXECOPT_DONTPROMPTUSER, &v, NULL);
        break;
    }

    case IDC_KBSTART:
    case FCIDM_NEXTCTL:
        if (_hwndTray)
        {
            // n.b. VK_TAB handled this way (among other things)
            SendMessage(_hwndTray, WM_COMMAND, wParam, lParam);
        }
        break;


    case IDM_CLOSE:
        SendMessage(_hwndTray, TM_DOEXITWINDOWS, 0, 0);
        break;

    default:
        _pbsInner->OnCommand(wParam, lParam);
        break;
    }
    
    return S_OK;
}


// Create desktop IShellView instance

HRESULT CDesktopBrowser::_CreateDesktopView()
{
    LPCITEMIDLIST pidl = SHCloneSpecialIDList(NULL, CSIDL_DESKTOP, TRUE);
    if (pidl)
    {
        FOLDERSETTINGS fs;
        DWORD cb = SIZEOF(fs);

        if (SHGetValueGoodBoot(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\DeskView"), 
            TEXT("Settings"), NULL, (BYTE *)&fs, &cb) == ERROR_SUCCESS &&
            cb >= SIZEOF(fs))
        {
            _fsd._fs.fFlags = fs.fFlags | FWF_DESKTOP | FWF_NOCLIENTEDGE;        // make it transparent, etc.
        }
        else
        {
            _fsd._fs.fFlags = FWF_DESKTOP | FWF_NOCLIENTEDGE;  // default
        }

        _fsd._fs.ViewMode = FVM_ICON;  // can't change this, sorry

        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_HIDEICONS, FALSE);
        if (ss.fHideIcons)
            _fsd._fs.fFlags |= FWF_NOICONS;
        else
            _fsd._fs.fFlags &= ~FWF_NOICONS;

        // We keep the active desktop in offline mode!
        ASSERT(_pbbd->_pautoWB2);
        _pbbd->_pautoWB2->put_Offline(TRUE);

        return _psbInner->BrowseObject(pidl, 0);
    }
    else
    {
        TCHAR szYouLoose[256];

        // BUGBUG (beta-only): deal with the inability to create the desktop
        LoadString(HINST_THISDLL, IDS_YOULOSE, szYouLoose, ARRAYSIZE(szYouLoose));
        MessageBox(NULL, szYouLoose, NULL, MB_ICONSTOP);
        return E_FAIL;
    }
}

HRESULT CDesktopBrowser::ActivatePendingView(void)
{
    HRESULT hres = _pbsInner->ActivatePendingView();
    if (SUCCEEDED(hres))
    {
        if (g_fRunningOnNT)
        {
            // On NT, simply calling SetShellWindow will cause the desktop
            // to initially paint white, then the background window.
            // This causes an ugly white trail when you move windows 
            // around until the desktop finally paints.
            // 
            // Calling SetShellWindowEx resolves this problem.
            //
            SHSetShellWindowEx(_pbbd->_hwnd, _GetDesktopListview());
        }
        else
        {
            SetShellWindow(_pbbd->_hwnd);
        }

        // Tell fsnotiy that our window is ready to handle PnP notifications
        // Must wait until we've done the SetShellWindow because
        SHChangeNotify_DesktopInit();

    }
    
    return hres;
}

#ifdef DEBUG // {

void CDesktopBrowser::_CreateDeskbars()
{
    HRESULT hres;
    BOOL fCreate = FALSE;
    HKEY hkey;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\DeskBar\\Bands"), &hkey)) {
        fCreate = TRUE;
        RegCloseKey(hkey);
    }
    
    if (fCreate) {
        IPersistStreamInit *ppstm;

        hres = CoCreateInstance(CLSID_DeskBarApp, NULL, CLSCTX_INPROC_SERVER, IID_IPersistStreamInit, (void **)&ppstm);
        if (SUCCEEDED(hres)) {
            hres = ppstm->InitNew();
            AddToolbar(ppstm, L"test", NULL);    // BUGBUG "Microsoft.DeskBarApp"
            ppstm->Release();
        }
    }
}
#endif // }

void CDesktopBrowser::_InitDeskbars()
{
    //
    // Load toolbars
    //

    // 1st, try persisted state
    IStream* pstm = GetDesktopViewStream(STGM_READ, TEXT("Toolbars"));
    HRESULT hres = E_FAIL;
    if (pstm) {
        hres = _pbsInner->_LoadToolbars(pstm);
        pstm->Release();
    }

    // 2nd, if there is none (or if version mismatch or other failure),
    // try settings from setup
    // BUGBUG n.b. this works fine for ie4 where we have no old toolbars,
    // but for future releases we'll need some kind of merging scheme,
    // so we probably want to change this after ie4-beta-1.
    if (FAILED(hres)) {
        // n.b. HKLM not HKCU
        // like GetDesktopViewStream but for HKLM
        pstm = OpenRegStream(g_hklmExplorer,
            TEXT("Streams\\Desktop"),
            TEXT("Default Toolbars"), STGM_READ);
        if (pstm) {
            hres = _pbsInner->_LoadToolbars(pstm);
            pstm->Release();
        }
    }

    // o.w., throw up our hands
    if (FAILED(hres)) {
        ASSERT(0);
#ifdef DEBUG
        // but for debug, need a way to bootstrap the entire process
        _CreateDeskbars();

#endif
    }
}

//---------------------------------------------------------------------------
// Handle creation of a new Desktop folder window. Creates everything except
// the viewer part.
// Returns -1 if something goes wrong.
HWND g_hwndTray = NULL;

HRESULT CDesktopBrowser::OnCreate(CREATESTRUCT *pcs)
{
    LRESULT lr;

    g_pdtray->GetTrayWindow(&_hwndTray);
    g_hwndTray = _hwndTray;
    g_pdtray->SetDesktopWindow(_pbbd->_hwnd);

    //
    // Notify IEDDE that the automation services are now available.
    //
    IEOnFirstBrowserCreation(NULL);

    ASSERT(_hwndTray);

    // BUGBUG TODO: we need to split out "ie registry settings" into a
    // browser component and a shell component.
    //
    //EnsureWebViewRegSettings();

    if (SUCCEEDED(_CreateDesktopView()))
    {
        SHChangeNotifyEntry fsne;
        
        lr = _pbsInner->OnCreate(pcs);   // success

        fsne.fRecursive = TRUE;
        fsne.pidl = NULL;
        _uNotifyID = SHChangeNotifyRegister(_pbbd->_hwnd, SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery, SHCNE_DRIVEADDGUI, WM_CHANGENOTIFY, 1, &fsne);

        PostMessage(_pbbd->_hwnd, DTM_CREATESAVEDWINDOWS, 0, 0);
        
        return (HRESULT) lr;
    }

    return (LRESULT)-1;   // failure
}

UINT GetDDEExecMsg()
{
    static UINT uDDEExec = 0;

    if (!uDDEExec)
        uDDEExec = RegisterWindowMessage(TEXT("DDEEXECUTESHORTCIRCUIT"));

    return uDDEExec;
}

LRESULT CDesktopBrowser::OnNotify(NMHDR * pnm)
{
    switch (pnm->code) 
    {
    case SEN_DDEEXECUTE:
        if (pnm->idFrom == 0) 
        {
            // short cut notifier around the dde conv.
            
            LPNMVIEWFOLDER pnmPost = DDECreatePostNotify((LPNMVIEWFOLDER)pnm);

            if (pnmPost)
            {
                PostMessage(_pbbd->_hwnd, GetDDEExecMsg(), 0, (LPARAM)pnmPost);
                return TRUE;
            }
        }
        break;

    case NM_STARTWAIT:
    case NM_ENDWAIT:
        _iWaitCount += (pnm->code == NM_STARTWAIT ? 1 :-1);

        ASSERT(_iWaitCount >= 0);

        // Don't let it go negative or we'll never get rid of it.
        if (_iWaitCount < 0)
            _iWaitCount = 0;

        // what we really want is for user to simulate a mouse move/setcursor
        SetCursor(LoadCursor(NULL, _iWaitCount ? IDC_APPSTARTING : IDC_ARROW));
        break;

    default:
        return _pbsInner->OnNotify(pnm);
    }
    return 0;
}

// HACKHACK: this hard codes in that we know a listview is the child
// of the view.
HWND CDesktopBrowser::_GetDesktopListview()
{
    HWND hwndView = _pbbd->_hwndView ? _pbbd->_hwndView : _pbbd->_hwndViewPending;
    
    if (!hwndView)
        return NULL;
    
    return FindWindowEx(hwndView, NULL, WC_LISTVIEW, NULL);
}

#ifndef ENUM_REGISTRY_SETTINGS
#define ENUM_REGISTRY_SETTINGS ((DWORD)-2)
#endif


BOOL IsTempDisplayMode()
{
    BOOL fTempMode = FALSE;

    // BUGBUG (scotth): the original code in explorer was #ifdef WINNT or MEMPHIS.
    //  We're just checking for NT right now.
    if (g_fRunningOnNT)
    {
        DEVMODE dm;

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm) &&
            dm.dmPelsWidth > 0 && dm.dmPelsHeight > 0)
        {
            HDC hdc = GetDC(NULL);
            int xres = GetDeviceCaps(hdc, HORZRES);
            int yres = GetDeviceCaps(hdc, VERTRES);
            ReleaseDC(NULL, hdc);

            if (xres != (int)dm.dmPelsWidth || yres != (int)dm.dmPelsHeight)
                fTempMode = TRUE;
        }
    }
    else
    {
        TCHAR ach[80];
        DWORD cb = SIZEOF(ach);

        if (SHGetValueGoodBoot(HKEY_CURRENT_CONFIG, REGSTR_PATH_DISPLAYSETTINGS, 
            REGSTR_VAL_RESOLUTION, NULL, (BYTE *)ach, &cb) == ERROR_SUCCESS)
        {
            int xres = StrToInt(ach);
            HDC hdc = GetDC(NULL);

            if ((GetDeviceCaps(hdc, CAPS1) & C1_REINIT_ABLE) &&
                (xres > 0) && (xres != GetDeviceCaps(hdc, HORZRES))) 
            {
                fTempMode = TRUE;
            }
            ReleaseDC(NULL, hdc);
        }

    }
    return fTempMode;
}

// BUGBUG: this is the hack andyp put in
// (dli) Currently, bottommost Tray is really wierd, it is not treated as toolbars.
// In a sense, it has higher priority than those toolbars. So they should be taken 
// off the EffectiveClientArea

void CDesktopBrowser::_SubtractBottommostTray(LPRECT prc)
{
    LRESULT lTmp;
    APPBARDATA abd;
    
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = _hwndTray;

    // lTmp = SHAppBarMessage(ABM_GETSTATE, &abd);
    lTmp = g_pdtray->AppBarGetState();

    if ((lTmp & (ABS_ALWAYSONTOP|ABS_AUTOHIDE)) == 0) {
        // tray is on bottom and takes 'real' space
        RECT rcTray = {0};
        
        GetWindowRect(_hwndTray, &rcTray);
        IntersectRect(&rcTray, prc, &rcTray);
        SubtractRect(prc, prc, &rcTray);
    }   
}

HRESULT CDesktopBrowser::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
    //
    // Cache the work area if
    //  (1) this is the very first call
    //  (2) cached value is blew off by WM_SIZE (in _OnSize)
    //
    if (hmon) {
        GetMonitorWorkArea(hmon, lprectBorder);
    }
    else {
        if (::IsRectEmpty(&_rcWorkArea)) {
            ::SystemParametersInfo(SPI_GETWORKAREA, 0, &_rcWorkArea, 0);
        }
        *lprectBorder = _rcWorkArea;
    }

    _SubtractBottommostTray(lprectBorder);
    MapWindowPoints(NULL, _pbbd->_hwnd, (LPPOINT)lprectBorder, 2);
    return S_OK;
}


BOOL EqualRects(LPRECT prcNew, LPRECT prcOld, int nRects)
{
    int i;
    for (i = 0; i < nRects; i++)
        if (!EqualRect(&prcNew[i], &prcOld[i]))
            return FALSE;
    return TRUE;
}

void CDesktopBrowser::_SetWorkAreas(int nWorkAreas, LPRECT prcWork)
{
    RECT rcListViewWork[LV_MAX_WORKAREAS];
    RECT rcNewWork[LV_MAX_WORKAREAS];
    int  nListViewWork = 0;
    HWND hwndList;
    int  i;

    ASSERT(prcWork);
    ASSERT(nWorkAreas <= LV_MAX_WORKAREAS);
    ASSERT(nWorkAreas > 0);

    if (nWorkAreas <= 0)
        return;

    if (IsTempDisplayMode())
        return;
    
    hwndList = _GetDesktopListview();
    ASSERT(IsWindow(hwndList));
    
    ListView_GetNumberOfWorkAreas(hwndList, &nListViewWork);

    if (nListViewWork > 0)
    {
        ListView_GetWorkAreas(hwndList, nListViewWork, rcListViewWork);
        // Map these rects back to DESKTOP coordinate
        // We need to convert the following only if WorkAreas > 1
        if (nListViewWork > 1)
        {
            // [msadek]; MapWindowPoints() is mirroring-aware only if you pass two points
            for(i = 0; i < nListViewWork; i++)
            {
                MapWindowPoints(hwndList, HWND_DESKTOP, (LPPOINT)(&rcListViewWork[i]), 2);
            }    
        }    
        if(nListViewWork == nWorkAreas && EqualRects(prcWork, rcListViewWork, nWorkAreas))
            return;
    }
    else if (_nOldWork > 1)
        // In single monitor case, listview workares always starts from (0,0)
        // It will be wrong to set the persisted workarea. 
    {
        for (nListViewWork = 0; nListViewWork < (int)_nOldWork && nListViewWork < LV_MAX_WORKAREAS; nListViewWork++)
            CopyRect(&rcListViewWork[nListViewWork], &_rcOldWorkAreas[nListViewWork]);

        // This may not be needed, because at this point, ListView is in Desktop coordinate
        // [msadek]; MapWindowPoints() is mirroring-aware only if you pass two points
        for(i = 0; i < nListViewWork; i++)
        {
            MapWindowPoints(HWND_DESKTOP, hwndList, (LPPOINT)(&rcListViewWork[i]), 2);
        }    
        // This call to SetWorkAreas sets the old work areas in the listview, which is not persisted there.
        ListView_SetWorkAreas(hwndList, nListViewWork, rcListViewWork);
    }
    
    //Make a copy of the new work area array because it gets modified by
    // the MapWindowPoints below.
    for(i = 0; i < nWorkAreas; i++)
        rcNewWork[i] = *(prcWork + i);

    // It's already in ListView coordinate if we just have one monitor
    if (nWorkAreas > 1)
    {
        for(i = 0; i < nWorkAreas; i++)
        {
            MapWindowPoints(HWND_DESKTOP, hwndList, (LPPOINT)(&prcWork[i]), 2);
        }    
    }
 
    // We need to set the new work area before we call AdjustDesktopComponents below
    // because that function results in a refresh and that ends up setting
    // the work areas again to the same values.
    ListView_SetWorkAreas(hwndList, nWorkAreas, prcWork);

    if (nWorkAreas == 1)
        MapWindowPoints(hwndList, HWND_DESKTOP, (LPPOINT)rcNewWork, 2 * nWorkAreas);

    //Move and size desktop components based on the new work areas.
    AdjustDesktopComponents((LPCRECT)rcNewWork, nWorkAreas, (LPCRECT)_rcOldMonitors, (LPCRECT)_rcOldWorkAreas, _nOldWork);

    // Backup the new Monitor rect's in _rcOldMonitors
    for (i = 0; i < _nMonitors; i++)
    {
        GetMonitorRect(_hMonitors[i], &_rcOldMonitors[i]);
    }
    // Backup the new workareas in _rcOldWorkAreas
    for (i = 0; i < nWorkAreas; i++)
    {
        _rcOldWorkAreas[i] = rcNewWork[i];
    }
    _nOldWork = nWorkAreas;
}

// Get all the view border rectangles (not including toolbars) for the monitors
// this is used for multi-monitor case only.  
void CDesktopBrowser::_GetViewBorderRects(int nRects, LPRECT prcBorders)
{
    int iMon;
    HMONITOR hmonTray;
    Tray_GetHMonitor(_hwndTray, &hmonTray);
    for (iMon = 0; iMon < min(_nMonitors, nRects); iMon++)
    {
        GetMonitorWorkArea(_hMonitors[iMon], &prcBorders[iMon]);
        if (hmonTray == _hMonitors[iMon])
            _SubtractBottommostTray(&prcBorders[iMon]);
    
        // Extract the border taken by all "frame" toolbars
        for (int itb=0; itb < _pbsInner->_GetToolbarCount(); itb++)
        {
            LPTOOLBARITEM ptbi = _pbsInner->_GetToolbarItem(itb);
            if (ptbi && ptbi->hMon == _hMonitors[iMon])
            {
                prcBorders[iMon].left += ptbi->rcBorderTool.left;
                prcBorders[iMon].top += ptbi->rcBorderTool.top;
                prcBorders[iMon].right -= ptbi->rcBorderTool.right;
                prcBorders[iMon].bottom -= ptbi->rcBorderTool.bottom;
            }
        }       
    }
}
HRESULT  CDesktopBrowser::_UpdateViewRectSize()
{
    HWND hwndView = _pbbd->_hwndView;
    if (!hwndView && ((hwndView = _pbbd->_hwndViewPending) == NULL))
        return S_FALSE;

    _pbsInner->_UpdateViewRectSize();

    if (_nMonitors <= 1)
    {
        RECT rcView, rcWork;
        GetViewRect(&rcView);
        rcWork.top = rcWork.left = 0;
        rcWork.right = RECTWIDTH(rcView);
        rcWork.bottom = RECTHEIGHT(rcView);
        _SetWorkAreas(1, &rcWork);
    }
    else
    {
        RECT rcWorks[LV_MAX_WORKAREAS];
        _GetViewBorderRects(_nMonitors, rcWorks);
        _SetWorkAreas(_nMonitors, rcWorks);
    }   

    return S_OK;
}

void CDesktopBrowser::_PositionViewWindow(HWND hwnd, LPRECT prc)
{
    RECT rcWindow;
    UINT uSWPFlags;
    int iWidth, iHeight;
    GetWindowRect(hwnd, &rcWindow);
    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rcWindow, 2);
    
    //
    // Erase it all so the wallpaper is repainted correctly.
    //
    if (rcWindow.left != prc->left ||
        rcWindow.top != prc->top)
    {   
        // optimize, if the top left didn't change, the wallpaper doesn't move
        InvalidateRect(hwnd, NULL, TRUE);
        uSWPFlags = SWP_NOCOPYBITS | SWP_NOZORDER | SWP_NOACTIVATE;
    }   
    else        
    {   
        uSWPFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;
    }   
    iWidth = RECTWIDTH(*prc);
    iHeight = RECTHEIGHT(*prc);
    SetWindowPos(hwnd, NULL, prc->left, prc->top, iWidth, iHeight, uSWPFlags);
}

void CDesktopBrowser::_SetViewArea()
{
    //
    // Invalidate the cached work area size
    //
    ::SetRectEmpty(&_rcWorkArea);

    v_ShowHideChildWindows(FALSE);
}

// we get called here when new drives come and go;
// things like net connections, hot insertions, etc.

void _OnDeviceBroadcast(HWND hwnd, ULONG_PTR code, DEV_BROADCAST_HDR *pbh)
{
    // do a bunch of this stuff here in desktop so it only happens
    // once...

    switch (code)
    {
    case DBT_LOW_DISK_SPACE:
    case DBT_NO_DISK_SPACE:
        SetTimer(hwnd, IDT_DISKFULL + PtrToUlong(pbh), DISKFULL_TIMEOUT, NULL);
        break;

    case DBT_DEVICEREMOVECOMPLETE:      // drive or media went away
    case DBT_DEVICEARRIVAL:             // new drive or media (or UNC) has arrived
    case DBT_DEVICEQUERYREMOVE:         // drive or media about to go away
    case DBT_DEVICEQUERYREMOVEFAILED:   // drive or media didn't go away

        // Don't process if we are being shutdown...
        if (!IsWindowVisible(hwnd))
            break;

        CMountPoint::HandleWMDeviceChange(code, pbh);
        SHChangeNotify_OnDeviceChange(code, pbh);

        break;

    case DBT_CONFIGCHANGED:
        Desktop_UpdateBriefcaseOnEvent(hwnd, UOE_CONFIGCHANGED);
        break;

    case DBT_QUERYCHANGECONFIG:
        Desktop_UpdateBriefcaseOnEvent(hwnd, UOE_QUERYCHANGECONFIG);
        break;
    }
}

BOOL AppAllowsAutoRun(HWND hwndApp)
{
    static UINT uMessage = 0;
    ULONG_PTR dwCancel = 0;

    if (!uMessage)
        uMessage = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

    SendMessageTimeout(hwndApp, uMessage, 0, 0, SMTO_NORMAL | SMTO_ABORTIFHUNG,
        1000, &dwCancel);

    return (dwCancel != 1); // check for exactly 1 (future expansion)
}

// Note: this overrides the one in CBaseBrowser
HRESULT CDesktopBrowser::GetViewRect(RECT* prc)
{
    //
    // Check if we are on a multiple-monitor system.  In multiple monitors the
    // view needs to cover all displays (ie the size of _pbbd->_hwnd) while on
    // single-monitor systems the view just needs to cover the work area (like
    // in Win95).
    //
    if (_nMonitors <= 1)
        _pbsInner->GetViewRect(prc);
    else
        GetClientRect(_pbbd->_hwnd, prc);

    return S_OK;
}

// BUGBUG LATER: This is only a temporary solution for NT.  We can have a much more
// efficient implementation inside USER on both NT and Win9x.  Then we could have
// common NT/Win9x code that says "if OnActiveDesktop() ..." below in Desktop_OnFSNotify

BOOL IsMyDesktopActive()
{ 
    BOOL bResult = FALSE;
    DWORD cb;

    if (!g_fRunningOnNT)    // Win95 does not support this stuff, assume it is OK
        return TRUE;

    HDESK hdeskInput = OpenInputDesktop(0, FALSE, STANDARD_RIGHTS_REQUIRED | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
    if (hdeskInput)
    {
        TCHAR szInput[256];
        if (GetUserObjectInformation(hdeskInput, UOI_NAME, (PVOID)szInput, sizeof(szInput), &cb)) 
        {
            HDESK hdeskMyDesktop = GetThreadDesktop(GetCurrentThreadId());
            if (hdeskMyDesktop)
            {
                TCHAR szMyDesktop[256];
                if (GetUserObjectInformation(hdeskMyDesktop, UOI_NAME, (PVOID)szMyDesktop, sizeof(szMyDesktop), &cb)) 
                {
                    bResult = lstrcmpi(szInput, szMyDesktop) == 0;
                }
            }
        }
        CloseDesktop(hdeskInput);
    }

    return bResult;
}


void CDesktopBrowser::_OnChangeNotify(LONG lNotification, LPITEMIDLIST *ppidl)
{
    // Currently only handle SHCNE_DRIVEADDGUI...
    if (lNotification == SHCNE_DRIVEADDGUI)
    {
        HWND hwnd = GetForegroundWindow();
        BOOL fShellForeground;
        TCHAR szDrive[MAX_PATH];
        int iDrive;
        DWORD dwRestricted;

        // dont run anything if the SHIFT key is down
        if (GetAsyncKeyState(VK_SHIFT) < 0)
        {
            TraceMsg(DM_TRACE, "Cabinet: SHIFT key is down skipping AutoOpen");
            return;
        }

        SHGetPathFromIDList(*ppidl, szDrive);
        ASSERT(szDrive[1] == TEXT(':'));

        if ((0 == *(szDrive + 2) || 0 == *(szDrive + 3)))
        {
            iDrive = DRIVEID(szDrive);

            // Now make sure that this drive is not restricted!
            // Handles cases where drives are mapped in under the covers
            // like when a drivespace floppy is discovered.
            //
            dwRestricted = SHRestricted(REST_NODRIVES);
            if (dwRestricted & (1 << iDrive))
            {
                // Restricted drive...
                TraceMsg(DM_TRACE, "_OnChangeNotify: Restricted Drive(%c)", szDrive[0]);
                return;
            }

            // On NT, bail out now if we're on the secure desktop (locked 
            // workstation or password-protected screensaver)
            if (!IsMyDesktopActive())
                return;

            fShellForeground = hwnd && (hwnd == _hwndTray || hwnd == _pbbd->_hwnd || IsFolderWindow(hwnd));

            // Always shlexec if its a net drive being added - gives feedback to user

            BOOL fShellExec = FALSE;

            if (IsNetDrive(iDrive))
            {
                fShellExec = TRUE;
            }
            else
            {
                if (CMountPoint::IsAutoOpen(iDrive))
                {
                    fShellExec = TRUE;
                }
                else
                {
                    if (fShellForeground && CMountPoint::IsShellOpen(iDrive))
                    {
                        fShellExec = TRUE;
                    }
                }

                if (fShellExec)
                {
                    if (CMountPoint::IsAutoRun(iDrive) && !AppAllowsAutoRun(hwnd))
                    {
                        fShellExec = FALSE;
                    }
                    else
                    {
                        // Do we have a fixed disk?
                        if (DRIVE_FIXED == GetDriveType(szDrive))
                        {
                            // Yes.
                            // Are we running on server?
                            if (IsOS(OS_WIN2000SERVER) || IsOS(OS_WIN2000ADVSERVER) || IsOS(OS_WIN2000DATACENTER))
                            {
                                // Yes.  On server having cluster service, when one disk goes down and another one
                                // replace it, we used to autoopen an explorer on the drive.  This was very annoying
                                // and a lot of explorer ended up opening.  BUG# 376481 (stephstm, 08/20/99)
                                fShellExec = FALSE;
                            }
                            else
                            {
                                // Is this a docked laptop?
                                if (SHGetMachineInfo(GMID_DOCKED))
                                {
                                    // Yes.  On docking station having a hard disk (not in the laptop, but in the 
                                    // docking station), each time the laptop is docked, we receive a WM_DEVICECHANGE
                                    // and used to popup an Explorer on the drive.  BUG# 358361 (stephstm, 08/20/99)
                                    fShellExec = FALSE;
                                }
                            }
                        }
                    }
                }
            }

            if (fShellExec)
            {
                //
                // use ShellExecuteEx() so the default verb gets invoked
                // (may not be open or even explore)
                //
                SHELLEXECUTEINFO ei = {
                    SIZEOF(ei),                 // size
                    SEE_MASK_INVOKEIDLIST,      // flags
                    _pbbd->_hwnd,                      // parent window
                    NULL,                       // verb
                    NULL,                       // file
                    szDrive,                    // params
                    szDrive,                    // directory
                    SW_NORMAL,                  // show.
                    NULL,                       // hinstance
                    *ppidl,                     // IDLIST
                    NULL,                       // class name
                    NULL,                       // class key
                    0,                          // hot key
                    NULL,                       // icon
                    NULL,                       // hProcess
                };

    #ifdef BUGBUG // IMPLEMENT LATER
                // BUGBUG: aahhh.. look how we force a new window!
                BOOL fPrevMode = g_CabState.fNewWindowMode;
                g_CabState.fNewWindowMode = TRUE;
    #endif //!BUGBUG

                if (!ShellExecuteEx(&ei))
                {
                    TraceMsg(DM_TRACE, "Cabinet: ShellExecuteEx() failed on drive notify");
                }
    #ifdef BUGBUG            
                g_CabState.fNewWindowMode = fPrevMode;
    #endif            
            }
        }
    }
}

HRESULT CDesktopBrowser::ReleaseShellView()
{
    _SaveState();

    return _pbsInner->ReleaseShellView();
}

void CDesktopBrowser::_ViewChange(DWORD dwAspect, LONG lindex)
{
    // do nothing here...
}

void CDesktopBrowser::_SaveDesktopToolbars()
{
    IStream * pstm = GetDesktopViewStream(STGM_WRITE, TEXT("Toolbars"));
    if (pstm) 
    {
        _pbsInner->_SaveToolbars(pstm);
        pstm->Release();
    }
}

void CDesktopBrowser::_SaveState()
{
    // save off the Recycle Bin information to the registry
    SaveRecycleBinInfo();

    if (!SHRestricted(REST_NOSAVESET) && _pbbd->_psv)
    {
        if (!GetSystemMetrics(SM_CLEANBOOT))
        {
            FOLDERSETTINGS fs;
            _pbbd->_psv->GetCurrentInfo(&fs);

            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\DeskView"), TEXT("Settings"), REG_BINARY, &fs, SIZEOF(fs));
        }

        _pbbd->_psv->SaveViewState();

        _SaveDesktopToolbars();
    }
}

HRESULT CDesktopBrowser::OnSize(WPARAM wParam)
{
    if (wParam == SIZE_MINIMIZED)
    {
        TraceMsg(DM_TRACE, "c.dwp: Desktop minimized by somebody!");
        // Put it back.
        ShowWindow(_pbbd->_hwnd, SW_RESTORE);
    }
    _SetViewArea();
    
    return S_OK;
}



HRESULT CDesktopBrowser::OnDestroy()
{
    TraceMsg(DM_SHUTDOWN, "cdtb._od (WM_DESTROY)");

    // Tell fsnotiy that he ain't got our window to kick around any mo'
    SHChangeNotify_DesktopTerm();

    if (_uNotifyID)
    {
        SHChangeNotifyDeregister(_uNotifyID);
        _uNotifyID = 0;
    }

    if (_hwndRaised) 
        DestroyWindow(_hwndRaised);

    //  get rid of the scheduler and the desktop tasks
    if (_psched)
    {
        _psched->RemoveTasks(TOID_Desktop, 0, TRUE);
        ATOMICRELEASE(_psched);
    }

    _pbsInner->OnDestroy();

    // BUGBUG: call this (q: what happens if we don't terminate this?)
    //OneTree_Terminate();
    
    _pbsInner->_CloseAndReleaseToolbars(TRUE);
    
    return S_OK;
}

#define DM_SWAP DM_TRACE

void CDesktopBrowser::_SwapParents(HWND hwndOldParent, HWND hwndNewParent)
{
    HWND hwnd = ::GetWindow(hwndOldParent, GW_CHILD);
    while (hwnd) 
    {
        //
        //  Note that we must get the next sibling BEFORE we set the new
        // parent.
        //
        HWND hwndNext = ::GetWindow(hwnd, GW_HWNDNEXT);
        ::SetParent(hwnd, hwndNewParent);
        hwnd = hwndNext;
    }
}

LRESULT CDesktopBrowser::RaisedWndProc(HWND  hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDesktopBrowser *psb = (CDesktopBrowser*)GetWindowLongPtr(hwnd, 0);

    return psb->_RaisedWndProc(uMsg, wParam, lParam);
}

LRESULT CDesktopBrowser::_RaisedWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATE:
        
#if 0
        if (_fRaised) {
            
            if (LOWORD(wParam) == WA_ACTIVE) {
                // always put the tray on top
                SetWindowPos(_hwndTray, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
            }
            if (LOWORD(wParam) == WA_INACTIVE) {
                DWORD dwProcID;
                HWND hwndForeground = GetForegroundWindow();
                
                if (!hwndForeground ||
                    !(GetWindowLongPtr(hwndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST) ||
                    GetWindowThreadProcessId(hwndForeground, &dwProcID), dwProcID != GetCurrentProcessId())
                {
                    _OnRaise((WPARAM)_hwndTray, DTRF_LOWER);
                }
            }
        }
#endif
        break;
        
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) 
            ShowWindow(_hwndRaised, SW_RESTORE);
        break;
        
    case WM_NOTIFY:
    case WM_ERASEBKGND:
        goto SendToDesktop;



    default:
        
        if (uMsg >= WM_USER) 
        {
SendToDesktop:
            return SendMessage(_pbbd->_hwnd, uMsg, wParam, lParam);
        } 
        else 
        {
            return ::SHDefWindowProc(_hwndRaised, uMsg, wParam, lParam);
        }
    }
    
    return 0;
}


void CDesktopBrowser::_Raise()
{
    RECT rc;
    HWND hwndDesktop = GetDesktopWindow();
    BOOL fLocked;
    HWND hwndLastActive = GetLastActivePopup(_pbbd->_hwnd);
    
    if (SHIsRestricted(NULL, REST_NODESKTOP))
        return;

    if (hwndLastActive != _pbbd->_hwnd) 
    {
        SetForegroundWindow(hwndLastActive);
        return;
    }

    if (!_hwndRaised)
        _hwndRaised = SHCreateWorkerWindow(RaisedWndProc, NULL, WS_EX_TOOLWINDOW, WS_POPUP | WS_CLIPCHILDREN, NULL, this);

    //SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    //rc.left = 0;  // need to always start from 0, 0 to get the wallpaper centered right
    //rc.top = 0;
    fLocked = LockWindowUpdate(hwndDesktop);
    _SwapParents(_pbbd->_hwnd, _hwndRaised);
    
    // set the view window to the bottom of the z order
    SetWindowPos(_pbbd->_hwndView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    GetWindowRect(_pbbd->_hwnd, &rc);
    SetWindowPos(_hwndRaised, HWND_TOP, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_SHOWWINDOW);
    SetForegroundWindow(_hwndRaised);

    if (fLocked)
        LockWindowUpdate(NULL);

    THR(RegisterDragDrop(_hwndRaised, (IDropTarget *)this));

    SetFocus(_pbbd->_hwndView);
    //SetWindowPos(_hwndTray, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
    _fRaised = TRUE;
}

void CDesktopBrowser::_Lower()
{
    BOOL fLocked;

    fLocked = LockWindowUpdate(_hwndRaised);
    _SwapParents(_hwndRaised, _pbbd->_hwnd);

    // set the view window to the bottom of the z order
    SetWindowPos(_pbbd->_hwndView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    ShowWindow(_hwndRaised, SW_HIDE);
    if (fLocked)
        LockWindowUpdate(NULL);
    RevokeDragDrop(_hwndRaised);
    _fRaised = FALSE;
}

void CDesktopBrowser::_OnRaise(WPARAM wParam, LPARAM lParam)
{
    VARIANTARG vaIn;

    VariantInit(&vaIn);
    vaIn.vt = VT_I4;
    vaIn.lVal = (DWORD) lParam;

    switch (lParam) {
        
    case DTRF_RAISE:
        _Raise();
        _ExecChildren(NULL, TRUE, &CGID_ShellDocView, SHDVID_RAISE, 0, &vaIn, NULL);
        break;
        
    case DTRF_LOWER:
        _ExecChildren(NULL, TRUE, &CGID_ShellDocView, SHDVID_RAISE, 0, &vaIn, NULL);
        _Lower();
        break;
    }

    VariantClear(&vaIn);

    if (!wParam) {
        wParam = (WPARAM)_hwndTray;
    }
    
    PostMessage((HWND)wParam, TM_DESKTOPSTATE, 0, _fRaised ? DTRF_RAISE : DTRF_LOWER);
}

#ifdef WINNT
BOOL CDesktopBrowser::_QueryHKCRChanged(HWND hwnd, DWORD *pdwCookie)
{
    //  we assume that the key has changed if 
    //  we were unable to init correctly
    BOOL fRet = TRUE;

    ASSERT(pdwCookie);
    
    if (_cChangeEvents)
    {
        DWORD dw = WaitForMultipleObjects(_cChangeEvents, _rghChangeEvents, FALSE, 0);

        dw -= WAIT_OBJECT_0;

        if (dw >= 0 && dw < _cChangeEvents)
        {

            //  this means the key changed...
            ResetEvent(_rghChangeEvents[dw]);
            _dwChangeCookie = GetTickCount();

            PostMessage(hwnd, DTM_SETUPAPPRAN, 0, NULL);
        }

        //
        //  if nothing has changed yet, or if nothing has changed
        //  since the client last checked,
        //  than this client doesnt need to update its cache
        //
        if (!_dwChangeCookie ||  (*pdwCookie && *pdwCookie  == _dwChangeCookie))
            fRet = FALSE;

        //  update the cookie
        *pdwCookie = _dwChangeCookie;
    }

    return fRet;
}
#endif //WINNT

// This msg gets posted to us after a setup application runs so that we can
// fix things up.

void CDesktopBrowser::_SetupAppRan(WPARAM wParam, LPARAM lParam)
{
    // Lotus 123R5 sometimes gets confused when installing over IE4 and
    // they leave their country setting blank. Detect this case and put
    // in USA so that they at least boot.       
    {
        TCHAR szPath[MAX_PATH];
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        if (PathAppend(szPath, TEXT("123r5.ini")) && PathFileExistsAndAttributes(szPath, NULL))
        {
            TCHAR szData[100];
        
            GetPrivateProfileString(TEXT("Country"), TEXT("driver"), TEXT(""), szData, ARRAYSIZE(szData), TEXT("123r5.ini"));
            if (!szData[0])
            {
                WritePrivateProfileString(TEXT("Country"), TEXT("driver"), TEXT("L1WUSF"), TEXT("123r5.ini"));
            }
        }
    }

    // Win95 post setup app processing wacks in SoundRec for .wav associations.
    // We want to fix this up to be wavfile for active movie wav file handling.
    {
        TCHAR szName[MAX_PATH];
        DWORD cbData = ARRAYSIZE(szName);
        SHGetValue(HKEY_CLASSES_ROOT, TEXT(".WAV"), NULL, NULL, &szName, &cbData);
        if (!lstrcmp(szName, TEXT("SoundRec")))
        {
            SHSetValue(HKEY_CLASSES_ROOT, TEXT(".WAV"), NULL, REG_SZ, TEXT("wavfile"), sizeof(TEXT("wavfile")));
        }
    }

    //  this location in the registry is a good place to cache info
    //  that needs to be invalided once 
    SHDeleteKey(HKEY_CURRENT_USER, STRREG_DISCARDABLE STRREG_POSTSETUP) ;

    //  Take this opportunity to freshen our component categories cache:
    IShellTaskScheduler* pScheduler ;
    if( SUCCEEDED( CoCreateInstance( CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC_SERVER, 
                                     IID_IShellTaskScheduler, (LPVOID*)&pScheduler ) ) )
    {
        IRunnableTask* pTask ;
        if( SUCCEEDED( CoCreateInstance( CLSID_ComCatCacheTask, NULL, CLSCTX_INPROC_SERVER,
                                         IID_IRunnableTask, (void**)&pTask ) ) )
        {
            pScheduler->AddTask( pTask, CLSID_ComCatCacheTask, 0L, ITSAT_DEFAULT_PRIORITY ) ;
            pTask->Release() ;  // Scheduler has AddRef'd him
        }
        pScheduler->Release() ; // OK to release global task scheduler.
    }


    // We get this notification from the OS that a setup app ran.
    //  Legacy app support for freshly written entries under [extensions] section;
    //  Scoop these up and shove into registry.   (re: bug 140986)
    CheckWinIniForAssocs() ;
}


//-----------------------------------------------------------------------------
struct propagatemsg
{
    UINT   uMsg;
    WPARAM wParam;
    LPARAM lParam;
    BOOL   fSend;
};

BOOL PropagateCallback(HWND hwndChild, struct propagatemsg *pmsg)
{
    if (pmsg->fSend)
        SendMessage(hwndChild, pmsg->uMsg, pmsg->wParam, pmsg->lParam);
    else
        PostMessage(hwndChild, pmsg->uMsg, pmsg->wParam, pmsg->lParam);

    return TRUE;
}

void PropagateMessage(HWND hwndParent, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    if (!hwndParent)
        return;

    struct propagatemsg msg = { uMsg, wParam, lParam, fSend };

    EnumChildWindows(hwndParent, (WNDENUMPROC)PropagateCallback, (LPARAM)&msg);
}

void CDesktopBrowser::v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    // WARNING: We can't propagate the message to a NULL hwnd because it will
    // turn into a broadcast.  This will come back to us and we will re-send it
    // causing an infinite loop.  BryanSt.

    if (_fRaised && EVAL(_hwndRaised)) 
        PropagateMessage(_hwndRaised, uMsg, wParam, lParam, fSend);
    else if (EVAL(_pbbd->_hwnd))
        PropagateMessage(_pbbd->_hwnd, uMsg, wParam, lParam, fSend);
}

//***
// NOTES
//  in the tray case, we actually do SetFocus etc.
HRESULT CDesktopBrowser::v_MayGetNextToolbarFocus(LPMSG lpMsg,
    UINT itbCur, int citb,
    LPTOOLBARITEM * pptbi, HWND * phwnd)
{
    HRESULT hr;

    // _fTrayHack?

    if (itbCur == ITB_VIEW) {
        if (citb == -1) {
            TraceMsg(DM_FOCUS, "cdtb.v_mgntf: ITB_VIEW,-1 => tray");
            goto Ltray;
        }
    }

    hr = _pbsInner->v_MayGetNextToolbarFocus(lpMsg, itbCur, citb, pptbi, phwnd);
    TraceMsg(DM_FOCUS, "cdtb.v_mgntf: SUPER hr=%x", hr);
    if (SUCCEEDED(hr)) {
        // S_OK: we got and handled a candidate
        // S_FALSE: we got a candidate and our parent will finish up
        ASSERT(hr != S_OK);  // currently never happens (but should work)
        return hr;
    }

    // E_xxx: no candidate
    ASSERT(citb == 1 || citb == -1);
    *pptbi = NULL;
    if (citb == 1) {
Ltray:
        *phwnd = _hwndTray;
        // AndyP REVIEW: why do we do this here instead of overriding
        // _SetFocus and letting commonsb call that function? Sure, this
        // is one less override, but why have different code paths?
        SendMessage(_hwndTray, TM_UIACTIVATEIO, TRUE, citb);
        return S_OK;
    }
    else 
    {
//Lview:
        *phwnd = _pbbd->_hwndView;
        return S_FALSE;
    }
    /*NOTREACHED*/
    ASSERT(0);
}

//
// NOTE: Please think before calling this function, in a multi-monitor system this function
// returns TRUE if you are within a certain edge for any monitor, so puEdge means puEdge
// of a certain monitor instead of the whole desktop. -- dli
//
BOOL CDesktopBrowser::_PtOnDesktopEdge(POINTL* ppt, LPUINT puEdge)
{
    RECT rcMonitor;
    POINT pt = {ppt->x, ppt->y};
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
    // We got this point from drop, so it definitely should belong to a valid monitor -- dli
    ASSERT(hMon);
    GetMonitorRect(hMon, &rcMonitor); 

    // if it's near/on the edge on this monitor
    if (ppt->x < rcMonitor.left + g_cxEdge) {
        *puEdge = ABE_LEFT;
    } else if (ppt->x > rcMonitor.right - g_cxEdge) {
        *puEdge = ABE_RIGHT;
    } else if (ppt->y < rcMonitor.top + g_cyEdge) {
        *puEdge = ABE_TOP;
    } else if (ppt->y > rcMonitor.bottom - g_cyEdge) {
        *puEdge = ABE_BOTTOM;
    } else {
        *puEdge = (UINT)-1;
        return FALSE;
    }
    return TRUE;
}

UINT g_cfDeskBand = 0;

HRESULT CDesktopBrowser::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    ASSERT(pdtobj);
    if (!g_cfDeskBand)
        g_cfDeskBand = RegisterClipboardFormat(TEXT("DeskBand"));

    FORMATETC fmte = { (CLIPFORMAT) g_cfDeskBand, NULL, 0, -1, TYMED_ISTREAM};
    if (pdtobj->QueryGetData(&fmte) == S_OK) {
        _dwEffectOnEdge = DROPEFFECT_COPY | DROPEFFECT_MOVE;
    } else {
        _dwEffectOnEdge = DROPEFFECT_NONE;
    }
    
    _grfKeyState = grfKeyState;

    HRESULT hr;

    if (_pdtInner)
        hr = _pdtInner->DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
    else
    {
        _DragEnter(_pbbd->_hwndView, ptl, pdtobj);
        hr = S_OK;
    }

    return hr;
}

HRESULT CDesktopBrowser::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    HRESULT hr = S_OK;
    _DragMove(_pbbd->_hwndView, ptl);
    _grfKeyState = grfKeyState;
    
    if (_dwEffectOnEdge != DROPEFFECT_NONE) {
        *pdwEffect &= _dwEffectOnEdge;
        return S_OK;
    }

    if (_pdtInner)
        hr = _pdtInner->DragOver(grfKeyState, ptl, pdwEffect);
    
    return hr;
    
}

HRESULT DeskBarApp_Create(IUnknown** ppunkBar, IUnknown** ppunkBandSite)
{
    IDeskBar* pdb;
    HRESULT hres = CoCreateInstance(CLSID_DeskBarApp, NULL, CLSCTX_INPROC_SERVER, IID_IDeskBar, (void **)&pdb);
    *ppunkBar = pdb;
    if (SUCCEEDED(hres)) {
        pdb->GetClient(ppunkBandSite);
    }
    return hres;
}

//***
// ENTRY/EXIT
//  hres        AddBand result on success; o.w. failure code
// NOTES
//  n.b. on success we *must* return AddBand's hres (which is a dwBandID)
HRESULT CDesktopBrowser::_CreateDeskBarForBand(UINT uEdge, IUnknown *punk, POINTL *pptl, IBandSite **ppbsOut)
{
    IBandSite *pbs;
    IUnknown *punkBar;
    IUnknown *punkBandSite;
    HRESULT hres;
#ifdef DEBUG
    HRESULT hresRet = -1;
#endif

    if (ppbsOut)
        *ppbsOut = NULL;

    hres = DeskBarApp_Create(&punkBar, &punkBandSite);
    if (SUCCEEDED(hres))
    {
        IDockingBarPropertyBagInit* ppbi;

        if (SUCCEEDED(CoCreateInstance(CLSID_CDockingBarPropertyBag, NULL, CLSCTX_INPROC_SERVER,
                        IID_IDockingBarPropertyBagInit, (void **)&ppbi)))
        {
            if ((UINT)uEdge != -1) {
                ppbi->SetDataDWORD(PROPDATA_MODE, WBM_BOTTOMMOST);
                ppbi->SetDataDWORD(PROPDATA_SIDE, uEdge);
            } else {
                ppbi->SetDataDWORD(PROPDATA_MODE, WBM_FLOATING);
            }

            ppbi->SetDataDWORD(PROPDATA_X, pptl->x);
            ppbi->SetDataDWORD(PROPDATA_Y, pptl->y);

            
            IPropertyBag * ppb;
            if (SUCCEEDED(ppbi->QueryInterface(IID_IPropertyBag, (void **)&ppb)))
            {
                SHLoadFromPropertyBag(punkBar, ppb);

                punkBandSite->QueryInterface(IID_IBandSite, (void **)&pbs);

                if (pbs) {
                    hres = pbs->AddBand(punk);
#ifdef DEBUG
                    hresRet = hres;
#endif

                    AddToolbar(punkBar, L"", NULL);

                    if (ppbsOut) {
                        // IUnknown_Set (sort of...)
                        *ppbsOut = pbs;
                        (*ppbsOut)->AddRef();
                    }

                    pbs->Release();

                    if (_fRaised) {
                        VARIANTARG vaIn = { 0 };

                        //VariantInit(&vaIn);
                        vaIn.vt = VT_I4;
                        vaIn.lVal = DTRF_RAISE;

                        ASSERT(punkBar != NULL);    // o.w. we'd do a broadcast
                        _ExecChildren(punkBar, FALSE, &CGID_ShellDocView, SHDVID_RAISE, 0, &vaIn, NULL);
                        //VariantClear(&vaIn);
                    }
                }

                ppb->Release();
            }

            ppbi->Release();
        }

        punkBandSite->Release();
        punkBar->Release();        
    }
    
    ASSERT(hres == hresRet || FAILED(hres));
    return hres;
}

HRESULT CDesktopBrowser::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UINT uEdge;
    HRESULT hres = E_FAIL;
    
    if (((_PtOnDesktopEdge(&pt, &uEdge) && (_grfKeyState & MK_LBUTTON)) ||
        (_dwEffectOnEdge != DROPEFFECT_NONE)) && !SHRestricted(REST_NOCLOSE_DRAGDROPBAND)) {
        
        
        // if the point is on the edge of the desktop and the item dropped was 
        // a single url object, then create a webbar
        // TODO: (reuse) w/ a little restructuring we might share this code
        // w/ CBandSite::Drop etc.

        FORMATETC fmte = {(CLIPFORMAT)g_cfDeskBand, NULL, 0, -1, TYMED_ISTREAM};
        STGMEDIUM stg;
        LPITEMIDLIST pidl;
        IUnknown* punk = NULL;

        // we can move a band from bar to bar, but we can only copy or link a folder
        // because the creation of a band relies on the source still abeing there
        if ((*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_MOVE)) &&
            SUCCEEDED(pdtobj->GetData(&fmte, &stg))) {

            // this is a drag of a band from another bar, create it!
            hres = OleLoadFromStream(stg.pstm, IID_IUnknown, (void **)&punk);
            if (SUCCEEDED(hres)) {
                if (*pdwEffect & DROPEFFECT_COPY)
                    *pdwEffect = DROPEFFECT_COPY;
                else 
                    *pdwEffect = DROPEFFECT_MOVE;
            }
            ReleaseStgMedium(&stg);

        } else if ((*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_LINK)) &&
                   SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0))) {

            hres = SHCreateBandForPidl(pidl, &punk, (grfKeyState & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT));

            ILFree(pidl);

            if (SUCCEEDED(hres)) {
                if (*pdwEffect & DROPEFFECT_LINK)
                    *pdwEffect = DROPEFFECT_LINK;
                else
                    *pdwEffect = DROPEFFECT_COPY;
            }

        }

        if (SUCCEEDED(hres)) {

            if (punk) {
                IBandSite *pbs;
                hres = _CreateDeskBarForBand(uEdge, punk, &pt, &pbs);

                if (SUCCEEDED(hres)) {
                    DWORD dwState;

                    dwState = IDataObject_GetDeskBandState(pdtobj);
                    pbs->SetBandState(ShortFromResult(hres), BSSF_NOTITLE, dwState & BSSF_NOTITLE);
                    pbs->Release();
                }

                punk->Release();
            }

            IDropTarget *pdtView;
            HRESULT hr = E_FAIL;

            //Get the view's drop target 
            if (_pbbd->_psv)
            {
                hr =  _pbbd->_psv->QueryInterface(IID_IDropTarget, (void **)&pdtView);
                if (SUCCEEDED(hr))
                {
                   pdtView->DragLeave();
                   pdtView->Release();
                }
            }
            return hres; 
        }


        // if we failed, pass this on to our child.
        // this allows things like d/d of wallpaper to the edge to do 
        // right thing
    } 
    
    if (_pdtInner)
        hres = _pdtInner->Drop(pdtobj, grfKeyState, pt, pdwEffect);
    
    return hres;
} 

BOOL _GetToken(LPCTSTR *ppszCmdLine, LPTSTR szToken, UINT cchMax)
{
    LPCTSTR pszCmdLine = *ppszCmdLine;

    TCHAR chTerm = ' ';
    if (*pszCmdLine == TEXT('"')) {
        chTerm = TEXT('"');
        pszCmdLine++;
    }

    UINT ichToken = 0;
    TCHAR ch;
    while((ch=*pszCmdLine) && (ch != chTerm)) {
        if (ichToken < cchMax-1) {
            szToken[ichToken++] = ch;
        }
        pszCmdLine++;
    }

    szToken[ichToken] = TEXT('\0');

    if (chTerm == TEXT('"') && ch == TEXT('"')) {
        pszCmdLine++;
    }

    // skip trailing spaces
    while(*pszCmdLine == TEXT(' '))
        pszCmdLine++;

    *ppszCmdLine = pszCmdLine;

    TraceMsg(TF_SHDAUTO, "_GetToken returning %s (+%s)", szToken, pszCmdLine);

    return szToken[0];
}


BOOL CDesktopBrowser::_OnCopyData(PCOPYDATASTRUCT pcds)
{
    IETHREADPARAM *piei = SHCreateIETHREADPARAM(NULL, (ULONG)pcds->dwData, NULL, NULL);
    if (piei)
    {
        LPCWSTR pwszSrc = (LPCWSTR)pcds->lpData;
        LPCWSTR pwszDdeRegEvent = NULL;
        LPCWSTR pwszCloseEvent = NULL;
        DWORD cchSrc = pcds->cbData / SIZEOF(WCHAR);

        piei->uFlags = COF_NORMAL | COF_WAITFORPENDING | COF_IEXPLORE;

        //
        // Remember where the command line parameters are.
        //
        LPCWSTR pszCmd = pwszSrc;
        int cch = lstrlenW(pwszSrc) + 1;
        pwszSrc += cch;
        cchSrc -= cch;

        TraceMsg(TF_SHDAUTO, "CDB::_OnCopyData got %hs", pszCmd);

        //
        // Get the dde reg event name into piei->szDdeRegEvent.
        //

        // NOTE: this is now conditional because we now launch the channel band from the desktop
        // NOTE: as a fake WM_COPYDATA command
        if (cchSrc)
        {
            ASSERT(cchSrc);
            pwszDdeRegEvent = pwszSrc;
            StrCpyNW(piei->szDdeRegEvent, pwszSrc, ARRAYSIZE(piei->szDdeRegEvent));
            cch = lstrlenW(pwszSrc) + 1;
            pwszSrc += cch;
            cchSrc -= cch;
            piei->uFlags |= COF_FIREEVENTONDDEREG;
            
            //
            // Get the name of the event to fire on close, if any.
            //
            if (cchSrc)
            {
                pwszCloseEvent = pwszSrc;
                StrCpyNW(piei->szCloseEvent, pwszSrc, ARRAYSIZE(piei->szCloseEvent));
                cch = lstrlenW(pwszSrc) + 1;
                pwszSrc += cch;
                cchSrc -= cch;
                piei->uFlags |= COF_FIREEVENTONCLOSE;
                
            }
        }
        
        ASSERT(cchSrc == 0);

        if (pszCmd && pszCmd[0])
        {
            // for compatibility with apps that spawn the browser with a command line
            // tell wininet to refresh its proxy settings. (this is particularly needed
            // for TravelSoft WebEx)
            MyInternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
            
            if (SHParseIECommandLine(&pszCmd, piei))
                piei->uFlags |= COF_NOFINDWINDOW;
            
            if (pszCmd[0] && FAILED(_ConvertPathToPidlW(_pbsInner, _pbbd->_hwnd, pszCmd, &piei->pidl)))
                piei->pidl = NULL;
        }
        else
        {
            piei->fCheckFirstOpen = TRUE;
        }

        // SHOpenFolderWindow takes ownership of piei
        BOOL fRes = SHOpenFolderWindow(piei);
        if (!fRes)
        {
            //
            // Something went wrong creating the browser,
            // let's fire all the events ourselves.
            //
            if (pwszDdeRegEvent) FireEventW(pwszDdeRegEvent);
            if (pwszCloseEvent) FireEventW(pwszCloseEvent);
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "OnCopyData unable to create IETHREADPARAM");
    }

    return TRUE;
}

BOOL CDesktopBrowser::_InitScheduler(void)
{
    if (!_psched)
    {
        // get the system background scheduler thread
        CoCreateInstance(CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC,
                         IID_IShellTaskScheduler, (void **) &_psched); 

    }
    return (_psched != NULL);
}
    

HRESULT CDesktopBrowser::_AddDesktopTask(IRunnableTask *ptask, DWORD dwPriority)
{
    if (_InitScheduler())
        return _psched->AddTask(ptask, TOID_Desktop, 0, dwPriority);

    return E_FAIL;
}

void CDesktopBrowser::_OnAddToRecent( HANDLE hMem, DWORD dwProcId )
{
    IRunnableTask *ptask;
    if (SUCCEEDED(CTaskAddDoc_Create(hMem, dwProcId, &ptask)))
    {
        _AddDesktopTask(ptask, ITSAT_DEFAULT_PRIORITY);
        ptask->Release();
    }
}

LRESULT CDesktopBrowser::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INSTRUMENT_WNDPROC(SHCNFI_DESKTOP_WNDPROC, _pbbd->_hwnd, uMsg, wParam, lParam);
    ASSERT(IsWindowTchar(hwnd));

    switch (uMsg)
    {
#ifdef DEBUG
    case WM_QUERYENDSESSION:
        TraceMsg(DM_SHUTDOWN, "cdtb.wp: WM_QUERYENDSESSION");
        goto DoDefault;
#endif

    case WM_ENDSESSION:
        TraceMsg(DM_SHUTDOWN, "cdtb.wp: WM_ENDSESSION wP=%d lP=%d", wParam, lParam);
        if (wParam)
        {
            SHELLSTATE ss = {0};

            // When we shut down, if the desktop is in WebView, we leave some temp
            // files undeleted because we never get the WM_DESTROY message below.
            // So, I just destroy the shellview which in turn destroys the temp
            // file here. Note: This is done only if we are in web view.

            SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); //Get the desktop_html flag
            if (ss.fDesktopHTML)
            {
                ReleaseShellView();
            }

            g_pdtray->SetVar(SVTRAY_EXITEXPLORER, FALSE);   // don't exit process

            // flush log before we exit
            if (StopWatchMode())
            {
                StopWatchFlush();
            }

            // Kill this window so that we free active desktop threads properly
            DestroyWindow(hwnd);
        }
        TraceMsg(DM_SHUTDOWN, "cdtb.wp: WM_ENDSESSION return 0");
        break;

    case WM_ERASEBKGND:
        PaintDesktop((HDC)wParam);
        return 1;

    case WM_TIMER:
        switch (wParam)
        {
        case IDT_DDETIMEOUT:
            DDEHandleTimeout(_pbbd->_hwnd);
            break;

        default:

            if (wParam >= IDT_DISKFULL  && (wParam <= IDT_DISKFULLLAST))
            {
                int idDrive = (int) (wParam - IDT_DISKFULL - 1);
                KillTimer(_pbbd->_hwnd, (UINT) wParam);
                TraceMsg(DM_TRACE, "Disk %c: is full", TEXT('a') +  idDrive);

                // are we already handling it?
                if (!((1 << idDrive) & _dwDiskFullDrives))
                {
                    _dwDiskFullDrives |= (1 << idDrive);
                    SHHandleDiskFull(_pbbd->_hwnd, idDrive);
                    _dwDiskFullDrives &= ~(1 << idDrive);
                }
                else
                {
                    TraceMsg(DM_TRACE, "Punting because we're already dealing");
                }
            }
        }
        break;

    case WM_SHELLNOTIFY:
        switch(wParam)
        {
        case SHELLNOTIFY_DISKFULL:
            SetTimer(_pbbd->_hwnd, IDT_DISKFULL + (UINT) lParam, DISKFULL_TIMEOUT, NULL);
            break;

        case SHELLNOTIFY_WALLPAPERCHANGED:
            // Changing the wallpaper through theme switcher and some other apps, do not 
            // result in WININICHANGE message. So, we handle this notification. However,
            // under NT, we do not get this notification. So, we do this under WinIniChange
            // processing too!
            if(!g_fRunningOnNT)
            {
                SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);
            }
            // this is done only to the shell window when someone sets
            // the wall paper but doesn't specify to broadcast
            _pbsInner->ForwardViewMsg(uMsg, wParam, lParam);
            break;
        }
        break;

    case WM_PALETTECHANGED:
    case WM_QUERYNEWPALETTE:
        //
        // in Win95 the desktop wndproc will invalidate the shell window when
        // a palette change occurs so we didn't have to do anything here before
        //
        // in Nashville the desktop can be HTML and needs the palette messages
        //
        // so now we fall through and propagate...
        //
    case WM_ACTIVATEAPP:
        if (!_pbbd->_hwndView)
            goto DoDefault;

        return _pbsInner->ForwardViewMsg(uMsg, wParam, lParam);

    case WM_DEVICECHANGE:
        _pbsInner->ForwardViewMsg(uMsg, wParam, lParam);
        _OnDeviceBroadcast(_pbbd->_hwnd, wParam, (DEV_BROADCAST_HDR *)lParam);
        goto DoDefault;

    case WM_WINDOWPOSCHANGING:
        #define ppos ((LPWINDOWPOS)lParam)
        ppos->x = g_xVirtualScreen;
        ppos->y = g_yVirtualScreen;
        ppos->cx = g_cxVirtualScreen;
        ppos->cy = g_cyVirtualScreen;
        break;

    case WM_CHANGENOTIFY:
        {
            LPITEMIDLIST *ppidl;
            LONG lEvent;
            LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
            if (pshcnl)
            {
                _OnChangeNotify(lEvent, ppidl);
                SHChangeNotification_Unlock(pshcnl);
            }
        }
        break;

    case WM_HOTKEY:
        // NOTE: forward hotkeys to the tray. This fixes the logitech mouseman who sends 
        // NOTE: hotkeys directly to the desktop. 
        // SPECIAL NOTE: the offset for GHID_FIRST is added because hotkeys that are sent to the
        // SPECIAL NOTE: desktop are not proper hotkeys generated from the keyboard, they are
        // SPECIAL NOTE: sent by an app, and the IDs have changed since win95....
        ASSERT( g_hwndTray );
        ASSERT( wParam < GHID_FIRST );
        PostMessage( g_hwndTray, uMsg, wParam + GHID_FIRST, lParam );
        return 0;
        
    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {
        // NB Dashboard 1.0 sends a WM_SYSCOMMAND SC_CLOSE to the desktop when it starts up.
        // What it was trying to do was to close down any non-shell versions of Progman. The
        // proper shell version would just ignore the close. Under Chicago, they think that
        // the desktop is Progman and send it the close, so we put up the exit windows dialog!
        // Dashboard 2.0 has been fixed to avoid this bogisity.
        case SC_CLOSE:
            break;

        // America alive tries to minimise Progman after installing - they end up minimising
        // the desktop on Chicago!
        case SC_MINIMIZE:
            break;

        default:
            goto DoDefault;
        }
        break;

    case WM_SETCURSOR:
        // REVIEW: is this really needed?
        if (_iWaitCount)
        {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
        else
            goto DoDefault;

    case WM_CLOSE:
        SendMessage(_hwndTray, TM_DOEXITWINDOWS, 0, 0);
        return 0;

        // REVIEW: do we need this, BUGBUG: can all of these cases be the same?
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (!_pbsInner->ForwardViewMsg(uMsg, wParam, lParam))
            goto DoDefault;
        break;

    case WM_INITMENUPOPUP:
    case WM_ENTERMENULOOP:
    case WM_EXITMENULOOP:
        // let the fsview deal with any popup menus it created
        _pbsInner->ForwardViewMsg(uMsg, wParam, lParam);
        break;

    // Looking at messages to try to capture when the workarea may
    // have changed...
    case WM_DISPLAYCHANGE:
        lParam = 0;
        if (GetNumberOfMonitors() != _nMonitors)
            _InitMonitors();
        
        // fall through

    case WM_WININICHANGE:
        _InitDesktopMetrics(wParam, (LPCTSTR)lParam);

        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            VARIANTARG varIn;
            VARIANTARG varOut = {0};
            varIn.vt = VT_BOOL;
            varIn.boolVal = VARIANT_TRUE;

            _pctInner->Exec(&CGID_Explorer, SBCMDID_CACHEINETZONEICON, OLECMDEXECOPT_DODEFAULT , &varIn, &varOut);
        }

        if (lParam)
        {
            if (lstrcmpi((LPCTSTR)lParam, TEXT("Extensions")) == 0)
            {
                // Post a message to our selves so we can do more stuff
                // slightly delayed.
                PostMessage(hwnd, DTM_SETUPAPPRAN, 0, 0);
            }
            else if (lstrcmpi((LPCTSTR)lParam, TEXT("ShellState")) == 0)
            {
                //  this should cover external apps changing
                //  our settings.
                SHRefreshSettings();
            }
            else
            {
                // SPI_GETICONTITLELONGFONT is broadcast from IE when the home page is changed.  We look for that so
                // we can be sure to update the MyCurrentHomePage component.
                if((wParam == SPI_SETDESKWALLPAPER) || (wParam == SPI_SETDESKPATTERN) || (wParam == SPI_GETICONTITLELOGFONT))
                {
                    // Some desktop attribute has changed. So, regenerate desktop.
                    if(lstrcmpi((LPCTSTR)lParam, TEXT("ToggleDesktop")) &&
                       lstrcmpi((LPCTSTR)lParam, TEXT("RefreshDesktop")) &&
                       lstrcmpi((LPCTSTR)lParam, TEXT("BufferedRefresh")))
                    {
                        DWORD dwFlags = AD_APPLY_HTMLGEN | AD_APPLY_REFRESH;

                        switch (wParam)
                        {
                            case SPI_SETDESKPATTERN:
                                dwFlags |= (AD_APPLY_FORCE | AD_APPLY_DYNAMICREFRESH);
                                break;
                            case SPI_SETDESKWALLPAPER:
                                dwFlags |= AD_APPLY_SAVE;
                                break;
                            case SPI_GETICONTITLELOGFONT:
                                dwFlags |= AD_APPLY_FORCE;
                                break;
                        }       

                        PokeWebViewDesktop(dwFlags);
                        
                        // If we are not currently in ActiveDesktop Mode, then we need to set the dirty bit
                        // sothat a new HTML file will be generated showing the new wallpaper,
                        // the next time the active desktop is turned ON!
                        if(g_fRunningOnNT && (wParam == SPI_SETDESKWALLPAPER))
                        {
                            SHELLSTATE ss = {0};
                            SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); //Get the desktop_html flag
                            if (!ss.fDesktopHTML)
                                SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);
                        }
                    }
                }
            }
            
        }

        if (g_fRunningOnNT)
        {
            // NT has a control panel applet that allows users to change the
            // environment with-which to spawn new applications.  On NT, we need
            // to pick up that environment change so that anything we spawn in
            // the future will pick up those updated environment values.
            //
            if (lParam && (lstrcmpi((LPTSTR)lParam, TEXT("Environment")) == 0))
            {
                void *pv;
                RegenerateUserEnvironment(&pv, TRUE);
            }
        }

        v_PropagateMessage(uMsg, wParam, lParam, TRUE);
        SetWindowPos(_pbbd->_hwnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER);
        if ((uMsg == WM_DISPLAYCHANGE) || (wParam == SPI_SETWORKAREA))
            _SetViewArea();
        break;

    case WM_SYSCOLORCHANGE:
        //
        // In memphis, when apps go into low-res mode, we get this message. We should not re-generate
        // desktop.htt in this scenario, or else the centered wallpaper gets screwed up because we do not
        // get the message when the app exists and the resoultion goes up. So, we make the following check.
        //
        if(!IsTempDisplayMode())
            OnDesktopSysColorChange();
        //This is done sothat the defview can set the listview in proper
        //colors.
        _pbsInner->ForwardViewMsg(uMsg, wParam, lParam);
        break;

    // Don't go to default wnd proc for this one...
    case WM_INPUTLANGCHANGEREQUEST:
        if (wParam)
            goto DoDefault;
        else
            return 0;

    case WM_COPYDATA:
        return _OnCopyData((PCOPYDATASTRUCT)lParam);

    case CWM_COMMANDLINE:
        SHOnCWMCommandLine(lParam);
        break;

    case CWM_FSNOTIFY:
        return SHChangeNotify_OnNotify(wParam, lParam);

    case CWM_CHANGEREGISTRATION:
        return (LRESULT)SHChangeRegistrationReceive((HANDLE)wParam, (DWORD)lParam);

    case CWM_FSNOTIFYSUSPENDRESUME:
        return (LRESULT)SHChangeNotifySuspendResumeReceive(wParam, lParam);

    case CWM_ADDTORECENT:
        _OnAddToRecent((HANDLE)wParam, (DWORD) lParam);
        return 0;

    case CWM_WAITOP:
        SHWaitOp_Operate((HANDLE)wParam, (DWORD)lParam);
        return 0;

    case CWM_SHOWFOLDEROPT:
        // appwiz.cpl sends this message to us
        DoGlobalFolderOptions();
        return 0;
                
    case DTM_CREATESAVEDWINDOWS:
        _InitDeskbars();
        SHCreateSavedWindows();
#ifdef ENABLE_CHANNELS
        _MaybeLaunchChannelBand();
#endif
        // we need to update the recycle bin icon because the recycle bin
        // is per user on NTFS, and thus the state could change w/ each new user
        // who logs in.
        SHUpdateRecycleBinIcon();
        break;
        
    case DTM_SAVESTATE:
        TraceMsg(DM_SHUTDOWN, "cdtb.wp: DTM_SAVESTATE");
        _SaveState();
        break;

    case DTM_RAISE:
        _OnRaise(wParam, lParam);
        break;
    
#ifdef DEBUG
    case DTM_NEXTCTL:
#endif
    case DTM_UIACTIVATEIO:
    case DTM_ONFOCUSCHANGEIS:
        _OnFocusMsg(uMsg, wParam, lParam);
        break;

    case DTM_SETUPAPPRAN:
        _SetupAppRan(wParam, lParam);
        break;

    case DTM_QUERYHKCRCHANGED:
#ifdef WINNT
        //  some clients are out of process, so we
        //  can cache their cookies for them.
        if (!lParam && wParam > QHKCRID_NONE && wParam < QHKCRID_MAX)
            lParam = (LPARAM)&_rgdwQHKCRCookies[wParam - QHKCRID_MIN];
            
        return _QueryHKCRChanged(hwnd, (LPDWORD)lParam);
#else
        // on win95 we assume it is the same
        return FALSE;
#endif //WINNNT
        
    case DTM_UPDATENOW:
        UpdateWindow(hwnd);
        break;

    case DTM_GETVIEWAREAS:
        {
            // wParam is an in/out param. in - the max. # of areas, out - the actual # of areas.
            // if "in" value < "out" value, lParam is not set.
            // The ViewAreas are already stored in the desktop Listview.
            int* pnViewAreas = (int*) wParam;
            LPRECT lprcViewAreas = (LPRECT) lParam;

            if (pnViewAreas)
            {
                int nMaxAreas = *pnViewAreas;
                HWND hwndList = _GetDesktopListview();

                ASSERT(IsWindow(hwndList));
                ListView_GetNumberOfWorkAreas(hwndList, pnViewAreas);
                if (*pnViewAreas >= 0 && *pnViewAreas <= nMaxAreas && lprcViewAreas)
                {
                    ListView_GetWorkAreas(hwndList, *pnViewAreas, lprcViewAreas);
                    // These are in Listview co-ordinates. We have to map them to screen co-ordinates.
                    // [msadek]; MapWindowPoints() is mirroring-aware only if you pass two points
                    for (int i = 0; i <  *pnViewAreas; i++)
                    {
                        MapWindowPoints(hwndList, HWND_DESKTOP, (LPPOINT)(&lprcViewAreas[i]), 2);
                    }    
                }
            }
        }
        break;

    case DTM_DESKTOPCONTEXTMENU:
        if (!SHRestricted(REST_NOACTIVEDESKTOP))
        {
            SetForegroundWindow(hwnd);
            SendMessage(_pbbd->_hwndView, WM_DSV_DESKTOPCONTEXTMENU, (WPARAM)_hwndTray, lParam);
        }
        break;

    case DTM_MAKEHTMLCHANGES:
        //Make changes to desktop's HTML using Dynamic HTML
        SendMessage(_pbbd->_hwndView, WM_DSV_DESKHTML_CHANGES, wParam, lParam);
        break;

    // Handle DDE messages for badly written apps (that assume the shell's
    // window is of class Progman and called Program Manager.
    case WM_DDE_INITIATE:
    case WM_DDE_TERMINATE:
    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_ACK:
    case WM_DDE_DATA:
    case WM_DDE_REQUEST:
    case WM_DDE_POKE:
    case WM_DDE_EXECUTE:
        return DDEHandleMsgs(_pbbd->_hwnd, uMsg, wParam, lParam);
        break;

    default:
        // Handle the MSWheel message - send it to the focus window if it isn't us
        extern UINT g_msgMSWheel;

        if (uMsg == g_msgMSWheel)
        {
            HWND hwndT = GetFocus();
            if (_pbbd->_hwnd != hwndT)
            {
                PostMessage(hwndT, uMsg, wParam, lParam);
                return 1;
            }
        }
        else if (uMsg == GetDDEExecMsg())
        {
            ASSERT(lParam && 0 == ((LPNMHDR)lParam)->idFrom);

            DDEHandleViewFolderNotify(NULL, _pbbd->_hwnd, (LPNMVIEWFOLDER)lParam);
            LocalFree((LPNMVIEWFOLDER)lParam);
            return TRUE;
        }

DoDefault:

        return _pbsInner->WndProcBS(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


#ifdef ENABLE_CHANNELS
// launch the channelbar, this is called when the desktop has finished starting up.
const WCHAR c_szwChannelBand[] = L"-channelband";

void CDesktopBrowser::_MaybeLaunchChannelBand( )
{
    DWORD dwType = REG_SZ;
    TCHAR  szYesOrNo[20];
    DWORD  cbSize = sizeof(szYesOrNo);
    
    BOOL bLaunchChannelBar = FALSE;
    
    if(SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), TEXT("Show_ChannelBand"),
                           &dwType, (LPVOID)szYesOrNo, &cbSize, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        bLaunchChannelBar = !lstrcmpi(szYesOrNo, TEXT("yes"));
    }
    // Don't launch by default post IE4
    //else if ( IsOS( OS_WINDOWS ))
    //{
    //    bLaunchChannelBar = TRUE;    // launch channel bar by default on Memphis and win95
    //}

    if ( bLaunchChannelBar )
    {
         // fake up a WM_COPYDATA struct 
        COPYDATASTRUCT cds;
        cds.dwData = SW_NORMAL;
        cds.cbData = sizeof( c_szwChannelBand );
        cds.lpData = (LPVOID) c_szwChannelBand;

        // fake it as if we had launched iexplore.exe, it saves us a whole process doing it this way....
        _OnCopyData( &cds );
    }
}
#endif // ENABLE_CHANNELS

//***
// NOTES
//  BUGBUG should this be CBaseBrowser::IInputObject::UIActIO etc.?
HRESULT CDesktopBrowser::_OnFocusMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fActivate = (BOOL) wParam;

    switch (uMsg) {
    case DTM_UIACTIVATEIO:
        fActivate = (BOOL) wParam;
        TraceMsg(DM_FOCUS, "cdtb.oxiois: DTM_UIActIO fAct=%d dtb=%d", fActivate, (int) lParam);

        if (fActivate) {
            MSG msg = {_pbbd->_hwnd, WM_KEYDOWN, VK_TAB, 0xf0001};
            BOOL bShift = (GetAsyncKeyState(VK_SHIFT) < 0);
            if (bShift) {
                int cToolbars = _pbsInner->_GetToolbarCount();
                while (--cToolbars >= 0) {
                    // activate last toolbar in tab order
                    LPTOOLBARITEM ptbi = _pbsInner->_GetToolbarItem(cToolbars);
                    if (ptbi && ptbi->ptbar) {
                        IInputObject* pio;
                        if (SUCCEEDED(ptbi->ptbar->QueryInterface(IID_IInputObject, (LPVOID*)&pio))) {
                            pio->UIActivateIO(TRUE, &msg);
                            pio->Release();
                            return S_OK;
                        }
                    }
                }
            }

#ifdef KEYBOARDCUES
            // Since we are Tab or Shift Tab we should turn the focus rect on.
            SendMessage(_pbbd->_hwnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);
#endif

            // activate view
            if (bShift && _pbbd->_psv)
            {
                _pbbd->_psv->TranslateAccelerator(&msg);
            }
            else
            {
                _pbsInner->_SetFocus(NULL, _pbbd->_hwndView, NULL);
            }
        }
        else {
Ldeact:
            // if we don't have focus, we're fine;
            // if we do have focus, there's nothing we can do about it...
            /*NOTHING*/
            ;
#ifdef DEBUG
            TraceMsg(DM_FOCUS, "cdtb.oxiois: GetFocus()=%x _pbbd->_hwnd=%x _pbbd->_hwndView=%x", GetFocus(), _pbbd->_hwnd, _pbbd->_hwndView);
#endif
        }

        break;

    case DTM_ONFOCUSCHANGEIS:
        TraceMsg(DM_FOCUS, "cdtb.oxiois: DTM_OnFocChgIS hwnd=%x fAct=%d", (HWND) lParam, fActivate);

        if (fActivate) {
            // someone else is activating, so we need to deactivate
            goto Ldeact;
        }
        // BUGBUG forward up? (to whom?)

        break;

    default:
        ASSERT(0);
        break;
    }

    return S_OK;
}

LRESULT CALLBACK CDesktopBrowser::DesktopWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDesktopBrowser *psb = (CDesktopBrowser*)GetWindowLongPtr(hwnd, 0);

    switch(uMsg)
    {
#ifdef KEYBOARDCUES
    case WM_CREATE:
        // Initialize our keyboard cues bits
        SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);

        // Set the localized name of the Desktop so it can be used in Error messages
        // that have the desktop window as the title.
        if (EVAL(LoadStringW(HINST_THISDLL, IDS_DESKTOP, psb->_wzDesktopTitle, ARRAYSIZE(psb->_wzDesktopTitle))))
        {
            EVAL(SetProp(hwnd, TEXT("pszDesktopTitleW"), (HANDLE)psb->_wzDesktopTitle));
        }

        if (psb)
            return psb->WndProcBS(hwnd, uMsg, wParam, lParam);
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam); // known charset

    case WM_ACTIVATE:
        if (WA_INACTIVE == LOWORD(wParam))
            SendMessage(hwnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

        goto DoDefault;
        break;
#endif

    case WM_NCCREATE:

        ASSERT(psb == NULL);

        CDesktopBrowser_CreateInstance(hwnd, (void **)&psb);

        SetWindowLongPtr(hwnd, 0, (LONG_PTR)psb);

        // BUGBUG needed?
        // DdeNewWindow(hwnd);
        if (psb)
            goto DoDefault;
        return 0;

    case WM_NCDESTROY:

        if (psb)
        {
            RemoveProp(hwnd, TEXT("pszDesktopTitleW"));
            // In case someone does a get shell window and post a WM_QUIT, we need to 
            //  make sure that we also close down our other thread.
            TraceMsg(DM_SHUTDOWN, "cdtb.wp(WM_NCDESTROY): ?post WM_QUIT hwndTray=%x(IsWnd=%d)", psb->_hwndTray, IsWindow(psb->_hwndTray));
            if (psb->_hwndTray && IsWindow(psb->_hwndTray))
                PostMessage(psb->_hwndTray, WM_QUIT, 0, 0);
            psb->ReleaseShellView();
            psb->Release();
        }

        PostQuitMessage(0); // exit out message loop
        break;

    default:
        if (psb)
            return psb->WndProcBS(hwnd, uMsg, wParam, lParam);
        else {
DoDefault:
            return DefWindowProc(hwnd, uMsg, wParam, lParam); // known charset
        }
    }

    return 0;
}

void RegisterDesktopClass()
{
    WNDCLASS wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CDesktopBrowser::DesktopWndProc;
    //wc.cbClsExtra = 0;
    wc.cbWndExtra = SIZEOF(void *);
    wc.hInstance = HINST_THISDLL;
    //wc.hIcon = NULL;
    wc.hCursor = GetClassCursor(GetDesktopWindow());
    wc.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
    //wc.lpszMenuName = NULL;
    wc.lpszClassName = DESKTOPCLASS;

    RegisterClass(&wc);
}

#ifdef BETA_WARNING // {
#pragma message("buidling with time bomb enabled")
void DoTimebomb(HWND hwnd)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    // BUGBUG: Remove after the beta-release!!!
    //
    // Revision History:
    //  End of October, 1996
    //  April, 1997
    //  September, 1997 (for beta-1)
    //  November 15th, 1997 (for beta-2)
    //
    if (st.wYear > 1997 || (st.wYear==1997 && st.wMonth > 11) ||
            (st.wYear==1997 && st.wMonth == 11 && st.wDay > 15))
    {
        TCHAR szTitle[128];
        TCHAR szBeta[512];

        LoadString(HINST_THISDLL, IDS_CABINET, szTitle, ARRAYSIZE(szTitle));
        LoadString(HINST_THISDLL, IDS_BETAEXPIRED, szBeta, ARRAYSIZE(szBeta));

        MessageBox(hwnd, szBeta, szTitle, MB_OK);
    }
}
#endif // BETA_WARNING // }


#define PEEK_NORMAL     0
#define PEEK_QUIT       1
#define PEEK_CONTINUE   2
#define PEEK_CLOSE      3


// RETURNS BOOL whehter to continue the search or not.
// so FALSE means we've found one.
// TRUE means we haven't.
BOOL CALLBACK FindBrowserWindow_Callback(HWND hwnd, LPARAM lParam)
{
    if (IsExplorerWindow(hwnd) || IsFolderWindow(hwnd) || IsTrayWindow(hwnd)) {
        DWORD dwProcID;
        GetWindowThreadProcessId(hwnd, &dwProcID);
        if (dwProcID == GetCurrentProcessId()) {
            if (lParam)
                *((BOOL*)lParam) = TRUE;    // found one!
            return FALSE;   // stop search
        }
    }
    return TRUE;            // continue search
}

#define IsBrowserWindow(hwnd)  !FindBrowserWindow_Callback(hwnd, NULL)

BOOL CALLBACK CloseWindow_Callback(HWND hwnd, LPARAM lParam)
{
    if (IsBrowserWindow(hwnd)) {
        TraceMsg(DM_SHUTDOWN, "s.cw_cb: post WM_CLOSE hwnd=%x", hwnd);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}


UINT CDesktopBrowser::_PeekForAMessage()
{
    MSG  msg;

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            TraceMsg(DM_SHUTDOWN, "cdtb.pfam: WM_QUIT wP=%d [lP=%d]", msg.wParam, msg.lParam);
            if (msg.lParam == 1) {
                return PEEK_CLOSE;
            }
            TraceMsg(DM_TRACE, "c.ml: Got quit message for %#08x", GetCurrentThreadId());
            return PEEK_QUIT;  // break all the way out of the main loop
        }

        if (_pbbd->_hwnd)
        {
            if (S_OK == _pbsInner->v_MayTranslateAccelerator(&msg))
                return PEEK_CONTINUE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        return PEEK_CONTINUE;   // Go back and get the next message
    }
    return PEEK_NORMAL;
}

void CDesktopBrowser::_MessageLoop()
{
    for ( ; ; )
    {
        switch (_PeekForAMessage())
        {
        case PEEK_QUIT:
            return;

        case PEEK_NORMAL:
            WaitMessage();
            break;

        case PEEK_CONTINUE:
            break;
            
        case PEEK_CLOSE:
            // we need to close all the shell windows too
            TraceMsg(DM_SHUTDOWN, "cdtb._ml: PEEK_CLOSE, close/wait all");
            EnumWindows(CloseWindow_Callback, 0);
            {
#define MAXIMUM_DESKTOP_WAIT 15000
                DWORD iStartTime = GetTickCount();
                // block until all other browser windows are closed
                for (;;) {
                    BOOL f = FALSE;
                    EnumWindows(FindBrowserWindow_Callback, (LPARAM)&f);
                    if (!f || (GetTickCount() - iStartTime > MAXIMUM_DESKTOP_WAIT))
                        return;

                    switch (_PeekForAMessage()) {
                    case PEEK_NORMAL:
                        // don't do a waitmessage because we want to exit when thelast other window is gone
                        // and we don't get a message to signal that
                        Sleep(100);
                        break;
                    }
                }
            }
            return;
        }
    }
}


HRESULT CDesktopBrowser::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_SShellDesktop))
        return QueryInterface(riid, ppvObj);

    return _pspInner->QueryService(guidService, riid, ppvObj);
}

void CDesktopBrowser::StartBackgroundShellTasks(void)
{
    TCHAR szClass[GUIDSTR_MAX];
    DWORD cchClass;
    DWORD dwType;
    int i = 0;

    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKey(g_hklmExplorer, TEXT("SharedTaskScheduler"), &hkey))
    {
        while (cchClass = ARRAYSIZE(szClass),
               ERROR_SUCCESS == RegEnumValue(hkey, i++, szClass, &cchClass, NULL, &dwType, NULL, NULL))
        {
            CLSID clsid;
            if (SUCCEEDED(SHCLSIDFromString(szClass, &clsid)))
            {
                IRunnableTask* ptask;
                if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC, IID_IRunnableTask, (void **) &ptask)))
                {
                    // Personally I think the start menu should have priority
                    // over itbar icon extraction, so set this priority list
                    // a tad lower than the default priority (which start menu is at)
                    _AddDesktopTask(ptask, ITSAT_DEFAULT_PRIORITY-1);
                    ptask->Release();
                }
                else
                {
                    TraceMsg(TF_WARNING, "startup background task %s not registered correctly!", szClass);
                }
            }
        }

        RegCloseKey(hkey);
    }

}


// create the desktop window and its shell view
DWORD_PTR DesktopWindowCreate(CDesktopBrowser ** ppBrowser)
{
    *ppBrowser = NULL;
    DWORD dwExStyle = WS_EX_TOOLWINDOW;

    OleInitialize(NULL);
    RegisterDesktopClass();

    _InitDesktopMetrics(0, NULL);

    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    //
    // NB This windows class is Progman and it's title is Program Manager. This makes
    // sure apps (like ATM) think that program is running and don't fail their install.
    //
    HWND hwnd = CreateWindowEx(dwExStyle, DESKTOPCLASS, PROGMAN,
        WS_POPUP | WS_CLIPCHILDREN,
        g_xVirtualScreen, g_yVirtualScreen,
        g_cxVirtualScreen, g_cyVirtualScreen,
        NULL, NULL, HINST_THISDLL, (void *)NULL);
    if (hwnd)
    {
        CDesktopBrowser *psb = (CDesktopBrowser*)GetWindowLongPtr(hwnd, 0);
        ASSERT(psb);

        if (!SHRestricted(REST_NODESKTOP))
        {
            // do this here to avoid painting the desktop, then repainting
            // when the tray appears and causes everything to move

            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }

        DoTimebomb(hwnd);

        // Cause Shell Service Objects to load, open.
        // This also causes the tray thread to resume other threads
        // it created during boot time, including the FS_NOTIFY thread
        // and the start menu building thread.  We do this here after
        // the desktop thread has initialized to speed up the boot process.
        // Otherwise the secondary threads created starve out the desktop
        // thread and cause extra paging, thus causing the desktop to display
        // much later.  --MikeSch
        ASSERT(psb->GetTrayWindow());

        psb->StartBackgroundShellTasks();
        
        PostMessage(psb->GetTrayWindow(), TM_HANDLEDELAYBOOTSTUFF, 0, 0);

        *ppBrowser = (CDesktopBrowser*)GetWindowLongPtr(hwnd, 0);
    }

    return (DWORD_PTR)hwnd;
}

STDAPI_(HANDLE) SHCreateDesktop(IDeskTray* pdtray)
{
    if (g_dwProfileCAP & 0x00000010)
        StartCAP();

    ASSERT(pdtray);
    ASSERT(g_pdtray==NULL);
    g_pdtray = pdtray;
    pdtray->AddRef();   // this is no-op, but we want to follow the COM rule.

    // put the desktop on the main thread (this is needed for win95 so that
    // the DDeExecuteHack() in user.exe works, it needs the desktop and the 
    // DDE window on the mainthread
    CDesktopBrowser *pBrowser;
    if (DesktopWindowCreate(&pBrowser))
    {
        if (g_dwProfileCAP & 0x00040000)
            StopCAP();
    
        // hack, cast the object to a handle (otherwise we have to export the class
        // declaration so that explorer.exe can use it)
        return (HANDLE) pBrowser;
    }
    return NULL;
}

// nash:49485 (IME focus) and nash:nobug (win95 compat)
// make sure keyboard input goes to the desktop.  this is
// a) win95 compat: where focus was on win95 and
// b) nash:49485: focus was in the empty taskband on login so
// the keys went into the bitbucket
//
// If some other process has stolen the foreground window,
// don't be rude and grab it back.

void FriendlySetForegroundWindow(HWND hwnd)
{
    HWND hwndOld = GetForegroundWindow();
    if (hwndOld)
    {
        DWORD dwProcessId;
        
        GetWindowThreadProcessId(hwndOld, &dwProcessId);
        if (dwProcessId == GetCurrentProcessId())
            hwndOld = NULL;
    }
    if (!hwndOld)
        SetForegroundWindow(hwnd);
}

STDAPI_(BOOL) SHDesktopMessageLoop(HANDLE hDesktop)
{
    CDesktopBrowser *pBrowser = (CDesktopBrowser *) hDesktop;
    if (pBrowser)
    {
        // We must AddRef the pBrowser because _MessageLoop() will
        // Release() it if another app initiated a system shutdown.
        // We will do our own Release() when we don't need the pointer
        // any more.

        pBrowser->AddRef();

        FriendlySetForegroundWindow(pBrowser->GetDesktopWindow());

        pBrowser->_MessageLoop();

        // In case someone posted us a WM_QUIT message, before terminating
        // the thread, make sure it is properly destroyed so that trident etc
        // gets properly freed up.
        HWND hwnd;
        if (hwnd = pBrowser->GetDesktopWindow())
        {
            DestroyWindow(hwnd);
        }
        pBrowser->Release();
        OleUninitialize();

    }
    return BOOLFROMPTR(pBrowser);
}

void EscapeAccelerators(LPTSTR psz)
{
    LPTSTR pszEscape;
    
    while (pszEscape = StrChr(psz, TEXT('&'))) {

        int iLen = lstrlen(pszEscape)+1;
        MoveMemory(pszEscape+1, pszEscape, iLen * SIZEOF(TCHAR));
        pszEscape[0] = TEXT('&');
        psz = pszEscape + 2;
    }
}


//
// Whenever we remove the toolbar from the desktop, we persist it.
//
HRESULT CDesktopBrowser::RemoveToolbar(IUnknown* punkSrc, DWORD dwRemoveFlags)
{
    HRESULT hres = E_FAIL;
    UINT itb = _pbsInner->_FindTBar(punkSrc);
    if (itb==(UINT)-1) {
        return E_INVALIDARG;
    }

    LPTOOLBARITEM ptbi = _pbsInner->_GetToolbarItem(itb);
    if (ptbi && ptbi->pwszItem) {
#ifdef UNICODE
        LPCWSTR szItem = ptbi->pwszItem;
#else
        TCHAR szItem[256];
        SHUnicodeToTChar(ptbi->pwszItem, szItem, ARRAYSIZE(szItem));
#endif
        
        if (dwRemoveFlags & STFRF_DELETECONFIGDATA) {
            
            DeleteDesktopViewStream(szItem);
            
        } else {
            IStream* pstm = GetDesktopViewStream(STGM_WRITE, szItem);
            if (pstm) {
                IPersistStreamInit* ppstm;
                HRESULT hresT = punkSrc->QueryInterface(IID_IPersistStreamInit, (void **)&ppstm);
                if (SUCCEEDED(hresT)) {
                    ppstm->Save(pstm, TRUE);
                    ppstm->Release();
                }
                pstm->Release();
            }
        }
    }

    hres = _pdwfInner->RemoveToolbar(punkSrc, dwRemoveFlags);
    _UpdateViewRectSize();
    return hres;
}

HRESULT CDesktopBrowser::GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder)
{
    BOOL bUseHmonitor = (GetNumberOfMonitors() > 1);
    return _pbsInner->_GetBorderDWHelper(punkSrc, lprectBorder, bUseHmonitor);    
}

HRESULT CDesktopBrowser::_ResizeNextBorder(UINT itb)
{
    _ResizeNextBorderHelper(itb, TRUE);
    return S_OK;
}

HRESULT CDesktopBrowser::GetMonitor(IUnknown* punkSrc, HMONITOR * phMon)
{
    ASSERT(phMon);
    *phMon = NULL;              // just in case

    UINT itb = _pbsInner->_FindTBar(punkSrc);
    if (itb==(UINT)-1) {
        return E_INVALIDARG;
    }

    LPTOOLBARITEM ptbi = _pbsInner->_GetToolbarItem(itb);
    if (ptbi) {
        *phMon = ptbi->hMon;
        return S_OK;
    } else {
        return E_FAIL;
    }
}

HRESULT CDesktopBrowser::RequestMonitor(IUnknown* punkSrc, HMONITOR * phMonitor)
{
    UINT itb = _pbsInner->_FindTBar(punkSrc);
    if (itb==(UINT)-1) {
        return E_INVALIDARG;
    }

    ASSERT(phMonitor);
    if (IsMonitorValid(*phMonitor))
        return S_OK;
    else
    {
        *phMonitor = GetPrimaryMonitor();
        return S_FALSE;
    }
}

HRESULT CDesktopBrowser::SetMonitor(IUnknown* punkSrc, HMONITOR hMonNew, HMONITOR * phMonOld)
{
    ASSERT(phMonOld);
    *phMonOld = NULL;           // just in case

    UINT itb = _pbsInner->_FindTBar(punkSrc);
    if (itb==(UINT)-1) {
        return E_INVALIDARG;
    }
    
    LPTOOLBARITEM ptbThis = _pbsInner->_GetToolbarItem(itb);
    if (ptbThis) {
        *phMonOld = ptbThis->hMon;
        ptbThis->hMon = hMonNew;
        return S_OK;
    } else {
        // BUGBUG -- browseui doesn't check the error code!
        return E_FAIL;
    }
}


///////////////////////////////////////////////////////////////////////
//
// CDesktopBrowser FORWARDERS to commonsb
//

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _psbInner-> _function _args ; }                                            

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IShellBrowser (same as IOleInPlaceFrame)
CALL_INNER_HRESULT(GetWindow, (HWND * lphwnd), (lphwnd));
CALL_INNER_HRESULT(ContextSensitiveHelp, (BOOL fEnterMode), (fEnterMode));
CALL_INNER_HRESULT(InsertMenusSB, (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths), (hmenuShared, lpMenuWidths));
CALL_INNER_HRESULT(SetMenuSB, (HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd), (hmenuShared, holemenu, hwnd));
CALL_INNER_HRESULT(RemoveMenusSB, (HMENU hmenuShared), (hmenuShared));
CALL_INNER_HRESULT(SetStatusTextSB, (LPCOLESTR lpszStatusText), (lpszStatusText));
CALL_INNER_HRESULT(EnableModelessSB, (BOOL fEnable), (fEnable));
CALL_INNER_HRESULT(TranslateAcceleratorSB, (LPMSG lpmsg, WORD wID), (lpmsg, wID) );
CALL_INNER_HRESULT(GetViewStateStream, (DWORD grfMode, LPSTREAM  *ppStrm), (grfMode, ppStrm) );
CALL_INNER_HRESULT(GetControlWindow, (UINT id, HWND * lphwnd), (id, lphwnd));
CALL_INNER_HRESULT(SendControlMsg, (UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret), (id, uMsg, wParam, lParam, pret));
CALL_INNER_HRESULT(QueryActiveShellView, (struct IShellView ** ppshv), (ppshv));
CALL_INNER_HRESULT(OnViewWindowActive, (struct IShellView * ppshv), (ppshv));
CALL_INNER_HRESULT(SetToolbarItems, (LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags), (lpButtons, nButtons, uFlags));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _pdwsInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IDockingWindowSite
    // TODO: move these up from basesb to commonsb - requires toolbars
CALL_INNER_HRESULT(RequestBorderSpaceDW, (IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths), (punkSrc, pborderwidths) );
CALL_INNER_HRESULT(SetBorderSpaceDW, (IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths), (punkSrc, pborderwidths) );

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }


// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _pdwfInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IDockingWindowFrame
CALL_INNER_HRESULT(AddToolbar, (IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwReserved), (punkSrc, pwszItem, dwReserved) );
CALL_INNER_HRESULT(FindToolbar, (LPCWSTR pwszItem, REFIID riid, void **ppvObj), (pwszItem, riid, ppvObj) );

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _piosInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IInputObjectSite
CALL_INNER_HRESULT(OnFocusChangeIS, (IUnknown* punkSrc, BOOL fSetFocus), (punkSrc, fSetFocus) );

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _pdtInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // *** IDropTarget ***
CALL_INNER_HRESULT(DragLeave, (void), ());

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CDesktopBrowser:: _function _arglist { return _pbsInner-> _function _args ; }                                            

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)
 

// *** IBrowserService2 specific methods ***
CALL_INNER_HRESULT(GetParentSite, (  IOleInPlaceSite** ppipsite), (  ppipsite) );
CALL_INNER_HRESULT(SetTitle, ( IShellView* psv, LPCWSTR pszName), ( psv, pszName) );
CALL_INNER_HRESULT(GetTitle, ( IShellView* psv, LPWSTR pszName, DWORD cchName), ( psv, pszName, cchName) );
CALL_INNER_HRESULT(GetOleObject, (  IOleObject** ppobjv), (  ppobjv) );

// think about this one.. I'm not sure we want to expose this -- Chee
// BUGBUG:: Yep soon we should have interface instead.
// My impression is that we won't document this whole interface???
CALL_INNER_HRESULT(GetTravelLog, ( ITravelLog** pptl), ( pptl) );

CALL_INNER_HRESULT(ShowControlWindow, ( UINT id, BOOL fShow), ( id, fShow) );
CALL_INNER_HRESULT(IsControlWindowShown, ( UINT id, BOOL *pfShown), ( id, pfShown) );
CALL_INNER_HRESULT(IEGetDisplayName, ( LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags), ( pidl, pwszName, uFlags) );
CALL_INNER_HRESULT(IEParseDisplayName, ( UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut), ( uiCP, pwszPath, ppidlOut) );
CALL_INNER_HRESULT(DisplayParseError, ( HRESULT hres, LPCWSTR pwszPath), ( hres, pwszPath) );
CALL_INNER_HRESULT(NavigateToPidl, ( LPCITEMIDLIST pidl, DWORD grfHLNF), ( pidl, grfHLNF) );

CALL_INNER_HRESULT(SetNavigateState, ( BNSTATE bnstate), ( bnstate) );
CALL_INNER_HRESULT(GetNavigateState,  ( BNSTATE *pbnstate), ( pbnstate) );

CALL_INNER_HRESULT(NotifyRedirect,  (  IShellView* psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse), (  psv, pidl, pfDidBrowse) );
CALL_INNER_HRESULT(UpdateWindowList,  (), () );

CALL_INNER_HRESULT(UpdateBackForwardState,  (), () );

CALL_INNER_HRESULT(SetFlags, ( DWORD dwFlags, DWORD dwFlagMask), ( dwFlags, dwFlagMask) );
CALL_INNER_HRESULT(GetFlags, ( DWORD *pdwFlags), ( pdwFlags) );

// Tells if it can navigate now or not.
CALL_INNER_HRESULT(CanNavigateNow,  (), () );

CALL_INNER_HRESULT(GetPidl,  ( LPITEMIDLIST *ppidl), ( ppidl) );
CALL_INNER_HRESULT(SetReferrer,  ( LPITEMIDLIST pidl), ( pidl) );
CALL_INNER(DWORD,  GetBrowserIndex ,(), () );
CALL_INNER_HRESULT(GetBrowserByIndex, ( DWORD dwID, IUnknown **ppunk), ( dwID, ppunk) );
CALL_INNER_HRESULT(GetHistoryObject, ( IOleObject **ppole, IStream **pstm, IBindCtx **ppbc), ( ppole, pstm, ppbc) );
CALL_INNER_HRESULT(SetHistoryObject, ( IOleObject *pole, BOOL fIsLocalAnchor), ( pole, fIsLocalAnchor) );

CALL_INNER_HRESULT(CacheOLEServer, ( IOleObject *pole), ( pole) );

CALL_INNER_HRESULT(GetSetCodePage, ( VARIANT* pvarIn, VARIANT* pvarOut), ( pvarIn, pvarOut) );
CALL_INNER_HRESULT(OnHttpEquiv, ( IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut), ( psv, fDone, pvarargIn, pvarargOut) );

CALL_INNER_HRESULT(GetPalette, (  HPALETTE * hpal ), (  hpal ) );

CALL_INNER_HRESULT(OnSetFocus, (), () );
CALL_INNER_HRESULT(OnFrameWindowActivateBS, (BOOL fActive), (fActive) );

CALL_INNER_HRESULT(RegisterWindow, ( BOOL fUnregister, int swc), ( fUnregister, swc) );
CALL_INNER_HRESULT(GetBaseBrowserData,( LPCBASEBROWSERDATA* ppbd ), ( ppbd ));
CALL_INNER(LPBASEBROWSERDATA, PutBaseBrowserData,(), ());
CALL_INNER_HRESULT(CreateViewWindow, (IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd), (psvNew, psvOld, prcView, phwnd));;
CALL_INNER_HRESULT(SetTopBrowser, (), ());
CALL_INNER_HRESULT(InitializeDownloadManager, (), ());
CALL_INNER_HRESULT(InitializeTransitionSite, (), ());
CALL_INNER_HRESULT(Offline, (int iCmd), (iCmd));
CALL_INNER_HRESULT(AllowViewResize, (BOOL f), (f));
CALL_INNER_HRESULT(SetActivateState, (UINT u), (u));
CALL_INNER_HRESULT(UpdateSecureLockIcon, (int eSecureLock), (eSecureLock));
CALL_INNER_HRESULT(CreateBrowserPropSheetExt, (REFIID riid, void **ppvObj), (riid, ppvObj));

CALL_INNER_HRESULT(GetViewWindow,( HWND * phwnd ), ( phwnd ));
CALL_INNER_HRESULT(InitializeTravelLog,( ITravelLog* ptl, DWORD dw ), ( ptl, dw ));

CALL_INNER_HRESULT(_UIActivateView, (UINT uState), (uState) );

CALL_INNER_HRESULT(_ResizeView,(), ());

CALL_INNER_HRESULT(_ExecChildren, (IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut),
        (punkBar, fBroadcast, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut) );
CALL_INNER_HRESULT(_SendChildren,
        (HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam),
        (hwndBar, fBroadcast, uMsg, wParam, lParam) );

CALL_INNER_HRESULT(_OnFocusChange, (UINT itb), (itb) );
CALL_INNER_HRESULT(v_ShowHideChildWindows, (BOOL fChildOnly), (fChildOnly) );

CALL_INNER_HRESULT(_GetViewBorderRect, (RECT* prc), (prc) );


    // BEGIN REVIEW:  review names and need of each.  
    // 
    // this first set could be basebrowser only members.  no one overrides
CALL_INNER_HRESULT(_CancelPendingNavigationAsync, (), () );
CALL_INNER_HRESULT(_MaySaveChanges, (), () ); 
CALL_INNER_HRESULT(_PauseOrResumeView, ( BOOL fPaused), ( fPaused) );
CALL_INNER_HRESULT(_DisableModeless, (), () );
    
    // rethink these... are all of these necessary?
CALL_INNER_HRESULT(_NavigateToPidl, ( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags), ( pidl, grfHLNF, dwFlags));
CALL_INNER_HRESULT(_TryShell2Rename, ( IShellView* psv, LPCITEMIDLIST pidlNew), ( psv, pidlNew));
CALL_INNER_HRESULT(_SwitchActivationNow, () , ( ));
CALL_INNER_HRESULT(_CancelPendingView, (), () );

    //END REVIEW:

CALL_INNER(UINT, _get_itbLastFocus, (), () );
CALL_INNER_HRESULT(_put_itbLastFocus, (UINT itbLastFocus), (itbLastFocus) );

CALL_INNER_HRESULT(_ResizeNextBorderHelper, (UINT itb, BOOL bUseHmonitor), (itb, bUseHmonitor));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }
