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
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IBanneredBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetIconSize(DWORD iIcon);
    virtual HRESULT STDMETHODCALLTYPE GetIconSize(DWORD* piIcon);
    virtual HRESULT STDMETHODCALLTYPE SetBitmap(HBITMAP hBitmap);
    virtual HRESULT STDMETHODCALLTYPE GetBitmap(HBITMAP* phBitmap);

    // *** IInitializeObject methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(THIS);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(LPUNKNOWN lpUnknown, BOOL bFocus);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL bActivating, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO(THIS);
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(LPRECT prc);

private:
    // message handlers
    LRESULT _OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnAppActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BOOL _IsSubMenuParent(HWND hwnd);
    HRESULT _CloseBar();
};
