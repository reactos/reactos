/*
 * Default dialog procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dialog);


/***********************************************************************
 *           DEFDLG_GetDlgProc
 */
static DLGPROC DEFDLG_GetDlgProc( HWND hwnd )
{
    DLGPROC ret;
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (!wndPtr) return 0;
    if (wndPtr == WND_OTHER_PROCESS)
    {
        ERR( "cannot get dlg proc %p from other process\n", hwnd );
        return 0;
    }
    ret = *(DLGPROC *)((char *)wndPtr->wExtra + DWLP_DLGPROC);
    WIN_ReleasePtr( wndPtr );
    return ret;
}

/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog.
 */
static void DEFDLG_SetFocus( HWND hwndDlg, HWND hwndCtrl )
{
    if (SendMessageW( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessageW( hwndCtrl, EM_SETSEL, 0, -1 );
    SetFocus( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static void DEFDLG_SaveFocus( HWND hwnd )
{
    DIALOGINFO *infoPtr;
    HWND hwndFocus = GetFocus();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return;
    if (!(infoPtr = DIALOG_get_info( hwnd, FALSE ))) return;
    infoPtr->hwndFocus = hwndFocus;
    /* Remove default button */
}


/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static void DEFDLG_RestoreFocus( HWND hwnd )
{
    DIALOGINFO *infoPtr;

    if (IsIconic( hwnd )) return;
    if (!(infoPtr = DIALOG_get_info( hwnd, FALSE ))) return;
    /* Don't set the focus back to controls if EndDialog is already called.*/
    if (infoPtr->flags & DF_END) return;
    if (!IsWindow(infoPtr->hwndFocus) || infoPtr->hwndFocus == hwnd) {
        /* If no saved focus control exists, set focus to the first visible,
           non-disabled, WS_TABSTOP control in the dialog */
        infoPtr->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );
        if (!IsWindow( infoPtr->hwndFocus )) return;
    }
    DEFDLG_SetFocus( hwnd, infoPtr->hwndFocus );

    /* This used to set infoPtr->hwndFocus to NULL for no apparent reason,
       sometimes losing focus when receiving WM_SETFOCUS messages. */
}


/***********************************************************************
 *           DEFDLG_FindDefButton
 *
 * Find the current default push-button.
 */
static HWND DEFDLG_FindDefButton( HWND hwndDlg )
{
    HWND hwndChild, hwndTmp;

    hwndChild = GetWindow( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessageW( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;

        /* Recurse into WS_EX_CONTROLPARENT controls */
        if (GetWindowLongW( hwndChild, GWL_EXSTYLE ) & WS_EX_CONTROLPARENT)
        {
            LONG dsStyle = GetWindowLongW( hwndChild, GWL_STYLE );
            if ((dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED) &&
                (hwndTmp = DEFDLG_FindDefButton(hwndChild)) != NULL)
           return hwndTmp;
        }
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}


/***********************************************************************
 *           DEFDLG_SetDefId
 *
 * Set the default button id.
 */
static BOOL DEFDLG_SetDefId( HWND hwndDlg, DIALOGINFO *dlgInfo, WPARAM wParam)
{
    DWORD dlgcode=0; /* initialize just to avoid a warning */
    HWND hwndOld, hwndNew = GetDlgItem(hwndDlg, wParam);
    INT old_id = dlgInfo->idResult;

    dlgInfo->idResult = wParam;
    if (hwndNew &&
        !((dlgcode=SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 ))
            & (DLGC_UNDEFPUSHBUTTON | DLGC_BUTTON)))
        return FALSE;  /* Destination is not a push button */

    /* Make sure the old default control is a valid push button ID */
    hwndOld = GetDlgItem( hwndDlg, old_id );
    if (!hwndOld || !(SendMessageW( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON))
        hwndOld = DEFDLG_FindDefButton( hwndDlg );
    if (hwndOld && hwndOld != hwndNew)
        SendMessageW( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );

    if (hwndNew)
    {
        if(dlgcode & DLGC_UNDEFPUSHBUTTON)
            SendMessageW( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
    }
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL DEFDLG_SetDefButton( HWND hwndDlg, DIALOGINFO *dlgInfo, HWND hwndNew )
{
    DWORD dlgcode=0; /* initialize just to avoid a warning */
    HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->idResult );

    if (hwndNew &&
        !((dlgcode=SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 ))
            & (DLGC_UNDEFPUSHBUTTON | DLGC_DEFPUSHBUTTON)))
    {
        /**
         * Need to draw only default push button rectangle.
         * Since the next control is not a push button, need to draw the push
         * button rectangle for the default control.
         */
        hwndNew = hwndOld;
        dlgcode = SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 );
    }

    /* Make sure the old default control is a valid push button ID */
    if (!hwndOld || !(SendMessageW( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON))
        hwndOld = DEFDLG_FindDefButton( hwndDlg );
    if (hwndOld && hwndOld != hwndNew)
        SendMessageW( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );

    if (hwndNew)
    {
        if(dlgcode & DLGC_UNDEFPUSHBUTTON)
            SendMessageW( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
    }
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_Proc
 *
 * Implementation of DefDlgProc(). Only handle messages that need special
 * handling for dialogs.
 */
static LRESULT DEFDLG_Proc( HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam, DIALOGINFO *dlgInfo )
{
    switch(msg)
    {
        case WM_ERASEBKGND:
        {
            HBRUSH brush = (HBRUSH)SendMessageW( hwnd, WM_CTLCOLORDLG, wParam, (LPARAM)hwnd );
            if (!brush) brush = (HBRUSH)DefWindowProcW( hwnd, WM_CTLCOLORDLG, wParam, (LPARAM)hwnd );
            if (brush)
            {
                RECT rect;
                HDC hdc = (HDC)wParam;
                GetClientRect( hwnd, &rect );
                DPtoLP( hdc, (LPPOINT)&rect, 2 );
                FillRect( hdc, &rect, brush );
            }
            return 1;
        }
        case WM_NCDESTROY:
            if (dlgInfo)
            {
                WND *wndPtr;

                if (dlgInfo->hUserFont) DeleteObject( dlgInfo->hUserFont );
                if (dlgInfo->hMenu) DestroyMenu( dlgInfo->hMenu );
                HeapFree( GetProcessHeap(), 0, dlgInfo );

                wndPtr = WIN_GetPtr( hwnd );
                wndPtr->dlgInfo = NULL;
                WIN_ReleasePtr( wndPtr );
            }
              /* Window clean-up */
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_SHOWWINDOW:
            if (!wParam) DEFDLG_SaveFocus( hwnd );
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_ACTIVATE:
            if (wParam) DEFDLG_RestoreFocus( hwnd );
            else DEFDLG_SaveFocus( hwnd );
            return 0;

        case WM_SETFOCUS:
            DEFDLG_RestoreFocus( hwnd );
            return 0;

        case DM_SETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
                DEFDLG_SetDefId( hwnd, dlgInfo, wParam );
            return 1;

        case DM_GETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
            {
                HWND hwndDefId;
                if (dlgInfo->idResult)
                    return MAKELONG( dlgInfo->idResult, DC_HASDEFID );
                if ((hwndDefId = DEFDLG_FindDefButton( hwnd )))
                    return MAKELONG( GetDlgCtrlID( hwndDefId ), DC_HASDEFID);
            }
            return 0;

        case WM_NEXTDLGCTL:
            if (dlgInfo)
            {
                HWND hwndDest = (HWND)wParam;
                if (!lParam)
                    hwndDest = GetNextDlgTabItem(hwnd, GetFocus(), wParam);
                if (hwndDest) DEFDLG_SetFocus( hwnd, hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            return 0;

        case WM_ENTERMENULOOP:
        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            {
                HWND hwndFocus = GetFocus();
                if (hwndFocus)
                {
                    /* always make combo box hide its listbox control */
                    if (!SendMessageW( hwndFocus, CB_SHOWDROPDOWN, FALSE, 0 ))
                        SendMessageW( GetParent(hwndFocus), CB_SHOWDROPDOWN, FALSE, 0 );
                }
            }
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_GETFONT:
            return dlgInfo ? (LRESULT)dlgInfo->hUserFont : 0;

        case WM_CLOSE:
            PostMessageA( hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED),
                            (LPARAM)GetDlgItem( hwnd, IDCANCEL ) );
            return 0;
    }
    return 0;
}

/***********************************************************************
*               DIALOG_get_info
*
* Get the DIALOGINFO structure of a window, allocating it if needed
* and 'create' is TRUE.
*/
DIALOGINFO *DIALOG_get_info( HWND hwnd, BOOL create )
{
    WND* wndPtr;
    DIALOGINFO* dlgInfo;

    wndPtr = WIN_GetPtr( hwnd );
    if (!wndPtr || wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return NULL;
    }

    dlgInfo = wndPtr->dlgInfo;

    if (!dlgInfo && create)
    {
        if (!(dlgInfo = HeapAlloc( GetProcessHeap(), 0, sizeof(*dlgInfo) )))
            goto out;
        dlgInfo->hwndFocus   = 0;
        dlgInfo->hUserFont   = 0;
        dlgInfo->hMenu       = 0;
        dlgInfo->xBaseUnit   = 0;
        dlgInfo->yBaseUnit   = 0;
        dlgInfo->idResult    = 0;
        dlgInfo->flags       = 0;
        wndPtr->dlgInfo = dlgInfo;
    }

out:
    WIN_ReleasePtr( wndPtr );
    return dlgInfo;
}

/***********************************************************************
 *              DefDlgProcA (USER32.@)
 */
LRESULT WINAPI DefDlgProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    DIALOGINFO *dlgInfo;
    DLGPROC dlgproc;
    LRESULT result = 0;

    /* Perform DIALOGINFO initialization if not done */
    if(!(dlgInfo = DIALOG_get_info( hwnd, TRUE ))) return 0;

    SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, 0 );

    if ((dlgproc = DEFDLG_GetDlgProc( hwnd ))) /* Call dialog procedure */
        result = WINPROC_CallDlgProcA( dlgproc, hwnd, msg, wParam, lParam );

    if (!result && IsWindow(hwnd))
    {
        /* callback didn't process this message */

        switch(msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hwnd, msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcA( hwnd, msg, wParam, lParam );
        }
    }

    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
         msg == WM_CTLCOLOR || msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return result;

    return GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );
}


/***********************************************************************
 *              DefDlgProcW (USER32.@)
 */
LRESULT WINAPI DefDlgProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    DIALOGINFO *dlgInfo;
    DLGPROC dlgproc;
    LRESULT result = 0;

    /* Perform DIALOGINFO initialization if not done */
    if(!(dlgInfo = DIALOG_get_info( hwnd, TRUE ))) return 0;

    SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, 0 );

    if ((dlgproc = DEFDLG_GetDlgProc( hwnd ))) /* Call dialog procedure */
        result = WINPROC_CallDlgProcW( dlgproc, hwnd, msg, wParam, lParam );

    if (!result && IsWindow(hwnd))
    {
        /* callback didn't process this message */

        switch(msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hwnd, msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcW( hwnd, msg, wParam, lParam );
        }
    }

    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
         msg == WM_CTLCOLOR || msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return result;

    return GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );
}
