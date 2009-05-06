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
Base bar that contains a vertical or horizontal explorer band. It also
provides resizing abilities.
*/
#include "precomp.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

/*
TODO:
****Fix so an already created bar will be detected and just shown instead of added again
****When a new bar is added, initiate a resize
  **Add owner draw for base bar
  **Make label text in base bar always draw in black
  **Make base bar show close box
  **Create close toolbar button
  **Fix to delete all CBarInfo on deletion

*/

class CBaseBarSite :
	public CWindowImpl<CBaseBarSite, CWindow, CControlWinTraits>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
//	public IDockingWindowSite,
	public IInputObject,
	public IServiceProvider,
	public IWinEventHandler,
	public IInputObjectSite,
	public IDeskBarClient,
	public IOleCommandTarget,
	public IBandSite,
//	public IBandSiteHelper,
//	public IExplorerToolbar,
	public IPersistStream
{
private:
	class CBarInfo
	{
	public:
		CComPtr<IUnknown>					fTheBar;
		CLSID								fBarClass;				// class of active bar
		DWORD								fBandID;
		
	};
	CBarInfo								*fCurrentActiveBar;		// 
//	HWND									fRebarWindow;			// rebar for top of window
	CComPtr<IUnknown>						fDeskBarSite;
	DWORD									fNextBandID;
public:
	CBaseBarSite();
	~CBaseBarSite();
private:
	HRESULT InsertBar(IUnknown *newBar);

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

	// *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

	// *** IServiceProvider methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

	// *** IWinEventHandler methods ***
	virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
	virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

	// *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus);

    // *** IDeskBarClient methods ***
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);

	// *** IOleCommandTarget methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	// *** IBandSite specific methods ***
	virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown *punk);
	virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD *pdwBandID);
	virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
	virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
	virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
	virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, void **ppv);
	virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
	virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistStream methods ***
	virtual HRESULT STDMETHODCALLTYPE IsDirty();
	virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
	virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

	// message handlers
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

BEGIN_MSG_MAP(CBaseBarSite)
	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
END_MSG_MAP()

BEGIN_COM_MAP(CBaseBarSite)
	COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
//	COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
	COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
	COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
	COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
	COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
	COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
	COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
	COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
//	COM_INTERFACE_ENTRY_IID(IID_IBandSiteHelper, IBandSiteHelper)
//	COM_INTERFACE_ENTRY_IID(IID_IExplorerToolbar, IExplorerToolbar)
	COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
END_COM_MAP()
};

CBaseBarSite::CBaseBarSite()
{
	fCurrentActiveBar = NULL;
	fNextBandID = 1;
}

CBaseBarSite::~CBaseBarSite()
{
}

HRESULT CBaseBarSite::InsertBar(IUnknown *newBar)
{
	CComPtr<IPersist>						persist;
	CComPtr<IObjectWithSite>				site;
	CComPtr<IOleWindow>						oleWindow;
	CComPtr<IDeskBand>						deskBand;
	CComPtr<IDockingWindow>					dockingWindow;
	CBarInfo								*newInfo;
	REBARBANDINFO							bandInfo;
	DESKBANDINFO							deskBandInfo;
	DWORD									thisBandID;
	HRESULT									hResult;

	hResult = newBar->QueryInterface(IID_IPersist, (void **)&persist);
	if (FAILED(hResult))
		return hResult;
	hResult = newBar->QueryInterface(IID_IObjectWithSite, (void **)&site);
	if (FAILED(hResult))
		return hResult;
	hResult = newBar->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = newBar->QueryInterface(IID_IDeskBand, (void **)&deskBand);
	if (FAILED(hResult))
		return hResult;
	hResult = newBar->QueryInterface(IID_IDockingWindow, (void **)&dockingWindow);
	if (FAILED(hResult))
		return hResult;
	hResult = site->SetSite((IOleWindow *)this);
	if (FAILED(hResult))
		return hResult;
	newInfo = new CBarInfo;
	if (newInfo == NULL)
		return E_OUTOFMEMORY;
	thisBandID = fNextBandID++;
	newInfo->fTheBar = newBar;
	newInfo->fBandID = thisBandID;
	hResult = persist->GetClassID(&newInfo->fBarClass);
	deskBandInfo.dwMask = DBIM_MINSIZE | DBIM_ACTUAL | DBIM_TITLE;
	deskBandInfo.wszTitle[0] = 0;
	hResult = deskBand->GetBandInfo(0, 0, &deskBandInfo);
	memset(&bandInfo, 0, sizeof(bandInfo));
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_TEXT | RBBIM_LPARAM | RBBIM_ID;
	bandInfo.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
	bandInfo.lpText = deskBandInfo.wszTitle;
	hResult = oleWindow->GetWindow(&bandInfo.hwndChild);
	bandInfo.cxMinChild = 200; //deskBandInfo.ptMinSize.x;
	bandInfo.cyMinChild = 200; //deskBandInfo.ptMinSize.y;
	bandInfo.cx = 0;
	bandInfo.wID = thisBandID;
	bandInfo.cyChild = -1; //deskBandInfo.ptActual.y;
	bandInfo.cyMaxChild = 32000;
	bandInfo.cyIntegral = 1;
	bandInfo.cxIdeal = 0; //deskBandInfo.ptActual.x;
	bandInfo.lParam = (LPARAM)newInfo;
	SendMessage(RB_INSERTBAND, -1, (LPARAM)&bandInfo);
	hResult = dockingWindow->ShowDW(TRUE);		// this call is what makes the tree fill with contents
	if (FAILED(hResult))
		return hResult;
	// for now
	fCurrentActiveBar = newInfo;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetWindow(HWND *lphwnd)
{
	if (lphwnd == NULL)
		return E_POINTER;
	*lphwnd = m_hWnd;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::HasFocusIO()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	CComPtr<IServiceProvider>				serviceProvider;
	HRESULT									hResult;

	if (fDeskBarSite == NULL)
		return E_FAIL;
	hResult = fDeskBarSite->QueryInterface(IID_IServiceProvider, (void **)&serviceProvider);
	if (FAILED(hResult))
		return hResult;
	// called for SID_STopLevelBrowser, IID_IBrowserService to find top level browser
	// called for SID_IWebBrowserApp, IID_IConnectionPointContainer
	// connection point called for DIID_DWebBrowserEvents2 to establish connection
	return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
	CComPtr<IDeskBar>						deskBar;
	CComPtr<IWinEventHandler>				winEventHandler;
	NMHDR									*notifyHeader;
	RECT									newBounds;
	HRESULT									hResult;

	hResult = S_OK;
	if (uMsg == WM_NOTIFY)
	{
		notifyHeader = (NMHDR *)lParam;
		if (notifyHeader->hwndFrom == m_hWnd && notifyHeader->code == RBN_AUTOSIZE)
		{
			hResult = fDeskBarSite->QueryInterface(IID_IDeskBar, (void **)&deskBar);
			GetClientRect(&newBounds);
			hResult = deskBar->OnPosRectChangeDB(&newBounds);
		}
	}
	if (fCurrentActiveBar != NULL)
	{
		hResult = fCurrentActiveBar->fTheBar->QueryInterface(IID_IWinEventHandler, (void **)&winEventHandler);
		if (SUCCEEDED(hResult) && winEventHandler.p != NULL)
			hResult = winEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::IsWindowOwner(HWND hWnd)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetDeskBarSite(IUnknown *punkSite)
{
	CComPtr<IOleWindow>						oleWindow;
	HWND									ownerWindow;
	HRESULT									hResult;

	if (punkSite == NULL)
		fDeskBarSite.Release();
	else
	{
		hResult = punkSite->QueryInterface(IID_IOleWindow, (void **)&oleWindow);
		if (FAILED(hResult))
			return hResult;
		hResult = punkSite->QueryInterface(IID_IUnknown, (void **)&fDeskBarSite);
		if (FAILED(hResult))
			return hResult;
		hResult = oleWindow->GetWindow(&ownerWindow);
		if (FAILED(hResult))
			return hResult;
		m_hWnd = CreateWindow(REBARCLASSNAME, _T(""), WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS |
					WS_CLIPCHILDREN |
					RBS_VARHEIGHT | RBS_REGISTERDROP | RBS_AUTOSIZE | RBS_VERTICALGRIPPER | RBS_DBLCLKTOGGLE |
					CCS_LEFT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE, 0, 0, 0, 0, ownerWindow, NULL,
					hExplorerInstance, NULL);
		SendMessage(RB_SETTEXTCOLOR, 0, CLR_DEFAULT);
		SendMessage(RB_SETBKCOLOR, 0, CLR_DEFAULT);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetModeDBC(DWORD dwMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::UIActivateDBC(DWORD dwState)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetSize(DWORD dwWhich, LPRECT prc)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
	{
		switch (nCmdID)
		{
			case 1:		// insert a new band
				if (pvaIn->vt != VT_UNKNOWN)
					return E_INVALIDARG;
				return InsertBar(pvaIn->punkVal);
		}
	}
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::AddBand(IUnknown *punk)
{
	return InsertBar(punk);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::EnumBands(UINT uBand, DWORD *pdwBandID)
{
	if (uBand == -1)
	{
		*pdwBandID = (DWORD)SendMessage(RB_GETBANDCOUNT, 0, 0);
		return S_OK;
	}
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::RemoveBand(DWORD dwBandID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetBandObject(DWORD dwBandID, REFIID riid, void **ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
	if (pbsinfo == NULL)
		return E_POINTER;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
	if (pbsinfo == NULL)
		return E_POINTER;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetClassID(CLSID *pClassID)
{
	if (pClassID == NULL)
		return E_POINTER;
	// TODO: what class to return here?
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	if (pcbSize == NULL)
		return E_POINTER;
	return E_NOTIMPL;
}

LRESULT CBaseBarSite::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	NMHDR						*notifyHeader;

	notifyHeader = (NMHDR *)lParam;
	if (notifyHeader->hwndFrom == m_hWnd)
	{
	}
	return 0;
}

HRESULT CreateBaseBarSite(REFIID riid, void **ppv)
{
	CComObject<CBaseBarSite>				*theBaseBarSite;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theBaseBarSite = new CComObject<CBaseBarSite>);
	if (theBaseBarSite == NULL)
		return E_OUTOFMEMORY;
	hResult = theBaseBarSite->QueryInterface (riid, (void **)ppv);
	if (FAILED (hResult))
	{
		delete theBaseBarSite;
		return hResult;
	}
	return S_OK;
}
