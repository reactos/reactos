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
#include "shellmenu.h"
#include <commoncontrols.h>
#include <shlwapi_undoc.h>
#include <uxtheme.h>
#include <vssym32.h>

#include "CMenuBand.h"
#include "CMenuToolbars.h"

#define IDS_MENU_EMPTY 34561

WINE_DEFAULT_DEBUG_CHANNEL(CMenuToolbars);

// FIXME: Enable if/when wine comctl supports this flag properly
#define USE_TBSTYLE_EX_VERTICAL 0

// User-defined timer ID used while hot-tracking around the menu
#define TIMERID_HOTTRACK 1

LRESULT CMenuToolbarBase::OnWinEventWrap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr;
    bHandled = OnWinEvent(m_hWnd, uMsg, wParam, lParam, &lr) != S_FALSE;
    return lr;
}

HRESULT CMenuToolbarBase::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    NMHDR * hdr;
    HRESULT hr;
    LRESULT result;

    if (theResult)
        *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:
        //return OnCommand(wParam, lParam, theResult);
        return S_OK;

    case WM_NOTIFY:
        hdr = reinterpret_cast<LPNMHDR>(lParam);
        switch (hdr->code)
        {
        case TBN_DELETINGBUTTON:
            return OnDeletingButton(reinterpret_cast<LPNMTOOLBAR>(hdr));

        case PGN_CALCSIZE:
            return OnPagerCalcSize(reinterpret_cast<LPNMPGCALCSIZE>(hdr));

        case TBN_DROPDOWN:
            return ProcessClick(reinterpret_cast<LPNMTOOLBAR>(hdr)->iItem);

        case TBN_HOTITEMCHANGE:
            //return OnHotItemChange(reinterpret_cast<LPNMTBHOTITEM>(hdr), theResult);
            return S_OK;

        case NM_CUSTOMDRAW:
            hr = OnCustomDraw(reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr), &result);
            if (theResult)
                *theResult = result;
            return hr;

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

        case TBN_DRAGOUT: return S_FALSE;

        default:
            TRACE("WM_NOTIFY unknown code %d, %d\n", hdr->code, hdr->idFrom);
            return S_OK;
        }
        return S_FALSE;
    case WM_WININICHANGE:
        if (wParam == SPI_SETFLATMENU)
        {
            SystemParametersInfo(SPI_GETFLATMENU, 0, &m_useFlatMenus, 0);
        }
    }

    return S_FALSE;
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
    bool     isHot, isPopup, isActive;
    TBBUTTONINFO btni;

    switch (cdraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *theResult = CDRF_NOTIFYITEMDRAW;
        return S_OK;

    case CDDS_ITEMPREPAINT:

        HWND tlw;
        m_menuBand->_GetTopLevelWindow(&tlw);

        // The item with an active submenu gets the CHECKED flag.
        isHot = m_hotBar == this && (int) cdraw->nmcd.dwItemSpec == m_hotItem;
        isPopup = m_popupBar == this && (int) cdraw->nmcd.dwItemSpec == m_popupItem;
        isActive = (GetForegroundWindow() == tlw) || (m_popupBar == this);

        if (m_hotItem < 0 && isPopup)
            isHot = TRUE;

        if ((m_useFlatMenus && isHot) || (m_initFlags & SMINIT_VERTICAL))
        {
            COLORREF clrText;
            HBRUSH   bgBrush;
            RECT rc = cdraw->nmcd.rc;
            HDC hdc = cdraw->nmcd.hdc;

            // Remove HOT and CHECKED flags (will restore HOT if necessary)
            cdraw->nmcd.uItemState &= ~(CDIS_HOT | CDIS_CHECKED);

            // Decide on the colors
            if (isHot)
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
            // Set the text color, will be used by the internal drawing code
            cdraw->clrText = GetSysColor(isActive ? COLOR_MENUTEXT : COLOR_GRAYTEXT);

            // Remove HOT and CHECKED flags (will restore HOT if necessary)
            cdraw->nmcd.uItemState &= ~CDIS_HOT;

            // Decide on the colors
            if (isHot)
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
        GetButtonInfo(cdraw->nmcd.dwItemSpec, &btni);

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

    default:
        *theResult = 0L;
        break;
    }
    return S_OK;
}

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager) :
    m_pager(WC_PAGESCROLLER, this),
    m_useFlatMenus(FALSE),
    m_disableMouseTrack(FALSE),
    m_timerEnabled(FALSE),
    m_menuBand(menuBand),
    m_dwMenuFlags(0),
    m_hasSizes(FALSE),
    m_usePager(usePager),
    m_hotBar(NULL),
    m_hotItem(-1),
    m_popupBar(NULL),
    m_popupItem(-1),
    m_isTrackingPopup(FALSE),
    m_cancelingPopup(FALSE)
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
    ClearToolbar();

    if (m_hWnd)
        DestroyWindow();

    if (m_pager.m_hWnd)
        m_pager.DestroyWindow();

    DeleteObject(m_marlett);
}

void CMenuToolbarBase::InvalidateDraw()
{
    InvalidateRect(NULL, FALSE);
}

HRESULT CMenuToolbarBase::ShowDW(BOOL fShow)
{
    ShowWindow(fShow ? SW_SHOW : SW_HIDE);

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
        SetImageList(NULL);
        return S_OK;
    }

    // Assign the correct imagelist and padding based on the current icon size

    int shiml;
    if (m_menuBand->UseBigIcons())
    {
        shiml = SHIL_LARGE;
        SetPadding(4, 0);
    }
    else
    {
        shiml = SHIL_SMALL;
        SetPadding(4, 4);
    }

    IImageList * piml;
    HRESULT hr = SHGetImageList(shiml, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        SetImageList(NULL);
    }
    else
    {
        SetImageList((HIMAGELIST)piml);
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    if (m_hWnd)
        DestroyWindow();

    if (m_pager.m_hWnd)
        m_pager.DestroyWindow();

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

    // HACK & FIXME: CORE-17505
    SubclassWindow(CToolbar::Create(hwndParent, tbStyles, tbExStyles));

    SetWindowTheme(m_hWnd, L"", L"");

    SystemParametersInfo(SPI_GETFLATMENU, 0, &m_useFlatMenus, 0);

    m_menuBand->AdjustForTheme(m_useFlatMenus);

    // If needed, create the pager.
    if (m_usePager)
    {
        LONG pgStyles = PGS_VERT | WS_CHILD | WS_VISIBLE;
        LONG pgExStyles = 0;

        HWND hwndPager = CreateWindowEx(
            pgExStyles, WC_PAGESCROLLER, NULL,
            pgStyles, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            hwndParent, NULL, _AtlBaseModule.GetModuleInstance(), 0);

        m_pager.SubclassWindow(hwndPager);

        ::SetParent(m_hWnd, hwndPager);

        m_pager.SendMessageW(PGM_SETCHILD, 0, reinterpret_cast<LPARAM>(m_hWnd));
    }

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

    TRACE("Sizes out of date, recalculating.\n");

    if (!m_hWnd)
    {
        return S_OK;
    }

    // Obtain the ideal size, to be used as min and max
    GetMaxSize(&m_idealSize);
    GetIdealSize((m_initFlags & SMINIT_VERTICAL) != 0, &m_idealSize);

    TRACE("Ideal Size: (%d, %d) for %d buttons\n", m_idealSize, GetButtonCount());

    // Obtain the button size, to be used as the integral size
    DWORD size = GetButtonSize();
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
    // Update the toolbar or pager to fit the requested rect
    // If we have a pager, set the toolbar height to the ideal height of the toolbar
    if (m_pager.m_hWnd)
    {
        SetWindowPos(NULL, x, y, cx, m_idealSize.cy, 0);
        m_pager.SetWindowPos(NULL, x, y, cx, cy, 0);
    }
    else
    {
        SetWindowPos(NULL, x, y, cx, cy, 0);
    }

    // In a vertical menu, resize the buttons to fit the width
    if (m_initFlags & SMINIT_VERTICAL)
    {
        DWORD btnSize = GetButtonSize();
        SetButtonSize(cx, GET_Y_LPARAM(btnSize));
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::IsWindowOwner(HWND hwnd)
{
    if (m_hWnd && m_hWnd == hwnd) return S_OK;
    if (m_pager.m_hWnd && m_pager.m_hWnd == hwnd) return S_OK;
    return S_FALSE;
}

HRESULT CMenuToolbarBase::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    if (m_pager.m_hWnd)
        *phwnd = m_pager.m_hWnd;
    else
        *phwnd = m_hWnd;

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

HRESULT CMenuToolbarBase::OnPopupTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam != TIMERID_HOTTRACK)
    {
        bHandled = FALSE;
        return 0;
    }

    KillTimer(TIMERID_HOTTRACK);

    if (!m_timerEnabled)
        return 0;

    m_timerEnabled = FALSE;

    if (m_hotItem < 0)
        return 0;

    // Returns S_FALSE if the current item did not show a submenu
    HRESULT hr = PopupItem(m_hotItem, FALSE);
    if (hr != S_FALSE)
        return 0;

    // If we didn't switch submenus, cancel the current popup regardless
    if (m_popupBar)
    {
        HRESULT hr = CancelCurrentPopup();
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
    }

    return 0;
}

HRESULT CMenuToolbarBase::KillPopupTimer()
{
    if (m_timerEnabled)
    {
        m_timerEnabled = FALSE;
        KillTimer(TIMERID_HOTTRACK);
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
        SetHotItem(-1);
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
                SetTimer(TIMERID_HOTTRACK, elapsed);
                m_timerEnabled = TRUE;
                TRACE("SetTimer called with m_hotItem=%d\n", m_hotItem);
            }
        }
        else
        {
            TBBUTTONINFO info;
            info.cbSize = sizeof(info);
            info.dwMask = 0;

            int index = GetButtonInfo(item, &info);

            SetHotItem(index);
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
        CheckButton(m_popupItem, FALSE);
        m_isTrackingPopup = FALSE;
    }

    m_popupBar = toolbar;
    m_popupItem = item;

    if (m_popupBar == this)
    {
        CheckButton(m_popupItem, TRUE);
    }

    InvalidateDraw();
    return S_OK;
}

LRESULT CMenuToolbarBase::IsTrackedItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TBBUTTON btn;
    INT idx = (INT)wParam;

    if (m_hotBar != this)
        return S_FALSE;

    if (idx < 0)
        return S_FALSE;

    if (!GetButton(idx, &btn))
        return E_FAIL;

    if (m_hotItem == btn.idCommand)
        return S_OK;

    if (m_popupItem == btn.idCommand)
        return S_OK;

    return S_FALSE;
}

LRESULT CMenuToolbarBase::ChangeTrackedItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TBBUTTON btn;
    BOOL wasTracking = LOWORD(lParam);
    BOOL mouse = HIWORD(lParam);
    INT idx = (INT)wParam;

    if (idx < 0)
    {
        m_isTrackingPopup = FALSE;
        return m_menuBand->_ChangeHotItem(NULL, -1, HICF_MOUSE);
    }

    if (!GetButton(idx, &btn))
        return E_FAIL;

    TRACE("ChangeTrackedItem %d, %d\n", idx, wasTracking);
    m_isTrackingPopup = wasTracking;
    return m_menuBand->_ChangeHotItem(this, btn.idCommand, mouse ? HICF_MOUSE : 0);
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT iItem, UINT index, IShellMenu* childShellMenu, BOOL keyInitiated)
{
    // Calculate the submenu position and exclude area
    RECT rc = { 0 };

    if (!GetItemRect(index, &rc))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(&a);
    ClientToScreen(&b);

    POINTL pt = { a.x, b.y };
    RECTL rcl = { a.x, a.y, b.x, b.y };

    if (m_initFlags & SMINIT_VERTICAL)
    {
        // FIXME: Hardcoding this here feels hacky.
        if (IsAppThemed())
        {
            pt.x = b.x - 1;
            pt.y = a.y - 1;
        }
        else
        {
            pt.x = b.x - 3;
            pt.y = a.y - 3;
        }
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

    if (!GetItemRect(index, &rc))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(&a);
    ClientToScreen(&b);

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

HRESULT CMenuToolbarBase::BeforeCancelPopup()
{
    m_cancelingPopup = TRUE;
    TRACE("BeforeCancelPopup\n");
    return S_OK;
}

HRESULT CMenuToolbarBase::ProcessClick(INT iItem)
{
    if (m_disableMouseTrack)
    {
        TRACE("Item click prevented by DisableMouseTrack\n");
        return S_OK;
    }

    // If a button is clicked while a submenu was open, cancel the submenu.
    if (!(m_initFlags & SMINIT_VERTICAL) && m_isTrackingPopup)
    {
        TRACE("OnCommand cancelled because it was tracking submenu.\n");
        return S_FALSE;
    }

    if (PopupItem(iItem, FALSE) == S_OK)
    {
        TRACE("PopupItem returned S_OK\n");
        return S_FALSE;
    }

    TRACE("Executing...\n");

    return m_menuBand->_MenuItemSelect(MPOS_EXECUTE);
}

HRESULT CMenuToolbarBase::ProcessContextMenu(INT iItem)
{
    INT index;
    DWORD_PTR data;

    GetDataFromId(iItem, &index, &data);

    DWORD pos = GetMessagePos();
    POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };

    return InternalContextMenu(iItem, index, data, pt);
}

HRESULT CMenuToolbarBase::MenuBarMouseDown(INT iIndex, BOOL isLButton)
{
    TBBUTTON btn;

    GetButton(iIndex, &btn);

    if ((m_initFlags & SMINIT_VERTICAL)
        || m_popupBar
        || m_cancelingPopup)
    {
        m_cancelingPopup = FALSE;
        return S_OK;
    }

    return ProcessClick(btn.idCommand);
}

HRESULT CMenuToolbarBase::MenuBarMouseUp(INT iIndex, BOOL isLButton)
{
    TBBUTTON btn;

    m_cancelingPopup = FALSE;

    if (!(m_initFlags & SMINIT_VERTICAL))
        return S_OK;

    GetButton(iIndex, &btn);

    if (isLButton)
        return ProcessClick(btn.idCommand);
    else
        return ProcessContextMenu(btn.idCommand);
}

HRESULT CMenuToolbarBase::PrepareExecuteItem(INT iItem)
{
    this->m_menuBand->_KillPopupTimers();

    m_executeItem = iItem;
    return GetDataFromId(iItem, &m_executeIndex, &m_executeData);
}

HRESULT CMenuToolbarBase::ExecuteItem()
{
    return InternalExecuteItem(m_executeItem, m_executeItem, m_executeData);
}

HRESULT CMenuToolbarBase::KeyboardItemChange(DWORD dwSelectType)
{
    int prev = m_hotItem;
    int index = -1;

    if (dwSelectType != 0xFFFFFFFF)
    {
        int count = GetButtonCount();

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
                index = GetButtonInfo(m_hotItem, &info);
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
            DWORD res = GetButton(index, &btn);
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
                        ::SendMessageW(tlw, WM_CANCELMODE, 0, 0);
                        PostMessageW(WM_USER_CHANGETRACKEDITEM, index, MAKELPARAM(m_isTrackingPopup, FALSE));
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

    m_hasSizes = FALSE;

    if (!AddButtons(1, &tbb))
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

    m_hasSizes = FALSE;

    if (!AddButtons(1, &tbb))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::AddPlaceholder()
{
    TBBUTTON tbb = { 0 };
    WCHAR MenuString[128];

    LoadStringW(GetModuleHandle(L"shell32.dll"), IDS_MENU_EMPTY, MenuString, _countof(MenuString));

    tbb.fsState = 0;
    tbb.fsStyle = 0;
    tbb.iString = (INT_PTR) MenuString;
    tbb.iBitmap = -1;

    m_hasSizes = FALSE;

    if (!AddButtons(1, &tbb))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT CMenuToolbarBase::ClearToolbar()
{
    while (DeleteButton(0))
    {
        // empty;
    }
    m_hasSizes = FALSE;
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

    int index = GetButtonInfo(iItem, &info);
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
    m_hmenu(NULL),
    m_hwndMenu(NULL)
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
        *phwnd = m_hwndMenu;
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
    m_hwndMenu = hwnd;
    m_dwMenuFlags = dwFlags;

    if (IsWindow())
        ClearToolbar();

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

            HRESULT hr = m_menuBand->_CallCBWithItemId(info.wID, SMC_GETINFO, 0, reinterpret_cast<LPARAM>(sminfo));
            if (FAILED_UNEXPECTEDLY(hr))
            {
                delete sminfo;
                return hr;
            }

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

int CALLBACK PidlListSort(void* item1, void* item2, LPARAM lParam)
{
    IShellFolder * psf = (IShellFolder*) lParam;
    PCUIDLIST_RELATIVE pidl1 = (PCUIDLIST_RELATIVE) item1;
    PCUIDLIST_RELATIVE pidl2 = (PCUIDLIST_RELATIVE) item2;
    HRESULT hr = psf->CompareIDs(0, pidl1, pidl2);
    if (FAILED(hr))
    {
        // No way to cancel, so sort to equal.
        return 0;
    }
    return (int)(short)LOWORD(hr);
}

HRESULT CMenuSFToolbar::FillToolbar(BOOL clearFirst)
{
    HRESULT hr;

    CComPtr<IEnumIDList> eidl;
    hr = m_shellFolder->EnumObjects(GetToolbar(), SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    HDPA dpaSort = DPA_Create(10);

    LPITEMIDLIST item = NULL;
    hr = eidl->Next(1, &item, NULL);
    while (hr == S_OK)
    {
        if (m_menuBand->_CallCBWithItemPidl(item, 0x10000000, 0, 0) == S_FALSE)
        {
            DPA_AppendPtr(dpaSort, item);
        }
        else
        {
            CoTaskMemFree(item);
        }

        hr = eidl->Next(1, &item, NULL);
    }

    // If no items were added, show the "empty" placeholder
    if (DPA_GetPtrCount(dpaSort) == 0)
    {
        DPA_Destroy(dpaSort);
        return AddPlaceholder();
    }

    TRACE("FillToolbar added %d items to the DPA\n", DPA_GetPtrCount(dpaSort));

    DPA_Sort(dpaSort, PidlListSort, (LPARAM) m_shellFolder.p);

    for (int i = 0; i<DPA_GetPtrCount(dpaSort);)
    {
        PWSTR MenuString;

        INT index = 0;
        INT indexOpen = 0;

        STRRET sr = { STRRET_CSTR, { 0 } };

        item = (LPITEMIDLIST)DPA_GetPtr(dpaSort, i);

        hr = m_shellFolder->GetDisplayNameOf(item, SIGDN_NORMALDISPLAY, &sr);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            DPA_Destroy(dpaSort);
            return hr;
        }

        StrRetToStr(&sr, NULL, &MenuString);

        index = SHMapPIDLToSystemImageListIndex(m_shellFolder, item, &indexOpen);

        LPCITEMIDLIST itemc = item;

        SFGAOF attrs = SFGAO_FOLDER;
        hr = m_shellFolder->GetAttributesOf(1, &itemc, &attrs);

        DWORD_PTR dwData = reinterpret_cast<DWORD_PTR>(item);

        // Fetch next item already, so we know if the current one is the last
        i++;

        AddButton(i, MenuString, attrs & SFGAO_FOLDER, index, dwData, i >= DPA_GetPtrCount(dpaSort));

        CoTaskMemFree(MenuString);
    }

    DPA_Destroy(dpaSort);
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

    if (IsWindow())
        ClearToolbar();

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
                ERR("ILClone failed!\n");
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

    hr = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &shellMenu));
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

    hr = shellMenu->SetShellFolder(childFolder, NULL, NULL, SMSET_TOP);
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
