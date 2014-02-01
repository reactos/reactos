/*
 * Start menu object
 *
 * Copyright 2009 Andrew Hill
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

#ifndef _STARTMENU_H_
#define _STARTMENU_H_

class CStartMenu :
    public CComCoClass<CStartMenu, &CLSID_StartMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IMenuPopup,
    public IObjectWithSite,
    public IInitializeObject,
    public IMenuBand // FIXME
{
private:
    IBandSite *m_pBandSite;
    IUnknown *m_pUnkSite;

public:
    CStartMenu();
    ~CStartMenu();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // *** IInitializeObject methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize();

    // *** IMenuBand methods *** FIXME
    virtual HRESULT STDMETHODCALLTYPE IsMenuMessage(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE TranslateMenuMessage(MSG *pmsg, LRESULT *plRet);

DECLARE_REGISTRY_RESOURCEID(IDR_STARTMENU)
DECLARE_NOT_AGGREGATABLE(CStartMenu)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStartMenu)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IDeskBar)
    COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    COM_INTERFACE_ENTRY_IID(IID_IInitializeObject, IInitializeObject)
    COM_INTERFACE_ENTRY_IID(IID_IMenuBand, IMenuBand) // FIXME: Win does not export it
END_COM_MAP()
};

class CMenuBandSite :
    public CComCoClass<CMenuBandSite, &CLSID_MenuBandSite>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBandSite,
    public IDeskBarClient,
    public IOleCommandTarget,
    public IInputObject,
    public IInputObjectSite,
    public IWinEventHandler,
    public IServiceProvider
{
private:
    IUnknown **m_pObjects;
    ULONG m_cObjects;

public:
    CMenuBandSite();
    ~CMenuBandSite();

    // *** IBandSite methods ***
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD *pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDeskBarClient methods ***
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND paramC, UINT param10, WPARAM param14, LPARAM param18, LRESULT *param1C);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND paramC);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

DECLARE_REGISTRY_RESOURCEID(IDR_MENUBANDSITE)
DECLARE_NOT_AGGREGATABLE(CMenuBandSite)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMenuBandSite)
    COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
    COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
    COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
    COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
END_COM_MAP()
};

#endif /* _STARTMENU_H_ */
