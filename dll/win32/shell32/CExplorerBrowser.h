/*
 * IExplorerBrowser implementation
 *
 * Copyright 2010 David Hedberg (Wine)
 * Copyright 2024 ReactOS Contributors
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

class CExplorerBrowser :
    public CComCoClass<CExplorerBrowser, &CLSID_ExplorerBrowser>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExplorerBrowser,
    public IShellBrowser,
    public IObjectWithSite,
    public IServiceProvider,
    public ICommDlgBrowser3
{
public:
    CExplorerBrowser();
    ~CExplorerBrowser();

    HRESULT Navigate(PCIDLIST_ABSOLUTE pidl);

    // IExplorerBrowser
    STDMETHODIMP Initialize(HWND hwndParent, const RECT *prc, const FOLDERSETTINGS *pfs) override;
    STDMETHODIMP Destroy() override;
    STDMETHODIMP SetRect(HDWP *phdwp, RECT rcBrowser) override;
    STDMETHODIMP SetPropertyBag(LPCWSTR pszPropertyBag) override;
    STDMETHODIMP SetEmptyText(LPCWSTR pszEmptyText) override;
    STDMETHODIMP SetFolderSettings(const FOLDERSETTINGS *pfs) override;
    STDMETHODIMP Advise(IExplorerBrowserEvents *psbe, DWORD *pdwCookie) override;
    STDMETHODIMP Unadvise(DWORD dwCookie) override;
    STDMETHODIMP SetOptions(EXPLORER_BROWSER_OPTIONS dwFlag) override;
    STDMETHODIMP GetOptions(EXPLORER_BROWSER_OPTIONS *pdwFlag) override;
    STDMETHODIMP BrowseToIDList(PCUIDLIST_RELATIVE pidl, UINT uFlags) override;
    STDMETHODIMP BrowseToObject(IUnknown *punk, UINT uFlags) override;
    STDMETHODIMP FillFromObject(IUnknown *punk, EXPLORER_BROWSER_FILL_FLAGS dwFlags) override;
    STDMETHODIMP RemoveAll() override;
    STDMETHODIMP GetCurrentView(REFIID riid, void **ppv) override;

    // IShellBrowser (IOleWindow base)
    STDMETHODIMP GetWindow(HWND *phwnd) override;
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;
    STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) override;
    STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject) override;
    STDMETHODIMP RemoveMenusSB(HMENU hmenuShared) override;
    STDMETHODIMP SetStatusTextSB(LPCWSTR pszStatusText) override;
    STDMETHODIMP EnableModelessSB(BOOL fEnable) override;
    STDMETHODIMP TranslateAcceleratorSB(MSG *pmsg, WORD wID) override;
    STDMETHODIMP BrowseObject(PCUIDLIST_RELATIVE pidl, UINT wFlags) override;
    STDMETHODIMP GetViewStateStream(DWORD grfMode, IStream **ppStrm) override;
    STDMETHODIMP GetControlWindow(UINT id, HWND *lphwnd) override;
    STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) override;
    STDMETHODIMP QueryActiveShellView(IShellView **ppshv) override;
    STDMETHODIMP OnViewWindowActive(IShellView *ppshv) override;
    STDMETHODIMP SetToolbarItems(LPTBBUTTONSB lpButtons, UINT nButtons, UINT uFlags) override;

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv) override;

    // ICommDlgBrowser (forwarded to site's ICommDlgBrowser3)
    STDMETHODIMP OnDefaultCommand(IShellView *ppshv) override;
    STDMETHODIMP OnStateChange(IShellView *ppshv, ULONG uChange) override;
    STDMETHODIMP IncludeObject(IShellView *ppshv, PCUITEMID_CHILD pidl) override;

    // ICommDlgBrowser2
    STDMETHODIMP Notify(IShellView *ppshv, DWORD dwNotifyType) override;
    STDMETHODIMP GetDefaultMenuText(IShellView *ppshv, WCHAR *pszText, INT cchMax) override;
    STDMETHODIMP GetViewFlags(DWORD *pdwFlags) override;

    // ICommDlgBrowser3
    STDMETHODIMP OnColumnClicked(IShellView *ppshv, int iColumn) override;
    STDMETHODIMP GetCurrentFilter(LPWSTR pszFileSpec, int cchFileSpec) override;
    STDMETHODIMP OnPreviewCreated(IShellView *ppshv) override;

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CExplorerBrowser)

    BEGIN_COM_MAP(CExplorerBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IExplorerBrowser,  IExplorerBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IShellBrowser,     IShellBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow,        IShellBrowser)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite,   IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider,  IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_ICommDlgBrowser3,  ICommDlgBrowser3)
        COM_INTERFACE_ENTRY_IID(IID_ICommDlgBrowser2,  ICommDlgBrowser3)
        COM_INTERFACE_ENTRY_IID(IID_ICommDlgBrowser,   ICommDlgBrowser3)
    END_COM_MAP()

private:
    HWND                        m_hwnd;        /* container window (parent of view) */
    HWND                        m_hwndSV;      /* current shell view window */
    CComPtr<IShellView>         m_psv;         /* current shell view */
    CComPtr<IUnknown>           m_pSite;       /* site object */
    LPITEMIDLIST                m_pidl;        /* current location PIDL */
    FOLDERSETTINGS              m_fs;          /* folder view settings */
    EXPLORER_BROWSER_OPTIONS    m_dwOptions;   /* EBO_* options */
    CComPtr<IExplorerBrowserEvents> m_pEvents; /* event sink */
    DWORD                       m_dwEventsCookie;
};
