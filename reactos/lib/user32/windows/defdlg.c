/*
 * Default dialog procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *
 */

#include <windows.h>
#include <user32/widgets.h>
#include <user32/dialog.h>
#include <user32/win.h>
#include <user32/paint.h>



/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog.
 */
static void DEFDLG_SetFocus( HWND hwndDlg, HWND hwndCtrl )
{
    HWND hwndPrev = GetFocus();

    if (IsChild( hwndDlg, hwndPrev ))
    {
        if (SendMessage( hwndPrev, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
            SendMessage( hwndPrev, EM_SETSEL, TRUE, MAKELONG( -1, 0 ) );
    }
    if (SendMessage( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessage( hwndCtrl, EM_SETSEL, FALSE, MAKELONG( 0, -1 ) );
    SetFocus( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static BOOL DEFDLG_SaveFocus( HWND hwnd, DIALOGINFO *infoPtr )
{
    HWND hwndFocus = GetFocus();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return FALSE;
    infoPtr->hwndFocus = hwndFocus;
      /* Remove default button */
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static BOOL DEFDLG_RestoreFocus( HWND hwnd, DIALOGINFO *infoPtr )
{
    if (!infoPtr->hwndFocus || IsIconic(hwnd)) return FALSE;
    if (!IsWindow( infoPtr->hwndFocus )) return FALSE;
    DEFDLG_SetFocus( hwnd, infoPtr->hwndFocus );
    /* This used to set infoPtr->hwndFocus to NULL for no apparent reason,
       sometimes losing focus when receiving WM_SETFOCUS messages. */
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_FindDefButton
 *
 * Find the current default push-button.
 */
static HWND DEFDLG_FindDefButton( HWND hwndDlg )
{
    HWND hwndChild = GetWindow( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessage( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}


/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL DEFDLG_SetDefButton( HWND hwndDlg, DIALOGINFO *dlgInfo,
                                   HWND hwndNew )
{
    if (hwndNew &&
        !(SendMessage(hwndNew, WM_GETDLGCODE, 0, 0 ) & DLGC_UNDEFPUSHBUTTON))
        return FALSE;  /* Destination is not a push button */
    
    if (dlgInfo->idResult)  /* There's already a default pushbutton */
    {
        HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->idResult );
        if (SendMessageA( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
            SendMessageA( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );
    }
    if (hwndNew)
    {
        SendMessageA( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
        dlgInfo->idResult = GetDlgCtrlID( hwndNew );
    }
    else dlgInfo->idResult = 0;
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_Proc
 *
 * Implementation of DefDlgProc(). Only handle messages that need special
 * handling for dialogs.
 */
LRESULT DEFDLG_Proc( HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam, DIALOGINFO *dlgInfo )
{
    switch(msg)
    {
        case WM_ERASEBKGND:
	    FillWindow( hwnd, hwnd, (HDC)wParam, (HBRUSH)CTLCOLOR_DLG );
	    return 1;

	case WM_NCDESTROY:

	      /* Free dialog heap (if created) */
	    if (dlgInfo->hDialogHeap)
	    {
		GlobalUnlock(dlgInfo->hDialogHeap);
		GlobalFree(dlgInfo->hDialogHeap);
		dlgInfo->hDialogHeap = 0;
	    }

	      /* Delete font */
	    if (dlgInfo->hUserFont)
	    {
		DeleteObject( dlgInfo->hUserFont );
		dlgInfo->hUserFont = 0;
	    }

	      /* Delete menu */
	    if (dlgInfo->hMenu)
	    {		
		DestroyMenu( dlgInfo->hMenu );
		dlgInfo->hMenu = 0;
	    }

            /* Delete window procedure */
            //WINPROC_FreeProc( dlgInfo->dlgProc, WIN_PROC_WINDOW );
            dlgInfo->dlgProc = NULL;
            dlgInfo->flags |= DF_END;  /* just in case */

	      /* Window clean-up */
	    return DefWindowProcA( hwnd, msg, wParam, lParam );

	case WM_SHOWWINDOW:
	    if (!wParam) DEFDLG_SaveFocus( hwnd, dlgInfo );
	    return DefWindowProcA( hwnd, msg, wParam, lParam );

	case WM_ACTIVATE:
	    if (wParam) DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    else DEFDLG_SaveFocus( hwnd, dlgInfo );
	    return 0;

	case WM_SETFOCUS:
	    DEFDLG_RestoreFocus( hwnd, dlgInfo );
	    return 0;

        case DM_SETDEFID:
            if (dlgInfo->flags & DF_END) return 1;
            DEFDLG_SetDefButton( hwnd, dlgInfo,
                                 wParam ? GetDlgItem( hwnd, wParam ) : 0 );
            return 1;

        case DM_GETDEFID:
            {
                HWND hwndDefId;
                if (dlgInfo->flags & DF_END) return 0;
                if (dlgInfo->idResult)
                    return MAKELONG( dlgInfo->idResult, DC_HASDEFID );
                if ((hwndDefId = DEFDLG_FindDefButton( hwnd )))
                    return MAKELONG( GetDlgCtrlID( hwndDefId ), DC_HASDEFID);
            }
	    return 0;

	case WM_NEXTDLGCTL:
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
                    WND *wnd = WIN_FindWndPtr( hwndFocus );

                    if( wnd )
                    {
                        /* always make combo box hide its listbox control */

                        if( WIDGETS_IsControl( wnd, BIC_COMBO ) )
                            SendMessageA( hwndFocus, CB_SHOWDROPDOWN,
                                            FALSE, 0 );
                        else if( WIDGETS_IsControl( wnd, BIC_EDIT ) &&
                                 WIDGETS_IsControl( wnd->parent,
                                                      BIC_COMBO ))
                            SendMessageA( wnd->parent->hwndSelf, 
                                            CB_SHOWDROPDOWN, FALSE, 0 );
                    }
                }
            }
	    return DefWindowProcA( hwnd, msg, wParam, lParam );

	case WM_GETFONT: 
	    return dlgInfo->hUserFont;

        case WM_CLOSE:
            PostMessageA( hwnd, WM_COMMAND, IDCANCEL,
                            (LPARAM)GetDlgItem( hwnd, IDCANCEL ) );
            return 0;
    }
    return 0;
}

/***********************************************************************
 *           DEFDLG_Epilog
 */
LRESULT DEFDLG_Epilog(DIALOGINFO* dlgInfo, UINT msg, BOOL fResult)
{
    /* see SDK 3.1 */

    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
	 msg == WM_CTLCOLOR || msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return fResult; 

    return dlgInfo->msgResult;
}




/***********************************************************************
 *           DefDlgProcA   (USER.120)
 */
LRESULT WINAPI DefDlgProcA( HWND hwnd, UINT msg,
                              WPARAM wParam, LPARAM lParam )
{

    DIALOGINFO * dlgInfo;
    BOOL result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = 0;

    if (dlgInfo->dlgProc) {      /* Call dialog procedure */
        result = CallWindowProcA( (WNDPROC)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );

        /* Check if window was destroyed by dialog procedure */
        if (dlgInfo->flags & DF_END && !(dlgInfo->flags & DF_ENDING)) {
            dlgInfo->flags |= DF_ENDING;
            DestroyWindow( hwnd );
        }
    }

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
                 return DEFDLG_Proc( (HWND)hwnd, msg,
                                     (WPARAM)wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcA( hwnd, msg, wParam, lParam );
        }
    }

    return DEFDLG_Epilog(dlgInfo, msg, result);

    
}


/***********************************************************************
 *           DefDlgProcW   (USER.121)
 */
LRESULT WINAPI DefDlgProcW( HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam )
{
    DIALOGINFO * dlgInfo;
    BOOL result = FALSE;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = 0;

    if (dlgInfo->dlgProc) {      /* Call dialog procedure */
        result = CallWindowProcW( (WNDPROC)dlgInfo->dlgProc,
                                            hwnd, msg, wParam, lParam );
 
        /* Check if window was destroyed by dialog procedure */
        if (dlgInfo->flags & DF_END && !(dlgInfo->flags & DF_ENDING)) {
            dlgInfo->flags |= DF_ENDING;
            DestroyWindow( hwnd );
        }
    }

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
                 return DEFDLG_Proc( (HWND)hwnd, msg,
                                     (WPARAM)wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcW( hwnd, msg, wParam, lParam );
        }
    }
    return DEFDLG_Epilog(dlgInfo, msg, result);
}
