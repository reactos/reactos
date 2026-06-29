/*
 * Shell Menu Site
 *
 * Copyright 2014 David Quintana
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

class CMenuSite :
    public CComCoClass<CMenuSite, &CLSID_MenuBandSite>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl<CMenuSite, CWindow, CControlWinTraits>,
    public IBandSite,
    public IDeskBarClient,
    public IOleCommandTarget,
    public IInputObject,
    public IInputObjectSite,
    public IWinEventHandler,
    public IServiceProvider
{
private:
    CComPtr<IUnknown>         m_DeskBarSite;
    CComPtr<IUnknown>         m_BandObject;
    CComPtr<IDeskBand>        m_DeskBand;
    CComPtr<IWinEventHandler> m_WinEventHandler;
    HWND                      m_hWndBand;

public:
    CMenuSite();
    virtual ~CMenuSite() {}

    DECLARE_WND_CLASS_EX(_T("MenuSite"), 0, COLOR_MENU)

    DECLARE_REGISTRY_RESOURCEID(IDR_MENUBANDSITE)
    DECLARE_NOT_AGGREGATABLE(CMenuSite)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CMenuSite)
        COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    END_COM_MAP()

    // IBandSite
    STDMETHOD(AddBand)(IUnknown * punk) override;
    STDMETHOD(EnumBands)(UINT uBand, DWORD* pdwBandID) override;
    STDMETHOD(QueryBand)(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName) override;
    STDMETHOD(GetBandObject)(DWORD dwBandID, REFIID riid, VOID **ppv) override;

    // IDeskBarClient
    STDMETHOD(SetDeskBarSite)(IUnknown *punkSite) override;
    STDMETHOD(GetSize)(DWORD dwWhich, LPRECT prc) override;
    STDMETHOD(UIActivateDBC)(DWORD dwState) override;

    // IOleWindow
    STDMETHOD(GetWindow)(HWND *phwnd) override;

    // IOleCommandTarget
    STDMETHOD(QueryStatus)(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // IInputObject
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // IInputObjectSite
    STDMETHOD(OnFocusChangeIS)(IUnknown *punkObj, BOOL fSetFocus) override;

    // IWinEventHandler
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // Using custom message map instead
    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD mapId = 0);

    // IDeskBarClient
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;
    STDMETHOD(GetBandSiteInfo)(BANDSITEINFO *pbsinfo) override;
    STDMETHOD(RemoveBand)(DWORD dwBandID) override;
    STDMETHOD(SetBandSiteInfo)(const BANDSITEINFO *pbsinfo) override;
    STDMETHOD(SetBandState)(DWORD dwBandID, DWORD dwMask, DWORD dwState) override;
    STDMETHOD(SetModeDBC)(DWORD dwMode) override;

private:
    IUnknown * ToIUnknown() { return static_cast<IDeskBarClient*>(this); }
    UINT GetBandCount() { return m_BandObject ? 1 : 0; }
};
