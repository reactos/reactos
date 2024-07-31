/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Favorites bar
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

EXTERN_C VOID CFavBand_Init(HINSTANCE hInstance);
EXTERN_C HRESULT CFavBand_DllCanUnloadNow(VOID);
EXTERN_C HRESULT CFavBand_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
EXTERN_C HRESULT CFavBand_DllRegisterServer(VOID);
EXTERN_C HRESULT CFavBand_DllUnregisterServer(VOID);

#define FAVBANDCLASSNAME L"ReactOS Favorites Band"

#ifdef __cplusplus
class CFavBand
    : public CComCoClass<CFavBand, &CLSID_SH_FavBand>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public CWindowImpl<CFavBand>
    , public IDispatch
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
{
public:
    DECLARE_WND_CLASS_EX(FAVBANDCLASSNAME, 0, COLOR_3DFACE)
    static LPCWSTR GetWndClassName() { return FAVBANDCLASSNAME; }

    CFavBand();
    virtual ~CFavBand();

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
    STDMETHODIMP Select(long paramC) override;

    // *** INamespaceProxy methods ***
    STDMETHODIMP GetNavigateTarget(
        _In_ PCIDLIST_ABSOLUTE pidl,
        _Out_ PIDLIST_ABSOLUTE ppidlTarget,
        _Out_ ULONG *pulAttrib) override;
    STDMETHODIMP Invoke(_In_ PCIDLIST_ABSOLUTE pidl) override;
    STDMETHODIMP OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl) override;
    STDMETHODIMP RefreshFlags(
        _Out_ DWORD *pdwStyle,
        _Out_ DWORD *pdwExStyle,
        _Out_ DWORD *dwEnum) override;
    STDMETHODIMP CacheItem(_In_ PCIDLIST_ABSOLUTE pidl) override;

    // *** IDispatch methods ***
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) override;
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHODIMP GetIDsOfNames(
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId) override;
    STDMETHODIMP Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_FAVBAND)
    DECLARE_NOT_AGGREGATABLE(CFavBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFavBand)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY2_IID(IID_IUnknown, IUnknown, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
    END_COM_MAP()

    BEGIN_MSG_MAP(CFavBand)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    END_MSG_MAP()

protected:
    BOOL m_fVisible;
    BOOL m_bFocused;
    DWORD m_dwBandID;
    CComPtr<IUnknown> m_pSite;
    CComHeapPtr<ITEMIDLIST> m_pidlFav;
    HIMAGELIST m_hToolbarImageList;
    HIMAGELIST m_hTreeViewImageList;
    CToolbar<> m_hwndToolbar;
    CTreeView m_hwndTreeView;

    VOID OnFinalMessage(HWND) override;

    // *** helper methods ***
    BOOL CreateToolbar();
    BOOL CreateTreeView();

    // *** message handlers ***
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};
#endif // def __cplusplus
