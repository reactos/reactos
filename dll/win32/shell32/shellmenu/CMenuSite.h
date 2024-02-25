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
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown * punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD* pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv);

    // IDeskBarClient
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);

    // IOleWindow
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // IInputObject
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // IInputObjectSite
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // IWinEventHandler
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);


    // Using custom message map instead
    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD mapId = 0);

    // UNIMPLEMENTED
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);

private:
    IUnknown * ToIUnknown() { return static_cast<IDeskBarClient*>(this); }
};
