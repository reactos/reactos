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
#include <CommonControls.h>
#include <shlwapi_undoc.h>

#include "CMenuBand.h"
#include "CMenuToolbars.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMenuToolbars);

extern "C"
HRESULT WINAPI SHGetImageList(
    _In_   int iImageList,
    _In_   REFIID riid,
    _Out_  void **ppv
    );

// FIXME: Enable if/when wine comctl supports this flag properly
//#define TBSTYLE_EX_VERTICAL 4

#define TIMERID_HOTTRACK 1
#define SUBCLASS_ID_MENUBAND 1

HRESULT CMenuToolbarBase::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    RECT rc;
    HDC hdc;
    HBRUSH bgBrush;
    HBRUSH hotBrush;
    NMHDR * hdr;
    NMTBCUSTOMDRAW * cdraw;
    NMTBHOTITEM * hot;
    NMMOUSE * rclick;
    NMPGCALCSIZE* csize;
    TBBUTTONINFO btni;
    COLORREF clrText;
    COLORREF clrTextHighlight;
    SIZE tbs;
    bool isHot, isPopup;

    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:
        return OnCommand(wParam, lParam, theResult);

    case WM_NOTIFY:
        hdr = reinterpret_cast<LPNMHDR>(lParam);
        switch (hdr->code)
        {
        case PGN_CALCSIZE:
            csize = reinterpret_cast<LPNMPGCALCSIZE>(hdr);

            GetIdealSize(tbs);
            if (csize->dwFlag == PGF_CALCHEIGHT)
            {
                csize->iHeight = tbs.cy;
            }
            else if (csize->dwFlag == PGF_CALCWIDTH)
            {
                csize->iHeight = tbs.cx;
            }
            return S_OK;

        case TBN_DROPDOWN:
            wParam = reinterpret_cast<LPNMTOOLBAR>(hdr)->iItem;
            return OnCommand(wParam, 0, theResult);

        case TBN_HOTITEMCHANGE:
            hot = reinterpret_cast<LPNMTBHOTITEM>(hdr);
            return OnHotItemChange(hot);

        case NM_RCLICK:
            rclick = reinterpret_cast<LPNMMOUSE>(hdr);

            return OnContextMenu(rclick);

        case NM_CUSTOMDRAW:
            cdraw = reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr);
            switch (cdraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                *theResult = CDRF_NOTIFYITEMDRAW;
                return S_OK;

            case CDDS_ITEMPREPAINT:
                
                clrText = GetSysColor(COLOR_MENUTEXT);
                clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);

                bgBrush = GetSysColorBrush(COLOR_MENU);
                hotBrush = GetSysColorBrush(m_useFlatMenus ? COLOR_MENUHILIGHT : COLOR_HIGHLIGHT);

                rc = cdraw->nmcd.rc;
                hdc = cdraw->nmcd.hdc;

                isHot = m_hotBar == this && m_hotItem == static_cast<INT>(cdraw->nmcd.dwItemSpec);
                isPopup = m_popupBar == this && m_popupItem == static_cast<INT>(cdraw->nmcd.dwItemSpec);

                if (isHot || (m_hotItem < 0 && isPopup))
                {
                    cdraw->nmcd.uItemState |= CDIS_HOT;
                }
                else
                {
                    cdraw->nmcd.uItemState &= ~CDIS_HOT;
                }

                if (cdraw->nmcd.uItemState&CDIS_HOT)
                {
                    FillRect(hdc, &rc, hotBrush);
                    SetTextColor(hdc, clrTextHighlight);
                    cdraw->clrText = clrTextHighlight;
                }
                else
                {
                    FillRect(hdc, &rc, bgBrush);
                    SetTextColor(hdc, clrText);
                    cdraw->clrText = clrText;
                }

                cdraw->iListGap += 4;

                *theResult = CDRF_NOTIFYPOSTPAINT | TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK | 0x00800000; // FIXME: the last bit is Vista+, for debugging only
                return S_OK;

            case CDDS_ITEMPOSTPAINT:
                btni.cbSize = sizeof(btni);
                btni.dwMask = TBIF_STYLE;
                SendMessage(hWnd, TB_GETBUTTONINFO, cdraw->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&btni));
                if (btni.fsStyle & BTNS_DROPDOWN)
                {
                    SelectObject(cdraw->nmcd.hdc, m_marlett);
                    WCHAR text[] = L"8";
                    SetBkMode(cdraw->nmcd.hdc, TRANSPARENT);
                    RECT rc = cdraw->nmcd.rc;
                    rc.right += 1;
                    DrawTextEx(cdraw->nmcd.hdc, text, 1, &rc, DT_NOCLIP | DT_VCENTER | DT_RIGHT | DT_SINGLELINE, NULL);
                }
                *theResult = TRUE;
                return S_OK;
            }
            return S_OK;
        }
        return S_OK;
    }

    return S_FALSE;
}

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager) :
    m_hwnd(NULL),
    m_useFlatMenus(FALSE),
    m_SubclassOld(NULL),
    m_menuBand(menuBand),
    m_hwndToolbar(NULL),
    m_dwMenuFlags(0),
    m_hasIdealSize(FALSE),
    m_usePager(usePager),
    m_hotItem(-1),
    m_popupItem(-1)
{
    m_marlett = CreateFont(
        0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FF_DONTCARE, L"Marlett");
}

CMenuToolbarBase::~CMenuToolbarBase()
{
    DeleteObject(m_marlett);
}

HRESULT CMenuToolbarBase::IsWindowOwner(HWND hwnd)
{
    return (m_hwnd && m_hwnd == hwnd) ||
           (m_hwndToolbar && m_hwndToolbar == hwnd) ? S_OK : S_FALSE;
}

void CMenuToolbarBase::InvalidateDraw()
{
    InvalidateRect(m_hwnd, NULL, FALSE);
}

HRESULT CMenuToolbarBase::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    UpdateImageLists();

    SystemParametersInfo(SPI_GETFLATMENU, 0, &m_useFlatMenus, 0);

    return S_OK;
}

HRESULT CMenuToolbarBase::UpdateImageLists()
{
    int shiml;
    if (m_menuBand->UseBigIcons())
    {
        shiml = SHIL_LARGE;
        SendMessageW(m_hwndToolbar, TB_SETPADDING, 0, MAKELPARAM(4, 0));
    }
    else
    {
        shiml = SHIL_SMALL;
        SendMessageW(m_hwndToolbar, TB_SETPADDING, 0, MAKELPARAM(4, 4));
    }

    IImageList * piml;
    HRESULT hr = SHGetImageList(shiml, IID_PPV_ARG(IImageList, &piml));
    if (SUCCEEDED(hr))
    {
        SendMessageW(m_hwndToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
    }
    else
    {
        SendMessageW(m_hwndToolbar, TB_SETIMAGELIST, 0, 0);
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    DestroyWindow(m_hwndToolbar);
    if (m_hwndToolbar != m_hwnd)
        DestroyWindow(m_hwnd);
    m_hwndToolbar = NULL;
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

#ifdef TBSTYLE_EX_VERTICAL
        // FIXME: Use when it works in ros (?)
        tbExStyles |= TBSTYLE_EX_VERTICAL | WS_EX_TOOLWINDOW;
#endif
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

    if (m_usePager)
    {
        LONG pgStyles = PGS_VERT | WS_CHILD | WS_VISIBLE;
        LONG pgExStyles = 0;

        HWND hwndPager = CreateWindowEx(
            pgExStyles, WC_PAGESCROLLER, NULL,
            pgStyles, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            hwndParent, NULL, _AtlBaseModule.GetModuleInstance(), 0);

        ::SetParent(hwndToolbar, hwndPager);
        ::SetParent(hwndPager, hwndParent);

        SendMessage(hwndPager, PGM_SETCHILD, 0, reinterpret_cast<LPARAM>(hwndToolbar));
        m_hwndToolbar = hwndToolbar;
        m_hwnd = hwndPager;
    }
    else
    {
        ::SetParent(hwndToolbar, hwndParent);
        m_hwndToolbar = hwndToolbar;
        m_hwnd = hwndToolbar;
    }
    
    /* Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure */
    SendMessageW(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    //if (dwFlags & SMINIT_TOPLEVEL)
    //{
    //    /* Hide the placeholders for the button images */
    //    SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
    //}
    //else

    SetWindowLongPtr(hwndToolbar, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_SubclassOld = (WNDPROC) SetWindowLongPtr(hwndToolbar, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CMenuToolbarBase::s_SubclassProc));

    UpdateImageLists();

    return S_OK;
}

HRESULT CMenuToolbarBase::GetIdealSize(SIZE& size)
{
    size.cx = size.cy = 0;

    if (m_hwndToolbar && !m_hasIdealSize)
    {
        SendMessageW(m_hwndToolbar, TB_AUTOSIZE, 0, 0);
        SendMessageW(m_hwndToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&m_idealSize));
        m_hasIdealSize = TRUE;
    }

    size = m_idealSize;

    return S_OK;
}

HRESULT CMenuToolbarBase::SetPosSize(int x, int y, int cx, int cy)
{
    if (m_hwnd != m_hwndToolbar)
    {
        SetWindowPos(m_hwndToolbar, NULL, x, y, cx, m_idealSize.cy, 0);
    }
    SetWindowPos(m_hwnd, NULL, x, y, cx, cy, 0);
    DWORD btnSize = SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0);
    SendMessage(m_hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, HIWORD(btnSize)));
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
            KillTimer(hWnd, TIMERID_HOTTRACK);

            m_menuBand->_OnPopupSubMenu(NULL, NULL, NULL, NULL, -1);

            if (HasSubMenu(m_hotItem) == S_OK)
            {
                PopupItem(m_hotItem);
            }
        }
    }

    return m_SubclassOld(hWnd, uMsg, wParam, lParam);
}

HRESULT CMenuToolbarBase::OnHotItemChange(const NMTBHOTITEM * hot)
{
    if (hot->dwFlags & HICF_LEAVING)
    {
        KillTimer(m_hwndToolbar, TIMERID_HOTTRACK);
        m_hotItem = -1;
        m_menuBand->_OnHotItemChanged(NULL, -1);
        m_menuBand->_MenuItemHotTrack(MPOS_CHILDTRACKING);
    }
    else if (m_hotItem != hot->idNew)
    {
        DWORD elapsed = 0;
        SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &elapsed, 0);
        SetTimer(m_hwndToolbar, TIMERID_HOTTRACK, elapsed, NULL);

        m_hotItem = hot->idNew;
        m_menuBand->_OnHotItemChanged(this, m_hotItem);
        m_menuBand->_MenuItemHotTrack(MPOS_CHILDTRACKING);
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::OnHotItemChanged(CMenuToolbarBase * toolbar, INT item)
{
    m_hotBar = toolbar;
    m_hotItem = item;
    InvalidateDraw();
    return S_OK;
}

HRESULT CMenuToolbarBase::OnPopupItemChanged(CMenuToolbarBase * toolbar, INT item)
{
    m_popupBar = toolbar;
    m_popupItem = item;
    InvalidateDraw();
    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT uItem, UINT index, IShellMenu* childShellMenu)
{
    IBandSite* pBandSite;
    IDeskBar* pDeskBar;

    HRESULT hr = 0;
    RECT rc = { 0 };

    if (!SendMessage(m_hwndToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwndToolbar, &a);
    ClientToScreen(m_hwndToolbar, &b);

    POINTL pt = { b.x - 3, a.y - 3 };
    RECTL rcl = { a.x, a.y, b.x, b.y }; // maybe-TODO: fetch client area of deskbar?

#if USE_SYSTEM_MENUSITE
    hr = CoCreateInstance(CLSID_MenuBandSite,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IBandSite, &pBandSite));
#else
    hr = CMenuSite_Constructor(IID_PPV_ARG(IBandSite, &pBandSite));
#endif
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#if WRAP_MENUSITE
    hr = CMenuSite_Wrapper(pBandSite, IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED_UNEXPECTEDLY(hr))
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
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#if WRAP_MENUDESKBAR
    hr = CMenuDeskBar_Wrapper(pDeskBar, IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#endif

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBandSite->AddBand(childShellMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IMenuPopup> popup;
    hr = pDeskBar->QueryInterface(IID_PPV_ARG(IMenuPopup, &popup));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_menuBand->_OnPopupSubMenu(popup, &pt, &rcl, this, m_popupItem);

    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT index, HMENU menu)
{
    RECT rc = { 0 };

    if (!SendMessage(m_hwndToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwndToolbar, &b);

    HMENU popup = GetSubMenu(menu, index);

    m_menuBand->_TrackSubMenuUsingTrackPopupMenu(popup, b.x, b.y);

    return S_OK;
}

HRESULT CMenuToolbarBase::DoContextMenu(IContextMenu* contextMenu)
{
    HRESULT hr;
    HMENU hPopup = CreatePopupMenu();

    if (hPopup == NULL)
        return E_FAIL;

    hr = contextMenu->QueryContextMenu(hPopup, 0, 0, UINT_MAX, CMF_NORMAL);
    if (FAILED_UNEXPECTEDLY(hr))
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
    theResult = 0;
    if (HasSubMenu(wParam) == S_OK)
    {
        KillTimer(m_hwndToolbar, TIMERID_HOTTRACK);
        PopupItem(wParam);
        return S_FALSE;
    }
    return m_menuBand->_MenuItemHotTrack(MPOS_EXECUTE);
}

HRESULT CMenuToolbarBase::ChangeHotItem(DWORD dwSelectType)
{
    int prev = m_hotItem;
    int index = -1;

    if (dwSelectType != 0xFFFFFFFF)
    {
        int count = SendMessage(m_hwndToolbar, TB_BUTTONCOUNT, 0, 0);

        if (m_hotItem >= 0)
        {
            TBBUTTONINFO info = { 0 };
            info.cbSize = sizeof(TBBUTTONINFO);
            info.dwMask = 0;
            index = SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, m_hotItem, reinterpret_cast<LPARAM>(&info));
        }

        if (dwSelectType == VK_HOME)
        {
            index = 0;
            dwSelectType = VK_DOWN;
        }
        else if (dwSelectType == VK_END)
        {
            index = count - 1;
            dwSelectType = VK_UP;
        }
        else if (index < 0)
        {
            if (dwSelectType == VK_UP)
            {
                index = count - 1;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index = 0;
            }
        }
        else
        {
            if (dwSelectType == VK_UP)
            {
                index--;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index++;
            }
        }

        TBBUTTON btn = { 0 };
        while (index >= 0 && index < count)
        {
            DWORD res = SendMessage(m_hwndToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn));
            if (!res)
                return E_FAIL;

            if (btn.dwData)
            {
                m_hotItem = btn.idCommand;
                if (prev != m_hotItem)
                {
                    SendMessage(m_hwndToolbar, TB_SETHOTITEM, index, 0);
                    return m_menuBand->_OnHotItemChanged(this, m_hotItem);
                }
                return S_OK;
            }

            if (dwSelectType == VK_UP)
            {
                index--;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index++;
            }
        }
    }

    m_hotItem = -1;
    if (prev != m_hotItem)
    {
        SendMessage(m_hwndToolbar, TB_SETHOTITEM, -1, 0);
        m_menuBand->_OnHotItemChanged(NULL, -1);
    }
    return S_FALSE;
}

HRESULT CMenuToolbarBase::AddButton(DWORD commandId, LPCWSTR caption, BOOL hasSubMenu, INT iconId, DWORD_PTR buttonData, BOOL last)
{
    TBBUTTON tbb = { 0 };

    tbb.fsState = TBSTATE_ENABLED;
#ifndef TBSTYLE_EX_VERTICAL
    if (!last)
        tbb.fsState |= TBSTATE_WRAP;
#endif
    tbb.fsStyle = 0;

    if (hasSubMenu)
        tbb.fsStyle |= BTNS_DROPDOWN;

    tbb.iString = (INT_PTR) caption;
    tbb.idCommand = commandId;

    tbb.iBitmap = iconId;
    tbb.dwData = buttonData;

    DbgPrint("Trying to add a new button with id %d and caption '%S'...\n", commandId, caption);

    if (!SendMessageW(m_hwndToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::AddSeparator(BOOL last)
{
    TBBUTTON tbb = { 0 };

    tbb.fsState = TBSTATE_ENABLED;
#ifndef TBSTYLE_EX_VERTICAL
    if (!last)
        tbb.fsState |= TBSTATE_WRAP;
#endif
    tbb.fsStyle = BTNS_SEP;
    tbb.iBitmap = 0;

    DbgPrint("Trying to add a new separator...\n");

    if (!SendMessageW(m_hwndToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::AddPlaceholder()
{
    TBBUTTON tbb = { 0 };
    PCWSTR MenuString = L"(Empty)";

    tbb.fsState = 0;
    tbb.fsStyle = 0;
    tbb.iString = (INT_PTR) MenuString;
    tbb.iBitmap = -1;

    DbgPrint("Trying to add a new placeholder...\n");

    if (!SendMessageW(m_hwndToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::GetDataFromId(INT uItem, INT* pIndex, DWORD_PTR* pData)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;

    if (pIndex)
        *pIndex = index;

    if (pData)
    {
        TBBUTTON btn = { 0 };
        if (!SendMessage(m_hwndToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn)))
            return E_FAIL;
        *pData = btn.dwData;
    }

    return S_OK;
}


HRESULT CMenuToolbarBase::PopupItem(INT uItem)
{
    INT index;
    DWORD_PTR dwData;

    GetDataFromId(uItem, &index, &dwData);

    return InternalPopupItem(uItem, index, dwData);
}

HRESULT CMenuToolbarBase::HasSubMenu(INT uItem)
{
    INT index;
    DWORD_PTR dwData;

    GetDataFromId(uItem, &index, &dwData);

    return InternalHasSubMenu(uItem, index, dwData);
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand) :
    CMenuToolbarBase(menuBand, FALSE),
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

    int count = 0;
    for (i = 0; i < ic; i++)
    {
        BOOL last = i + 1 == ic;

        MENUITEMINFOW info;

        info.cbSize = sizeof(info);
        info.dwTypeData = NULL;
        info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;

        if (!GetMenuItemInfoW(m_hmenu, i, TRUE, &info))
        {
            DbgPrint("Error obtaining info for menu item at pos=%d\n", i);
            continue;
        }

        count++;

        DbgPrint("Found item with fType=%x, cmdId=%d\n", info.fType, info.wID);

        if (info.fType & MFT_SEPARATOR)
        {
            AddSeparator(last);
        }
        else if (!(info.fType & MFT_BITMAP))
        {

            info.cch++;
            info.dwTypeData = (PWSTR) HeapAlloc(GetProcessHeap(), 0, (info.cch + 1) * sizeof(WCHAR));

            info.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID;
            GetMenuItemInfoW(m_hmenu, i, TRUE, &info);

            SMINFO * sminfo = new SMINFO();
            sminfo->dwMask = SMIM_ICON | SMIM_FLAGS;
            // FIXME: remove before deleting the toolbar or it will leak

            HRESULT hr = m_menuBand->_CallCBWithItemId(info.wID, SMC_GETINFO, 0, reinterpret_cast<LPARAM>(sminfo));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            AddButton(info.wID, info.dwTypeData, info.hSubMenu != NULL, sminfo->iIcon, reinterpret_cast<DWORD_PTR>(sminfo), last);

            HeapFree(GetProcessHeap(), 0, info.dwTypeData);
        }
    }

    DbgPrint("Created toolbar with %d buttons.\n", count);

    return S_OK;
}

HRESULT CMenuStaticToolbar::OnContextMenu(NMMOUSE * rclick)
{
    CComPtr<IContextMenu> contextMenu;
    HRESULT hr = m_menuBand->_CallCBWithItemId(rclick->dwItemSpec, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IContextMenu), reinterpret_cast<LPARAM>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuStaticToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr;
    hr = CMenuToolbarBase::OnCommand(wParam, lParam, theResult);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // in case the clicked item has a submenu, we do not need to execute the item
    if (hr == S_FALSE)
        return hr;

    return m_menuBand->_CallCBWithItemId(wParam, SMC_EXEC, 0, 0);
}

HRESULT CMenuStaticToolbar::InternalPopupItem(INT uItem, INT index, DWORD_PTR dwData)
{
    SMINFO * nfo = reinterpret_cast<SMINFO*>(dwData);
    if (!nfo)
        return E_FAIL;

    if (nfo->dwFlags&SMIF_TRACKPOPUP)
    {
        return PopupSubMenu(index, m_hmenu);
    }
    else
    {
        CComPtr<IShellMenu> shellMenu;
        HRESULT hr = m_menuBand->_CallCBWithItemId(uItem, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IShellMenu), reinterpret_cast<LPARAM>(&shellMenu));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return PopupSubMenu(uItem, index, shellMenu);
    }
}

HRESULT CMenuStaticToolbar::InternalHasSubMenu(INT uItem, INT index, DWORD_PTR dwData)
{
    return ::GetSubMenu(m_hmenu, index) ? S_OK : S_FALSE;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand * menuBand) :
    CMenuToolbarBase(menuBand, TRUE),
    m_shellFolder(NULL)
{
}

CMenuSFToolbar::~CMenuSFToolbar()
{
}

HRESULT CMenuSFToolbar::FillToolbar()
{
    HRESULT hr;
    int i = 0;
    PWSTR MenuString;

    IEnumIDList * eidl;
    m_shellFolder->EnumObjects(m_hwndToolbar, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);

    LPITEMIDLIST item = static_cast<LPITEMIDLIST>(CoTaskMemAlloc(sizeof(ITEMIDLIST)));
    ULONG fetched;
    hr = eidl->Next(1, &item, &fetched);
    while (SUCCEEDED(hr) && fetched > 0)
    {
        INT index = 0;
        INT indexOpen = 0;

        STRRET sr = { STRRET_CSTR, { 0 } };

        hr = m_shellFolder->GetDisplayNameOf(item, SIGDN_NORMALDISPLAY, &sr);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        StrRetToStr(&sr, NULL, &MenuString);

        index = SHMapPIDLToSystemImageListIndex(m_shellFolder, item, &indexOpen);

        LPCITEMIDLIST itemc = item;

        SFGAOF attrs = SFGAO_FOLDER;
        hr = m_shellFolder->GetAttributesOf(1, &itemc, &attrs);

        DWORD_PTR dwData = reinterpret_cast<DWORD_PTR>(ILClone(item));
        // FIXME: remove before deleting the toolbar or it will leak

        // Fetch next item already, so we know if the current one is the last
        hr = eidl->Next(1, &item, &fetched);

        AddButton(++i, MenuString, attrs & SFGAO_FOLDER, index, dwData, FAILED(hr) || fetched == 0);

        CoTaskMemFree(MenuString);
    }
    CoTaskMemFree(item);

    // If no items were added, show the "empty" placeholder
    if (i == 0)
    {
        DbgPrint("The toolbar is empty, adding placeholder.\n");

        return AddPlaceholder();
    }

    DbgPrint("Created toolbar with %d buttons.\n", i);

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
    if (FAILED_UNEXPECTEDLY(hr))
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

HRESULT CMenuSFToolbar::OnContextMenu(NMMOUSE * rclick)
{
    HRESULT hr;
    CComPtr<IContextMenu> contextMenu;
    LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(rclick->dwItemData);

    hr = m_shellFolder->GetUIObjectOf(m_hwndToolbar, 1, &pidl, IID_IContextMenu, NULL, reinterpret_cast<VOID **>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuSFToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr;
    hr = CMenuToolbarBase::OnCommand(wParam, lParam, theResult);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // in case the clicked item has a submenu, we do not need to execute the item
    if (hr == S_FALSE)
        return hr;

    DWORD_PTR data;
    GetDataFromId(wParam, NULL, &data);

    return m_menuBand->_CallCBWithItemPidl(reinterpret_cast<LPITEMIDLIST>(data), SMC_SFEXEC, 0, 0);
}

HRESULT CMenuSFToolbar::InternalPopupItem(INT uItem, INT index, DWORD_PTR dwData)
{
    HRESULT hr;
    UINT uId;
    UINT uIdAncestor;
    DWORD flags;
    CComPtr<IShellMenuCallback> psmc;
    CComPtr<IShellMenu> shellMenu;

    LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(dwData);

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
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#if WRAP_MENUBAND
    hr = CMenuBand_Wrapper(shellMenu, IID_PPV_ARG(IShellMenu, &shellMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#endif

    m_menuBand->GetMenuInfo(&psmc, &uId, &uIdAncestor, &flags);

    // FIXME: not sure what to use as uId/uIdAncestor here
    hr = shellMenu->Initialize(psmc, 0, uId, SMINIT_VERTICAL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder> childFolder;
    hr = m_shellFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &childFolder));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = shellMenu->SetShellFolder(childFolder, NULL, NULL, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return PopupSubMenu(uItem, index, shellMenu);
}

HRESULT CMenuSFToolbar::InternalHasSubMenu(INT uItem, INT index, DWORD_PTR dwData)
{
    HRESULT hr;
    LPCITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(dwData);

    SFGAOF attrs = SFGAO_FOLDER;
    hr = m_shellFolder->GetAttributesOf(1, &pidl, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return (attrs & SFGAO_FOLDER) ? S_OK : S_FALSE;
}
