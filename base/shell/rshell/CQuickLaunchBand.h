/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * COPYRIGHT:   Copyright Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */
#pragma once

extern const GUID CLSID_QuickLaunchBand;

// Component category registration
HRESULT RegisterComCat();
HRESULT UnregisterComCat();

// COM class for quick launch
class CQuickLaunchBand :
    public CComCoClass<CQuickLaunchBand, &CLSID_QuickLaunchBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectWithSite,
    public IDeskBand,
    public IPersistStream,
    public IWinEventHandler,
    public IOleCommandTarget,
    public IContextMenu
{
    HWND m_hWndBro;
    CComPtr<IUnknown> m_punkISFB;

public:
    CQuickLaunchBand();
    virtual ~CQuickLaunchBand();

    STDMETHOD(ContainsWindow)(IN HWND hWnd);

// ATL construct

    HRESULT FinalConstruct();

// IObjectWithSite

    STDMETHOD(GetSite)(
        IN  REFIID riid,
        OUT void   **ppvSite) override;

    STDMETHOD(SetSite)(IN IUnknown *pUnkSite) override;

// IDeskBand

    STDMETHOD(GetWindow)(OUT HWND *phwnd) override;

    STDMETHOD(ContextSensitiveHelp)(IN BOOL fEnterMode) override;

    STDMETHOD(ShowDW)(IN BOOL bShow) override;

    STDMETHOD(CloseDW)(IN DWORD dwReserved) override;

    STDMETHOD(ResizeBorderDW)(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) override;

    STDMETHOD(GetBandInfo)(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi) override;

// IPersistStream

    STDMETHOD(GetClassID)(OUT CLSID *pClassID) override;

    STDMETHOD(GetSizeMax)(OUT ULARGE_INTEGER *pcbSize) override;

    STDMETHOD(IsDirty)() override;

    STDMETHOD(Load)(IN IStream *pStm) override;

    STDMETHOD(Save)(
        IN IStream *pStm,
        IN BOOL    fClearDirty) override;

// IWinEventHandler

    STDMETHOD(OnWinEvent)(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *theResult) override;

    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

// IOleCommandTarget

    STDMETHOD(Exec)(
        IN const GUID *pguidCmdGroup,
        IN DWORD nCmdID,
        IN DWORD nCmdexecopt,
        IN VARIANT *pvaIn,
        IN OUT VARIANT *pvaOut) override;

    STDMETHOD(QueryStatus)(
        IN const GUID *pguidCmdGroup,
        IN ULONG cCmds,
        IN OUT OLECMD prgCmds[],
        IN OUT OLECMDTEXT *pCmdText) override;

// IContextMenu
    STDMETHOD(GetCommandString)(
        UINT_PTR idCmd,
        UINT uFlags,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax) override;

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO pici) override;

    STDMETHOD(QueryContextMenu)(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags) override;

//*****************************************************************************************************

    DECLARE_NOT_AGGREGATABLE(CQuickLaunchBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CQuickLaunchBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};
