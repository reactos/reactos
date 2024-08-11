/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * COPYRIGHT:   Copyright Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#pragma once

EXTERN_C const GUID CLSID_QuickLaunchBand;

// Component category registration
EXTERN_C HRESULT RegisterComCat(VOID);
EXTERN_C HRESULT UnregisterComCat(VOID);

// COM class for quick launch
class CQuickLaunchBand
    : public CComCoClass<CQuickLaunchBand, &CLSID_QuickLaunchBand>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IObjectWithSite
    , public IDeskBand
    , public IPersistStream
    , public IWinEventHandler
    , public IOleCommandTarget
    , public IContextMenu
{
protected:
    HWND m_hWndBro;
    CComPtr<IUnknown> m_punkISFB;

public:
    CQuickLaunchBand();
    virtual ~CQuickLaunchBand();

    STDMETHOD(ContainsWindow)(_In_ HWND hWnd);

    // ATL construct
    HRESULT FinalConstruct();

    // IObjectWithSite methods
    STDMETHODIMP GetSite(_In_ REFIID riid, _Out_ void **ppvSite) override;
    STDMETHODIMP SetSite(_In_ IUnknown *pUnkSite) override;

    // IDeskBand methods
    STDMETHODIMP GetWindow(_Out_ HWND *phwnd) override;
    STDMETHODIMP ContextSensitiveHelp(_In_ BOOL fEnterMode) override;
    STDMETHODIMP ShowDW(_In_ BOOL bShow) override;
    STDMETHODIMP CloseDW(_In_ DWORD dwReserved) override;
    STDMETHODIMP ResizeBorderDW(
        _In_ LPCRECT prcBorder,
        _In_ IUnknown *punkToolbarSite,
        _In_ BOOL fReserved) override;
    STDMETHODIMP GetBandInfo(
        _In_ DWORD dwBandID,
        _In_ DWORD dwViewMode,
        _Inout_ DESKBANDINFO *pdbi) override;

    // IPersistStream methods
    STDMETHODIMP GetClassID(_Out_ CLSID *pClassID) override;
    STDMETHODIMP GetSizeMax(_Out_ ULARGE_INTEGER *pcbSize) override;
    STDMETHODIMP IsDirty() override;
    STDMETHODIMP Load(_In_ IStream *pStm) override;
    STDMETHODIMP Save(_In_ IStream *pStm, _In_ BOOL fClearDirty) override;

    // IWinEventHandler methods
    STDMETHODIMP OnWinEvent(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *theResult) override;
    STDMETHODIMP IsWindowOwner(HWND hWnd) override;

    // IOleCommandTarget methods
    STDMETHODIMP Exec(
        _In_ const GUID *pguidCmdGroup,
        _In_ DWORD nCmdID,
        _In_ DWORD nCmdexecopt,
        _In_ VARIANT *pvaIn,
        _Inout_ VARIANT *pvaOut) override;
    STDMETHODIMP QueryStatus(
        _In_ const GUID *pguidCmdGroup,
        _In_ ULONG cCmds,
        _Inout_ OLECMD prgCmds[],
        _Inout_ OLECMDTEXT *pCmdText) override;

    // IContextMenu methods
    STDMETHODIMP GetCommandString(
        _In_ UINT_PTR idCmd,
        _In_ UINT uFlags,
        _In_ UINT *pwReserved,
        _Out_ LPSTR pszName,
        _In_ UINT cchMax) override;
    STDMETHODIMP InvokeCommand(_In_ LPCMINVOKECOMMANDINFO pici) override;
    STDMETHODIMP QueryContextMenu(
        _Out_ HMENU hmenu,
        _In_ UINT indexMenu,
        _In_ UINT idCmdFirst,
        _In_ UINT idCmdLast,
        _In_ UINT uFlags) override;

    /*****************************************************************************/

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
