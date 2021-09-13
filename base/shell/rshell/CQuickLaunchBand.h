/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CQuickLaunchBand.h
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
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

// ATL construct

    HRESULT FinalConstruct();

// IObjectWithSite

    virtual STDMETHODIMP GetSite(
        IN  REFIID riid,
        OUT void   **ppvSite
    );

    virtual STDMETHODIMP SetSite(
        IN IUnknown *pUnkSite
    );

// IDeskBand

    virtual STDMETHODIMP GetWindow(
        OUT HWND *phwnd
    );

    virtual STDMETHODIMP ContextSensitiveHelp(
        IN BOOL fEnterMode
    );

    virtual STDMETHODIMP ShowDW(
        IN BOOL bShow
    );

    virtual STDMETHODIMP CloseDW(
        IN DWORD dwReserved
    );

    virtual STDMETHODIMP ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved
    );

    virtual STDMETHODIMP GetBandInfo(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi
    );

// IPersistStream

    virtual STDMETHODIMP GetClassID(
        OUT CLSID *pClassID
    );

    virtual STDMETHODIMP GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize
    );

    virtual STDMETHODIMP IsDirty();

    virtual STDMETHODIMP Load(
        IN IStream *pStm
    );

    virtual STDMETHODIMP Save(
        IN IStream *pStm,
        IN BOOL    fClearDirty
    );

// IWinEventHandler

    virtual STDMETHODIMP ContainsWindow(
        IN HWND hWnd
    );

    virtual STDMETHODIMP OnWinEvent(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *theResult
    );

    virtual STDMETHODIMP IsWindowOwner(
        HWND hWnd
    );

// IOleCommandTarget

    virtual STDMETHODIMP Exec(
        IN const GUID *pguidCmdGroup,
        IN DWORD nCmdID,
        IN DWORD nCmdexecopt,
        IN VARIANT *pvaIn,
        IN OUT VARIANT *pvaOut
    );

    virtual STDMETHODIMP QueryStatus(
        IN const GUID *pguidCmdGroup,
        IN ULONG cCmds,
        IN OUT OLECMD prgCmds[],
        IN OUT OLECMDTEXT *pCmdText
    );

// IContextMenu
    virtual STDMETHODIMP GetCommandString(
        UINT_PTR idCmd,
        UINT uFlags,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax
    );

    virtual STDMETHODIMP InvokeCommand(
        LPCMINVOKECOMMANDINFO pici
    );

    virtual STDMETHODIMP QueryContextMenu(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags
    );

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