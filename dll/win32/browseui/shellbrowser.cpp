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
    bool                                    fStatusBarVisible;
    CToolbarProxy                           fToolbarProxy;
    barInfo                                 fClientBars[3];
    CComPtr<ITravelLog>                     fTravelLog;
    HMENU                                   fCurrentMenuBar;
    CABINETSTATE                            fCabinetState;
    GUID                                    fCurrentVertBar;             //The guid of the built in vertical bar that is being shown
    // The next three fields support persisted history for shell views.
    // They do not need to be reference counted.
    IOleObject                              *fHistoryObject;
    IStream                                 *fHistoryStream;
    IBindCtx                                *fHistoryBindContext;
    HDSA menuDsa;
    HACCEL m_hAccel;
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
    HRESULT BrowseToPIDL(LPCITEMIDLIST pidl, long flags);
    HRESULT BrowseToPath(IShellFolder *newShellFolder, LPCITEMIDLIST absolutePIDL,
        FOLDERSETTINGS *folderSettings, long flags);
    HRESULT GetMenuBand(REFIID riid, void **shellMenu);
    HRESULT GetBaseBar(bool vertical, REFIID riid, void **theBaseBar);
    BOOL IsBandLoaded(const CLSID clsidBand, bool verticali, DWORD *pdwBandID);
    HRESULT ShowBand(const CLSID &classID, bool vertical);
    HRESULT NavigateToParent();
    HRESULT DoFolderOptions();
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

/*    // *** IDockingWindowFrame methods ***
    virtual HRESULT STDMETHODCALLTYPE AddToolbar(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags);
    virtual HRESULT STDMETHODCALLTYPE RemoveToolbar(IUnknown *punkSrc, DWORD dwRemoveFlags);
    virtual HRESULT STDMETHODCALLTYPE FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppv);
    */

    // *** IDockingWindowSite methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkObj, LPRECT prcBorder);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
        OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IShellBrowser methods ***
    virtual HRESULT STDMETHODCALLTYPE InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    virtual HRESULT STDMETHODCALLTYPE SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenusSB(HMENU hmenuShared);
    virtual HRESULT STDMETHODCALLTYPE SetStatusTextSB(LPCOLESTR pszStatusText);
    virtual HRESULT STDMETHODCALLTYPE EnableModelessSB(BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorSB(MSG *pmsg, WORD wID);
    virtual HRESULT STDMETHODCALLTYPE BrowseObject(LPCITEMIDLIST pidl, UINT wFlags);
    virtual HRESULT STDMETHODCALLTYPE GetViewStateStream(DWORD grfMode, IStream **ppStrm);
    virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id, HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView **ppshv);
    virtual HRESULT STDMETHODCALLTYPE OnViewWindowActive(IShellView *ppshv);
    virtual HRESULT STDMETHODCALLTYPE SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // *** IDropTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IShellBowserService methods ***
    virtual HRESULT STDMETHODCALLTYPE GetPropertyBag(long flags, REFIID riid, void **ppvObject);

    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

    // *** IBrowserService methods ***
    virtual HRESULT STDMETHODCALLTYPE GetParentSite(IOleInPlaceSite **ppipsite);
    virtual HRESULT STDMETHODCALLTYPE SetTitle(IShellView *psv, LPCWSTR pszName);
    virtual HRESULT STDMETHODCALLTYPE GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName);
    virtual HRESULT STDMETHODCALLTYPE GetOleObject(IOleObject **ppobjv);
    virtual HRESULT STDMETHODCALLTYPE GetTravelLog(ITravelLog **pptl);
    virtual HRESULT STDMETHODCALLTYPE ShowControlWindow(UINT id, BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE IsControlWindowShown(UINT id, BOOL *pfShown);
    virtual HRESULT STDMETHODCALLTYPE IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags);
    virtual HRESULT STDMETHODCALLTYPE IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST *ppidlOut);
    virtual HRESULT STDMETHODCALLTYPE DisplayParseError(HRESULT hres, LPCWSTR pwszPath);
    virtual HRESULT STDMETHODCALLTYPE NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF);
    virtual HRESULT STDMETHODCALLTYPE SetNavigateState(BNSTATE bnstate);
    virtual HRESULT STDMETHODCALLTYPE GetNavigateState(BNSTATE *pbnstate);
    virtual HRESULT STDMETHODCALLTYPE NotifyRedirect(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse);
    virtual HRESULT STDMETHODCALLTYPE UpdateWindowList();
    virtual HRESULT STDMETHODCALLTYPE UpdateBackForwardState();
    virtual HRESULT STDMETHODCALLTYPE SetFlags(DWORD dwFlags, DWORD dwFlagMask);
    virtual HRESULT STDMETHODCALLTYPE GetFlags(DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE CanNavigateNow( void);
    virtual HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST *ppidl);
    virtual HRESULT STDMETHODCALLTYPE SetReferrer(LPCITEMIDLIST pidl);
    virtual DWORD STDMETHODCALLTYPE GetBrowserIndex();
    virtual HRESULT STDMETHODCALLTYPE GetBrowserByIndex(DWORD dwID, IUnknown **ppunk);
    virtual HRESULT STDMETHODCALLTYPE GetHistoryObject(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc);
    virtual HRESULT STDMETHODCALLTYPE SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor);
    virtual HRESULT STDMETHODCALLTYPE CacheOLEServer(IOleObject *pole);
    virtual HRESULT STDMETHODCALLTYPE GetSetCodePage(VARIANT *pvarIn, VARIANT *pvarOut);
    virtual HRESULT STDMETHODCALLTYPE OnHttpEquiv(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut);
    virtual HRESULT STDMETHODCALLTYPE GetPalette(HPALETTE *hpal);
    virtual HRESULT STDMETHODCALLTYPE RegisterWindow(BOOL fForceRegister, int swc);

    // *** IBrowserService2 methods ***
    virtual LRESULT STDMETHODCALLTYPE WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE SetAsDefFolderSettings();
    virtual HRESULT STDMETHODCALLTYPE GetViewRect(RECT *prc);
    virtual HRESULT STDMETHODCALLTYPE OnSize(WPARAM wParam);
    virtual HRESULT STDMETHODCALLTYPE OnCreate(struct tagCREATESTRUCTW *pcs);
    virtual LRESULT STDMETHODCALLTYPE OnCommand(WPARAM wParam, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE OnDestroy();
    virtual LRESULT STDMETHODCALLTYPE OnNotify(struct tagNMHDR *pnm);
    virtual HRESULT STDMETHODCALLTYPE OnSetFocus();
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivateBS(BOOL fActive);
    virtual HRESULT STDMETHODCALLTYPE ReleaseShellView();
    virtual HRESULT STDMETHODCALLTYPE ActivatePendingView();
    virtual HRESULT STDMETHODCALLTYPE CreateViewWindow(IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE CreateBrowserPropSheetExt(REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE GetViewWindow(HWND *phwndView);
    virtual HRESULT STDMETHODCALLTYPE GetBaseBrowserData(LPCBASEBROWSERDATA *pbbd);
    virtual LPBASEBROWSERDATA STDMETHODCALLTYPE PutBaseBrowserData( void);
    virtual HRESULT STDMETHODCALLTYPE InitializeTravelLog(ITravelLog *ptl, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE SetTopBrowser();
    virtual HRESULT STDMETHODCALLTYPE Offline(int iCmd);
    virtual HRESULT STDMETHODCALLTYPE AllowViewResize(BOOL f);
    virtual HRESULT STDMETHODCALLTYPE SetActivateState(UINT u);
    virtual HRESULT STDMETHODCALLTYPE UpdateSecureLockIcon(int eSecureLock);
    virtual HRESULT STDMETHODCALLTYPE InitializeDownloadManager();
    virtual HRESULT STDMETHODCALLTYPE InitializeTransitionSite();
    virtual HRESULT STDMETHODCALLTYPE _Initialize(HWND hwnd, IUnknown *pauto);
    virtual HRESULT STDMETHODCALLTYPE _CancelPendingNavigationAsync( void);
    virtual HRESULT STDMETHODCALLTYPE _CancelPendingView();
    virtual HRESULT STDMETHODCALLTYPE _MaySaveChanges();
    virtual HRESULT STDMETHODCALLTYPE _PauseOrResumeView(BOOL fPaused);
    virtual HRESULT STDMETHODCALLTYPE _DisableModeless();
    virtual HRESULT STDMETHODCALLTYPE _NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE _TryShell2Rename(IShellView *psv, LPCITEMIDLIST pidlNew);
    virtual HRESULT STDMETHODCALLTYPE _SwitchActivationNow();
    virtual HRESULT STDMETHODCALLTYPE _ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    virtual HRESULT STDMETHODCALLTYPE _SendChildren(
        HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE GetFolderSetData(struct tagFolderSetData *pfsd);
    virtual HRESULT STDMETHODCALLTYPE _OnFocusChange(UINT itb);
    virtual HRESULT STDMETHODCALLTYPE v_ShowHideChildWindows(BOOL fChildOnly);
    virtual UINT STDMETHODCALLTYPE _get_itbLastFocus();
    virtual HRESULT STDMETHODCALLTYPE _put_itbLastFocus(UINT itbLastFocus);
    virtual HRESULT STDMETHODCALLTYPE _UIActivateView(UINT uState);
    virtual HRESULT STDMETHODCALLTYPE _GetViewBorderRect(RECT *prc);
    virtual HRESULT STDMETHODCALLTYPE _UpdateViewRectSize();
    virtual HRESULT STDMETHODCALLTYPE _ResizeNextBorder(UINT itb);
    virtual HRESULT STDMETHODCALLTYPE _ResizeView();
    virtual HRESULT STDMETHODCALLTYPE _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);
    virtual IStream *STDMETHODCALLTYPE v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName);
    virtual LRESULT STDMETHODCALLTYPE ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE SetAcceleratorMenu(HACCEL hacc);
    virtual int STDMETHODCALLTYPE _GetToolbarCount();
    virtual LPTOOLBARITEM STDMETHODCALLTYPE _GetToolbarItem(int itb);
    virtual HRESULT STDMETHODCALLTYPE _SaveToolbars(IStream *pstm);
    virtual HRESULT STDMETHODCALLTYPE _LoadToolbars(IStream *pstm);
    virtual HRESULT STDMETHODCALLTYPE _CloseAndReleaseToolbars(BOOL fClose);
    virtual HRESULT STDMETHODCALLTYPE v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext,
        int citb, LPTOOLBARITEM *pptbi, HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor);
    virtual UINT STDMETHODCALLTYPE _FindTBar(IUnknown *punkSrc);
    virtual HRESULT STDMETHODCALLTYPE _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE v_MayTranslateAccelerator(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE _GetBorderDWHelper(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor);
    virtual HRESULT STDMETHODCALLTYPE v_CheckZoneCrossing(LPCITEMIDLIST pidl);

    // *** IWebBrowser methods ***
    virtual HRESULT STDMETHODCALLTYPE GoBack();
    virtual HRESULT STDMETHODCALLTYPE GoForward();
    virtual HRESULT STDMETHODCALLTYPE GoHome();
    virtual HRESULT STDMETHODCALLTYPE GoSearch();
    virtual HRESULT STDMETHODCALLTYPE Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName,
        VARIANT *PostData, VARIANT *Headers);
    virtual HRESULT STDMETHODCALLTYPE Refresh();
    virtual HRESULT STDMETHODCALLTYPE Refresh2(VARIANT *Level);
    virtual HRESULT STDMETHODCALLTYPE Stop();
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Container(IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Document(IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_TopLevelContainer(VARIANT_BOOL *pBool);
    virtual HRESULT STDMETHODCALLTYPE get_Type(BSTR *Type);
    virtual HRESULT STDMETHODCALLTYPE get_Left(long *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Left(long Left);
    virtual HRESULT STDMETHODCALLTYPE get_Top(long *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Top(long Top);
    virtual HRESULT STDMETHODCALLTYPE get_Width(long *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Width(long Width);
    virtual HRESULT STDMETHODCALLTYPE get_Height(long *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Height(long Height);
    virtual HRESULT STDMETHODCALLTYPE get_LocationName(BSTR *LocationName);
    virtual HRESULT STDMETHODCALLTYPE get_LocationURL(BSTR *LocationURL);
    virtual HRESULT STDMETHODCALLTYPE get_Busy(VARIANT_BOOL *pBool);

    // *** IWebBrowserApp methods ***
    virtual HRESULT STDMETHODCALLTYPE Quit();
    virtual HRESULT STDMETHODCALLTYPE ClientToWindow(int *pcx, int *pcy);
    virtual HRESULT STDMETHODCALLTYPE PutProperty(BSTR Property, VARIANT vtValue);
    virtual HRESULT STDMETHODCALLTYPE GetProperty(BSTR Property, VARIANT *pvtValue);
    virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *Name);
    virtual HRESULT STDMETHODCALLTYPE get_HWND(SHANDLE_PTR *pHWND);
    virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR *FullName);
    virtual HRESULT STDMETHODCALLTYPE get_Path(BSTR *Path);
    virtual HRESULT STDMETHODCALLTYPE get_Visible(VARIANT_BOOL *pBool);
    virtual HRESULT STDMETHODCALLTYPE put_Visible(VARIANT_BOOL Value);
    virtual HRESULT STDMETHODCALLTYPE get_StatusBar(VARIANT_BOOL *pBool);
    virtual HRESULT STDMETHODCALLTYPE put_StatusBar(VARIANT_BOOL Value);
    virtual HRESULT STDMETHODCALLTYPE get_StatusText(BSTR *StatusText);
    virtual HRESULT STDMETHODCALLTYPE put_StatusText(BSTR StatusText);
    virtual HRESULT STDMETHODCALLTYPE get_ToolBar(int *Value);
    virtual HRESULT STDMETHODCALLTYPE put_ToolBar(int Value);
    virtual HRESULT STDMETHODCALLTYPE get_MenuBar(VARIANT_BOOL *Value);
    virtual HRESULT STDMETHODCALLTYPE put_MenuBar(VARIANT_BOOL Value);
    virtual HRESULT STDMETHODCALLTYPE get_FullScreen(VARIANT_BOOL *pbFullScreen);
    virtual HRESULT STDMETHODCALLTYPE put_FullScreen(VARIANT_BOOL bFullScreen);

    // *** IWebBrowser2 methods ***
    virtual HRESULT STDMETHODCALLTYPE Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName,
        VARIANT *PostData, VARIANT *Headers);
    virtual HRESULT STDMETHODCALLTYPE QueryStatusWB(OLECMDID cmdID, OLECMDF *pcmdf);
    virtual HRESULT STDMETHODCALLTYPE ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt,
        VARIANT *pvaIn, VARIANT *pvaOut);
    virtual HRESULT STDMETHODCALLTYPE ShowBrowserBar(VARIANT *pvaClsid, VARIANT *pvarShow, VARIANT *pvarSize);
    virtual HRESULT STDMETHODCALLTYPE get_ReadyState(READYSTATE *plReadyState);
    virtual HRESULT STDMETHODCALLTYPE get_Offline(VARIANT_BOOL *pbOffline);
    virtual HRESULT STDMETHODCALLTYPE put_Offline(VARIANT_BOOL bOffline);
    virtual HRESULT STDMETHODCALLTYPE get_Silent(VARIANT_BOOL *pbSilent);
    virtual HRESULT STDMETHODCALLTYPE put_Silent(VARIANT_BOOL bSilent);
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsBrowser(VARIANT_BOOL *pbRegister);
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsBrowser(VARIANT_BOOL bRegister);
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsDropTarget(VARIANT_BOOL *pbRegister);
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsDropTarget(VARIANT_BOOL bRegister);
    virtual HRESULT STDMETHODCALLTYPE get_TheaterMode(VARIANT_BOOL *pbRegister);
    virtual HRESULT STDMETHODCALLTYPE put_TheaterMode(VARIANT_BOOL bRegister);
    virtual HRESULT STDMETHODCALLTYPE get_AddressBar(VARIANT_BOOL *Value);
    virtual HRESULT STDMETHODCALLTYPE put_AddressBar(VARIANT_BOOL Value);
    virtual HRESULT STDMETHODCALLTYPE get_Resizable(VARIANT_BOOL *Value);
    virtual HRESULT STDMETHODCALLTYPE put_Resizable(VARIANT_BOOL Value);

    // *** ITravelLogClient methods ***
    virtual HRESULT STDMETHODCALLTYPE FindWindowByIndex(DWORD dwID, IUnknown **ppunk);
    virtual HRESULT STDMETHODCALLTYPE GetWindowData(IStream *pStream, LPWINDOWDATA pWinData);
    virtual HRESULT STDMETHODCALLTYPE LoadHistoryPosition(LPWSTR pszUrlLocation, DWORD dwPosition);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistHistory methods ***
    virtual HRESULT STDMETHODCALLTYPE LoadHistory(IStream *pStream, IBindCtx *pbc);
    virtual HRESULT STDMETHODCALLTYPE SaveHistory(IStream *pStream);
    virtual HRESULT STDMETHODCALLTYPE SetPositionCookie(DWORD dwPositioncookie);
    virtual HRESULT STDMETHODCALLTYPE GetPositionCookie(DWORD *pdwPositioncookie);

    // message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT RelayMsgToShellView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
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
        COMMAND_ID_HANDLER(IDM_FILE_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDM_TOOLS_FOLDEROPTIONS, OnFolderOptions)
        COMMAND_ID_HANDLER(IDM_TOOLS_MAPNETWORKDRIVE, OnMapNetworkDrive)
        COMMAND_ID_HANDLER(IDM_TOOLS_DISCONNECTNETWORKDRIVE, OnDisconnectNetworkDrive)
        COMMAND_ID_HANDLER(IDM_HELP_ABOUT, OnAboutReactOS)
        COMMAND_ID_HANDLER(IDM_GOTO_BACK, OnGoBack)
        COMMAND_ID_HANDLER(IDM_GOTO_FORWARD, OnGoForward)
        COMMAND_ID_HANDLER(IDM_GOTO_UPONELEVEL, OnGoUpLevel)
        COMMAND_ID_HANDLER(IDM_GOTO_HOMEPAGE, OnGoHome)
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
    fCurrentShellViewWindow = NULL;
    fCurrentDirectoryPIDL = NULL;
    fStatusBar = NULL;
    fStatusBarVisible = true;
    fCurrentMenuBar = NULL;
    fHistoryObject = NULL;
    fHistoryStream = NULL;
    fHistoryBindContext = NULL;
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

    fCabinetState.cLength = sizeof(fCabinetState);
    if (ReadCabinetState(&fCabinetState, sizeof(fCabinetState)) == FALSE)
    {
    }

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

    // TODO: create settingsStream from registry entry
    //if (settingsStream.p)
    //{
    //    hResult = persistStreamInit->Load(settingsStream);
    //    if (FAILED_UNEXPECTEDLY(hResult))
    //        return hResult;
    //}
    //else
    {
        hResult = persistStreamInit->InitNew();
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }

    hResult = IUnknown_ShowDW(clientBar, TRUE);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    fToolbarProxy.Initialize(m_hWnd, clientBar);


    // create status bar
    fStatusBar = CreateWindow(STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
                    SBT_NOBORDERS | SBT_TOOLTIPS, 0, 0, 500, 20, m_hWnd, (HMENU)0xa001,
                    _AtlBaseModule.GetModuleInstance(), 0);
    fStatusBarVisible = true;


    ShowWindow(SW_SHOWNORMAL);
    UpdateWindow();

    return S_OK;
}

HRESULT CShellBrowser::BrowseToPIDL(LPCITEMIDLIST pidl, long flags)
{
    CComPtr<IShellFolder>                   newFolder;
    FOLDERSETTINGS                          newFolderSettings;
    HRESULT                                 hResult;

    // called by shell view to browse to new folder
    // also called by explorer band to navigate to new folder
    hResult = SHBindToFolder(pidl, &newFolder);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    newFolderSettings.ViewMode = FVM_ICON;
    newFolderSettings.fFlags = 0;
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

long IEGetNameAndFlags(LPITEMIDLIST pidl, SHGDNF uFlags, LPWSTR pszBuf, UINT cchBuf, SFGAOF *rgfInOut)
{
    return IEGetNameAndFlagsEx(pidl, uFlags, 0, pszBuf, cchBuf, rgfInOut);
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

    if (newShellFolder == NULL)
        return E_INVALIDARG;

    hResult = GetTravelLog(&travelLog);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // update history
    if (flags & BTP_UPDATE_CUR_HISTORY)
    {
        if (travelLog->CountEntries(static_cast<IDropTarget *>(this)) > 0)
            hResult = travelLog->UpdateEntry(static_cast<IDropTarget *>(this), FALSE);
        // what to do with error? Do we want to halt browse because state save failed?
    }

    if (fCurrentShellView)
    {
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
    fCurrentDirectoryPIDL = ILClone(absolutePIDL);

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

    hResult = newShellView->UIActivate(SVUIA_ACTIVATE_FOCUS);

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

    if (fCabinetState.fFullPathTitle)
        nameFlags = SHGDN_FORADDRESSBAR | SHGDN_FORPARSING;
    else
        nameFlags = SHGDN_FORADDRESSBAR;
    hResult = IEGetNameAndFlags(fCurrentDirectoryPIDL, nameFlags, newTitle,
        sizeof(newTitle) / sizeof(wchar_t), NULL);
    if (SUCCEEDED(hResult))
    {
        SetWindowText(newTitle);

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
        newBaseBar->AddRef();

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

    hResult = bandSite->EnumBands(-1, &numBands);
    if (FAILED_UNEXPECTEDLY(hResult))
        return FALSE;

    for(i = 0; i < numBands; i++)
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

// CORE-11140 : Disabled this bit, because it prevents the folder options from showing.
//              It returns 'E_NOTIMPL'
#if 0
    if (fCurrentShellView != NULL)
    {
        hResult = fCurrentShellView->AddPropertySheetPages(
            0, AddFolderOptionsPage, reinterpret_cast<LPARAM>(&m_PropSheet));
        if (FAILED_UNEXPECTEDLY(hResult))
            return E_FAIL;
    }
#endif

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
        menuBand.Release();
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

    if (fStatusBarVisible && fStatusBar)
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
    SHCheckMenuItem(theMenu, IDM_VIEW_STATUSBAR, fStatusBarVisible ? TRUE : FALSE);
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
    static const INT excludeItems[] = { 1, 1, 1, 0xa001, 0, 0 };

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
                case 0x23:  // folders
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
                case 0xa022:    // up level
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
            case 0x23: //Toggle Folders
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
            case 1:
                // Reset All Folders option in Folder Options
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
    if ((wFlags & SBSP_EXPLOREMODE) != NULL)
        ShowBand(CLSID_ExplorerBand, true);

    long flags = BTP_UPDATE_NEXT_HISTORY;
    if (fTravelLog)
        flags |= BTP_UPDATE_CUR_HISTORY;
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
            // find the directory browser and return it
            // this should be used only to determine if a tree is present
            return S_OK;
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
        fCurrentShellView.p->AddRef();
    return S_OK;
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
    if (IsEqualIID(guidService, SID_IExplorerToolbar))
        return fClientBars[BIInternetToolbar].clientBar->QueryInterface(riid, ppvObject);
    if (IsEqualIID(riid, IID_IShellBrowser))
        return this->QueryInterface(riid, ppvObject);
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPropertyBag(long flags, REFIID riid, void **ppvObject)
{
    if (ppvObject == NULL)
        return E_POINTER;
    *ppvObject = NULL;
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    return E_NOTIMPL;
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
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetFlags(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::CanNavigateNow()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPidl(LPITEMIDLIST *ppidl)
{
    // called by explorer bar to get current pidl
    if (ppidl == NULL)
        return E_POINTER;
    *ppidl = ILClone(fCurrentDirectoryPIDL);
    return S_OK;
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
    return E_NOTIMPL;
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
    return E_NOTIMPL;
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

    /* The current thread is about to go down so render any IDataObject that may be left in the clipboard */
    OleFlushClipboard();

    // TODO: rip down everything
    {
        fToolbarProxy.Destroy();

        fCurrentShellView->DestroyViewWindow();
        fCurrentShellView->UIActivate(SVUIA_DEACTIVATE);

        for (int i = 0; i < 3; i++)
        {
            CComPtr<IDockingWindow> pdw;
            CComPtr<IDeskBar> bar;
            CComPtr<IUnknown> pBarSite;
            CComPtr<IDeskBarClient> pClient;

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
                    hr = pBarSite->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pClient));
                    if (SUCCEEDED(hr))
                        pClient->SetDeskBarSite(NULL);
                }
            }
            pdw->CloseDW(0);

            pClient = NULL;
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
    static const INT                        excludeItems[] = {1, 1, 1, 0xa001, 0, 0};
    HRESULT                                 hResult;

    if (wParam != SIZE_MINIMIZED)
    {
        GetEffectiveClientRect(m_hWnd, &availableBounds, excludeItems);
        for (INT x = 0; x < 3; x++)
        {
            if (fClientBars[x].clientBar != NULL)
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

LRESULT CShellBrowser::OnOrganizeFavorites(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    CComPtr<IShellFolder> psfDesktop;
    LPITEMIDLIST pidlFavs;
    HRESULT hr;
    hr = SHGetSpecialFolderLocation(m_hWnd, CSIDL_FAVORITES, &pidlFavs);
    if (FAILED(hr))
    {
        hr = SHGetSpecialFolderLocation(m_hWnd, CSIDL_COMMON_FAVORITES, &pidlFavs);
        if (FAILED(hr))
            return 0;
    }

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    hr = SHInvokeDefaultCommand(m_hWnd, psfDesktop, pidlFavs);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    return 0;
}

LRESULT CShellBrowser::OnToggleStatusBarVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    fStatusBarVisible = !fStatusBarVisible;
    if (fStatusBar)
    {
        ::ShowWindow(fStatusBar, fStatusBarVisible ? SW_SHOW : SW_HIDE);
        RepositionBars();
    }
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

HRESULT CShellBrowser_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CShellBrowser>(riid, ppv);
}
