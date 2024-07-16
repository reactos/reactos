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

#ifdef __cplusplus
class CFavBand
    : public CComCoClass<CFavBand, &CLSID_SH_FavBand>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
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
    , public CWindowImpl<CFavBand, CWindow, CControlWinTraits>
{
public:
    DECLARE_WND_CLASS_EX(L"ReactOS CFavBand", CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, COLOR_3DFACE)

    CFavBand();
    virtual ~CFavBand();

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;
    STDMETHOD(ShowDW)(BOOL fShow) override;

    // *** IDeskBand methods ***
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags) override;
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici) override;
    STDMETHOD(GetCommandString)(
        UINT_PTR idCmd,
        UINT uType,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IBandNavigate methods ***
    STDMETHOD(Select)(long paramC) override;

    // *** INamespaceProxy methods ***
    STDMETHOD(GetNavigateTarget)(long paramC, long param10, long param14) override;
    STDMETHOD(Invoke)(long paramC) override;
    STDMETHOD(OnSelectionChanged)(long paramC) override;
    STDMETHOD(RefreshFlags)(long paramC, long param10, long param14) override;
    STDMETHOD(CacheItem)(long paramC) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId) override;
    STDMETHOD(Invoke)(
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
    END_MSG_MAP()

protected:
    BOOL m_fVisible;
    BOOL m_bFocused;
    DWORD m_dwBandID;
    CComPtr<IUnknown> m_pSite;
    CComHeapPtr<ITEMIDLIST> m_pidlCurrent;

    // *** helper methods ***
    HRESULT UpdateLocation();

    // *** message handlers ***
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};
#endif // def __cplusplus
