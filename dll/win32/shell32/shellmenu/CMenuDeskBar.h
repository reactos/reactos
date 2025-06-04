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

#include <vector>

typedef CWinTraits<
    WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_PALETTEWINDOW // Added WS_EX_LAYERED for potential transparency
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

    DWORD m_IconSize; // Stores BMICON_SMALL or BMICON_LARGE, set via IBanneredBar::SetIconSize
    HBITMAP m_Banner;

    BOOL  m_Shown;
    DWORD m_ShowFlags;

    BOOL m_didAddRef;

    HWND m_hSearchBox;

    HWND m_hPinnedItemsView;
    HWND m_hFrequentAppsView;

    void _LoadAndDisplayPinnedItemsInView();
    void _ClearPinnedItemsView();
    void _ShowPinnedItemContextMenu(int iItem, POINT pt);

    void _LoadAndDisplayFrequentApps();
    void _ClearFrequentAppsView();
    void _ShowFrequentAppsContextMenu(int iItem, POINT pt);

    virtual void OnFinalMessage(HWND hWnd);
public:
    CMenuDeskBar();
    virtual ~CMenuDeskBar();

    static void RecordAppLaunch(LPCWSTR appPath);
    static void PinItemToStartMenu(LPCITEMIDLIST pidl);

    static const UINT WM_USER_REFRESH_PINNED_ITEMS = WM_USER + 0x100;
    static const UINT WM_USER_REFRESH_FREQUENT_APPS = WM_USER + 0x101;

    DECLARE_REGISTRY_RESOURCEID(IDR_MENUDESKBAR)
    DECLARE_NOT_AGGREGATABLE(CMenuDeskBar)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    // Try adding WS_EX_LAYERED here for transparency. If it causes issues, it will be removed.
    DECLARE_WND_CLASS_EX(_T("BaseBar"), CS_SAVEBITS | CS_DROPSHADOW, COLOR_3DFACE)
    // DECLARE_WND_CLASS_EX(_T("BaseBar"), CS_SAVEBITS | CS_DROPSHADOW, COLOR_3DFACE, WS_EX_LAYERED) would be the change

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
        MESSAGE_HANDLER(WM_COMMAND, _OnCommand)
        MESSAGE_HANDLER(WM_USER_REFRESH_PINNED_ITEMS, _OnRefreshPinnedItems)
        MESSAGE_HANDLER(WM_USER_REFRESH_FREQUENT_APPS, _OnRefreshFrequentApps)
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

    STDMETHOD(Popup)(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) override;
    STDMETHOD(OnSelect)(DWORD dwSelectType) override;
    STDMETHOD(SetSubMenu)(IMenuPopup *pmp, BOOL fSet) override;
    STDMETHOD(GetWindow)(HWND *phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, PVOID *ppvSite) override;
    STDMETHOD(SetIconSize)(DWORD iIcon) override; // This is key for small/large icons
    STDMETHOD(GetIconSize)(DWORD* piIcon) override;
    STDMETHOD(SetBitmap)(HBITMAP hBitmap) override;
    STDMETHOD(GetBitmap)(HBITMAP* phBitmap) override;
    STDMETHOD(Initialize)(THIS) override;
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;
    STDMETHOD(OnFocusChangeIS)(LPUNKNOWN lpUnknown, BOOL bFocus) override;
    STDMETHOD(UIActivateIO)(BOOL bActivating, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)(THIS) override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;
    STDMETHOD(SetClient)(IUnknown *punkClient) override;
    STDMETHOD(GetClient)(IUnknown **ppunkClient) override;
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc) override;

private:
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
    LRESULT _OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnRefreshPinnedItems(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT _OnRefreshFrequentApps(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    HRESULT _AdjustForTheme(BOOL bFlatStyle);
    BOOL _IsSubMenuParent(HWND hwnd);
    HRESULT _CloseBar();
};

static inline BOOL IntersectsRect(const RECT *r1, const RECT *r2) { return !(r2->left >= r1->right || r2->right <= r1->left || r2->top >= r1->bottom || r2->bottom <= r1->top); }
#ifndef IUnknown_OnSelect
#define IUnknown_OnSelect(punk, dwSelType) { CComQIPtr<IMenuPopup> _pmp(punk); if (_pmp) _pmp->OnSelect(dwSelType); }
#endif
#ifndef IUnknown_UIActivateIO
#define IUnknown_UIActivateIO(punk, fActivate, lpMsg) { CComQIPtr<IInputObject> _pio(punk); if (_pio) _pio->UIActivateIO(fActivate, lpMsg); }
#endif
#ifndef IUnknown_HasFocusIO
#define IUnknown_HasFocusIO(punk) ({ HRESULT _hr = S_FALSE; CComQIPtr<IInputObject> _pio(punk); if (_pio) _hr = _pio->HasFocusIO(); _hr; })
#endif
#ifndef IUnknown_TranslateAcceleratorIO
#define IUnknown_TranslateAcceleratorIO(punk, lpMsg) ({ HRESULT _hr = S_FALSE; CComQIPtr<IInputObject> _pio(punk); if (_pio) _hr = _pio->TranslateAcceleratorIO(lpMsg); _hr; })
#endif
#ifndef IUnknown_GetWindow
#define IUnknown_GetWindow(punk, lphwnd) ({ HRESULT _hr = E_FAIL; CComQIPtr<IOleWindow> _pow(punk); if (_pow) _hr = _pow->GetWindow(lphwnd); _hr; })
#endif
#ifndef IUnknown_QueryServiceExec
#define IUnknown_QueryServiceExec(punkClient, SID, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut) ({ CComPtr<IOleCommandTarget> _poct; HRESULT _hr = E_FAIL; if (punkClient) { _hr = IUnknown_QueryService(punkClient, SID, IID_IOleCommandTarget, (void**)&_poct); if (SUCCEEDED(_hr)) _hr = _poct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut); } _hr; })
#endif

static void AdjustForExcludeArea(BOOL alignLeft, BOOL alignTop, BOOL preferVertical, PINT px, PINT py, INT cx, INT cy, RECTL rcExclude) { RECT rcWindow = { *px, *py, *px + cx, *py + cy }; if (IntersectsRect(&rcWindow, &rcExclude)) { if (preferVertical) { if (alignTop && rcWindow.bottom > rcExclude.top) *py = rcExclude.top - cy; else if (!alignTop && rcWindow.top < rcExclude.bottom) *py = rcExclude.bottom; else if (alignLeft && rcWindow.right > rcExclude.left) *px = rcExclude.left - cx; else if (!alignLeft && rcWindow.left < rcExclude.right) *px = rcExclude.right; } else { if (alignLeft && rcWindow.right > rcExclude.left) *px = rcExclude.left - cx; else if (!alignLeft && rcWindow.left < rcExclude.right) *px = rcExclude.right; else if (alignTop && rcWindow.bottom > rcExclude.top) *py = rcExclude.top - cy; else if (!alignTop && rcWindow.top < rcExclude.bottom) *py = rcExclude.bottom; } } }
