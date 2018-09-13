#ifndef _SHBROWS2_H
#define _SHBROWS2_H

#include "iface.h"
#include "commonsb.h"
#include "browmenu.h"
#include "bsmenu.h"
#include "fldset.h"
#include <iethread.h>

#define MAX_SB_TEXT_LEN 256

class CIEFrameAuto;


// Define structure to be used at head of state stream that is
// not dependent on 16 or 32 bits...
typedef struct _CABSHOLD       // Cabinet Stream header
{
    DWORD   dwSize;       // Offset to where the View streamed additional info

    // First stuff from the window placement
    DWORD  flags;
    DWORD  showCmd;
    POINTL ptMinPosition;
    POINTL ptMaxPosition;
    RECTL  rcNormalPosition;

    // Stuff from Folder Settings;
    DWORD   ViewMode;       // View mode (FOLDERVIEWMODE values)
    DWORD   fFlags;         // View options (FOLDERFLAGS bits)
    DWORD   TreeSplit;      // Position of split in pixels (BUGBUG?)

    // Hot Key
    DWORD   dwHotkey;        // Hotkey

    WINVIEW wv;
} CABSHOLD;

typedef struct _CABSH       // Cabinet Stream header
{
    DWORD   dwSize;       // Offset to where the View streamed additional info

    // First stuff from the window placement
    DWORD  flags;
    DWORD  showCmd;
    POINTL ptMinPosition;
    POINTL ptMaxPosition;
    RECTL  rcNormalPosition;

    // Stuff from Folder Settings;
    DWORD   ViewMode;       // View mode (FOLDERVIEWMODE values)
    DWORD   fFlags;         // View options (FOLDERFLAGS bits)
    DWORD   TreeSplit;      // Position of split in pixels (BUGBUG?)

    // Hot Key
    DWORD   dwHotkey;        // Hotkey

    WINVIEW wv;

    DWORD   fMask;          // Flags specifying which fields are valid
    SHELLVIEWID vid;        // extended view id
    DWORD   dwVersionId;    // CABSH_VER below
    DWORD   dwRevCount;     // rev count of default settings when the folder was saved to the stream
} CABSH;

#define CABSHM_VIEWID  0x00000001
#define CABSHM_VERSION 0x00000002
#define CABSHM_REVCOUNT 0x00000004

#define CABSH_VER 1 // change this version whenever we want to change defaults
#define CABSH_WIN95_VER 0 // this was the pre-ie4 version number

class CTheater;

#define CSBSUPERCLASS CCommonBrowser

#define CSHELLBROWSER CShellBrowser2
class CShellBrowser2 :
    public CSBSUPERCLASS
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CSBSUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void) { return CSBSUPERCLASS::Release(); };

    // IShellBrowser (same as IOleInPlaceFrame)
    virtual STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    virtual STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved, HWND hwnd);
    virtual STDMETHODIMP RemoveMenusSB(HMENU hmenuShared);
    virtual STDMETHODIMP EnableModelessSB(BOOL fEnable);
    virtual STDMETHODIMP GetViewStateStream(DWORD grfMode, IStream **ppstm);
    virtual STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd);
    virtual STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, LRESULT *pret);
    virtual STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);
    virtual STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags);
    virtual STDMETHODIMP OnViewWindowActive(IShellView * psv);

    // IServiceProvider
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObj);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IBrowserService
    virtual STDMETHODIMP ShowControlWindow(UINT id, BOOL fShow);
    virtual STDMETHODIMP IsControlWindowShown(UINT id, BOOL *pfShown);
    virtual STDMETHODIMP SetTitle(IShellView* psv, LPCWSTR pszName);
    virtual STDMETHODIMP UpdateWindowList(void);
    virtual STDMETHODIMP SetFlags(DWORD dwFlags, DWORD dwFlagMask);
    virtual STDMETHODIMP GetFlags(DWORD *pdwFlags);
    virtual STDMETHODIMP RegisterWindow(BOOL fUnregister, int swc);
    virtual STDMETHODIMP Offline(int iCmd);
    virtual STDMETHODIMP SetReferrer ( LPITEMIDLIST pidl) ;
    virtual STDMETHODIMP_(LRESULT) WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP OnCreate(LPCREATESTRUCT pcs);
    virtual STDMETHODIMP_(LRESULT) OnCommand(WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP OnDestroy();
    virtual STDMETHODIMP_(LRESULT) OnNotify(NMHDR * pnm);
    virtual STDMETHODIMP OnSetFocus();
    virtual STDMETHODIMP ReleaseShellView();
    virtual STDMETHODIMP ActivatePendingView();
    virtual STDMETHODIMP CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd);
    virtual STDMETHODIMP UpdateBackForwardState () ;
    virtual STDMETHODIMP CreateBrowserPropSheetExt(REFIID, LPVOID *);
    virtual STDMETHODIMP SetAsDefFolderSettings();

    virtual STDMETHODIMP _TryShell2Rename( IShellView* psv, LPCITEMIDLIST pidlNew);
    virtual STDMETHODIMP _NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    virtual STDMETHODIMP v_MayTranslateAccelerator( MSG* pmsg);
    virtual STDMETHODIMP _SwitchActivationNow( );
    virtual STDMETHODIMP _Initialize(HWND hwnd, IUnknown *pauto);
    virtual STDMETHODIMP _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);

    virtual STDMETHODIMP_(LPSTREAM) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName); 
    
    // IDockingWindowSite
    virtual STDMETHODIMP SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);

    // IInputSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);
    virtual STDMETHODIMP ShowToolbar(IUnknown* punkSrc, BOOL fShow);

    BOOL IsCShellBrowser2() { return !_fSubclassed; };

protected:
    CShellBrowser2();
    virtual ~CShellBrowser2();

    friend HRESULT CShellBrowser2_CreateInstance(HWND hwnd, void ** ppsb);

    // topmost CBaseBrowser2 in a frameset (IE3/AOL/CIS/VB)
    virtual STDMETHODIMP v_CheckZoneCrossing(LPCITEMIDLIST pidl){return _CheckZoneCrossing(pidl);};
    virtual HRESULT     _CheckZoneCrossing(LPCITEMIDLIST pidl);

    // CShellBrowser2 virtuals
    virtual LRESULT     v_ForwardMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual DWORD       v_ShowControl(UINT iControl, int iCmd);
    virtual STDMETHODIMP        v_ShowHideChildWindows(BOOL fChildOnly = FALSE);
    virtual void        v_InitMembers();
    virtual void        v_SetIcon();
    virtual void        v_GetAppTitleTemplate(LPTSTR pszBuffer, LPTSTR pszTitle);
    virtual LRESULT     v_OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu);
    virtual void        v_HandleFileSysChange(LONG lEvent, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
    virtual DWORD       v_RestartFlags();
    virtual void        v_FillCabStateHeader(CABSH* pcabsh, FOLDERSETTINGS *pfs);
    virtual void        v_ParentFolder();
    virtual BOOL        v_OnContextMenu(WPARAM wParam, LPARAM lParam);
    virtual BOOL        v_IsIEModeBrowser();    
    virtual BOOL        _CreateToolbar();
    virtual void        _PositionViewWindow(HWND hwnd, LPRECT prc);
    virtual IStream *   _GetITBarStream(BOOL fWebBrowser, DWORD grfMode);

    //ViewStateStream related
    virtual void        v_GetDefaultSettings(IETHREADPARAM *pcv);

    void         _FillIEThreadParam(LPCITEMIDLIST pidl, IETHREADPARAM *piei);
    virtual void _UpdateFolderSettings(LPCITEMIDLIST pidl);

    BOOL        _ShouldAllowNavigateParent();
    BOOL        _ShouldSaveWindowPlacement();
    HRESULT     _GetBSForBar(LPCWSTR pwszItem, IBandSite **ppbs);
    void        _UpdateBackForwardStateNow();
    void        _ExecAllBands(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    void        _HideToolbar(IUnknown *punk);
    void        _BandClosed(IUnknown *punk, DWORD dwBandID);
    void        _ShowHideProgress();
    void        _CloseAllParents();
    void        _OnConfirmedClose();
    void        _OnClose(BOOL fPushed);
    void        _AfterWindowCreated(IETHREADPARAM *piei);
    BOOL        _ValidTargetPidl(LPCITEMIDLIST pidl, BOOL *pfTranslateRoot);
    LPITEMIDLIST _TranslateRoot(LPCITEMIDLIST pidl);
    BOOL        _GetVID(SHELLVIEWID *pvid);
    void         _UpdateRegFlags();

    void        _SaveState();
    HRESULT     _FillCabinetStateHeader(IStream *pstm, CABSH *cabsh);
    BOOL        _ReadSettingsFromStream(IStream *pstm, IETHREADPARAM *piei);
    void        _LoadBrowserWindowSettings(IETHREADPARAM *piei, LPCITEMIDLIST pidl);
    IDockingWindow* _GetITBar() { return _GetToolbarItem(ITB_ITBAR)->ptbar; }
    HMENU       _GetMenuFromID(UINT uID);
    void        _UpdateChildWindowSize(void);
    void        _GetAppTitle(LPTSTR pszBuffer, DWORD dwSize);
    IMenuBand*  _GetMenuBand(BOOL bDestroy);
    BOOL        _TranslateMenuMessage(HWND hwnd, UINT uMsg, WPARAM * pwParam, LPARAM * plParam, LRESULT * plRet);
    void        _ReloadTitle();
    void        _ReloadStatusbarIcon();

    void        _OnTimer(UINT_PTR idTimer);
    LRESULT     _ToolbarOnNotify(LPNMHDR pnm);
    LRESULT     _OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu);
    BOOL        _OnSysMenuClick(BOOL bLeftButton, WPARAM wParam, LPARAM lParam);
    LRESULT     _OnMenuSelect(WPARAM wParam, LPARAM lParam, UINT uHelpFlags);
    void        _SetMenuHelp(HMENU hmenu, UINT wID, LPCTSTR pszHelp);
    void        _SetBrowserBarMenuHelp(HMENU hmenu, UINT wID);
    void        _SetExternalBandMenuHelp(HMENU hmenu, UINT wID);
    void        _AddMailNewsMenu(HMENU hmenu);
    void        _SetTitle( LPCWSTR pwszName);
    HRESULT     _CreateFrameServices(void);

    void        _FavoriteOnCommand(HMENU hmenu, UINT idCmd);

    void        _CommonHandleFileSysChange(LONG lEvent, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
    void        _FSChangeCheckClose(LPCITEMIDLIST pidl, BOOL fDisconnect);
    void        _OnFSNotify(WPARAM, LPARAM);
//    replaced by CBaseBrowser2::NavigateToTLItem()
//    void        _RecentOnCommand(UINT idCmd);
    LRESULT     _FwdTBNotify(LPTBNOTIFY ptbn);
    void        _ToolTipFromCmd(LPTOOLTIPTEXT pnm);
    void        _AddFolderOptionsPage(PROPSHEETHEADER *ppsh);
    void        _AddAdvancedOptionsPage(PROPSHEETHEADER *ppsh);
    
    void        _IncrNetSessionCount();
    void        _DecrNetSessionCount();

    friend LRESULT CALLBACK IEFrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend void CALLBACK BrowserThreadProc(IETHREADPARAM* piei);
    
    void        _ExecFileContext(UINT idCmd);
    void        _EnableFileContext(HMENU hmenuPopup);
    LPITEMIDLIST _GetSubscriptionPidl();
    HRESULT     _GetBrowserBar(int idBar, BOOL fShow, IBandSite** ppbs, const CLSID* pclsid);

    BANDCLASSINFO* _BandClassInfoFromCmdID(UINT idCmd);

    void        _AddBrowserBarMenuItems(HMENU hmenu);
    HMENU       _GetBrowserBarMenu();
    int         _IdBarFromCmdID(UINT idCmd);
    int         _eOnOffNotMunge(int eOnOffNot, UINT idCmd, UINT idBar);
    void        _SetBrowserBarState(UINT idCmd, const CLSID *pclsid, int eOnOffNot, LPCITEMIDLIST pidl = NULL);
    const CLSID * _ShowHideBrowserBar(int idBar, const CLSID *pclsid, int eOnOff, LPCITEMIDLIST pidl);
    const CLSID * _InfoShowClsid(int idBar, const CLSID *pclsid, int eOnOff, LPCITEMIDLIST pidl = NULL);
    const CLSID * _InfoIdmToCLSID(UINT idCmd);
    const CATID * _InfoIdmToCATID(UINT idCmd);
    UINT        _InfoCLSIDToIdm(const CLSID *pguid);
    BOOL        _IsSameToolbar(LPWSTR wszBarName, IUnknown *punkBar);
    HRESULT     _AddInfoBands(IBandSite *punkBandSite);
    HRESULT     _AddSearchBand(IBandSite *punkBandSite);
    HRESULT     DoCopyData(COPYDATASTRUCT *pCDS);
    HRESULT     DoRemoteNavigate();
    void        _OrganizeFavorites();    
    BOOL        _ShouldForwardMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void        _TheaterMode(BOOL fShow, BOOL fRestorePrevious);
#ifdef DEBUG
    void        _DumpMenus(LPCTSTR pszMsg, BOOL bMsg);
#endif
    BOOL        _LoadBrowserHelperObjects(void);

    void        _CreateBrowserBarClose(HWND hwndParent);
    void        _SetTheaterBrowserBar();
    void        _SetMenu(HMENU hmenu);
    HWND        _GetCaptionWindow();
    LRESULT     _ThunkTTNotify(LPTOOLTIPTEXTA pnmTTTA);
    BOOL        _PrepareInternetToolbar(IETHREADPARAM* piei);
    HRESULT     _SaveITbarLayout(void);
    static LRESULT CALLBACK DummyTBWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    DWORD       _GetTempZone();
    void        _UpdateZonesPane(VARIANT *pvar);
    void        _DisplayFavoriteStatus(LPCITEMIDLIST pidl);
    void        _SendCurrentPage(DWORD dwSendAs);
    void        _OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam); 
    HRESULT     _FreshenComponentCategoriesCache( BOOL bForceUpdate ) ;
    void        _QueryHKCRChanged() ;    

    void        _PruneGoSubmenu(HMENU hmenu);
    HMENU       _MenuTemplate(int id, BOOL fShell);

    void        _MungeGoMyComputer(HMENU hmenuPopup);
    void        _OnGoMenuPopup(HMENU hmenuPopup);
    void        _OnViewMenuPopup(HMENU hmenuPopup);
    void        _OnToolsMenuPopup(HMENU hmenuPopup);
    void        _OnFileMenuPopup(HMENU hmenuPopup);
    void        _OnSearchMenuPopup(HMENU hmenuPopup);
    void        _OnHelpMenuPopup(HMENU hmenuPopup);
    void        _OnMailMenuPopup(HMENU hmenuPopup);
    void        _OnEditMenuPopup(HMENU hmenuPopup);
    void        _OnFindMenuPopup(HMENU hmenuPopup);
    void        _OnExplorerBarMenuPopup(HMENU hmenuPopup);
    void        _InsertTravelLogItems(HMENU hmenu, int nPos);

    // methods and members for monitoring the current status bar code page and font
    HRESULT _GetCodePage(UINT * puiCodePage, DWORD dwCharSet);

    int         _GetIconIndex(void);
    HRESULT     _QIExplorerBand(REFIID riid, void ** ppvObj);
    virtual BOOL _CanHandleAcceleratorNow(void) {return _fActivated;}

    bool        _IsExplorerBandVisible (void);

#ifdef UNIX
    BOOL        _HandleActivation( WPARAM wParam );
#endif

    // NT5 specific message handling:
    BOOL        _OnAppCommand(WPARAM wParam, LPARAM lParam);
    LPARAM      _WndProcBSNT5(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    

    // Menus: see dochost.h for an explanation from the dochost perspective.
    //        BUGBUG (scotth): difference b/t _hmenuTemplate and _hmenuFull?
    //        

    HMENU       _hmenuTemplate;
    HMENU       _hmenuFull;
    HMENU       _hmenuBrowser;          // Provided from dochost, used for _menulist
    HMENU       _hmenuCur;              // Used to keep track.  Needed when in Kiosk mode...
    HMENU       _hmenuPreMerged;
#ifdef DEBUG    
    HMENU       _hmenuHelp;             // Our help menu
#endif

    HICON       _hZoneIcon;

    CMenuList   _menulist;              // Menu list for dispatching

    IContextMenu2 *_pcmNsc;             // iff we've popped up File Menu w/ NSC context item
    IOleCommandTarget *_poctNsc;       // iff we have visible name space control band
    int         _idMenuBand;
    HWND        _hwndStatus;
    HWND        _hwndProgress;
    int         _iTBOffset[3];
    
    CBandSiteMenu *_pbsmInfo;
    // BUGBUG could use relative IDM and make this a 'char' (or smaller)
    UINT        _idmInfo;               // selected View.BrowserBar submenu

    IMenuBand* _pmb;                   // Use _GetMenuBand to access this

    LPITEMIDLIST _pidlReferrer;

    WINDOWPLACEMENT _wndpl;             // Used to go into and out of full screen mode.
    CLSID           _clsidThis;         // browser class ID
    
    BITBOOL     _fStatusBar : 1;
    BITBOOL     _fAutomation : 1;       // was it launched from automation?
    BITBOOL     _fShowMenu : 1;         // should we show menus?
    BITBOOL     _fForwardMenu : 1;      // should we be forwarding menu messages?
    BITBOOL     _fNoLocalFileWarning: 1; // Do we directly execute local files without warning?
    BITBOOL     _fDispatchMenuMsgs:1;   // TRUE: forward menu messages as appropriate
    BITBOOL     _fUISetByAutomation: 1; // has ui (things that go in view options) been set by automation?
    BITBOOL     _fDontSaveViewOptions:1;   // Set if we should not save the view options on exit
    BITBOOL     _fSBWSaved :1;          // single browser window saved yet?
    BITBOOL     _fInternetStart :1;     // did we start on a net site?
    BITBOOL     _fWin95ViewState:1;
    BITBOOL     _fSubclassed :1;        // are we subclassed?
    BITBOOL     _fVisitedNet :1;        // did we ever hit the net in this session?
    BITBOOL     _fUnicode :1;           // is our view window unicode?
    BITBOOL     _fKioskMode : 1;        // Are we in full screen kiosk mode?  (not the same as fullscreen/theater mode)
    BITBOOL     _fTitleSet :1;          // has our content changed our title?
    BITBOOL     _fMarshalledDispatch:1; // have we marshalled dispatch?
    BITBOOL     _fShowBrowserBar :1;       // 1:BrowserBar is visible
    BITBOOL     _fMinimized :1;         // Minimized or not. 
    BITBOOL     _fReceivedDestroy: 1;   // Received WM_DESTROY message
    BITBOOL     _fDisallowSizing: 1;
    BITBOOL     _fShowNetworkButtons:1; // Based off the registry, we display the network connect button 
    BITBOOL     _fIgnoreNextMenuDeselect:1; // TRUE: Ignore the next deselection WM_MENUSELECT (hack)
    
    BITBOOL     _fActivated:1; // TRUE: the frame is set active, FALSE: the frame is inactive
    BITBOOL     _fClosingWindow:1; // TRUE: while closing, to avoid double-save and fault

    BITBOOL     _fDidRegisterWindow:1;  // called RegisterWindow() which registers the window with the winlist
#ifdef UNIX
    BITBOOL     _fSetAddressBarFocus:1;   
#endif
#ifdef NO_MARSHALLING
    BITBOOL     _fDelayedClose:1;  // Used to mark windows for delay close.
    BITBOOL     _fOnIEThread:1;    // used to flag if we are not running on the main thread
#endif
    BITBOOL     _fUpdateBackForwardPosted:1; // have we posted to update the back/forward state?
    BITBOOL     _fValidComCatCache ;         // Have we ensured that the component cache is valid

    BITBOOL     _fShowFortezza : 1;          // status of the Fortezza menu
    BITBOOL     _fRunningInIexploreExe:1;    // are we running in iexplore.exe
    BITBOOL     _fShowSynchronize : 1;       // should we show Tools|Synchronize?
    BITBOOL     _fNilViewStream : 1;         // CabView settings were loaded for unknown pidl.

    BITBOOL     _fAppendIEToCaptionBar : 1;  // Does the NSE want Append " - IE" to the captionbar display name?  See IBrowserFrameOptions
    BITBOOL     _fAddDialUpRef : 1;          // Does the NSE want a ref on the modem connection while browsed to this folder?  See IBrowserFrameOptions
    BITBOOL     _fUseIEToolbar : 1;          // Does the NSE want to use the IE style toolbar?  See IBrowserFrameOptions
    BITBOOL     _fEnableOfflineFeature : 1;  // Does the NSE want to enable the IE Offline feature?  See IBrowserFrameOptions
    BITBOOL     _fUseIEPersistence : 1;      // Does the NSE want to use IE type persistence?  Window pos, size, icon layout. See IBrowserFrameOptions

    UINT        _idmComm;               // selected View.BrowserBar submenu
    UINT        _iCommOffset;           // Start position in dynamically loaded comp categories

    ULONG _uFSNotify;

    int     _iSynchronizePos;
    TCHAR   *_pszSynchronizeText;       // What is Tools|Synchronize in your language?

    // Window list related (also _fMarshalledDispatch above)
    IShellWindows*   _psw;
    int         _swcRegistered;         // the SWC_* that this window is in the winlist as
    
    // Item ContextMenu 
    IContextMenu3 * _pcm;
    long  _dwRegisterWinList;  // Have we registered with the windows list

    HWND            _hwndDummyTB;
    IExplorerToolbar* _pxtb;
    int             _nTBTextRows; 

    // For the two members below, see notes above CShellBrowser2::_SwitchActivationNow
    TBBUTTON *  _lpButtons;
    int         _nButtons;

    TBBUTTON *  _lpPendingButtons;
    int         _nButtonsPending;

    IContextMenu3*   _pcmSearch;// new style
    IContextMenu*    _pcmFind;  // old style find context menu
        
    CTheater *_ptheater;

    LPITEMIDLIST    _pidlLastHist;
    LPITEMIDLIST    _pidlMenuSelect;

    UINT            _uiZonePane;
    IInternetSecurityManager * _pism;
    HMENU                      _hfm;

    DWORD       _dwRevCount;     // rev count of the global window settings
    UINT  _nMBIgnoreNextDeselect;
    IUnknown *_punkMsgLoop;

#ifdef NO_MARSHALLING
    BOOL  _fReallyClosed;
#endif

private:
    HRESULT _SetBrowserFrameOptions(LPCITEMIDLIST pidl);
};

BOOL FindBrowserWindow(void);

//
// Useful functions.
//

STDAPI_(BOOL) AddToRestartList(UINT flags, LPCITEMIDLIST pidl);
STDAPI_(BOOL) OpenFolderWindow(IETHREADPARAM* pieiIn);
void _CreateSavedWindows(void);
IDeskBand* _FindBandByClsidBS(IBandSite *pbs, const CLSID *pclsid);
HMENU _MenuTemplate(int id, BOOL bNukeTools);

#define INFOBAR_TBNAME  L"BrowserBar"
#ifndef UNIX
#define INFOBAR_WIDTH   204
#else
#define INFOBAR_WIDTH   277
#endif
#define COMMBAR_TBNAME  L"CommBar"
#define COMMBAR_HEIGHT  100

// Constants for different Browser/Info BARs
#define IDBAR_INFO         1
#define IDBAR_COMM         2
#define IDBAR_VERTICAL     IDBAR_INFO
#define IDBAR_HORIZONTAL   IDBAR_COMM
#define IDBAR_INVALID      42

#endif  // _SHBROWS2_H
