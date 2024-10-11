/*
 * Drag List control
 *
 * Copyright 1999 Eric Kohl
 * Copyright 2004 Robert Shearman
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
 *
 * NOTES
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Mar. 10, 2004, by Robert Shearman.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 * 
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

#define DRAGLIST_SUBCLASSID     0
#define DRAGLIST_SCROLLPERIOD 200
#define DRAGLIST_TIMERID      666

/* properties relating to IDI_DRAGICON */
#define DRAGICON_HOTSPOT_X 17
#define DRAGICON_HOTSPOT_Y  7
#define DRAGICON_HEIGHT    32

/* internal Wine specific data for the drag list control */
typedef struct _DRAGLISTDATA
{
    /* are we currently in dragging mode? */
    BOOL dragging;

    /* cursor to use as determined by DL_DRAGGING notification.
     * NOTE: as we use LoadCursor we don't have to use DeleteCursor
     * when we are finished with it */
    HCURSOR cursor;

    /* optimisation so that we don't have to load the cursor
     * all of the time whilst dragging */
    LRESULT last_dragging_response;

    /* prevents flicker with drawing drag arrow */
    RECT last_drag_icon_rect;
} DRAGLISTDATA;

UINT uDragListMessage = 0; /* registered window message code */
static DWORD dwLastScrollTime = 0;
static HICON hDragArrow = NULL;

/***********************************************************************
 *		DragList_Notify (internal)
 *
 * Sends notification messages to the parent control. Note that it
 * does not use WM_NOTIFY like the rest of the controls, but a registered
 * window message.
 */
static LRESULT DragList_Notify(HWND hwndLB, UINT uNotification)
{
    DRAGLISTINFO dli;
    dli.hWnd = hwndLB;
    dli.uNotification = uNotification;
    GetCursorPos(&dli.ptCursor);
    return SendMessageW(GetParent(hwndLB), uDragListMessage, GetDlgCtrlID(hwndLB), (LPARAM)&dli);
}

/* cleans up after dragging */
static void DragList_EndDrag(HWND hwnd, DRAGLISTDATA * data)
{
    KillTimer(hwnd, DRAGLIST_TIMERID);
    ReleaseCapture();
    /* clear any drag insert icon present */
    InvalidateRect(GetParent(hwnd), &data->last_drag_icon_rect, TRUE);
    /* clear data for next use */
    memset(data, 0, sizeof(*data));
}

/***********************************************************************
 *		DragList_SubclassWindowProc (internal)
 *
 * Handles certain messages to enable dragging for the ListBox and forwards
 * the rest to the ListBox.
 */
static LRESULT CALLBACK
DragList_SubclassWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    DRAGLISTDATA * data = (DRAGLISTDATA*)dwRefData;
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        data->dragging = DragList_Notify(hwnd, DL_BEGINDRAG);
        if (data->dragging)
        {
            SetCapture(hwnd);
            SetTimer(hwnd, DRAGLIST_TIMERID, DRAGLIST_SCROLLPERIOD, NULL);
        }
        /* note that we don't absorb this message to let the list box
         * do its thing (normally selecting an item) */
        break;

    case WM_KEYDOWN:
    case WM_RBUTTONDOWN:
        /* user cancelled drag by either right clicking or
         * by pressing the escape key */
        if ((data->dragging) &&
            ((uMsg == WM_RBUTTONDOWN) || (wParam == VK_ESCAPE)))
        {
            /* clean up and absorb message */
            DragList_EndDrag(hwnd, data);
            DragList_Notify(hwnd, DL_CANCELDRAG);
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    case WM_TIMER:
        if (data->dragging)
        {
            LRESULT cursor = DragList_Notify(hwnd, DL_DRAGGING);
            /* optimisation so that we don't have to load the cursor
             * all of the time whilst dragging */
            if (data->last_dragging_response != cursor)
            {
                switch (cursor)
                {
                case DL_STOPCURSOR:
                    data->cursor = LoadCursorW(NULL, (LPCWSTR)IDC_NO);
                    SetCursor(data->cursor);
                    break;
                case DL_COPYCURSOR:
                    data->cursor = LoadCursorW(COMCTL32_hModule, (LPCWSTR)IDC_COPY);
                    SetCursor(data->cursor);
                    break;
                case DL_MOVECURSOR:
                    data->cursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
                    SetCursor(data->cursor);
                    break;
                }
                data->last_dragging_response = cursor;
            }
            /* don't pass this message on to List Box */
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (data->dragging)
        {
            DragList_EndDrag(hwnd, data);
            DragList_Notify(hwnd, DL_DROPPED);
        }
        break;

    case WM_GETDLGCODE:
        /* tell dialog boxes that we want to receive WM_KEYDOWN events
         * for keys like VK_ESCAPE */
        if (data->dragging)
            return DLGC_WANTALLKEYS;
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DragList_SubclassWindowProc, DRAGLIST_SUBCLASSID);
        Free(data);
        break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

/***********************************************************************
 *		MakeDragList (COMCTL32.13)
 *
 * Makes a normal ListBox into a DragList by subclassing it.
 *
 * RETURNS
 *      Success: Non-zero
 *      Failure: Zero
 */
BOOL WINAPI MakeDragList (HWND hwndLB)
{
    DRAGLISTDATA *data = Alloc(sizeof(DRAGLISTDATA));

    TRACE("(%p)\n", hwndLB);

    if (!uDragListMessage)
        uDragListMessage = RegisterWindowMessageW(DRAGLISTMSGSTRINGW);

    return SetWindowSubclass(hwndLB, DragList_SubclassWindowProc, DRAGLIST_SUBCLASSID, (DWORD_PTR)data);
}

/***********************************************************************
 *		DrawInsert (COMCTL32.15)
 *
 * Draws insert arrow by the side of the ListBox item in the parent window.
 *
 * RETURNS
 *      Nothing.
 */
VOID WINAPI DrawInsert (HWND hwndParent, HWND hwndLB, INT nItem)
{
    RECT rcItem, rcListBox, rcDragIcon;
    HDC hdc;
    DRAGLISTDATA * data;

    TRACE("(%p %p %d)\n", hwndParent, hwndLB, nItem);

    if (!hDragArrow)
        hDragArrow = LoadIconW(COMCTL32_hModule, (LPCWSTR)IDI_DRAGARROW);

    if (LB_ERR == SendMessageW(hwndLB, LB_GETITEMRECT, nItem, (LPARAM)&rcItem))
        return;

    if (!GetWindowRect(hwndLB, &rcListBox))
        return;

    /* convert item rect to parent co-ordinates */
    if (!MapWindowPoints(hwndLB, hwndParent, (LPPOINT)&rcItem, 2))
        return;

    /* convert list box rect to parent co-ordinates */
    if (!MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rcListBox, 2))
        return;

    rcDragIcon.left = rcListBox.left - DRAGICON_HOTSPOT_X;
    rcDragIcon.top = rcItem.top - DRAGICON_HOTSPOT_Y;
    rcDragIcon.right = rcListBox.left;
    rcDragIcon.bottom = rcDragIcon.top + DRAGICON_HEIGHT;

    if (!GetWindowSubclass(hwndLB, DragList_SubclassWindowProc, DRAGLIST_SUBCLASSID, (DWORD_PTR*)&data))
        return;

    if (nItem < 0)
        SetRectEmpty(&rcDragIcon);

    /* prevent flicker by only redrawing when necessary */
    if (!EqualRect(&rcDragIcon, &data->last_drag_icon_rect))
    {
        /* get rid of any previous inserts drawn */
        RedrawWindow(hwndParent, &data->last_drag_icon_rect, NULL,
            RDW_INTERNALPAINT | RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

        data->last_drag_icon_rect = rcDragIcon;

        if (nItem >= 0)
        {
            hdc = GetDC(hwndParent);

            DrawIcon(hdc, rcDragIcon.left, rcDragIcon.top, hDragArrow);

            ReleaseDC(hwndParent, hdc);
        }
    }
}

/***********************************************************************
 *		LBItemFromPt (COMCTL32.14)
 *
 * Gets the index of the ListBox item under the specified point,
 * scrolling if bAutoScroll is TRUE and pt is outside of the ListBox.
 *
 * RETURNS
 *      The ListBox item ID if pt is over a list item or -1 otherwise.
 */
INT WINAPI LBItemFromPt (HWND hwndLB, POINT pt, BOOL bAutoScroll)
{
    RECT rcClient;
    INT nIndex;
    DWORD dwScrollTime;

    TRACE("%p, %ld x %ld, %s\n", hwndLB, pt.x, pt.y, bAutoScroll ? "TRUE" : "FALSE");

    ScreenToClient (hwndLB, &pt);
    GetClientRect (hwndLB, &rcClient);
    nIndex = (INT)SendMessageW (hwndLB, LB_GETTOPINDEX, 0, 0);

    if (PtInRect (&rcClient, pt))
    {
        /* point is inside -- get the item index */
        while (TRUE)
        {
            if (SendMessageW (hwndLB, LB_GETITEMRECT, nIndex, (LPARAM)&rcClient) == LB_ERR)
                return -1;

            if (PtInRect (&rcClient, pt))
                return nIndex;

            nIndex++;
        }
    }
    else
    {
        /* point is outside */
        if (!bAutoScroll)
            return -1;

        if ((pt.x > rcClient.right) || (pt.x < rcClient.left))
            return -1;

        if (pt.y < 0)
            nIndex--;
        else
            nIndex++;

        dwScrollTime = GetTickCount ();

        if ((dwScrollTime - dwLastScrollTime) < DRAGLIST_SCROLLPERIOD)
            return -1;

        dwLastScrollTime = dwScrollTime;

        SendMessageW (hwndLB, LB_SETTOPINDEX, nIndex, 0);
    }

    return -1;
}
