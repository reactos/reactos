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
This class knows how to contain base bar site in a cabinet window.
*/
#include "precomp.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

/*
Base bar that contains a vertical or horizontal explorer band. It also
provides resizing abilities.
*/
/*
TODO:
  **Make base bar support resizing
    Add context menu for base bar
    Fix base bar to correctly initialize fVertical field
    Fix base bar to correctly reposition its base bar site when resized

*/

class CBaseBar :
	public CWindowImpl<CBaseBar, CWindow, CControlWinTraits>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IInputObjectSite,
	public IOleCommandTarget,
	public IServiceProvider,
	public IInputObject,
	public IDeskBar,
	public IDockingWindow,
	public IPersistStream,
	public IPersistStreamInit,
	public IPersistPropertyBag,
	public IObjectWithSite
{
public:
	CComPtr<IUnknown>						fSite;
	CComPtr<IUnknown>						fClient;
	HWND									fClientWindow;
	bool									fVertical;
	bool									fVisible;
	int										fNeededSize;		// width or height

	// used by resize tracking loop
	bool									fTracking;
	POINT									fLastLocation;
public:
	CBaseBar();
	~CBaseBar();
public:
	HRESULT ReserveBorderSpace();

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

	// *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus);

	// *** IOleCommandTarget specific methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	// *** IServiceProvider methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

	// *** IInputObject methods ***
	// forward the methods to the contained active bar
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

	// *** IDeskBar methods ***
	virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
	virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
	virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(LPRECT prc);

	// *** IDockingWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
	virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
	virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

	// *** IObjectWithSite methods ***
	virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
	virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistStream methods ***
	virtual HRESULT STDMETHODCALLTYPE IsDirty();
	virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
	virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

	// *** IPersistStreamInit methods ***
	virtual HRESULT STDMETHODCALLTYPE InitNew();

	// *** IPersistPropertyBag methods ***
	virtual HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog);
	virtual HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

	// message handlers
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

DECLARE_WND_CLASS_EX(_T("BaseBar"), 0, COLOR_3DFACE)

BEGIN_MSG_MAP(CBaseBar)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
	MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
	MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
END_MSG_MAP()

BEGIN_COM_MAP(CBaseBar)
	COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDockingWindow)
	COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
	COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
	COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
	COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
	COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IDeskBar)
	COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
	COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
	COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStreamInit, IPersistStreamInit)
	COM_INTERFACE_ENTRY_IID(IID_IPersistPropertyBag, IPersistPropertyBag)
END_COM_MAP()
};

CBaseBar::CBaseBar()
{
	fClientWindow = NULL;
	fVertical = true;
	fVisible = false;
	fNeededSize = 200;
	fTracking = false;
}

CBaseBar::~CBaseBar()
{
}

HRESULT CBaseBar::ReserveBorderSpace()
{
	CComPtr<IDockingWindowSite>				dockingWindowSite;
	RECT									availableBorderSpace;
	RECT									neededBorderSpace;
	HRESULT									hResult;

	hResult = fSite->QueryInterface(IID_IDockingWindowSite, (void **)&dockingWindowSite);
	if (FAILED(hResult))
		return hResult;
	hResult = dockingWindowSite->GetBorderDW((IDeskBar *)this, &availableBorderSpace);
	if (FAILED(hResult))
		return hResult;
	memset(&neededBorderSpace, 0, sizeof(neededBorderSpace));
	if (fVisible)
	{
		if (fVertical)
			neededBorderSpace.left = fNeededSize + GetSystemMetrics(SM_CXFRAME);
		else
			neededBorderSpace.bottom = fNeededSize + GetSystemMetrics(SM_CXFRAME);
	}
	hResult = dockingWindowSite->SetBorderSpaceDW((IDeskBar *)this, &neededBorderSpace);
	if (FAILED(hResult))
		return hResult;
	return S_OK;
}

// current bar size is stored in the registry under
// key=HKCU\Software\Microsoft\Internet Explorer\Explorer Bars
// value=current bar GUID
// result is 8 bytes of binary data, 2 longs. First is the size, second is reserved and will always be 0
/*HRESULT CBaseBar::StopCurrentBar()
{
	CComPtr<IOleCommandTarget>				commandTarget;
	HRESULT									hResult;

	if (fCurrentBar.p != NULL)
	{
		hResult = fCurrentBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
		hResult = commandTarget->Exec(NULL, 0x17, 0, NULL, NULL);
	}
	// hide the current bar
	memcpy(&fCurrentActiveClass, &GUID_NULL, sizeof(fCurrentActiveClass));
	return S_OK;
}*/

HRESULT STDMETHODCALLTYPE CBaseBar::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = m_hWnd;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
	// forward to owner
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
	{
	}
	else if (IsEqualIID(*pguidCmdGroup, IID_IDeskBarClient))
	{
		switch (nCmdID)
		{
			case 0:
				// hide current band
				break;
			case 2:
				break;
			case 3:
				break;
		}
	}
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	CComPtr<IServiceProvider>				serviceProvider;
	HRESULT									hResult;

	if (fSite == NULL)
		return E_FAIL;
	hResult = fSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED(hResult))
		return hResult;
	// called for SID_STopLevelBrowser, IID_IBrowserService to find top level browser
	// called for SID_IWebBrowserApp, IID_IConnectionPointContainer
	// connection point called for DIID_DWebBrowserEvents2 to establish connection
	return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CBaseBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	// forward to contained bar
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::HasFocusIO()
{
	// forward to contained bar
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
	// forward to contained bar
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::SetClient(IUnknown *punkClient)
{
	CComPtr<IOleWindow>						oleWindow;
	HWND									ownerWindow;
	HRESULT									hResult;

	if (punkClient == NULL)
		fClient.Release();
	else
	{
		hResult = punkClient->QueryInterface(IID_IUnknown, (void **)&fClient);
		if (FAILED(hResult))
			return hResult;
		hResult = fSite->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
		if (FAILED(hResult))
			return hResult;
		hResult = oleWindow->GetWindow(&ownerWindow);
		if (FAILED(hResult))
			return hResult;
		Create(ownerWindow, 0, NULL, WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
		ReserveBorderSpace();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetClient(IUnknown **ppunkClient)
{
	if (ppunkClient == NULL)
		return E_POINTER;
	*ppunkClient = fClient;
	if (fClient.p != NULL)
		fClient.p->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::OnPosRectChangeDB(LPRECT prc)
{
	if (prc == NULL)
		return E_POINTER;
	if (fVertical)
		fNeededSize = prc->right - prc->left;
	else
		fNeededSize = prc->bottom - prc->top;
	ReserveBorderSpace();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ShowDW(BOOL fShow)
{
	fVisible = fShow ? true : false;
	ReserveBorderSpace();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::CloseDW(DWORD dwReserved)
{
	fVisible = false;
	ReserveBorderSpace();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
	ReserveBorderSpace();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::SetSite(IUnknown *pUnkSite)
{
	fSite = pUnkSite;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetSite(REFIID riid, void **ppvSite)
{
	if (ppvSite == NULL)
		return E_POINTER;
	*ppvSite = fSite;
	if (fSite.p != NULL)
		fSite.p->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetClassID(CLSID *pClassID)
{
	if (pClassID == NULL)
		return E_POINTER;
	// TODO: what class to return here?
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	if (pcbSize == NULL)
		return E_POINTER;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::InitNew()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
	return E_NOTIMPL;
}

LRESULT CBaseBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
/*	CComPtr<IOleWindow>						oleWindow;
	RECT									clientRect;
	HRESULT									hResult;

	if (fClientWindow == NULL && fClient.p != NULL)
	{
		hResult = fClient->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
		hResult = oleWindow->GetWindow(&fClientWindow);
	}
	if (fClientWindow != NULL)
	{
		GetClientRect(&clientRect);
		::SetWindowPos(fClientWindow, NULL, clientRect.left, clientRect.top, clientRect.right - clientRect.left - GetSystemMetrics(SM_CXFRAME),
					clientRect.bottom - clientRect.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
	}*/
	return 0;
}

LRESULT CBaseBar::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	if ((short)lParam != HTCLIENT || (HWND)wParam != m_hWnd)
	{
		bHandled = FALSE;
		return 0;
	}
	if (fVertical)
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
	else
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
	return 1;
}

LRESULT CBaseBar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	CComPtr<IWinEventHandler>				winEventHandler;
	LRESULT									result;
	HRESULT									hResult;

	result = 0;
	if (fClient.p != NULL)
	{
		hResult = fClient->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
		if (SUCCEEDED(hResult) && winEventHandler.p != NULL)
			hResult = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
	}
	return result;
}

LRESULT CBaseBar::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	SetCapture();
	fTracking = true;
	fLastLocation.x = (short)LOWORD(lParam);
	fLastLocation.y = (short)HIWORD(lParam);
	return 0;
}

LRESULT CBaseBar::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	ReleaseCapture();
	fTracking = false;
	return 0;
}

LRESULT CBaseBar::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	POINT									newLocation;
	int										delta;

	if (fTracking)
	{
		newLocation.x = (short)LOWORD(lParam);
		newLocation.y = (short)HIWORD(lParam);
		if (fVertical)
			delta = newLocation.x - fLastLocation.x;
		else
			delta = newLocation.y - fLastLocation.y;
		

		fLastLocation = newLocation;
	}
	return 0;
}

LRESULT CBaseBar::OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	fTracking = false;
	return 0;
}

LRESULT CBaseBar::OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	fTracking = false;
	return 0;
}

HRESULT CreateBaseBar(REFIID riid, void **ppv)
{
	CComObject<CBaseBar>					*theBaseBar;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theBaseBar = new CComObject<CBaseBar>);
	if (theBaseBar == NULL)
		return E_OUTOFMEMORY;
	hResult = theBaseBar->QueryInterface (riid, (void **)ppv);
	if (FAILED (hResult))
	{
		delete theBaseBar;
		return hResult;
	}
	return S_OK;
}
