#include <windows.h>
#include <user32/win.h>
#include <user32/dialog.h>
#include <user32/debug.h>
/***********************************************************************
 *           CreateDialog   (USER32.89)
 */
#undef CreateDialogA
HWND STDCALL CreateDialogA( HINSTANCE hInst, LPCSTR dlgTemplate,
                              HWND owner, DLGPROC dlgProc )
{
    return CreateDialogParamA( hInst, dlgTemplate, owner, dlgProc, 0 );
}

/***********************************************************************
 *           CreateDialog   (USER32.89)
 */

#undef CreateDialogW
HWND STDCALL CreateDialogW( HINSTANCE hInst, LPCWSTR dlgTemplate,
                              HWND owner, DLGPROC dlgProc )
{
    return CreateDialogParamW( hInst, dlgTemplate, owner, dlgProc, 0 );
}



/***********************************************************************
 *           CreateDialogParamA   (USER32.73)
 */
HWND STDCALL CreateDialogParamA( HINSTANCE hInst, LPCSTR name,
                                    HWND owner, DLGPROC dlgProc,
                                    LPARAM param )
{
    HANDLE hrsrc = FindResourceA( hInst, name, RT_DIALOG );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParamA( hInst,
                                         (LPVOID)LoadResource(hInst, hrsrc),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogParamW   (USER32.74)
 */
HWND STDCALL CreateDialogParamW( HINSTANCE hInst, LPCWSTR name,
                                    HWND owner, DLGPROC dlgProc,
                                    LPARAM param )
{
    HANDLE hrsrc = FindResourceW( hInst, name, (LPCWSTR)RT_DIALOG );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParamW( hInst,
                                         (LPVOID)LoadResource(hInst, hrsrc),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogIndirect   (USER32.219)
 */
#undef CreateDialogIndirectA
HWND STDCALL CreateDialogIndirectA( HINSTANCE hInst, LPCDLGTEMPLATE dlgTemplate,
                                      HWND owner, DLGPROC dlgProc )
{
    return CreateDialogIndirectParamA( hInst, dlgTemplate, owner, dlgProc, 0);
}

/***********************************************************************
 *           CreateDialogIndirect   (USER32.219)
 */
#undef CreateDialogIndirectW
HWND STDCALL CreateDialogIndirectW( HINSTANCE hInst, LPCDLGTEMPLATE dlgTemplate,
                                      HWND owner, DLGPROC dlgProc )
{
    return CreateDialogIndirectParamW( hInst, dlgTemplate, owner, dlgProc, 0);
}


/***********************************************************************
 *           CreateDialogIndirectParamA   (USER32.69)
 */
HWND STDCALL CreateDialogIndirectParamA( HINSTANCE hInst,
                                            LPCDLGTEMPLATE dlgTemplate,
                                            HWND owner, DLGPROC dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate,  owner,
                                  (DLGPROC)dlgProc, param, FALSE );
}


/***********************************************************************
 *           CreateDialogIndirectParamW   (USER32.72)
 */
HWND STDCALL CreateDialogIndirectParamW( HINSTANCE hInst,
                                            LPCDLGTEMPLATE dlgTemplate,
                                            HWND owner, DLGPROC dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, owner,
                                  (DLGPROC)dlgProc, param, TRUE );
}


/***********************************************************************
 *           DialogBox   (USER32.87)
 */

#undef DialogBoxA
INT STDCALL DialogBoxA( HINSTANCE hInst, LPCSTR dlgTemplate,
                          HWND owner, DLGPROC dlgProc )
{
    return DialogBoxParamA( hInst, dlgTemplate, owner, dlgProc, 0 );
}

/***********************************************************************
 *           DialogBox   (USER32.87)
 */
#undef DialogBoxW
INT STDCALL DialogBoxW( HINSTANCE hInst, LPCWSTR dlgTemplate,
                          HWND owner, DLGPROC dlgProc )
{
    return DialogBoxParamW( hInst, dlgTemplate, owner, dlgProc, 0 );
}




/***********************************************************************
 *           DialogBoxParamA   (USER32.139)
 */
INT STDCALL DialogBoxParamA( HINSTANCE hInst, LPCSTR name,
                                HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd = CreateDialogParamA( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParamW   (USER32.140)
 */
INT STDCALL DialogBoxParamW( HINSTANCE hInst, LPCWSTR name,
                                HWND owner, DLGPROC dlgProc, LPARAM param )
{
    HWND hwnd = CreateDialogParamW( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect   (USER32.218)
 */
#undef DialogBoxIndirectA
INT STDCALL DialogBoxIndirectA( HINSTANCE hInst, LPCDLGTEMPLATE dlgTemplate,
                                  HWND owner, DLGPROC dlgProc )
{
    return DialogBoxIndirectParamA( hInst, dlgTemplate, owner, dlgProc, 0 );
}

/***********************************************************************
 *           DialogBoxIndirect   (USER32.218)
 */
#undef DialogBoxIndirectW
INT STDCALL DialogBoxIndirectW( HINSTANCE hInst, LPCDLGTEMPLATE dlgTemplate,
                                  HWND owner, DLGPROC dlgProc )
{
    return DialogBoxIndirectParam( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParamA   (USER32.136)
 */
INT STDCALL DialogBoxIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATE template,
                                       HWND owner, DLGPROC dlgProc,
                                       LPARAM param )
{
    HWND hwnd = CreateDialogIndirectParamA( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParamW   (USER32.138)
 */
INT STDCALL DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE template,
                                       HWND owner, DLGPROC dlgProc,
                                       LPARAM param )
{
    HWND hwnd = CreateDialogIndirectParamW( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog   (USER32.88)
 */
WINBOOL STDCALL EndDialog( HWND hwnd, INT retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;

    DPRINT( "%04x %d\n", hwnd, retval );

    if( dlgInfo )
    {
        dlgInfo->idResult = retval;
        dlgInfo->flags |= DF_END;
    }
    return TRUE;
}


/***********************************************************************
 *           IsDialogMessageA   (USER32.342)
 */
WINBOOL STDCALL IsDialogMessageA( HWND hwndDlg, LPMSG msg )
{
    WINBOOL ret, translate, dispatch;
    INT dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessageA( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage( msg );
    if (dispatch) DispatchMessageA( msg );
    return ret;
}


/***********************************************************************
 *           IsDialogMessageW   (USER32.343)
 */
WINBOOL STDCALL IsDialogMessageW( HWND hwndDlg, LPMSG msg )
{
    WINBOOL ret, translate, dispatch;
    INT dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessageW( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage( msg );
    if (dispatch) DispatchMessageW( msg );
    return ret;
}


 

/****************************************************************
 *         GetDlgCtrlID   (USER32.234)
 */
INT STDCALL GetDlgCtrlID( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 
HWND STDCALL GetDlgItem(HWND  hDlg, int  nIDDlgItem )
{
}




/*******************************************************************
 *           SendDlgItemMessageA   (USER32.452)
 */
LRESULT STDCALL SendDlgItemMessageA( HWND hwnd, INT id, UINT msg,
                                      WPARAM wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessageA( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessageW   (USER32.453)
 */
LRESULT STDCALL SendDlgItemMessageW( HWND hwnd, INT id, UINT msg,
                                      WPARAM wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessageW( hwndCtrl, msg, wParam, lParam );
    else return 0;
}



/*******************************************************************
 *           SetDlgItemTextA   (USER32.478)
 */
WINBOOL STDCALL SetDlgItemTextA( HWND hwnd, INT id, LPCSTR lpString )
{
    return SendDlgItemMessageA( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemTextW   (USER32.479)
 */
WINBOOL STDCALL SetDlgItemTextW( HWND hwnd, INT id, LPCWSTR lpString )
{
    return SendDlgItemMessageW( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}




/***********************************************************************
 *           GetDlgItemTextA   (USER32.237)
 */
UINT STDCALL GetDlgItemTextA( HWND hwnd, INT id, LPSTR str, INT len )
{
    return (UINT)SendDlgItemMessageA( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemTextW   (USER32.238)
 */
UINT STDCALL GetDlgItemTextW( HWND hwnd, INT id, LPWSTR str, INT len )
{
    return (UINT)SendDlgItemMessageW( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}




/*******************************************************************
 *           SetDlgItemInt   (USER32.477)
 */
WINBOOL STDCALL SetDlgItemInt( HWND hwnd, INT id, UINT value,
                             WINBOOL fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessageA( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
}




/***********************************************************************
 *           GetDlgItemInt   (USER32.236)
 */
UINT STDCALL GetDlgItemInt( HWND hwnd, INT id, WINBOOL *translated,
                               WINBOOL fSigned )
{
    char str[30];
    char * endptr;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!SendDlgItemMessageA(hwnd, id, WM_GETTEXT, sizeof(str), (LPARAM)str))
        return 0;
    if (fSigned)
    {
        result = strtol( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if (((result == LONG_MIN) || (result == LONG_MAX)) ) {
		// errno == ERANGE
            return 0;
	}
    }
    else
    {
        result = strtoul( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if ((result == ULONG_MAX) ) {
	//	&& (errno == ERANGE)
		return 0;
	}
    }
    if (translated) *translated = TRUE;
    return (UINT)result;
}


/***********************************************************************
 *           CheckDlgButton   (USER32.97)
 */
BOOL STDCALL CheckDlgButton( HWND hwnd, INT id, UINT check )
{
    SendDlgItemMessageW( hwnd, id, BM_SETCHECK, check, 0 );
    return TRUE;
}





/***********************************************************************
 *           IsDlgButtonChecked   (USER32.98)
 */
UINT STDCALL IsDlgButtonChecked( HWND hwnd, INT id )
{
    return (UINT)SendDlgItemMessageA( hwnd, id, BM_GETCHECK, 0, 0 );
}




/***********************************************************************
 *           CheckRadioButton   (USER32.48)
 */
WINBOOL STDCALL CheckRadioButton( HWND hwndDlg, INT firstID,
                                  INT lastID, INT checkID )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    if (!pWnd) return FALSE;

    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if ((pWnd->wIDmenu == firstID) || (pWnd->wIDmenu == lastID)) break;
    if (!pWnd) return FALSE;

    if (pWnd->wIDmenu == lastID)
        lastID = firstID;  /* Buttons are in reverse order */
    while (pWnd)
    {
	SendMessageA( pWnd->hwndSelf, BM_SETCHECK,
                        (pWnd->wIDmenu == checkID), 0 );
        if (pWnd->wIDmenu == lastID) break;
	pWnd = pWnd->next;
    }
    return TRUE;
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER32.243) (USER32.233)
 */
LONG STDCALL GetDialogBaseUnits(void)
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect   (USER32.103)
 */
WINBOOL STDCALL MapDialogRect( HWND hwnd, LPRECT rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
    return TRUE;
}








/***********************************************************************
 *           GetNextDlgGroupItem   (USER32.275)
 */
HWND STDCALL GetNextDlgGroupItem( HWND hwndDlg, HWND hwndCtrl,
                                     WINBOOL fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (hwndCtrl)
    {
        if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
        /* Make sure hwndCtrl is a top-level child */
        while ((pWndCtrl->dwStyle & WS_CHILD) && (pWndCtrl->parent != pWndDlg))
            pWndCtrl = pWndCtrl->parent;
        if (pWndCtrl->parent != pWndDlg) return 0;
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(pWndCtrl = pWndDlg->child)) return 0;
        if (fPrevious) while (pWndCtrl->next) pWndCtrl = pWndCtrl->next;
    }

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd || (pWnd->dwStyle & WS_GROUP))
        {
            /* Wrap-around to the beginning of the group */
            WND *pWndStart = pWndDlg->child;
            for (pWnd = pWndStart; pWnd; pWnd = pWnd->next)
            {
                if (pWnd->dwStyle & WS_GROUP) pWndStart = pWnd;
                if (pWnd == pWndCtrl) break;
            }
            pWnd = pWndStart;
        }
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_VISIBLE) && !(pWnd->dwStyle & WS_DISABLED))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}





/***********************************************************************
 *           GetNextDlgTabItem   (USER32.276)
 */
HWND STDCALL GetNextDlgTabItem( HWND hwndDlg, HWND hwndCtrl,
                                   WINBOOL fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (hwndCtrl)
    {
        if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
        /* Make sure hwndCtrl is a top-level child */
        while ((pWndCtrl->dwStyle & WS_CHILD) && (pWndCtrl->parent != pWndDlg))
            pWndCtrl = pWndCtrl->parent;
        if (pWndCtrl->parent != pWndDlg) return 0;
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(pWndCtrl = pWndDlg->child)) return 0;
        if (!fPrevious) while (pWndCtrl->next) pWndCtrl = pWndCtrl->next;
    }

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd) pWnd = pWndDlg->child;
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_TABSTOP) && (pWnd->dwStyle & WS_VISIBLE) &&
            !(pWnd->dwStyle & WS_DISABLED))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}

