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
#include <commoncontrols.h>
#include <shlwapi_undoc.h>
#include <uxtheme.h>

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
#define USE_TBSTYLE_EX_VERTICAL 0

// User-defined timer ID used while hot-tracking around the menu
#define TIMERID_HOTTRACK 1

HRESULT CMenuToolbarBase::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    NMHDR * hdr;

    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:
        return OnCommand(wParam, lParam, theResult);

    case WM_NOTIFY:
        hdr = reinterpret_cast<LPNMHDR>(lParam);
        switch (hdr->code)
        {
        case TBN_DELETINGBUTTON:
            return OnDeletingButton(reinterpret_cast<LPNMTOOLBAR>(hdr));

        case PGN_CALCSIZE:
            return OnPagerCalcSize(reinterpret_cast<LPNMPGCALCSIZE>(hdr));

        case TBN_DROPDOWN:
            return OnCommand(reinterpret_cast<LPNMTOOLBAR>(hdr)->iItem, 0, theResult);

        case TBN_HOTITEMCHANGE:
            //return OnHotItemChange(reinterpret_cast<LPNMTBHOTITEM>(hdr), theResult);
            return S_OK;

        case NM_RCLICK:
            return OnContextMenu(reinterpret_cast<LPNMMOUSE>(hdr));

        case NM_CUSTOMDRAW:
            return OnCustomDraw(reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr), theResult);

        case TBN_GETINFOTIP:
            return OnGetInfoTip(reinterpret_cast<LPNMTBGETINFOTIP>(hdr));

            // Silence unhandled items so that they don't print as unknown
        case RBN_CHILDSIZE:
            return S_OK;

        case TTN_GETDISPINFO:
            return S_OK;

        case NM_RELEASEDCAPTURE:
            break;

        case NM_CLICK:
        case NM_RDOWN:
        case NM_LDOWN:
            break;

        case TBN_GETDISPINFO:
            break;

        case TBN_BEGINDRAG:
        case TBN_ENDDRAG:
            break;

        case NM_TOOLTIPSCREATED:
            break;

            // Unknown
        case -714: return S_FALSE;

        default:
            TRACE("WM_NOTIFY unknown code %d, %d\n", hdr->code, hdr->idFrom);
            return S_OK;
        }
        return S_FALSE;
    }

    return S_FALSE;
}

LRESULT CALLBACK CMenuToolbarBase::s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMenuToolbarBase * pthis = reinterpret_cast<CMenuToolbarBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return pthis->SubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CMenuToolbarBase::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr;

    switch (uMsg)
    {
    case WM_USER_ISTRACKEDITEM:
        m_SubclassOld(hWnd, uMsg, wParam, lParam);
        return IsTrackedItem(wParam);
    case WM_USER_CHANGETRACKEDITEM:
        m_SubclassOld(hWnd, uMsg, wParam, lParam);
        return ChangeTrackedItem(wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_COMMAND:
        OnWinEvent(hWnd, uMsg, wParam, lParam, &lr);
        break;
    case WM_NOTIFY:
        OnWinEvent(hWnd, uMsg, wParam, lParam, &lr);
        break;
    case WM_TIMER:
        OnPopupTimer(wParam);
    }

    return m_SubclassOld(hWnd, uMsg, wParam, lParam);
}

HRESULT CMenuToolbarBase::DisableMouseTrack(BOOL bDisable)
{
    if (m_disableMouseTrack != bDisable)
    {
        m_disableMouseTrack = bDisable;
        TRACE("DisableMouseTrack %d\n", bDisable);
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::OnPagerCalcSize(LPNMPGCALCSIZE csize)
{
    SIZE tbs;
    GetSizes(NULL, &tbs, NULL);
    if (csize->dwFlag == PGF_CALCHEIGHT)
    {
        csize->iHeight = tbs.cy;
    }
    else if (csize->dwFlag == PGF_CALCWIDTH)
    {
        csize->iWidth = tbs.cx;
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::OnCustomDraw(LPNMTBCUSTOMDRAW cdraw, LRESULT * theResult)
{
    RECT     rc;
    HDC      hdc;
    COLORREF clrText;
    HBRUSH   bgBrush;
    bool     isHot, isPopup;
    TBBUTTONINFO btni;

    switch (cdraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *theResult = CDRF_NOTIFYITEMDRAW;
        return S_OK;

    case CDDS_ITEMPREPAINT:

        rc = cdraw->nmcd.rc;
        hdc = cdraw->nmcd.hdc;

        // The item with an active submenu gets the CHECKED flag.
        isHot = m_hotBar == this && (int) cdraw->nmcd.dwItemSpec == m_hotItem;
        isPopup = m_popupBar == this && (int) cdraw->nmcd.dwItemSpec == m_popupItem;

        if (m_initFlags & SMINIT_VERTICAL || IsAppThemed())
        {
            // Remove HOT and CHECKED flags (will restore HOT if necessary)
            cdraw->nmcd.uItemState &= ~(CDIS_HOT | CDIS_CHECKED);

            // Decide on the colors
            if (isHot || (m_hotItem < 0 && isPopup))
            {
                cdraw->nmcd.uItemState |= CDIS_HOT;

                clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                bgBrush = GetSysColorBrush(m_useFlatMenus ? COLOR_MENUHILIGHT : COLOR_HIGHLIGHT);
            }
            else
            {
                clrText = GetSysColor(COLOR_MENUTEXT);
                bgBrush = GetSysColorBrush(COLOR_MENU);
            }

            // Paint the background color with the selected color
            FillRect(hdc, &rc, bgBrush);

            // Set the text color in advance, this color will be assigned when the ITEMPOSTPAINT triggers
            SetTextColor(hdc, clrText);

            // Set the text color, will be used by the internal drawing code
            cdraw->clrText = clrText;
            cdraw->iListGap += 4;

            // Tell the default drawing code we don't want any fanciness, not even a background.
            *theResult = CDRF_NOTIFYPOSTPAINT | TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK | 0x00800000; // FIXME: the last bit is Vista+, useful for debugging only
        }
        else
        {
            // Remove HOT and CHECKED flags (will restore HOT if necessary)
            cdraw->nmcd.uItemState &= ~CDIS_HOT;

            // Decide on the colors
            if (isHot || (m_hotItem < 0 && isPopup))
            {
                cdraw->nmcd.uItemState |= CDIS_HOT;
            }

            *theResult = 0;
        }

        return S_OK;

    case CDDS_ITEMPOSTPAINT:

        // Fetch the button style
        btni.cbSize = sizeof(btni);
        btni.dwMask = TBIF_STYLE;
        SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, cdraw->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&btni));

        // Check if we need to draw a submenu arrow
        if (btni.fsStyle & BTNS_DROPDOWN)
        {
            // TODO: Support RTL text modes by drawing a leftwards arrow aligned to the left of the control

            // "8" is the rightwards dropdown arrow in the Marlett font
            WCHAR text [] = L"8";

            // Configure the font to draw with Marlett, keeping the current background color as-is
            SelectObject(cdraw->nmcd.hdc, m_marlett);
            SetBkMode(cdraw->nmcd.hdc, TRANSPARENT);

            // Tweak the alignment by 1 pixel so the menu draws like the Windows start menu.
            RECT rc = cdraw->nmcd.rc;
            rc.right += 1;

            // The arrow is drawn at the right of the item's rect, aligned vertically.
            DrawTextEx(cdraw->nmcd.hdc, text, 1, &rc, DT_NOCLIP | DT_VCENTER | DT_RIGHT | DT_SINGLELINE, NULL);
        }
        *theResult = TRUE;
        return S_OK;
    }
    return S_OK;
}

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager) :
    m_hwnd(NULL),
    m_hwndToolbar(NULL),
    m_useFlatMenus(FALSE),
    m_SubclassOld(NULL),
    m_disableMouseTrack(FALSE),
    m_timerEnabled(FALSE),
    m_menuBand(menuBand),
    m_dwMenuFlags(0),
    m_hasSizes(FALSE),
    m_usePager(usePager),
    m_hotItem(-1),
    m_popupItem(-1),
    m_isTrackingPopup(FALSE)
{
    m_idealSize.cx = 0;
    m_idealSize.cy = 0;
    m_itemSize.cx = 0;
    m_itemSize.cy = 0;
    m_marlett = CreateFont(
        0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FF_DONTCARE, L"Marlett");
}

CMenuToolbarBase::~CMenuToolbarBase()
{
    if (m_hwndToolbar && m_hwndToolbar != m_hwnd)
        DestroyWindow(m_hwndToolbar);

    if (m_hwnd)
        DestroyWindow(m_hwnd);

    DeleteObject(m_marlett);
}

void CMenuToolbarBase::InvalidateDraw()
{
    InvalidateRect(m_hwnd, NULL, FALSE);
}

HRESULT CMenuToolbarBase::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    // Ensure that the right image list is assigned to the toolbar
    UpdateImageLists();

    // For custom-drawing
    SystemParametersInfo(SPI_GETFLATMENU, 0, &m_useFlatMenus, 0);

    return S_OK;
}

HRESULT CMenuToolbarBase::UpdateImageLists()
{
    if ((m_initFlags & (SMINIT_TOPLEVEL | SMINIT_VERTICAL)) == SMINIT_TOPLEVEL) // not vertical.
    {
        // No image list, prevents the buttons from having a margin at the left side
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
        return S_OK;
    }

    // Assign the correct imagelist and padding based on the current icon size

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
    if (FAILED_UNEXPECTEDLY(hr))
    {
        SendMessageW(m_hwndToolbar, TB_SETIMAGELIST, 0, 0);
    }
    else
    {
        SendMessageW(m_hwndToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    if (m_hwndToolbar != m_hwnd)
        DestroyWindow(m_hwndToolbar);

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
    LONG tbExStyles = TBSTYLE_EX_DOUBLEBUFFER | WS_EX_TOOLWINDOW;

    if (dwFlags & SMINIT_VERTICAL)
    {
        // Activate vertical semantics
        tbStyles |= CCS_VERT;

#if USE_TBSTYLE_EX_VERTICAL
        tbExStyles |= TBSTYLE_EX_VERTICAL;
#endif
    }

    m_initFlags = dwFlags;

    // Get a temporary rect to use while creating the toolbar window.
    // Ensure that it is not a null rect.
    RECT rc;
    if (!::GetClientRect(hwndParent, &rc) ||
        (rc.left == rc.right) ||
        (rc.top == rc.bottom))
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

    // If needed, create the pager.
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

    // Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure.
    SendMessageW(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Apply subclassing
    SetWindowLongPtr(hwndToolbar, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_SubclassOld = (WNDPROC) SetWindowLongPtr(hwndToolbar, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CMenuToolbarBase::s_SubclassProc));

    // Configure the image lists
    UpdateImageLists();

    return S_OK;
}

HRESULT CMenuToolbarBase::GetSizes(SIZE* pMinSize, SIZE* pMaxSize, SIZE* pIntegralSize)
{
    if (pMinSize)
        *pMinSize = m_idealSize;
    if (pMaxSize)
        *pMaxSize = m_idealSize;
    if (pIntegralSize)
        *pIntegralSize = m_itemSize;

    if (m_hasSizes)
        return S_OK;

    if (!m_hwndToolbar)
        return S_OK;

    // Obtain the ideal size, to be used as min and max
    SendMessageW(m_hwndToolbar, TB_AUTOSIZE, 0, 0);
    SendMessageW(m_hwndToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&m_idealSize));
    SendMessageW(m_hwndToolbar, TB_GETIDEALSIZE, (m_initFlags & SMINIT_VERTICAL) != 0, reinterpret_cast<LPARAM>(&m_idealSize));

    // Obtain the button size, to be used as the integral size
    DWORD size = SendMessageW(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0);
    m_itemSize.cx = GET_X_LPARAM(size);
    m_itemSize.cy = GET_Y_LPARAM(size);
    m_hasSizes = TRUE;

    if (pMinSize)
        *pMinSize = m_idealSize;
    if (pMaxSize)
        *pMaxSize = m_idealSize;
    if (pIntegralSize)
        *pIntegralSize = m_itemSize;

    return S_OK;
}

HRESULT CMenuToolbarBase::SetPosSize(int x, int y, int cx, int cy)
{
    // If we have a pager, set the toolbar height to the ideal height of the toolbar
    if (m_hwnd != m_hwndToolbar)
    {
        SetWindowPos(m_hwndToolbar, NULL, x, y, cx, m_idealSize.cy, 0);
    }

    // Update the toolbar or pager to fit the requested rect
    SetWindowPos(m_hwnd, NULL, x, y, cx, cy, 0);

    // In a vertical menu, resize the buttons to fit the width
    if (m_initFlags & SMINIT_VERTICAL)
    {
        DWORD btnSize = SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(m_hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, HIWORD(btnSize)));
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::IsWindowOwner(HWND hwnd)
{
    if (m_hwnd && m_hwnd == hwnd) return S_OK;
    if (m_hwndToolbar && m_hwndToolbar == hwnd) return S_OK;
    return S_FALSE;
}

HRESULT CMenuToolbarBase::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    *phwnd = m_hwnd;

    return S_OK;
}

HRESULT CMenuToolbarBase::OnGetInfoTip(NMTBGETINFOTIP * tip)
{
    INT index;
    DWORD_PTR dwData;

    INT iItem = tip->iItem;

    GetDataFromId(iItem, &index, &dwData);

    return InternalGetTooltip(iItem, index, dwData, tip->pszText, tip->cchTextMax);
}

HRESULT CMenuToolbarBase::OnPopupTimer(DWORD timerId)
{
    if (timerId != TIMERID_HOTTRACK)
        return S_FALSE;

    KillTimer(m_hwndToolbar, TIMERID_HOTTRACK);

    if (!m_timerEnabled)
        return S_FALSE;

    m_timerEnabled = FALSE;

    if (m_hotItem < 0)
        return S_FALSE;

    // Returns S_FALSE if the current item did not show a submenu
    HRESULT hr = PopupItem(m_hotItem, FALSE);
    if (hr != S_FALSE)
        return hr;

    // If we didn't switch submenus, cancel the current popup regardless
    if (m_popupBar)
    {
        HRESULT hr = CancelCurrentPopup();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::KillPopupTimer()
{
    if (m_timerEnabled)
    {
        m_timerEnabled = FALSE;
        KillTimer(m_hwndToolbar, TIMERID_HOTTRACK);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT CMenuToolbarBase::ChangeHotItem(CMenuToolbarBase * toolbar, INT item, DWORD dwFlags)
{
    // Ignore the change if it already matches the stored info
    if (m_hotBar == toolbar && m_hotItem == item)
        return S_FALSE;

    // Prevent a change of hot item if the change was triggered by the mouse,
    // and mouse tracking is disabled.
    if (m_disableMouseTrack && dwFlags & HICF_MOUSE)
    {
        TRACE("Hot item change prevented by DisableMouseTrack\n");
        return S_OK;
    }

    // Notify the toolbar if the hot-tracking left this toolbar
    if (m_hotBar == this && toolbar != this)
    {
        SendMessage(m_hwndToolbar, TB_SETHOTITEM, (WPARAM) -1, 0);
    }

    TRACE("Hot item changed from %p %p, to %p %p\n", m_hotBar, m_hotItem, toolbar, item);
    m_hotBar = toolbar;
    m_hotItem = item;

    if (m_hotBar == this)
    {
        if (m_isTrackingPopup && !(m_initFlags & SMINIT_VERTICAL))
        {
            // If the menubar has an open submenu, switch to the new item's submenu immediately
            PopupItem(m_hotItem, FALSE);
        }
        else if (dwFlags & HICF_MOUSE)
        {
            // Vertical menus show/hide the submenu after a delay,
            // but only with the mouse.
            if (m_initFlags & SMINIT_VERTICAL)
            {
                DWORD elapsed = 0;
                SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &elapsed, 0);
                SetTimer(m_hwndToolbar, TIMERID_HOTTRACK, elapsed, NULL);
                m_timerEnabled = TRUE;
                TRACE("SetTimer called with m_hotItem=%d\n", m_hotItem);
            }
        }
        else
        {
            TBBUTTONINFO info;
            info.cbSize = sizeof(info);
            info.dwMask = 0;

            int index = SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, item, reinterpret_cast<LPARAM>(&info));

            SendMessage(m_hwndToolbar, TB_SETHOTITEM, index, 0);
        }
    }

    InvalidateDraw();
    return S_OK;
}

HRESULT CMenuToolbarBase::ChangePopupItem(CMenuToolbarBase * toolbar, INT item)
{
    // Ignore the change if it already matches the stored info
    if (m_popupBar == toolbar && m_popupItem == item)
        return S_FALSE;

    // Notify the toolbar if the popup-tracking this toolbar
    if (m_popupBar == this && toolbar != this)
    {
        SendMessage(m_hwndToolbar, TB_CHECKBUTTON, m_popupItem, FALSE);
        m_isTrackingPopup = FALSE;
    }

    m_popupBar = toolbar;
    m_popupItem = item;

    if (m_popupBar == this)
    {
        SendMessage(m_hwndToolbar, TB_CHECKBUTTON, m_popupItem, TRUE);
    }

    InvalidateDraw();
    return S_OK;
}

HRESULT CMenuToolbarBase::IsTrackedItem(INT index)
{
    TBBUTTON btn;

    if (m_hotBar != this)
        return S_FALSE;

    if (index < 0)
        return S_FALSE;

    if (!SendMessage(m_hwndToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn)))
        return E_FAIL;

    if (m_hotItem == btn.idCommand)
        return S_OK;

    if (m_popupItem == btn.idCommand)
        return S_OK;

    return S_FALSE;
}

HRESULT CMenuToolbarBase::ChangeTrackedItem(INT index, BOOL wasTracking, BOOL mouse)
{
    TBBUTTON btn;

    if (index < 0)
    {
        m_isTrackingPopup = FALSE;
        return m_menuBand->_ChangeHotItem(NULL, -1, HICF_MOUSE);
    }

    if (!SendMessage(m_hwndToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn)))
        return E_FAIL;

    TRACE("ChangeTrackedItem %d, %d\n", index, wasTracking);
    m_isTrackingPopup = wasTracking;
    return m_menuBand->_ChangeHotItem(this, btn.idCommand, mouse ? HICF_MOUSE : 0);
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT iItem, UINT index, IShellMenu* childShellMenu, BOOL keyInitiated)
{
    // Calculate the submenu position and exclude area
    RECT rc = { 0 };
    RECT rcx = { 0 };

    if (!SendMessage(m_hwndToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    GetWindowRect(m_hwnd, &rcx);

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };
    POINT c = { rcx.left, rcx.top };
    POINT d = { rcx.right, rcx.bottom };

    ClientToScreen(m_hwndToolbar, &a);
    ClientToScreen(m_hwndToolbar, &b);
    ClientToScreen(m_hwnd, &c);
    ClientToScreen(m_hwnd, &d);

    POINTL pt = { a.x, b.y };
    RECTL rcl = { c.x, c.y, d.x, d.y };

    if (m_initFlags & SMINIT_VERTICAL)
    {
        pt.x = b.x - 3;
        pt.y = a.y - 3;
    }

    // Display the submenu
    m_isTrackingPopup = TRUE;

    m_menuBand->_ChangePopupItem(this, iItem);
    m_menuBand->_OnPopupSubMenu(childShellMenu, &pt, &rcl, keyInitiated);

    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT iItem, UINT index, HMENU menu)
{
    // Calculate the submenu position and exclude area
    RECT rc = { 0 };

    if (!SendMessage(m_hwndToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwndToolbar, &a);
    ClientToScreen(m_hwndToolbar, &b);

    POINT pt = { a.x, b.y };
    RECT rcl = { a.x, a.y, b.x, b.y };

    if (m_initFlags & SMINIT_VERTICAL)
    {
        pt.x = b.x;
        pt.y = a.y;
    }

    HMENU popup = GetSubMenu(menu, index);

    // Display the submenu
    m_isTrackingPopup = TRUE;
    m_menuBand->_ChangePopupItem(this, iItem);
    m_menuBand->_TrackSubMenu(popup, pt.x, pt.y, rcl);
    m_menuBand->_ChangePopupItem(NULL, -1);
    m_isTrackingPopup = FALSE;

    m_menuBand->_ChangeHotItem(NULL, -1, 0);

    return S_OK;
}

HRESULT CMenuToolbarBase::TrackContextMenu(IContextMenu* contextMenu, POINT pt)
{
    // Cancel submenus
    m_menuBand->_KillPopupTimers();
    if (m_popupBar)
        m_menuBand->_CancelCurrentPopup();

    // Display the context menu
    return m_menuBand->_TrackContextMenu(contextMenu, pt.x, pt.y);
}

HRESULT CMenuToolbarBase::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    if (m_disableMouseTrack)
    {
        *theResult = 1;
        TRACE("Item click prevented by DisableMouseTrack\n");
        return S_OK;
    }

    // If a button is clicked while a submenu was open, cancel the submenu.
    if (!(m_initFlags & SMINIT_VERTICAL) && m_isTrackingPopup)
    {
        TRACE("OnCommand cancelled because it was tracking submenu.\n");
        return S_FALSE;
    }

    *theResult = 0;

    INT iItem = (INT)wParam;

    if (PopupItem(iItem, FALSE) == S_OK)
    {
        TRACE("PopupItem returned S_OK\n");
        return S_FALSE;
    }

    TRACE("Executing...\n");

    return m_menuBand->_MenuItemHotTrack(MPOS_EXECUTE);
}

HRESULT CMenuToolbarBase::ExecuteItem(INT iItem)
{
    this->m_menuBand->_KillPopupTimers();

    INT index;
    DWORD_PTR data;

    GetDataFromId(iItem, &index, &data);

    return InternalExecuteItem(iItem, index, data);
}

HRESULT CMenuToolbarBase::OnContextMenu(NMMOUSE * rclick)
{
    INT iItem = rclick->dwItemSpec;
    INT index = rclick->dwHitInfo;
    DWORD_PTR data = rclick->dwItemData;

    GetDataFromId(iItem, &index, &data);

    return InternalContextMenu(iItem, index, data, rclick->pt);
}

HRESULT CMenuToolbarBase::KeyboardItemChange(DWORD dwSelectType)
{
    int prev = m_hotItem;
    int index = -1;

    if (dwSelectType != 0xFFFFFFFF)
    {
        int count = SendMessage(m_hwndToolbar, TB_BUTTONCOUNT, 0, 0);

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
        else
        {
            if (m_hotItem >= 0)
            {
                TBBUTTONINFO info = { 0 };
                info.cbSize = sizeof(TBBUTTONINFO);
                info.dwMask = 0;
                index = SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, m_hotItem, reinterpret_cast<LPARAM>(&info));
            }

            if (index < 0)
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
        }

        TBBUTTON btn = { 0 };
        while (index >= 0 && index < count)
        {
            DWORD res = SendMessage(m_hwndToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn));
            if (!res)
                return E_FAIL;

            if (btn.dwData)
            {
                if (prev != btn.idCommand)
                {
                    TRACE("Setting Hot item to %d\n", index);
                    if (!(m_initFlags & SMINIT_VERTICAL) && m_isTrackingPopup)
                    {
                        HWND tlw;
                        m_menuBand->_GetTopLevelWindow(&tlw);
                        SendMessage(tlw, WM_CANCELMODE, 0, 0);
                        PostMessage(m_hwndToolbar, WM_USER_CHANGETRACKEDITEM, index, MAKELPARAM(m_isTrackingPopup, FALSE));
                    }
                    else
                        m_menuBand->_ChangeHotItem(this, btn.idCommand, 0);
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

        return S_FALSE;
    }

    if (prev != -1)
    {
        TRACE("Setting Hot item to null\n");
        m_menuBand->_ChangeHotItem(NULL, -1, 0);
    }

    return S_FALSE;
}

HRESULT CMenuToolbarBase::AddButton(DWORD commandId, LPCWSTR caption, BOOL hasSubMenu, INT iconId, DWORD_PTR buttonData, BOOL last)
{
    TBBUTTON tbb = { 0 };

    tbb.fsState = TBSTATE_ENABLED;
#if !USE_TBSTYLE_EX_VERTICAL
    if (!last && (m_initFlags & SMINIT_VERTICAL))
        tbb.fsState |= TBSTATE_WRAP;
#endif
    tbb.fsStyle = BTNS_CHECKGROUP;

    if (hasSubMenu && (m_initFlags & SMINIT_VERTICAL))
        tbb.fsStyle |= BTNS_DROPDOWN;

    if (!(m_initFlags & SMINIT_VERTICAL))
        tbb.fsStyle |= BTNS_AUTOSIZE;

    tbb.iString = (INT_PTR) caption;
    tbb.idCommand = commandId;

    tbb.iBitmap = iconId;
    tbb.dwData = buttonData;

    if (!SendMessageW(m_hwndToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::AddSeparator(BOOL last)
{
    TBBUTTON tbb = { 0 };

    tbb.fsState = TBSTATE_ENABLED;
#if !USE_TBSTYLE_EX_VERTICAL
    if (!last && (m_initFlags & SMINIT_VERTICAL))
        tbb.fsState |= TBSTATE_WRAP;
#endif
    tbb.fsStyle = BTNS_SEP;
    tbb.iBitmap = 0;

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

    if (!SendMessageW(m_hwndToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::ClearToolbar()
{
    while (SendMessage(m_hwndToolbar, TB_DELETEBUTTON, 0, 0))
    {
        // empty;
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::GetDataFromId(INT iItem, INT* pIndex, DWORD_PTR* pData)
{
    if (pData)
        *pData = NULL;

    if (pIndex)
        *pIndex = -1;

    if (iItem < 0)
        return S_OK;

    TBBUTTONINFO info = { 0 };

    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = TBIF_COMMAND | TBIF_LPARAM;

    int index = SendMessage(m_hwndToolbar, TB_GETBUTTONINFO, iItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;

    if (pIndex)
        *pIndex = index;

    if (pData)
        *pData = info.lParam;

    return S_OK;
}

HRESULT CMenuToolbarBase::CancelCurrentPopup()
{
    return m_menuBand->_CancelCurrentPopup();
}

HRESULT CMenuToolbarBase::PopupItem(INT iItem, BOOL keyInitiated)
{
    INT index;
    DWORD_PTR dwData;

    if (iItem < 0)
        return S_OK;

    if (m_popupBar == this && m_popupItem == iItem)
        return S_OK;

    GetDataFromId(iItem, &index, &dwData);

    HRESULT hr = InternalHasSubMenu(iItem, index, dwData);
    if (hr != S_OK)
        return hr;

    if (m_popupBar)
    {
        HRESULT hr = CancelCurrentPopup();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (!(m_initFlags & SMINIT_VERTICAL))
    {
        TRACE("PopupItem non-vertical %d %d\n", index, iItem);
        m_menuBand->_ChangeHotItem(this, iItem, 0);
    }

    return InternalPopupItem(iItem, index, dwData, keyInitiated);
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand) :
    CMenuToolbarBase(menuBand, FALSE),
    m_hmenu(NULL)
{
}

CMenuStaticToolbar::~CMenuStaticToolbar()
{
}

HRESULT  CMenuStaticToolbar::GetMenu(
    _Out_opt_ HMENU *phmenu,
    _Out_opt_ HWND *phwnd,
    _Out_opt_ DWORD *pdwFlags)
{
    if (phmenu)
        *phmenu = m_hmenu;
    if (phwnd)
        *phwnd = NULL;
    if (pdwFlags)
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

HRESULT CMenuStaticToolbar::FillToolbar(BOOL clearFirst)
{
    int i;
    int ic = GetMenuItemCount(m_hmenu);

    if (clearFirst)
    {
        ClearToolbar();
    }

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
            TRACE("Error obtaining info for menu item at pos=%d\n", i);
            continue;
        }

        count++;

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

    return S_OK;
}

HRESULT CMenuStaticToolbar::InternalGetTooltip(INT iItem, INT index, DWORD_PTR dwData, LPWSTR pszText, INT cchTextMax)
{
    //SMINFO * info = reinterpret_cast<SMINFO*>(dwData);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT CMenuStaticToolbar::OnDeletingButton(const NMTOOLBAR * tb)
{
    delete reinterpret_cast<SMINFO*>(tb->tbButton.dwData);
    return S_OK;
}

HRESULT CMenuStaticToolbar::InternalContextMenu(INT iItem, INT index, DWORD_PTR dwData, POINT pt)
{
    CComPtr<IContextMenu> contextMenu;
    HRESULT hr = m_menuBand->_CallCBWithItemId(iItem, SMC_GETOBJECT, 
        reinterpret_cast<WPARAM>(&IID_IContextMenu), reinterpret_cast<LPARAM>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return TrackContextMenu(contextMenu, pt);
}

HRESULT CMenuStaticToolbar::InternalExecuteItem(INT iItem, INT index, DWORD_PTR data)
{
    return m_menuBand->_CallCBWithItemId(iItem, SMC_EXEC, 0, 0);
}

HRESULT CMenuStaticToolbar::InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData, BOOL keyInitiated)
{
    SMINFO * nfo = reinterpret_cast<SMINFO*>(dwData);
    if (!nfo)
        return E_FAIL;

    if (nfo->dwFlags&SMIF_TRACKPOPUP)
    {
        return PopupSubMenu(iItem, index, m_hmenu);
    }
    else
    {
        CComPtr<IShellMenu> shellMenu;
        HRESULT hr = m_menuBand->_CallCBWithItemId(iItem, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IShellMenu), reinterpret_cast<LPARAM>(&shellMenu));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return PopupSubMenu(iItem, index, shellMenu, keyInitiated);
    }
}

HRESULT CMenuStaticToolbar::InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData)
{
    return ::GetSubMenu(m_hmenu, index) ? S_OK : S_FALSE;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand * menuBand) :
    CMenuToolbarBase(menuBand, TRUE),
    m_shellFolder(NULL),
    m_idList(NULL),
    m_hKey(NULL)
{
}

CMenuSFToolbar::~CMenuSFToolbar()
{
}

HRESULT CMenuSFToolbar::FillToolbar(BOOL clearFirst)
{
    HRESULT hr;
    int i = 0;
    PWSTR MenuString;

    IEnumIDList * eidl;
    m_shellFolder->EnumObjects(GetToolbar(), SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);

    LPITEMIDLIST item = NULL;
    hr = eidl->Next(1, &item, NULL);
    while (hr == S_OK)
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

        // Fetch next item already, so we know if the current one is the last
        hr = eidl->Next(1, &item, NULL);

        AddButton(++i, MenuString, attrs & SFGAO_FOLDER, index, dwData, hr != S_OK);

        CoTaskMemFree(MenuString);
    }
    ILFree(item);

    // If no items were added, show the "empty" placeholder
    if (i == 0)
    {
        return AddPlaceholder();
    }

    return hr;
}

HRESULT CMenuSFToolbar::InternalGetTooltip(INT iItem, INT index, DWORD_PTR dwData, LPWSTR pszText, INT cchTextMax)
{
    //ITEMIDLIST * pidl = reinterpret_cast<LPITEMIDLIST>(dwData);
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT CMenuSFToolbar::OnDeletingButton(const NMTOOLBAR * tb)
{
    ILFree(reinterpret_cast<LPITEMIDLIST>(tb->tbButton.dwData));
    return S_OK;
}

HRESULT CMenuSFToolbar::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    m_shellFolder = psf;
    m_idList = ILClone(pidlFolder);
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
                (*reinterpret_cast<IUnknown**>(ppv))->Release();
                return E_FAIL;
            }
        }

        *ppidl = pidl;
    }

    return hr;
}

HRESULT CMenuSFToolbar::InternalContextMenu(INT iItem, INT index, DWORD_PTR dwData, POINT pt)
{
    HRESULT hr;
    CComPtr<IContextMenu> contextMenu = NULL;
    LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(dwData);

    hr = m_shellFolder->GetUIObjectOf(GetToolbar(), 1, &pidl, IID_NULL_PPV_ARG(IContextMenu, &contextMenu));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        return hr;
    }

    hr = TrackContextMenu(contextMenu, pt);

    return hr;
}

HRESULT CMenuSFToolbar::InternalExecuteItem(INT iItem, INT index, DWORD_PTR data)
{
    return m_menuBand->_CallCBWithItemPidl(reinterpret_cast<LPITEMIDLIST>(data), SMC_SFEXEC, 0, 0);
}

HRESULT CMenuSFToolbar::InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData, BOOL keyInitiated)
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

    hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &shellMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

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

    return PopupSubMenu(iItem, index, shellMenu, keyInitiated);
}

HRESULT CMenuSFToolbar::InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData)
{
    HRESULT hr;
    LPCITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(dwData);

    SFGAOF attrs = SFGAO_FOLDER;
    hr = m_shellFolder->GetAttributesOf(1, &pidl, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return (attrs & SFGAO_FOLDER) ? S_OK : S_FALSE;
}
