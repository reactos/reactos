/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Processes List.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

INT_PTR CALLBACK    ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WNDPROC             OldProcessListWndProc;


INT_PTR CALLBACK
ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBRUSH  hbrBackground;
    RECT    rcItem;
    RECT    rcClip;
    HDC     hDC;
    int     DcSave;

    switch (message)
    {
    case WM_ERASEBKGND:

        /*
         * The list control produces a nasty flicker
         * when the user is resizing the window because
         * it erases the background to white, then
         * paints the list items over it.
         *
         * We will clip the drawing so that it only
         * erases the parts of the list control that
         * show only the background.
         */

        /*
         * Get the device context and save it's state
         * to be restored after we're done
         */
        hDC = (HDC) wParam;
        DcSave = SaveDC(hDC);

        /*
         * Get the background brush
         */
        hbrBackground = (HBRUSH)(LONG_PTR) GetClassLongPtrW(hWnd, GCL_HBRBACKGROUND);

        /*
         * Calculate the clip rect by getting the RECT
         * of the first and last items and adding them up.
         *
         * We also have to get the item's icon RECT and
         * subtract it from our clip rect because we don't
         * use icons in this list control.
         */
        rcClip.left = LVIR_BOUNDS;
        SendMessageW(hWnd, LVM_GETITEMRECT, 0, (LPARAM)&rcClip);
        rcItem.left = LVIR_BOUNDS;
        SendMessageW(hWnd, LVM_GETITEMRECT, ListView_GetItemCount(hWnd) - 1, (LPARAM)&rcItem);
        rcClip.bottom = rcItem.bottom;
        rcClip.right = rcItem.right;
        rcItem.left = LVIR_ICON;
        SendMessageW(hWnd, LVM_GETITEMRECT, 0, (LPARAM)&rcItem);
        rcClip.left = rcItem.right;

        /*
         * Now exclude the clip rect
         */
        ExcludeClipRect(hDC, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

        /*
         * Now erase the background
         *
         *
         * FIXME: Should I erase it myself or
         * pass down the updated HDC and let
         * the default handler do it?
         */
        GetClientRect(hWnd, &rcItem);
        FillRect(hDC, &rcItem, hbrBackground);

        /*
         * Now restore the DC state that we
         * saved earlier
         */
        RestoreDC(hDC, DcSave);

        return TRUE;
    }

    /*
     * We pass on all messages except WM_ERASEBKGND
     */
    return CallWindowProcW(OldProcessListWndProc, hWnd, message, wParam, lParam);
}
