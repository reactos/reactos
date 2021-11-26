/*
 * ReactOS Explorer
 *
 * Copyright 2016 Sylvain Deverre <deverre dot sylv at gmail dot com>
 * Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#define WM_USER_SHELLEVENT WM_USER+88
#define WM_USER_FOLDEREVENT WM_USER+88

class CExplorerBand :
    public CComCoClass<CExplorerBand, &CLSID_ExplorerBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IBandNavigate,
    public IWinEventHandler,
    public INamespaceProxy,
    public IDispatch,
    public IDropSource,
    public IDropTarget,
    public CWindowImpl<CExplorerBand, CWindow, CControlWinTraits>
{

private:
    class NodeInfo
    {
    public:
        LPITEMIDLIST absolutePidl;
        LPITEMIDLIST relativePidl;
        BOOL expanded;
    };

    // *** BaseBarSite information ***
    CComPtr<IUnknown> m_pSite;
    CComPtr<IShellFolder> m_pDesktop;

    // *** tree explorer band stuff ***
    BOOL m_fVisible;
    BOOL m_bNavigating;
    BOOL m_bFocused;
    DWORD m_dwBandID;
    HIMAGELIST m_hImageList;
    HTREEITEM  m_hRoot;
    HTREEITEM  m_oldSelected;
    LPITEMIDLIST m_pidlCurrent;

    // *** notification cookies ***
    DWORD m_adviseCookie;
    ULONG m_shellRegID;

    // *** Drop target information ***
    CComPtr<IDropTarget> m_pDropTarget;
    HTREEITEM m_childTargetNode;
    CComPtr<IDataObject> m_pCurObject;

    void InitializeExplorerBand();
    void DestroyExplorerBand();
    HRESULT ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd);

    // *** notifications handling ***
    BOOL OnTreeItemExpanding(LPNMTREEVIEW pnmtv);
    void OnSelectionChanged(LPNMTREEVIEW pnmtv);
    BOOL OnTreeItemDeleted(LPNMTREEVIEW pnmtv);
    void OnTreeItemDragging(LPNMTREEVIEW pnmtv, BOOL isRightClick);

    // *** ATL event handlers ***
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT ContextMenuHack(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnShellEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    // *** Helper functions ***
    NodeInfo* GetNodeInfo(HTREEITEM hItem);
    HRESULT UpdateBrowser(LPITEMIDLIST pidlGoto);
    HTREEITEM InsertItem(HTREEITEM hParent, IShellFolder *psfParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort);
    HTREEITEM InsertItem(HTREEITEM hParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort);
    BOOL InsertSubitems(HTREEITEM hItem, NodeInfo *pNodeInfo);
    BOOL NavigateToPIDL(LPITEMIDLIST dest, HTREEITEM *item, BOOL bExpand, BOOL bInsert, BOOL bSelect);
    BOOL DeleteItem(LPITEMIDLIST toDelete);
    BOOL RenameItem(HTREEITEM toRename, LPITEMIDLIST newPidl);
    BOOL RefreshTreePidl(HTREEITEM tree, LPITEMIDLIST pidlParent);
    BOOL NavigateToCurrentFolder();

    // *** Tree item sorting callback ***
    static int CALLBACK CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3);

public:
    CExplorerBand();
    virtual ~CExplorerBand();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IBandNavigate methods ***
    virtual HRESULT STDMETHODCALLTYPE Select(long paramC);

    // *** INamespaceProxy ***
    virtual HRESULT STDMETHODCALLTYPE GetNavigateTarget(long paramC, long param10, long param14);
    virtual HRESULT STDMETHODCALLTYPE Invoke(long paramC);
    virtual HRESULT STDMETHODCALLTYPE OnSelectionChanged(long paramC);
    virtual HRESULT STDMETHODCALLTYPE RefreshFlags(long paramC, long param10, long param14);
    virtual HRESULT STDMETHODCALLTYPE CacheItem(long paramC);

    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

    // *** IDropTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IDropSource methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    virtual HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

    DECLARE_REGISTRY_RESOURCEID(IDR_EXPLORERBAND)
    DECLARE_NOT_AGGREGATABLE(CExplorerBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerBand)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    END_COM_MAP()

    BEGIN_MSG_MAP(CExplorerBand)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_USER_SHELLEVENT, OnShellEvent)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, ContextMenuHack)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        // MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    END_MSG_MAP()
};
