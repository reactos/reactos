/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "precomp.h"

#include <shellapi.h>
#include <htiframe.h>
#include <strsafe.h>

extern HRESULT IUnknown_ShowDW(IUnknown * punk, BOOL fShow);

#include "newatlinterfaces.h"

/*
TODO:
  **Provide implementation of new and delete that use LocalAlloc
  **Persist history for shell view isn't working correctly, possibly because of the mismatch between traveling and updating the travel log. The
        view doesn't restore the selection correctly.
  **Build explorer.exe, browseui.dll, comctl32.dll, shdocvw.dll, shell32.dll, shlwapi.dll into a directory and run them for testing...
  **Add brand band bitmaps to shell32.dll
  **If Go button on address bar is clicked, each time a new duplicate entry is added to travel log
****The current entry is updated in travel log before doing the travel, which means when traveling back the update of the
        current state overwrites the wrong entry's contents. This needs to be changed.
****Fix close of browser window to release all objects
****Given only a GUID in ShowBrowserBar, what is the correct way to determine if the bar is vertical or horizontal?
  **When a new bar is added to base bar site, how is base bar told so it can resize?
  **Does the base bar site have a classid?
  **What should refresh command send to views to make them refresh?
  **When new bar is created, what status notifications need to be fired?
  **How does keyboard filtering dispatch?
  **For deferred persist history load, how does the view connect up and get the state?
    How does context menu send open, cut, rename commands to its site (the shell view)?
  **Fix browser to implement IProfferService and hold onto brand band correctly - this will allow animations.

  **Route View->Toolbars commands to internet toolbar
  **Handle travel log items in View->Go
  **Fix ShowBrowserBar to pass correct size on when bar is shown
****Fix SetBorderSpaceDW to cascade resize to subsequent bars
****Make ShowToolbar check if bar is already created before creating it again
****Shell should fill in the list of explorer bars in the View submenus
  **Add folder menu in the file menu
  **Fix CShellBrowser::GetBorderDW to compute available size correctly
  **When a new bar is shown, re-fire the navigate event. This makes the explorer band select the correct folder
  **Implement support for refresh. Forward refresh to explorer bar (refresh on toolbar and in menu is dispatched different)
    Make folders toolbar item update state appropriately
    Read list of bands from registry on launch
    Read list of bars from registry on launch
    If the folders or search bars don't exist, disable the toolbar buttons
    If the favorites or history bars don't exist, disable the toolbar butons
    Fix Apply to all Folders in Folder Options
    Implement close command
    Add explorer band context menu to file menu
    Add code to allow restore of internet toolbar from registry
    Fix code that calls FireNavigateComplete to pass the correct new path

    What are the other command ids for QueryStatus/FireCommandStateChange?

    Add handler for cabinet settings change
    Add handler for system metrics change (renegotiate border space?)
    Add handler for theme change and forward to contained windows

    When folders are shown, the status bar text should change
    Add code to save/restore shell view settings
    Implement tabbing between frames
    Fix handling of focus everywhere
    Most keyboard shortcuts don't work, such as F2 for rename, F5 for refresh (see list in "explorer keyboard shortcuts")

    The status bar doesn't show help text for items owned by frame during menu tracking
    Stub out frame command handlers
    "Arrange icons by" group is not checked properly

    When folders are hidden, icon is the same as the current shell object being displayed. When folders are shown,
        the icon is always an open folder with magnifying glass
    Fix bars to calculate height correctly
    Hookup policies for everything...
    Investigate toolbar message WM_USER+93
    Investigate toolbar message WM_USER+100 (Adds extra padding between parts of buttons with BTNS_DROPDOWN | BTNS_SHOWTEXT style

    Vertical Explorer Bar		CATID_InfoBand
    Horizontal Explorer Bar		CATID_CommBand
    Desk Band					CATID_DeskBand

    cache of bars
    HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Discardable\PostSetup\Component Categories\{00021493-0000-0000-C000-000000000046}\Enum
    HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Discardable\PostSetup\Component Categories\{00021494-0000-0000-C000-000000000046}\Enum

    create key here with CLSID of bar to register tool band
    HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Explorer\Toolbar

*/

#ifndef __GNUC__
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")
#endif // __GNUC__

static const unsigned int                   folderOptionsPageCountMax = 20;
static const long                           BTP_DONT_UPDATE_HISTORY = 0;
static const long                           BTP_UPDATE_CUR_HISTORY = 1;
static const long                           BTP_UPDATE_NEXT_HISTORY = 2;
static const long                           BTP_ACTIVATE_NOFOCUS = 0x04;

BOOL                                        createNewStuff = false;


// this class is private to browseui.dll and is not registered externally?
//DEFINE_GUID(CLSID_ShellFldSetExt, 0x6D5313C0, 0x8C62, 0x11D1, 0xB2, 0xCD, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);

void DeleteMenuItems(HMENU theMenu, unsigned int firstIDToDelete, unsigned int lastIDToDelete)
{
    MENUITEMINFO                            menuItemInfo;
    int                                     menuItemCount;
    int                                     curIndex;

    menuItemCount = GetMenuItemCount(theMenu);
    curIndex = 0;
    while (curIndex < menuItemCount)
    {
        menuItemInfo.cbSize = sizeof(menuItemInfo);
        menuItemInfo.fMask = MIIM_ID;
        if (GetMenuItemInfo(theMenu, curIndex, TRUE, &menuItemInfo) &&
            menuItemInfo.wID >= firstIDToDelete && menuItemInfo.wID <= lastIDToDelete)
        {
            DeleteMenu(theMenu, curIndex, MF_BYPOSITION);
            menuItemCount--;
        }
        else
            curIndex++;
    }
}

HRESULT WINAPI SHBindToFolder(LPCITEMIDLIST path, IShellFolder **newFolder)
{
    CComPtr<IShellFolder>                   desktop;

    HRESULT hr = ::SHGetDesktopFolder(&desktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_FAIL;
    if (path == NULL || path->mkid.cb == 0)
    {
        *newFolder = desktop;
        desktop.p->AddRef ();
        return S_OK;
    }
    return desktop->BindToObject (path, NULL, IID_PPV_ARG(IShellFolder, newFolder));
}

static const TCHAR szCabinetWndClass[] = TEXT("CabinetWClass");
//static const TCHAR szExploreWndClass[] = TEXT("ExploreWClass");

class CDockManager;
class CShellBrowser;

class CToolbarProxy :
    public CWindowImpl<CToolbarProxy, CWindow, CControlWinTraits>
{
private:
    CComPtr<IExplorerToolbar>               fExplorerToolbar;
public:
    void Initialize(HWND parent, IUnknown *explorerToolbar);
    void Destroy();
private:

    // message handlers
    LRESULT OnAddBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BEGIN_MSG_MAP(CToolbarProxy)
        MESSAGE_HANDLER(TB_ADDBITMAP, OnAddBitmap)
        MESSAGE_RANGE_HANDLER(WM_USER, 0x7fff, OnForwardMessage)
    END_MSG_MAP()
};

void CToolbarProxy::Initialize(HWND parent, IUnknown *explorerToolbar)
{
    HWND                                    myWindow;
    HRESULT                                 hResult;

    myWindow = SHCreateWorkerWindowW(0, parent, 0, WS_CHILD, NULL, 0);
    if (myWindow != NULL)
    {
        SubclassWindow(myWindow);
        SetWindowPos(NULL, -32000, -32000, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER);
        hResult = explorerToolbar->QueryInterface(
            IID_PPV_ARG(IExplorerToolbar, &fExplorerToolbar));
    }
}

void CToolbarProxy::Destroy()
{
    DestroyWindow();
    fExplorerToolbar = NULL;
}

LRESULT CToolbarProxy::OnAddBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    long int                                result;
    HRESULT                                 hResult;

    result = 0;
    if (fExplorerToolbar.p != NULL)
    {
        hResult = fExplorerToolbar->AddBitmap(&CGID_ShellBrowser, 1, (long)wParam,
            reinterpret_cast<TBADDBITMAP *>(lParam), &result, RGB(192, 192, 192));
        hResult = fExplorerToolbar->AddBitmap(&CGID_ShellBrowser, 2, (long)wParam,
            reinterpret_cast<TBADDBITMAP *>(lParam), &result, RGB(192, 192, 192));
    }
    return result;
}

LRESULT CToolbarProxy::OnForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LRESULT                                 result;
    HRESULT                                 hResult;

    result = 0;
    if (fExplorerToolbar.p != NULL)
        hResult = fExplorerToolbar->SendToolbarMsg(&CGID_ShellBrowser, uMsg, wParam, lParam, &result);
    return result;
}

/*
Switch to a new bar when it receives an Exec(CGID_IDeskBand, 1, 1, vaIn, NULL);
    where vaIn will be a VT_UNKNOWN with the new bar. It also sends a RB_SHOWBAND to the
    rebar
*/

struct MenuBandInfo {
    GUID barGuid;
    BOOL fVertical;
};

class CShellBrowser :
    public CWindowImpl<CShellBrowser, CWindow, CFrameWinTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellBrowser,
    public IDropTarget,
    public IServiceProvider,
    public IProfferServiceImpl<CShellBrowser>,
    public IShellBrowserService,
    public IWebBrowser2,
    public ITravelLogClient,
    public IPersistHistory,
    public IDockingWindowSite,
    public IOleCommandTarget,
    public IBrowserService2,
    public IConnectionPointContainerImpl<CShellBrowser>,
    public MyIConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents2>,
    public MyIConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents>
{
private:
    class barInfo
    {
    public:
        barInfo()
        {
            memset(&borderSpace, 0, sizeof(borderSpace));
            hwnd = NULL;
        }

        RECT                                borderSpace;
        CComPtr<IUnknown>                   clientBar;
        HWND                                hwnd;
    };
    static const int                        BIInternetToolbar = 0;
    static const int                        BIVerticalBaseBar = 1;
    static const int                        BIHorizontalBaseBar = 2;

    HWND                                    fCurrentShellViewWindow;    // our currently hosted shell view window
    CComPtr<IShellFolder>                   fCurrentShellFolder;        //
    CComPtr<IShellView>                     fCurrentShellView;          //
    LPITEMIDLIST                            fCurrentDirectoryPIDL;      //
    HWND                                    fStatusBar;
    CToolbarProxy                           fToolbarProxy;
    barInfo                                 fClientBars[3];
    CComPtr<ITravelLog>                     fTravelLog;
    HMENU                                   fCurrentMenuBar;
    GUID                                    fCurrentVertBar;             //The guid of the built in vertical bar that is being shown
    // The next three fields support persisted history for shell views.
    // They do not need to be reference counted.
    IOleObject                              *fHistoryObject;
    IStream                                 *fHistoryStream;
    IBindCtx                                *fHistoryBindContext;
    HDSA menuDsa;
    HACCEL m_hAccel;
    ShellSettings m_settings;
    SBFOLDERSETTINGS m_deffoldersettings;
    DWORD m_BrowserSvcFlags;
    bool m_Destroyed;
public:
#if 0
    ULONG InternalAddRef()
    {
        OutputDebugString(_T("AddRef\n"));
        return CComObjectRootEx<CComMultiThreadModelNoCS>::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        OutputDebugString(_T("Release\n"));
        return CComObjectRootEx<CComMultiThreadModelNoCS>::InternalRelease();
    }
#endif

    CShellBrowser();
    ~CShellBrowser();
    HRESULT Initialize();
public:
    UINT ApplyNewBrowserFlag(UINT Flags);
    HRESULT OpenNewBrowserWindow(LPCITEMIDLIST pidl, UINT SbspFlags);
    HRESULT CreateRelativeBrowsePIDL(LPCITEMIDLIST relative, UINT SbspFlags, LPITEMIDLIST *ppidl);
    HRESULT BrowseToPIDL(LPCITEMIDLIST pidl, long flags);
    HRESULT BrowseToPath(IShellFolder *newShellFolder, LPCITEMIDLIST absolutePIDL,
        FOLDERSETTINGS *folderSettings, long flags);
    void SaveViewState();
    HRESULT GetMenuBand(REFIID riid, void **shellMenu);
    HRESULT GetBaseBar(bool vertical, REFIID riid, void **theBaseBar);
    BOOL IsBandLoaded(const CLSID clsidBand, bool vertical, DWORD *pdwBandID);
    HRESULT ShowBand(const CLSID &classID, bool vertical);
    HRESULT NavigateToParent();
    HRESULT DoFolderOptions();
    HRESULT ApplyBrowserDefaultFolderSettings(IShellView *pvs);
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RepositionBars();
    HRESULT BuildExplorerBandMenu();
    HRESULT BuildExplorerBandCategory(HMENU hBandsMenu, CATID category, DWORD dwPos, UINT *nbFound);
    BOOL IsBuiltinBand(CLSID &bandID);
    virtual WNDPROC GetWindowProc()
    {
        return WindowProc;
    }
    HRESULT FireEvent(DISPID dispIdMember, int argCount, VARIANT *arguments);
    HRESULT FireNavigateComplete(const wchar_t *newDirectory);
    HRESULT FireCommandStateChange(bool newState, int commandID);
    HRESULT FireCommandStateChangeAll();
    HRESULT UpdateForwardBackState();
    HRESULT UpdateUpState();
    void UpdateGotoMenu(HMENU theMenu);
    void UpdateViewMenu(HMENU theMenu);
    HRESULT IsInternetToolbarBandShown(UINT ITId);
    void RefreshCabinetState();
    void UpdateWindowTitle();
    void SaveITBarLayout();

/*    // *** IDockingWindowFrame methods ***
    STDMETHOD(AddToolbar)(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags) override;
    STDMETHOD(RemoveToolbar)(IUnknown *punkSrc, DWORD dwRemoveFlags) override;
    STDMETHOD(FindToolbar)(LPCWSTR pwszItem, REFIID riid, void **ppv) override;
    */

    // *** IDockingWindowSite methods ***
    STDMETHOD(GetBorderDW)(IUnknown* punkObj, LPRECT prcBorder) override;
    STDMETHOD(RequestBorderSpaceDW)(IUnknown* punkObj, LPCBORDERWIDTHS pbw) override;
    STDMETHOD(SetBorderSpaceDW)(IUnknown* punkObj, LPCBORDERWIDTHS pbw) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds,
        OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IShellBrowser methods ***
    STDMETHOD(InsertMenusSB)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) override;
    STDMETHOD(SetMenuSB)(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject) override;
    STDMETHOD(RemoveMenusSB)(HMENU hmenuShared) override;
    STDMETHOD(SetStatusTextSB)(LPCOLESTR pszStatusText) override;
    STDMETHOD(EnableModelessSB)(BOOL fEnable) override;
    STDMETHOD(TranslateAcceleratorSB)(MSG *pmsg, WORD wID) override;
    STDMETHOD(BrowseObject)(LPCITEMIDLIST pidl, UINT wFlags) override;
    STDMETHOD(GetViewStateStream)(DWORD grfMode, IStream **ppStrm) override;
    STDMETHOD(GetControlWindow)(UINT id, HWND *lphwnd) override;
    STDMETHOD(SendControlMsg)(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) override;
    STDMETHOD(QueryActiveShellView)(IShellView **ppshv) override;
    STDMETHOD(OnViewWindowActive)(IShellView *ppshv) override;
    STDMETHOD(SetToolbarItems)(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) override;

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IShellBrowserService methods ***
    STDMETHOD(GetPropertyBag)(long flags, REFIID riid, void **ppvObject) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override;
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    // *** IBrowserService methods ***
    STDMETHOD(GetParentSite)(IOleInPlaceSite **ppipsite) override;
    STDMETHOD(SetTitle)(IShellView *psv, LPCWSTR pszName) override;
    STDMETHOD(GetTitle)(IShellView *psv, LPWSTR pszName, DWORD cchName) override;
    STDMETHOD(GetOleObject)(IOleObject **ppobjv) override;
    STDMETHOD(GetTravelLog)(ITravelLog **pptl) override;
    STDMETHOD(ShowControlWindow)(UINT id, BOOL fShow) override;
    STDMETHOD(IsControlWindowShown)(UINT id, BOOL *pfShown) override;
    STDMETHOD(IEGetDisplayName)(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags) override;
    STDMETHOD(IEParseDisplayName)(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST *ppidlOut) override;
    STDMETHOD(DisplayParseError)(HRESULT hres, LPCWSTR pwszPath) override;
    STDMETHOD(NavigateToPidl)(LPCITEMIDLIST pidl, DWORD grfHLNF) override;
    STDMETHOD(SetNavigateState)(BNSTATE bnstate) override;
    STDMETHOD(GetNavigateState)(BNSTATE *pbnstate) override;
    STDMETHOD(NotifyRedirect)(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse) override;
    STDMETHOD(UpdateWindowList)() override;
    STDMETHOD(UpdateBackForwardState)() override;
    STDMETHOD(SetFlags)(DWORD dwFlags, DWORD dwFlagMask) override;
    STDMETHOD(GetFlags)(DWORD *pdwFlags) override;
    STDMETHOD(CanNavigateNow)( void) override;
    STDMETHOD(GetPidl)(LPITEMIDLIST *ppidl) override;
    STDMETHOD(SetReferrer)(LPCITEMIDLIST pidl) override;
    STDMETHOD_(DWORD, GetBrowserIndex)() override;
    STDMETHOD(GetBrowserByIndex)(DWORD dwID, IUnknown **ppunk) override;
    STDMETHOD(GetHistoryObject)(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc) override;
    STDMETHOD(SetHistoryObject)(IOleObject *pole, BOOL fIsLocalAnchor) override;
    STDMETHOD(CacheOLEServer)(IOleObject *pole) override;
    STDMETHOD(GetSetCodePage)(VARIANT *pvarIn, VARIANT *pvarOut) override;
    STDMETHOD(OnHttpEquiv)(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut) override;
    STDMETHOD(GetPalette)(HPALETTE *hpal) override;
    STDMETHOD(RegisterWindow)(BOOL fForceRegister, int swc) override;

    // *** IBrowserService2 methods ***
    STDMETHOD_(LRESULT, WndProcBS)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(SetAsDefFolderSettings)() override;
    STDMETHOD(GetViewRect)(RECT *prc) override;
    STDMETHOD(OnSize)(WPARAM wParam) override;
    STDMETHOD(OnCreate)(struct tagCREATESTRUCTW *pcs) override;
    STDMETHOD_(LRESULT, OnCommand)(WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(OnDestroy)() override;
    STDMETHOD_(LRESULT, OnNotify)(struct tagNMHDR *pnm) override;
    STDMETHOD(OnSetFocus)() override;
    STDMETHOD(OnFrameWindowActivateBS)(BOOL fActive) override;
    STDMETHOD(ReleaseShellView)() override;
    STDMETHOD(ActivatePendingView)() override;
    STDMETHOD(CreateViewWindow)(IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd) override;
    STDMETHOD(CreateBrowserPropSheetExt)(REFIID riid, void **ppv) override;
    STDMETHOD(GetViewWindow)(HWND *phwndView) override;
    STDMETHOD(GetBaseBrowserData)(LPCBASEBROWSERDATA *pbbd) override;
    STDMETHOD_(LPBASEBROWSERDATA, PutBaseBrowserData)(void) override;
    STDMETHOD(InitializeTravelLog)(ITravelLog *ptl, DWORD dw) override;
    STDMETHOD(SetTopBrowser)() override;
    STDMETHOD(Offline)(int iCmd) override;
    STDMETHOD(AllowViewResize)(BOOL f) override;
    STDMETHOD(SetActivateState)(UINT u) override;
    STDMETHOD(UpdateSecureLockIcon)(int eSecureLock) override;
    STDMETHOD(InitializeDownloadManager)() override;
    STDMETHOD(InitializeTransitionSite)() override;
    STDMETHOD(_Initialize)(HWND hwnd, IUnknown *pauto) override;
    STDMETHOD(_CancelPendingNavigationAsync)( void) override;
    STDMETHOD(_CancelPendingView)() override;
    STDMETHOD(_MaySaveChanges)() override;
    STDMETHOD(_PauseOrResumeView)(BOOL fPaused) override;
    STDMETHOD(_DisableModeless)() override;
    STDMETHOD(_NavigateToPidl)(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags) override;
    STDMETHOD(_TryShell2Rename)(IShellView *psv, LPCITEMIDLIST pidlNew) override;
    STDMETHOD(_SwitchActivationNow)() override;
    STDMETHOD(_ExecChildren)(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut) override;
    STDMETHOD(_SendChildren)(
        HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(GetFolderSetData)(struct tagFolderSetData *pfsd) override;
    STDMETHOD(_OnFocusChange)(UINT itb) override;
    STDMETHOD(v_ShowHideChildWindows)(BOOL fChildOnly) override;
    STDMETHOD_(UINT, _get_itbLastFocus)() override;
    STDMETHOD(_put_itbLastFocus)(UINT itbLastFocus) override;
    STDMETHOD(_UIActivateView)(UINT uState) override;
    STDMETHOD(_GetViewBorderRect)(RECT *prc) override;
    STDMETHOD(_UpdateViewRectSize)() override;
    STDMETHOD(_ResizeNextBorder)(UINT itb) override;
    STDMETHOD(_ResizeView)() override;
    STDMETHOD(_GetEffectiveClientArea)(LPRECT lprectBorder, HMONITOR hmon) override;
    STDMETHOD_(IStream *, v_GetViewStream)(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName) override;
    STDMETHOD_(LRESULT, ForwardViewMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(SetAcceleratorMenu)(HACCEL hacc) override;
    STDMETHOD_(int, _GetToolbarCount)() override;
    STDMETHOD_(LPTOOLBARITEM, _GetToolbarItem)(int itb) override;
    STDMETHOD(_SaveToolbars)(IStream *pstm) override;
    STDMETHOD(_LoadToolbars)(IStream *pstm) override;
    STDMETHOD(_CloseAndReleaseToolbars)(BOOL fClose) override;
    STDMETHOD(v_MayGetNextToolbarFocus)(LPMSG lpMsg, UINT itbNext,
        int citb, LPTOOLBARITEM *pptbi, HWND *phwnd) override;
    STDMETHOD(_ResizeNextBorderHelper)(UINT itb, BOOL bUseHmonitor) override;
    STDMETHOD_(UINT, _FindTBar)(IUnknown *punkSrc) override;
    STDMETHOD(_SetFocus)(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) override;
    STDMETHOD(v_MayTranslateAccelerator)(MSG *pmsg) override;
    STDMETHOD(_GetBorderDWHelper)(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) override;
    STDMETHOD(v_CheckZoneCrossing)(LPCITEMIDLIST pidl) override;

    // *** IWebBrowser methods ***
    STDMETHOD(GoBack)() override;
    STDMETHOD(GoForward)() override;
    STDMETHOD(GoHome)() override;
    STDMETHOD(GoSearch)() override;
    STDMETHOD(Navigate)(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName,
        VARIANT *PostData, VARIANT *Headers) override;
    STDMETHOD(Refresh)() override;
    STDMETHOD(Refresh2)(VARIANT *Level) override;
    STDMETHOD(Stop)() override;
    STDMETHOD(get_Application)(IDispatch **ppDisp) override;
    STDMETHOD(get_Parent)(IDispatch **ppDisp) override;
    STDMETHOD(get_Container)(IDispatch **ppDisp) override;
    STDMETHOD(get_Document)(IDispatch **ppDisp) override;
    STDMETHOD(get_TopLevelContainer)(VARIANT_BOOL *pBool) override;
    STDMETHOD(get_Type)(BSTR *Type) override;
    STDMETHOD(get_Left)(long *pl) override;
    STDMETHOD(put_Left)(long Left) override;
    STDMETHOD(get_Top)(long *pl) override;
    STDMETHOD(put_Top)(long Top) override;
    STDMETHOD(get_Width)(long *pl) override;
    STDMETHOD(put_Width)(long Width) override;
    STDMETHOD(get_Height)(long *pl) override;
    STDMETHOD(put_Height)(long Height) override;
    STDMETHOD(get_LocationName)(BSTR *LocationName) override;
    STDMETHOD(get_LocationURL)(BSTR *LocationURL) override;
    STDMETHOD(get_Busy)(VARIANT_BOOL *pBool) override;

    // *** IWebBrowserApp methods ***
    STDMETHOD(Quit)() override;
    STDMETHOD(ClientToWindow)(int *pcx, int *pcy) override;
    STDMETHOD(PutProperty)(BSTR Property, VARIANT vtValue) override;
    STDMETHOD(GetProperty)(BSTR Property, VARIANT *pvtValue) override;
    STDMETHOD(get_Name)(BSTR *Name) override;
    STDMETHOD(get_HWND)(SHANDLE_PTR *pHWND) override;
    STDMETHOD(get_FullName)(BSTR *FullName) override;
    STDMETHOD(get_Path)(BSTR *Path) override;
    STDMETHOD(get_Visible)(VARIANT_BOOL *pBool) override;
    STDMETHOD(put_Visible)(VARIANT_BOOL Value) override;
    STDMETHOD(get_StatusBar)(VARIANT_BOOL *pBool) override;
    STDMETHOD(put_StatusBar)(VARIANT_BOOL Value) override;
    STDMETHOD(get_StatusText)(BSTR *StatusText) override;
    STDMETHOD(put_StatusText)(BSTR StatusText) override;
    STDMETHOD(get_ToolBar)(int *Value) override;
    STDMETHOD(put_ToolBar)(int Value) override;
    STDMETHOD(get_MenuBar)(VARIANT_BOOL *Value) override;
    STDMETHOD(put_MenuBar)(VARIANT_BOOL Value) override;
    STDMETHOD(get_FullScreen)(VARIANT_BOOL *pbFullScreen) override;
    STDMETHOD(put_FullScreen)(VARIANT_BOOL bFullScreen) override;

    // *** IWebBrowser2 methods ***
    STDMETHOD(Navigate2)(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName,
        VARIANT *PostData, VARIANT *Headers) override;
    STDMETHOD(QueryStatusWB)(OLECMDID cmdID, OLECMDF *pcmdf) override;
    STDMETHOD(ExecWB)(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt,
        VARIANT *pvaIn, VARIANT *pvaOut) override;
    STDMETHOD(ShowBrowserBar)(VARIANT *pvaClsid, VARIANT *pvarShow, VARIANT *pvarSize) override;
    STDMETHOD(get_ReadyState)(READYSTATE *plReadyState) override;
    STDMETHOD(get_Offline)(VARIANT_BOOL *pbOffline) override;
    STDMETHOD(put_Offline)(VARIANT_BOOL bOffline) override;
    STDMETHOD(get_Silent)(VARIANT_BOOL *pbSilent) override;
    STDMETHOD(put_Silent)(VARIANT_BOOL bSilent) override;
    STDMETHOD(get_RegisterAsBrowser)(VARIANT_BOOL *pbRegister) override;
    STDMETHOD(put_RegisterAsBrowser)(VARIANT_BOOL bRegister) override;
    STDMETHOD(get_RegisterAsDropTarget)(VARIANT_BOOL *pbRegister) override;
    STDMETHOD(put_RegisterAsDropTarget)(VARIANT_BOOL bRegister) override;
    STDMETHOD(get_TheaterMode)(VARIANT_BOOL *pbRegister) override;
    STDMETHOD(put_TheaterMode)(VARIANT_BOOL bRegister) override;
    STDMETHOD(get_AddressBar)(VARIANT_BOOL *Value) override;
    STDMETHOD(put_AddressBar)(VARIANT_BOOL Value) override;
    STDMETHOD(get_Resizable)(VARIANT_BOOL *Value) override;
    STDMETHOD(put_Resizable)(VARIANT_BOOL Value) override;

    // *** ITravelLogClient methods ***
    STDMETHOD(FindWindowByIndex)(DWORD dwID, IUnknown **ppunk) override;
    STDMETHOD(GetWindowData)(IStream *pStream, LPWINDOWDATA pWinData) override;
    STDMETHOD(LoadHistoryPosition)(LPWSTR pszUrlLocation, DWORD dwPosition) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistHistory methods ***
    STDMETHOD(LoadHistory)(IStream *pStream, IBindCtx *pbc) override;
    STDMETHOD(SaveHistory)(IStream *pStream) override;
    STDMETHOD(SetPositionCookie)(DWORD dwPositioncookie) override;
    STDMETHOD(GetPositionCookie)(DWORD *pdwPositioncookie) override;

    // message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT RelayMsgToShellView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnFolderOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnMapNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnDisconnectNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnAboutReactOS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnGoBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnGoForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnGoUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnBackspace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnGoHome(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnAddToFavorites(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnOrganizeFavorites(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleStatusBarVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleToolbarLock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleToolbarBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleAddressBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleLinksBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToggleTextLabels(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnToolbarCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnGoTravel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnExplorerBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT RelayCommands(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCabinetStateChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSettingsChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetSettingsPtr(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HRESULT OnSearch();

    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        static ATL::CWndClassInfo wc =
        {
            { sizeof(WNDCLASSEX), CS_DBLCLKS, StartWindowProc,
              0, 0, NULL, LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_CABINET)),
              LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, szCabinetWndClass, NULL },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    BEGIN_MSG_MAP(CShellBrowser)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_MEASUREITEM, RelayMsgToShellView)
        MESSAGE_HANDLER(WM_DRAWITEM, RelayMsgToShellView)
        MESSAGE_HANDLER(WM_MENUSELECT, RelayMsgToShellView)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        COMMAND_ID_HANDLER(IDM_FILE_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDM_TOOLS_FOLDEROPTIONS, OnFolderOptions)
        COMMAND_ID_HANDLER(IDM_TOOLS_MAPNETWORKDRIVE, OnMapNetworkDrive)
        COMMAND_ID_HANDLER(IDM_TOOLS_DISCONNECTNETWORKDRIVE, OnDisconnectNetworkDrive)
        COMMAND_ID_HANDLER(IDM_HELP_ABOUT, OnAboutReactOS)
        COMMAND_ID_HANDLER(IDM_GOTO_BACK, OnGoBack)
        COMMAND_ID_HANDLER(IDM_GOTO_FORWARD, OnGoForward)
        COMMAND_ID_HANDLER(IDM_GOTO_UPONELEVEL, OnGoUpLevel)
        COMMAND_ID_HANDLER(IDM_GOTO_HOMEPAGE, OnGoHome)
        COMMAND_ID_HANDLER(IDM_FAVORITES_ADDTOFAVORITES, OnAddToFavorites)
        COMMAND_ID_HANDLER(IDM_FAVORITES_ORGANIZEFAVORITES, OnOrganizeFavorites)
        COMMAND_ID_HANDLER(IDM_VIEW_STATUSBAR, OnToggleStatusBarVisible)
        COMMAND_ID_HANDLER(IDM_VIEW_REFRESH, OnRefresh)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_LOCKTOOLBARS, OnToggleToolbarLock)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_STANDARDBUTTONS, OnToggleToolbarBandVisible)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_ADDRESSBAR, OnToggleAddressBandVisible)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_LINKSBAR, OnToggleLinksBandVisible)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_TEXTLABELS, OnToggleTextLabels)
        COMMAND_ID_HANDLER(IDM_TOOLBARS_CUSTOMIZE, OnToolbarCustomize)
        COMMAND_ID_HANDLER(IDM_EXPLORERBAR_SEARCH, OnExplorerBar)
        COMMAND_ID_HANDLER(IDM_EXPLORERBAR_FOLDERS, OnExplorerBar)
        COMMAND_ID_HANDLER(IDM_EXPLORERBAR_HISTORY, OnExplorerBar)
        COMMAND_ID_HANDLER(IDM_EXPLORERBAR_FAVORITES, OnExplorerBar)
        COMMAND_ID_HANDLER(IDM_BACKSPACE, OnBackspace)
        COMMAND_RANGE_HANDLER(IDM_GOTO_TRAVEL_FIRSTTARGET, IDM_GOTO_TRAVEL_LASTTARGET, OnGoTravel)
        COMMAND_RANGE_HANDLER(IDM_EXPLORERBAND_BEGINCUSTOM, IDM_EXPLORERBAND_ENDCUSTOM, OnExplorerBar)
        MESSAGE_HANDLER(WM_COMMAND, RelayCommands)
        MESSAGE_HANDLER(CWM_STATECHANGE, OnCabinetStateChange)
        MESSAGE_HANDLER(BWM_SETTINGCHANGE, OnSettingsChange)
        MESSAGE_HANDLER(BWM_GETSETTINGSPTR, OnGetSettingsPtr)
        MESSAGE_HANDLER(WM_APPCOMMAND, OnAppCommand)
    END_MSG_MAP()

    BEGIN_CONNECTION_POINT_MAP(CShellBrowser)
        CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents2)
        CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents)
    END_CONNECTION_POINT_MAP()

    BEGIN_COM_MAP(CShellBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDockingWindowSite)
        COM_INTERFACE_ENTRY_IID(IID_IShellBrowser, IShellBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IProfferService, IProfferService)
        COM_INTERFACE_ENTRY_IID(IID_IShellBrowserService, IShellBrowserService)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IConnectionPointContainer, IConnectionPointContainer)
        COM_INTERFACE_ENTRY_IID(IID_IWebBrowser, IWebBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IWebBrowserApp, IWebBrowserApp)
        COM_INTERFACE_ENTRY_IID(IID_IWebBrowser2, IWebBrowser2)
        COM_INTERFACE_ENTRY_IID(IID_ITravelLogClient, ITravelLogClient)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistHistory, IPersistHistory)
        COM_INTERFACE_ENTRY_IID(IID_IBrowserService, IBrowserService)
        COM_INTERFACE_ENTRY_IID(IID_IBrowserService2, IBrowserService2)
    END_COM_MAP()
};

extern HRESULT CreateProgressDialog(REFIID riid, void **ppv);

CShellBrowser::CShellBrowser()
{
    m_BrowserSvcFlags = BSF_RESIZABLE | BSF_CANMAXIMIZE;
    m_Destroyed = false;
    fCurrentShellViewWindow = NULL;
    fCurrentDirectoryPIDL = NULL;
    fStatusBar = NULL;
    fCurrentMenuBar = NULL;
    fHistoryObject = NULL;
    fHistoryStream = NULL;
    fHistoryBindContext = NULL;
    m_settings.Load();
    m_deffoldersettings.Load();
    gCabinetState.Load();
    SetTopBrowser();
}

CShellBrowser::~CShellBrowser()
{
    if (menuDsa)
        DSA_Destroy(menuDsa);
}

HRESULT CShellBrowser::Initialize()
{
    CComPtr<IPersistStreamInit>             persistStreamInit;
    HRESULT                                 hResult;
    CComPtr<IUnknown> clientBar;

    _AtlInitialConstruct();

    menuDsa = DSA_Create(sizeof(MenuBandInfo), 5);
    if (!menuDsa)
        return E_OUTOFMEMORY;

    // create window
    Create(HWND_DESKTOP);
    if (m_hWnd == NULL)
        return E_FAIL;

    hResult = CInternetToolbar_CreateInstance(IID_PPV_ARG(IUnknown, &clientBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    fClientBars[BIInternetToolbar].clientBar = clientBar;

    // create interfaces
    hResult = clientBar->QueryInterface(IID_PPV_ARG(IPersistStreamInit, &persistStreamInit));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = IUnknown_SetSite(clientBar, static_cast<IShellBrowser *>(this));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = IUnknown_Exec(clientBar, CGID_PrivCITCommands, 1, 1 /* or 0 */, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    CComPtr<IStream> pITBarStream;
    hResult = CInternetToolbar::GetStream(ITBARSTREAM_EXPLORER, STGM_READ, &pITBarStream);
    hResult = SUCCEEDED(hResult) ? persistStreamInit->Load(pITBarStream) : persistStreamInit->InitNew();
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = IUnknown_ShowDW(clientBar, TRUE);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    fToolbarProxy.Initialize(m_hWnd, clientBar);

    // create status bar
    DWORD dwStatusStyle = WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | SBARS_TOOLTIPS;
    if (m_settings.fStatusBarVisible)
        dwStatusStyle |= WS_VISIBLE;
    fStatusBar = ::CreateWindowExW(0, STATUSCLASSNAMEW, NULL, dwStatusStyle,
                                   0, 0, 500, 20, m_hWnd, (HMENU)IDC_STATUSBAR,
                                   _AtlBaseModule.GetModuleInstance(), 0);

    ShowWindow(SW_SHOWNORMAL);
    UpdateWindow();

    return S_OK;
}

HRESULT CShellBrowser::ApplyBrowserDefaultFolderSettings(IShellView *pvs)
{
    HRESULT hr;
    if (pvs)
    {
        m_settings.Save();
        SBFOLDERSETTINGS &sbfs = m_deffoldersettings, defsbfs;
        if (FAILED(pvs->GetCurrentInfo(&sbfs.FolderSettings)))
        {
            defsbfs.InitializeDefaults();
            sbfs = defsbfs;
        }
        hr = CGlobalFolderSettings::SaveBrowserSettings(sbfs);
    }
    else
    {
        m_settings.Reset();
        hr = CGlobalFolderSettings::ResetBrowserSettings();
        if (SUCCEEDED(hr))
            m_deffoldersettings.Load();
    }
    return hr;
}

UINT CShellBrowser::ApplyNewBrowserFlag(UINT Flags)
{
    if ((Flags & (SBSP_SAMEBROWSER | SBSP_NEWBROWSER)) == SBSP_DEFBROWSER)
    {
        if (!fCurrentDirectoryPIDL || IsControlWindowShown(FCW_TREE, NULL) == S_OK)
            Flags |= SBSP_SAMEBROWSER; // Force if this is the first navigation or the folder tree is present
        else
            Flags |= (!!gCabinetState.fNewWindowMode) ^ (GetAsyncKeyState(VK_CONTROL) < 0) ? SBSP_NEWBROWSER : SBSP_SAMEBROWSER;
    }
    if (Flags & (SBSP_NAVIGATEBACK | SBSP_NAVIGATEFORWARD))
        Flags = (Flags & ~SBSP_NEWBROWSER) | SBSP_SAMEBROWSER; // Force same browser for now
    return Flags;
}

HRESULT CShellBrowser::OpenNewBrowserWindow(LPCITEMIDLIST pidl, UINT SbspFlags)
{
    SaveITBarLayout(); // Do this now so the new window inherits the current layout
    // TODO: www.geoffchappell.com/studies/windows/ie/shdocvw/interfaces/inotifyappstart.htm
    DWORD flags = (SbspFlags & SBSP_EXPLOREMODE) ? SH_EXPLORER_CMDLINE_FLAG_E : 0;
    if ((SbspFlags & (SBSP_OPENMODE | SBSP_EXPLOREMODE)) == SBSP_DEFMODE)
        flags |= IsControlWindowShown(FCW_TREE, NULL) == S_OK ? SH_EXPLORER_CMDLINE_FLAG_E : 0;
    LPITEMIDLIST pidlDir;
    HRESULT hr = SHILClone(pidl, &pidlDir);
    if (FAILED(hr))
        return hr;
    // TODO: !SBSP_NOTRANSFERHIST means we are supposed to pass the history here somehow?
    return SHOpenNewFrame(pidlDir, NULL, 0, flags | SH_EXPLORER_CMDLINE_FLAG_NEWWND | SH_EXPLORER_CMDLINE_FLAG_NOREUSE);
}

HRESULT CShellBrowser::CreateRelativeBrowsePIDL(LPCITEMIDLIST relative, UINT SbspFlags, LPITEMIDLIST *ppidl)
{
    if (SbspFlags & SBSP_RELATIVE)
        return SHILCombine(fCurrentDirectoryPIDL, relative, ppidl);

    if (SbspFlags & SBSP_PARENT)
    {
        HRESULT hr = GetPidl(ppidl);
        if (FAILED(hr))
            return hr;
        ILRemoveLastID(*ppidl);
        return S_OK;
    }
    // TODO: SBSP_NAVIGATEBACK and SBSP_NAVIGATEFORWARD?
    return E_UNEXPECTED;
}

HRESULT CShellBrowser::BrowseToPIDL(LPCITEMIDLIST pidl, long flags)
{
    CComPtr<IShellFolder>   newFolder;
    FOLDERSETTINGS          newFolderSettings = m_deffoldersettings.FolderSettings;
    HRESULT                 hResult;
    CLSID                   clsid;
    BOOL                    HasIconViewType;

    // called by shell view to browse to new folder
    // also called by explorer band to navigate to new folder
    hResult = SHBindToFolder(pidl, &newFolder);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    // HACK & FIXME: Get view mode from shellbag when fully implemented.
    IUnknown_GetClassID(newFolder, &clsid);
    HasIconViewType = clsid == CLSID_MyComputer || clsid == CLSID_ControlPanel ||
                      clsid == CLSID_ShellDesktop;

    if (HasIconViewType)
        newFolderSettings.ViewMode = FVM_ICON;
    hResult = BrowseToPath(newFolder, pidl, &newFolderSettings, flags);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

BOOL WINAPI _ILIsPidlSimple(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST                           pidlnext;
    WORD                                    length;
    BOOL                                    ret;

    ret = TRUE;
    if (! _ILIsDesktop(pidl))
    {
        length = pidl->mkid.cb;
        pidlnext =
            reinterpret_cast<LPCITEMIDLIST>(
                reinterpret_cast<const BYTE *>(pidl) + length);
        if (pidlnext->mkid.cb != 0)
            ret = FALSE;
    }
    return ret;
}

HRESULT WINAPI SHBindToFolderIDListParent(IShellFolder *unused, LPCITEMIDLIST pidl,
    const IID *riid, LPVOID *ppv, LPITEMIDLIST *ppidlLast)
{
    CComPtr<IShellFolder>                   psf;
    LPITEMIDLIST                            pidlChild;
    LPITEMIDLIST                            pidlParent;
    HRESULT                                 hResult;

    hResult = E_FAIL;
    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    if (ppidlLast != NULL)
        *ppidlLast = NULL;
    if (_ILIsPidlSimple(pidl))
    {
        if (ppidlLast != NULL)
            *ppidlLast = ILClone(pidl);
        hResult = SHGetDesktopFolder((IShellFolder **)ppv);
    }
    else
    {
        pidlChild = ILClone(ILFindLastID(pidl));
        pidlParent = ILClone(pidl);
        ILRemoveLastID(pidlParent);
        hResult = SHGetDesktopFolder(&psf);
        if (SUCCEEDED(hResult))
            hResult = psf->BindToObject(pidlParent, NULL, *riid, ppv);
        if (SUCCEEDED(hResult) && ppidlLast != NULL)
            *ppidlLast = pidlChild;
        else
            ILFree(pidlChild);
        ILFree(pidlParent);
    }
    return hResult;
}

HRESULT IEGetNameAndFlagsEx(LPITEMIDLIST pidl, SHGDNF uFlags, long param10,
    LPWSTR pszBuf, UINT cchBuf, SFGAOF *rgfInOut)
{
    CComPtr<IShellFolder>                   parentFolder;
    LPITEMIDLIST                            childPIDL = NULL;
    STRRET                                  L108;
    HRESULT                                 hResult;

    hResult = SHBindToFolderIDListParent(NULL, pidl, &IID_PPV_ARG(IShellFolder, &parentFolder), &childPIDL);
    if (FAILED(hResult))
        goto cleanup;

    hResult = parentFolder->GetDisplayNameOf(childPIDL, uFlags, &L108);
    if (FAILED(hResult))
        goto cleanup;

    StrRetToBufW(&L108, childPIDL, pszBuf, cchBuf);
    if (rgfInOut)
    {
        hResult = parentFolder->GetAttributesOf(1, const_cast<LPCITEMIDLIST *>(&childPIDL), rgfInOut);
        if (FAILED(hResult))
            goto cleanup;
    }

    hResult = S_OK;

cleanup:
    if (childPIDL)
        ILFree(childPIDL);
    return hResult;
}

HRESULT IEGetNameAndFlags(LPITEMIDLIST pidl, SHGDNF uFlags, LPWSTR pszBuf, UINT cchBuf, SFGAOF *rgfInOut)
{
    return IEGetNameAndFlagsEx(pidl, uFlags, 0, pszBuf, cchBuf, rgfInOut);
}

void CShellBrowser::SaveViewState()
{
    // TODO: Also respect EBO_NOPERSISTVIEWSTATE?
    if (gCabinetState.fSaveLocalView && fCurrentShellView && !SHRestricted(REST_NOSAVESET))
        fCurrentShellView->SaveViewState();
}

HRESULT CShellBrowser::BrowseToPath(IShellFolder *newShellFolder,
    LPCITEMIDLIST absolutePIDL, FOLDERSETTINGS *folderSettings, long flags)
{
    CComPtr<IShellFolder>                   saveCurrentShellFolder;
    CComPtr<IShellView>                     saveCurrentShellView;
    CComPtr<IShellView>                     newShellView;
    CComPtr<ITravelLog>                     travelLog;
    HWND                                    newShellViewWindow;
    BOOL                                    windowUpdateIsLocked;
    RECT                                    shellViewWindowBounds;
    HWND                                    previousView;
    HCURSOR                                 saveCursor;
    wchar_t                                 newTitle[MAX_PATH];
    SHGDNF                                  nameFlags;
    HRESULT                                 hResult;
    //TODO: BOOL                            nohistory = m_BrowserSvcFlags & BSF_NAVNOHISTORY;

    if (m_Destroyed)
        return S_FALSE;

    if (newShellFolder == NULL)
        return E_INVALIDARG;

    hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (FAILED_UNEXPECTEDLY(hResult = SHILClone(absolutePIDL, &absolutePIDL)))
        return hResult;
    CComHeapPtr<ITEMIDLIST> pidlAbsoluteClone(const_cast<LPITEMIDLIST>(absolutePIDL));

    // update history
    if (flags & BTP_UPDATE_CUR_HISTORY)
    {
        if (travelLog->CountEntries(static_cast<IDropTarget *>(this)) > 0)
            hResult = travelLog->UpdateEntry(static_cast<IDropTarget *>(this), FALSE);
        // what to do with error? Do we want to halt browse because state save failed?
    }

    if (fCurrentShellView)
    {
        SaveViewState();
        fCurrentShellView->UIActivate(SVUIA_DEACTIVATE);
    }

    // create view object
    hResult = newShellFolder->CreateViewObject(m_hWnd, IID_PPV_ARG(IShellView, &newShellView));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    previousView = fCurrentShellViewWindow;

    // enter updating section
    saveCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    windowUpdateIsLocked = LockWindowUpdate(TRUE);
    if (fCurrentShellView != NULL)
        ::SendMessage(fCurrentShellViewWindow, WM_SETREDRAW, 0, 0);

    // set site
    hResult = IUnknown_SetSite(newShellView, static_cast<IDropTarget *>(this));

    // update folder and view
    saveCurrentShellFolder = fCurrentShellFolder;
    saveCurrentShellView = fCurrentShellView;
    fCurrentShellFolder = newShellFolder;
    fCurrentShellView = newShellView;

    // get boundary
    if (previousView != NULL)
        ::GetWindowRect(previousView, &shellViewWindowBounds);
    else
        ZeroMemory(&shellViewWindowBounds, sizeof(shellViewWindowBounds));
    ::MapWindowPoints(0, m_hWnd, reinterpret_cast<POINT *>(&shellViewWindowBounds), 2);

    // update current pidl
    ILFree(fCurrentDirectoryPIDL);
    fCurrentDirectoryPIDL = pidlAbsoluteClone.Detach();
    /* CORE-19697: CAddressEditBox::OnWinEvent(CBN_SELCHANGE) causes CAddressEditBox to
     * call BrowseObject(pidlLastParsed). As part of our browsing we call FireNavigateComplete
     * and this in turn causes CAddressEditBox::Invoke to ILFree(pidlLastParsed)!
     * We then call SHBindToParent on absolutePIDL (which is really (the now invalid) pidlLastParsed) and we
     * end up accessing invalid memory! We therefore set absolutePIDL to be our cloned PIDL here.
     */
    absolutePIDL = fCurrentDirectoryPIDL;

    // create view window
    hResult = newShellView->CreateViewWindow(saveCurrentShellView, folderSettings,
        this, &shellViewWindowBounds, &newShellViewWindow);
    if (FAILED_UNEXPECTEDLY(hResult) || newShellViewWindow == NULL)
    {
        fCurrentShellView = saveCurrentShellView;
        fCurrentShellFolder = saveCurrentShellFolder;
        ::SendMessage(fCurrentShellViewWindow, WM_SETREDRAW, 1, 0);
        if (windowUpdateIsLocked)
            LockWindowUpdate(FALSE);
        SetCursor(saveCursor);
        return hResult;
    }

    // update view window
    if (saveCurrentShellView != NULL)
        saveCurrentShellView->DestroyViewWindow();
    fCurrentShellViewWindow = newShellViewWindow;

    if (previousView == NULL)
    {
        RepositionBars();
    }

    // no use
    saveCurrentShellView.Release();
    saveCurrentShellFolder.Release();

    hResult = newShellView->UIActivate((flags & BTP_ACTIVATE_NOFOCUS) ? SVUIA_ACTIVATE_NOFOCUS : SVUIA_ACTIVATE_FOCUS);

    // leave updating section
    if (windowUpdateIsLocked)
        LockWindowUpdate(FALSE);
    SetCursor(saveCursor);

    // update history
    if (flags & BTP_UPDATE_NEXT_HISTORY)
    {
        hResult = travelLog->AddEntry(static_cast<IDropTarget *>(this), FALSE);
        hResult = travelLog->UpdateEntry(static_cast<IDropTarget *>(this), FALSE);
    }

    // completed
    nameFlags = SHGDN_FORADDRESSBAR | SHGDN_FORPARSING;
    hResult = IEGetNameAndFlags(fCurrentDirectoryPIDL, nameFlags, newTitle,
        sizeof(newTitle) / sizeof(wchar_t), NULL);
    if (SUCCEEDED(hResult))
    {
        FireNavigateComplete(newTitle);
    }
    else
    {
        FireNavigateComplete(L"ERROR");
    }

    UpdateWindowTitle();

    LPCITEMIDLIST pidlChild;
    INT index, indexOpen;
    HIMAGELIST himlSmall, himlLarge;

    CComPtr<IShellFolder> sf;
    hResult = SHBindToParent(absolutePIDL, IID_PPV_ARG(IShellFolder, &sf), &pidlChild);
    if (SUCCEEDED(hResult))
    {
        index = SHMapPIDLToSystemImageListIndex(sf, pidlChild, &indexOpen);

        Shell_GetImageLists(&himlLarge, &himlSmall);

        HICON icSmall = ImageList_GetIcon(himlSmall, indexOpen, 0);
        HICON icLarge = ImageList_GetIcon(himlLarge, indexOpen, 0);

        /* Hack to make it possible to release the old icons */
        /* Something seems to go wrong with WM_SETICON */
        HICON oldSmall = (HICON)SendMessage(WM_GETICON, ICON_SMALL, 0);
        HICON oldLarge = (HICON)SendMessage(WM_GETICON, ICON_BIG,   0);

        SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icSmall));
        SendMessage(WM_SETICON, ICON_BIG,   reinterpret_cast<LPARAM>(icLarge));

        DestroyIcon(oldSmall);
        DestroyIcon(oldLarge);
    }

    FireCommandStateChangeAll();
    hResult = UpdateForwardBackState();
    hResult = UpdateUpState();
    return S_OK;
}

HRESULT CShellBrowser::GetMenuBand(REFIID riid, void **shellMenu)
{
    CComPtr<IBandSite>                      bandSite;
    CComPtr<IDeskBand>                      deskBand;
    HRESULT                                 hResult;

    if (!fClientBars[BIInternetToolbar].clientBar)
        return E_FAIL;

    hResult = IUnknown_QueryService(fClientBars[BIInternetToolbar].clientBar, SID_IBandSite, IID_PPV_ARG(IBandSite, &bandSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = bandSite->QueryBand(1, &deskBand, NULL, NULL, 0);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    return deskBand->QueryInterface(riid, shellMenu);
}

HRESULT CShellBrowser::GetBaseBar(bool vertical, REFIID riid, void **theBaseBar)
{
    CComPtr<IUnknown>                       newBaseBar;
    CComPtr<IDeskBar>                       deskBar;
    CComPtr<IUnknown>                       newBaseBarSite;
    CComPtr<IDeskBarClient>                 deskBarClient;
    IUnknown                                **cache;
    HRESULT                                 hResult;

    if (vertical)
        cache = &fClientBars[BIVerticalBaseBar].clientBar.p;
    else
        cache = &fClientBars[BIHorizontalBaseBar].clientBar.p;
    if (*cache == NULL)
    {
        hResult = CBaseBar_CreateInstance(IID_PPV_ARG(IUnknown, &newBaseBar), vertical);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = CBaseBarSite_CreateInstance(IID_PPV_ARG(IUnknown, &newBaseBarSite), vertical);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        // we have to store our basebar into cache now
        *cache = newBaseBar;
        (*cache)->AddRef();

        // tell the new base bar about the shell browser
        hResult = IUnknown_SetSite(newBaseBar, static_cast<IDropTarget *>(this));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        // tell the new base bar about the new base bar site
        hResult = newBaseBar->QueryInterface(IID_PPV_ARG(IDeskBar, &deskBar));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = deskBar->SetClient(newBaseBarSite);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        // tell the new base bar site about the new base bar
        hResult = newBaseBarSite->QueryInterface(IID_PPV_ARG(IDeskBarClient, &deskBarClient));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = deskBarClient->SetDeskBarSite(newBaseBar);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

    }
    return (*cache)->QueryInterface(riid, theBaseBar);
}

BOOL CShellBrowser::IsBandLoaded(const CLSID clsidBand, bool vertical, DWORD *pdwBandID)
{
    HRESULT                                 hResult;
    CComPtr<IDeskBar>                       deskBar;
    CComPtr<IUnknown>                       baseBarSite;
    CComPtr<IBandSite>                      bandSite;
    CLSID                                   clsidTmp;
    DWORD                                   numBands;
    DWORD                                   dwBandID;
    DWORD                                   i;

    /* Get our basebarsite to be able to enumerate bands */
    hResult = GetBaseBar(vertical, IID_PPV_ARG(IDeskBar, &deskBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;
    hResult = deskBar->GetClient(&baseBarSite);
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;
    hResult = baseBarSite->QueryInterface(IID_PPV_ARG(IBandSite, &bandSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;

    numBands = bandSite->EnumBands(-1, NULL);
    for (i = 0; i < numBands; i++)
    {
        CComPtr<IPersist> bandPersist;

        hResult = bandSite->EnumBands(i, &dwBandID);
        if (FAILED_UNEXPECTEDLY(hResult))
            return FALSE;

        hResult = bandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &bandPersist));
        if (FAILED_UNEXPECTEDLY(hResult))
            return FALSE;
        hResult = bandPersist->GetClassID(&clsidTmp);
        if (FAILED_UNEXPECTEDLY(hResult))
            return FALSE;
        if (IsEqualGUID(clsidBand, clsidTmp))
        {
            if (pdwBandID) *pdwBandID = dwBandID;
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT CShellBrowser::ShowBand(const CLSID &classID, bool vertical)
{
    CComPtr<IDockingWindow>                 dockingWindow;
    CComPtr<IUnknown>                       baseBarSite;
    CComPtr<IUnknown>                       newBand;
    CComPtr<IDeskBar>                       deskBar;
    VARIANT                                 vaIn;
    HRESULT                                 hResult;
    DWORD                                   dwBandID;

    hResult = GetBaseBar(vertical, IID_PPV_ARG(IDeskBar, &deskBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = deskBar->GetClient(&baseBarSite);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = deskBar->QueryInterface(IID_PPV_ARG(IDockingWindow, &dockingWindow));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (!IsBandLoaded(classID, vertical, &dwBandID))
    {
        TRACE("ShowBand called for CLSID %s, vertical=%d...\n", wine_dbgstr_guid(&classID), vertical);
        if (IsEqualCLSID(CLSID_ExplorerBand, classID))
        {
            TRACE("CLSID_ExplorerBand requested, building internal band.\n");
            hResult = CExplorerBand_CreateInstance(IID_PPV_ARG(IUnknown, &newBand));
            if (FAILED_UNEXPECTEDLY(hResult))
                return hResult;
        }
        else if (IsEqualCLSID(classID, CLSID_FileSearchBand))
        {
            TRACE("CLSID_FileSearchBand requested, building internal band.\n");
            hResult = CSearchBar_CreateInstance(IID_PPV_ARG(IUnknown, &newBand));
            if (FAILED_UNEXPECTEDLY(hResult))
                return hResult;
        }
        else
        {
            TRACE("A different CLSID requested, using CoCreateInstance.\n");
            hResult = CoCreateInstance(classID, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &newBand));
            if (FAILED_UNEXPECTEDLY(hResult))
                return hResult;
        }
    }
    else
    {
        CComPtr<IBandSite>                  pBandSite;

        hResult = baseBarSite->QueryInterface(IID_PPV_ARG(IBandSite, &pBandSite));
        if (!SUCCEEDED(hResult))
        {
            ERR("Can't get IBandSite interface\n");
            return E_FAIL;
        }
        hResult = pBandSite->GetBandObject(dwBandID, IID_PPV_ARG(IUnknown, &newBand));
        if (!SUCCEEDED(hResult))
        {
            ERR("Can't find band object\n");
            return E_FAIL;
        }

        // It's hackish, but we should be able to show the wanted band until we
        // find the proper way to do this (but it seems to work to add a new band)
        // Here we'll just re-add the existing band to the site, causing it to display.
    }
    V_VT(&vaIn) = VT_UNKNOWN;
    V_UNKNOWN(&vaIn) = newBand.p;
    hResult = IUnknown_Exec(baseBarSite, CGID_IDeskBand, 1, 1, &vaIn, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
    {
        return hResult;
    }

    hResult = dockingWindow->ShowDW(TRUE);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (vertical)
    {
        fCurrentVertBar = classID;
        FireCommandStateChangeAll();
    }

    return S_OK;
}

HRESULT CShellBrowser::NavigateToParent()
{
    LPITEMIDLIST newDirectory = ILClone(fCurrentDirectoryPIDL);
    if (newDirectory == NULL)
        return E_OUTOFMEMORY;
    if (_ILIsDesktop(newDirectory))
    {
        ILFree(newDirectory);
        return E_INVALIDARG;
    }
    ILRemoveLastID(newDirectory);
    HRESULT hResult = BrowseToPIDL(newDirectory, BTP_UPDATE_CUR_HISTORY | BTP_UPDATE_NEXT_HISTORY);
    ILFree(newDirectory);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

BOOL CALLBACK AddFolderOptionsPage(HPROPSHEETPAGE thePage, LPARAM lParam)
{
    PROPSHEETHEADER* sheetInfo = reinterpret_cast<PROPSHEETHEADER*>(lParam);
    if (sheetInfo->nPages >= folderOptionsPageCountMax)
        return FALSE;
    sheetInfo->phpage[sheetInfo->nPages] = thePage;
    sheetInfo->nPages++;
    return TRUE;
}

HRESULT CShellBrowser::DoFolderOptions()
{
    CComPtr<IShellPropSheetExt>             folderOptionsSheet;
    PROPSHEETHEADER                         m_PropSheet;
    HPROPSHEETPAGE                          m_psp[folderOptionsPageCountMax];
//    CComPtr<IGlobalFolderSettings>          globalSettings;
//    SHELLSTATE2                             shellState;
    HRESULT                                 hResult;

    memset(m_psp, 0, sizeof(m_psp));
    memset(&m_PropSheet, 0, sizeof(m_PropSheet));

    // create sheet object
    hResult = CoCreateInstance(CLSID_ShellFldSetExt, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IShellPropSheetExt, &folderOptionsSheet));
    if (FAILED_UNEXPECTEDLY(hResult))
        return E_FAIL;

    // must set site in order for Apply to all Folders on Advanced page to be enabled
    hResult = IUnknown_SetSite(folderOptionsSheet, static_cast<IDispatch *>(this));
    m_PropSheet.phpage = m_psp;

#if 0
    hResult = CoCreateInstance(CLSID_GlobalFolderSettings, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IGlobalFolderSettings, &globalSettings));
    if (FAILED_UNEXPECTEDLY(hResult))
        return E_FAIL;
    hResult = globalSettings->Get(&shellState, sizeof(shellState));
    if (FAILED_UNEXPECTEDLY(hResult))
        return E_FAIL;
#endif

    // add pages
    hResult = folderOptionsSheet->AddPages(AddFolderOptionsPage, reinterpret_cast<LPARAM>(&m_PropSheet));
    if (FAILED_UNEXPECTEDLY(hResult))
        return E_FAIL;

    if (fCurrentShellView)
    {
        hResult = fCurrentShellView->AddPropertySheetPages(
            0, AddFolderOptionsPage, reinterpret_cast<LPARAM>(&m_PropSheet));
        if (FAILED_UNEXPECTEDLY(hResult))
            return E_FAIL;
    }

    // show sheet
    CStringW strFolderOptions(MAKEINTRESOURCEW(IDS_FOLDER_OPTIONS));
    m_PropSheet.dwSize = sizeof(PROPSHEETHEADER);
    m_PropSheet.dwFlags = 0;
    m_PropSheet.hwndParent = m_hWnd;
    m_PropSheet.hInstance = _AtlBaseModule.GetResourceInstance();
    m_PropSheet.pszCaption = strFolderOptions;
    m_PropSheet.nStartPage = 0;
    PropertySheet(&m_PropSheet);
    return S_OK;
}

LRESULT CALLBACK CShellBrowser::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CShellBrowser                           *pThis = reinterpret_cast<CShellBrowser *>(hWnd);
    _ATL_MSG                                msg(pThis->m_hWnd, uMsg, wParam, lParam);
    LRESULT                                 lResult;
    const _ATL_MSG                          *previousMessage;
    BOOL                                    handled;
    WNDPROC                                 saveWindowProc;
    HRESULT                                 hResult;

    hWnd = pThis->m_hWnd;
    previousMessage = pThis->m_pCurrentMsg;
    pThis->m_pCurrentMsg = &msg;

    /* If the shell browser is initialized, let the menu band preprocess the messages */
    if (pThis->fCurrentDirectoryPIDL)
    {
        CComPtr<IMenuBand> menuBand;
        hResult = pThis->GetMenuBand(IID_PPV_ARG(IMenuBand, &menuBand));
        if (SUCCEEDED(hResult) && menuBand.p != NULL)
        {
            hResult = menuBand->TranslateMenuMessage(&msg, &lResult);
            if (hResult == S_OK)
                return lResult;
            uMsg = msg.message;
            wParam = msg.wParam;
            lParam = msg.lParam;
        }
    }

    handled = pThis->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, 0);
    ATLASSERT(pThis->m_pCurrentMsg == &msg);
    if (handled == FALSE)
    {
        if (uMsg == WM_NCDESTROY)
        {
            saveWindowProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
            lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
            if (saveWindowProc == reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC)))
                ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
            pThis->m_dwState |= WINSTATE_DESTROYED;
        }
        else
            lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
    }
    pThis->m_pCurrentMsg = previousMessage;
    if (previousMessage == NULL && (pThis->m_dwState & WINSTATE_DESTROYED) != 0)
    {
        pThis->m_dwState &= ~WINSTATE_DESTROYED;
        pThis->m_hWnd = NULL;
        pThis->OnFinalMessage(hWnd);
    }
    return lResult;
}

void CShellBrowser::RepositionBars()
{
    RECT                                    clientRect;
    RECT                                    statusRect;
    int                                     x;

    GetClientRect(&clientRect);

    if (m_settings.fStatusBarVisible && fStatusBar)
    {
        ::GetWindowRect(fStatusBar, &statusRect);
        ::SetWindowPos(fStatusBar, NULL, clientRect.left, clientRect.bottom - (statusRect.bottom - statusRect.top),
                            clientRect.right - clientRect.left,
                            statusRect.bottom - statusRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
        clientRect.bottom -= statusRect.bottom - statusRect.top;
    }

    for (x = 0; x < 3; x++)
    {
        HWND hwnd = fClientBars[x].hwnd;
        RECT borderSpace = fClientBars[x].borderSpace;
        if (hwnd == NULL && fClientBars[x].clientBar != NULL)
        {
            IUnknown_GetWindow(fClientBars[x].clientBar, &hwnd);
            fClientBars[x].hwnd = hwnd;
        }
        if (hwnd != NULL)
        {
            RECT toolbarRect = clientRect;
            if (borderSpace.top != 0)
            {
                toolbarRect.bottom = toolbarRect.top + borderSpace.top;
            }
            else if (borderSpace.bottom != 0)
            {
                toolbarRect.top = toolbarRect.bottom - borderSpace.bottom;
            }
            else if (borderSpace.left != 0)
            {
                toolbarRect.right = toolbarRect.left + borderSpace.left;
            }
            else if (borderSpace.right != 0)
            {
                toolbarRect.left = toolbarRect.right - borderSpace.right;
            }

            ::SetWindowPos(hwnd, NULL,
                toolbarRect.left,
                toolbarRect.top,
                toolbarRect.right - toolbarRect.left,
                toolbarRect.bottom - toolbarRect.top,
                SWP_NOOWNERZORDER | SWP_NOZORDER);

            if (borderSpace.top != 0)
            {
                clientRect.top = toolbarRect.bottom;
            }
            else if (borderSpace.bottom != 0)
            {
                clientRect.bottom = toolbarRect.top;
            }
            else if (borderSpace.left != 0)
            {
                clientRect.left = toolbarRect.right;
            }
            else if (borderSpace.right != 0)
            {
                clientRect.right = toolbarRect.left;
            }
        }
    }
    ::SetWindowPos(fCurrentShellViewWindow, NULL, clientRect.left, clientRect.top,
                        clientRect.right - clientRect.left,
                        clientRect.bottom - clientRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
}

HRESULT CShellBrowser::FireEvent(DISPID dispIdMember, int argCount, VARIANT *arguments)
{
    DISPPARAMS                          params;
    CComDynamicUnkArray                 &vec = IConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents2>::m_vec;
    CComDynamicUnkArray                 &vec2 = IConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents>::m_vec;
    HRESULT                             hResult;

    params.rgvarg = arguments;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = argCount;
    params.cNamedArgs = 0;
    IUnknown** pp = vec.begin();
    while (pp < vec.end())
    {
        if (*pp != NULL)
        {
            CComPtr<IDispatch>          theDispatch;

            hResult = (*pp)->QueryInterface(IID_PPV_ARG(IDispatch, &theDispatch));
            hResult = theDispatch->Invoke(dispIdMember, GUID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, NULL);
        }
        pp++;
    }
    pp = vec2.begin();
    while (pp < vec2.end())
    {
        if (*pp != NULL)
        {
            CComPtr<IDispatch>          theDispatch;

            hResult = (*pp)->QueryInterface(IID_PPV_ARG(IDispatch, &theDispatch));
            hResult = theDispatch->Invoke(dispIdMember, GUID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, NULL);
        }
        pp++;
    }
    return S_OK;
}

HRESULT CShellBrowser::FireNavigateComplete(const wchar_t *newDirectory)
{
    // these two variants intentionally to do use CComVariant because it would double free/release
    // or does not need to dispose at all
    VARIANT                             varArg[2];
    VARIANT                             varArgs;
    CComBSTR                            tempString(newDirectory);

    V_VT(&varArgs) = VT_BSTR;
    V_BSTR(&varArgs) = tempString.m_str;

    V_VT(&varArg[0]) = VT_VARIANT | VT_BYREF;
    V_VARIANTREF(&varArg[0]) = &varArgs;
    V_VT(&varArg[1]) = VT_DISPATCH;
    V_DISPATCH(&varArg[1]) = (IDispatch *)this;

    return FireEvent(DISPID_NAVIGATECOMPLETE2, 2, varArg);
}

HRESULT CShellBrowser::FireCommandStateChange(bool newState, int commandID)
{
    VARIANT                             varArg[2];

    V_VT(&varArg[0]) = VT_BOOL;
    V_BOOL(&varArg[0]) = newState ? VARIANT_TRUE : VARIANT_FALSE;
    V_VT(&varArg[1]) = VT_I4;
    V_I4(&varArg[1]) = commandID;

    return FireEvent(DISPID_COMMANDSTATECHANGE, 2, varArg);
}

HRESULT CShellBrowser::FireCommandStateChangeAll()
{
    return FireCommandStateChange(false, -1);
}

HRESULT CShellBrowser::UpdateForwardBackState()
{
    CComPtr<ITravelLog>                     travelLog;
    CComPtr<ITravelEntry>                   unusedEntry;
    bool                                    canGoBack;
    bool                                    canGoForward;
    HRESULT                                 hResult;

    canGoBack = false;
    canGoForward = false;
    hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = travelLog->GetTravelEntry(static_cast<IDropTarget *>(this), TLOG_BACK, &unusedEntry);
    if (SUCCEEDED(hResult))
    {
        canGoBack = true;
        unusedEntry.Release();
    }
    hResult = travelLog->GetTravelEntry(static_cast<IDropTarget *>(this), TLOG_FORE, &unusedEntry);
    if (SUCCEEDED(hResult))
    {
        canGoForward = true;
        unusedEntry.Release();
    }
    hResult = FireCommandStateChange(canGoBack, 2);
    hResult = FireCommandStateChange(canGoForward, 1);
    return S_OK;
}

HRESULT CShellBrowser::UpdateUpState()
{
    bool canGoUp;
    HRESULT hResult;

    canGoUp = true;
    if (_ILIsDesktop(fCurrentDirectoryPIDL))
        canGoUp = false;
    hResult = FireCommandStateChange(canGoUp, 3);
    return S_OK;
}

void CShellBrowser::UpdateGotoMenu(HMENU theMenu)
{
    CComPtr<ITravelLog>                     travelLog;
    CComPtr<ITravelEntry>                   unusedEntry;
    int                                     position;
    MENUITEMINFO                            menuItemInfo;
    HRESULT                                 hResult;

    DeleteMenuItems(theMenu, IDM_GOTO_TRAVEL_FIRST, IDM_GOTO_TRAVEL_LAST);

    position = GetMenuItemCount(theMenu);
    hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return;

    hResult = travelLog->GetTravelEntry(static_cast<IDropTarget *>(this),
                                        TLOG_BACK,
                                        &unusedEntry);

    if (SUCCEEDED(hResult))
    {
        SHEnableMenuItem(theMenu, IDM_GOTO_BACK, TRUE);
        unusedEntry.Release();
    }
    else
        SHEnableMenuItem(theMenu, IDM_GOTO_BACK, FALSE);

    hResult = travelLog->GetTravelEntry(static_cast<IDropTarget *>(this),
                                        TLOG_FORE,
                                        &unusedEntry);

    if (SUCCEEDED(hResult))
    {
        SHEnableMenuItem(theMenu, IDM_GOTO_FORWARD, TRUE);
        unusedEntry.Release();
    }
    else
        SHEnableMenuItem(theMenu, IDM_GOTO_FORWARD, FALSE);

    SHEnableMenuItem(theMenu,
                     IDM_GOTO_UPONELEVEL,
                     !_ILIsDesktop(fCurrentDirectoryPIDL));

    hResult = travelLog->InsertMenuEntries(static_cast<IDropTarget *>(this), theMenu, position,
        IDM_GOTO_TRAVEL_FIRSTTARGET, IDM_GOTO_TRAVEL_LASTTARGET, TLMENUF_BACKANDFORTH | TLMENUF_CHECKCURRENT);
    if (SUCCEEDED(hResult))
    {
        menuItemInfo.cbSize = sizeof(menuItemInfo);
        menuItemInfo.fMask = MIIM_TYPE | MIIM_ID;
        menuItemInfo.fType = MF_SEPARATOR;
        menuItemInfo.wID = IDM_GOTO_TRAVEL_SEP;
        InsertMenuItem(theMenu, position, TRUE, &menuItemInfo);
    }
}

void CShellBrowser::UpdateViewMenu(HMENU theMenu)
{
    CComPtr<ITravelLog>                     travelLog;
    HMENU                                   gotoMenu;
    OLECMD                                  commandList[5];
    HMENU                                   toolbarMenuBar;
    HMENU                                   toolbarMenu;
    MENUITEMINFO                            menuItemInfo;
    HRESULT                                 hResult;

    gotoMenu = SHGetMenuFromID(theMenu, FCIDM_MENU_EXPLORE);
    if (gotoMenu != NULL)
        UpdateGotoMenu(gotoMenu);

    commandList[0].cmdID = ITID_TOOLBARBANDSHOWN;
    commandList[1].cmdID = ITID_ADDRESSBANDSHOWN;
    commandList[2].cmdID = ITID_LINKSBANDSHOWN;
    commandList[3].cmdID = ITID_TOOLBARLOCKED;
    commandList[4].cmdID = ITID_CUSTOMIZEENABLED;

    hResult = IUnknown_QueryStatus(fClientBars[BIInternetToolbar].clientBar,
                                   CGID_PrivCITCommands, 5, commandList, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        DeleteMenu(theMenu, IDM_VIEW_TOOLBARS, MF_BYCOMMAND);
    else
    {
        menuItemInfo.cbSize = sizeof(menuItemInfo);
        menuItemInfo.fMask = MIIM_SUBMENU;
        GetMenuItemInfo(theMenu, IDM_VIEW_TOOLBARS, FALSE, &menuItemInfo);
        DestroyMenu(menuItemInfo.hSubMenu);

        toolbarMenuBar = LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDM_CABINET_CONTEXTMENU));
        toolbarMenu = GetSubMenu(toolbarMenuBar, 0);
        RemoveMenu(toolbarMenuBar, 0, MF_BYPOSITION);
        DestroyMenu(toolbarMenuBar);

        // TODO: Implement
        SHEnableMenuItem(toolbarMenu, IDM_TOOLBARS_STANDARDBUTTONS, commandList[0].cmdf & OLECMDF_ENABLED);
        SHEnableMenuItem(toolbarMenu, IDM_TOOLBARS_ADDRESSBAR, commandList[1].cmdf & OLECMDF_ENABLED);
        SHEnableMenuItem(toolbarMenu, IDM_TOOLBARS_LINKSBAR, commandList[2].cmdf & OLECMDF_ENABLED);
        SHEnableMenuItem(toolbarMenu, IDM_TOOLBARS_CUSTOMIZE, commandList[4].cmdf & OLECMDF_ENABLED);

        SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_STANDARDBUTTONS, commandList[0].cmdf & OLECMDF_LATCHED);
        SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_ADDRESSBAR, commandList[1].cmdf & OLECMDF_LATCHED);
        SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_LINKSBAR, commandList[2].cmdf & OLECMDF_LATCHED);
        SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_LOCKTOOLBARS, commandList[3].cmdf & OLECMDF_LATCHED);
        if ((commandList[4].cmdf & OLECMDF_ENABLED) == 0)
            DeleteMenu(toolbarMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
        DeleteMenu(toolbarMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
        DeleteMenu(toolbarMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);

        menuItemInfo.cbSize = sizeof(menuItemInfo);
        menuItemInfo.fMask = MIIM_SUBMENU;
        menuItemInfo.hSubMenu = toolbarMenu;
        SetMenuItemInfo(theMenu, IDM_VIEW_TOOLBARS, FALSE, &menuItemInfo);
    }
    SHCheckMenuItem(theMenu, IDM_VIEW_STATUSBAR, m_settings.fStatusBarVisible ? TRUE : FALSE);

    // Check the menu items for Explorer bar
    BOOL bSearchBand = (IsEqualCLSID(CLSID_SH_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_IE_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_FileSearchBand, fCurrentVertBar));
    BOOL bHistory = IsEqualCLSID(CLSID_SH_HistBand, fCurrentVertBar);
    BOOL bFavorites = IsEqualCLSID(CLSID_SH_FavBand, fCurrentVertBar);
    BOOL bFolders = IsEqualCLSID(CLSID_ExplorerBand, fCurrentVertBar);
    SHCheckMenuItem(theMenu, IDM_EXPLORERBAR_SEARCH, bSearchBand);
    SHCheckMenuItem(theMenu, IDM_EXPLORERBAR_HISTORY, bHistory);
    SHCheckMenuItem(theMenu, IDM_EXPLORERBAR_FAVORITES, bFavorites);
    SHCheckMenuItem(theMenu, IDM_EXPLORERBAR_FOLDERS, bFolders);
}

HRESULT CShellBrowser::BuildExplorerBandMenu()
{
    HMENU                                   hBandsMenu;
    UINT                                    nbFound;

    hBandsMenu = SHGetMenuFromID(fCurrentMenuBar, IDM_VIEW_EXPLORERBAR);
    if (!hBandsMenu)
    {
        OutputDebugString(L"No menu !\n");
        return E_FAIL;
    }
    DSA_DeleteAllItems(menuDsa);
    BuildExplorerBandCategory(hBandsMenu, CATID_InfoBand, 4, NULL);
    BuildExplorerBandCategory(hBandsMenu, CATID_CommBand, 20, &nbFound);
    if (!nbFound)
    {
        // Remove separator
        DeleteMenu(hBandsMenu, IDM_EXPLORERBAR_SEPARATOR, MF_BYCOMMAND);
    }
    // Remove media menu since XP does it (according to API Monitor)
    DeleteMenu(hBandsMenu, IDM_EXPLORERBAR_MEDIA, MF_BYCOMMAND);
    return S_OK;
}

HRESULT CShellBrowser::BuildExplorerBandCategory(HMENU hBandsMenu, CATID category, DWORD dwPos, UINT *nbFound)
{
    HRESULT                                 hr;
    CComPtr<IEnumGUID>                      pEnumGUID;
    WCHAR                                   wszBandName[MAX_PATH];
    WCHAR                                   wszBandGUID[MAX_PATH];
    WCHAR                                   wRegKey[MAX_PATH];
    UINT                                    cBands;
    DWORD                                   dwRead;
    DWORD                                   dwDataSize;
    GUID                                    iter;
    MenuBandInfo                            mbi;

    mbi.fVertical = IsEqualGUID(category, CATID_InfoBand);
    cBands = 0;
    hr = SHEnumClassesOfCategories(1, &category, 0, NULL, &pEnumGUID);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        return hr;
    }
    do
    {
        pEnumGUID->Next(1, &iter, &dwRead);
        if (dwRead)
        {
            // Get the band name
            if (IsBuiltinBand(iter))
                continue;
            if (!StringFromGUID2(iter, wszBandGUID, MAX_PATH))
                continue;
            StringCchPrintfW(wRegKey, MAX_PATH, L"CLSID\\%s", wszBandGUID);
            dwDataSize = MAX_PATH;
            SHGetValue(HKEY_CLASSES_ROOT, wRegKey, NULL, NULL, wszBandName, &dwDataSize);

            mbi.barGuid = iter;
            InsertMenu(hBandsMenu, dwPos + cBands, MF_BYPOSITION, IDM_EXPLORERBAND_BEGINCUSTOM + DSA_GetItemCount(menuDsa), wszBandName);
            DSA_AppendItem(menuDsa, &mbi);
            cBands++;
        }
    }
    while (dwRead > 0);
    if (nbFound)
        *nbFound = cBands;
    return S_OK;
}

BOOL CShellBrowser::IsBuiltinBand(CLSID &bandID)
{
    if (IsEqualCLSID(bandID, CLSID_ExplorerBand))
        return TRUE;
    if (IsEqualCLSID(bandID, CLSID_SH_SearchBand) || IsEqualCLSID(bandID, CLSID_SearchBand))
        return TRUE;
    if (IsEqualCLSID(bandID, CLSID_IE_SearchBand) || IsEqualCLSID(bandID, CLSID_FileSearchBand))
        return TRUE;
    if (IsEqualCLSID(bandID, CLSID_SH_HistBand))
        return TRUE;
    if (IsEqualCLSID(bandID, CLSID_SH_FavBand))
        return TRUE;
    if (IsEqualCLSID(bandID, CLSID_ChannelsBand))
        return TRUE;
    return FALSE;
}

HRESULT CShellBrowser::OnSearch()
{
    CComPtr<IObjectWithSite>                objectWithSite;
    CComPtr<IContextMenu>                   contextMenu;
    CMINVOKECOMMANDINFO                     commandInfo;
    const char                              *searchGUID = "{169A0691-8DF9-11d1-A1C4-00C04FD75D13}";
    HRESULT                                 hResult;

    // TODO: Query shell if this command is enabled first

    memset(&commandInfo, 0, sizeof(commandInfo));
    commandInfo.cbSize = sizeof(commandInfo);
    commandInfo.hwnd = m_hWnd;
    commandInfo.lpParameters = searchGUID;
    commandInfo.nShow = SW_SHOWNORMAL;

    hResult = CoCreateInstance(CLSID_ShellSearchExt, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IContextMenu, &contextMenu));
    if (FAILED_UNEXPECTEDLY(hResult))
        return 0;
    hResult = contextMenu->QueryInterface(IID_PPV_ARG(IObjectWithSite, &objectWithSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return 0;
    hResult = objectWithSite->SetSite(dynamic_cast<IShellBrowser*>(this));
    if (FAILED_UNEXPECTEDLY(hResult))
        return 0;
    hResult = contextMenu->InvokeCommand(&commandInfo);
    hResult = objectWithSite->SetSite(NULL);
    return hResult;
}

bool IUnknownIsEqual(IUnknown *int1, IUnknown *int2)
{
    CComPtr<IUnknown>                       int1Retry;
    CComPtr<IUnknown>                       int2Retry;
    HRESULT                                 hResult;

    if (int1 == int2)
        return true;
    if (int1 == NULL || int2 == NULL)
        return false;
    hResult = int1->QueryInterface(IID_PPV_ARG(IUnknown, &int1Retry));
    if (FAILED_UNEXPECTEDLY(hResult))
        return false;
    hResult = int2->QueryInterface(IID_PPV_ARG(IUnknown, &int2Retry));
    if (FAILED_UNEXPECTEDLY(hResult))
        return false;
    if (int1Retry == int2Retry)
        return true;
    return false;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetBorderDW(IUnknown *punkObj, LPRECT prcBorder)
{
    static const INT excludeItems[] = { 1, 1, 1, IDC_STATUSBAR, 0, 0 };

    RECT availableBounds;

    GetEffectiveClientRect(m_hWnd, &availableBounds, excludeItems);
    for (INT x = 0; x < 3; x++)
    {
        if (fClientBars[x].clientBar.p != NULL && !IUnknownIsEqual(fClientBars[x].clientBar, punkObj))
        {
            availableBounds.top += fClientBars[x].borderSpace.top;
            availableBounds.left += fClientBars[x].borderSpace.left;
            availableBounds.bottom -= fClientBars[x].borderSpace.bottom;
            availableBounds.right -= fClientBars[x].borderSpace.right;
        }
    }
    *prcBorder = availableBounds;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    for (INT x = 0; x < 3; x++)
    {
        if (IUnknownIsEqual(fClientBars[x].clientBar, punkObj))
        {
            fClientBars[x].borderSpace = *pbw;
            // if this bar changed size, it cascades and forces all subsequent bars to resize
            RepositionBars();
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    CComPtr<IOleCommandTarget>              commandTarget;
    HRESULT                                 hResult;

    if (prgCmds == NULL)
        return E_INVALIDARG;
    if (pguidCmdGroup == NULL)
    {
        if (fCurrentShellView.p != NULL)
        {
            hResult = fCurrentShellView->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &commandTarget));
            if (SUCCEEDED(hResult) && commandTarget.p != NULL)
                return commandTarget->QueryStatus(NULL, 1, prgCmds, pCmdText);
        }
        while (cCmds != 0)
        {
            prgCmds->cmdf = 0;
            prgCmds++;
            cCmds--;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
        while (cCmds != 0)
        {
            switch (prgCmds->cmdID)
            {
                case 0x1c:  // search
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsEqualCLSID(CLSID_SH_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_IE_SearchBand, fCurrentVertBar) ||
                        IsEqualCLSID(CLSID_FileSearchBand, fCurrentVertBar))
                    {
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    }
                    break;
                case 0x1d:  // history
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsEqualCLSID(CLSID_SH_HistBand, fCurrentVertBar))
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                case 0x1e:  // favorites
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsEqualCLSID(CLSID_SH_FavBand, fCurrentVertBar))
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                case SBCMDID_EXPLORERBARFOLDERS:  // folders
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsEqualCLSID(CLSID_ExplorerBand, fCurrentVertBar))
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                default:
                    prgCmds->cmdf = 0;
                    break;
            }
            prgCmds++;
            cCmds--;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_ShellBrowser))
    {
        while (cCmds != 0)
        {
            switch (prgCmds->cmdID)
            {
                case IDM_GOTO_UPONELEVEL:
                    prgCmds->cmdf = OLECMDF_SUPPORTED;
                    if (fCurrentDirectoryPIDL->mkid.cb != 0)
                        prgCmds->cmdf |= OLECMDF_ENABLED;
                    break;
            }
            prgCmds++;
            cCmds--;
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HRESULT                                 hResult;

    if (!pguidCmdGroup)
    {
        TRACE("Unhandled null CGID %d %d %p %p\n", nCmdID, nCmdexecopt, pvaIn, pvaOut);
        return E_NOTIMPL;
    }
    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
        switch (nCmdID)
        {
            case 0x1c: //Toggle Search
            case 0x1d: //Toggle History
            case 0x1e: //Toggle Favorites
            case SBCMDID_EXPLORERBARFOLDERS: //Toggle Folders
                const GUID* pclsid;
                if (nCmdID == 0x1c) pclsid = &CLSID_FileSearchBand;
                else if (nCmdID == 0x1d) pclsid = &CLSID_SH_HistBand;
                else if (nCmdID == 0x1e) pclsid = &CLSID_SH_FavBand;
                else pclsid = &CLSID_ExplorerBand;

                if (IsEqualCLSID(*pclsid, fCurrentVertBar))
                {
                    hResult = IUnknown_ShowDW(fClientBars[BIVerticalBaseBar].clientBar.p, FALSE);
                    memset(&fCurrentVertBar, 0, sizeof(fCurrentVertBar));
                    FireCommandStateChangeAll();
                }
                else
                {
                    hResult = ShowBand(*pclsid, true);
                }
                return S_OK;
            case 0x22:
                //Sent when a band closes
                if (V_VT(pvaIn) != VT_UNKNOWN)
                    return E_INVALIDARG;

                if (IUnknownIsEqual(V_UNKNOWN(pvaIn), fClientBars[BIVerticalBaseBar].clientBar.p))
                {
                    memset(&fCurrentVertBar, 0, sizeof(fCurrentVertBar));
                    FireCommandStateChangeAll();
                }
                return S_OK;
            case 0x27:
                if (nCmdexecopt == 1)
                {
                    // pvaIn is a VT_UNKNOWN with a band that is being hidden
                }
                else
                {
                    // update zones part of the status bar
                }
                return S_OK;
            case 0x35: // don't do this, and the internet toolbar doesn't create a menu band
                V_VT(pvaOut) = VT_INT_PTR;
                V_INTREF(pvaOut) = reinterpret_cast<INT *>(
                    LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDM_CABINET_MAINMENU)));
                return S_OK;
            case 0x38:
                // indicate if this cabinet was opened as a browser
                return S_FALSE;
            default:
                return E_NOTIMPL;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_InternetButtons))
    {
        switch (nCmdID)
        {
            case 0x23:
                // placeholder
                return S_OK;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_Theater))
    {
        switch (nCmdID)
        {
            case 6:
                // what is theater mode and why do we receive this?
                return E_NOTIMPL;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_MenuBand))
    {
        switch (nCmdID)
        {
            case 14:
                // initialize favorites menu
                return S_OK;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_ShellDocView))
    {
        switch (nCmdID)
        {
            case 0x12:
                // refresh on toolbar clicked
                return S_OK;
            case 0x26:
                // called for unknown bands ?
                return S_OK;
            case 0x4d:
                // tell the view if it should hide the task pane or not
                return (fClientBars[BIVerticalBaseBar].clientBar.p == NULL) ? S_FALSE : S_OK;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_ShellBrowser))
    {
        switch (nCmdID)
        {
            case 40994:
                return NavigateToParent();
            case IDM_NOTIFYITBARDIRTY:
                SaveITBarLayout();
                break;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_IExplorerToolbar))
    {
        switch (nCmdID)
        {
            case 0x7063:
                return DoFolderOptions();
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_DefView))
    {
        switch (nCmdID)
        {
            case DVCMDID_RESET_DEFAULTFOLDER_SETTINGS:
                ApplyBrowserDefaultFolderSettings(NULL);
                IUnknown_Exec(fCurrentShellView, CGID_DefView, nCmdID, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
                break;
        }
    }
    else
    {
        return E_NOTIMPL;
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    HMENU mainMenu = LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDM_CABINET_MAINMENU));

    Shell_MergeMenus(hmenuShared, mainMenu, 0, 0, FCIDM_BROWSERLAST, MM_SUBMENUSHAVEIDS);

    int GCCU(itemCount3) = GetMenuItemCount(hmenuShared);
    Unused(itemCount3);

    DestroyMenu(mainMenu);

    lpMenuWidths->width[0] = 2;
    lpMenuWidths->width[2] = 3;
    lpMenuWidths->width[4] = 1;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
    CComPtr<IShellMenu>                     shellMenu;
    HRESULT                                 hResult;

    if (hmenuShared && IsMenu(hmenuShared) == FALSE)
        return E_FAIL;
    hResult = GetMenuBand(IID_PPV_ARG(IShellMenu, &shellMenu));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (!hmenuShared)
    {
        hmenuShared = LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDM_CABINET_MAINMENU));
    }
    // FIXME: Figure out the proper way to do this.
    HMENU hMenuFavs = GetSubMenu(hmenuShared, 3);
    if (hMenuFavs)
    {
        DeleteMenu(hMenuFavs, IDM_FAVORITES_EMPTY, MF_BYCOMMAND);
    }

    hResult = shellMenu->SetMenu(hmenuShared, m_hWnd, SMSET_DONTOWN);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    fCurrentMenuBar = hmenuShared;
    BuildExplorerBandMenu();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::RemoveMenusSB(HMENU hmenuShared)
{
    if (hmenuShared == fCurrentMenuBar)
    {
        //DestroyMenu(fCurrentMenuBar);
        SetMenuSB(NULL, NULL, NULL);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetStatusTextSB(LPCOLESTR pszStatusText)
{
    //
    if (pszStatusText)
    {
        ::SetWindowText(fStatusBar, pszStatusText);
    }
    else
    {

    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::EnableModelessSB(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::TranslateAcceleratorSB(MSG *pmsg, WORD wID)
{
    if (!::TranslateAcceleratorW(m_hWnd, m_hAccel, pmsg))
        return S_FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    wFlags = ApplyNewBrowserFlag(wFlags);
    // FIXME: Should not automatically show the Explorer band
    if ((wFlags & SBSP_EXPLOREMODE) && !(wFlags & SBSP_NEWBROWSER))
        ShowBand(CLSID_ExplorerBand, true);

    CComHeapPtr<ITEMIDLIST> pidlResolved;
    if (wFlags & (SBSP_RELATIVE | SBSP_PARENT))
    {
        HRESULT hr = CreateRelativeBrowsePIDL(pidl, wFlags, &pidlResolved);
        if (FAILED(hr))
            return hr;
        pidl = pidlResolved;
    }

    if (wFlags & SBSP_NEWBROWSER)
        return OpenNewBrowserWindow(pidl, wFlags);

    switch (wFlags & (SBSP_ABSOLUTE | SBSP_RELATIVE | SBSP_PARENT | SBSP_NAVIGATEBACK | SBSP_NAVIGATEFORWARD))
    {
        case SBSP_PARENT:
            return NavigateToParent();
        case SBSP_NAVIGATEBACK:
            return GoBack();
        case SBSP_NAVIGATEFORWARD:
            return GoForward();
    }

    // TODO: SBSP_WRITENOHISTORY? SBSP_CREATENOHISTORY?
    long flags = BTP_UPDATE_NEXT_HISTORY;
    if (fTravelLog)
        flags |= BTP_UPDATE_CUR_HISTORY;
    if (wFlags & SBSP_ACTIVATE_NOFOCUS)
        flags |= BTP_ACTIVATE_NOFOCUS;
    return BrowseToPIDL(pidl, flags);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetViewStateStream(DWORD grfMode, IStream **ppStrm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetControlWindow(UINT id, HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = NULL;
    switch (id)
    {
        case FCW_TOOLBAR:
            *lphwnd = fToolbarProxy.m_hWnd;
            return S_OK;
        case FCW_STATUS:
            *lphwnd = fStatusBar;
            return S_OK;
        case FCW_TREE:
        {
            BOOL shown;
            if (SUCCEEDED(IsControlWindowShown(id, &shown)) && shown)
                return IUnknown_GetWindow(fClientBars[BIVerticalBaseBar].clientBar.p, lphwnd);
            return S_FALSE;
        }
        case FCW_PROGRESS:
            // is this a progress dialog?
            return S_OK;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SendControlMsg(
    UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    LPARAM                                  result;

    if (pret != NULL)
        *pret = 0;
    switch (id)
    {
        case FCW_TOOLBAR:
            result = fToolbarProxy.SendMessage(uMsg, wParam, lParam);
            if (pret != NULL)
                *pret = result;
            break;
        case FCW_STATUS:
            result = SendMessage(fStatusBar, uMsg, wParam, lParam);
            if (pret != NULL)
                *pret = result;
            break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryActiveShellView(IShellView **ppshv)
{
    if (ppshv == NULL)
        return E_POINTER;
    *ppshv = fCurrentShellView;
    if (fCurrentShellView.p != NULL)
    {
        fCurrentShellView.p->AddRef();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnViewWindowActive(IShellView *ppshv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::DragEnter(
    IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::DragLeave()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Drop(
    IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    // view does a query for SID_STopLevelBrowser, IID_IShellBrowserService
    // the returned interface has a method GetPropertyBag on it
    if (IsEqualIID(guidService, SID_STopLevelBrowser))
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_SShellBrowser))
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_ITargetFrame2))
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_IWebBrowserApp))        // without this, the internet toolbar won't reflect notifications
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_SProxyBrowser))
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_IExplorerToolbar) && fClientBars[BIInternetToolbar].clientBar.p)
        return fClientBars[BIInternetToolbar].clientBar->QueryInterface(riid, ppvObject);
    if (IsEqualIID(riid, IID_IShellBrowser))
        return this->QueryInterface(riid, ppvObject);
    return E_NOINTERFACE;
}

static BOOL _ILIsNetworkPlace(LPCITEMIDLIST pidl)
{
    WCHAR szPath[MAX_PATH];
    return SHGetPathFromIDListWrapW(pidl, szPath) && PathIsUNCW(szPath);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPropertyBag(long flags, REFIID riid, void **ppvObject)
{
    if (ppvObject == NULL)
        return E_POINTER;

    *ppvObject = NULL;

    LPITEMIDLIST pidl;
    HRESULT hr = GetPidl(&pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_FAIL;

    // FIXME: pidl for Internet etc.

    if (_ILIsNetworkPlace(pidl))
        flags |= SHGVSPB_ROAM;

    hr = SHGetViewStatePropertyBag(pidl, L"Shell", flags, riid, ppvObject);

    ILFree(pidl);
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetParentSite(IOleInPlaceSite **ppipsite)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetTitle(IShellView *psv, LPCWSTR pszName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetOleObject(IOleObject **ppobjv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetTravelLog(ITravelLog **pptl)
{
    HRESULT                                 hResult;

    // called by toolbar when displaying tooltips
    if (pptl == NULL)
        return E_FAIL;

    *pptl = NULL;
    if (fTravelLog.p == NULL)
    {
        hResult = CTravelLog_CreateInstance(IID_PPV_ARG(ITravelLog, &fTravelLog));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    *pptl = fTravelLog.p;
    fTravelLog.p->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ShowControlWindow(UINT id, BOOL fShow)
{
    BOOL shown;
    if (FAILED(IsControlWindowShown(id, &shown)))
        return E_NOTIMPL;
    else if (!shown == !fShow) // Negated for true boolean comparison
        return S_OK;
    else switch (id)
    {
        case FCW_STATUS:
            OnToggleStatusBarVisible(0, 0, NULL, shown);
            return S_OK;
        case FCW_TREE:
            return Exec(&CGID_Explorer, SBCMDID_EXPLORERBARFOLDERS, 0, NULL, NULL);
        case FCW_ADDRESSBAR:
            return IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                                 CGID_PrivCITCommands, ITID_ADDRESSBANDSHOWN, 0, NULL, NULL);
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    HRESULT hr = S_OK;
    BOOL shown = FALSE;
    switch (id)
    {
        case FCW_STATUS:
            shown = m_settings.fStatusBarVisible;
            break;
        case FCW_TREE:
        {
            OLECMD cmd = { SBCMDID_EXPLORERBARFOLDERS };
            hr = QueryStatus(&CGID_Explorer, 1, &cmd, NULL);
            shown = cmd.cmdf & OLECMDF_LATCHED;
            break;
        }
        case FCW_ADDRESSBAR:
            hr = IsInternetToolbarBandShown(ITID_ADDRESSBANDSHOWN);
            shown = hr == S_OK;
            break;
        default:
            hr = E_NOTIMPL;
    }
    if (pfShown)
    {
        *pfShown = shown;
        return hr;
    }
    return SUCCEEDED(hr) ? (shown ? S_OK : S_FALSE) : hr;
}

HRESULT CShellBrowser::IsInternetToolbarBandShown(UINT ITId)
{
    OLECMD cmd = { ITId };
    HRESULT hr = IUnknown_QueryStatus(fClientBars[BIInternetToolbar].clientBar,
                                      CGID_PrivCITCommands, 1, &cmd, NULL);
    return SUCCEEDED(hr) ? (cmd.cmdf & OLECMDF_LATCHED) ? S_OK : S_FALSE : hr;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::DisplayParseError(HRESULT hres, LPCWSTR pwszPath)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    return _NavigateToPidl(pidl, grfHLNF, 0);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetNavigateState(BNSTATE bnstate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetNavigateState(BNSTATE *pbnstate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::NotifyRedirect(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::UpdateWindowList()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::UpdateBackForwardState()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetFlags(DWORD dwFlags, DWORD dwFlagMask)
{
    m_BrowserSvcFlags = (m_BrowserSvcFlags & ~dwFlagMask) | (dwFlags & dwFlagMask);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetFlags(DWORD *pdwFlags)
{
    *pdwFlags = m_BrowserSvcFlags;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::CanNavigateNow()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPidl(LPITEMIDLIST *ppidl)
{
    // called by explorer bar to get current pidl
    return ppidl ? SHILClone(fCurrentDirectoryPIDL, ppidl) : E_POINTER;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetReferrer(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

DWORD STDMETHODCALLTYPE CShellBrowser::GetBrowserIndex()
{
    return -1;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetBrowserByIndex(DWORD dwID, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetHistoryObject(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc)
{
    if (ppole == NULL || pstm == NULL || ppbc == NULL)
        return E_INVALIDARG;
    *ppole = fHistoryObject;
    if (fHistoryObject != NULL)
        fHistoryObject->AddRef();
    *pstm = fHistoryStream;
    if (fHistoryStream != NULL)
        fHistoryStream->AddRef();
    *ppbc = fHistoryBindContext;
    if (fHistoryBindContext != NULL)
        fHistoryBindContext->AddRef();
    fHistoryObject = NULL;
    fHistoryStream = NULL;
    fHistoryBindContext = NULL;
    if (*ppole == NULL)
        return E_FAIL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::CacheOLEServer(IOleObject *pole)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetSetCodePage(VARIANT *pvarIn, VARIANT *pvarOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnHttpEquiv(
    IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPalette(HPALETTE *hpal)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::RegisterWindow(BOOL fForceRegister, int swc)
{
    return E_NOTIMPL;
}

LRESULT STDMETHODCALLTYPE CShellBrowser::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetAsDefFolderSettings()
{
    HRESULT hr = E_FAIL;
    if (fCurrentShellView)
    {
        hr = ApplyBrowserDefaultFolderSettings(fCurrentShellView);
        IUnknown_Exec(fCurrentShellView, CGID_DefView, DVCMDID_SET_DEFAULTFOLDER_SETTINGS, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetViewRect(RECT *prc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnSize(WPARAM wParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnCreate(struct tagCREATESTRUCTW *pcs)
{
    m_hAccel = LoadAcceleratorsW(GetModuleHandle(L"browseui.dll"), MAKEINTRESOURCEW(256));
    return S_OK;
}

LRESULT STDMETHODCALLTYPE CShellBrowser::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnDestroy()
{
    return E_NOTIMPL;
}

LRESULT STDMETHODCALLTYPE CShellBrowser::OnNotify(struct tagNMHDR *pnm)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnSetFocus()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnFrameWindowActivateBS(BOOL fActive)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ReleaseShellView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ActivatePendingView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::CreateViewWindow(
    IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::CreateBrowserPropSheetExt(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetViewWindow(HWND *phwndView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetBaseBrowserData(LPCBASEBROWSERDATA *pbbd)
{
    return E_NOTIMPL;
}

LPBASEBROWSERDATA STDMETHODCALLTYPE CShellBrowser::PutBaseBrowserData()
{
    return NULL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::InitializeTravelLog(ITravelLog *ptl, DWORD dw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetTopBrowser()
{
    m_BrowserSvcFlags |= BSF_TOPBROWSER;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Offline(int iCmd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::AllowViewResize(BOOL f)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetActivateState(UINT u)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::UpdateSecureLockIcon(int eSecureLock)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::InitializeDownloadManager()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::InitializeTransitionSite()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_Initialize(HWND hwnd, IUnknown *pauto)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_CancelPendingNavigationAsync()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_CancelPendingView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_MaySaveChanges()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_PauseOrResumeView(BOOL fPaused)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_DisableModeless()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags)
{
    const UINT navflags = HLNF_NAVIGATINGBACK | HLNF_NAVIGATINGFORWARD;
    if ((grfHLNF & navflags) && grfHLNF != ~0ul)
    {
        UINT SbspFlags = (grfHLNF & HLNF_NAVIGATINGBACK) ? SBSP_NAVIGATEBACK : SBSP_NAVIGATEFORWARD;
        if (grfHLNF & SHHLNF_WRITENOHISTORY)
            SbspFlags |= SBSP_WRITENOHISTORY;
        if (grfHLNF & SHHLNF_NOAUTOSELECT)
            SbspFlags |= SBSP_NOAUTOSELECT;
        return BrowseObject(pidl, SbspFlags);
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_TryShell2Rename(IShellView *psv, LPCITEMIDLIST pidlNew)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_SwitchActivationNow()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast,
    const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_SendChildren(
    HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetFolderSetData(struct tagFolderSetData *pfsd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_OnFocusChange(UINT itb)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::v_ShowHideChildWindows(BOOL fChildOnly)
{
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CShellBrowser::_get_itbLastFocus()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_put_itbLastFocus(UINT itbLastFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_UIActivateView(UINT uState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_GetViewBorderRect(RECT *prc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_UpdateViewRectSize()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_ResizeNextBorder(UINT itb)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_ResizeView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
    return E_NOTIMPL;
}

IStream *STDMETHODCALLTYPE CShellBrowser::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName)
{
    return NULL;
}

LRESULT STDMETHODCALLTYPE CShellBrowser::ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetAcceleratorMenu(HACCEL hacc)
{
    return E_NOTIMPL;
}

int STDMETHODCALLTYPE CShellBrowser::_GetToolbarCount()
{
    return 0;
}

LPTOOLBARITEM STDMETHODCALLTYPE CShellBrowser::_GetToolbarItem(int itb)
{
    return NULL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_SaveToolbars(IStream *pstm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_LoadToolbars(IStream *pstm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_CloseAndReleaseToolbars(BOOL fClose)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::v_MayGetNextToolbarFocus(
    LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM *pptbi, HWND *phwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor)
{
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CShellBrowser::_FindTBar(IUnknown *punkSrc)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::v_MayTranslateAccelerator(MSG *pmsg)
{
    for (int i = 0; i < 3; i++)
    {
        if (IUnknown_TranslateAcceleratorIO(fClientBars[i].clientBar, pmsg) == S_OK)
            return S_OK;
    }

    if (!fCurrentShellView)
        return S_FALSE;

    return fCurrentShellView->TranslateAcceleratorW(pmsg);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_GetBorderDWHelper(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::v_CheckZoneCrossing(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoBack()
{
    CComPtr<ITravelLog> travelLog;
    HRESULT hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return travelLog->Travel(static_cast<IDropTarget *>(this), TLOG_BACK);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoForward()
{
    CComPtr<ITravelLog> travelLog;
    HRESULT hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return travelLog->Travel(static_cast<IDropTarget *>(this), TLOG_FORE);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoHome()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoSearch()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Navigate(BSTR URL, VARIANT *Flags,
    VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hResult;
    CComPtr<IShellFolder> pDesktop;

    hResult = SHGetDesktopFolder(&pDesktop);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = pDesktop->ParseDisplayName(NULL, NULL, URL, NULL, &pidl, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return BrowseObject(pidl, 1);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Refresh()
{
    VARIANT                                 level;

    V_VT(&level) = VT_I4;
    V_I4(&level) = 4;
    return Refresh2(&level);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Refresh2(VARIANT *Level)
{
    CComPtr<IOleCommandTarget>              oleCommandTarget;
    HRESULT                                 hResult;

    hResult = fCurrentShellView->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &oleCommandTarget));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return oleCommandTarget->Exec(NULL, 22, 1, Level, NULL);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Stop()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Application(IDispatch **ppDisp)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Parent(IDispatch **ppDisp)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Container(IDispatch **ppDisp)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Document(IDispatch **ppDisp)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_TopLevelContainer(VARIANT_BOOL *pBool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Type(BSTR *Type)
{
    return E_NOTIMPL;
}
#ifdef __exdisp_h__
#define long LONG
#endif
HRESULT STDMETHODCALLTYPE CShellBrowser::get_Left(long *pl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Left(long Left)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Top(long *pl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Top(long Top)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Width(long *pl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Width(long Width)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Height(long *pl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Height(long Height)
{
    return E_NOTIMPL;
}
#ifdef __exdisp_h__
#undef long
#endif
HRESULT STDMETHODCALLTYPE CShellBrowser::get_LocationName(BSTR *LocationName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_LocationURL(BSTR *LocationURL)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Busy(VARIANT_BOOL *pBool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Quit()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ClientToWindow(int *pcx, int *pcy)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::PutProperty(BSTR Property, VARIANT vtValue)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetProperty(BSTR Property, VARIANT *pvtValue)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Name(BSTR *Name)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_HWND(SHANDLE_PTR *pHWND)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_FullName(BSTR *FullName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Path(BSTR *Path)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Visible(VARIANT_BOOL *pBool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Visible(VARIANT_BOOL Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_StatusBar(VARIANT_BOOL *pBool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_StatusBar(VARIANT_BOOL Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_StatusText(BSTR *StatusText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_StatusText(BSTR StatusText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_ToolBar(int *Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_ToolBar(int Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_MenuBar(VARIANT_BOOL *Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_MenuBar(VARIANT_BOOL Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_FullScreen(VARIANT_BOOL *pbFullScreen)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_FullScreen(VARIANT_BOOL bFullScreen)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Navigate2(VARIANT *URL, VARIANT *Flags,
    VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hResult;
    // called from drive combo box to navigate to a directory
    // Also called by search band to display shell results folder view

    if (V_VT(URL) == VT_BSTR)
    {
        return this->Navigate(V_BSTR(URL), Flags, TargetFrameName, PostData, Headers);
    }
    if (V_VT(URL) == (VT_ARRAY | VT_UI1))
    {
        if (V_ARRAY(URL)->cDims != 1 || V_ARRAY(URL)->cbElements != 1)
            return E_INVALIDARG;

        pidl = static_cast<LPITEMIDLIST>(V_ARRAY(URL)->pvData);
    }
    hResult = BrowseToPIDL(pidl, BTP_UPDATE_CUR_HISTORY | BTP_UPDATE_NEXT_HISTORY);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryStatusWB(OLECMDID cmdID, OLECMDF *pcmdf)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt,
    VARIANT *pvaIn, VARIANT *pvaOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ShowBrowserBar(VARIANT *pvaClsid, VARIANT *pvarShow, VARIANT *pvarSize)
{
    CLSID                                   classID;
    bool                                    vertical;

    // called to show search bar
    if (V_VT(pvaClsid) != VT_BSTR)
        return E_INVALIDARG;
    CLSIDFromString(V_BSTR(pvaClsid), &classID);
    // TODO: properly compute the value of vertical
    vertical = true;
    return ShowBand(classID, vertical);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_ReadyState(READYSTATE *plReadyState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Offline(VARIANT_BOOL *pbOffline)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Offline(VARIANT_BOOL bOffline)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Silent(VARIANT_BOOL *pbSilent)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Silent(VARIANT_BOOL bSilent)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_RegisterAsBrowser(VARIANT_BOOL *pbRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_RegisterAsBrowser(VARIANT_BOOL bRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_RegisterAsDropTarget(VARIANT_BOOL *pbRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_RegisterAsDropTarget(VARIANT_BOOL bRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_TheaterMode(VARIANT_BOOL *pbRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_TheaterMode(VARIANT_BOOL bRegister)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_AddressBar(VARIANT_BOOL *Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_AddressBar(VARIANT_BOOL Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::get_Resizable(VARIANT_BOOL *Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::put_Resizable(VARIANT_BOOL Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::FindWindowByIndex(DWORD dwID, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetWindowData(IStream *pStream, LPWINDOWDATA pWinData)
{
    if (pWinData == NULL)
        return E_POINTER;

    pWinData->dwWindowID = -1;
    pWinData->uiCP = 0;
    pWinData->pidl = ILClone(fCurrentDirectoryPIDL);
    pWinData->lpszUrl = NULL;
    pWinData->lpszUrlLocation = NULL;
    pWinData->lpszTitle = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::LoadHistoryPosition(LPWSTR pszUrlLocation, DWORD dwPosition)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::LoadHistory(IStream *pStream, IBindCtx *pbc)
{
    CComPtr<IPersistHistory>                viewPersistHistory;
    CComPtr<IOleObject>                     viewHistoryObject;
    persistState                            oldState;
    ULONG                                   numRead;
    LPITEMIDLIST                            pidl;
    HRESULT                                 hResult;

    hResult = pStream->Read(&oldState, sizeof(oldState), &numRead);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    if (numRead != sizeof(oldState) || oldState.dwSize != sizeof(oldState))
        return E_FAIL;
    if (oldState.browseType != 2)
        return E_FAIL;
    pidl = static_cast<LPITEMIDLIST>(CoTaskMemAlloc(oldState.pidlSize));
    if (pidl == NULL)
        return E_OUTOFMEMORY;
    hResult = pStream->Read(pidl, oldState.pidlSize, &numRead);
    if (FAILED_UNEXPECTEDLY(hResult))
    {
        ILFree(pidl);
        return hResult;
    }
    if (numRead != oldState.pidlSize)
    {
        ILFree(pidl);
        return E_FAIL;
    }
    hResult = CoCreateInstance(oldState.persistClass, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IOleObject, &viewHistoryObject));
    fHistoryObject = viewHistoryObject;
    fHistoryStream = pStream;
    fHistoryBindContext = pbc;
    hResult = BrowseToPIDL(pidl, BTP_DONT_UPDATE_HISTORY);
    fHistoryObject = NULL;
    fHistoryStream = NULL;
    fHistoryBindContext = NULL;
    ILFree(pidl);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SaveHistory(IStream *pStream)
{
    CComPtr<IPersistHistory>                viewPersistHistory;
    persistState                            newState;
    HRESULT                                 hResult;

    hResult = fCurrentShellView->GetItemObject(
        SVGIO_BACKGROUND, IID_PPV_ARG(IPersistHistory, &viewPersistHistory));
    memset(&newState, 0, sizeof(newState));
    newState.dwSize = sizeof(newState);
    newState.browseType = 2;
    newState.browserIndex = GetBrowserIndex();
    if (viewPersistHistory.p != NULL)
    {
        hResult = viewPersistHistory->GetClassID(&newState.persistClass);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    newState.pidlSize = ILGetSize(fCurrentDirectoryPIDL);
    hResult = pStream->Write(&newState, sizeof(newState), NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = pStream->Write(fCurrentDirectoryPIDL, newState.pidlSize, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    if (viewPersistHistory.p != NULL)
    {
        hResult = viewPersistHistory->SaveHistory(pStream);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetPositionCookie(DWORD dwPositioncookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPositionCookie(DWORD *pdwPositioncookie)
{
    return E_NOTIMPL;
}

LRESULT CShellBrowser::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    OnCreate(reinterpret_cast<LPCREATESTRUCT> (lParam));
    return 0;
}

LRESULT CShellBrowser::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HRESULT hr;
    SaveViewState();

    /* The current thread is about to go down so render any IDataObject that may be left in the clipboard */
    OleFlushClipboard();

    // TODO: rip down everything
    {
        m_Destroyed = true; // Ignore browse requests from Explorer band TreeView during destruction
        fCurrentShellView->UIActivate(SVUIA_DEACTIVATE);
        fToolbarProxy.Destroy();
        fCurrentShellView->DestroyViewWindow();

        for (int i = 0; i < 3; i++)
        {
            CComPtr<IDockingWindow> pdw;
            CComPtr<IDeskBar> bar;
            CComPtr<IUnknown> pBarSite;

            if (fClientBars[i].clientBar == NULL)
                continue;

            hr = fClientBars[i].clientBar->QueryInterface(IID_PPV_ARG(IDockingWindow, &pdw));
            if (FAILED_UNEXPECTEDLY(hr))
                continue;

            /* We should destroy our basebarsite too */
            hr = pdw->QueryInterface(IID_PPV_ARG(IDeskBar, &bar));
            if (SUCCEEDED(hr))
            {
                hr = bar->GetClient(&pBarSite);
                if (SUCCEEDED(hr) && pBarSite)
                {
                    CComPtr<IDeskBarClient> pClient;
                    hr = pBarSite->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pClient));
                    if (SUCCEEDED(hr))
                        pClient->SetDeskBarSite(NULL);
                }
            }
            pdw->CloseDW(0);

            pBarSite = NULL;
            pdw = NULL;
            bar = NULL;
            ReleaseCComPtrExpectZero(fClientBars[i].clientBar);
        }
        ReleaseCComPtrExpectZero(fCurrentShellView);
        ReleaseCComPtrExpectZero(fTravelLog);

        fCurrentShellFolder.Release();
        ILFree(fCurrentDirectoryPIDL);
        ::DestroyWindow(fStatusBar);
        DestroyMenu(fCurrentMenuBar);
    }
    PostQuitMessage(0);
    return 0;
}

LRESULT CShellBrowser::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComPtr<IDockingWindow>                 dockingWindow;
    RECT                                    availableBounds;
    static const INT                        excludeItems[] = {1, 1, 1, IDC_STATUSBAR, 0, 0};
    HRESULT                                 hResult;

    if (wParam != SIZE_MINIMIZED)
    {
        GetEffectiveClientRect(m_hWnd, &availableBounds, excludeItems);
        for (INT x = 0; x < 3; x++)
        {
            if (fClientBars[x].clientBar)
            {
                hResult = fClientBars[x].clientBar->QueryInterface(
                    IID_PPV_ARG(IDockingWindow, &dockingWindow));
                if (SUCCEEDED(hResult) && dockingWindow != NULL)
                {
                    hResult = dockingWindow->ResizeBorderDW(
                        &availableBounds, static_cast<IDropTarget *>(this), TRUE);
                    break;
                }
            }
        }
        RepositionBars();
    }
    return 1;
}

LRESULT CShellBrowser::OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HMENU  theMenu;
    LPARAM menuIndex = lParam;

    theMenu = reinterpret_cast<HMENU>(wParam);

    if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_FILE))
    {
        menuIndex = 0;
    }
    else if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_EDIT))
    {
        menuIndex = 1;
    }
    else if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_VIEW))
    {
        UpdateViewMenu(theMenu);
        menuIndex = 2;
    }
    else if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_FAVORITES))
    {
        menuIndex = 3;
    }
    else if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_TOOLS))
    {
        // FIXME: Remove once implemented
        SHEnableMenuItem(theMenu, IDM_TOOLS_MAPNETWORKDRIVE, FALSE);
        SHEnableMenuItem(theMenu, IDM_TOOLS_SYNCHRONIZE, FALSE);
        menuIndex = 4;
    }
    else if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_HELP))
    {
        menuIndex = 5;
    }

    LRESULT ret = RelayMsgToShellView(uMsg, wParam, menuIndex, bHandled);

    return ret;
}

LRESULT CShellBrowser::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    ::SetFocus(fCurrentShellViewWindow);
    return 0;
}

LRESULT CShellBrowser::RelayMsgToShellView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (fCurrentShellViewWindow != NULL)
        return SendMessage(fCurrentShellViewWindow, uMsg, wParam, lParam);
    return 0;
}

LRESULT CShellBrowser::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RefreshCabinetState();
    SHPropagateMessage(m_hWnd, uMsg, wParam, lParam, TRUE);
    return 0;
}

LRESULT CShellBrowser::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    SHPropagateMessage(m_hWnd, uMsg, wParam, lParam, TRUE);
    return 0;
}

LRESULT CShellBrowser::OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    return SendMessage(WM_CLOSE);
}

LRESULT CShellBrowser::OnFolderOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult = DoFolderOptions();
    if (FAILED(hResult))
        TRACE("DoFolderOptions failed with hResult=%08lx\n", hResult);
    return 0;
}

LRESULT CShellBrowser::OnMapNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
#ifndef __REACTOS__
    WNetConnectionDialog(m_hWnd, RESOURCETYPE_DISK);
#endif /* __REACTOS__ */
    return 0;
}

LRESULT CShellBrowser::OnDisconnectNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    WNetDisconnectDialog(m_hWnd, RESOURCETYPE_DISK);
    return 0;
}

LRESULT CShellBrowser::OnAboutReactOS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    ShellAbout(m_hWnd, _T("ReactOS"), NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnGoBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult = GoBack();
    if (FAILED(hResult))
        TRACE("GoBack failed with hResult=%08lx\n", hResult);
    return 0;
}

LRESULT CShellBrowser::OnGoForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult = GoForward();
    if (FAILED(hResult))
        TRACE("GoForward failed with hResult=%08lx\n", hResult);
    return 0;
}

LRESULT CShellBrowser::OnGoUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult = NavigateToParent();
    if (FAILED(hResult))
        TRACE("NavigateToParent failed with hResult=%08lx\n", hResult);
    return 0;
}

LRESULT CShellBrowser::OnGoHome(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult = GoHome();
    if (FAILED(hResult))
        TRACE("GoHome failed with hResult=%08lx\n", hResult);
    return 0;
}

LRESULT CShellBrowser::OnBackspace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    // FIXME: This does not appear to be what windows does.
    HRESULT hResult = NavigateToParent();
    if (FAILED(hResult))
        TRACE("NavigateToParent failed with hResult=%08lx\n", hResult);
    return 0;
}

static BOOL
CreateShortcut(
    IN LPCWSTR pszLnkFileName,
    IN LPCITEMIDLIST pidl,
    IN LPCWSTR pszDescription OPTIONAL)
{
    IPersistFile *pPF;
    IShellLinkW *pSL;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkW, (LPVOID*)&pSL);
    if (SUCCEEDED(hr))
    {
        pSL->SetIDList(pidl);

        if (pszDescription)
            pSL->SetDescription(pszDescription);

        hr = pSL->QueryInterface(IID_IPersistFile, (LPVOID*)&pPF);
        if (SUCCEEDED(hr))
        {
            hr = pPF->Save(pszLnkFileName, TRUE);
            pPF->Release();
        }
        pSL->Release();
    }

    CoUninitialize();

    return SUCCEEDED(hr);
}

HRESULT GetFavsLocation(HWND hWnd, LPITEMIDLIST *pPidl)
{
    HRESULT hr = SHGetSpecialFolderLocation(hWnd, CSIDL_FAVORITES, pPidl);
    if (FAILED(hr))
        hr = SHGetSpecialFolderLocation(hWnd, CSIDL_COMMON_FAVORITES, pPidl);

    return hr;
}

LRESULT CShellBrowser::OnAddToFavorites(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    LPITEMIDLIST pidlFavs;
    HRESULT hr = GetFavsLocation(m_hWnd, &pidlFavs);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    SHFILEINFOW fileInfo = { NULL };
    if (!SHGetFileInfoW((LPCWSTR)fCurrentDirectoryPIDL, 0, &fileInfo, sizeof(fileInfo),
                        SHGFI_PIDL | SHGFI_DISPLAYNAME))
    {
        return 0;
    }

    WCHAR szPath[MAX_PATH];
    SHGetPathFromIDListW(pidlFavs, szPath);
    PathAppendW(szPath, fileInfo.szDisplayName);
    PathAddExtensionW(szPath, L".lnk");

    CreateShortcut(szPath, fCurrentDirectoryPIDL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnOrganizeFavorites(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    CComPtr<IShellFolder> psfDesktop;
    LPITEMIDLIST pidlFavs;
    HRESULT hr = GetFavsLocation(m_hWnd, &pidlFavs);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ILFree(pidlFavs);
        return 0;
    }

    hr = SHInvokeDefaultCommand(m_hWnd, psfDesktop, pidlFavs);
    ILFree(pidlFavs);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    return 0;
}

LRESULT CShellBrowser::OnToggleStatusBarVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    m_settings.fStatusBarVisible = !m_settings.fStatusBarVisible;
    m_settings.Save();
    SendMessageW(BWM_SETTINGCHANGE, 0, (LPARAM)&m_settings);
    return 0;
}

LRESULT CShellBrowser::OnToggleToolbarLock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_TOOLBARLOCKED, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnToggleToolbarBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_TOOLBARBANDSHOWN, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnToggleAddressBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_ADDRESSBANDSHOWN, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnToggleLinksBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_LINKSBANDSHOWN, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnToggleTextLabels(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_TEXTLABELS, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnToolbarCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT hResult;
    hResult = IUnknown_Exec(fClientBars[BIInternetToolbar].clientBar,
                            CGID_PrivCITCommands, ITID_CUSTOMIZEENABLED, 0, NULL, NULL);
    return 0;
}

LRESULT CShellBrowser::OnRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    if (fCurrentShellView)
        fCurrentShellView->Refresh();
    return 0;
}

LRESULT CShellBrowser::OnGoTravel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    return 0;
}

LRESULT CShellBrowser::OnExplorerBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    // TODO: HACK ! use the proper mechanism to show the band (i.e. pass the BSTR to basebar)
    if (wID >= IDM_EXPLORERBAND_BEGINCUSTOM && wID <= IDM_EXPLORERBAND_ENDCUSTOM)
    {
        MenuBandInfo *mbi;
        mbi = (MenuBandInfo*)DSA_GetItemPtr(menuDsa, (wID - IDM_EXPLORERBAND_BEGINCUSTOM));
        if (!mbi)
            return 0;
        ShowBand(mbi->barGuid, mbi->fVertical);
        bHandled = TRUE;
        return 1;
    }
    switch (wID)
    {
    case IDM_EXPLORERBAR_SEARCH:
        ShowBand(CLSID_FileSearchBand, true);
        break;
    case IDM_EXPLORERBAR_FOLDERS:
        ShowBand(CLSID_ExplorerBand, true);
        break;
    case IDM_EXPLORERBAR_HISTORY:
        ShowBand(CLSID_SH_HistBand, true);
        break;
    case IDM_EXPLORERBAR_FAVORITES:
        ShowBand(CLSID_SH_FavBand, true);
        break;
    default:
        WARN("Unknown id %x\n", wID);
    }
    bHandled = TRUE;
    return 1;
}

LRESULT CShellBrowser::RelayCommands(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (HIWORD(wParam) == 0 && LOWORD(wParam) < FCIDM_SHVIEWLAST && fCurrentShellViewWindow != NULL)
        return SendMessage(fCurrentShellViewWindow, uMsg, wParam, lParam);
    return 0;
}

LRESULT CShellBrowser::OnCabinetStateChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RefreshCabinetState();
    return 0;
}

LRESULT CShellBrowser::OnSettingsChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* Refresh child windows */
    ::SendMessageW(fClientBars[BIInternetToolbar].hwnd, uMsg, wParam, lParam);

    /* Refresh status bar */
    if (fStatusBar)
    {
        ::ShowWindow(fStatusBar, m_settings.fStatusBarVisible ? SW_SHOW : SW_HIDE);
        RepositionBars();
    }

    return 0;
}

LRESULT CShellBrowser::OnGetSettingsPtr(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!lParam)
        return ERROR_INVALID_PARAMETER;

    *(ShellSettings**)lParam = &m_settings;
    return NO_ERROR;
}

// WM_APPCOMMAND
LRESULT CShellBrowser::OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT uCmd = GET_APPCOMMAND_LPARAM(lParam);
    switch (uCmd)
    {
        case APPCOMMAND_BROWSER_BACKWARD:
            GoBack();
            break;

        case APPCOMMAND_BROWSER_FORWARD:
            GoForward();
            break;

        default:
            FIXME("uCmd: %u\n", uCmd);
            break;
    }
    return 0;
}

HRESULT CShellBrowser_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CShellBrowser>(riid, ppv);
}

void CShellBrowser::RefreshCabinetState()
{
    gCabinetState.Load();
    UpdateWindowTitle();
}

void CShellBrowser::UpdateWindowTitle()
{
    WCHAR title[MAX_PATH];
    SHGDNF flags = SHGDN_FORADDRESSBAR;

    if (gCabinetState.fFullPathTitle)
        flags |= SHGDN_FORPARSING;

    if (SUCCEEDED(IEGetNameAndFlags(fCurrentDirectoryPIDL, flags, title, _countof(title), NULL)))
        SetWindowText(title);
}

void CShellBrowser::SaveITBarLayout()
{
    if (!gCabinetState.fSaveLocalView || (m_BrowserSvcFlags & (BSF_THEATERMODE | BSF_UISETBYAUTOMATION)))
        return;
#if 0 // If CDesktopBrowser aggregates us, skip saving
    FOLDERSETTINGS fs;
    if (fCurrentShellView && SUCCEEDED(fCurrentShellView->GetCurrentInfo(&fs)) && (fs.fFlags & FWF_DESKTOP))
        return;
#endif

    CComPtr<IPersistStreamInit> pPSI;
    CComPtr<IStream> pITBarStream;
    if (!fClientBars[BIInternetToolbar].clientBar.p)
        return;
    HRESULT hr = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_PPV_ARG(IPersistStreamInit, &pPSI));
    if (FAILED(hr))
        return;
    if (FAILED(hr = CInternetToolbar::GetStream(ITBARSTREAM_EXPLORER, STGM_WRITE, &pITBarStream)))
        return;
    pPSI->Save(pITBarStream, TRUE);
}
