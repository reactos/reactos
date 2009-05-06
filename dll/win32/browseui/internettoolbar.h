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

#ifndef _internettoolbar_h
#define _internettoolbar_h

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

class CMenuCallback :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IShellMenuCallback
{
private:
	CComPtr<IShellMenu>				fFavoritesMenu;
public:
	CMenuCallback();
	~CMenuCallback();

	HRESULT STDMETHODCALLTYPE GetObject(LPSMDATA psmd, REFIID riid, void **ppvObject);
public:
	// *** IShellMenuCallback methods ***
	virtual HRESULT STDMETHODCALLTYPE CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BEGIN_COM_MAP(CMenuCallback)
	COM_INTERFACE_ENTRY(IShellMenuCallback)
END_COM_MAP()
};

class CInternetToolbar :
	public CWindowImpl<CInternetToolbar, CWindow, CControlWinTraits>,
	public CComCoClass<CInternetToolbar, &CLSID_InternetToolbar>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IInputObject,
	public IDockingWindow,
	public IPersistStreamInit,
	public IOleCommandTarget,
	public IDispatch,
	public IExplorerToolbar,
	public IShellChangeNotify,
	public IObjectWithSite,
	public IServiceProvider,
	public IWinEventHandler,
	public IBandSite
{
public:
	CComPtr<IUnknown>						fSite;				// our site
	HWND									fMainReBar;			// rebar for top of window
	bool									fLocked;			// is bar locked to prevent changes?
	CComPtr<IShellMenu>						fMenuBar;			// the menu rebar
	HWND									fMenuBandWindow;
	HWND									fNavigationWindow;
	CComPtr<IUnknown>						fLogoBar;			// the reactos logo
	CComPtr<IUnknown>						fControlsBar;		// navigation controls
	CComPtr<IUnknown>						fNavigationBar;		// address bar
	CComObject<CMenuCallback>				fMenuCallback;
	CComPtr<IOleCommandTarget>				fCommandTarget;
	GUID									fCommandCategory;
	HWND									fToolbarWindow;
	DWORD									fAdviseCookie;
	CComPtr<IBandProxy>						fBandProxy;
public:
	CInternetToolbar();
	~CInternetToolbar();
	void AddDockItem(IUnknown *newItem, int bandID, int flags);
	HRESULT ReserveBorderSpace();
	HRESULT CreateMenuBar(IShellMenu **menuBar);
	HRESULT CreateBrandBand(IUnknown **logoBar);
	HRESULT CreateToolsBar(IUnknown **toolsBar);
	HRESULT CreateAddressBand(IUnknown **toolsBar);
	HRESULT LockUnlockToolbars(bool locked);
	HRESULT CommandStateChanged(bool newValue, int commandID);
	HRESULT CreateAndInitBandProxy();
public:
    // *** IInputObject specific methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

	// *** IDockingWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
	virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
	virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistStreamInit methods ***
	virtual HRESULT STDMETHODCALLTYPE IsDirty();
	virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
	virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);
	virtual HRESULT STDMETHODCALLTYPE InitNew();

	// *** IOleCommandTarget methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	// *** IDispatch methods ***
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

	// *** IExplorerToolbar methods ***
	virtual HRESULT STDMETHODCALLTYPE SetCommandTarget(IUnknown *theTarget, GUID *category, long param14);
	virtual HRESULT STDMETHODCALLTYPE Unknown1();
	virtual HRESULT STDMETHODCALLTYPE AddButtons(const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons);
	virtual HRESULT STDMETHODCALLTYPE AddString(const GUID *pguidCmdGroup, HINSTANCE param10, LPCTSTR param14, long *param18);
	virtual HRESULT STDMETHODCALLTYPE GetButton(long paramC, long param10, long param14);
	virtual HRESULT STDMETHODCALLTYPE GetState(const GUID *pguidCmdGroup, long commandID, long *theState);
	virtual HRESULT STDMETHODCALLTYPE SetState(const GUID *pguidCmdGroup, long commandID, long theState);
	virtual HRESULT STDMETHODCALLTYPE AddBitmap(const GUID *pguidCmdGroup, long param10, long buttonCount, TBADDBITMAP *lParam, long *newIndex, COLORREF param20);
	virtual HRESULT STDMETHODCALLTYPE GetBitmapSize(long *paramC);
	virtual HRESULT STDMETHODCALLTYPE SendToolbarMsg(const GUID *pguidCmdGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual HRESULT STDMETHODCALLTYPE SetImageList(const GUID *pguidCmdGroup, HIMAGELIST param10, HIMAGELIST param14, HIMAGELIST param18);
	virtual HRESULT STDMETHODCALLTYPE ModifyButton(long paramC, long param10, long param14);

    // *** IShellChangeNotify methods ***
    virtual HRESULT STDMETHODCALLTYPE OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

	// *** IObjectWithSite methods ***
	virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
	virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

	// *** IServiceProvider methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

	// *** IWinEventHandler methods ***
	virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
	virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

	// *** IBandSite specific methods ***
	virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown *punk);
	virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD *pdwBandID);
	virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
	virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
	virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
	virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, void **ppv);
	virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
	virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);

	// message handlers
	LRESULT OnTravelBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnTravelForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnSearch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnForwardToCommandTarget(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
	LRESULT OnMenuDropDown(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);
	LRESULT OnQueryInsert(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);
	LRESULT OnQueryDelete(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);
	LRESULT OnNavigateCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnTipText(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

BEGIN_MSG_MAP(CInternetToolbar)
	COMMAND_ID_HANDLER(gBackCommandID, OnTravelBack)
	COMMAND_ID_HANDLER(gForwardCommandID, OnTravelForward)
	COMMAND_ID_HANDLER(gUpLevelCommandID, OnUpLevel)
	COMMAND_ID_HANDLER(gSearchCommandID, OnSearch)
	COMMAND_ID_HANDLER(gFoldersCommandID, OnFolders)
	COMMAND_RANGE_HANDLER(0x7000, 0x7fff, OnForwardToCommandTarget)
	NOTIFY_HANDLER(0, TBN_DROPDOWN, OnMenuDropDown)
	NOTIFY_HANDLER(0, TBN_QUERYINSERT, OnQueryInsert)
	NOTIFY_HANDLER(0, TBN_QUERYDELETE, OnQueryDelete)
	MESSAGE_HANDLER(WM_COMMAND, OnNavigateCommand)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
	NOTIFY_CODE_HANDLER(TTN_NEEDTEXT, OnTipText)
	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
END_MSG_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_INTERNETTOOLBAR)
DECLARE_NOT_AGGREGATABLE(CInternetToolbar)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CInternetToolbar)
	COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
	COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
	COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
	COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
//	COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStreamInit, IPersistStreamInit)
	COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
	COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
	COM_INTERFACE_ENTRY_IID(IID_IExplorerToolbar, IExplorerToolbar)
	COM_INTERFACE_ENTRY_IID(IID_IShellChangeNotify, IShellChangeNotify)
	COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
	COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
	COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
	COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
END_COM_MAP()
};

#endif // _internettoolbar_h
