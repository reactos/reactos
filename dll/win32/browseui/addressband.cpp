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
Implements the navigation band of the cabinet window
*/
#include "precomp.h"
#include "browseui_resource.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "addressband.h"

/*
TODO:
****Add command handler for show/hide Go button to OnWinEvent
****Add tooltip notify handler
  **Properly implement GetBandInfo
  **Add support for showing/hiding Go button
  **Fix so Go button will be shown/hidden properly on load
  **Add correct text to Go button
  **Implement TranslateAcceleratorIO
    Implement Exec
    Implement QueryService
    Implement Load
    Implement Save
*/

CAddressBand::CAddressBand()
{
	fEditControl = NULL;
	fGoButton = NULL;
	fComboBox = NULL;
	fGoButtonShown = false;
}

CAddressBand::~CAddressBand()
{
}

void CAddressBand::FocusChange(BOOL bFocus)
{
//	m_bFocus = bFocus;

	//Inform the input object site that the focus has changed.
	if (fSite)
	{
#if 0
		fSite->OnFocusChangeIS((IDockingWindow *)this, bFocus);
#endif
	}
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
	if (pdbi->dwMask & DBIM_MINSIZE)
	{
		pdbi->ptMinSize.x = 400;
		pdbi->ptMinSize.y = 22;
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
		pdbi->ptActual.y = 22;
	}
	if (pdbi->dwMask & DBIM_TITLE)
		wcscpy(pdbi->wszTitle, L"Address");
	if (pdbi->dwMask & DBIM_MODEFLAGS)
		pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
	if (pdbi->dwMask & DBIM_BKCOLOR)
		pdbi->crBkgnd = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::SetSite(IUnknown *pUnkSite)
{
	CComPtr<IShellService>					shellService;
	CComPtr<IUnknown>						offset34;
	HWND									parentWindow;
	IOleWindow								*oleWindow;
	HWND									toolbar;
	static const TBBUTTON					buttonInfo[] = {0, 1, TBSTATE_ENABLED, 0};
	HIMAGELIST								normalImagelist;
	HIMAGELIST								hotImageList;
	HINSTANCE								shellInstance;
	HRESULT									hResult;

	fSite.Release();
	if (pUnkSite == NULL)
		return S_OK;
	hResult = pUnkSite->QueryInterface(IID_IDockingWindowSite, (void **)&fSite);
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

	toolbar = CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEX, NULL, WS_CHILD | WS_VISIBLE |
					WS_CLIPCHILDREN | WS_TABSTOP | CCS_NODIVIDER | CCS_NOMOVEY,
					0, 0, 500, 250, parentWindow, (HMENU)0xa205, hExplorerInstance, 0);
	if (toolbar == NULL)
		return E_FAIL;
	SubclassWindow(toolbar);
	SendMessage(CBEM_SETEXTENDEDSTYLE, CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT, CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT);
	fEditControl = (HWND)SendMessage(CBEM_GETEDITCONTROL, 0, 0);
	fComboBox = (HWND)SendMessage(CBEM_GETCOMBOCONTROL, 0, 0);
#if 1
	hResult = CoCreateInstance(CLSID_AddressEditBox, NULL, COM_RIGHTS_EXECUTE, IID_IAddressEditBox, (void **)&fAddressEditBox);
	if (FAILED(hResult))
		return hResult;
#else
	// instantiate new version
#endif
	hResult = fAddressEditBox->QueryInterface(IID_IShellService, (void **)&shellService);
	if (FAILED(hResult))
		return hResult;
	hResult = fAddressEditBox->Init(toolbar, fEditControl, 8, pUnkSite /*(IAddressBand *)this*/ );
	if (FAILED(hResult))
		return hResult;
	hResult = shellService->SetOwner(pUnkSite);
	if (FAILED(hResult))
		return hResult;

	// TODO: properly initialize this from registry
	fGoButtonShown = true;

	shellInstance = GetModuleHandle(_T("shell32.dll"));
	normalImagelist = ImageList_LoadImage(shellInstance, MAKEINTRESOURCE(IDB_GOBUTTON_NORMAL), 20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	hotImageList = ImageList_LoadImage(shellInstance, MAKEINTRESOURCE(IDB_GOBUTTON_HOT), 20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

	fGoButton = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_LIST |
						TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
						0, 0, 0, 0, m_hWnd, NULL, hExplorerInstance, NULL);
	SendMessage(fGoButton, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(fGoButton, TB_SETMAXTEXTROWS, 1, 0);
	SendMessage(fGoButton, TB_SETIMAGELIST, 0, (LPARAM)normalImagelist);
	SendMessage(fGoButton, TB_SETHOTIMAGELIST, 0, (LPARAM)hotImageList);
	SendMessage(fGoButton, TB_ADDSTRING, (WPARAM)hExplorerInstance, IDS_GOBUTTONLABEL);
	SendMessage(fGoButton, TB_ADDBUTTONS, 1, (LPARAM)&buttonInfo);

	return hResult;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSite(REFIID riid, void **ppvSite)
{
	if (fSite == NULL)
		return E_FAIL;
	return fSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = m_hWnd;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::CloseDW(unsigned long dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ShowDW(BOOL fShow)
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

HRESULT STDMETHODCALLTYPE CAddressBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	CComPtr<IOleCommandTarget>				oleCommandTarget;
	HRESULT									hResult;

	hResult = fAddressEditBox->QueryInterface(IID_IOleCommandTarget, (void **)&oleCommandTarget);
	if (FAILED(hResult))
		return hResult;
	return oleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	// incomplete
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::HasFocusIO()
{
	if (GetFocus() == fEditControl || SendMessage(CB_GETDROPPEDSTATE, 0, 0))
		return S_OK;
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
	// incomplete
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	CComPtr<IInputObjectSite>				inputObjectSite;
	HRESULT									hResult;

	if (fActivate)
	{
		hResult = fSite->QueryInterface(IID_IInputObjectSite, (void **)&inputObjectSite);
		if (FAILED(hResult))
			return hResult;
		hResult = inputObjectSite->OnFocusChangeIS((IDeskBand *)this, fActivate);
		SetFocus();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
	CComPtr<IWinEventHandler>				winEventHandler;
	HRESULT									hResult;

	switch (uMsg)
	{
		case WM_WININICHANGE:
			break;
		case WM_COMMAND:
			if (wParam == IDM_TOOLBARS_GOBUTTON)
			{
				// toggle whether the Go button is displayed
				// setting is Yes or No, stored in key "Software\Microsoft\Internet Explorer\Main" in value ShowGoButton
				// broadcast change notification to all explorer windows
			}
			break;
	}
	hResult = fAddressEditBox->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
	if (FAILED(hResult))
		return hResult;
	return winEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsWindowOwner(HWND hWnd)
{
	CComPtr<IWinEventHandler>				winEventHandler;
	HRESULT									hResult;

	hResult = fAddressEditBox->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
	if (FAILED(hResult))
		return hResult;
	return winEventHandler->IsWindowOwner(hWnd);
}

HRESULT STDMETHODCALLTYPE CAddressBand::FileSysChange(long param8, long paramC)
{
	CComPtr<IAddressBand>					addressBand;
	HRESULT									hResult;

	hResult = fAddressEditBox->QueryInterface(IID_IAddressBand, (void **)&addressBand);
	if (FAILED(hResult))
		return hResult;
	return addressBand->FileSysChange(param8, paramC);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Refresh(long param8)
{
	CComPtr<IAddressBand>					addressBand;
	HRESULT									hResult;

	hResult = fAddressEditBox->QueryInterface(IID_IAddressBand, (void **)&addressBand);
	if (FAILED(hResult))
		return hResult;
	return addressBand->Refresh(param8);
}

HRESULT STDMETHODCALLTYPE CAddressBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetClassID(CLSID *pClassID)
{
	if (pClassID == NULL)
		return E_POINTER;
	*pClassID = CLSID_SH_AddressBand;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Load(IStream *pStm)
{
	// incomplete
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Save(IStream *pStm, BOOL fClearDirty)
{
	// incomplete
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	// incomplete
	return E_NOTIMPL;
}

LRESULT CAddressBand::OnNotifyClick(WPARAM wParam, NMHDR *notifyHeader, BOOL &bHandled)
{
	if (notifyHeader->hwndFrom == fGoButton)
	{
		SendMessage(fEditControl, WM_KEYDOWN, 13, 0);
		SendMessage(fEditControl, WM_KEYUP, 13, 0);
	}
	return 0;
}

LRESULT CAddressBand::OnTipText(UINT idControl, NMHDR *notifyHeader, BOOL &bHandled)
{
	if (notifyHeader->hwndFrom == fGoButton)
	{
		// TODO
		// Go to "destination path"
	}
	return 0;
}

LRESULT CAddressBand::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	POINT									pt;
	POINT									ptOrig;
	HWND									parentWindow;
	LRESULT									result;

	if (fGoButtonShown == false)
	{
		bHandled = FALSE;
		return 0;
	}
	pt.x = 0;
	pt.y = 0;
	parentWindow = GetParent();
	::MapWindowPoints(m_hWnd, parentWindow, &pt, 1);
	OffsetWindowOrgEx((HDC)wParam, pt.x, pt.y, &ptOrig);
	result = SendMessage(parentWindow, WM_ERASEBKGND, wParam, 0);
	SetWindowOrgEx((HDC)wParam, ptOrig.x, ptOrig.y, NULL);
	if (result == 0)
	{
		bHandled = FALSE;
		return 0;
	}
	return result;
}

LRESULT CAddressBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	RECT									goButtonBounds;
	RECT									buttonBounds;
	long									buttonWidth;
	long									buttonHeight;
	RECT									comboBoxBounds;
	long									newHeight;
	long									newWidth;

	if (fGoButtonShown == false)
	{
		bHandled = FALSE;
		return 0;
	}
	newHeight = HIWORD(lParam);
	newWidth = LOWORD(lParam);
	SendMessage(fGoButton, TB_GETITEMRECT, 0, (LPARAM)&buttonBounds);
	buttonWidth = buttonBounds.right - buttonBounds.left;
	buttonHeight = buttonBounds.bottom - buttonBounds.top;
	DefWindowProc(WM_SIZE, wParam, MAKELONG(newWidth - buttonWidth - 2, newHeight));
	::GetWindowRect(fComboBox, &comboBoxBounds);
	::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
					buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	goButtonBounds.left = newWidth - buttonWidth;
	goButtonBounds.top = 0;
	goButtonBounds.right = newWidth - buttonWidth;
	goButtonBounds.bottom = newHeight;
	InvalidateRect(&goButtonBounds, TRUE);
	SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
	return 0;
}

LRESULT CAddressBand::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	RECT									goButtonBounds;
	RECT									buttonBounds;
	long									buttonWidth;
	long									buttonHeight;
	RECT									comboBoxBounds;
	WINDOWPOS								positionInfoCopy;
	long									newHeight;
	long									newWidth;

	if (fGoButtonShown == false)
	{
		bHandled = FALSE;
		return 0;
	}
	positionInfoCopy = *(WINDOWPOS *)lParam;
	newHeight = positionInfoCopy.cy;
	newWidth = positionInfoCopy.cx;
	SendMessage(fGoButton, TB_GETITEMRECT, 0, (LPARAM)&buttonBounds);
	buttonWidth = buttonBounds.right - buttonBounds.left;
	buttonHeight = buttonBounds.bottom - buttonBounds.top;
	positionInfoCopy.cx = newWidth - 2 - buttonWidth;
	DefWindowProc(WM_WINDOWPOSCHANGING, wParam, (LPARAM)&positionInfoCopy);
	::GetWindowRect(fComboBox, &comboBoxBounds);
	::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
					buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	goButtonBounds.left = newWidth - buttonWidth;
	goButtonBounds.top = 0;
	goButtonBounds.right = newWidth - buttonWidth;
	goButtonBounds.bottom = newHeight;
	InvalidateRect(&goButtonBounds, TRUE);
	SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
	return 0;
}

HRESULT CreateAddressBand(REFIID riid, void **ppv)
{
	CComObject<CAddressBand>				*theMenuBar;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theMenuBar = new CComObject<CAddressBand>);
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
