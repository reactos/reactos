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

/*
Implements a class that knows how to hold and manage the menu band, brand band,
toolbar, and address band for an explorer window
*/
#include "precomp.h"
#include "browseui_resource.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <exdispid.h>
#include "internettoolbar.h"

// navigation controls and menubar just send a message to parent window
/*
TODO:
****Implement BandProxy methods
****Add QueryStatus handler for built-in bands
****Enable/Disable up, search, and folders commands appropriately
  **Why are explorer toolbar separators a nonstandard width?
  **Remove "(Empty)" item from Favorites menu. Probably something missing in CMenuCallback::CallbackSM
  **Chevron menu on menuband doesn't work
  **Fix CInternetToolbar::QueryBand to be generic

****Fix context menu to strip divider when menu shown for menu band
****Fix context menu to have items checked appropriately
****Implement -1 command id update
****When bands are rearranged, resize the internet toolbar and fix height of brand band
****Right clicking on the browse back and forward toolbar buttons displays the same as pulldown menus
    Implement show/hide of bands
    Why is the background color of my toolbars different from explorer?
    Internet Toolbar command handler should get the target for the command and call Exec on the target.
		For commands built in to the Internet Toolbar, its Exec handles the command
    When window width is changed, brand band flashes badly
    Add all bands with correct ids (system bands now add with correct ids)
    Implement IBandSite
	Implement remaining IExplorerToolbar methods
	Fix toolbar buttons to enable/disable correctly
	After toolbar is customized, it may be necessary to patch the widths of separators
	Add theme support
	Check sizes and spacing of toolbars against Explorer
	Implement resizing of the dock bar
	Add missing icons for toolbar items
	Draw History item in forward/back dropdown menus with icon
	Fix toolbar customize dialog to not include separators as possible selections
	Implement save/restore of toolbar state
	Refactor drop down menu code to use a common function since code is so similar
*/

extern HRESULT WINAPI SHBindToFolder(LPITEMIDLIST path, IShellFolder **newFolder);
extern HRESULT CreateToolsBar(REFIID riid, void **ppv);
extern HRESULT CreateBrandBand(REFIID riid, void **ppv);
extern HRESULT CreateBandProxy(REFIID riid, void **ppv);
extern HRESULT CreateAddressBand(REFIID riid, void **ppv);

class CInternetToolbar;

class CDockSite :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IDockingWindowSite,
	public IInputObjectSite,
	public IOleCommandTarget,
	public IServiceProvider
{
public:
	enum {
		ITF_NOGRIPPER = 1,
		ITF_NOTITLE = 2,
		ITF_NEWBANDALWAYS = 4,
		ITF_GRIPPERALWAYS = 8,
		ITF_FIXEDSIZE = 16
	};
private:
	CComPtr<IUnknown>						fContainedBand;			// the band inside us
	CInternetToolbar						*fToolbar;				// our browser
	HWND									fRebarWindow;
	HWND									fChildWindow;
	int										fBandID;
public:
	int										fFlags;
private:
	bool									fInitialized;
	// fields of DESKBANDINFO must be preserved between calls to GetBandInfo
	DESKBANDINFO							fDeskBandInfo;
public:
	CDockSite();
	~CDockSite();
	HRESULT Initialize(IUnknown *containedBand, CInternetToolbar *browser, HWND hwnd, int bandID, int flags);
	HRESULT GetRBBandInfo(REBARBANDINFO &bandInfo);
private:

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

	// *** IDockingWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkObj, LPRECT prcBorder);
	virtual HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);
	virtual HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);

	// *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

	// *** IOleCommandTarget specific methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	// *** IServiceProvider methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

BEGIN_COM_MAP(CDockSite)
	COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
	COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
	COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
	COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
	COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
END_COM_MAP()
};

CDockSite::CDockSite()
{
	fToolbar = NULL;
	fRebarWindow = NULL;
	fChildWindow = NULL;
	fInitialized = false;
	memset(&fDeskBandInfo, 0, sizeof(fDeskBandInfo));
}

CDockSite::~CDockSite()
{
}

HRESULT CDockSite::Initialize(IUnknown *containedBand, CInternetToolbar *browser, HWND hwnd, int bandID, int flags)
{
	CComPtr<IObjectWithSite>				site;
	CComPtr<IOleWindow>						oleWindow;
	CComPtr<IDeskBand>						deskBand;
	TCHAR									textBuffer[40];
	REBARBANDINFO							bandInfo;
	int										bandCount;
	HRESULT									hResult;

	hResult = containedBand->QueryInterface(IID_IObjectWithSite, (void **)&site);
	if (FAILED(hResult))
		return hResult;
	hResult = containedBand->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = containedBand->QueryInterface(IID_IDeskBand, (void **)&deskBand);
	if (FAILED(hResult))
		return hResult;
	fContainedBand = containedBand;
	fToolbar = browser;
	fRebarWindow = hwnd;
	fBandID = bandID;
	fFlags = flags;
	hResult = site->SetSite((IOleWindow *)this);
	if (FAILED(hResult))
		return hResult;
	hResult = oleWindow->GetWindow(&fChildWindow);
	if (FAILED(hResult))
		return hResult;

	memset(&bandInfo, 0, sizeof(bandInfo));
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.lpText = textBuffer;
	bandInfo.cch = sizeof(textBuffer) / sizeof(TCHAR);
	hResult = GetRBBandInfo(bandInfo);

	bandCount = (int)SendMessage(fRebarWindow, RB_GETBANDCOUNT, 0, 0);
	SendMessage(fRebarWindow, RB_INSERTBAND, -1, (LPARAM)&bandInfo);
	fInitialized = true;
	return S_OK;
}

HRESULT CDockSite::GetRBBandInfo(REBARBANDINFO &bandInfo)
{
	CComPtr<IDeskBand>						deskBand;
	HRESULT									hResult;

	hResult = fContainedBand->QueryInterface(IID_IDeskBand, (void **)&deskBand);
	if (FAILED(hResult))
		return hResult;

	fDeskBandInfo.dwMask = DBIM_BKCOLOR | DBIM_MODEFLAGS | DBIM_TITLE | DBIM_ACTUAL | DBIM_INTEGRAL | DBIM_MAXSIZE | DBIM_MINSIZE;
	hResult = deskBand->GetBandInfo(fBandID, 0, &fDeskBandInfo);
	// result of call is ignored

	bandInfo.fMask = RBBIM_LPARAM | RBBIM_IDEALSIZE | RBBIM_ID | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_TEXT | RBBIM_STYLE;

	bandInfo.fStyle = RBBS_FIXEDBMP;
	if (fDeskBandInfo.dwModeFlags & DBIMF_VARIABLEHEIGHT)
		bandInfo.fStyle |= RBBS_VARIABLEHEIGHT;
	if (fDeskBandInfo.dwModeFlags & DBIMF_USECHEVRON)
		bandInfo.fStyle |= RBBS_USECHEVRON;
	if (fDeskBandInfo.dwModeFlags & DBIMF_BREAK)
		bandInfo.fStyle |= RBBS_BREAK;
	if (fDeskBandInfo.dwModeFlags & DBIMF_TOPALIGN)
		bandInfo.fStyle |= RBBS_TOPALIGN;
	if (fFlags & ITF_NOGRIPPER || fToolbar->fLocked == true)
		bandInfo.fStyle |= RBBS_NOGRIPPER;
	if (fFlags & ITF_NOTITLE)
		bandInfo.fStyle |= RBBS_HIDETITLE;
	if (fFlags & ITF_GRIPPERALWAYS && fToolbar->fLocked == false)
		bandInfo.fStyle |= RBBS_GRIPPERALWAYS;
	if (fFlags & ITF_FIXEDSIZE)
		bandInfo.fStyle |= RBBS_FIXEDSIZE;

	if (fDeskBandInfo.dwModeFlags & DBIMF_BKCOLOR)
	{
		bandInfo.fMask |= RBBIM_COLORS;
		bandInfo.clrFore = CLR_DEFAULT;
		bandInfo.clrBack = fDeskBandInfo.crBkgnd;
	}
	wcsncpy(bandInfo.lpText, fDeskBandInfo.wszTitle, bandInfo.cch);
	bandInfo.hwndChild = fChildWindow;
	bandInfo.cxMinChild = fDeskBandInfo.ptMinSize.x;
	bandInfo.cyMinChild = fDeskBandInfo.ptMinSize.y;
	bandInfo.wID = fBandID;
	bandInfo.cyChild = fDeskBandInfo.ptActual.y;
	bandInfo.cyMaxChild = fDeskBandInfo.ptMaxSize.y;
	bandInfo.cyIntegral = fDeskBandInfo.ptIntegral.y;
	bandInfo.cxIdeal = fDeskBandInfo.ptActual.x;
	bandInfo.lParam = (LPARAM)this;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDockSite::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = fRebarWindow;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDockSite::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::GetBorderDW(IUnknown* punkObj, LPRECT prcBorder)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	TCHAR									textBuffer[40];
	REBARBANDINFO							bandInfo;
	int										index;
	HRESULT									hResult;

	if (IsEqualIID(*pguidCmdGroup, CGID_DeskBand))
	{
		switch (nCmdID)
		{
			case DBID_BANDINFOCHANGED:
				if (fInitialized == false)
					return S_OK;
				if (pvaIn->vt != VT_I4)
					return E_INVALIDARG;
				if (pvaIn->lVal != fBandID)
					return E_FAIL;
				// deskband information changed
				// call GetBandInfo and refresh information in rebar
				memset(&bandInfo, 0, sizeof(bandInfo));
				bandInfo.cbSize = sizeof(bandInfo);
				bandInfo.lpText = textBuffer;
				bandInfo.cch = sizeof(textBuffer) / sizeof(TCHAR);
				hResult = GetRBBandInfo(bandInfo);
				if (FAILED(hResult))
					return hResult;
				index = (int)SendMessage(fRebarWindow, RB_IDTOINDEX, fBandID, 0);
				SendMessage(fRebarWindow, RB_SETBANDINFO, index, (LPARAM)&bandInfo);
				return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CDockSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	CComPtr<IServiceProvider>				serviceProvider;
	HRESULT									hResult;

	if (IsEqualIID(guidService, SID_SMenuBandParent))
		return this->QueryInterface(riid, ppvObject);
	hResult = fToolbar->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED (hResult))
		return hResult;
	return serviceProvider->QueryService(guidService, riid, ppvObject);
}

CMenuCallback::CMenuCallback()
{
}

CMenuCallback::~CMenuCallback()
{
}

HRESULT STDMETHODCALLTYPE CMenuCallback::GetObject(LPSMDATA psmd, REFIID riid, void **ppvObject)
{
	CComPtr<IShellMenu>						parentMenu;
	CComPtr<IShellMenu>						newMenu;
	CComPtr<IShellFolder>					favoritesFolder;
	LPITEMIDLIST							favoritesPIDL;
	HWND									ownerWindow;
	HMENU									parentHMenu;
	HMENU									favoritesHMenu;
	HKEY									orderRegKey;
	DWORD									disposition;
	HRESULT									hResult;

	if (!IsEqualIID(riid, IID_IShellMenu))
		return E_FAIL;
	if (psmd->uId != FCIDM_MENU_FAVORITES)
		return E_FAIL;
	if (fFavoritesMenu.p == NULL)
	{
		hResult = psmd->punk->QueryInterface(IID_IShellMenu, (void **)&parentMenu);
		if (FAILED(hResult))
			return hResult;
		hResult = parentMenu->GetMenu(&parentHMenu, &ownerWindow, NULL);
		if (FAILED(hResult))
			return hResult;
		favoritesHMenu = GetSubMenu(parentHMenu, 3);
		if (favoritesHMenu == NULL)
			return E_FAIL;
		hResult = CoCreateInstance(CLSID_MenuBand, NULL, COM_RIGHTS_EXECUTE, IID_IShellMenu, (void **)&newMenu);
		if (FAILED(hResult))
			return hResult;
		hResult = newMenu->Initialize(this, FCIDM_MENU_FAVORITES, -1, SMINIT_VERTICAL | SMINIT_CACHED);
		if (FAILED(hResult))
			return hResult;
		hResult = newMenu->SetMenu(favoritesHMenu, ownerWindow, SMSET_TOP | SMSET_DONTOWN);
		if (FAILED(hResult))
			return hResult;
		hResult = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &favoritesPIDL);
		if (FAILED(hResult))
			return hResult;
		hResult = SHBindToFolder(favoritesPIDL, &favoritesFolder);
		if (FAILED(hResult))
			return hResult;
		RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites"),
				0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &orderRegKey, &disposition);
		hResult = newMenu->SetShellFolder(favoritesFolder, favoritesPIDL, orderRegKey, SMSET_BOTTOM | 0x18);
		ILFree(favoritesPIDL);
		if (SUCCEEDED(hResult))
			fFavoritesMenu.Attach(newMenu.Detach());
	}
	if (fFavoritesMenu.p == NULL)
		return E_FAIL;
	return fFavoritesMenu->QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case SMC_INITMENU:
			break;
		case SMC_CREATE:
			break;
		case SMC_EXITMENU:
			break;
		case SMC_GETINFO:
			{
			SMINFO *infoPtr = (SMINFO *)lParam;
			if ((infoPtr->dwMask & SMIM_FLAGS) != 0)
				if (psmd->uId == FCIDM_MENU_FAVORITES)
					infoPtr->dwFlags |= SMIF_DROPCASCADE;
				else
					infoPtr->dwFlags |= SMIF_TRACKPOPUP;
			if ((infoPtr->dwMask & SMIM_ICON) != 0)
				infoPtr->iIcon = -1;
			}
			return S_OK;
		case SMC_GETSFINFO:
			break;
		case SMC_GETOBJECT:
			return GetObject(psmd, *(IID *)wParam, (void **)lParam);
		case SMC_GETSFOBJECT:
			break;
		case SMC_SFEXEC:
			__asm int 3;
			break;
		case SMC_SFSELECTITEM:
			break;
		case 13:
			// return tooltip
			break;
		case SMC_REFRESH:
			__asm int 3;
			break;
		case SMC_DEMOTE:
			__asm int 3;
			break;
		case SMC_PROMOTE:
			__asm int 3;
			break;
		case 0x13:
			break;
		case SMC_DEFAULTICON:
			break;
		case SMC_NEWITEM:
			break;
		case SMC_CHEVRONEXPAND:
			__asm int 3;
			break;
		case SMC_DISPLAYCHEVRONTIP:
			__asm int 3;
			break;
		case SMC_SETSFOBJECT:
			break;
		case SMC_SHCHANGENOTIFY:
			break;
		case SMC_CHEVRONGETTIP:
			__asm int 3;
			break;
		case SMC_SFDDRESTRICTED:
			__asm int 3;
			break;
		case 0x35:
			break;
		case 49:
			break;
		case 0x10000000:
			break;
	}
	return S_FALSE;
}

CInternetToolbar::CInternetToolbar()
{
	fMainReBar = NULL;
	fLocked = false;
	fMenuBandWindow = NULL;
	fNavigationWindow = NULL;
	fMenuCallback.AddRef();
	fToolbarWindow = NULL;
	fAdviseCookie = 0;
}

CInternetToolbar::~CInternetToolbar()
{
	fMenuCallback.Release();
}

void CInternetToolbar::AddDockItem(IUnknown *newItem, int bandID, int flags)
{
	CDockSite			*newSite;

	newSite = new CComObject<CDockSite>;
	newSite->AddRef();
	newSite->Initialize(newItem, this, fMainReBar, bandID, flags);
}

HRESULT CInternetToolbar::ReserveBorderSpace()
{
	CComPtr<IDockingWindowSite>				dockingWindowSite;
	RECT									availableBorderSpace;
	RECT									neededBorderSpace;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IDockingWindowSite, (void **)&dockingWindowSite);
	if (FAILED(hResult))
		return hResult;
	hResult = dockingWindowSite->GetBorderDW((IDockingWindow *)this, &availableBorderSpace);
	if (FAILED(hResult))
		return hResult;
	SendMessage(fMainReBar, RB_SIZETORECT, RBSTR_CHANGERECT, (LPARAM)&availableBorderSpace);
	neededBorderSpace.left = 0;
	neededBorderSpace.top = availableBorderSpace.bottom - availableBorderSpace.top;
	if (fLocked == false)
		neededBorderSpace.top += 3;
	neededBorderSpace.right = 0;
	neededBorderSpace.bottom = 0;
	hResult = dockingWindowSite->SetBorderSpaceDW((IDockingWindow *)this, &neededBorderSpace);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT CInternetToolbar::CreateMenuBar(IShellMenu **menuBar)
{
	CComPtr<IOleCommandTarget>				siteCommandTarget;
	CComPtr<IOleWindow>						oleWindow;
	CComPtr<IOleCommandTarget>				commandTarget;
	CComPtr<IShellMenuCallback>				callback;
	VARIANT									menuOut;
	HWND									ownerWindow;
	HRESULT									hResult;

	hResult = CoCreateInstance(CLSID_MenuBand, NULL, COM_RIGHTS_EXECUTE, IID_IShellMenu, (void **)menuBar);
	if (FAILED(hResult))
		return hResult;
	hResult = fMenuCallback.QueryInterface(IID_IShellMenuCallback, (void **)&callback);
	if (FAILED(hResult))
		return hResult;
	hResult = (*menuBar)->Initialize(callback, -1, ANCESTORDEFAULT, SMINIT_HORIZONTAL | SMINIT_TOPLEVEL);
	if (FAILED(hResult))
		return hResult;
	hResult = fSite->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = oleWindow->GetWindow(&ownerWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = fSite->QueryInterface(IID_IOleCommandTarget, (void **)&siteCommandTarget);
	if (FAILED(hResult))
		return hResult;
	hResult = siteCommandTarget->Exec(&CGID_Explorer, 0x35, 0, NULL, &menuOut);
	if (FAILED(hResult))
		return hResult;
	if (menuOut.vt != VT_INT_PTR || menuOut.pintVal == NULL)
		return E_FAIL;
	hResult = (*menuBar)->SetMenu((HMENU)menuOut.pintVal, ownerWindow, SMSET_DONTOWN);
	if (FAILED(hResult))
		return hResult;
	hResult = (*menuBar)->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
	if (FAILED(hResult))
		return hResult;
	hResult = commandTarget->Exec(&CGID_MenuBand, 3, 1, NULL, NULL);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT CInternetToolbar::CreateBrandBand(IUnknown **logoBar)
{
	CComPtr<IUnknown>						tempBand;
	HRESULT									hResult;

#if 1
	hResult = ::CreateBrandBand(IID_IUnknown, (void **)logoBar);
#else
	hResult = CoCreateInstance(CLSID_BrandBand, NULL, COM_RIGHTS_EXECUTE, IID_IUnknown, (void **)logoBar);
#endif
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT CInternetToolbar::CreateToolsBar(IUnknown **toolsBar)
{
	HRESULT									hResult;

	hResult = ::CreateToolsBar(IID_IUnknown, (void **)toolsBar);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

HRESULT CInternetToolbar::CreateAddressBand(IUnknown **toolsBar)
{
	CComPtr<IAddressBand>					addressBand;
	HRESULT									hResult;

#if 1
	hResult = ::CreateAddressBand(IID_IUnknown, (void **)toolsBar);
#else
	hResult = CoCreateInstance(CLSID_SH_AddressBand, NULL, COM_RIGHTS_EXECUTE, IID_IUnknown, (void **)toolsBar);
#endif
	if (FAILED(hResult))
		return hResult;
	hResult = (*toolsBar)->QueryInterface(IID_IAddressBand, (void **)&addressBand);
	return S_OK;
}

HRESULT CInternetToolbar::LockUnlockToolbars(bool locked)
{
	REBARBANDINFO							rebarBandInfo;
	int										bandCount;
	int										x;
	CDockSite								*dockSite;
	HRESULT									hResult;

	if (locked != fLocked)
	{
		fLocked = locked;
		rebarBandInfo.cbSize = sizeof(rebarBandInfo);
		rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_LPARAM;
		bandCount = (int)SendMessage(fMainReBar, RB_GETBANDCOUNT, 0, 0);
		for (x  = 0; x < bandCount; x++)
		{
			SendMessage(fMainReBar, RB_GETBANDINFO, x, (LPARAM)&rebarBandInfo);
			dockSite = (CDockSite *)rebarBandInfo.lParam;
			if (dockSite != NULL)
			{
				rebarBandInfo.fStyle &= ~(RBBS_NOGRIPPER | RBBS_GRIPPERALWAYS);
				if (dockSite->fFlags & CDockSite::ITF_NOGRIPPER || fLocked == true)
					rebarBandInfo.fStyle |= RBBS_NOGRIPPER;
				if (dockSite->fFlags & CDockSite::ITF_GRIPPERALWAYS && fLocked == false)
					rebarBandInfo.fStyle |= RBBS_GRIPPERALWAYS;
				SendMessage(fMainReBar, RB_SETBANDINFO, x, (LPARAM)&rebarBandInfo);
			}
		}
		hResult = ReserveBorderSpace();
	}
	return S_OK;
}

HRESULT CInternetToolbar::CommandStateChanged(bool newValue, int commandID)
{
	HRESULT									hResult;

	hResult = S_OK;
	switch (commandID)
	{
		case -1:
			// loop through buttons
			//		for buttons in CLSID_CommonButtons
			//			if up, QueryStatus for up state and update it
			//			
			//		for buttons in fCommandCategory, update with QueryStatus of fCommandTarget
			break;
		case 1:
			// forward
			hResult = SetState(&CLSID_CommonButtons, gForwardCommandID, newValue ? TBSTATE_ENABLED : 0);
			break;
		case 2:
			// back
			hResult = SetState(&CLSID_CommonButtons, gBackCommandID, newValue ? TBSTATE_ENABLED : 0);
			break;
	}
	return hResult;
}

HRESULT CInternetToolbar::CreateAndInitBandProxy()
{
	CComPtr<IServiceProvider>				serviceProvider;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED (hResult))
		return hResult;
	hResult = serviceProvider->QueryService(SID_IBandProxy, IID_IBandProxy, (void **)&fBandProxy);
	if (FAILED (hResult))
	{
		hResult = CreateBandProxy(IID_IBandProxy, (void **)&fBandProxy);
		if (FAILED (hResult))
			return hResult;
		hResult = fBandProxy->SetSite(fSite);
		if (FAILED (hResult))
			return hResult;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::HasFocusIO()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::TranslateAcceleratorIO(LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = m_hWnd;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ShowDW(BOOL fShow)
{
	CComPtr<IDockingWindow>		dockingWindow;
	HRESULT						hResult;

	// show the bar here
	hResult = ReserveBorderSpace();
	hResult = fMenuBar->QueryInterface(IID_IDockingWindow, (void **)&dockingWindow);
	hResult = dockingWindow->ShowDW(fShow);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::CloseDW(DWORD dwReserved)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetClassID(CLSID *pClassID)
{
	if (pClassID == NULL)
		return E_POINTER;
	*pClassID = CLSID_InternetToolbar;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::InitNew()
{
	CComPtr<IShellMenu>						menuBar;
	CComPtr<IUnknown>						logoBar;
	CComPtr<IUnknown>						toolsBar;
	CComPtr<IUnknown>						navigationBar;
	CComPtr<IOleWindow>						menuOleWindow;
	CComPtr<IOleWindow>						toolbarOleWindow;
	CComPtr<IOleWindow>						navigationOleWindow;
	HRESULT									hResult;

	hResult = CreateMenuBar(&menuBar);
	if (FAILED(hResult))
		return hResult;
	AddDockItem(menuBar, ITBBID_MENUBAND, CDockSite::ITF_NOTITLE | CDockSite::ITF_NEWBANDALWAYS | CDockSite::ITF_GRIPPERALWAYS);
	hResult = menuBar->QueryInterface(IID_IOleWindow, (void **)&menuOleWindow);
	hResult = menuOleWindow->GetWindow(&fMenuBandWindow);
	fMenuBar.Attach(menuBar.Detach());					// transfer the ref count

	hResult = CreateBrandBand(&logoBar);
	if (FAILED(hResult))
		return hResult;
	AddDockItem(logoBar, ITBBID_BRANDBAND, CDockSite::ITF_NOGRIPPER | CDockSite::ITF_NOTITLE | CDockSite::ITF_FIXEDSIZE);
	fLogoBar.Attach(logoBar.Detach());					// transfer the ref count

	hResult = CreateToolsBar(&toolsBar);
	if (FAILED(hResult))
		return hResult;
	AddDockItem(toolsBar, ITBBID_TOOLSBAND, CDockSite::ITF_NOTITLE | CDockSite::ITF_NEWBANDALWAYS);
	fControlsBar.Attach(toolsBar.Detach());					// transfer the ref count
	hResult = fControlsBar->QueryInterface(IID_IOleWindow, (void **)&toolbarOleWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = toolbarOleWindow->GetWindow(&fToolbarWindow);
	if (FAILED(hResult))
		return hResult;

	hResult = CreateAddressBand(&navigationBar);
	if (FAILED(hResult))
		return hResult;
	AddDockItem(navigationBar, ITBBID_ADDRESSBAND, CDockSite::ITF_NEWBANDALWAYS);
	hResult = navigationBar->QueryInterface(IID_IOleWindow, (void **)&navigationOleWindow);
	hResult = navigationOleWindow->GetWindow(&fNavigationWindow);
	fNavigationBar.Attach(navigationBar.Detach());

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	if (IsEqualIID(*pguidCmdGroup, CGID_PrivCITCommands))
	{
		while (cCmds != 0)
		{
			switch (prgCmds->cmdID)
			{
				case ITID_TEXTLABELS:		// Text Labels state
					prgCmds->cmdf = OLECMDF_SUPPORTED;
					break;
				case ITID_TOOLBARBANDSHOWN:	// toolbar visibility
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case ITID_ADDRESSBANDSHOWN:	// address bar visibility
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case ITID_LINKSBANDSHOWN:	// links bar visibility
					prgCmds->cmdf = 0;
					break;
				case ITID_MENUBANDSHOWN:	// Menubar band visibility
					prgCmds->cmdf = 0;
					break;
				case ITID_AUTOHIDEENABLED:	// Auto hide enabled/disabled
					prgCmds->cmdf = 0;
					break;
				case ITID_CUSTOMIZEENABLED:	// customize enabled
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					break;
				case ITID_TOOLBARLOCKED:	// lock toolbars
					prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
					if (fLocked)
						prgCmds->cmdf |= OLECMDF_LATCHED;
					break;
				default:
					prgCmds->cmdf = 0;
					break;
			}
			prgCmds++;
			cCmds--;
		}
		return S_OK;
	}
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	if (IsEqualIID(*pguidCmdGroup, CGID_PrivCITCommands))
	{
		switch (nCmdID)
		{
			case 1:
				// what do I do here?
				return S_OK;
			case ITID_TEXTLABELS:
				// toggle text labels
				return S_OK;
			case ITID_TOOLBARBANDSHOWN:
				// toggle toolbar band visibility
				return S_OK;
			case ITID_ADDRESSBANDSHOWN:
				// toggle address band visibility
				return S_OK;
			case ITID_LINKSBANDSHOWN:
				// toggle links band visibility
				return S_OK;
			case ITID_CUSTOMIZEENABLED:
				// run customize
				return S_OK;
			case ITID_TOOLBARLOCKED:
				return LockUnlockToolbars(!fLocked);
		}
	}
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetTypeInfoCount(UINT *pctinfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	HRESULT									hResult;

	switch(dispIdMember)
	{
		case DISPID_BEFORENAVIGATE:
			hResult = S_OK;
			break;
		case DISPID_DOWNLOADCOMPLETE:
			hResult = S_OK;
			break;
		case DISPID_COMMANDSTATECHANGE:
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			if (pDispParams->rgvarg[0].vt != VT_BOOL || pDispParams->rgvarg[1].vt != VT_I4)
				return E_INVALIDARG;
			return CommandStateChanged(pDispParams->rgvarg[0].boolVal == VARIANT_FALSE ? false : true,
										pDispParams->rgvarg[1].lVal);
		case DISPID_DOWNLOADBEGIN:
			hResult = S_OK;
			break;
		case DISPID_NAVIGATECOMPLETE2:
			hResult = S_OK;
			break;
		case DISPID_DOCUMENTCOMPLETE:
			hResult = S_OK;
			break;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetCommandTarget(IUnknown *theTarget, GUID *category, long param14)
{
	HRESULT									hResult;

	fCommandTarget.Release();
	hResult = theTarget->QueryInterface(IID_IOleCommandTarget, (void **)&fCommandTarget);
	if (FAILED(hResult))
		return hResult;
	fCommandCategory = *category;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Unknown1()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddButtons(const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddString(const GUID *pguidCmdGroup, HINSTANCE param10, LPCTSTR param14, long *param18)
{
	long									result;

	result = (long)::SendMessage(fToolbarWindow, TB_ADDSTRING, (WPARAM)param10, (LPARAM)param14);
	*param18 = result;
	if (result == -1)
		return E_FAIL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetButton(long paramC, long param10, long param14)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetState(const GUID *pguidCmdGroup, long commandID, long *theState)
{
	if (theState == NULL)
		return E_POINTER;
	// map the command id
	*theState = (long)::SendMessage(fToolbarWindow, TB_GETSTATE, commandID, 0);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetState(const GUID *pguidCmdGroup, long commandID, long theState)
{
	// map the command id
	::SendMessage(fToolbarWindow, TB_SETSTATE, commandID, MAKELONG(theState, 0));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddBitmap(const GUID *pguidCmdGroup, long param10, long buttonCount, TBADDBITMAP *lParam, long *newIndex, COLORREF param20)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBitmapSize(long *paramC)
{
	if (paramC == NULL)
		return E_POINTER;
	*paramC = MAKELONG(24, 24);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SendToolbarMsg(const GUID *pguidCmdGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetImageList(const GUID *pguidCmdGroup, HIMAGELIST param10, HIMAGELIST param14, HIMAGELIST param18)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ModifyButton(long paramC, long param10, long param14)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetSite(IUnknown *pUnkSite)
{
	CComPtr<IBrowserService>				browserService;
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IOleWindow>						oleWindow;
	HWND									ownerWindow;
	HWND									dockContainer;
	HRESULT									hResult;

	if (pUnkSite == NULL)
	{
		hResult = AtlUnadvise(fSite, DIID_DWebBrowserEvents, fAdviseCookie);
		::DestroyWindow(fMainReBar);
		DestroyWindow();
		fSite.Release();
	}
	else
	{
		hResult = pUnkSite->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
		if (FAILED(hResult))
			return hResult;
		hResult = oleWindow->GetWindow(&ownerWindow);
		if (FAILED(hResult))
			return hResult;
		if (ownerWindow == NULL)
			return E_FAIL;
		fSite = pUnkSite;
		dockContainer = SHCreateWorkerWindow(0, ownerWindow, 0, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, 0);
		if (dockContainer == NULL)
			return E_FAIL;
		SubclassWindow(dockContainer);
		fMainReBar = CreateWindow(REBARCLASSNAME, _T(""), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT |
							RBS_BANDBORDERS | RBS_REGISTERDROP | RBS_AUTOSIZE | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_TOP,
							0, 0, 700, 60, dockContainer, NULL, hExplorerInstance, NULL);
		if (fMainReBar == NULL)
			return E_FAIL;
		hResult = pUnkSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
		hResult = serviceProvider->QueryService(SID_SShellBrowser, IID_IBrowserService, (void **)&browserService);
		hResult = AtlAdvise(browserService, (IDispatch *)this, DIID_DWebBrowserEvents, &fAdviseCookie);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetSite(REFIID riid, void **ppvSite)
{
	if (ppvSite == NULL)
		return E_POINTER;
	if (fSite.p != NULL)
		return fSite->QueryInterface(riid, ppvSite);
	*ppvSite = NULL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	CComPtr<IServiceProvider>				serviceProvider;
	HRESULT									hResult;

	if (IsEqualIID(guidService, IID_IBandSite))
		return this->QueryInterface(riid, ppvObject);
	if (IsEqualIID(guidService, SID_IBandProxy))
	{
		if (fBandProxy.p == NULL)
		{
			hResult = CreateAndInitBandProxy();
			if (FAILED (hResult))
				return hResult;
		}
		return fBandProxy->QueryInterface(riid, ppvObject);
	}
	hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED (hResult))
		return hResult;
	return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
	CComPtr<IWinEventHandler>				menuWinEventHandler;
	HRESULT									hResult;

	if (fMenuBar.p != NULL)
	{
		hResult = fMenuBar->QueryInterface(IID_IWinEventHandler, (void **)&menuWinEventHandler);
		return menuWinEventHandler->OnWinEvent(fMenuBandWindow, uMsg, wParam, lParam, theResult);
	}
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::IsWindowOwner(HWND hWnd)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddBand(IUnknown *punk)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::EnumBands(UINT uBand, DWORD *pdwBandID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
	if (ppstb == NULL)
		return E_POINTER;
	if (dwBandID == ITBBID_MENUBAND && fMenuBar.p != NULL)
		return fMenuBar->QueryInterface(IID_IDeskBand, (void **)ppstb);
	if (dwBandID == ITBBID_BRANDBAND && fLogoBar.p != NULL)
		return fLogoBar->QueryInterface(IID_IDeskBand, (void **)ppstb);
	*ppstb = NULL;
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::RemoveBand(DWORD dwBandID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBandObject(DWORD dwBandID, REFIID riid, void **ppv)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
	return E_NOTIMPL;
}

LRESULT CInternetToolbar::OnTravelBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IWebBrowser>					webBrowser;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED (hResult))
		return 0;
	hResult = serviceProvider->QueryService(SID_SShellBrowser, IID_IWebBrowser, (void **)&webBrowser);
	if (FAILED (hResult))
		return 0;
	hResult = webBrowser->GoBack();
	return 1;
}

LRESULT CInternetToolbar::OnTravelForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IWebBrowser>					webBrowser;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED (hResult))
		return 0;
	hResult = serviceProvider->QueryService(SID_SShellBrowser, IID_IWebBrowser, (void **)&webBrowser);
	if (FAILED (hResult))
		return 0;
	hResult = webBrowser->GoForward();
	return 1;
}

LRESULT CInternetToolbar::OnUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (FAILED (hResult))
		return hResult;
	hResult = oleCommandTarget->Exec(&CGID_ShellBrowser, IDM_GOTO_UPONELEVEL, 0, NULL, NULL);
	return 1;
}

LRESULT CInternetToolbar::OnSearch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IObjectWithSite>				objectWithSite;
	CComPtr<IContextMenu>					contextMenu;
	CMINVOKECOMMANDINFO						commandInfo;
	char									*searchGUID = "{169A0691-8DF9-11d1-A1C4-00C04FD75D13}";
	HRESULT									hResult;

	// TODO: Query shell if this command is enabled first

	memset(&commandInfo, 0, sizeof(commandInfo));
	commandInfo.cbSize = sizeof(commandInfo);
	commandInfo.hwnd = m_hWnd;
	commandInfo.lpParameters = searchGUID;
	commandInfo.nShow = SW_SHOWNORMAL;

	hResult = CoCreateInstance(CLSID_ShellSearchExt, NULL, COM_RIGHTS_EXECUTE, IID_IContextMenu, (void **)&contextMenu);
	if (FAILED (hResult))
		return 0;
	hResult = contextMenu->QueryInterface(IID_IObjectWithSite, (void **)&objectWithSite);
	if (FAILED (hResult))
		return 0;
	hResult = objectWithSite->SetSite(fSite);
	if (FAILED (hResult))
		return 0;
	hResult = contextMenu->InvokeCommand(&commandInfo);
	hResult = objectWithSite->SetSite(NULL);
	return 0;
}

LRESULT CInternetToolbar::OnFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (FAILED (hResult))
		return hResult;
	hResult = oleCommandTarget->Exec(&CGID_Explorer, 0x23, 0, NULL, NULL);
	return 1;
}

LRESULT CInternetToolbar::OnForwardToCommandTarget(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
	HRESULT									hResult;

	if (fCommandTarget.p != NULL)
	{
		hResult = fCommandTarget->Exec(&fCommandCategory, wID, 0, NULL, NULL);
	}
	return 1;
}

LRESULT CInternetToolbar::OnMenuDropDown(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
	CComPtr<IServiceProvider>				serviceProvider;
	CComPtr<IBrowserService>				browserService;
	CComPtr<IOleCommandTarget>				commandTarget;
	CComPtr<ITravelLog>						travelLog;
	NMTOOLBAR								*notifyInfo;
	RECT									bounds;
	HMENU									newMenu;
	TPMPARAMS								params;
	int										selectedItem;
	VARIANT									parmIn;
	OLECMD									commandInfo;
	HRESULT									hResult;

	notifyInfo = (NMTOOLBAR *)pNMHDR;
	if (notifyInfo->hdr.hwndFrom != fToolbarWindow)
	{
		// not from the toolbar, keep looking for a message handler
		bHandled = FALSE;
		return 0;
	}
	SendMessage(fToolbarWindow, TB_GETRECT, notifyInfo->iItem, (LPARAM)&bounds);
	::MapWindowPoints(fToolbarWindow, NULL, (POINT *)&bounds, 2);
	switch (notifyInfo->iItem)
	{
		case gBackCommandID:
			newMenu = CreatePopupMenu();
			hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
			hResult = serviceProvider->QueryService(SID_SShellBrowser, IID_IBrowserService, (void **)&browserService);
			hResult = browserService->GetTravelLog(&travelLog);
			hResult = travelLog->InsertMenuEntries(browserService, newMenu, 0, 1, 9, TLMENUF_BACK);
			hResult = browserService->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
			commandInfo.cmdID = 0x1d;
			hResult = commandTarget->QueryStatus(&CGID_Explorer, 1, &commandInfo, NULL);
			if ((commandInfo.cmdf & (OLECMDF_ENABLED | OLECMDF_LATCHED)) == OLECMDF_ENABLED && travelLog->CountEntries(browserService) > 1)
			{
				AppendMenu(newMenu, MF_SEPARATOR, -1, L"");
				AppendMenu(newMenu, MF_STRING /* | MF_OWNERDRAW */, IDM_EXPLORERBAR_HISTORY, L"&History\tCtrl+H");
			}
			params.cbSize = sizeof (params);
			params.rcExclude = bounds;
			selectedItem = TrackPopupMenuEx(newMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
									bounds.left, bounds.bottom, m_hWnd, &params);
			if (selectedItem == IDM_EXPLORERBAR_HISTORY)
			{
				parmIn.vt = VT_I4;
				parmIn.lVal = 1;
				Exec(&CGID_Explorer, 0x1d, 2, &parmIn, NULL);
			}
			else if (selectedItem != 0)
				hResult = travelLog->Travel(browserService, -selectedItem);
			DestroyMenu(newMenu);
			break;
		case gForwardCommandID:
			newMenu = CreatePopupMenu();
			hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
			hResult = serviceProvider->QueryService(SID_SShellBrowser, IID_IBrowserService, (void **)&browserService);
			hResult = browserService->GetTravelLog(&travelLog);
			hResult = travelLog->InsertMenuEntries(browserService, newMenu, 0, 1, 9, TLMENUF_FORE);
			hResult = browserService->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
			commandInfo.cmdID = 0x1d;
			hResult = commandTarget->QueryStatus(&CGID_Explorer, 1, &commandInfo, NULL);
			if ((commandInfo.cmdf & (OLECMDF_ENABLED | OLECMDF_LATCHED)) == OLECMDF_ENABLED && travelLog->CountEntries(browserService) > 1)
			{
				AppendMenu(newMenu, MF_SEPARATOR, -1, L"");
				AppendMenu(newMenu, MF_STRING /* | MF_OWNERDRAW */, IDM_EXPLORERBAR_HISTORY, L"&History\tCtrl+H");
			}
			params.cbSize = sizeof (params);
			params.rcExclude = bounds;
			selectedItem = TrackPopupMenuEx(newMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
									bounds.left, bounds.bottom, m_hWnd, &params);
			if (selectedItem == IDM_EXPLORERBAR_HISTORY)
			{
				parmIn.vt = VT_I4;
				parmIn.lVal = 1;
				Exec(&CGID_Explorer, 0x1d, 2, &parmIn, NULL);
			}
			else if (selectedItem != 0)
				hResult = travelLog->Travel(browserService, -selectedItem);
			DestroyMenu(newMenu);
			break;
		case gViewsCommandID:
			VARIANT						inValue;
			CComVariant					outValue;
			HRESULT						hResult;

			inValue.vt = VT_INT_PTR;
			inValue.pintVal = (INT *)&bounds;

			hResult = fCommandTarget->Exec(&fCommandCategory, 0x7031, 1, &inValue, &outValue);
			// pvaOut is VT_I4 with value 0x403
			break;
	}
	return TBDDRET_DEFAULT;
}

LRESULT CInternetToolbar::OnQueryInsert(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
	return 1;
}

LRESULT CInternetToolbar::OnQueryDelete(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
	return 1;
}

LRESULT CInternetToolbar::OnNavigateCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	CComPtr<IWinEventHandler>				winEventHandler;
	LRESULT									theResult;
	HRESULT									hResult;

	hResult = fNavigationBar->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
	hResult = winEventHandler->OnWinEvent(m_hWnd, uMsg, wParam, lParam, &theResult);
	if (SUCCEEDED(hResult))
		return theResult;
	bHandled = FALSE;
	return 0;
}

LRESULT CInternetToolbar::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	HMENU									contextMenuBar;
	HMENU									contextMenu;
	POINT									clickLocation;
	int										command;
	RBHITTESTINFO							hitTestInfo;
	REBARBANDINFO							rebarBandInfo;
	int										bandID;

	clickLocation.x = LOWORD(lParam);
	clickLocation.y = HIWORD(lParam);
	hitTestInfo.pt = clickLocation;
	ScreenToClient(&hitTestInfo.pt);
	SendMessage(fMainReBar, RB_HITTEST, 0, (LPARAM)&hitTestInfo);
	if (hitTestInfo.iBand == -1)
		return 0;
	rebarBandInfo.cbSize = sizeof(rebarBandInfo);
	rebarBandInfo.fMask = RBBIM_ID;
	SendMessage(fMainReBar, RB_GETBANDINFO, hitTestInfo.iBand, (LPARAM)&rebarBandInfo);
	bandID = rebarBandInfo.wID;
	contextMenuBar = LoadMenu(hExplorerInstance, MAKEINTRESOURCE(IDM_CABINET_CONTEXTMENU));
	contextMenu = GetSubMenu(contextMenuBar, 0);
	switch (bandID)
	{
		case ITBBID_MENUBAND:	// menu band
			DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
			break;
		case ITBBID_BRANDBAND:	// brand band
			DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
			break;
		case ITBBID_TOOLSBAND:	// tools band
			DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
			break;
		case ITBBID_ADDRESSBAND:	// navigation band
			DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
			DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
			break;
		default:
			break;
	}
	// TODO: use GetSystemMetrics(SM_MENUDROPALIGNMENT) to determine menu alignment
	command = TrackPopupMenu(contextMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
				clickLocation.x, clickLocation.y, 0, m_hWnd, NULL);
	switch (command)
	{
		case IDM_TOOLBARS_STANDARDBUTTONS:	// standard buttons
			break;
		case IDM_TOOLBARS_ADDRESSBAR:	// address bar
			break;
		case IDM_TOOLBARS_LINKSBAR:	// links
			break;
		case IDM_TOOLBARS_LOCKTOOLBARS:	// lock the toolbars
			LockUnlockToolbars(!fLocked);
			break;
		case IDM_TOOLBARS_CUSTOMIZE:	// customize
			SendMessage(fToolbarWindow, TB_CUSTOMIZE, 0, 0);
			break;
	}
	return 1;
}

LRESULT CInternetToolbar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if (wParam != SIZE_MINIMIZED)
	{
		::SetWindowPos(fMainReBar, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	}
	return 1;
}

LRESULT CInternetToolbar::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if ((short)lParam != HTCLIENT || (HWND)wParam != m_hWnd)
	{
		bHandled = FALSE;
		return 0;
	}
	SetCursor(LoadCursor(NULL, IDC_SIZENS));
	return 1;
}

LRESULT CInternetToolbar::OnTipText(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
	CComPtr<IBrowserService>				browserService;
	CComPtr<ITravelLog>						travelLog;
	TOOLTIPTEXT								*pTTTW;
	UINT									nID;
	wchar_t									tempString[300];
	HRESULT									hResult;

	pTTTW = (TOOLTIPTEXT *)pNMHDR;
	if ((pTTTW->uFlags & TTF_IDISHWND) != 0)
		nID = ::GetDlgCtrlID((HWND)pNMHDR->idFrom);
	else
		nID = (UINT)pNMHDR->idFrom;

	if (nID != 0)
	{
		if (nID == gBackCommandID || nID == gForwardCommandID)
		{
			// TODO: Should this call QueryService?
			hResult = fSite->QueryInterface(IID_IBrowserService, (void **)&browserService);
			hResult = browserService->GetTravelLog(&travelLog);
			hResult = travelLog->GetToolTipText(browserService, nID == gBackCommandID ? TLOG_BACK : TLOG_FORE,
								0, tempString, 299);
			if (FAILED(hResult))
			{
				bHandled = FALSE;
				return 0;
			}
		}
		else
			tempString[0] = 0;
		wcsncpy (pTTTW->szText, tempString, sizeof (pTTTW->szText) / sizeof (wchar_t));
		::SetWindowPos (pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		return 0;
	}
	return 0;
}

LRESULT CInternetToolbar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	NMHDR									*notifyHeader;
	CComPtr<IWinEventHandler>				menuWinEventHandler;
	LRESULT									theResult;
	HRESULT									hResult;

	notifyHeader = (NMHDR *)lParam;
	if (fMenuBar.p != NULL && notifyHeader->hwndFrom == fMenuBandWindow)
	{
		hResult = fMenuBar->QueryInterface(IID_IWinEventHandler, (void **)&menuWinEventHandler);
		hResult = menuWinEventHandler->OnWinEvent(fMenuBandWindow, uMsg, wParam, lParam, &theResult);
		return theResult;
	}
	if (fNavigationBar.p != NULL && notifyHeader->hwndFrom == fNavigationWindow)
	{
		hResult = fNavigationBar->QueryInterface(IID_IWinEventHandler, (void **)&menuWinEventHandler);
		hResult = menuWinEventHandler->OnWinEvent(m_hWnd, uMsg, wParam, lParam, &theResult);
		return theResult;
	}
	return 0;
}

HRESULT CreateInternetToolbar(REFIID riid, void **ppv)
{
	CComObject<CInternetToolbar>			*theToolbar;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theToolbar = new CComObject<CInternetToolbar>);
	if (theToolbar == NULL)
		return E_OUTOFMEMORY;
	hResult = theToolbar->QueryInterface (riid, (void **)ppv);
	if (FAILED (hResult))
	{
		delete theToolbar;
		return hResult;
	}
	return S_OK;
}
