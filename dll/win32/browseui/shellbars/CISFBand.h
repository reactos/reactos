/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CISFBand.h
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#pragma once

class CISFBand :
    public CWindow,
    public CComCoClass<CBandSiteMenu, &CLSID_ISFBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectWithSite,
    public IDeskBand,
    public IPersistStream,
    public IWinEventHandler,
    public IOleCommandTarget,
    public IShellFolderBand,
    public IContextMenu
{
    // Band
    DWORD m_BandID;
    CComPtr<IUnknown> m_Site;

    // Toolbar
    CComPtr<IShellFolder> m_pISF;
    PIDLIST_ABSOLUTE m_pidl;

    // Menu
    BOOL m_textFlag;
    BOOL m_iconFlag;
    BOOL m_QLaunch;

public:

    CISFBand();
    virtual ~CISFBand();

// Personal Methods
    HRESULT CreateSimpleToolbar(HWND hWndParent);

    STDMETHOD(ContainsWindow)(
        IN HWND hWnd
    );

// IObjectWithSite

    STDMETHOD(GetSite)(
        IN  REFIID riid,
        OUT void   **ppvSite
    ) override;

    STDMETHOD(SetSite)(
        IN IUnknown *pUnkSite
    ) override;

// IDeskBand

    STDMETHOD(GetWindow)(
        OUT HWND *phwnd
    ) override;

    STDMETHOD(ContextSensitiveHelp)(
        IN BOOL fEnterMode
    ) override;

    STDMETHOD(ShowDW)(
        IN BOOL bShow
    ) override;

    STDMETHOD(CloseDW)(
        IN DWORD dwReserved
    ) override;

    STDMETHOD(ResizeBorderDW)(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved
    ) override;

    STDMETHOD(GetBandInfo)(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi
    ) override;

// IPersistStream

    STDMETHOD(GetClassID)(
        OUT CLSID *pClassID
    ) override;

    STDMETHOD(GetSizeMax)(
        OUT ULARGE_INTEGER *pcbSize
    ) override;

    STDMETHOD(IsDirty)() override;

    STDMETHOD(Load)(
        IN IStream *pStm
    ) override;

    STDMETHOD(Save)(
        IN IStream *pStm,
        IN BOOL    fClearDirty
    ) override;

// IWinEventHandler

    STDMETHOD(OnWinEvent)(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *theResult
    ) override;

    STDMETHOD(IsWindowOwner)(
        HWND hWnd
    ) override;

// IOleCommandTarget

    STDMETHOD(Exec)(
        IN const GUID *pguidCmdGroup,
        IN DWORD nCmdID,
        IN DWORD nCmdexecopt,
        IN VARIANT *pvaIn,
        IN OUT VARIANT *pvaOut
    ) override;

    STDMETHOD(QueryStatus)(
        IN const GUID *pguidCmdGroup,
        IN ULONG cCmds,
        IN OUT OLECMD prgCmds[],
        IN OUT OLECMDTEXT *pCmdText
    ) override;

// IShellFolderBand
    STDMETHOD(GetBandInfoSFB)(
        PBANDINFOSFB pbi
    ) override;

    STDMETHOD(InitializeSFB)(
        IShellFolder      *psf,
        PCIDLIST_ABSOLUTE pidl
    ) override;

    STDMETHOD(SetBandInfoSFB)(
        PBANDINFOSFB pbi
    ) override;

// IContextMenu
    STDMETHOD(GetCommandString)(
        UINT_PTR idCmd,
        UINT uFlags,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax
    ) override;

    STDMETHOD(InvokeCommand)(
        LPCMINVOKECOMMANDINFO pici
    ) override;

    STDMETHOD(QueryContextMenu)(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags
    ) override;

//*****************************************************************************************************

    DECLARE_REGISTRY_RESOURCEID(IDR_ISFBAND)
    DECLARE_NOT_AGGREGATABLE(CISFBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CISFBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderBand, IShellFolderBand)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

extern "C" HRESULT WINAPI RSHELL_CISFBand_CreateInstance(REFIID riid, void** ppv);
