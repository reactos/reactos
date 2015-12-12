/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/scrollutils.c
 * PURPOSE:         Scrolling releated helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

static int InternalSetScrollInfo(HWND hWnd, int nMin, int nMax, UINT nPage, int nPos, int fnBar)
{
    SCROLLINFO si;

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    si.nMin   = nMin;
    si.nMax   = nMax;
    si.nPage  = nPage;
    si.nPos   = nPos;

    return SetScrollInfo(hWnd, fnBar, &si, TRUE);
}

void HandleKeyboardScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case VK_UP:
        {
            SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
            break;
        }

        case VK_DOWN:
        {
            SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
            break;
        }

        case VK_LEFT:
        {
            SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEUP, 0), 0);
            break;
        }

        case VK_RIGHT:
        {
            SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
            break;
        }

        case VK_PRIOR:
        {
            SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEUP, 0), 0);
            break;
        }

        case VK_NEXT:
        {
            SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEDOWN, 0), 0);
            break;
        }

        default:
        {
            break;
        }
    }
}

void HandleHorizontalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state)
{
    SCROLLINFO si; 
    int Delta;
    int NewPos;

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_TRACKPOS;
    GetScrollInfo(hWnd, SB_HORZ, &si);

    switch (LOWORD(wParam))
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        {
            NewPos = si.nTrackPos;
            break;
        }

        case SB_LINELEFT:
        {
            NewPos = state->CurrentX - 5;
            break;
        }

        case SB_LINERIGHT:
        {
            NewPos = state->CurrentX + 5;
            break;
        }

        case SB_PAGELEFT:
        {
            NewPos = state->CurrentX - si.nPage;
            break;
        }

        case SB_PAGERIGHT:
        {
            NewPos = state->CurrentX + si.nPage;
            break;
        }

        default:
        {
            NewPos = state->CurrentX;
            break;
        }
    } 

   NewPos = min(state->MaxX, max(0, NewPos));

   if (NewPos == state->CurrentX)
   {
       return;
   }

   Delta = NewPos - state->CurrentX;

   state->CurrentX = NewPos;

   ScrollWindowEx(hWnd, -Delta, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);

   si.cbSize = sizeof(si);
   si.fMask = SIF_POS;
   si.nPos = state->CurrentX;
   SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
}

void HandleVerticalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state)
{
    SCROLLINFO si; 
    int Delta;
    int NewPos;

    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_TRACKPOS;
    GetScrollInfo(hWnd, SB_VERT, &si);

    switch (LOWORD(wParam))
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        {
            NewPos = si.nTrackPos;
            break;
        }

        case SB_LINEUP:
        {
            NewPos = state->CurrentY - 5;
            break;
        }

        case SB_LINEDOWN:
        {
            NewPos = state->CurrentY + 5;
            break;
        }

        case SB_PAGEUP:
        {
            NewPos = state->CurrentY - si.nPage;
            break;
        }

        case SB_PAGEDOWN:
        {
            NewPos = state->CurrentY + si.nPage;
            break;
        }

        default:
        {
            NewPos = state->CurrentY;
            break;
        }
   }

   NewPos = min(state->MaxY, max(0, NewPos));

   if (NewPos == state->CurrentY)
   {
       return;
   }

   Delta = NewPos - state->CurrentY;

   state->CurrentY = NewPos;

   ScrollWindowEx(hWnd, 0, -Delta, NULL, NULL, NULL, NULL, SW_INVALIDATE);

   si.cbSize = sizeof(si);
   si.fMask = SIF_POS;
   si.nPos = state->CurrentY;
   SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

void UpdateWindowScrollState(HWND hWnd, HBITMAP hBitmap, LPSCROLLSTATE lpState)
{
    BITMAP bmp;
    RECT rc;

    if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
    {
        bmp.bmWidth = 0;
        bmp.bmHeight = 0;
    }

    if (!GetClientRect(hWnd, &rc))
    {
        SetRectEmpty(&rc);
    }

    lpState->MaxX = max(bmp.bmWidth - rc.right, 0);
    lpState->CurrentX = min(lpState->CurrentX, lpState->MaxX);
    InternalSetScrollInfo(hWnd, 0, bmp.bmWidth, rc.right, lpState->CurrentX, SB_HORZ);

    lpState->MaxY = max(bmp.bmHeight - rc.bottom, 0);
    lpState->CurrentY = min(lpState->CurrentY, lpState->MaxY);
    InternalSetScrollInfo(hWnd, 0, bmp.bmHeight, rc.bottom, lpState->CurrentY, SB_VERT);
}

BOOL ScrollBlt(PAINTSTRUCT ps, HBITMAP hBmp, SCROLLSTATE state)
{
    RECT rect;
    BOOL ret;
    HDC hdc;
    int xpos;
    int ypos;

    rect.left = ps.rcPaint.left;
    rect.top = ps.rcPaint.top;
    rect.right = (ps.rcPaint.right - ps.rcPaint.left);
    rect.bottom = (ps.rcPaint.bottom - ps.rcPaint.top);

    xpos = ps.rcPaint.left + state.CurrentX;
    ypos = ps.rcPaint.top + state.CurrentY;

    ret = FALSE;

    hdc = CreateCompatibleDC(ps.hdc);
    if (hdc)
    {
        if (SelectObject(hdc, hBmp))
        {
            ret = BitBlt(ps.hdc, rect.left, rect.top, rect.right, rect.bottom, hdc, xpos, ypos, SRCCOPY);
        }
        DeleteDC(hdc);
    }

    return ret;
}
