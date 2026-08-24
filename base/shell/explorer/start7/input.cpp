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
 * input.cpp - selection state and user input handling of CWin7StartMenu:
 * hover/keyboard selection, search box notifications and the power menu.
 */

#include "../precomp.h"

/*****************************************************************************
 * selection state
 */

VOID CWin7StartMenu::SetHover(INT pane, INT index)
{
    if (m_HoverPane == pane && m_HoverIndex == index)
        return;

    if (m_HoverIndex >= 0 && m_HoverPane != PANE_NONE)
        InvalidateItem(m_HoverPane, m_HoverIndex);

    m_HoverPane = pane;
    m_HoverIndex = index;

    if (index >= 0 && pane != PANE_NONE)
        InvalidateItem(pane, index);
}

VOID CWin7StartMenu::SetActive(INT pane, INT index)
{
    if (m_ActivePane == pane && m_ActiveIndex == index)
        return;

    if (m_ActiveIndex >= 0 && m_ActivePane != PANE_NONE)
        InvalidateItem(m_ActivePane, m_ActiveIndex);

    m_ActivePane = pane;
    m_ActiveIndex = index;

    if (index >= 0 && pane != PANE_NONE)
        InvalidateItem(pane, index);
}

VOID CWin7StartMenu::MoveSelection(INT pane, INT delta)
{
    int count = ItemCount(pane);
    int idx;

    if (count == 0)
        return;

    idx = m_ActivePane == pane ? m_ActiveIndex : (delta > 0 ? -1 : count);

    do
    {
        idx += delta;
    } while ((idx >= 0 && idx < count) && IsSeparator(pane, idx));

    if (idx < 0 || idx >= count)
        return;

    SetActive(pane, idx);
}

VOID CWin7StartMenu::SwitchColumn(INT pane)
{
    int count = ItemCount(pane);
    int idx;

    if (count == 0)
    {
        SetActive(PANE_NONE, -1);
        return;
    }

    idx = min(max(m_ActiveIndex, 0), count - 1);
    while (idx >= 0 && IsSeparator(pane, idx))
    {
        ++idx;
        if (idx >= count)
            idx = count - 1;
    }

    SetActive(pane, idx);
}

/*****************************************************************************
 * message handlers
 */

LRESULT CWin7StartMenu::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = TRUE;

    /* Dismiss when something else takes the foreground, but stay open when
       the tray window itself was clicked: the start button posts
       TWM_OPENSTARTMENU and CTrayWindow toggles us closed in that case. */
    if (LOWORD(wParam) == WA_INACTIVE &&
        ::GetForegroundWindow() != m_Tray->GetHWND())
    {
        Hide();
    }
    return 0;
}

LRESULT CWin7StartMenu::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = TRUE;

    if ((wParam & 0xFFF0) == SC_KEYMENU)
    {
        Hide();
        return 0;
    }
    bHandled = FALSE;
    return 0;
}

LRESULT CWin7StartMenu::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = TRUE;

    switch (wParam)
    {
        case VK_ESCAPE:
            Hide();
            break;

        case VK_UP:
            MoveSelection(m_ActivePane != PANE_NONE ? m_ActivePane : PANE_LEFT, -1);
            break;

        case VK_DOWN:
            MoveSelection(m_ActivePane != PANE_NONE ? m_ActivePane : PANE_LEFT, +1);
            break;

        case VK_LEFT:
            SwitchColumn(PANE_LEFT);
            break;

        case VK_RIGHT:
            SwitchColumn(PANE_RIGHT);
            break;

        case VK_RETURN:
            if (m_ActivePane != PANE_NONE && m_ActiveIndex >= 0)
                ExecuteItem(m_ActivePane, m_ActiveIndex);
            break;

        case VK_TAB:
            if (m_hwndSearch)
                SetFocus(m_hwndSearch);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    return 0;
}

LRESULT CWin7StartMenu::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    bHandled = TRUE;

    /* EN_CHANGE from the search box */
    if ((HWND)lParam == m_hwndSearch && HIWORD(wParam) == EN_CHANGE)
    {
        WCHAR szNew[_countof(m_szFilter)];

        GetWindowTextW(m_hwndSearch, szNew, _countof(szNew));
        if (_wcsicmp(szNew, m_szFilter) != 0)
        {
            lstrcpynW(m_szFilter, szNew, _countof(m_szFilter));
            FillLeftItems();
            ResizeToContent();
            ::InvalidateRect(m_hwndLeft, NULL, FALSE);
        }
        return 0;
    }

    bHandled = FALSE;
    return 0;
}

/*****************************************************************************
 * canvas input
 */

LRESULT CWin7StartMenu::OnCanvasMouse(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT pt = { SM7_GET_X_LPARAM(lParam), SM7_GET_Y_LPARAM(lParam) };
    INT pane = PANE_NONE;

    UNREFERENCED_PARAMETER(uMsg);
    bHandled = TRUE;

    if (::PtInRect(&m_rcLeft, pt) && m_hwndLeft)
        pane = PANE_LEFT;
    else if (::PtInRect(&m_rcRight, pt) && m_hwndRight)
        pane = PANE_RIGHT;
    else if (::PtInRect(&m_rcBar, pt) && m_hwndBar)
        pane = PANE_BAR;

    switch (wParam)
    {
        case WM_MOUSEMOVE:
        {
            if (pane == PANE_LEFT || pane == PANE_RIGHT)
            {
                SetHover(pane, HitTestPane(pane, pt.x, pt.y));
                if (m_HoverIndex >= 0)
                    SetActive(pane, m_HoverIndex);
            }

            INT hover = 0;
            if (pane == PANE_BAR)
            {
                if (::PtInRect(&m_rcShutdown, pt))
                    hover = 1;
                else if (::PtInRect(&m_rcArrow, pt))
                    hover = 2;
            }

            if (hover != m_BarHover)
            {
                m_BarHover = hover;
                ::InvalidateRect(m_hwndBar, NULL, FALSE);
            }
            break;
        }

        case WM_LBUTTONDOWN:
            if (pane == PANE_LEFT || pane == PANE_RIGHT)
            {
                int idx = HitTestPane(pane, pt.x, pt.y);
                SetActive(idx >= 0 ? pane : PANE_NONE, idx);
                SetHover(pane, idx);
            }
            break;

        case WM_LBUTTONUP:
            if (pane == PANE_LEFT || pane == PANE_RIGHT)
            {
                int idx = HitTestPane(pane, pt.x, pt.y);
                if (idx >= 0 && idx == m_ActiveIndex && m_ActivePane == pane)
                    ExecuteItem(pane, idx);
            }
            else if (pane == PANE_BAR)
            {
                if (::PtInRect(&m_rcShutdown, pt))
                {
                    PostMessageW(m_Tray->GetHWND(), WM_COMMAND, SM7_CMD_SHUTDOWN, 0);
                    Hide();
                }
                else if (::PtInRect(&m_rcArrow, pt))
                {
                    TrackPowerMenu();
                }
            }
            break;
    }

    return 0;
}

LRESULT CWin7StartMenu::OnCanvasLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    bHandled = TRUE;

    INT pane = (INT)lParam;
    if (pane == PANE_LEFT || pane == PANE_RIGHT)
        SetHover(PANE_NONE, -1);

    if (m_BarHover)
    {
        m_BarHover = 0;
        ::InvalidateRect(m_hwndBar, NULL, FALSE);
    }

    return 0;
}

/*****************************************************************************
 * power menu
 */

/* TODO: localize these entries once explorer gains suitable string ids */
VOID CWin7StartMenu::TrackPowerMenu()
{
    static const struct
    {
        UINT cmd;
        LPCWSTR label;
    } entries[] =
    {
        { SM7_CMD_SWITCHUSER, L"Switch user" },
        { SM7_CMD_LOGOFF,     L"Log off" },
        { SM7_CMD_LOCK,       L"Lock" },
    };

    HMENU hmenu = CreatePopupMenu();
    if (!hmenu)
        return;

    for (SIZE_T i = 0; i < _countof(entries); i++)
    {
        MENUITEMINFOW mii = { sizeof(mii) };
        mii.fMask = MIIM_ID | MIIM_STRING;
        mii.wID = entries[i].cmd;
        mii.dwTypeData = const_cast<LPWSTR>(entries[i].label);
        InsertMenuItemW(hmenu, GetMenuItemCount(hmenu), TRUE, &mii);
    }

    POINT pt = { m_rcArrow.right, m_rcArrow.top };
    ClientToScreen(m_hWnd, &pt);

    UINT uCmd = TrackPopupMenuEx(hmenu,
                                 TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_RIGHTALIGN |
                                 TPM_BOTTOMALIGN,
                                 pt.x, pt.y, m_hWnd, NULL);
    DestroyMenu(hmenu);

    if (uCmd)
    {
        PostMessageW(m_Tray->GetHWND(), WM_COMMAND, uCmd, 0);
        Hide();
    }
}
