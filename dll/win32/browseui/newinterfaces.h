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

#include <shobjidl.h>

// these resources are in shell32.dll
#define IDB_GOBUTTON_NORMAL			0x0e6
#define IDB_GOBUTTON_HOT			0x0e7

// band ids in internet toolbar
#define ITBBID_MENUBAND				1
#define ITBBID_BRANDBAND			5
#define ITBBID_TOOLSBAND			2
#define ITBBID_ADDRESSBAND			4

// commands in the CGID_PrivCITCommands command group handled by the internet toolbar
// there seems to be some support for hiding the menubar and an auto hide feature that are
// unavailable in the UI
#define ITID_TEXTLABELS				3
#define ITID_TOOLBARBANDSHOWN		4
#define ITID_ADDRESSBANDSHOWN		5
#define ITID_LINKSBANDSHOWN			6
#define ITID_MENUBANDSHOWN			12
#define ITID_AUTOHIDEENABLED		13
#define ITID_CUSTOMIZEENABLED		20
#define ITID_TOOLBARLOCKED			27

// commands in the CGID_BrandCmdGroup command group handled by the brand band
#define BBID_STARTANIMATION			1
#define BBID_STOPANIMATION			2

// undocumented flags for IShellMenu::SetShellFolder
#define SMSET_UNKNOWN08				0x08
#define SMSET_UNKNOWN10				0x10

EXTERN_C const IID CLSID_SH_AddressBand;
EXTERN_C const IID CLSID_BrandBand;
EXTERN_C const IID CLSID_SearchBand;
EXTERN_C const IID CLSID_InternetToolbar;
EXTERN_C const IID IID_IExplorerToolbar;
EXTERN_C const IID CGID_PrivCITCommands;
EXTERN_C const IID CGID_ShellBrowser;
EXTERN_C const IID SID_IBandProxy;
EXTERN_C const IID CLSID_ShellSearchExt;
EXTERN_C const IID CLSID_CommonButtons;
EXTERN_C const IID CGID_BrandCmdGroup;
EXTERN_C const IID SID_SBrandBand;
EXTERN_C const IID CLSID_AddressEditBox;
EXTERN_C const IID IID_IDeskBand;
EXTERN_C const IID CLSID_MenuBand;
EXTERN_C const IID CLSID_InternetButtons;
EXTERN_C const IID CLSID_BandProxy;

#define CGID_IExplorerToolbar IID_IExplorerToolbar
#define SID_IExplorerToolbar IID_IExplorerToolbar
#define SID_ITargetFrame2 IID_ITargetFrame2
#define SID_IWebBrowserApp IID_IWebBrowserApp
#define CGID_IDeskBand IID_IDeskBand
#define CGID_MenuBand CLSID_MenuBand
#define CGID_InternetButtons CLSID_InternetButtons
#define SID_STravelLogCursor IID_ITravelLogStg 
#define SID_IBandSite IID_IBandSite 

struct persistState
{
	long									dwSize;
	long									browseType;
	long									alwaysZero;
	long									browserIndex;
	CLSID									persistClass;
	long									pidlSize;
};



EXTERN_C const IID IID_INscTree;

class INscTree : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CreateTree(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE Initialize(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE ShowWindow(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE Refresh() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedItem(long paramC, long param10) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSelectedItem(long paramC, long param10, long param14, long param18) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetNscMode(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNscMode(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedItemName(long paramC, long param10) = 0;
	virtual HRESULT STDMETHODCALLTYPE BindToSelectedItemParent(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE InLabelEdit() = 0;
	virtual HRESULT STDMETHODCALLTYPE RightPaneNavigationStarted(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE RightPaneNavigationFinished(long paramC) = 0;
};

EXTERN_C const IID IID_INscTree2;

class INscTree2 : public INscTree
{
public:
	virtual HRESULT STDMETHODCALLTYPE CreateTree2(long paramC, long param10, long param14, long param18) = 0;
};

EXTERN_C const IID IID_IAddressEditBox;

class IAddressEditBox : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Init(HWND comboboxEx, HWND editControl, long param14, IUnknown *param18) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCurrentDir(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE ParseNow(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE Execute(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE Save(long paramC) = 0;
};

EXTERN_C const IID IID_IBandProxy;

class IBandProxy : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateNewWindow(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBrowserWindow(IUnknown **paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsConnected() = 0;
	virtual HRESULT STDMETHODCALLTYPE NavigateToPIDL(LPCITEMIDLIST pidl) = 0;
	virtual HRESULT STDMETHODCALLTYPE NavigateToURL(long paramC, long param10) = 0;
};

EXTERN_C const IID IID_IExplorerToolbar;

class IExplorerToolbar : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetCommandTarget(IUnknown *theTarget, GUID *category, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unknown1() = 0;
	virtual HRESULT STDMETHODCALLTYPE AddButtons(const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddString(const GUID *pguidCmdGroup, HINSTANCE param10, LPCTSTR param14, long *param18) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetButton(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetState(const GUID *pguidCmdGroup, long commandID, long *theState) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetState(const GUID *pguidCmdGroup, long commandID, long theState) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddBitmap(const GUID *pguidCmdGroup, long param10, long buttonCount, TBADDBITMAP *lParam, long *newIndex, COLORREF param20) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBitmapSize(long *paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE SendToolbarMsg(const GUID *pguidCmdGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *result) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetImageList(const GUID *pguidCmdGroup, HIMAGELIST param10, HIMAGELIST param14, HIMAGELIST param18) = 0;
	virtual HRESULT STDMETHODCALLTYPE ModifyButton(long paramC, long param10, long param14) = 0;
};

EXTERN_C const IID IID_IBandNavigate;

class IBandNavigate : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Select(long paramC) = 0;
};

EXTERN_C const IID IID_INamespaceProxy;

class INamespaceProxy : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetNavigateTarget(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE Invoke(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnSelectionChanged(long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE RefreshFlags(long paramC, long param10, long param14) = 0;
	virtual HRESULT STDMETHODCALLTYPE CacheItem(long paramC) = 0;
};

EXTERN_C const IID IID_IShellMenu2;

class IShellMenu2 : public IShellMenu
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetSubMenu() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetToolbar() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMinWidth() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNoBorder() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTheme() = 0;
};

EXTERN_C const IID IID_IWinEventHandler;

class IWinEventHandler : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd) = 0;
};

EXTERN_C const IID IID_IAddressBand;

class IAddressBand : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE FileSysChange(long param8, long paramC) = 0;
	virtual HRESULT STDMETHODCALLTYPE Refresh(long param8) = 0;
};

EXTERN_C const IID IID_IShellMenuAcc;

class IShellMenuAcc : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetTop() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBottom() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTracked() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetParentSite() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetState() = 0;
	virtual HRESULT STDMETHODCALLTYPE DoDefaultAction() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSubMenu() = 0;
	virtual HRESULT STDMETHODCALLTYPE IsEmpty() = 0;
};

EXTERN_C const IID IID_IShellBrowserService;

class IShellBrowserService : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetPropertyBag(long flags, REFIID riid, void **ppvObject) = 0;
};

typedef struct _WINDOWDATA
{
	DWORD dwWindowID;
	UINT uiCP;
	LPITEMIDLIST pidl;				// absolute
	LPWSTR lpszUrl;
	LPWSTR lpszUrlLocation;
	LPWSTR lpszTitle;
} 	WINDOWDATA;

typedef WINDOWDATA *LPWINDOWDATA;

typedef const WINDOWDATA *LPCWINDOWDATA;

#define TLOG_BACK  -1 
#define TLOG_FORE   1 

#define TLMENUF_INCLUDECURRENT      0x00000001 
#define TLMENUF_CHECKCURRENT        (TLMENUF_INCLUDECURRENT | 0x00000002) 
#define TLMENUF_BACK                0x00000010  // Default 
#define TLMENUF_FORE                0x00000020 
#define TLMENUF_BACKANDFORTH        (TLMENUF_BACK | TLMENUF_FORE | TLMENUF_INCLUDECURRENT) 

EXTERN_C const IID IID_ITravelLogClient;

/*
Contrary to MSDN, it appears GetWindowData takes only one parameter, not two.
*/
class ITravelLogClient : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE FindWindowByIndex(DWORD dwID, IUnknown **ppunk) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWindowData(LPWINDOWDATA pWinData) = 0;
	virtual HRESULT STDMETHODCALLTYPE LoadHistoryPosition(LPWSTR pszUrlLocation, DWORD dwPosition) = 0;
};

EXTERN_C const IID IID_ITravelEntry;

class ITravelEntry : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Invoke(IUnknown *punk) = 0;
	virtual HRESULT STDMETHODCALLTYPE Update(IUnknown *punk, BOOL fIsLocalAnchor) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST *ppidl) = 0;
};

EXTERN_C const IID IID_ITravelLog;

class ITravelLog : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE AddEntry(IUnknown *punk, BOOL fIsLocalAnchor) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext) = 0;
	virtual HRESULT STDMETHODCALLTYPE Travel(IUnknown *punk, int iOffset) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte) = 0;
	virtual HRESULT STDMETHODCALLTYPE FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetToolTipText(IUnknown *punk, int iOffset, int idsTemplate, LPWSTR pwzText, DWORD cchText) = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertMenuEntries(IUnknown *punk, HMENU hmenu, int nPos, int idFirst, int idLast, DWORD dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE Clone(ITravelLog **pptl) = 0;
	virtual DWORD STDMETHODCALLTYPE CountEntries(IUnknown *punk) = 0;
	virtual HRESULT STDMETHODCALLTYPE Revert() = 0;
};

typedef enum tagBNSTATE
{
	BNS_NORMAL	= 0,
	BNS_BEGIN_NAVIGATE	= 1,
	BNS_NAVIGATE	= 2
} 	BNSTATE;

#pragma warning(push)
#pragma warning(disable:4214)
#include <pshpack8.h>
typedef struct basebrowserdataxp
    {
    HWND _hwnd;
    ITravelLog *_ptl;
    IUnknown *_phlf;
    IUnknown *_pautoWB2;
    IUnknown *_pautoEDS;
    IShellService *_pautoSS;
    int _eSecureLockIcon;
    DWORD _fCreatingViewWindow	: 1;
    UINT _uActivateState;
    LPITEMIDLIST _pidlViewState;
    IOleCommandTarget *_pctView;
    LPITEMIDLIST _pidlCur;
    IShellView *_psv;
    IShellFolder *_psf;
    HWND _hwndView;
    LPWSTR _pszTitleCur;
    LPITEMIDLIST _pidlPending;
    IShellView *_psvPending;
    IShellFolder *_psfPending;
    HWND _hwndViewPending;
    LPWSTR _pszTitlePending;
    BOOL _fIsViewMSHTML;
    BOOL _fPrivacyImpacted;
    CLSID _clsidView;
    CLSID _clsidViewPending;
    HWND _hwndFrame;
    } 	BASEBROWSERDATAXP;

typedef struct basebrowserdataxp *LPBASEBROWSERDATAXP;

typedef struct basebrowserdatalh
{
	HWND _hwnd;
	ITravelLog *_ptl;
	IUnknown *_phlf;
	IUnknown *_pautoWB2;
	IUnknown *_pautoEDS;
	IShellService *_pautoSS;
	int _eSecureLockIcon;
	DWORD _fCreatingViewWindow	: 1;
	UINT _uActivateState;
	LPITEMIDLIST _pidlViewState;
	IOleCommandTarget *_pctView;
	LPITEMIDLIST _pidlCur;
	IShellView *_psv;
	IShellFolder *_psf;
	HWND _hwndView;
	LPWSTR _pszTitleCur;
	LPITEMIDLIST _pidlPending;
	IShellView *_psvPending;
	IShellFolder *_psfPending;
	HWND _hwndViewPending;
	LPWSTR _pszTitlePending;
	BOOL _fIsViewMSHTML;
	BOOL _fPrivacyImpacted;
	CLSID _clsidView;
	CLSID _clsidViewPending;
	HWND _hwndFrame;
	LONG _lPhishingFilterStatus;
} 	BASEBROWSERDATALH;
#pragma warning(pop)

typedef struct basebrowserdatalh *LPBASEBROWSERDATALH;

typedef BASEBROWSERDATAXP BASEBROWSERDATA;

typedef const BASEBROWSERDATA *LPCBASEBROWSERDATA;

typedef BASEBROWSERDATA *LPBASEBROWSERDATA;

typedef struct SToolbarItem
{
	IDockingWindow *ptbar;
	BORDERWIDTHS rcBorderTool;
	LPWSTR pwszItem;
	BOOL fShow;
	HMONITOR hMon;
} 	TOOLBARITEM;

typedef struct SToolbarItem *LPTOOLBARITEM;

EXTERN_C const IID IID_IBrowserService;

class IBrowserService : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetParentSite(IOleInPlaceSite **ppipsite) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTitle(IShellView *psv, LPCWSTR pszName) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOleObject(IOleObject **ppobjv) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTravelLog(ITravelLog **pptl) = 0;
	virtual HRESULT STDMETHODCALLTYPE ShowControlWindow(UINT id, BOOL fShow) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsControlWindowShown(UINT id, BOOL *pfShown) = 0;
	virtual HRESULT STDMETHODCALLTYPE IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPCITEMIDLIST *ppidlOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE DisplayParseError(HRESULT hres, LPCWSTR pwszPath) = 0;
	virtual HRESULT STDMETHODCALLTYPE NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNavigateState(BNSTATE bnstate) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetNavigateState(BNSTATE *pbnstate) = 0;
	virtual HRESULT STDMETHODCALLTYPE NotifyRedirect(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateWindowList() = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateBackForwardState() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetFlags(DWORD dwFlags, DWORD dwFlagMask) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFlags(DWORD *pdwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanNavigateNow() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidl(LPCITEMIDLIST *ppidl) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetReferrer(LPCITEMIDLIST pidl) = 0;
	virtual DWORD STDMETHODCALLTYPE GetBrowserIndex() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBrowserByIndex(DWORD dwID, IUnknown **ppunk) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHistoryObject(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor) = 0;
	virtual HRESULT STDMETHODCALLTYPE CacheOLEServer(IOleObject *pole) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSetCodePage(VARIANT *pvarIn, VARIANT *pvarOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnHttpEquiv(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPalette(HPALETTE *hpal) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterWindow(BOOL fForceRegister, int swc) = 0;
};

EXTERN_C const IID IID_IBrowserService2;

class IBrowserService2 : public IBrowserService
{
public:
	virtual LRESULT STDMETHODCALLTYPE WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetAsDefFolderSettings() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewRect(RECT *prc) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnSize(WPARAM wParam) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnCreate(struct tagCREATESTRUCTW *pcs) = 0;
	virtual LRESULT STDMETHODCALLTYPE OnCommand(WPARAM wParam, LPARAM lParam) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnDestroy() = 0;
	virtual LRESULT STDMETHODCALLTYPE OnNotify(struct tagNMHDR *pnm) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnSetFocus() = 0;
	virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivateBS(BOOL fActive) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseShellView() = 0;
	virtual HRESULT STDMETHODCALLTYPE ActivatePendingView() = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateViewWindow(IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateBrowserPropSheetExt(REFIID riid, void **ppv) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewWindow(HWND *phwndView) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBaseBrowserData(LPCBASEBROWSERDATA *pbbd) = 0;
	virtual LPBASEBROWSERDATA STDMETHODCALLTYPE PutBaseBrowserData() = 0;
	virtual HRESULT STDMETHODCALLTYPE InitializeTravelLog(ITravelLog *ptl, DWORD dw) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTopBrowser() = 0;
	virtual HRESULT STDMETHODCALLTYPE Offline(int iCmd) = 0;
	virtual HRESULT STDMETHODCALLTYPE AllowViewResize(BOOL f) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetActivateState(UINT u) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateSecureLockIcon(int eSecureLock) = 0;
	virtual HRESULT STDMETHODCALLTYPE InitializeDownloadManager() = 0;
	virtual HRESULT STDMETHODCALLTYPE InitializeTransitionSite() = 0;
	virtual HRESULT STDMETHODCALLTYPE _Initialize(HWND hwnd, IUnknown *pauto) = 0;
	virtual HRESULT STDMETHODCALLTYPE _CancelPendingNavigationAsync() = 0;
	virtual HRESULT STDMETHODCALLTYPE _CancelPendingView() = 0;
	virtual HRESULT STDMETHODCALLTYPE _MaySaveChanges() = 0;
	virtual HRESULT STDMETHODCALLTYPE _PauseOrResumeView(BOOL fPaused) = 0;
	virtual HRESULT STDMETHODCALLTYPE _DisableModeless() = 0;
	virtual HRESULT STDMETHODCALLTYPE _NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE _TryShell2Rename(IShellView *psv, LPCITEMIDLIST pidlNew) = 0;
	virtual HRESULT STDMETHODCALLTYPE _SwitchActivationNow() = 0;
	virtual HRESULT STDMETHODCALLTYPE _ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE _SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFolderSetData(struct tagFolderSetData *pfsd) = 0;
	virtual HRESULT STDMETHODCALLTYPE _OnFocusChange(UINT itb) = 0;
	virtual HRESULT STDMETHODCALLTYPE v_ShowHideChildWindows(BOOL fChildOnly) = 0;
	virtual UINT STDMETHODCALLTYPE _get_itbLastFocus() = 0;
	virtual HRESULT STDMETHODCALLTYPE _put_itbLastFocus(UINT itbLastFocus) = 0;
	virtual HRESULT STDMETHODCALLTYPE _UIActivateView(UINT uState) = 0;
	virtual HRESULT STDMETHODCALLTYPE _GetViewBorderRect(RECT *prc) = 0;
	virtual HRESULT STDMETHODCALLTYPE _UpdateViewRectSize() = 0;
	virtual HRESULT STDMETHODCALLTYPE _ResizeNextBorder(UINT itb) = 0;
	virtual HRESULT STDMETHODCALLTYPE _ResizeView() = 0;
	virtual HRESULT STDMETHODCALLTYPE _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon) = 0;
	virtual IStream *STDMETHODCALLTYPE v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName) = 0;
	virtual LRESULT STDMETHODCALLTYPE ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetAcceleratorMenu(HACCEL hacc) = 0;
	virtual int STDMETHODCALLTYPE _GetToolbarCount() = 0;
	virtual LPTOOLBARITEM STDMETHODCALLTYPE _GetToolbarItem(int itb) = 0;
	virtual HRESULT STDMETHODCALLTYPE _SaveToolbars(IStream *pstm) = 0;
	virtual HRESULT STDMETHODCALLTYPE _LoadToolbars(IStream *pstm) = 0;
	virtual HRESULT STDMETHODCALLTYPE _CloseAndReleaseToolbars(BOOL fClose) = 0;
	virtual HRESULT STDMETHODCALLTYPE v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM *pptbi, HWND *phwnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor) = 0;
	virtual UINT STDMETHODCALLTYPE _FindTBar(IUnknown *punkSrc) = 0;
	virtual HRESULT STDMETHODCALLTYPE _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) = 0;
	virtual HRESULT STDMETHODCALLTYPE v_MayTranslateAccelerator(MSG *pmsg) = 0;
	virtual HRESULT STDMETHODCALLTYPE _GetBorderDWHelper(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) = 0;
	virtual HRESULT STDMETHODCALLTYPE v_CheckZoneCrossing(LPCITEMIDLIST pidl) = 0;
};
