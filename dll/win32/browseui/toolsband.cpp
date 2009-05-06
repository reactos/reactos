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
Implements the toolbar band of a cabinet window
*/
#include "precomp.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

/*
TODO:
  **Fix GetBandInfo to calculate size correctly
*/
static const int gBackCommandID = 0xa121;
static const int gForwardCommandID = 0xa122;
static const int gUpLevelCommandID = 0xa022;
static const int gSearchCommandID = 1003;
static const int gFoldersCommandID = 1004;
static const int gMoveToCommandID = 0x701f;
static const int gCopyToCommandID = 0x701e;
static const int gDeleteCommandID = 0x7011;
static const int gUndoCommandID = 0x701b;
static const int gViewsCommandID = 0x7031;
static const int gStopCommandID = 1010;
static const int gRefreshCommandID = 0xa220;
static const int gHomeCommandID = 1012;
static const int gMapDriveCommandID = 41089;
static const int gDisconnectCommandID = 41090;
static const int gFavoritesCommandID = 1015;
static const int gHistoryCommandID = 1016;
static const int gFullScreenCommandID = 1017;
static const int gPropertiesCommandID = 0x7013;
static const int gCutCommandID = 0x7018;
static const int gCopyCommandID = 0x7019;
static const int gPasteCommandID = 0x701a;
static const int gFolderOptionsCommandID = 41251;

class CToolsBand :
	public CWindowImpl<CToolsBand, CWindow, CControlWinTraits>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IDeskBand,
	public IObjectWithSite,
	public IInputObject,
	public IPersistStream
{
private:
	IDockingWindowSite			*fDockSite;
	GUID						fExecCommandCategory;
	CComPtr<IOleCommandTarget>	fExecCommandTarget;
public:
	CToolsBand();
	~CToolsBand();
public:
	// *** IDeskBand methods ***
	virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi);

	// *** IObjectWithSite methods ***
	virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite);
	virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

	// *** IDockingWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE CloseDW(unsigned long dwReserved);
	virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);
	virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);

	// *** IInputObject methods ***
	virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
	virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);
	virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistStream methods ***
	virtual HRESULT STDMETHODCALLTYPE IsDirty();
	virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
	virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

	// message handlers
	LRESULT OnGetButtonInfo(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);

BEGIN_MSG_MAP(CToolsBand)
//	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
//	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	NOTIFY_HANDLER(0, TBN_GETBUTTONINFO, OnGetButtonInfo)
END_MSG_MAP()

BEGIN_COM_MAP(CToolsBand)
	COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
	COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
	COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
	COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
	COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
	COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
END_COM_MAP()
};

CToolsBand::CToolsBand()
{
	fDockSite = NULL;
}

CToolsBand::~CToolsBand()
{
	if (fDockSite)
		fDockSite->Release();
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
	if (pdbi->dwMask & DBIM_MINSIZE)
	{
		pdbi->ptMinSize.x = 400;
		pdbi->ptMinSize.y = 38;
	}
	if (pdbi->dwMask & DBIM_MAXSIZE)
	{
		pdbi->ptMaxSize.x = 0;
		pdbi->ptMaxSize.y = 0;
	}
	if (pdbi->dwMask & DBIM_INTEGRAL)
	{
		pdbi->ptIntegral.x = 0;
		pdbi->ptIntegral.y = 0;
	}
	if (pdbi->dwMask & DBIM_ACTUAL)
	{
		pdbi->ptActual.x = 400;
		pdbi->ptActual.y = 38;
	}
	if (pdbi->dwMask & DBIM_TITLE)
		wcscpy(pdbi->wszTitle, L"");
	if (pdbi->dwMask & DBIM_MODEFLAGS)
		pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
	if (pdbi->dwMask & DBIM_BKCOLOR)
		pdbi->crBkgnd = 0;
	return S_OK;
}

static const int backImageIndex = 0;
static const int forwardImageIndex = 1;
static const int favoritesImageIndex = 2;
// 3
// 4
static const int cutImageIndex = 5;
static const int copyImageIndex = 6;
static const int pasteImageIndex = 7;
static const int undoImageIndex = 8;
static const int redoImageIndex = 9;
static const int deleteImageIndex = 10;
// 11
// 12
// 13
// 14
static const int propertiesImageIndex = 15;
// 16
static const int searchImageIndex = 17;
// 18
// 19
// 20
// 21
static const int viewsImageIndex = 22;
// 23
// 24
// 25
// 26
// 27
static const int upImageIndex = 28;
static const int mapDriveImageIndex = 29;
static const int disconnectImageIndex = 30;
// 31
static const int viewsAltImageIndex = 32;		// same image as viewsImageIndex
// 33
// 34
// 35
// 36
// 37
static const int viewsAlt2ImageIndex = 38;		// same image as viewsAltImageIndex & viewsImageIndex
// 39
// 40
// 41
// 42
static const int foldersImageIndex = 43;
static const int moveToImageIndex = 44;
static const int copyToImageIndex = 45;
static const int folderOptionsImageIndex = 46;

const int numShownButtons = 13;
const int numHiddenButtons = 13;
TBBUTTON tbButtonsAdd[numShownButtons + numHiddenButtons] = 
{
	{backImageIndex, gBackCommandID, TBSTATE_ENABLED, BTNS_DROPDOWN | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)_T("Back")},
	{forwardImageIndex, gForwardCommandID, TBSTATE_ENABLED, BTNS_DROPDOWN, {0}, 0, (INT_PTR)_T("Forward")},
	{upImageIndex, gUpLevelCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Up")},
	{6, -1, TBSTATE_ENABLED, BTNS_SEP},
	{searchImageIndex, gSearchCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)_T("Search")},
	{foldersImageIndex, gFoldersCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)_T("Folders")},
	{6, -1, TBSTATE_ENABLED, BTNS_SEP},
	{moveToImageIndex, gMoveToCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Move To")},
	{copyToImageIndex, gCopyToCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Copy To")},
	{deleteImageIndex, gDeleteCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Delete")},
	{undoImageIndex, gUndoCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Undo")},
	{6, -1, TBSTATE_ENABLED, BTNS_SEP},
	{viewsImageIndex, gViewsCommandID, TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0}, 0, (INT_PTR)_T("Views")},

	{0, gStopCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Stop")},
	{0, gRefreshCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Refresh")},
	{0, gHomeCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Home")},
	{mapDriveImageIndex, gMapDriveCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Map Drive")},
	{disconnectImageIndex, gDisconnectCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Disconnect")},
	{favoritesImageIndex, gFavoritesCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)_T("Favorites")},
	{0, gHistoryCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("History")},
	{0, gFullScreenCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Full Screen")},
	{propertiesImageIndex, gPropertiesCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Properties")},
	{cutImageIndex, gCutCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Cut")},
	{copyImageIndex, gCopyCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Copy")},
	{pasteImageIndex, gPasteCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Paste")},
	{folderOptionsImageIndex, gFolderOptionsCommandID, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)_T("Folder Options")},
};

HRESULT STDMETHODCALLTYPE CToolsBand::SetSite(IUnknown* pUnkSite)
{
	HWND					parentWindow;
	IOleWindow				*oleWindow;
	HWND					toolbar;
	HRESULT					hResult;

	if (fDockSite != NULL)
		fDockSite->Release();
	if (pUnkSite == NULL)
		return S_OK;
	hResult = pUnkSite->QueryInterface(IID_IDockingWindowSite, (void **)&fDockSite);
	if (FAILED(hResult))
		return hResult;
	parentWindow = NULL;
	hResult = pUnkSite->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
	if (SUCCEEDED(hResult))
	{
		oleWindow->GetWindow(&parentWindow);
		oleWindow->Release();
	}
	if (!::IsWindow(parentWindow))
		return E_FAIL;

	toolbar = CreateWindowEx(TBSTYLE_EX_DOUBLEBUFFER, TOOLBARCLASSNAME, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
					WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT |
					CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP, 0, 0, 500, 20, parentWindow, NULL, hExplorerInstance, 0);
	if (toolbar == NULL)
		return E_FAIL;
	SubclassWindow(toolbar);

	SendMessage(WM_USER + 100, GetSystemMetrics(SM_CXEDGE) / 2, 0);
	SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(TB_SETMAXTEXTROWS, 1, 0);
	SendMessage(TB_SETEXTENDEDSTYLE, TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);

	HINSTANCE shell32Instance = GetModuleHandle(_T("shell32.dll"));
	HBITMAP imageBitmap = (HBITMAP)LoadImage(shell32Instance, MAKEINTRESOURCE(214), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);

	DIBSECTION bitmapInfo;
	GetObject(imageBitmap, sizeof(bitmapInfo), &bitmapInfo);
	HIMAGELIST imageList = ImageList_Create(bitmapInfo.dsBm.bmHeight, bitmapInfo.dsBm.bmHeight, ILC_COLOR32, 4, 4);

	ImageList_Add(imageList, imageBitmap, NULL);
	DeleteObject(imageBitmap);

	SendMessage(TB_SETIMAGELIST, 0, (LPARAM)imageList);

	SendMessage(TB_ADDBUTTONS, numShownButtons, (LPARAM)&tbButtonsAdd);

	return hResult;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetSite(REFIID riid, void **ppvSite)
{
	if (fDockSite == NULL)
		return E_FAIL;
	return fDockSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = m_hWnd;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ContextSensitiveHelp(BOOL fEnterMode)
{
	
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::CloseDW(unsigned long dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ResizeBorderDW(const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ShowDW(BOOL fShow)
{
    if (m_hWnd)
    {
        if (fShow)
            ShowWindow(SW_SHOW);
        else
            ShowWindow(SW_HIDE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::HasFocusIO()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetClassID(CLSID *pClassID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

LRESULT CToolsBand::OnGetButtonInfo(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
	TBNOTIFY *pTBntf = (TBNOTIFY *)pNMHDR;

	if (pTBntf->iItem >= 0 && pTBntf->iItem < (numShownButtons + numHiddenButtons))
	{
		pTBntf->tbButton = tbButtonsAdd[pTBntf->iItem];
		return TRUE;
	}
	else
		return FALSE;
	return 0;
}

HRESULT CreateToolsBar(REFIID riid, void **ppv)
{
	CComObject<CToolsBand>					*theMenuBar;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theMenuBar = new CComObject<CToolsBand>);
	if (theMenuBar == NULL)
		return E_OUTOFMEMORY;
	hResult = theMenuBar->QueryInterface (riid, (void **)ppv);
	if (FAILED (hResult))
	{
		delete theMenuBar;
		return hResult;
	}
	return S_OK;
}

