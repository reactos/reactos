/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     NameSpace Control Band
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define NSCBANDCLASSNAME L"ReactOS NameSpace Control Band"

#define WM_USER_SHELLEVENT (WM_USER + 88)

#ifdef __cplusplus
class CNSCBand
    : public CWindowImpl<CNSCBand>
    , public IDeskBand
    , public IObjectWithSite
    , public IInputObject
    , public IPersistStream
    , public IOleCommandTarget
    , public IServiceProvider
    , public IContextMenu
    , public IBandNavigate
    , public IWinEventHandler
    , public INamespaceProxy
    , public IDropTarget
    , public IDropSource
{
public:
    DECLARE_WND_CLASS_EX(NSCBANDCLASSNAME, 0, COLOR_3DFACE)
    static LPCWSTR GetWndClassName() { return NSCBANDCLASSNAME; }

    CNSCBand();
    virtual ~CNSCBand();

    // The node of TreeView
    struct CItemData
    {
        CComHeapPtr<ITEMIDLIST> absolutePidl;
        CComHeapPtr<ITEMIDLIST> relativePidl;
        BOOL expanded = FALSE;
    };
    CItemData* GetItemData(_In_ HTREEITEM hItem);

    // *** IOleWindow methods ***
    STDMETHODIMP GetWindow(HWND *lphwnd) override;
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHODIMP CloseDW(DWORD dwReserved) override;
    STDMETHODIMP ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;
    STDMETHODIMP ShowDW(BOOL fShow) override;

    // *** IDeskBand methods ***
    STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;

    // *** IOleCommandTarget methods ***
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici) override;
    STDMETHODIMP GetCommandString(
        UINT_PTR idCmd,
        UINT uType,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax) override;

    // *** IInputObject methods ***
    STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHODIMP HasFocusIO() override;
    STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg) override;

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHODIMP IsDirty() override;
    STDMETHODIMP Load(IStream *pStm) override;
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) override;
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize) override;

    // *** IWinEventHandler methods ***
    STDMETHODIMP OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHODIMP IsWindowOwner(HWND hWnd) override;

    // *** IBandNavigate methods ***
    STDMETHODIMP Select(LPCITEMIDLIST pidl) override;

    // *** INamespaceProxy methods ***
    STDMETHODIMP GetNavigateTarget(
        _In_ PCIDLIST_ABSOLUTE pidl,
        _Out_ PIDLIST_ABSOLUTE *ppidlTarget,
        _Out_ ULONG *pulAttrib) override;
    STDMETHODIMP Invoke(_In_ PCIDLIST_ABSOLUTE pidl) override;
    STDMETHODIMP OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl) override;
    STDMETHODIMP RefreshFlags(
        _Out_ DWORD *pdwStyle,
        _Out_ DWORD *pdwExStyle,
        _Out_ DWORD *dwEnum) override;
    STDMETHODIMP CacheItem(_In_ PCIDLIST_ABSOLUTE pidl) override;

    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHODIMP DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;

    // *** IDropSource methods ***
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override;
    STDMETHODIMP GiveFeedback(DWORD dwEffect) override;

protected:
    DWORD m_dwTVStyle = 0;
    DWORD m_dwTVExStyle = 0;
    DWORD m_dwEnumFlags = 0;
    BOOL m_fVisible = FALSE;
    BOOL m_bFocused = FALSE;
    DWORD m_dwBandID = 0;
    CComPtr<IUnknown> m_pSite;
    CComPtr<IShellFolder> m_pDesktop;
    CComHeapPtr<ITEMIDLIST> m_pidlRoot;
    HIMAGELIST m_hToolbarImageList = NULL;
    CToolbar<> m_hwndToolbar;
    CTreeView m_hwndTreeView;
    LONG m_mtxBlockNavigate = 0; // A "lock" that prevents internal selection changes to initiate a navigation to the newly selected item.
    BOOL m_isEditing = FALSE;
    HTREEITEM m_hRoot = NULL;
    HTREEITEM m_oldSelected = NULL;
    DWORD m_adviseCookie = 0;
    ULONG m_shellRegID = 0;

    // *** Drop target information ***
    CComPtr<IDropTarget> m_pDropTarget;
    HTREEITEM m_childTargetNode = NULL;
    CComPtr<IDataObject> m_pCurObject;

    VOID OnFinalMessage(HWND) override;

    // *** helper methods ***
    virtual INT _GetRootCsidl() = 0;
    virtual HRESULT _CreateTreeView(HWND hwndParent);
    virtual HRESULT _CreateToolbar(HWND hwndParent) { return S_OK; }
    virtual void _DestroyTreeView();
    virtual void _DestroyToolbar();
    virtual DWORD _GetTVStyle() = 0;
    virtual DWORD _GetTVExStyle() = 0;
    virtual DWORD _GetEnumFlags() = 0;
    virtual BOOL _GetTitle(LPWSTR pszTitle, INT cchTitle) = 0;
    virtual BOOL _WantsRootItem() = 0;
    virtual void _SortItems(HTREEITEM hParent) = 0;
    void _RegisterChangeNotify();
    void _UnregisterChangeNotify();
    BOOL OnTreeItemExpanding(_In_ LPNMTREEVIEW pnmtv);
    BOOL OnTreeItemDeleted(_In_ LPNMTREEVIEW pnmtv);
    void _OnSelectionChanged(_In_ LPNMTREEVIEW pnmtv);
    void OnTreeItemDragging(_In_ LPNMTREEVIEW pnmtv, _In_ BOOL isRightClick);
    LRESULT OnBeginLabelEdit(_In_ LPNMTVDISPINFO dispInfo);
    LRESULT OnEndLabelEdit(_In_ LPNMTVDISPINFO dispInfo);
    void OnChangeNotify(
        _In_opt_ LPCITEMIDLIST pidl0,
        _In_opt_ LPCITEMIDLIST pidl1,
        _In_ LONG lEvent);
    HRESULT _ExecuteCommand(_In_ CComPtr<IContextMenu>& menu, _In_ UINT nCmd);
    HTREEITEM _InsertItem(
        _In_opt_ HTREEITEM hParent,
        _Inout_ IShellFolder *psfParent,
        _In_ LPCITEMIDLIST pElt,
        _In_ LPCITEMIDLIST pEltRelative,
        _In_ BOOL bSort);
    HTREEITEM _InsertItem(
        _In_opt_ HTREEITEM hParent,
        _In_ LPCITEMIDLIST pElt,
        _In_ LPCITEMIDLIST pEltRelative,
        _In_ BOOL bSort);
    BOOL _InsertSubitems(HTREEITEM hItem, LPCITEMIDLIST entry);
    HRESULT _UpdateBrowser(LPCITEMIDLIST pidlGoto);
    HRESULT _GetCurrentLocation(_Out_ PIDLIST_ABSOLUTE *ppidl);
    HRESULT _IsCurrentLocation(_In_ PCIDLIST_ABSOLUTE pidl);
    void _Refresh();
    void _RefreshRecurse(_In_ HTREEITEM hItem);
    BOOL _IsTreeItemInEnum(_In_ HTREEITEM hItem, _In_ IEnumIDList *pEnum);
    BOOL _TreeItemHasThisChild(_In_ HTREEITEM hItem, _In_ PCITEMID_CHILD pidlChild);
    HRESULT _GetItemEnum(
        _Out_ CComPtr<IEnumIDList>& pEnum,
        _In_ HTREEITEM hItem,
        _Out_opt_ IShellFolder **ppFolder = NULL);
    BOOL _ItemHasAnyChild(_In_ HTREEITEM hItem);
    HRESULT _AddFavorite();

    // *** ATL message handlers ***
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnShellEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BEGIN_MSG_MAP(CNSCBand)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_USER_SHELLEVENT, OnShellEvent)
    END_MSG_MAP()
};
#endif // def __cplusplus
