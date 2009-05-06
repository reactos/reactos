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
#include "browseui_resource.h"
#include "newinterfaces.h"
#include <perhist.h>
#include <tlogstg.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <exdispid.h>
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

#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")

struct SHELLSTATE2
{
	SHELLSTATE			oldState;
	long				newState1;
	long				newState2;
};

struct categoryCacheHeader
{
	long				dwSize;			// size of header only
	long				version;		// currently 1
	SYSTEMTIME			writeTime;		// time we were written to registry
	long				classCount;		// number of classes following
};

class IGlobalFolderSettings : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Get(SHELLSTATE2 *buffer, int theSize) = 0;
	virtual HRESULT STDMETHODCALLTYPE Set(SHELLSTATE2 *buffer, int theSize, BOOL param14) = 0;
};

static const int							folderOptionsPageCountMax = 20;
static const long							BTP_UPDATE_CUR_HISTORY = 1;
static const long							BTP_UPDATE_NEXT_HISTORY = 2;

extern "C" BOOL								createNewStuff = false;

class CBrowseUIModule : public CAtlDllModuleT<CBrowseUIModule>
{
public:
//	DECLARE_LIBID(LIBID_MyLib)
//	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_MYPROJ, "{...}")
};

CBrowseUIModule								gModule;
CAtlWinModule								gWinModule;

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID(IID_IBrowserService, 0x02BA3B52, 0x0547, 0x11D1, 0xB8, 0x33, 0x00, 0xC0, 0x4F, 0xC9, 0xB3, 0x1F);
DEFINE_GUID(IID_IBrowserService2, 0x68BD21CC, 0x438B, 0x11D2, 0xA5, 0x60, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);
DEFINE_GUID(IID_IShellMenu2, 0x6F51C646, 0x0EFE, 0x4370, 0x88, 0x2A, 0xC1, 0xF6, 0x1C, 0xB2, 0x7C, 0x3B);
DEFINE_GUID(IID_IWinEventHandler, 0xEA5F2D61, 0xE008, 0x11CF, 0x99, 0xCB, 0x00, 0xC0, 0x4F, 0xD6, 0x44, 0x97);
DEFINE_GUID(IID_IShellMenuAcc, 0xFAF6FE96, 0xCE5E, 0x11D1, 0x83, 0x71, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xD0);
DEFINE_GUID(IID_IShellBrowserService, 0x1307EE17, 0xEA83, 0x49EB, 0x96, 0xB2, 0x3A, 0x28, 0xE2, 0xD7, 0x04, 0x8A);
DEFINE_GUID(IID_ITargetFrame2, 0x86D52E11, 0x94A8, 0x11D0, 0x11, 0x82, 0xAF, 0x00, 0xC0, 0x4F, 0xD5, 0xAE);
DEFINE_GUID(IID_IFolderView, 0xCDE725B0, 0xCCC9, 0x4519, 0x91, 0x7E, 0x32, 0x5D, 0x72, 0xFA, 0xB4, 0xCE);

DEFINE_GUID(SID_SProxyBrowser, 0x20C46561, 0x8491, 0x11CF, 0x96, 0x0C, 0x00, 0x80, 0xC7, 0xF4, 0xEE, 0x85);

// this class lives in shell32.dll
DEFINE_GUID(IID_IGlobalFolderSettings, 0xEF8AD2D3, 0xAE36, 0x11D1, 0xB2, 0xD2, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);
DEFINE_GUID(CLSID_GlobalFolderSettings, 0xEF8AD2D1, 0xAE36, 0x11D1, 0xB2, 0xD2, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);
DEFINE_GUID(IID_IRegTreeOptions, 0xAF4F6511, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8);
DEFINE_GUID(CLSID_CRegTreeOptions, 0xAF4F6510, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8);
DEFINE_GUID(IID_IExplorerToolbar, 0x8455F0C1, 0x158F, 0x11D0, 0x89, 0xAE, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);

// not registered, lives in browseui.dll
DEFINE_GUID(CLSID_BrowserBar, 0x9581015C, 0xD08E, 0x11D0, 0x8D, 0x36, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);

DEFINE_GUID(CGID_DefViewFrame, 0x710EB7A1, 0x45ED, 0x11D0, 0x92, 0x4A, 0x00, 0x20, 0xAF, 0xC7, 0xAC, 0x4D);

// browseui.dll
DEFINE_GUID(CLSID_SH_AddressBand, 0x01E04581, 0x4EEE, 0x11D0, 0xBF, 0xE9, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_AddressEditBox, 0xA08C11D2, 0xA228, 0x11D0, 0x82, 0x5B, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(IID_IAddressEditBox, 0xA08C11D1, 0xA228, 0x11D0, 0x82, 0x5B, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);

DEFINE_GUID(IID_IAddressBand, 0x106E86E1, 0x52B5, 0x11D0, 0xBF, 0xED, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_BrandBand, 0x22BF0C20, 0x6DA7, 0x11D0, 0xB3, 0x73, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0x38);
DEFINE_GUID(SID_SBrandBand, 0x82A62DE8, 0x32AC, 0x4E4A, 0x99, 0x35, 0x90, 0x46, 0xC3, 0x78, 0xCF, 0x90);
DEFINE_GUID(CLSID_InternetToolbar, 0x5E6AB780, 0x7743, 0x11CF, 0xA1, 0x2B, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

DEFINE_GUID(CGID_PrivCITCommands, 0x67077B95, 0x4F9D, 0x11D0, 0xB8, 0x84, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04);
DEFINE_GUID(CGID_Theater, 0x0F12079C, 0xC193, 0x11D0, 0x8D, 0x49, 0x00, 0xC0, 0x4F, 0xC9, 0x9D, 0x61);
DEFINE_GUID(CGID_ShellBrowser, 0x3531F060, 0x22B3, 0x11D0, 0x96, 0x9E, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04);

DEFINE_GUID(CLSID_SearchBand, 0x2559A1F0, 0x21D7, 0x11D4, 0xBD, 0xAF, 0x00, 0xC0, 0x4F, 0x60, 0xB9, 0xF0);
DEFINE_GUID(CLSID_TipOfTheDayBand, 0x4D5C8C25, 0xD075, 0x11D0, 0xB4, 0x16, 0x00, 0xC0, 0x4F, 0xB9, 0x03, 0x76);
DEFINE_GUID(CLSID_DiscussBand, 0xBDEADE7F, 0xC265, 0x11D0, 0xBC, 0xED, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x0F);
DEFINE_GUID(CLSID_SH_FavBand, 0xEFA24E61, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_SH_HistBand, 0xEFA24E62, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_ExplorerBand, 0xEFA24E64, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_SH_SearchBand, 0x21569614, 0xB795, 0x46B1, 0x85, 0xF4, 0xE7, 0x37, 0xA8, 0xDC, 0x09, 0xAD);
DEFINE_GUID(CLSID_FileSearchBand, 0xC4EE31F3, 0x4768, 0x11D2, 0x5C, 0xBE, 0x00, 0xA0, 0xC9, 0xA8, 0x3D, 0xA1);
// missing ResearchBand

DEFINE_GUID(IID_IBandNavigate, 0x3697C30B, 0xCD88, 0x11D0, 0x8A, 0x3E, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(IID_INamespaceProxy, 0xCF1609EC, 0xFA4B, 0x4818, 0xAB, 0x01, 0x55, 0x64, 0x33, 0x67, 0xE6, 0x6D);
DEFINE_GUID(IID_IBandProxy, 0x208CE801, 0x754B, 0x11D0, 0x80, 0xCA, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_BandProxy, 0xF61FFEC1, 0x754F, 0x11D0, 0x80, 0xCA, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(SID_IBandProxy, 0x80243AC1, 0x0569, 0x11D1, 0xA7, 0xAE, 0x00, 0x60, 0x97, 0xDF, 0x5B, 0xD4);
DEFINE_GUID(CLSID_ShellSearchExt, 0x169A0691, 0x8DF9, 0x11D1, 0xA1, 0xC4, 0x00, 0xC0, 0x4F, 0xD7, 0x5D, 0x13);

DEFINE_GUID(CLSID_CommonButtons, 0x1E79697E, 0x9CC5, 0x11D1, 0xA8, 0x3F, 0x00, 0xC0, 0x4F, 0xC9, 0x9D, 0x61);
DEFINE_GUID(CLSID_BandSiteMenu, 0xECD4FC4E, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

DEFINE_GUID(CGID_BrandCmdGroup, 0x25019D8C, 0x9EE0, 0x45C0, 0x88, 0x3B, 0x97, 0x2D, 0x48, 0x32, 0x5E, 0x18);

DEFINE_GUID(CLSID_ShellDesktop, 0x00021400, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

DEFINE_GUID(CLSID_ShellShellNameSpace, 0x2F2F1F96, 0x2BC1, 0x4B1C, 0xBE, 0x28, 0xEA, 0x37, 0x74, 0xF4, 0x67, 0x6A);

DEFINE_GUID(IID_INSCTree, 0x43A8F463, 0x4222, 0x11D2, 0xB6, 0x41, 0x00, 0x60, 0x97, 0xDF, 0x5B, 0xD4);
DEFINE_GUID(IID_INSCTree2, 0x801C1AD5, 0xC47C, 0x428C, 0x97, 0xAF, 0xE9, 0x91, 0xE4, 0x85, 0x7D, 0x97);









// this class is private to browseui.dll and is not registered externally?
//DEFINE_GUID(CLSID_ShellFldSetExt, 0x6D5313C0, 0x8C62, 0x11D1, 0xB2, 0xCD, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);


extern HRESULT CreateTravelLog(REFIID riid, void **ppv);
extern HRESULT CreateBaseBar(REFIID riid, void **ppv);
extern HRESULT CreateBaseBarSite(REFIID riid, void **ppv);
SHSTDAPI_(void *) SHAlloc(SIZE_T cb);

// temporary
extern HRESULT CreateInternetToolbar(REFIID riid, void **ppv);



#ifdef SetWindowLongPtr
#undef SetWindowLongPtr
inline LONG_PTR SetWindowLongPtr(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	return SetWindowLong(hWnd, nIndex, (LONG)dwNewLong);
}
#endif

#ifdef GetWindowLongPtr
#undef GetWindowLongPtr
inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex)
{
	return (LONG_PTR)GetWindowLong(hWnd, nIndex);
}
#endif

HMENU SHGetMenuFromID(HMENU topMenu, int theID)
{
	MENUITEMINFO							menuItemInfo;

	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_SUBMENU;
	if (GetMenuItemInfo(topMenu, theID, FALSE, &menuItemInfo) == FALSE)
		return NULL;
	return menuItemInfo.hSubMenu;
}

void SHCheckMenuItem(HMENU theMenu, int theID, BOOL checked)
{
	MENUITEMINFO							menuItemInfo;

	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STATE;
	if (GetMenuItemInfo(theMenu, theID, FALSE, &menuItemInfo))
	{
		if (checked)
			menuItemInfo.fState |= MF_CHECKED;
		else
			menuItemInfo.fState &= ~MF_CHECKED;
		SetMenuItemInfo(theMenu, theID, FALSE, &menuItemInfo);
	}
}

void DeleteMenuItems(HMENU theMenu, unsigned int firstIDToDelete, unsigned int lastIDToDelete)
{
	MENUITEMINFO							menuItemInfo;
	int										menuItemCount;
	int										curIndex;

	menuItemCount = GetMenuItemCount(theMenu);
	curIndex = 0;
	while (curIndex < menuItemCount)
	{
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID;
		if (GetMenuItemInfo(theMenu, curIndex, TRUE, &menuItemInfo) && menuItemInfo.wID >= firstIDToDelete && menuItemInfo.wID <= lastIDToDelete)
		{
			DeleteMenu(theMenu, curIndex, MF_BYPOSITION);
			menuItemCount--;
		}
		else
			curIndex++;
	}
}

HRESULT WINAPI SHBindToFolder(LPITEMIDLIST path, IShellFolder **newFolder)
{
	CComPtr<IShellFolder>					desktop;

	::SHGetDesktopFolder(&desktop);
	if (desktop == NULL)
		return E_FAIL;
	if (path == NULL || path->mkid.cb == 0)
	{
		*newFolder = desktop;
		desktop.p->AddRef ();
		return S_OK;
	}
	return desktop->BindToObject (path, NULL, IID_IShellFolder, (void **)newFolder);
}

static const TCHAR szCabinetWndClass[] = TEXT("CabinetWClassX");
static const TCHAR szExploreWndClass[] = TEXT("ExploreWClassX");

class CDockManager;
class CShellBrowser;

class CToolbarProxy :
	public CWindowImpl<CToolbarProxy, CWindow, CControlWinTraits>
{
private:
	CComPtr<IExplorerToolbar>				fExplorerToolbar;
public:
	void Initialize(HWND parent, IUnknown *explorerToolbar);
	
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
	HWND									myWindow;
	HRESULT									hResult;

	myWindow = SHCreateWorkerWindow(0, parent, 0, WS_CHILD, NULL, 0);
	if (myWindow != NULL)
	{
		SubclassWindow(myWindow);
		SetWindowPos(NULL, -32000, -32000, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER);
		hResult = explorerToolbar->QueryInterface(IID_IExplorerToolbar, (void **)&fExplorerToolbar);
	}
}

LRESULT CToolbarProxy::OnAddBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	LRESULT									result;
	HRESULT									hResult;

	result = 0;
	if (fExplorerToolbar.p != NULL)
	{
		hResult = fExplorerToolbar->AddBitmap(&CGID_ShellBrowser, 1, (long)wParam, (TBADDBITMAP *)lParam, &result, RGB(192, 192, 192));
		hResult = fExplorerToolbar->AddBitmap(&CGID_ShellBrowser, 2, (long)wParam, (TBADDBITMAP *)lParam, &result, RGB(192, 192, 192));
	}
	return result;
}

LRESULT CToolbarProxy::OnForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	LRESULT									result;
	HRESULT									hResult;

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

class CShellBrowser :
	public CWindowImpl<CShellBrowser, CWindow, CControlWinTraits>,
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
		RECT								borderSpace;
		CComPtr<IUnknown>					clientBar;
		HWND								hwnd;
	};
	static const int						BIInternetToolbar = 0;
	static const int						BIVerticalBaseBar = 1;
	static const int						BIHorizontalBaseBar = 2;

	HWND									fCurrentShellViewWindow;	// our currently hosted shell view window
	CComPtr<IShellFolder>					fCurrentShellFolder;		// 
	CComPtr<IShellView>						fCurrentShellView;			// 
	LPITEMIDLIST							fCurrentDirectoryPIDL;		// 
	HWND									fStatusBar;
	bool									fStatusBarVisible;
	CToolbarProxy							fToolbarProxy;
	barInfo									fClientBars[3];
	CComPtr<ITravelLog>						fTravelLog;
	HMENU									fCurrentMenuBar;
	CABINETSTATE							fCabinetState;
	// The next three fields support persisted history for shell views. They do not need to be reference counted.
	IOleObject								*fHistoryObject;
	IStream									*fHistoryStream;
	IBindCtx								*fHistoryBindContext;
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
	HRESULT Initialize(LPITEMIDLIST pidl, long b, long c, long d);
public:
	HRESULT BrowseToPIDL(LPCITEMIDLIST pidl, long flags);
	HRESULT BrowseToPath(IShellFolder *newShellFolder, LPITEMIDLIST absolutePIDL, FOLDERSETTINGS *folderSettings, long flags);
	HRESULT GetMenuBand(REFIID riid, void **shellMenu);
	HRESULT GetBaseBar(bool vertical, IUnknown **theBaseBar);
	HRESULT ShowBand(const CLSID &classID, bool vertical);
	HRESULT NavigateToParent();
	HRESULT DoFolderOptions();
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void RepositionBars();
	virtual WNDPROC GetWindowProc()
	{
		return WindowProc;
	}
	HRESULT FireEvent(DISPID dispIdMember, int argCount, VARIANT *arguments);
	HRESULT FireNavigateComplete(const wchar_t *newDirectory);
	HRESULT FireCommandStateChange(bool newState, int commandID);
	HRESULT FireCommandStateChangeAll();
	HRESULT UpdateForwardBackState();
	void UpdateGotoMenu(HMENU theMenu);
	void UpdateViewMenu(HMENU theMenu);

/*	// *** IDockingWindowFrame methods ***
	virtual HRESULT STDMETHODCALLTYPE AddToolbar(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags);
	virtual HRESULT STDMETHODCALLTYPE RemoveToolbar(IUnknown *punkSrc, DWORD dwRemoveFlags);
	virtual HRESULT STDMETHODCALLTYPE FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppv);
	*/

	// *** IDockingWindowSite methods ***
	virtual HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkObj, LPRECT prcBorder);
	virtual HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);
	virtual HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);

	// *** IOleCommandTarget methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

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
    virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(struct IShellView **ppshv);
    virtual HRESULT STDMETHODCALLTYPE OnViewWindowActive(struct IShellView *ppshv);
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
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

	// *** IBrowserService methods ***
	virtual HRESULT STDMETHODCALLTYPE GetParentSite(IOleInPlaceSite **ppipsite);
	virtual HRESULT STDMETHODCALLTYPE SetTitle(IShellView *psv, LPCWSTR pszName);
	virtual HRESULT STDMETHODCALLTYPE GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName);
	virtual HRESULT STDMETHODCALLTYPE GetOleObject(IOleObject **ppobjv);
	virtual HRESULT STDMETHODCALLTYPE GetTravelLog(ITravelLog **pptl);
	virtual HRESULT STDMETHODCALLTYPE ShowControlWindow(UINT id, BOOL fShow);
	virtual HRESULT STDMETHODCALLTYPE IsControlWindowShown(UINT id, BOOL *pfShown);
	virtual HRESULT STDMETHODCALLTYPE IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags);
	virtual HRESULT STDMETHODCALLTYPE IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPCITEMIDLIST *ppidlOut);
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
	virtual HRESULT STDMETHODCALLTYPE GetPidl(LPCITEMIDLIST *ppidl);
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
	virtual HRESULT STDMETHODCALLTYPE _ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
	virtual HRESULT STDMETHODCALLTYPE _SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam);
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
	virtual HRESULT STDMETHODCALLTYPE v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM *pptbi, HWND *phwnd);
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
	virtual HRESULT STDMETHODCALLTYPE Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers);
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

	// *** IWebBrowser2 methods ***
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
	virtual HRESULT STDMETHODCALLTYPE Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers);
	virtual HRESULT STDMETHODCALLTYPE QueryStatusWB(OLECMDID cmdID, OLECMDF *pcmdf);
	virtual HRESULT STDMETHODCALLTYPE ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
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
	virtual HRESULT STDMETHODCALLTYPE GetWindowData(LPWINDOWDATA pWinData);
	virtual HRESULT STDMETHODCALLTYPE LoadHistoryPosition(LPWSTR pszUrlLocation, DWORD dwPosition);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistHistory methods ***
	virtual HRESULT STDMETHODCALLTYPE LoadHistory(IStream *pStream, IBindCtx *pbc);
	virtual HRESULT STDMETHODCALLTYPE SaveHistory(IStream *pStream);
	virtual HRESULT STDMETHODCALLTYPE SetPositionCookie(DWORD dwPositioncookie);
	virtual HRESULT STDMETHODCALLTYPE GetPositionCookie(DWORD *pdwPositioncookie);

	// *** IBrowserService2 methods ***


	// message handlers
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT RelayMsgToShellView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnFolderOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnMapNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnDisconnectNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnAboutReactOS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnGoBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnGoForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnGoUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnGoHome(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnIsThisLegal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleStatusBarVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleToolbarLock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleToolbarBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleAddressBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleLinksBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToggleTextLabels(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnToolbarCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnGoTravel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT RelayCommands(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

	static ATL::CWndClassInfo& GetWndClassInfo()
	{
		static ATL::CWndClassInfo wc =
		{
			{ sizeof(WNDCLASSEX), CS_DBLCLKS, StartWindowProc,
			  0, 0, NULL, LoadIcon(hExplorerInstance, MAKEINTRESOURCE(IDI_CABINET)),
			  LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, szCabinetWndClass, NULL },
			NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
		};
		return wc;
	}

BEGIN_MSG_MAP(CShellBrowser)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
	MESSAGE_HANDLER(WM_MEASUREITEM, RelayMsgToShellView)
	MESSAGE_HANDLER(WM_DRAWITEM, RelayMsgToShellView)
	MESSAGE_HANDLER(WM_MENUSELECT, RelayMsgToShellView)
	COMMAND_ID_HANDLER(IDM_FILE_CLOSE, OnClose)
	COMMAND_ID_HANDLER(IDM_TOOLS_FOLDEROPTIONS, OnFolderOptions)
	COMMAND_ID_HANDLER(IDM_TOOLS_MAPNETWORKDRIVE, OnMapNetworkDrive)
	COMMAND_ID_HANDLER(IDM_TOOLS_DISCONNECTNETWORKDRIVE, OnDisconnectNetworkDrive)
	COMMAND_ID_HANDLER(IDM_HELP_ABOUT, OnAboutReactOS)
	COMMAND_ID_HANDLER(IDM_GOTO_BACK, OnGoBack)
	COMMAND_ID_HANDLER(IDM_GOTO_FORWARD, OnGoForward)
	COMMAND_ID_HANDLER(IDM_GOTO_UPONELEVEL, OnGoUpLevel)
	COMMAND_ID_HANDLER(IDM_GOTO_HOMEPAGE, OnGoHome)
	COMMAND_ID_HANDLER(IDM_HELP_ISTHISCOPYLEGAL, OnIsThisLegal)
	COMMAND_ID_HANDLER(IDM_VIEW_STATUSBAR, OnToggleStatusBarVisible)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_LOCKTOOLBARS, OnToggleToolbarLock)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_STANDARDBUTTONS, OnToggleToolbarBandVisible)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_ADDRESSBAR, OnToggleAddressBandVisible)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_LINKSBAR, OnToggleLinksBandVisible)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_TEXTLABELS, OnToggleTextLabels)
	COMMAND_ID_HANDLER(IDM_TOOLBARS_CUSTOMIZE, OnToolbarCustomize)
	COMMAND_RANGE_HANDLER(IDM_GOTO_TRAVEL_FIRSTTARGET, IDM_GOTO_TRAVEL_LASTTARGET, OnGoTravel)
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
	int										x;

	fCurrentShellViewWindow = NULL;
	fCurrentDirectoryPIDL = NULL;
	fStatusBar = NULL;
	fStatusBarVisible = true;
	for (x = 0; x < 3; x++)
		fClientBars[x].hwnd = NULL;
	fCurrentMenuBar = NULL;
	fHistoryObject = NULL;
	fHistoryStream = NULL;
	fHistoryBindContext = NULL;
}

CShellBrowser::~CShellBrowser()
{
}

HRESULT CShellBrowser::Initialize(LPITEMIDLIST pidl, long b, long c, long d)
{
	CComPtr<IDockingWindow>					dockingWindow;
	CComPtr<IStream>						settingsStream;
	CComPtr<IPersistStreamInit>				persistStreamInit;
	CComPtr<IOleCommandTarget>				commandTarget;
	CComPtr<IObjectWithSite>				objectSite;
	RECT									bounds = {0, 0, 800, 591};
	HRESULT									hResult;

	_AtlInitialConstruct();

	fCabinetState.cLength = sizeof(fCabinetState);
	if (ReadCabinetState(&fCabinetState, sizeof(fCabinetState)) == FALSE)
	{
	}

	Create(HWND_DESKTOP, bounds, NULL, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0U);
	if (m_hWnd == NULL)
		return E_FAIL;

#if 0
	hResult = CoCreateInstance(CLSID_InternetToolbar, NULL, COM_RIGHTS_EXECUTE, IID_IUnknown, (void **)&fClientBars[BIInternetToolbar].clientBar);
	if (FAILED(hResult))
		return hResult;
#else
	hResult = CreateInternetToolbar(IID_IUnknown, (void **)&fClientBars[BIInternetToolbar].clientBar);
	if (FAILED(hResult))
		return hResult;
#endif
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IDockingWindow, (void **)&dockingWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IPersistStreamInit, (void **)&persistStreamInit);
	if (FAILED(hResult))
		return hResult;
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return hResult;
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IObjectWithSite, (void **)&objectSite);
	if (FAILED(hResult))
		return hResult;
	hResult = objectSite->SetSite((IShellBrowser *)this);
	if (FAILED(hResult))
		return hResult;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, 1, 1 /* or 0 */, NULL, NULL);
	if (FAILED(hResult))
		return hResult;
	// TODO: create settingsStream from registry entry
	if (settingsStream.p == NULL)
	{
		hResult = persistStreamInit->InitNew();
		if (FAILED(hResult))
			return hResult;
	}
	else
	{
		hResult = persistStreamInit->Load(settingsStream);
		if (FAILED(hResult))
			return hResult;
	}
	hResult = dockingWindow->ShowDW(TRUE);
	if (FAILED(hResult))
		return hResult;

	fToolbarProxy.Initialize(m_hWnd, fClientBars[BIInternetToolbar].clientBar);

	fStatusBar = CreateWindow(STATUSCLASSNAME, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
					SBT_NOBORDERS | SBT_TOOLTIPS, 0, 0, 500, 20, m_hWnd, (HMENU)0xa001, hExplorerInstance, 0);
	fStatusBarVisible = true;

	LPITEMIDLIST							desktopPIDL;
	FOLDERSETTINGS							newFolderSettings;

	hResult = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &desktopPIDL);
	if (FAILED(hResult))
		return hResult;
	newFolderSettings.ViewMode = FVM_LIST;
	newFolderSettings.fFlags = 0;
	hResult = BrowseToPIDL(desktopPIDL, BTP_UPDATE_NEXT_HISTORY);
	ILFree(desktopPIDL);
	if (FAILED (hResult))
		return hResult;

	ShowWindow(SW_SHOWNORMAL);




	// test code to make brand band animate
	{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IBandSite>						bandSite;
	CComPtr<IDeskBand>						deskBand;
	CComPtr<IWinEventHandler>				winEventHandler;
	HRESULT									hResult;

	if (fClientBars[BIInternetToolbar].clientBar.p == NULL)
		return 0;
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	hResult = serviceProvider->QueryService(SID_IBandSite, IID_IBandSite, (void **)&bandSite);
	hResult = bandSite->QueryBand(5, &deskBand, NULL, NULL, 0);
	deskBand->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
	winEventHandler->IsWindowOwner(NULL);
	deskBand->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	oleCommandTarget->QueryStatus(&CGID_BrandCmdGroup, 0, NULL, NULL);
	oleCommandTarget->Exec(&CGID_BrandCmdGroup, BBID_STARTANIMATION, 0, NULL, NULL);
	}




	return S_OK;
}

HRESULT CShellBrowser::BrowseToPIDL(LPCITEMIDLIST pidl, long flags)
{
	CComPtr<IShellFolder>					newFolder;
	FOLDERSETTINGS							newFolderSettings;
	HRESULT									hResult;

	// called by shell view to browse to new folder
	// also called by explorer band to navigate to new folder
	hResult = SHBindToFolder((LPITEMIDLIST)pidl, &newFolder);
	newFolderSettings.ViewMode = FVM_LIST;
	newFolderSettings.fFlags = 0;
	hResult = BrowseToPath(newFolder, (LPITEMIDLIST)pidl, &newFolderSettings, flags);
	if (FAILED (hResult))
		return hResult;
	return S_OK;
}

BOOL WINAPI _ILIsDesktop(LPCITEMIDLIST pidl)
{
	return (pidl == NULL || pidl->mkid.cb == 0);
}

BOOL WINAPI _ILIsPidlSimple(LPCITEMIDLIST pidl)
{
	LPCITEMIDLIST							pidlnext;
	WORD									length;
	BOOL									ret;

	ret = TRUE;
	if (! _ILIsDesktop(pidl))
	{
		length = pidl->mkid.cb;
		pidlnext = (LPCITEMIDLIST)(((LPBYTE)pidl) + length);
		if (pidlnext->mkid.cb != 0)
			ret = FALSE;
	}
	return ret;
}

HRESULT WINAPI SHBindToFolderIDListParent(IShellFolder *unused, LPCITEMIDLIST pidl, const IID *riid, LPVOID *ppv, LPITEMIDLIST *ppidlLast)
{
	CComPtr<IShellFolder>					psf;
	LPITEMIDLIST							pidlChild;
	LPITEMIDLIST							pidlParent;
	HRESULT									hResult;

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

HRESULT IEGetNameAndFlagsEx(LPITEMIDLIST pidl, SHGDNF uFlags, long param10, LPWSTR pszBuf, UINT cchBuf, SFGAOF *rgfInOut)
{
	CComPtr<IShellFolder>					parentFolder;
	LPITEMIDLIST							childPIDL;
	STRRET									L108;
	HRESULT									hResult;

	hResult = SHBindToFolderIDListParent(NULL, pidl, &IID_IShellFolder, (void **)&parentFolder, &childPIDL);
	hResult = parentFolder->GetDisplayNameOf(childPIDL, uFlags, &L108);
	StrRetToBuf(&L108, childPIDL, pszBuf, cchBuf);
	if (rgfInOut)
		hResult = parentFolder->GetAttributesOf(1, (LPCITEMIDLIST *)&childPIDL, rgfInOut);
	ILFree(childPIDL);
	return S_OK;
}

long IEGetNameAndFlags(LPITEMIDLIST pidl, SHGDNF uFlags, LPWSTR pszBuf, UINT cchBuf, SFGAOF *rgfInOut)
{
	return IEGetNameAndFlagsEx(pidl, uFlags, NULL, pszBuf, cchBuf, rgfInOut);
}

HRESULT CShellBrowser::BrowseToPath(IShellFolder *newShellFolder, LPITEMIDLIST absolutePIDL, FOLDERSETTINGS *folderSettings, long flags)
{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	CComPtr<IObjectWithSite>				objectWithSite;
	CComPtr<IShellFolder>					saveCurrentShellFolder;
	CComPtr<IShellView>						saveCurrentShellView;
	CComPtr<IShellView>						newShellView;
	CComPtr<ITravelLog>						travelLog;
	HWND									newShellViewWindow;
	BOOL									windowUpdateIsLocked;
	RECT									shellViewWindowBounds;
	HWND									previousView;
	HCURSOR									saveCursor;
	wchar_t									newTitle[MAX_PATH];
	SHGDNF									nameFlags;
	HRESULT									hResult;

	if (newShellFolder == NULL)
		return E_INVALIDARG;
	hResult = GetTravelLog(&travelLog);
	if (FAILED (hResult))
		return hResult;
	if (flags & BTP_UPDATE_CUR_HISTORY)
	{
		if (travelLog->CountEntries((IDropTarget *)this) > 0)
			hResult = travelLog->UpdateEntry((IDropTarget *)this, FALSE);
		// what to do with error? Do we want to halt browse because state save failed?
	}
	hResult = newShellFolder->CreateViewObject(m_hWnd, IID_IShellView, (void **)&newShellView);
	if (FAILED (hResult))
		return hResult;
	previousView = fCurrentShellViewWindow;
	saveCursor = SetCursor(LoadCursor (NULL, MAKEINTRESOURCE (IDC_WAIT)));
	windowUpdateIsLocked = LockWindowUpdate(TRUE);
	if (fCurrentShellView != NULL)
		::SendMessage(fCurrentShellViewWindow, WM_SETREDRAW, 0, 0);
	hResult = newShellView->QueryInterface(IID_IObjectWithSite, (void **)&objectWithSite);
	if (SUCCEEDED(hResult) && objectWithSite.p != NULL)
		hResult = objectWithSite->SetSite((IDropTarget *)this);
	saveCurrentShellFolder = fCurrentShellFolder;
	saveCurrentShellView = fCurrentShellView;
	fCurrentShellFolder = newShellFolder;
	fCurrentShellView = newShellView;
	if (previousView != NULL)
		::GetWindowRect(previousView, &shellViewWindowBounds);
	else
		memset(&shellViewWindowBounds, 0, sizeof(shellViewWindowBounds));
	::MapWindowPoints(0, m_hWnd, (POINT *)&shellViewWindowBounds, 2);
	hResult = newShellView->CreateViewWindow(saveCurrentShellView, folderSettings, this, &shellViewWindowBounds, &newShellViewWindow);
	if (FAILED (hResult) || newShellViewWindow == NULL)
	{
		fCurrentShellView = saveCurrentShellView;
		fCurrentShellFolder = saveCurrentShellFolder;
		::SendMessage(fCurrentShellViewWindow, WM_SETREDRAW, 1, 0);
		if (windowUpdateIsLocked)
			LockWindowUpdate(FALSE);
		SetCursor(saveCursor);
		return hResult;
	}
	if (objectWithSite.p != NULL)
		hResult = objectWithSite->SetSite(NULL);
	ILFree(fCurrentDirectoryPIDL);
	fCurrentDirectoryPIDL = ILClone(absolutePIDL);
	if (saveCurrentShellView != NULL)
		saveCurrentShellView->DestroyViewWindow();
	fCurrentShellViewWindow = newShellViewWindow;
	oleCommandTarget.Release();
	hResult = newShellView->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	saveCurrentShellView.Release();
	saveCurrentShellFolder.Release();
	hResult = newShellView->UIActivate(SVUIA_ACTIVATE_FOCUS);
	if (windowUpdateIsLocked)
		LockWindowUpdate(FALSE);
	SetCursor(saveCursor);
	if (flags & BTP_UPDATE_NEXT_HISTORY)
	{
		hResult = travelLog->AddEntry((IDropTarget *)this, FALSE);
		hResult = travelLog->UpdateEntry((IDropTarget *)this, FALSE);
	}
	FireNavigateComplete(L"c:\\temp");		// TODO: use real path here
	if (fCabinetState.fFullPathTitle)
		nameFlags = SHGDN_FORADDRESSBAR | SHGDN_FORPARSING;
	else
		nameFlags = SHGDN_FORADDRESSBAR;
	hResult = IEGetNameAndFlags(fCurrentDirectoryPIDL, nameFlags, newTitle, sizeof(newTitle) / sizeof(wchar_t), NULL);
	if (SUCCEEDED(hResult))
		SetWindowText(newTitle);
	// TODO: Update the window icon
	FireCommandStateChangeAll();
	hResult = UpdateForwardBackState();
	return S_OK;
}

HRESULT CShellBrowser::GetMenuBand(REFIID riid, void **shellMenu)
{
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IBandSite>						bandSite;
	CComPtr<IDeskBand>						deskBand;
	HRESULT									hResult;

	if (fClientBars[BIInternetToolbar].clientBar.p == NULL)
		return E_FAIL;
	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED(hResult))
		return hResult;
	hResult = serviceProvider->QueryService(SID_IBandSite, IID_IBandSite, (void **)&bandSite);
	if (FAILED(hResult))
		return hResult;
	hResult = bandSite->QueryBand(1, &deskBand, NULL, NULL, 0);
	if (FAILED(hResult))
		return hResult;
	return deskBand->QueryInterface(riid, shellMenu);
}

HRESULT CShellBrowser::GetBaseBar(bool vertical, IUnknown **theBaseBar)
{
	CComPtr<IUnknown>						newBaseBar;
	CComPtr<IDeskBar>						deskBar;
	CComPtr<IUnknown>						newBaseBarSite;
	CComPtr<IObjectWithSite>				objectWithSite;
	CComPtr<IDeskBarClient>					deskBarClient;
	IUnknown								**cache;
	HRESULT									hResult;

	if (vertical)
		cache = &fClientBars[BIVerticalBaseBar].clientBar.p;
	else
		cache = &fClientBars[BIHorizontalBaseBar].clientBar.p;
	if (*cache == NULL)
	{
		hResult = CreateBaseBar(IID_IUnknown, (void **)&newBaseBar);
		if (FAILED(hResult))
			return hResult;
		hResult = CreateBaseBarSite(IID_IUnknown, (void **)&newBaseBarSite);
		if (FAILED(hResult))
			return hResult;

		// tell the new base bar about the shell browser
		hResult = newBaseBar->QueryInterface(IID_IObjectWithSite, (void **)&objectWithSite);
		if (FAILED(hResult))
			return hResult;
		hResult = objectWithSite->SetSite((IDropTarget *)this);
		if (FAILED(hResult))
			return hResult;

		// tell the new base bar about the new base bar site
		hResult = newBaseBar->QueryInterface(IID_IDeskBar, (void **)&deskBar);
		if (FAILED(hResult))
			return hResult;
		hResult = deskBar->SetClient(newBaseBarSite);
		if (FAILED(hResult))
			return hResult;

		// tell the new base bar site about the new base bar
		hResult = newBaseBarSite->QueryInterface(IID_IDeskBarClient, (void **)&deskBarClient);
		if (FAILED(hResult))
			return hResult;
		hResult = deskBarClient->SetDeskBarSite(newBaseBar);
		if (FAILED(hResult))
			return hResult;

		*cache = newBaseBar.Detach();
	}
	return (*cache)->QueryInterface(IID_IUnknown, (void **)theBaseBar);
}

HRESULT CShellBrowser::ShowBand(const CLSID &classID, bool vertical)
{
	CComPtr<IDockingWindow>					dockingWindow;
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	CComPtr<IUnknown>						baseBarSite;
	CComPtr<IUnknown>						newBand;
	CComPtr<IUnknown>						theBaseBar;
	CComPtr<IDeskBar>						deskBar;
	VARIANT									vaIn;
	HRESULT									hResult;

	hResult = GetBaseBar(vertical, (IUnknown **)&theBaseBar);
	if (FAILED(hResult))
		return hResult;
	hResult = CoCreateInstance(classID, NULL, COM_RIGHTS_EXECUTE, IID_IUnknown, (void **)&newBand);
	if (FAILED(hResult))
		return hResult;
	hResult = theBaseBar->QueryInterface(IID_IDeskBar, (void **)&deskBar);
	if (FAILED(hResult))
		return hResult;
	hResult = deskBar->GetClient(&baseBarSite);
	if (FAILED(hResult))
		return hResult;
	hResult = theBaseBar->QueryInterface(IID_IDockingWindow, (void **)&dockingWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = baseBarSite->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (FAILED(hResult))
		return hResult;
	vaIn.vt = VT_UNKNOWN;
	vaIn.punkVal = newBand.p;
	hResult = oleCommandTarget->Exec(&CGID_IDeskBand, 1, 1, &vaIn, NULL);
	if (FAILED(hResult))
		return hResult;
	hResult = dockingWindow->ShowDW(TRUE);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT CShellBrowser::NavigateToParent()
{
	LPITEMIDLIST							newDirectory;
	HRESULT									hResult;

	newDirectory = ILClone(fCurrentDirectoryPIDL);
	if (newDirectory == NULL)
		return E_OUTOFMEMORY;
	ILRemoveLastID(newDirectory);
	hResult = BrowseToPIDL((LPITEMIDLIST)newDirectory, BTP_UPDATE_CUR_HISTORY | BTP_UPDATE_NEXT_HISTORY);
	ILFree(newDirectory);
	if (FAILED (hResult))
		return hResult;
	return S_OK;
}

BOOL CALLBACK AddFolderOptionsPage(HPROPSHEETPAGE thePage, LPARAM lParam)
{
	PROPSHEETHEADER							*sheetInfo;

	sheetInfo = (PROPSHEETHEADER *)lParam;
	if (sheetInfo->nPages >= folderOptionsPageCountMax)
		return FALSE;
	sheetInfo->phpage[sheetInfo->nPages] = thePage;
	sheetInfo->nPages++;
	return TRUE;
}

HRESULT CShellBrowser::DoFolderOptions()
{
	CComPtr<IShellPropSheetExt>				folderOptionsSheet;
	CComPtr<IObjectWithSite>				objectWithSite;
	PROPSHEETHEADER							m_PropSheet;
	HPROPSHEETPAGE							m_psp[folderOptionsPageCountMax];
//	CComPtr<IGlobalFolderSettings>			globalSettings;
//	SHELLSTATE2								shellState;
	HRESULT									hResult;

	memset(m_psp, 0, sizeof(m_psp));
    memset(&m_PropSheet, 0, sizeof(m_PropSheet));

	hResult = CoCreateInstance(CLSID_ShellFldSetExt, NULL, COM_RIGHTS_EXECUTE, IID_IShellPropSheetExt, (void **)&folderOptionsSheet);
	if (FAILED(hResult))
		return E_FAIL;
	// must set site in order for Apply to all Folders on Advanced page to be enabled
	hResult = folderOptionsSheet->QueryInterface(IID_IObjectWithSite, (void **)&objectWithSite);
	if (SUCCEEDED(hResult) && objectWithSite.p != NULL)
		hResult = objectWithSite->SetSite((IDispatch *)this);
	m_PropSheet.phpage = m_psp;
#if 0
	hResult = CoCreateInstance(CLSID_GlobalFolderSettings, NULL, COM_RIGHTS_EXECUTE, IID_IGlobalFolderSettings, (void **)&globalSettings);
	if (FAILED(hResult))
		return E_FAIL;
	hResult = globalSettings->Get(&shellState, sizeof(shellState));
	if (FAILED(hResult))
		return E_FAIL;
#endif
	hResult = folderOptionsSheet->AddPages(AddFolderOptionsPage, (LPARAM)&m_PropSheet);
	if (FAILED(hResult))
		return E_FAIL;
	if (fCurrentShellView != NULL)
	{
        hResult = fCurrentShellView->AddPropertySheetPages( 0, AddFolderOptionsPage, (LPARAM)&m_PropSheet);
		if (FAILED(hResult))
			return E_FAIL;
	}

	m_PropSheet.dwSize = sizeof(PROPSHEETHEADER);
	m_PropSheet.dwFlags = 0;
	m_PropSheet.hwndParent = m_hWnd;
	m_PropSheet.hInstance = hExplorerInstance;
	m_PropSheet.pszCaption = _T("Folder Options");
	m_PropSheet.nStartPage = 0;

    PropertySheet(&m_PropSheet);
	return S_OK;
}

LRESULT CALLBACK CShellBrowser::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CShellBrowser							*pThis = reinterpret_cast<CShellBrowser *>(hWnd);
	_ATL_MSG								msg(pThis->m_hWnd, uMsg, wParam, lParam);
	LRESULT									lResult;
	CComPtr<IMenuBand>						menuBand;
	const _ATL_MSG							*previousMessage;
	BOOL									handled;
	WNDPROC									saveWindowProc;
	HRESULT									hResult;

	hWnd = pThis->m_hWnd;
	previousMessage = pThis->m_pCurrentMsg;
	pThis->m_pCurrentMsg = &msg;

	hResult = pThis->GetMenuBand(IID_IMenuBand, (void **)&menuBand);
	if (SUCCEEDED(hResult) && menuBand.p != NULL)
	{
		hResult = menuBand->TranslateMenuMessage(&msg, &lResult);
		if (hResult == S_OK)
			return lResult;
		uMsg = msg.message;
		wParam = msg.wParam;
		lParam = msg.lParam;
	}

	handled = pThis->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, 0);
	ATLASSERT(pThis->m_pCurrentMsg == &msg);
	if (handled == FALSE)
	{
		if (uMsg == WM_NCDESTROY)
		{
			saveWindowProc = (WNDPROC)GetWindowLongPtr(hWnd, GWL_WNDPROC);
			lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
			if (saveWindowProc == (WNDPROC)GetWindowLongPtr(hWnd, GWL_WNDPROC))
				SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
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
	RECT									clientRect;
	RECT									statusRect;
	RECT									toolbarRect;
	int										x;
	HRESULT									hResult;

	GetClientRect(&clientRect);

	if (fStatusBarVisible)
	{
		::GetWindowRect(fStatusBar, &statusRect);
		::SetWindowPos(fStatusBar, NULL, clientRect.left, clientRect.bottom - (statusRect.bottom - statusRect.top),
							clientRect.right - clientRect.left,
							statusRect.bottom - statusRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
		clientRect.bottom -= statusRect.bottom - statusRect.top;
	}

	for (x = 0; x < 3; x++)
	{
		CComPtr<IOleWindow>					oleWindow;

		if (fClientBars[x].hwnd == NULL && fClientBars[x].clientBar != NULL)
		{
			hResult = fClientBars[x].clientBar->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
			if (SUCCEEDED(hResult))
				hResult = oleWindow->GetWindow(&fClientBars[x].hwnd);
		}
		if (fClientBars[x].hwnd != NULL)
		{
			toolbarRect = clientRect;
			if (fClientBars[x].borderSpace.top != 0)
				toolbarRect.bottom = toolbarRect.top + fClientBars[x].borderSpace.top;
			else if (fClientBars[x].borderSpace.bottom != 0)
				toolbarRect.top = toolbarRect.bottom - fClientBars[x].borderSpace.bottom;
			if (fClientBars[x].borderSpace.left != 0)
				toolbarRect.right = toolbarRect.left + fClientBars[x].borderSpace.left;
			else if (fClientBars[x].borderSpace.right != 0)
				toolbarRect.left = toolbarRect.right - fClientBars[x].borderSpace.right;
			::SetWindowPos(fClientBars[x].hwnd, NULL, toolbarRect.left, toolbarRect.top,
								toolbarRect.right - toolbarRect.left,
								toolbarRect.bottom - toolbarRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
			clientRect.top += fClientBars[x].borderSpace.top;
			clientRect.left += fClientBars[x].borderSpace.left;
			clientRect.bottom += fClientBars[x].borderSpace.bottom;
			clientRect.right += fClientBars[x].borderSpace.right;
		}
	}
	::SetWindowPos(fCurrentShellViewWindow, NULL, clientRect.left, clientRect.top,
						clientRect.right - clientRect.left,
						clientRect.bottom - clientRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
}

HRESULT CShellBrowser::FireEvent(DISPID dispIdMember, int argCount, VARIANT *arguments)
{
	DISPPARAMS							params;
	CComDynamicUnkArray					&vec = IConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents2>::m_vec;
	CComDynamicUnkArray					&vec2 = IConnectionPointImpl<CShellBrowser, &DIID_DWebBrowserEvents>::m_vec;
	HRESULT								hResult;

	params.rgvarg = arguments;
	params.rgdispidNamedArgs = NULL;
	params.cArgs = argCount;
	params.cNamedArgs = 0;
	IUnknown** pp = vec.begin();
	while (pp < vec.end())
	{
		if (*pp != NULL)
		{
			CComPtr<IDispatch>			theDispatch;

			hResult = (*pp)->QueryInterface(IID_IDispatch, (void **)&theDispatch);
			hResult = theDispatch->Invoke(dispIdMember, GUID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, NULL);
		}
		pp++;
	}
	pp = vec2.begin();
	while (pp < vec2.end())
	{
		if (*pp != NULL)
		{
			CComPtr<IDispatch>			theDispatch;

			hResult = (*pp)->QueryInterface(IID_IDispatch, (void **)&theDispatch);
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
	VARIANT								varArg[2];
	VARIANT								varArgs;
	CComBSTR							tempString(newDirectory);

	varArgs.vt = VT_BSTR;
	varArgs.bstrVal = tempString.m_str;

	varArg[0].vt = VT_VARIANT | VT_BYREF;
	varArg[0].pvarVal = &varArgs;
	varArg[1].vt = VT_DISPATCH;
	varArg[1].pdispVal = (IDispatch *)this;

	return FireEvent(DISPID_NAVIGATECOMPLETE2, 2, varArg);
}

HRESULT CShellBrowser::FireCommandStateChange(bool newState, int commandID)
{
	VARIANT								varArg[2];

	varArg[0].vt = VT_BOOL;
	varArg[0].boolVal = newState ? VARIANT_TRUE : VARIANT_FALSE;
	varArg[1].vt = VT_I4;
	varArg[1].lVal = commandID;

	return FireEvent(DISPID_COMMANDSTATECHANGE, 2, varArg);
}

HRESULT CShellBrowser::FireCommandStateChangeAll()
{
	return FireCommandStateChange(false, -1);
}

HRESULT CShellBrowser::UpdateForwardBackState()
{
	CComPtr<ITravelLog>						travelLog;
	CComPtr<ITravelEntry>					unusedEntry;
	bool									canGoBack;
	bool									canGoForward;
	HRESULT									hResult;

	canGoBack = false;
	canGoForward = false;
	hResult = GetTravelLog(&travelLog);
	if (FAILED (hResult))
		return hResult;
	hResult = travelLog->GetTravelEntry((IDropTarget *)this, TLOG_BACK, &unusedEntry);
	if (SUCCEEDED(hResult))
	{
		canGoBack = true;
		unusedEntry.Release();
	}
	hResult = travelLog->GetTravelEntry((IDropTarget *)this, TLOG_FORE, &unusedEntry);
	if (SUCCEEDED(hResult))
	{
		canGoForward = true;
		unusedEntry.Release();
	}
	hResult = FireCommandStateChange(canGoBack, 2);
	hResult = FireCommandStateChange(canGoForward, 1);
	return S_OK;
}

void CShellBrowser::UpdateGotoMenu(HMENU theMenu)
{
	CComPtr<ITravelLog>						travelLog;
	int										position;
	MENUITEMINFO							menuItemInfo;
	HRESULT									hResult;

	DeleteMenuItems(theMenu, IDM_GOTO_TRAVEL_FIRST, IDM_GOTO_TRAVEL_LAST);

	position = GetMenuItemCount(theMenu);
	hResult = GetTravelLog(&travelLog);
	if (FAILED (hResult))
		return;
	hResult = travelLog->InsertMenuEntries((IDropTarget *)this, theMenu, position, IDM_GOTO_TRAVEL_FIRSTTARGET, IDM_GOTO_TRAVEL_LASTTARGET, TLMENUF_BACKANDFORTH | TLMENUF_CHECKCURRENT);
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
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	CComPtr<ITravelLog>						travelLog;
	HMENU									gotoMenu;
	OLECMD									commandList[5];
	HMENU									toolbarMenuBar;
	HMENU									toolbarMenu;
	MENUITEMINFO							menuItemInfo;
	HRESULT									hResult;

	gotoMenu = SHGetMenuFromID(theMenu, FCIDM_MENU_EXPLORE);
	if (gotoMenu != NULL)
		UpdateGotoMenu(gotoMenu);

	commandList[0].cmdID = ITID_TOOLBARBANDSHOWN;
	commandList[1].cmdID = ITID_ADDRESSBANDSHOWN;
	commandList[2].cmdID = ITID_LINKSBANDSHOWN;
	commandList[3].cmdID = ITID_TOOLBARLOCKED;
	commandList[4].cmdID = ITID_CUSTOMIZEENABLED;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (SUCCEEDED(hResult))
		hResult = oleCommandTarget->QueryStatus(&CGID_PrivCITCommands, 5, commandList, NULL);
	if (FAILED(hResult))
		DeleteMenu(theMenu, IDM_VIEW_TOOLBARS, MF_BYCOMMAND);
	else
	{
		toolbarMenuBar = LoadMenu(hExplorerInstance, MAKEINTRESOURCE(IDM_CABINET_CONTEXTMENU));
		toolbarMenu = GetSubMenu(toolbarMenuBar, 0);

		SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_STANDARDBUTTONS, commandList[0].cmdf);
		SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_ADDRESSBAR, commandList[1].cmdf & OLECMDF_ENABLED);
		SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_LINKSBAR, commandList[2].cmdf & OLECMDF_ENABLED);
		SHCheckMenuItem(toolbarMenu, IDM_TOOLBARS_LOCKTOOLBARS, commandList[3].cmdf & OLECMDF_ENABLED);
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

bool IUnknownIsEqual(IUnknown *int1, IUnknown *int2)
{
	CComPtr<IUnknown>						int1Retry;
	CComPtr<IUnknown>						int2Retry;
	HRESULT									hResult;

	if (int1 == int2)
		return true;
	if (int1 == NULL || int2 == NULL)
		return false;
	hResult = int1->QueryInterface(IID_IUnknown, (void **)&int1Retry);
	if (FAILED(hResult))
		return false;
	hResult = int2->QueryInterface(IID_IUnknown, (void **)&int2Retry);
	if (FAILED(hResult))
		return false;
	if (int1Retry == int2Retry)
		return true;
	return false;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GetBorderDW(IUnknown *punkObj, LPRECT prcBorder)
{
	RECT									availableBounds;
	static const INT						excludeItems[] = {1, 1, 1, 0xa001, 0, 0};
	int										x;

	GetEffectiveClientRect(m_hWnd, &availableBounds, (INT *)&excludeItems);
	for (x = 0; x < 3; x++)
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
	int										x;

	for (x = 0; x < 3; x++)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	if (prgCmds == NULL)
		return E_INVALIDARG;
	if (pguidCmdGroup == NULL)
	{
		if (fCurrentShellView.p != NULL)
		{
			hResult = fCurrentShellView->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
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
				case 0x1c:	// search
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case 0x1d:	// history
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case 0x1e:	// favorites
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case 0x23:	// folders
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_LATCHED;
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
				case 0xa022:	// up level
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

HRESULT STDMETHODCALLTYPE CShellBrowser::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	HRESULT									hResult;

	if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
	{
		switch (nCmdID)
		{
			case 0x23:
				hResult = ShowBand(CLSID_ExplorerBand, true);
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
				pvaOut->vt = VT_INT_PTR;
				pvaOut->pintVal = (INT *)LoadMenu(hExplorerInstance, MAKEINTRESOURCE(IDM_CABINET_MAINMENU));
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
	HMENU mainMenu = LoadMenu(hExplorerInstance, MAKEINTRESOURCE(IDM_CABINET_MAINMENU));
	Shell_MergeMenus(hmenuShared, mainMenu, 0, 0, FCIDM_BROWSERLAST, MM_SUBMENUSHAVEIDS);

	int itemCount3 = GetMenuItemCount(hmenuShared);

	lpMenuWidths->width[0] = 2;
	lpMenuWidths->width[2] = 3;
	lpMenuWidths->width[4] = 1;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
	CComPtr<IShellMenu>						shellMenu;
	HRESULT									hResult;

	if (IsMenu(hmenuShared) == FALSE)
		return E_FAIL;
	hResult = GetMenuBand(IID_IShellMenu, (void **)&shellMenu);
	if (FAILED(hResult))
		return hResult;
	hResult = shellMenu->SetMenu(hmenuShared, NULL, SMSET_DONTOWN);
	if (FAILED(hResult))
		return hResult;
	fCurrentMenuBar = hmenuShared;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::RemoveMenusSB(HMENU hmenuShared)
{
	if (hmenuShared == fCurrentMenuBar)
		fCurrentMenuBar = NULL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetStatusTextSB(LPCOLESTR pszStatusText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::EnableModelessSB(BOOL fEnable)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::TranslateAcceleratorSB(MSG *pmsg, WORD wID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
	return BrowseToPIDL((LPITEMIDLIST)pidl, BTP_UPDATE_CUR_HISTORY | BTP_UPDATE_NEXT_HISTORY);
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
	switch(id)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
	LPARAM									result;

	if (pret != NULL)
		*pret = 0;
	switch(id)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryActiveShellView(struct IShellView **ppshv)
{
	if (ppshv == NULL)
		return E_POINTER;
	*ppshv = fCurrentShellView;
	if (fCurrentShellView.p != NULL)
		fCurrentShellView.p->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::OnViewWindowActive(struct IShellView *ppshv)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
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
	if (IsEqualIID(guidService, SID_IWebBrowserApp))		// without this, the internet toolbar won't reflect notifications
		return this->QueryInterface(riid, ppvObject);
	if (IsEqualIID(guidService, SID_SProxyBrowser))
		return this->QueryInterface(riid, ppvObject);
	if (IsEqualIID(guidService, SID_IExplorerToolbar))
		return fClientBars[BIInternetToolbar].clientBar->QueryInterface(riid, ppvObject);
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

HRESULT STDMETHODCALLTYPE CShellBrowser::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
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
	HRESULT									hResult;

	// called by toolbar when displaying tooltips
	if (pptl != NULL)
		*pptl = NULL;
	if (fTravelLog.p == NULL)
	{
		hResult = CreateTravelLog(IID_ITravelLog, (void **)&fTravelLog);
		if (FAILED(hResult))
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

HRESULT STDMETHODCALLTYPE CShellBrowser::IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPCITEMIDLIST *ppidlOut)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::GetPidl(LPCITEMIDLIST *ppidl)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::OnHttpEquiv(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut)
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
	return E_NOTIMPL;
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

HRESULT STDMETHODCALLTYPE CShellBrowser::CreateViewWindow(IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::_SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

HRESULT STDMETHODCALLTYPE CShellBrowser::v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM *pptbi, HWND *phwnd)
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
	return E_NOTIMPL;
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
	CComPtr<ITravelLog>						travelLog;
	HRESULT									hResult;

	hResult = GetTravelLog(&travelLog);
	if (FAILED (hResult))
		return hResult;
	return travelLog->Travel((IDropTarget *)this, TLOG_BACK);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoForward()
{
	CComPtr<ITravelLog>						travelLog;
	HRESULT									hResult;

	hResult = GetTravelLog(&travelLog);
	if (FAILED (hResult))
		return hResult;
	return travelLog->Travel((IDropTarget *)this, TLOG_FORE);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoHome()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::GoSearch()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Refresh()
{
	VARIANT									level;

	level.vt = VT_I4;
	level.lVal = 4;
	return Refresh2(&level);
}

HRESULT STDMETHODCALLTYPE CShellBrowser::Refresh2(VARIANT *Level)
{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	HRESULT									hResult;

	hResult = fCurrentShellView->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (FAILED (hResult))
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

HRESULT STDMETHODCALLTYPE CShellBrowser::Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
	LPITEMIDLIST							pidl;
	HRESULT									hResult;

	// called from drive combo box to navigate to a directory
	if (URL->vt != (VT_ARRAY | VT_UI1))
		return E_INVALIDARG;
	if (URL->parray->cDims != 1 || URL->parray->cbElements != 1)
		return E_INVALIDARG;
	pidl = (LPITEMIDLIST)URL->parray->pvData;
	hResult = BrowseToPIDL((LPITEMIDLIST)pidl, BTP_UPDATE_CUR_HISTORY | BTP_UPDATE_NEXT_HISTORY);
	if (FAILED (hResult))
		return hResult;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::QueryStatusWB(OLECMDID cmdID, OLECMDF *pcmdf)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::ShowBrowserBar(VARIANT *pvaClsid, VARIANT *pvarShow, VARIANT *pvarSize)
{
	CLSID									classID;
	bool									vertical;

	// called to show search bar
	if (pvaClsid->vt != VT_BSTR)
		return E_INVALIDARG;
	CLSIDFromString(pvaClsid->bstrVal, &classID);
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

HRESULT STDMETHODCALLTYPE CShellBrowser::GetWindowData(LPWINDOWDATA pWinData)
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
	CComPtr<IPersistHistory>				viewPersistHistory;
	CComPtr<IOleObject>						viewHistoryObject;
	persistState							oldState;
	unsigned long							numRead;
	LPITEMIDLIST							pidl;
	HRESULT									hResult;

	hResult = pStream->Read(&oldState, sizeof(oldState), &numRead);
	if (FAILED(hResult))
		return hResult;
	if (numRead != sizeof(oldState) || oldState.dwSize != sizeof(oldState))
		return E_FAIL;
	if (oldState.browseType != 2)
		return E_FAIL;
	pidl = (LPITEMIDLIST)SHAlloc(oldState.pidlSize);
	if (pidl == NULL)
		return E_OUTOFMEMORY;
	hResult = pStream->Read(pidl, oldState.pidlSize, &numRead);
	if (FAILED(hResult))
	{
		ILFree(pidl);
		return hResult;
	}
	if (numRead != oldState.pidlSize)
	{
		ILFree(pidl);
		return E_FAIL;
	}
	hResult = CoCreateInstance(oldState.persistClass, NULL, COM_RIGHTS_EXECUTE_REMOTE | COM_RIGHTS_EXECUTE, IID_IOleObject, (void **)&viewHistoryObject);
	fHistoryObject = viewHistoryObject;
	fHistoryStream = pStream;
	fHistoryBindContext = pbc;
	hResult = BrowseToPIDL(pidl, BTP_UPDATE_CUR_HISTORY);
	fHistoryObject = NULL;
	fHistoryStream = NULL;
	fHistoryBindContext = NULL;
	ILFree(pidl);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellBrowser::SaveHistory(IStream *pStream)
{
	CComPtr<IPersistHistory>				viewPersistHistory;
	persistState							newState;
	HRESULT									hResult;

	hResult = fCurrentShellView->GetItemObject(SVGIO_BACKGROUND, IID_IPersistHistory, (void **)&viewPersistHistory);
	memset(&newState, 0, sizeof(newState));
	newState.dwSize = sizeof(newState);
	newState.browseType = 2;
	newState.browserIndex = GetBrowserIndex();
	if (viewPersistHistory.p != NULL)
	{
		hResult = viewPersistHistory->GetClassID(&newState.persistClass);
		if (FAILED(hResult))
			return hResult;
	}
	newState.pidlSize = ILGetSize(fCurrentDirectoryPIDL);
	hResult = pStream->Write(&newState, sizeof(newState), NULL);
	if (FAILED(hResult))
		return hResult;
	hResult = pStream->Write(fCurrentDirectoryPIDL, newState.pidlSize, NULL);
	if (FAILED(hResult))
		return hResult;
	if (viewPersistHistory.p != NULL)
	{
		hResult = viewPersistHistory->SaveHistory(pStream);
		if (FAILED(hResult))
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

LRESULT CShellBrowser::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// TODO: rip down everything
	PostQuitMessage(0);
	return 0;
}

LRESULT CShellBrowser::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	CComPtr<IDockingWindow>					dockingWindow;
	RECT									availableBounds;
	static const INT						excludeItems[] = {1, 1, 1, 0xa001, 0, 0};
	int										x;
	HRESULT									hResult;

	if (wParam != SIZE_MINIMIZED)
	{
		GetEffectiveClientRect(m_hWnd, &availableBounds, (INT *)&excludeItems);
		for (x = 0; x < 3; x++)
		{
			if (fClientBars[x].clientBar != NULL)
			{
				hResult = fClientBars[x].clientBar->QueryInterface(IID_IDockingWindow, (void **)&dockingWindow);
				if (SUCCEEDED(hResult) && dockingWindow != NULL)
				{
					hResult = dockingWindow->ResizeBorderDW(&availableBounds, (IDropTarget *)this, TRUE);
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
	HMENU									theMenu;

	theMenu = (HMENU)wParam;
	if (theMenu == SHGetMenuFromID(fCurrentMenuBar, FCIDM_MENU_VIEW))
		UpdateViewMenu(theMenu);
	return RelayMsgToShellView(uMsg, wParam, lParam, bHandled);
}

LRESULT CShellBrowser::RelayMsgToShellView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if (fCurrentShellViewWindow != NULL)
		return SendMessage(fCurrentShellViewWindow, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShellBrowser::OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	return 0;
}

LRESULT CShellBrowser::OnFolderOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT					hResult;

	hResult = DoFolderOptions();
	return 0;
}

LRESULT CShellBrowser::OnMapNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	WNetConnectionDialog(m_hWnd, RESOURCETYPE_DISK);
	return 0;
}

LRESULT CShellBrowser::OnDisconnectNetworkDrive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	WNetDisconnectDialog(m_hWnd, RESOURCETYPE_DISK);
	return 0;
}

LRESULT CShellBrowser::OnAboutReactOS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	ShellAbout(m_hWnd, _T("ReactOS"), _T(""), NULL);
	return 0;
}

LRESULT CShellBrowser::OnGoBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	hResult = GoBack();
	return 0;
}

LRESULT CShellBrowser::OnGoForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	hResult = GoForward();
	return 0;
}

LRESULT CShellBrowser::OnGoUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	hResult = NavigateToParent();
	return 0;
}

LRESULT CShellBrowser::OnGoHome(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	hResult = GoHome();
	return 0;
}

LRESULT CShellBrowser::OnIsThisLegal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	typedef HRESULT (WINAPI *PSHOpenNewFrame)(LPITEMIDLIST pidl, IUnknown *b, long c, long d);
	PSHOpenNewFrame Func;
	HMODULE hShlwapi;
	hShlwapi = LoadLibrary(TEXT("browseui.dll"));
	if (hShlwapi != NULL)
	{
		Func = (PSHOpenNewFrame)GetProcAddress(hShlwapi, (LPCSTR)103);
	}
	if (Func != NULL)
	{
		LPITEMIDLIST						desktopPIDL;

		hResult = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &desktopPIDL);
		if (SUCCEEDED(hResult))
		{
			hResult = Func(desktopPIDL, NULL, -1, 1);
		}
	}
	return 0;
}

LRESULT CShellBrowser::OnToggleStatusBarVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	fStatusBarVisible = !fStatusBarVisible;
	// TODO: trigger a relayout of contained items
	return 0;
}

LRESULT CShellBrowser::OnToggleToolbarLock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_TOOLBARLOCKED, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnToggleToolbarBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_TOOLBARBANDSHOWN, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnToggleAddressBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_ADDRESSBANDSHOWN, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnToggleLinksBandVisible(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_LINKSBANDSHOWN, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnToggleTextLabels(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_TEXTLABELS, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnToolbarCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	hResult = fClientBars[BIInternetToolbar].clientBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return 0;
	hResult = commandTarget->Exec(&CGID_PrivCITCommands, ITID_CUSTOMIZEENABLED, 0, NULL, NULL);
	return 0;
}

LRESULT CShellBrowser::OnGoTravel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	return 0;
}

LRESULT CShellBrowser::RelayCommands(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if (HIWORD(wParam) == 0 && LOWORD(wParam) < FCIDM_SHVIEWLAST && fCurrentShellViewWindow != NULL)
		return SendMessage(fCurrentShellViewWindow, uMsg, wParam, lParam);
	return 0;
}

static LRESULT CALLBACK ExplorerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static void ExplorerMessageLoop()
{
	MSG Msg;
	BOOL Ret;

	while (1)
	{
		Ret = (GetMessage(&Msg, NULL, 0, 0) != 0);

		if (Ret != -1)
		{
			if (!Ret)
				break;

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
}

struct IEThreadParamBlock
{
	long							offset0;
	long							offset4;
	long							offset8;
	IUnknown						*offsetC;
	long							offset10;
	IUnknown						*offset14;
	LPITEMIDLIST					directoryPIDL;
	char							filler1[84];	// unknown contents
	IUnknown						*offset70;
	long							filler2;		// unknown contents
	IUnknown						*offset78;
	LPITEMIDLIST					offset7C;
	LPITEMIDLIST					offset80;
	char							filler3[116];	// unknown contents
	IUnknown						*offsetF8;		// instance explorer
	long							filler4;		// unknown contents
};

DWORD WINAPI BrowserThreadProc(LPVOID lpThreadParameter)
{
	CComPtr<IShellBrowser>					shellBrowser;
	CComObject<CShellBrowser>				*theCabinet;
	IEThreadParamBlock						*parameters;
	HRESULT									hResult;

	parameters = (IEThreadParamBlock *)lpThreadParameter;
    OleInitialize(NULL);
	ATLTRY (theCabinet = new CComObject<CShellBrowser>);
	if (theCabinet == NULL)
		return E_OUTOFMEMORY;
	hResult = theCabinet->QueryInterface(IID_IShellBrowser, (void **)&shellBrowser);
	if (FAILED (hResult))
	{
		delete theCabinet;
		return hResult;
	}
	hResult = theCabinet->Initialize(parameters->directoryPIDL, 0, 0, 0);
	if (FAILED (hResult))
		return hResult;
	ExplorerMessageLoop();
	OleUninitialize();
	return 0;
}

/*************************************************************************
 * InitOCHostClass				[BROWSEUI.101]
 */
extern "C" void WINAPI InitOCHostClass(long param8)
{
	// forwards to shdocvw
}

/*************************************************************************
 * SHOpenFolderWindow			[BROWSEUI.102]
 */
extern "C" long WINAPI SHOpenFolderWindow(IEThreadParamBlock *param8)
{
	return 0;
}

/*************************************************************************
 * SHCreateSavedWindows			[BROWSEUI.105]
 * Called to recreate explorer windows from previous session
 */
extern "C" void WINAPI SHCreateSavedWindows()
{
}

/*************************************************************************
 * SHCreateFromDesktop			[BROWSEUI.106]
 * parameter is a FolderInfo
 */
extern "C" long WINAPI SHCreateFromDesktop(long param8)
{
	return -1;
}

/*************************************************************************
 * SHExplorerParseCmdLine		[BROWSEUI.107]
 */
extern "C" long WINAPI SHExplorerParseCmdLine(LPCTSTR commandLine)
{
	return -1;
}

/*************************************************************************
 * UEMRegisterNotify			[BROWSEUI.118]
 */
extern "C" void WINAPI UEMRegisterNotify(long param8, long paramC)
{
}

/*************************************************************************
 * SHCreateBandForPidl			[BROWSEUI.120]
 */
extern "C" HRESULT WINAPI SHCreateBandForPidl(LPCITEMIDLIST param8, IUnknown *paramC, BOOL param10)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * SHPidlFromDataObject			[BROWSEUI.121]
 */
extern "C" HRESULT WINAPI SHPidlFromDataObject(IDataObject *param8, long *paramC, long param10, FILEDESCRIPTOR *param14)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * IDataObject_GetDeskBandState	[BROWSEUI.122]
 */
extern "C" long WINAPI IDataObject_GetDeskBandState(long param8)
{
	return -1;
}

/*************************************************************************
 * SHCreateIETHREADPARAM		[BROWSEUI.123]
 */
extern "C" IEThreadParamBlock *WINAPI SHCreateIETHREADPARAM(long param8, long paramC, IUnknown *param10, IUnknown *param14)
{
	IEThreadParamBlock						*result;

	result = (IEThreadParamBlock *)LocalAlloc(LMEM_ZEROINIT, 256);
	if (result == NULL)
		return NULL;
	result->offset0 = param8;
	result->offset8 = paramC;
	result->offsetC = param10;
	if (param10 != NULL)
		param10->AddRef();
	result->offset14 = param14;
	if (param14 != NULL)
		param14->AddRef();
	return result;
}

/*************************************************************************
 * SHCloneIETHREADPARAM			[BROWSEUI.124]
 */
extern "C" IEThreadParamBlock *WINAPI SHCloneIETHREADPARAM(IEThreadParamBlock *param)
{
	IEThreadParamBlock						*result;

	result = (IEThreadParamBlock *)LocalAlloc(LMEM_FIXED, 256);
	if (result == NULL)
		return NULL;
	memcpy(result, param, 0x40 * 4);
	if (result->directoryPIDL != NULL)
		result->directoryPIDL = ILClone(result->directoryPIDL);
	if (result->offset7C != NULL)
		result->offset7C = ILClone(result->offset7C);
	if (result->offset80 != NULL)
		result->offset80 = ILClone(result->offset80);
	if (result->offset70 != NULL)
		result->offset70->AddRef();
#if 0
	if (result->offsetC != NULL)
		result->offsetC->Method2C();
#endif
	return result;
}

/*************************************************************************
 * SHParseIECommandLine			[BROWSEUI.125]
 */
extern "C" long WINAPI SHParseIECommandLine(long param8, long paramC)
{
	return -1;
}

/*************************************************************************
 * SHDestroyIETHREADPARAM		[BROWSEUI.126]
 */
extern "C" void WINAPI SHDestroyIETHREADPARAM(IEThreadParamBlock *param)
{
	if (param == NULL)
		return;
	if (param->directoryPIDL != NULL)
		ILFree(param->directoryPIDL);
	if (param->offset7C != NULL)
		ILFree(param->offset7C);
	if ((param->offset4 & 0x80000) == 0 && param->offset80 != NULL)
		ILFree(param->offset80);
	if (param->offset14 != NULL)
		param->offset14->Release();
	if (param->offset70 != NULL)
		param->offset70->Release();
	if (param->offset78 != NULL)
		param->offset78->Release();
	if (param->offsetC != NULL)
		param->offsetC->Release();
	if (param->offsetF8 != NULL)
		param->offsetF8->Release();
	LocalFree(param);
}

/*************************************************************************
 * SHOnCWMCommandLine			[BROWSEUI.127]
 */
extern "C" HRESULT WINAPI SHOnCWMCommandLine(long param8)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * Channel_GetFolderPidl		[BROWSEUI.128]
 */
extern "C" LPITEMIDLIST WINAPI Channel_GetFolderPidl()
{
	return NULL;
}

/*************************************************************************
 * ChannelBand_Create			[BROWSEUI.129]
 */
extern "C" IUnknown *WINAPI ChannelBand_Create(LPITEMIDLIST pidl)
{
	return NULL;
}

/*************************************************************************
 * Channels_SetBandInfoSFB		[BROWSEUI.130]
 */
extern "C" HRESULT WINAPI Channels_SetBandInfoSFB(IUnknown *param8)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * IUnknown_SetBandInfoSFB		[BROWSEUI.131]
 */
extern "C" HRESULT WINAPI IUnknown_SetBandInfoSFB(IUnknown *param8, long paramC)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * Channel_QuickLaunch			[BROWSEUI.133]
 */
extern "C" HRESULT WINAPI Channel_QuickLaunch()
{
	return E_NOTIMPL;
}

/*************************************************************************
 * SHGetNavigateTarget			[BROWSEUI.134]
 */
extern "C" HRESULT WINAPI SHGetNavigateTarget(long param8, long paramC, long param10, long param14)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * GetInfoTip					[BROWSEUI.135]
 */
extern "C" HRESULT WINAPI GetInfoTip(IUnknown *param8, long paramC, LPTSTR *param10, long cchMax)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * SHEnumClassesOfCategories	[BROWSEUI.136]
 */
extern "C" HRESULT WINAPI SHEnumClassesOfCategories(long param8, long paramC, long param10, long param14, long param18)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * SHWriteClassesOfCategories	[BROWSEUI.137]
 */
extern "C" HRESULT WINAPI SHWriteClassesOfCategories(long param8, long paramC, long param10, long param14, long param18, long param1C, long param20)
{
	return E_NOTIMPL;
}

/*************************************************************************
 * SHIsExplorerBrowser			[BROWSEUI.138]
 */
extern "C" BOOL WINAPI SHIsExplorerBrowser()
{
	return TRUE;
}

// 75FA56C1h
// (pidl, 0, -1, 1)
// this function should handle creating a new process if needed, but I'm leaving that out for now
// this function always opens a new window - it does NOT check for duplicates
/*************************************************************************
 * SHOpenNewFrame				[BROWSEUI.103]
 */
extern "C" HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14)
{
	IEThreadParamBlock						*parameters;
	HANDLE									threadHandle;
	DWORD									threadID;

	parameters = SHCreateIETHREADPARAM(0, 1, paramC, NULL);
	if (parameters == NULL)
	{
		ILFree(pidl);
		return E_OUTOFMEMORY;
	}
	if (paramC != NULL)
		parameters->offset10 = param10;
	parameters->directoryPIDL = pidl;
	parameters->offset4 = param14;
	threadHandle = CreateThread(NULL, 0x10000, BrowserThreadProc, parameters, 0, &threadID);
	if (threadHandle != NULL)
	{
		CloseHandle(threadHandle);
		return S_OK;
	}
	SHDestroyIETHREADPARAM(parameters);
	return E_FAIL;
}

