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
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IBrowserService methods ***
    STDMETHOD(GetParentSite)(IOleInPlaceSite **ppipsite) override;
    STDMETHOD(SetTitle)(IShellView *psv, LPCWSTR pszName) override;
    STDMETHOD(GetTitle)(IShellView *psv, LPWSTR pszName, DWORD cchName) override;
    STDMETHOD(GetOleObject)(IOleObject **ppobjv) override;
    STDMETHOD(GetTravelLog)(ITravelLog **pptl) override;
    STDMETHOD(ShowControlWindow)(UINT id, BOOL fShow) override;
    STDMETHOD(IsControlWindowShown)(UINT id, BOOL *pfShown) override;
    STDMETHOD(IEGetDisplayName)(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags) override;
    STDMETHOD(IEParseDisplayName)(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST *ppidlOut) override;
    STDMETHOD(DisplayParseError)(HRESULT hres, LPCWSTR pwszPath) override;
    STDMETHOD(NavigateToPidl)(LPCITEMIDLIST pidl, DWORD grfHLNF) override;
    STDMETHOD(SetNavigateState)(BNSTATE bnstate) override;
    STDMETHOD(GetNavigateState)(BNSTATE *pbnstate) override;
    STDMETHOD(NotifyRedirect)(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse) override;
    STDMETHOD(UpdateWindowList)() override;
    STDMETHOD(UpdateBackForwardState)() override;
    STDMETHOD(SetFlags)(DWORD dwFlags, DWORD dwFlagMask) override;
    STDMETHOD(GetFlags)(DWORD *pdwFlags) override;
    STDMETHOD(CanNavigateNow)() override;
    STDMETHOD(GetPidl)(LPITEMIDLIST *ppidl) override;
    STDMETHOD(SetReferrer)(LPCITEMIDLIST pidl) override;
    STDMETHOD_(DWORD, GetBrowserIndex)() override;
    STDMETHOD(GetBrowserByIndex)(DWORD dwID, IUnknown **ppunk) override;
    STDMETHOD(GetHistoryObject)(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc) override;
    STDMETHOD(SetHistoryObject)(IOleObject *pole, BOOL fIsLocalAnchor) override;
    STDMETHOD(CacheOLEServer)(IOleObject *pole) override;
    STDMETHOD(GetSetCodePage)(VARIANT *pvarIn, VARIANT *pvarOut) override;
    STDMETHOD(OnHttpEquiv)(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut) override;
    STDMETHOD(GetPalette)(HPALETTE *hpal) override;
    STDMETHOD(RegisterWindow)(BOOL fForceRegister, int swc) override;

    // *** IBrowserService2 methods ***
    STDMETHOD_(LRESULT, WndProcBS)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(SetAsDefFolderSettings)() override;
    STDMETHOD(GetViewRect)(RECT *prc) override;
    STDMETHOD(OnSize)(WPARAM wParam) override;
    STDMETHOD(OnCreate)(struct tagCREATESTRUCTW *pcs) override;
    STDMETHOD_(LRESULT, OnCommand)(WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(OnDestroy)() override;
    STDMETHOD_(LRESULT, OnNotify)(struct tagNMHDR *pnm) override;
    STDMETHOD(OnSetFocus)() override;
    STDMETHOD(OnFrameWindowActivateBS)(BOOL fActive) override;
    STDMETHOD(ReleaseShellView)() override;
    STDMETHOD(ActivatePendingView)() override;
    STDMETHOD(CreateViewWindow)(IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd) override;
    STDMETHOD(CreateBrowserPropSheetExt)(REFIID riid, void **ppv) override;
    STDMETHOD(GetViewWindow)(HWND *phwndView) override;
    STDMETHOD(GetBaseBrowserData)(LPCBASEBROWSERDATA *pbbd) override;
    STDMETHOD_(LPBASEBROWSERDATA, PutBaseBrowserData)() override;
    STDMETHOD(InitializeTravelLog)(ITravelLog *ptl, DWORD dw) override;
    STDMETHOD(SetTopBrowser)() override;
    STDMETHOD(Offline)(int iCmd) override;
    STDMETHOD(AllowViewResize)(BOOL f) override;
    STDMETHOD(SetActivateState)(UINT u) override;
    STDMETHOD(UpdateSecureLockIcon)(int eSecureLock) override;
    STDMETHOD(InitializeDownloadManager)() override;
    STDMETHOD(InitializeTransitionSite)() override;
    STDMETHOD(_Initialize)(HWND hwnd, IUnknown *pauto) override;
    STDMETHOD(_CancelPendingNavigationAsync)() override;
    STDMETHOD(_CancelPendingView)() override;
    STDMETHOD(_MaySaveChanges)() override;
    STDMETHOD(_PauseOrResumeView)(BOOL fPaused) override;
    STDMETHOD(_DisableModeless)() override;
    STDMETHOD(_NavigateToPidl)(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags) override;
    STDMETHOD(_TryShell2Rename)(IShellView *psv, LPCITEMIDLIST pidlNew) override;
    STDMETHOD(_SwitchActivationNow)() override;
    STDMETHOD(_ExecChildren)(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut) override;
    STDMETHOD(_SendChildren)(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(GetFolderSetData)(struct tagFolderSetData *pfsd) override;
    STDMETHOD(_OnFocusChange)(UINT itb) override;
    STDMETHOD(v_ShowHideChildWindows)(BOOL fChildOnly) override;
    STDMETHOD_(UINT, _get_itbLastFocus)() override;
    STDMETHOD(_put_itbLastFocus)(UINT itbLastFocus) override;
    STDMETHOD(_UIActivateView)(UINT uState) override;
    STDMETHOD(_GetViewBorderRect)(RECT *prc) override;
    STDMETHOD(_UpdateViewRectSize)() override;
    STDMETHOD(_ResizeNextBorder)(UINT itb) override;
    STDMETHOD(_ResizeView)() override;
    STDMETHOD(_GetEffectiveClientArea)(LPRECT lprectBorder, HMONITOR hmon) override;
    STDMETHOD_(IStream *, v_GetViewStream)(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName) override;
    STDMETHOD_(LRESULT, ForwardViewMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(SetAcceleratorMenu)(HACCEL hacc) override;
    STDMETHOD_(int, _GetToolbarCount)() override;
    STDMETHOD_(LPTOOLBARITEM, _GetToolbarItem)(int itb) override;
    STDMETHOD(_SaveToolbars)(IStream *pstm) override;
    STDMETHOD(_LoadToolbars)(IStream *pstm) override;
    STDMETHOD(_CloseAndReleaseToolbars)(BOOL fClose) override;
    STDMETHOD(v_MayGetNextToolbarFocus)(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM *pptbi, HWND *phwnd) override;
    STDMETHOD(_ResizeNextBorderHelper)(UINT itb, BOOL bUseHmonitor) override;
    STDMETHOD_(UINT, _FindTBar)(IUnknown *punkSrc) override;
    STDMETHOD(_SetFocus)(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) override;
    STDMETHOD(v_MayTranslateAccelerator)(MSG *pmsg) override;
    STDMETHOD(_GetBorderDWHelper)(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) override;
    STDMETHOD(v_CheckZoneCrossing)(LPCITEMIDLIST pidl) override;

    // *** IBrowserService3 methods ***
    STDMETHOD(_PositionViewWindow)(HWND, RECT *) override;
    STDMETHOD(IEParseDisplayNameEx)(UINT, PCWSTR, DWORD, LPITEMIDLIST *) override;

    // *** IShellBrowser methods ***
    STDMETHOD(InsertMenusSB)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) override;
    STDMETHOD(SetMenuSB)(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject) override;
    STDMETHOD(RemoveMenusSB)(HMENU hmenuShared) override;
    STDMETHOD(SetStatusTextSB)(LPCOLESTR pszStatusText) override;
    STDMETHOD(EnableModelessSB)(BOOL fEnable) override;
    STDMETHOD(TranslateAcceleratorSB)(MSG *pmsg, WORD wID) override;
    STDMETHOD(BrowseObject)(LPCITEMIDLIST pidl, UINT wFlags) override;
    STDMETHOD(GetViewStateStream)(DWORD grfMode, IStream **ppStrm) override;
    STDMETHOD(GetControlWindow)(UINT id, HWND *lphwnd) override;
    STDMETHOD(SendControlMsg)(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) override;
    STDMETHOD(QueryActiveShellView)(struct IShellView **ppshv) override;
    STDMETHOD(OnViewWindowActive)(struct IShellView *ppshv) override;
    STDMETHOD(SetToolbarItems)(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) override;

    // *** IShellBowserService methods ***
    STDMETHOD(GetPropertyBag)(long flags, REFIID riid, void **ppvObject) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindowSite methods ***
    STDMETHOD(GetBorderDW)(IUnknown* punkObj, LPRECT prcBorder) override;
    STDMETHOD(RequestBorderSpaceDW)(IUnknown* punkObj, LPCBORDERWIDTHS pbw) override;
    STDMETHOD(SetBorderSpaceDW)(IUnknown* punkObj, LPCBORDERWIDTHS pbw) override;

    // *** IDockingWindowFrame methods ***
    STDMETHOD(AddToolbar)(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags) override;
    STDMETHOD(RemoveToolbar)(IUnknown *punkSrc, DWORD dwRemoveFlags) override;
    STDMETHOD(FindToolbar)(LPCWSTR pwszItem, REFIID riid, void **ppv) override;

    // *** IInputObjectSite specific methods ***
    STDMETHOD(OnFocusChangeIS)(IUnknown *punkObj, BOOL fSetFocus) override;

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_COMMONBROWSER)
    DECLARE_AGGREGATABLE(CCommonBrowser)

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
