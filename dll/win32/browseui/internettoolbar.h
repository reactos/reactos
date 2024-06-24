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

#pragma once

#define ITBARSTREAM_SHELLBROWSER 0
#define ITBARSTREAM_WEBBROWSER 1
#define ITBARSTREAM_EXPLORER 2

static const int gSearchCommandID = 1003;
static const int gFoldersCommandID = 1004;
static const int gMoveToCommandID = FCIDM_SHVIEW_MOVETO;
static const int gCopyToCommandID = FCIDM_SHVIEW_COPYTO;
static const int gDeleteCommandID = FCIDM_SHVIEW_DELETE;
static const int gUndoCommandID = FCIDM_SHVIEW_UNDO;
static const int gViewsCommandID = FCIDM_SHVIEW_AUTOARRANGE;
static const int gStopCommandID = 1010;
static const int gHomeCommandID = 1012;
static const int gFavoritesCommandID = 1015;
static const int gHistoryCommandID = 1016;
static const int gFullScreenCommandID = 1017;
static const int gPropertiesCommandID = FCIDM_SHVIEW_PROPERTIES;
static const int gCutCommandID = FCIDM_SHVIEW_CUT;
static const int gCopyCommandID = FCIDM_SHVIEW_COPY;
static const int gPasteCommandID = FCIDM_SHVIEW_INSERT;

class CMenuCallback :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellMenuCallback
{
private:
    CComPtr<IShellMenu>             fFavoritesMenu;
public:
    CMenuCallback();
    virtual ~CMenuCallback();

    HRESULT STDMETHODCALLTYPE GetObject(LPSMDATA psmd, REFIID riid, void **ppvObject);
public:
    // *** IShellMenuCallback methods ***
    virtual HRESULT STDMETHODCALLTYPE CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BEGIN_COM_MAP(CMenuCallback)
    COM_INTERFACE_ENTRY_IID(IID_IShellMenuCallback, IShellMenuCallback)
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
    CComPtr<IUnknown>                       fSite;              // our site
    HWND                                    fMainReBar;         // rebar for top of window
    bool                                    fLocked;            // is bar locked to prevent changes?
    CComPtr<IShellMenu>                     fMenuBar;           // the menu rebar
    HWND                                    fMenuBandWindow;
    HWND                                    fNavigationWindow;
    CComPtr<IUnknown>                       fLogoBar;           // the reactos logo
    CComPtr<IUnknown>                       fControlsBar;       // navigation controls
    CComPtr<IUnknown>                       fNavigationBar;     // address bar
    CComPtr<CMenuCallback>                  fMenuCallback;
    CComPtr<IOleCommandTarget>              fCommandTarget;
    GUID                                    fCommandCategory;
    HWND                                    fToolbarWindow;
    DWORD                                   fAdviseCookie;
    CComPtr<IBandProxy>                     fBandProxy;
    BOOL                                    fSizing;
    POINT                                   fStartPosition;
    LONG                                    fStartHeight;
    ShellSettings                           *pSettings;
    BOOL                                    fIgnoreChanges;
public:
    CInternetToolbar();
    virtual ~CInternetToolbar();
    void AddDockItem(IUnknown *newItem, int bandID, int flags);
    HRESULT EnumBands(UINT Index, int *pBandId, IUnknown **ppUnkBand);
    HRESULT ReserveBorderSpace(LONG maxHeight = -1);
    HRESULT CreateMenuBar(IShellMenu **menuBar);
    HRESULT CreateToolsBar(IUnknown **toolsBar);
    HRESULT LockUnlockToolbars(bool locked);
    HRESULT CommandStateChanged(bool newValue, int commandID);
    HRESULT CreateAndInitBandProxy();
    HRESULT IsBandVisible(int BandID);
    HRESULT SetBandVisibility(int BandID, int Show);
    HRESULT ToggleBandVisibility(int BandID);
    HRESULT SetState(const GUID *pguidCmdGroup, long commandID, OLECMD* pcmd);
    void RefreshLockedToolbarState();
    HRESULT SetDirty();

    static HRESULT GetStream(UINT StreamFor, DWORD Stgm, IStream **ppS);

public:
    // *** IInputObject specific methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHOD(ShowDW)(BOOL fShow) override;
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStreamInit methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;
    STDMETHOD(InitNew)() override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override;
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    // *** IExplorerToolbar methods ***
    STDMETHOD(SetCommandTarget)(IUnknown *theTarget, GUID *category, long param14) override;
    STDMETHOD(Unknown1)() override;
    STDMETHOD(AddButtons)(const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons) override;
    STDMETHOD(AddString)(const GUID *pguidCmdGroup, HINSTANCE param10, LPCTSTR param14, long *param18) override;
    STDMETHOD(GetButton)(const GUID *pguidCmdGroup, long param10, long param14) override;
    STDMETHOD(GetState)(const GUID *pguidCmdGroup, long commandID, long *theState) override;
    STDMETHOD(SetState)(const GUID *pguidCmdGroup, long commandID, long theState) override;
    STDMETHOD(AddBitmap)(const GUID *pguidCmdGroup, long param10, long buttonCount, TBADDBITMAP *lParam, long *newIndex, COLORREF param20) override;
    STDMETHOD(GetBitmapSize)(long *paramC) override;
    STDMETHOD(SendToolbarMsg)(const GUID *pguidCmdGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *result) override;
    STDMETHOD(SetImageList)(const GUID *pguidCmdGroup, HIMAGELIST param10, HIMAGELIST param14, HIMAGELIST param18) override;
    STDMETHOD(ModifyButton)(const GUID *pguidCmdGroup, long param10, long param14) override;

    // *** IShellChangeNotify methods ***
    STDMETHOD(OnChange)(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IBandSite specific methods ***
    STDMETHOD(AddBand)(IUnknown *punk) override;
    STDMETHOD(EnumBands)(UINT uBand, DWORD *pdwBandID) override;
    STDMETHOD(QueryBand)(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName) override;
    STDMETHOD(SetBandState)(DWORD dwBandID, DWORD dwMask, DWORD dwState) override;
    STDMETHOD(RemoveBand)(DWORD dwBandID) override;
    STDMETHOD(GetBandObject)(DWORD dwBandID, REFIID riid, void **ppv) override;
    STDMETHOD(SetBandSiteInfo)(const BANDSITEINFO *pbsinfo) override;
    STDMETHOD(GetBandSiteInfo)(BANDSITEINFO *pbsinfo) override;

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
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnWinIniChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSettingsChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BEGIN_MSG_MAP(CInternetToolbar)
        COMMAND_ID_HANDLER(IDM_GOTO_BACK, OnTravelBack)
        COMMAND_ID_HANDLER(IDM_GOTO_FORWARD, OnTravelForward)
        COMMAND_ID_HANDLER(IDM_GOTO_UPONELEVEL, OnUpLevel)
        COMMAND_ID_HANDLER(gSearchCommandID, OnSearch)
        COMMAND_ID_HANDLER(gFoldersCommandID, OnFolders)
        COMMAND_RANGE_HANDLER(0x7000, 0x7fff, OnForwardToCommandTarget)
        NOTIFY_HANDLER(0, TBN_DROPDOWN, OnMenuDropDown)
        NOTIFY_HANDLER(0, TBN_QUERYINSERT, OnQueryInsert)
        NOTIFY_HANDLER(0, TBN_QUERYDELETE, OnQueryDelete)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        NOTIFY_CODE_HANDLER(TTN_NEEDTEXTW, OnTipText)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLUp)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_WININICHANGE, OnWinIniChange)
        MESSAGE_HANDLER(BWM_SETTINGCHANGE, OnSettingsChange)
    END_MSG_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_INTERNETTOOLBAR)
    DECLARE_NOT_AGGREGATABLE(CInternetToolbar)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CInternetToolbar)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
//        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
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
