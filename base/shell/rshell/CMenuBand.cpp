/*
* Shell Menu Band
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
#include "precomp.h"
#include <windowsx.h>
#include <shlwapi_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);

#define TBSTYLE_EX_VERTICAL 4

#define TIMERID_HOTTRACK 1
#define SUBCLASS_ID_MENUBAND 1

extern "C" BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

class CMenuBand;

class CMenuToolbarBase
{
public:
    CMenuToolbarBase(CMenuBand *menuBand);
    virtual ~CMenuToolbarBase() {}

    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowWindow(BOOL fShow);
    HRESULT Close();

    BOOL IsWindowOwner(HWND hwnd) { return m_hwnd && m_hwnd == hwnd; }

    virtual HRESULT FillToolbar() = 0;
    virtual HRESULT PopupItem(UINT uItem) = 0;
    virtual HRESULT HasSubMenu(UINT uItem) = 0;
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick) = 0;

    HRESULT OnHotItemChange(const NMTBHOTITEM * hot);

    HRESULT PopupSubMenu(UINT index, IShellMenu* childShellMenu);
    HRESULT PopupSubMenu(UINT index, HMENU menu);
    HRESULT DoContextMenu(IContextMenu* contextMenu);

    static LRESULT CALLBACK s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:

    static const UINT WM_USER_SHOWPOPUPMENU = WM_USER + 1;

    LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMenuBand *m_menuBand;
    HWND m_hwnd;
    DWORD m_dwMenuFlags;
    INT m_hotItem;
    WNDPROC m_SubclassOld;
};

class CMenuStaticToolbar : public CMenuToolbarBase
{
public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar() {}

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

private:
    HMENU m_hmenu;
};

class CMenuSFToolbar : public CMenuToolbarBase
{
public:
    CMenuSFToolbar(CMenuBand *menuBand);
    virtual ~CMenuSFToolbar();

    HRESULT SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    HRESULT GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

private:
    LPITEMIDLIST GetPidlFromId(UINT uItem, INT* pIndex);

    IShellFolder * m_shellFolder;
    LPCITEMIDLIST m_idList;
    HKEY m_hKey;
};

class CMenuBand :
    public CComCoClass<CMenuBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IMenuPopup,
    public IMenuBand,
    public IShellMenu2,
    public IWinEventHandler,
    public IShellMenuAcc
{
public:
    CMenuBand();
    ~CMenuBand();

private:
    IOleWindow *m_site;
    IShellMenuCallback *m_psmc;

    CMenuStaticToolbar *m_staticToolbar;
    CMenuSFToolbar *m_SFToolbar;

    UINT m_uId;
    UINT m_uIdAncestor;
    DWORD m_dwFlags;
    PVOID m_UserData;
    HMENU m_hmenu;
    HWND m_menuOwner;

    BOOL m_useBigIcons;

    HWND m_topLevelWindow;

public:

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc);

    // *** IMenuBand methods ***
    virtual HRESULT STDMETHODCALLTYPE IsMenuMessage(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE TranslateMenuMessage(MSG *pmsg, LRESULT *plRet);

    // *** IShellMenu methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetState(LPSMDATA psmd);
    virtual HRESULT STDMETHODCALLTYPE SetMenuToolbar(IUnknown *punk, DWORD dwFlags);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IShellMenu2 methods ***
    virtual HRESULT STDMETHODCALLTYPE GetSubMenu(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetToolbar(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetMinWidth(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetNoBorder(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetTheme(THIS);

    // *** IShellMenuAcc methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTop(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetBottom(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetTracked(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetParentSite(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetState(THIS);
    virtual HRESULT STDMETHODCALLTYPE DoDefaultAction(THIS);
    virtual HRESULT STDMETHODCALLTYPE IsEmpty(THIS);

    HRESULT CallCBWithId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT CallCBWithPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT TrackPopup(HMENU popup, INT x, INT y);

    BOOL UseBigIcons() {
        return m_useBigIcons;
    }

    DECLARE_NOT_AGGREGATABLE(CMenuBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu, IShellMenu)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IMenuBand, IMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu2, IShellMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuAcc, IShellMenuAcc)
    END_COM_MAP()

private:
    HRESULT _CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id = 0, LPITEMIDLIST pidl = NULL);
};

extern "C"
HRESULT CMenuBand_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuBand * site = new CComObject<CMenuBand>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand) :
    m_menuBand(menuBand),
    m_hwnd(NULL),
    m_dwMenuFlags(0)
{
}

HRESULT CMenuToolbarBase::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    HIMAGELIST ilBig, ilSmall;
    Shell_GetImageLists(&ilBig, &ilSmall);

    if (m_menuBand->UseBigIcons())
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilBig));
    }
    else
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilSmall));
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    return S_OK;
}

HRESULT CMenuToolbarBase::CreateToolbar(HWND hwndParent, DWORD dwFlags)
{
    LONG tbStyles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
        TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_CUSTOMERASE |
        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP;
    LONG tbExStyles = TBSTYLE_EX_DOUBLEBUFFER;

    if (dwFlags & SMINIT_VERTICAL)
    {
        tbStyles |= CCS_VERT;
        tbExStyles |= TBSTYLE_EX_VERTICAL | WS_EX_TOOLWINDOW;
    }

    RECT rc;

    if (!::GetClientRect(hwndParent, &rc) || (rc.left == rc.right) || (rc.top == rc.bottom))
    {
        rc.left = 0;
        rc.top = 0;
        rc.right = 1;
        rc.bottom = 1;
    }

    HWND hwndToolbar = CreateWindowEx(
        tbExStyles, TOOLBARCLASSNAMEW, NULL,
        tbStyles, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hwndParent, NULL, _AtlBaseModule.GetModuleInstance(), 0);

    if (hwndToolbar == NULL)
        return E_FAIL;

    ::SetParent(hwndToolbar, hwndParent);

    m_hwnd = hwndToolbar;

    /* Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure */
    SendMessageW(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    HIMAGELIST ilBig, ilSmall;
    Shell_GetImageLists(&ilBig, &ilSmall);

    //if (dwFlags & SMINIT_TOPLEVEL)
    //{
    //    /* Hide the placeholders for the button images */
    //    SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
    //}
    //else
    if (m_menuBand->UseBigIcons())
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilBig));
    }
    else
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilSmall));
    }

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_SubclassOld = (WNDPROC) SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CMenuToolbarBase::s_SubclassProc));

    return S_OK;
}

HRESULT CMenuToolbarBase::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    *phwnd = m_hwnd;

    return S_OK;
}

LRESULT CALLBACK CMenuToolbarBase::s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMenuToolbarBase * pthis = reinterpret_cast<CMenuToolbarBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return pthis->SubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CMenuToolbarBase::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == TIMERID_HOTTRACK)
        {
            PopupItem(m_hotItem);
            KillTimer(hWnd, TIMERID_HOTTRACK);
        }
    }

    return m_SubclassOld(hWnd, uMsg, wParam, lParam);
}

HRESULT CMenuToolbarBase::OnHotItemChange(const NMTBHOTITEM * hot)
{
    if (hot->dwFlags & HICF_LEAVING)
    {
        KillTimer(m_hwnd, TIMERID_HOTTRACK);
    }
    else if (m_hotItem != hot->idNew)
    {
        if (HasSubMenu(hot->idNew) == S_OK)
        {
            DWORD elapsed = 0;
            SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &elapsed, 0);

            m_hotItem = hot->idNew;

            SetTimer(m_hwnd, TIMERID_HOTTRACK, elapsed, NULL);
        }
    }

    m_menuBand->OnSelect(MPOS_CHILDTRACKING);
    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT index, IShellMenu* childShellMenu)
{
    IBandSite* pBandSite;
    IDeskBar* pDeskBar;

    HRESULT hr = 0;
    RECT rc = { 0 };

    if (!SendMessage(m_hwnd, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwnd, &a);
    ClientToScreen(m_hwnd, &b);

    POINTL pt = { b.x, b.y };
    RECTL rcl = { a.x, a.y, b.x, b.y }; // maybe-TODO: fetch client area of deskbar?


#if USE_SYSTEM_MENUSITE
    hr = CoCreateInstance(CLSID_MenuBandSite,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IBandSite, &pBandSite));
#else
    hr = CMenuSite_Constructor(IID_PPV_ARG(IBandSite, &pBandSite));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUSITE
    hr = CMenuSite_Wrapper(pBandSite, IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED(hr))
        return hr;
#endif

#if USE_SYSTEM_MENUDESKBAR
    hr = CoCreateInstance(CLSID_MenuDeskBar,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IDeskBar, &pDeskBar));
#else
    hr = CMenuDeskBar_Constructor(IID_PPV_ARG(IDeskBar, &pDeskBar));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUDESKBAR
    hr = CMenuDeskBar_Wrapper(pDeskBar, IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED(hr))
        return hr;
#endif

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED(hr))
        return hr;

    hr = pBandSite->AddBand(childShellMenu);
    if (FAILED(hr))
        return hr;

    CComPtr<IMenuPopup> popup;
    hr = pDeskBar->QueryInterface(IID_PPV_ARG(IMenuPopup, &popup));
    if (FAILED(hr))
        return hr;

    popup->Popup(&pt, &rcl, MPPF_TOP | MPPF_RIGHT);

    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT index, HMENU menu)
{
    RECT rc = { 0 };

    if (!SendMessage(m_hwnd, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwnd, &b);

    HMENU popup = GetSubMenu(menu, index);

    m_menuBand->TrackPopup(popup, b.x, b.y);

    return S_OK;
}

HRESULT CMenuToolbarBase::DoContextMenu(IContextMenu* contextMenu)
{
    HRESULT hr;
    HMENU hPopup = CreatePopupMenu();

    if (hPopup == NULL)
        return E_FAIL;

    hr = contextMenu->QueryContextMenu(hPopup, 0, 0, UINT_MAX, CMF_NORMAL);
    if (FAILED(hr))
    {
        DestroyMenu(hPopup);
        return hr;
    }

    DWORD dwPos = GetMessagePos();
    UINT uCommand = ::TrackPopupMenu(hPopup, TPM_RETURNCMD, GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, m_hwnd, NULL);
    if (uCommand == 0)
        return S_FALSE;

    CMINVOKECOMMANDINFO cmi = { 0 };
    cmi.cbSize = sizeof(cmi);
    cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
    cmi.hwnd = m_hwnd;
    hr = contextMenu->InvokeCommand(&cmi);

    DestroyMenu(hPopup);
    return hr;
}

HRESULT CMenuToolbarBase::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    m_menuBand->OnSelect(MPOS_EXECUTE);
    return S_OK;
}

BOOL
AllocAndGetMenuString(HMENU hMenu, UINT ItemIDByPosition, WCHAR** String)
{
    int Length;

    Length = GetMenuStringW(hMenu, ItemIDByPosition, NULL, 0, MF_BYPOSITION);

    if (!Length)
        return FALSE;

    /* Also allocate space for the terminating NULL character */
    ++Length;
    *String = (PWSTR) HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));

    GetMenuStringW(hMenu, ItemIDByPosition, *String, Length, MF_BYPOSITION);

    return TRUE;
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand) :
    CMenuToolbarBase(menuBand),
    m_hmenu(NULL)
{
}

HRESULT  CMenuStaticToolbar::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    *phmenu = m_hmenu;
    *phwnd = NULL;
    *pdwFlags = m_dwMenuFlags;

    return S_OK;
}

HRESULT  CMenuStaticToolbar::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    m_hmenu = hmenu;
    m_dwMenuFlags = dwFlags;

    return S_OK;
}

HRESULT CMenuStaticToolbar::FillToolbar()
{
    int i;
    int ic = GetMenuItemCount(m_hmenu);

    for (i = 0; i < ic; i++)
    {
        MENUITEMINFOW info;
        TBBUTTON tbb = { 0 };
        PWSTR MenuString = NULL;

        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = 0;

        info.cbSize = sizeof(info);
        info.fMask = MIIM_FTYPE | MIIM_ID;

        GetMenuItemInfoW(m_hmenu, i, TRUE, &info);

        if (info.fType == MFT_STRING)
        {
            if (!AllocAndGetMenuString(m_hmenu, i, &MenuString))
                return E_OUTOFMEMORY;
            if (::GetSubMenu(m_hmenu, i) != NULL)
                tbb.fsStyle |= BTNS_DROPDOWN;
            tbb.iString = (INT_PTR) MenuString;
            tbb.idCommand = info.wID;

            SMINFO * sminfo = new SMINFO();
            sminfo->dwMask = SMIM_ICON | SMIM_FLAGS;
            if (SUCCEEDED(m_menuBand->CallCBWithId(info.wID, SMC_GETINFO, 0, reinterpret_cast<LPARAM>(sminfo))))
            {
                tbb.iBitmap = sminfo->iIcon;
                tbb.dwData = reinterpret_cast<DWORD_PTR>(sminfo);
                // FIXME: remove before deleting the toolbar or it will leak
            }
        }
        else
        {
            tbb.fsStyle |= BTNS_SEP;
        }

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));

        if (MenuString)
            HeapFree(GetProcessHeap(), 0, MenuString);
    }

    return S_OK;
}

HRESULT CMenuStaticToolbar::OnContextMenu(NMMOUSE * rclick)
{
    CComPtr<IContextMenu> contextMenu;
    HRESULT hr = m_menuBand->CallCBWithId(rclick->dwItemSpec, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IContextMenu), reinterpret_cast<LPARAM>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuStaticToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr = m_menuBand->CallCBWithId(wParam, SMC_EXEC, 0, 0);
    if (FAILED(hr))
        return hr;

    return CMenuToolbarBase::OnCommand(wParam, lParam, theResult);
}

HRESULT CMenuStaticToolbar::PopupItem(UINT uItem)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;
    
    TBBUTTON btn = { 0 };
    SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn));

    SMINFO * nfo = reinterpret_cast<SMINFO*>(btn.dwData);
    if (!nfo)
        return E_FAIL;

    if (nfo->dwFlags&SMIF_TRACKPOPUP)
    {
        return PopupSubMenu(index, m_hmenu);
    }
    else
    {
        CComPtr<IShellMenu> shellMenu;
        HRESULT hr = m_menuBand->CallCBWithId(uItem, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IShellMenu), reinterpret_cast<LPARAM>(&shellMenu));
        if (FAILED(hr))
            return hr;

        return PopupSubMenu(index, shellMenu);
    }
}

HRESULT CMenuStaticToolbar::HasSubMenu(UINT uItem)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;
    return ::GetSubMenu(m_hmenu, index) ? S_OK : S_FALSE;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand * menuBand) :
    CMenuToolbarBase(menuBand),
    m_shellFolder(NULL)
{
}

CMenuSFToolbar::~CMenuSFToolbar()
{
}

HRESULT CMenuSFToolbar::FillToolbar()
{
    HRESULT hr;
    TBBUTTON tbb = { 0 };
    int i = 0;
    PWSTR MenuString;

    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = 0;

    IEnumIDList * eidl;
    m_shellFolder->EnumObjects(m_hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);

    LPITEMIDLIST item = static_cast<LPITEMIDLIST>(CoTaskMemAlloc(sizeof(ITEMIDLIST)));
    ULONG fetched;
    while ((hr = eidl->Next(1, &item, &fetched)) == S_OK)
    {
        INT index = 0;
        INT indexOpen = 0;

        CComPtr<IShellItem> psi;
        SHCreateShellItem(NULL, m_shellFolder, item, &psi);

        hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &MenuString);
        if (FAILED(hr))
            return hr;

        index = SHMapPIDLToSystemImageListIndex(m_shellFolder, item, &indexOpen);

        SFGAOF attrs;
        hr = psi->GetAttributes(SFGAO_FOLDER, &attrs);

        if (attrs != 0)
        {
            tbb.fsStyle |= BTNS_DROPDOWN;
        }

        tbb.idCommand = i++;
        tbb.iString = (INT_PTR) MenuString;
        tbb.iBitmap = index;
        tbb.dwData = reinterpret_cast<DWORD_PTR>(ILClone(item));
        // FIXME: remove before deleting the toolbar or it will leak

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));
        HeapFree(GetProcessHeap(), 0, MenuString);

    }
    CoTaskMemFree(item);

    return hr;
}

HRESULT CMenuSFToolbar::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    m_shellFolder = psf;
    m_idList = pidlFolder;
    m_hKey = hKey;
    m_dwMenuFlags = dwFlags;
    return S_OK;
}

HRESULT CMenuSFToolbar::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    HRESULT hr;

    hr = m_shellFolder->QueryInterface(riid, ppv);
    if (FAILED(hr))
        return hr;

    if (pdwFlags)
        *pdwFlags = m_dwMenuFlags;

    if (ppidl)
    {
        LPITEMIDLIST pidl = NULL;

        if (m_idList)
        {
            pidl = ILClone(m_idList);
            if (!pidl)
            {
                (*(IUnknown**) ppv)->Release();
                return E_FAIL;
            }
        }

        *ppidl = pidl;
    }

    return hr;
}

LPITEMIDLIST CMenuSFToolbar::GetPidlFromId(UINT uItem, INT* pIndex)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return NULL;

    if (pIndex)
        *pIndex = index;

    TBBUTTON btn = { 0 };
    if (!SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn)))
        return NULL;

    return reinterpret_cast<LPITEMIDLIST>(btn.dwData);
}

HRESULT CMenuSFToolbar::OnContextMenu(NMMOUSE * rclick)
{
    HRESULT hr;
    CComPtr<IContextMenu> contextMenu;
    LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(rclick->dwItemData);

    hr = m_shellFolder->GetUIObjectOf(m_hwnd, 1, &pidl, IID_IContextMenu, NULL, reinterpret_cast<VOID **>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuSFToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    return m_menuBand->CallCBWithPidl(GetPidlFromId(wParam, NULL), SMC_SFEXEC, 0, 0);
}

HRESULT CMenuSFToolbar::PopupItem(UINT uItem)
{
    HRESULT hr;
    UINT uId;
    UINT uIdAncestor;
    DWORD flags;
    int index;
    CComPtr<IShellMenuCallback> psmc;
    CComPtr<IShellMenu> shellMenu;
    
    LPITEMIDLIST pidl = GetPidlFromId(uItem, &index);

    if (!pidl)
        return E_FAIL;

#if USE_SYSTEM_MENUBAND
    hr = CoCreateInstance(CLSID_MenuBand,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IShellMenu, &shellMenu));
#else
    hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &shellMenu));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUBAND
    hr = CMenuBand_Wrapper(shellMenu, IID_PPV_ARG(IShellMenu, &shellMenu));
    if (FAILED(hr))
        return hr;
#endif

    m_menuBand->GetMenuInfo(&psmc, &uId, &uIdAncestor, &flags);

    // FIXME: not sure waht to use as uId/uIdAncestor here
    hr = shellMenu->Initialize(psmc, 0, uId, SMINIT_VERTICAL);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> childFolder;
    hr = m_shellFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &childFolder));
    if (FAILED(hr))
        return hr;

    hr = shellMenu->SetShellFolder(childFolder, NULL, NULL, 0);
    if (FAILED(hr))
        return hr;

    return PopupSubMenu(index, shellMenu);
}

HRESULT CMenuSFToolbar::HasSubMenu(UINT uItem)
{
    HRESULT hr;
    CComPtr<IShellItem> psi;
    SHCreateShellItem(NULL, m_shellFolder, GetPidlFromId(uItem, NULL), &psi);

    SFGAOF attrs;
    hr = psi->GetAttributes(SFGAO_FOLDER, &attrs);
    if (FAILED(hr))
        return hr;

    return (attrs != 0) ? S_OK : S_FALSE;
}

CMenuBand::CMenuBand() :
    m_site(NULL),
    m_psmc(NULL),
    m_staticToolbar(NULL),
    m_SFToolbar(NULL),
    m_useBigIcons(FALSE)
{
}

CMenuBand::~CMenuBand()
{
    if (m_site)
        m_site->Release();

    if (m_psmc)
        m_psmc->Release();

    if (m_staticToolbar)
        delete m_staticToolbar;

    if (m_SFToolbar)
        delete m_SFToolbar;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::Initialize(
    IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags)
{
    if (m_psmc)
        m_psmc->Release();

    m_psmc = psmc;
    m_uId = uId;
    m_uIdAncestor = uIdAncestor;
    m_dwFlags = dwFlags;

    if (m_psmc)
    {
        m_psmc->AddRef();

        _CallCB(SMC_CREATE, 0, reinterpret_cast<LPARAM>(&m_UserData));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenuInfo(
    IShellMenuCallback **ppsmc,
    UINT *puId,
    UINT *puIdAncestor,
    DWORD *pdwFlags)
{
    if (!pdwFlags) // maybe?
        return E_INVALIDARG;

    if (ppsmc)
        *ppsmc = m_psmc;

    if (puId)
        *puId = m_uId;

    if (puIdAncestor)
        *puIdAncestor = m_uIdAncestor;

    *pdwFlags = m_dwFlags;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    if (m_staticToolbar == NULL)
    {
        m_staticToolbar = new CMenuStaticToolbar(this);
    }
    m_hmenu = hmenu;
    m_menuOwner;

    HRESULT hr = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    if (m_staticToolbar == NULL)
        return E_FAIL;

    return m_staticToolbar->GetMenu(phmenu, phwnd, pdwFlags);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetSite(IUnknown *pUnkSite)
{
    HWND    hwndParent;
    HRESULT hr;

    if (m_site != NULL)
        m_site->Release();

    if (pUnkSite == NULL)
        return S_OK;

    hwndParent = NULL;
    hr = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (SUCCEEDED(hr))
    {
        m_site->Release();

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;
    }

    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    if (m_SFToolbar != NULL)
    {
        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    CComPtr<IOleWindow> pTopLevelWindow;
    hr = IUnknown_QueryService(m_site, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
    if (FAILED(hr))
        return hr;

    return pTopLevelWindow->GetWindow(&m_topLevelWindow);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(
    HWND *phwnd)
{
    if (m_SFToolbar != NULL)
        return m_SFToolbar->GetWindow(phwnd);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    SIZE sizeStaticY = { 0 };
    SIZE sizeShlFldY = { 0 };
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hr))
        return hr;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStaticY));
    if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFldY));

    int sy = max(prc->bottom - prc->top, sizeStaticY.cy + sizeShlFldY.cy);

    if (hwndShlFld)
    {
        SetWindowPos(hwndShlFld, NULL,
            prc->left,
            prc->top,
            prc->right - prc->left,
            sizeShlFldY.cy,
            0);
        DWORD btnSize = SendMessage(hwndShlFld, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndShlFld, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }
    if (hwndStatic)
    {
        SetWindowPos(hwndStatic, hwndShlFld,
            prc->left,
            prc->top + sizeShlFldY.cy,
            prc->right - prc->left,
            sy - sizeShlFldY.cy,
            0);
        DWORD btnSize = SendMessage(hwndStatic, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndStatic, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hr))
        return hr;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    // HACK (?)
    if (pdbi->dwMask == 0)
    {
        pdbi->dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL | DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;
    }

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFld));

        pdbi->ptMinSize.x = 0;
        pdbi->ptMinSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeShlFld));

        pdbi->ptMaxSize.x = max(sizeStatic.cx, sizeShlFld.cx); // ignored
        pdbi->ptMaxSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&sizeShlFld));
        pdbi->ptActual.x = max(sizeStatic.cx, sizeShlFld.cx);

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFld));
        pdbi->ptActual.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

/* IDockingWindow */
HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;
    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;

    if (fShow)
        return _CallCB(SMC_INITMENU, 0, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->Close();

    if (m_SFToolbar != NULL)
        return m_SFToolbar->Close();

    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMenuBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr;

    CComPtr<IMenuPopup> pmp;

    hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp));
    if (FAILED(hr))
        return hr;

    hr = pmp->SetSubMenu(this, TRUE);
    if (FAILED(hr))
        return hr;

    CComPtr<IOleWindow> pTopLevelWindow;
    hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
    if (FAILED(hr))
        return hr;

    hr = pTopLevelWindow->GetWindow(&m_topLevelWindow);
    if (FAILED(hr))
        return hr;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::HasFocusIO()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsDirty()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClassID(CLSID *pClassID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (!pguidCmdGroup)
        return E_FAIL;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    {
        if (nCmdID == 16) // set (big) icon size
        {
            this->m_useBigIcons = TRUE;
            return S_OK;
        }
        else if (nCmdID == 19) // popup-related
        {
            return S_FALSE;
        }
    }

    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_SMenuBandChild) ||
        IsEqualIID(guidService, SID_SMenuBandBottom) || 
        IsEqualIID(guidService, SID_SMenuBandBottomSelected))
        return this->QueryInterface(riid, ppvObject);
    WARN("Unknown service requested %s\n", wine_dbgstr_guid(&guidService));
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    CComPtr<IMenuPopup> pmp;
    HRESULT hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp));
    if (FAILED(hr))
        return hr;
    pmp->OnSelect(dwSelectType);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    //UNIMPLEMENTED;
    //return S_OK;
    return S_FALSE;
    //return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //UNIMPLEMENTED;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    if (m_SFToolbar == NULL)
    {
        m_SFToolbar = new CMenuSFToolbar(this);
    }

    HRESULT hr = m_SFToolbar->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    if (m_SFToolbar)
        return m_SFToolbar->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(LPSMDATA psmd)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:

        if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
        {
            return m_staticToolbar->OnCommand(wParam, lParam, theResult);
        }

        if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
        {
            return m_SFToolbar->OnCommand(wParam, lParam, theResult);
        }

        return S_OK;

    case WM_NOTIFY:
        NMHDR * hdr = reinterpret_cast<LPNMHDR>(lParam);
        NMTBCUSTOMDRAW * cdraw;
        NMTBHOTITEM * hot;
        NMMOUSE * rclick;
        switch (hdr->code)
        {
        case TBN_HOTITEMCHANGE:
            hot = reinterpret_cast<LPNMTBHOTITEM>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
            {
                return m_staticToolbar->OnHotItemChange(hot);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
            {
                return m_SFToolbar->OnHotItemChange(hot);
            }

            return S_OK;

        case NM_RCLICK:
            rclick = reinterpret_cast<LPNMMOUSE>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
            {
                return m_staticToolbar->OnContextMenu(rclick);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
            {
                return m_SFToolbar->OnContextMenu(rclick);
            }

            return S_OK;
        case NM_CUSTOMDRAW:
            cdraw = reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr);
            switch (cdraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                *theResult = CDRF_NOTIFYITEMDRAW;
                return S_OK;

            case CDDS_ITEMPREPAINT:

                cdraw->clrBtnFace = GetSysColor(COLOR_MENU);
                cdraw->clrBtnHighlight = GetSysColor(COLOR_MENUHILIGHT);

                cdraw->clrText = GetSysColor(COLOR_MENUTEXT);
                cdraw->clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
                cdraw->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHTTEXT);

                RECT rc = cdraw->nmcd.rc;
                HDC hdc = cdraw->nmcd.hdc;

                HBRUSH bgBrush = GetSysColorBrush(COLOR_MENU);
                HBRUSH hotBrush = GetSysColorBrush(COLOR_MENUHILIGHT);

                switch (cdraw->nmcd.uItemState)
                {
                case CDIS_HOT:
                case CDIS_FOCUS:
                    FillRect(hdc, &rc, hotBrush);
                    break;
                default:
                    FillRect(hdc, &rc, bgBrush);
                    break;
                }

                *theResult = TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOETCHEDEFFECT | TBCDRF_HILITEHOTTRACK | TBCDRF_NOOFFSET;
                return S_OK;
            }
            return S_OK;
        }
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
        return S_OK;

    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
        return S_OK;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSubMenu(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetToolbar(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMinWidth(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetNoBorder(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetTheme(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTop(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetBottom(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTracked(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetParentSite(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::DoDefaultAction(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsEmpty(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT CMenuBand::CallCBWithId(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, id);
}

HRESULT CMenuBand::CallCBWithPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, 0, pidl);
}

HRESULT CMenuBand::_CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id, LPITEMIDLIST pidl)
{
    if (!m_psmc)
        return S_FALSE;

    HWND hwnd;
    GetWindow(&hwnd);

    SMDATA smData = { 0 };
    smData.punk = static_cast<IShellMenu2*>(this);
    smData.uId = id;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.hwnd = hwnd;
    smData.pidlItem = pidl;
    if (m_staticToolbar)
    {
        smData.hmenu = m_hmenu;
    }
    smData.pvUserData = NULL;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}

HRESULT CMenuBand::TrackPopup(HMENU popup, INT x, INT y)
{
    ::TrackPopupMenu(popup, 0, x, y, 0, m_menuOwner, NULL);
    return S_OK;
}
