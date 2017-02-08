/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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

class CCommonBrowser :
    public CComCoClass<CCommonBrowser, &CLSID_CCommonBrowser>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellBrowser,
    public IBrowserService3,
    public IServiceProvider,
    public IOleCommandTarget,
    public IDockingWindowSite,
    public IDockingWindowFrame,
    public IInputObjectSite,
    public IDropTarget,
    public IShellBrowserService
{
private:
public:
    CCommonBrowser();
    ~CCommonBrowser();

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

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
    virtual HRESULT STDMETHODCALLTYPE CanNavigateNow();
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
    virtual LPBASEBROWSERDATA STDMETHODCALLTYPE PutBaseBrowserData();
    virtual HRESULT STDMETHODCALLTYPE InitializeTravelLog(ITravelLog *ptl, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE SetTopBrowser();
    virtual HRESULT STDMETHODCALLTYPE Offline(int iCmd);
    virtual HRESULT STDMETHODCALLTYPE AllowViewResize(BOOL f);
    virtual HRESULT STDMETHODCALLTYPE SetActivateState(UINT u);
    virtual HRESULT STDMETHODCALLTYPE UpdateSecureLockIcon(int eSecureLock);
    virtual HRESULT STDMETHODCALLTYPE InitializeDownloadManager();
    virtual HRESULT STDMETHODCALLTYPE InitializeTransitionSite();
    virtual HRESULT STDMETHODCALLTYPE _Initialize(HWND hwnd, IUnknown *pauto);
    virtual HRESULT STDMETHODCALLTYPE _CancelPendingNavigationAsync();
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

    // *** IBrowserService3 methods ***
    virtual HRESULT STDMETHODCALLTYPE _PositionViewWindow(HWND, RECT *);
    virtual HRESULT STDMETHODCALLTYPE IEParseDisplayNameEx(UINT, PCWSTR, DWORD, LPITEMIDLIST *);

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

    // *** IShellBowserService methods ***
    virtual HRESULT STDMETHODCALLTYPE GetPropertyBag(long flags, REFIID riid, void **ppvObject);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindowSite methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkObj, LPRECT prcBorder);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);

    // *** IDockingWindowFrame methods ***
    virtual HRESULT STDMETHODCALLTYPE AddToolbar(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags);
    virtual HRESULT STDMETHODCALLTYPE RemoveToolbar(IUnknown *punkSrc, DWORD dwRemoveFlags);
    virtual HRESULT STDMETHODCALLTYPE FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppv);

    // *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // *** IDropTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    DECLARE_REGISTRY_RESOURCEID(IDR_COMMONBROWSER)
    DECLARE_NOT_AGGREGATABLE(CCommonBrowser)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCommonBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IBrowserService, IBrowserService)
        COM_INTERFACE_ENTRY_IID(IID_IBrowserService2, IBrowserService2)
        COM_INTERFACE_ENTRY_IID(IID_IBrowserService3, IBrowserService3)
        COM_INTERFACE_ENTRY_IID(IID_IShellBrowser, IShellBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IShellBrowserService, IShellBrowserService)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDockingWindowSite)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindowFrame, IDockingWindowFrame)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()
};
