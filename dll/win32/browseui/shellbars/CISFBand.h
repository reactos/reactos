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

// IShellFolderBand
    virtual STDMETHODIMP GetBandInfoSFB(
        PBANDINFOSFB pbi
    );

    virtual STDMETHODIMP InitializeSFB(
        IShellFolder      *psf,
        PCIDLIST_ABSOLUTE pidl
    );

    virtual STDMETHODIMP SetBandInfoSFB(
        PBANDINFOSFB pbi
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
