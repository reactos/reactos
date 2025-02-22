/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Scrolling related helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#include "precomp.h"

void OnKeyScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state)
{
    // NOTE: Windows uses an offset of 16 pixels
    switch (wParam)
    {
        case VK_UP:
            OnScroll(hWnd, SB_VERT, MAKELONG(SB_LINEUP, 0), 5, state);
            break;

        case VK_DOWN:
            OnScroll(hWnd, SB_VERT, MAKELONG(SB_LINEDOWN, 0), 5, state);
            break;

        case VK_LEFT:
            OnScroll(hWnd, SB_HORZ, MAKELONG(SB_LINELEFT, 0), 5, state);
            break;

        case VK_RIGHT:
            OnScroll(hWnd, SB_HORZ, MAKELONG(SB_LINERIGHT, 0), 5, state);
            break;

        case VK_PRIOR:
            OnScroll(hWnd, SB_VERT, MAKELONG(SB_PAGEUP, 0), state->nPageY, state);
            break;

        case VK_NEXT:
            OnScroll(hWnd, SB_VERT, MAKELONG(SB_PAGEDOWN, 0), state->nPageY, state);
            break;

        case VK_HOME:
        {
            OnScroll(hWnd, SB_HORZ, MAKELONG(SB_LEFT, 0), 0, state);
            if (GetKeyState(VK_CONTROL) & 0x8000)
                OnScroll(hWnd, SB_VERT, MAKELONG(SB_TOP, 0), 0, state);
            break;
        }

        case VK_END:
        {
            OnScroll(hWnd, SB_HORZ, MAKELONG(SB_RIGHT, 0), 0, state);
            if (GetKeyState(VK_CONTROL) & 0x8000)
                OnScroll(hWnd, SB_VERT, MAKELONG(SB_BOTTOM, 0), 0, state);
            break;
        }

        default:
            break;
    }
}

void OnMouseScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state)
{
    INT nBar;
    INT nPage;
    INT iDelta;
    UINT uLinesToScroll = state->uLinesToScroll;
    INT zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    WORD sbCode;

    assert(uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL);

    if (uMsg == WM_MOUSEWHEEL)
    {
        nBar = SB_VERT;
        nPage = state->nPageY;

        /* Accumulate wheel rotation ticks */
        zDelta += state->iWheelCarryoverY;
        state->iWheelCarryoverY = zDelta % WHEEL_DELTA;
    }
    else // if (uMsg == WM_MOUSEHWHEEL)
    {
        nBar = SB_HORZ;
        nPage = state->nPageX;

        /* Accumulate wheel rotation ticks */
        zDelta += state->iWheelCarryoverX;
        state->iWheelCarryoverX = zDelta % WHEEL_DELTA;
    }

    /*
     * If the user specified scrolling by pages, do so.
     * Due to a bug on Windows where, if the window height is
     * less than the scroll lines delta default value (== 3),
     * several lines would be skipped when scrolling if we
     * used the WHEEL_PAGESCROLL value. Instead of this, use
     * the number of lines per page as the limiting value.
     * See https://www.strchr.com/corrections_to_raymond_chen_s_wheel_scrolling_code
     * for more details.
     */
    if (uLinesToScroll > nPage) // (uLinesToScroll == WHEEL_PAGESCROLL)
        uLinesToScroll = nPage;
    /* If the user specified no wheel scrolling, don't do anything */
    else if (uLinesToScroll == 0)
        return;

    /* Compute the scroll direction and the absolute delta value */
    if (zDelta > 0)
    {
        sbCode = SB_LINEUP;
    }
    else
    {
        sbCode = SB_LINEDOWN;
        zDelta = -zDelta;
    }

    /* Compute how many lines we should scroll (in absolute value) */
    iDelta = (INT)uLinesToScroll * zDelta / WHEEL_DELTA;

    OnScroll(hWnd, nBar, MAKELONG(sbCode, 0), iDelta, state);
}

void OnScroll(HWND hWnd, INT nBar, WPARAM wParam, INT iDelta, LPSCROLLSTATE state)
{
    SCROLLINFO si;
    PINT pCurrent;
    INT Maximum;
    INT NewPos;
    INT OldX, OldY;

    assert(nBar == SB_HORZ || nBar == SB_VERT);

    if (Globals.uDisplayFormat == CF_OWNERDISPLAY)
    {
        if (nBar == SB_HORZ)
        {
            SendClipboardOwnerMessage(TRUE, WM_HSCROLLCLIPBOARD,
                                      (WPARAM)hWnd, (LPARAM)wParam);
        }
        else // if (nBar == SB_VERT)
        {
            SendClipboardOwnerMessage(TRUE, WM_VSCROLLCLIPBOARD,
                                      (WPARAM)hWnd, (LPARAM)wParam);
        }
        return;
    }

    if (nBar == SB_HORZ)
    {
        pCurrent = &state->CurrentX;
        Maximum = state->MaxX;
    }
    else // if (nBar == SB_VERT)
    {
        pCurrent = &state->CurrentY;
        Maximum = state->MaxY;
    }

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
    GetScrollInfo(hWnd, nBar, &si);

    switch (LOWORD(wParam))
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        {
            NewPos = si.nTrackPos;
            break;
        }

        case SB_LINEUP:     // SB_LINELEFT:
        {
            NewPos = si.nPos - iDelta;
            break;
        }

        case SB_LINEDOWN:   // SB_LINERIGHT:
        {
            NewPos = si.nPos + iDelta;
            break;
        }

        case SB_PAGEUP:     // SB_PAGELEFT:
        {
            NewPos = si.nPos - si.nPage;
            break;
        }

        case SB_PAGEDOWN:   // SB_PAGERIGHT:
        {
            NewPos = si.nPos + si.nPage;
            break;
        }

        case SB_TOP:        // SB_LEFT:
        {
            NewPos = si.nMin;
            break;
        }

        case SB_BOTTOM:     // SB_RIGHT:
        {
            NewPos = si.nMax;
            break;
        }

        default:
        {
            NewPos = si.nPos;
            break;
        }
    }

    NewPos = min(max(NewPos, 0), Maximum);

    if (si.nPos == NewPos)
        return;

    OldX = state->CurrentX;
    OldY = state->CurrentY;
    *pCurrent = NewPos;

    ScrollWindowEx(hWnd,
                   OldX - state->CurrentX,
                   OldY - state->CurrentY,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   SW_ERASE | SW_INVALIDATE);
    UpdateWindow(hWnd);

    si.fMask = SIF_POS;
    si.nPos = NewPos;
    SetScrollInfo(hWnd, nBar, &si, TRUE);
}

void UpdateLinesToScroll(LPSCROLLSTATE state)
{
    UINT uLinesToScroll;

    if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uLinesToScroll, 0))
    {
        /* Default value on Windows */
        state->uLinesToScroll = 3;
    }
    else
    {
        state->uLinesToScroll = uLinesToScroll;
    }
}

void UpdateWindowScrollState(HWND hWnd, INT nMaxWidth, INT nMaxHeight, LPSCROLLSTATE lpState)
{
    RECT rc;
    SCROLLINFO si;

    if (!GetClientRect(hWnd, &rc))
        SetRectEmpty(&rc);

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;

    lpState->nMaxWidth = nMaxWidth;
    lpState->MaxX = max(nMaxWidth - rc.right, 0);
    lpState->CurrentX = min(lpState->CurrentX, lpState->MaxX);
    lpState->nPageX = rc.right;
    si.nMin  = 0;
    si.nMax  = nMaxWidth;
    si.nPage = lpState->nPageX;
    si.nPos  = lpState->CurrentX;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

    lpState->nMaxHeight = nMaxHeight;
    lpState->MaxY = max(nMaxHeight - rc.bottom, 0);
    lpState->CurrentY = min(lpState->CurrentY, lpState->MaxY);
    lpState->nPageY = rc.bottom;
    si.nMin  = 0;
    si.nMax  = nMaxHeight;
    si.nPage = lpState->nPageY;
    si.nPos  = lpState->CurrentY;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}
