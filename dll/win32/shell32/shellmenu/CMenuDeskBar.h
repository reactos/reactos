/*
* Shell Menu Desk Bar
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

typedef CWinTraits<
    WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_PALETTEWINDOW
> CMenuWinTraits;

class CMenuDeskBar :
    public CComCoClass<CMenuDeskBar, &CLSID_MenuDeskBar>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl<CMenuDeskBar, CWindow, CMenuWinTraits>,
    public IOleCommandTarget,
    public IServiceProvider,
    public IInputObjectSite,
    public IInputObject,
    public IMenuPopup,
    public IObjectWithSite,
    public IBanneredBar,
    public IInitializeObject
{
private:
    CComPtr<IUnknown>   m_Site;
    CComPtr<IUnknown>   m_Client;
    CComPtr<IMenuPopup> m_SubMenuParent;
    CComPtr<IMenuPopup> m_SubMenuChild;

    HWND m_ClientWindow;

    DWORD m_IconSize;
    HBITMAP m_Banner;

    BOOL  m_Shown;
    DWORD m_ShowFlags;

    BOOL m_didAddRef;

    virtual void OnFinalMessage(HWND hWnd);
public:
    CMenuDeskBar();
    virtual ~CMenuDeskBar();

    DECLARE_REGISTRY_RESOURCEID(IDR_MENUDESKBAR)
    DECLARE_NOT_AGGREGATABLE(CMenuDeskBar)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_WND_CLASS_EX(_T("BaseBar"), CS_SAVEBITS | CS_DROPSHADOW, COLOR_3DFACE)

    BEGIN_MSG_MAP(CMenuDeskBar)
        MESSAGE_HANDLER(WM_CREATE, _OnCreate)
        MESSAGE_HANDLER(WM_SIZE, _OnSize)
        MESSAGE_HANDLER(WM_NOTIFY, _OnNotify)
        MESSAGE_HANDLER(WM_PAINT, _OnPaint)
        MESSAGE_HANDLER(WM_ACTIVATE, _OnActivate)
        MESSAGE_HANDLER(WM_ACTIVATEAPP, _OnAppActivate)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, _OnMouseActivate)
        MESSAGE_HANDLER(WM_WININICHANGE , _OnWinIniChange)
        MESSAGE_HANDLER(WM_NCPAINT, _OnNcPaint)
        MESSAGE_HANDLER(WM_CLOSE, _OnClose)
    END_MSG_MAP()

    BEGIN_COM_MAP(CMenuDeskBar)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IBanneredBar, IBanneredBar)
        COM_INTERFACE_ENTRY_IID(IID_IInitializeObject, IInitializeObject)
    END_COM_MAP()

    // *** IMenuPopup methods ***
    STDMETHOD(Popup)(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) override;
    STDMETHOD(OnSelect)(DWORD dwSelectType) override;
    STDMETHOD(SetSubMenu)(IMenuPopup *pmp, BOOL fSet) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, PVOID *ppvSite) override;

    // *** IBanneredBar methods ***
    STDMETHOD(SetIconSize)(DWORD iIcon) override;
    STDMETHOD(GetIconSize)(DWORD* piIcon) override;
    STDMETHOD(SetBitmap)(HBITMAP hBitmap) override;
    STDMETHOD(GetBitmap)(HBITMAP* phBitmap) override;

    // *** IInitializeObject methods ***
    STDMETHOD(Initialize)(THIS) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IInputObjectSite methods ***
    STDMETHOD(OnFocusChangeIS)(LPUNKNOWN lpUnknown, BOOL bFocus) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL bActivating, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)(THIS) override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IDeskBar methods ***
    STDMETHOD(SetClient)(IUnknown *punkClient) override;
    STDMETHOD(GetClient)(IUnknown **ppunkClient) override;
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc) override;

private:
    // message handlers
    LRESULT _OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnAppActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnWinIniChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    HRESULT _AdjustForTheme(BOOL bFlatStyle);
    BOOL _IsSubMenuParent(HWND hwnd);
    HRESULT _CloseBar();
};
