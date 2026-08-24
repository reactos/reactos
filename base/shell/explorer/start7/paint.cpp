/*
 * ReactOS Explorer
 *
 * Copyright 2026 ReactOS contributors
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * paint.cpp - geometry helpers and all painting of CWin7StartMenu.
 */

#include "../precomp.h"

/*****************************************************************************
 * geometry helpers
 */

int CWin7StartMenu::ItemHeight(INT pane)
{
    return pane == PANE_RIGHT ? SM7_RITEM_HEIGHT : SM7_ITEM_HEIGHT;
}

int CWin7StartMenu::ItemCount(INT pane) const
{
    return pane == PANE_RIGHT ? (INT)m_RightItems.GetCount() : (INT)m_LeftItems.GetCount();
}

RECT CWin7StartMenu::PaneRect(INT pane) const
{
    RECT rc = { 0, 0, 0, 0 };

    switch (pane)
    {
        case PANE_LEFT: rc = m_rcLeft; break;
        case PANE_RIGHT: rc = m_rcRight; break;
        case PANE_BAR: rc = m_rcBar; break;
    }
    return rc;
}

RECT CWin7StartMenu::ItemRect(INT pane, INT index) const
{
    RECT rc = PaneRect(pane);
    int ih = ItemHeight(pane);

    rc.top += index * ih;
    rc.bottom = rc.top + ih;
    if (pane == PANE_RIGHT)
    {
        rc.left += 4;
        rc.right -= 4;
    }
    return rc;
}

BOOL CWin7StartMenu::IsSeparator(INT pane, INT index) const
{
    if (pane == PANE_RIGHT && index >= 0 && (UINT)index < m_RightItems.GetCount())
        return m_RightItems[index].bSeparator;
    return FALSE;
}

int CWin7StartMenu::HitTestPane(INT pane, INT x, INT y) const
{
    RECT rc = PaneRect(pane);
    POINT pt = { x, y };
    int idx;

    if (!PtInRect(&rc, pt))
        return -1;

    idx = (y - rc.top) / ItemHeight(pane);
    if (idx < 0 || idx >= ItemCount(pane))
        return -1;

    RECT rcItem = ItemRect(pane, idx);
    if (!PtInRect(&rcItem, pt))
        return -1;
    if (IsSeparator(pane, idx))
        return -1;

    return idx;
}

HWND CWin7StartMenu::PaneWindow(INT pane) const
{
    switch (pane)
    {
        case PANE_LEFT: return m_hwndLeft;
        case PANE_RIGHT: return m_hwndRight;
        case PANE_BAR: return m_hwndBar;
    }
    return NULL;
}

VOID CWin7StartMenu::InvalidateItem(INT pane, INT index)
{
    HWND hwndPane = PaneWindow(pane);

    if (!hwndPane || index < 0 || pane == PANE_BAR)
        return;

    RECT rc = ItemRect(pane, index);
    MapWindowPoints(m_hWnd, hwndPane, (LPPOINT)&rc, 2);
    ::InvalidateRect(hwndPane, &rc, FALSE);
}

/*****************************************************************************
 * main window frame
 */

LRESULT CWin7StartMenu::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);
    RECT rc;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    GetClientRect(&rc);

    /* Column backgrounds behind the canvases (visible in the margins) */
    SM7FillGradient(hdc, m_rcRight, SM7_CLR_RIGHT_TOP, SM7_CLR_RIGHT_BOT);
    SM7FillGradient(hdc, m_rcBar, SM7_CLR_BAR_TOP, SM7_CLR_BAR_BOT);
    SM7FillGradient(hdc, m_rcLeft, SM7_CLR_WHITE, SM7_CLR_WHITE);

    /* Vertical divider between the columns */
    RECT rcDiv = { m_rcLeft.right, m_rcLeft.top, m_rcLeft.right + 1, m_rcLeft.bottom };
    SetDCBrushColor(hdc, SM7_CLR_DIVIDER);
    FillRect(hdc, &rcDiv, (HBRUSH)GetStockObject(DC_BRUSH));

    /* Horizontal divider above the bottom bar */
    RECT rcBDiv = { m_rcBar.left, m_rcBar.top - 1, m_rcBar.right, m_rcBar.top };
    FillRect(hdc, &rcBDiv, (HBRUSH)GetStockObject(DC_BRUSH));

    /* Search box outline around the edit control */
    SelectObject(hdc, GetStockObject(DC_PEN));
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    SetDCPenColor(hdc, SM7_CLR_HOVER_BRD);
    Rectangle(hdc, m_rcSearch.left - 2, m_rcSearch.top - 2,
              m_rcSearch.right + 2, m_rcSearch.bottom + 2);

    /* Outer rounded frame */
    SetDCPenColor(hdc, SM7_CLR_FRAME);
    RoundRect(hdc, 0, 0, rc.right, rc.bottom, SM7_RADIUS * 2, SM7_RADIUS * 2);

    EndPaint(&ps);
    bHandled = TRUE;
    return 0;
}

/*****************************************************************************
 * pane painting (delegated from the canvas children)
 */

VOID CWin7StartMenu::PaintItem(HDC hdc, INT pane, INT index, BOOL bHover, BOOL bActive)
{
    RECT rcItem = ItemRect(pane, index);

    if (pane == PANE_RIGHT && IsSeparator(pane, index))
    {
        RECT rc = rcItem;
        rc.left += 10;
        rc.right -= 10;
        rc.top = (rc.top + rc.bottom) / 2;
        rc.bottom = rc.top + 1;
        SetDCBrushColor(hdc, SM7_CLR_SEP);
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        return;
    }

    CAtlArray<SM7_ITEM> &arr = (pane == PANE_RIGHT) ? m_RightItems : m_LeftItems;
    if ((UINT)index >= arr.GetCount())
        return;
    SM7_ITEM &item = arr[index];

    if (bHover || bActive)
    {
        SelectObject(hdc, GetStockObject(DC_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCPenColor(hdc, SM7_CLR_HOVER_BRD);
        SetDCBrushColor(hdc, SM7_CLR_HOVER_BG);
        RoundRect(hdc, rcItem.left, rcItem.top, rcItem.right - 3, rcItem.bottom - 1, 6, 6);
    }

    int cxIcon = pane == PANE_RIGHT ? 16 : 24;
    int xIcon = rcItem.left + (pane == PANE_RIGHT ? 8 : 7);
    int yIcon = rcItem.top + ((rcItem.bottom - rcItem.top) - cxIcon) / 2;

    if (item.hIcon)
        DrawIconEx(hdc, xIcon, yIcon, item.hIcon, cxIcon, cxIcon, 0, NULL, DI_NORMAL);

    RECT rcText = rcItem;
    rcText.left = xIcon + cxIcon + 8;
    rcText.right -= 8;
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, item.bBold ? m_FontBold : m_Font);
    SetTextColor(hdc, SM7_CLR_TEXT);
    DrawTextW(hdc, item.name, -1, &rcText,
              DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}

VOID CWin7StartMenu::DrawBarButton(HDC hdc, INT which)
{
    RECT rcBtn = which == 1 ? m_rcShutdown : m_rcArrow;
    BOOL bHover = m_BarHover == which;

    if (bHover)
    {
        SelectObject(hdc, GetStockObject(DC_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCPenColor(hdc, RGB(150, 190, 225));
        SetDCBrushColor(hdc, RGB(222, 240, 252));
        RoundRect(hdc, rcBtn.left, rcBtn.top, rcBtn.right, rcBtn.bottom, 6, 6);
    }

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_Font);

    if (which == 1)
    {
        RECT rcText = rcBtn;
        /* TODO: localized label */
        SetTextColor(hdc, SM7_CLR_TEXT);
        DrawTextW(hdc, L"Shut down", -1, &rcText,
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    }
    else
    {
        int cx = rcBtn.left + (rcBtn.right - rcBtn.left) / 2;
        int cy = rcBtn.top + (rcBtn.bottom - rcBtn.top) / 2;
        POINT pts[3] =
        {
            { cx - 4, cy - 2 },
            { cx + 5, cy - 2 },
            { cx, cy + 4 },
        };
        SelectObject(hdc, GetStockObject(DC_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCPenColor(hdc, SM7_CLR_FRAME);
        SetDCBrushColor(hdc, SM7_CLR_FRAME);
        Polygon(hdc, pts, _countof(pts));
    }
}

LRESULT CWin7StartMenu::OnCanvasPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HDC hdc = (HDC)wParam;
    INT pane = (INT)lParam;
    RECT rcPane = PaneRect(pane);

    UNREFERENCED_PARAMETER(uMsg);
    bHandled = TRUE;

    /* Paint using parent-client coordinates */
    SaveDC(hdc);
    SetViewportOrgEx(hdc, -rcPane.left, -rcPane.top, NULL);

    switch (pane)
    {
        case PANE_LEFT:
        {
            SM7FillGradient(hdc, m_rcLeft, SM7_CLR_WHITE, SM7_CLR_WHITE);

            int count = ItemCount(PANE_LEFT);
            if (count == 0)
            {
                RECT rcEmpty = m_rcLeft;
                rcEmpty.left += 12;
                SetBkMode(hdc, TRANSPARENT);
                SelectObject(hdc, m_Font);
                SetTextColor(hdc, SM7ClrGrayText);
                DrawTextW(hdc, L"(Empty)", -1, &rcEmpty,
                          DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            }
            else
            {
                for (int i = 0; i < count; i++)
                {
                    PaintItem(hdc, PANE_LEFT, i,
                              m_HoverPane == PANE_LEFT && m_HoverIndex == i,
                              m_ActivePane == PANE_LEFT && m_ActiveIndex == i);
                }
            }
            break;
        }

        case PANE_RIGHT:
        {
            SM7FillGradient(hdc, m_rcRight, SM7_CLR_RIGHT_TOP, SM7_CLR_RIGHT_BOT);

            int count = ItemCount(PANE_RIGHT);
            for (int i = 0; i < count; i++)
            {
                PaintItem(hdc, PANE_RIGHT, i,
                          m_HoverPane == PANE_RIGHT && m_HoverIndex == i,
                          m_ActivePane == PANE_RIGHT && m_ActiveIndex == i);
            }
            break;
        }

        case PANE_BAR:
        {
            SM7FillGradient(hdc, m_rcBar, SM7_CLR_BAR_TOP, SM7_CLR_BAR_BOT);
            DrawBarButton(hdc, 1);
            DrawBarButton(hdc, 2);
            break;
        }
    }

    RestoreDC(hdc, -1);
    return 0;
}
