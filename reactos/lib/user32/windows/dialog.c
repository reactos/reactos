/***********************************************************************
 *           CreateDialog16   (USER.89)
 */
HWND16 WINAPI CreateDialog16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                              HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam16   (USER.241)
 */
HWND16 WINAPI CreateDialogParam16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                                   HWND16 owner, DLGPROC16 dlgProc,
                                   LPARAM param )
{
    HWND16 hwnd = 0;
    HRSRC16 hRsrc;
    HGLOBAL16 hmem;
    LPCVOID data;

    TRACE(dialog, "%04x,%08lx,%04x,%08lx,%ld\n",
                   hInst, (DWORD)dlgTemplate, owner, (DWORD)dlgProc, param );

    if (!(hRsrc = FindResource16( hInst, dlgTemplate, RT_DIALOG16 ))) return 0;
    if (!(hmem = LoadResource16( hInst, hRsrc ))) return 0;
    if (!(data = LockResource16( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam16( hInst, data, owner,
                                             dlgProc, param );
    FreeResource16( hmem );
    return hwnd;
}

/***********************************************************************
 *           CreateDialogParam32A   (USER32.73)
 */
HWND32 WINAPI CreateDialogParam32A( HINSTANCE32 hInst, LPCSTR name,
                                    HWND32 owner, DLGPROC32 dlgProc,
                                    LPARAM param )
{
    if (HIWORD(name))
    {
        LPWSTR str = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
        HWND32 hwnd = CreateDialogParam32W( hInst, str, owner, dlgProc, param);
        HeapFree( GetProcessHeap(), 0, str );
        return hwnd;
    }
    return CreateDialogParam32W( hInst, (LPCWSTR)name, owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogParam32W   (USER32.74)
 */
HWND32 WINAPI CreateDialogParam32W( HINSTANCE32 hInst, LPCWSTR name,
                                    HWND32 owner, DLGPROC32 dlgProc,
                                    LPARAM param )
{
    HANDLE32 hrsrc = FindResource32W( hInst, name, RT_DIALOG32W );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParam32W( hInst,
                                         (LPVOID)LoadResource32(hInst, hrsrc),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogIndirect16   (USER.219)
 */
HWND16 WINAPI CreateDialogIndirect16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                      HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0);
}


/***********************************************************************
 *           CreateDialogIndirectParam16   (USER.242)
 */
HWND16 WINAPI CreateDialogIndirectParam16( HINSTANCE16 hInst,
                                           LPCVOID dlgTemplate,
                                           HWND16 owner, DLGPROC16 dlgProc,
                                           LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, FALSE, owner,
                                  dlgProc, param, WIN_PROC_16 );
}


/***********************************************************************
 *           CreateDialogIndirectParam32A   (USER32.69)
 */
HWND32 WINAPI CreateDialogIndirectParam32A( HINSTANCE32 hInst,
                                            LPCVOID dlgTemplate,
                                            HWND32 owner, DLGPROC32 dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32A );
}

/***********************************************************************
 *           CreateDialogIndirectParam32AorW   (USER32.71)
 */
HWND32 WINAPI CreateDialogIndirectParam32AorW( HINSTANCE32 hInst,
                                            LPCVOID dlgTemplate,
                                            HWND32 owner, DLGPROC32 dlgProc,
                                            LPARAM param )
{   FIXME(dialog,"assume WIN_PROC_32W\n");
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32W );
}

/***********************************************************************
 *           CreateDialogIndirectParam32W   (USER32.72)
 */
HWND32 WINAPI CreateDialogIndirectParam32W( HINSTANCE32 hInst,
                                            LPCVOID dlgTemplate,
                                            HWND32 owner, DLGPROC32 dlgProc,
                                            LPARAM param )
{
    return DIALOG_CreateIndirect( hInst, dlgTemplate, TRUE, owner,
                                  (DLGPROC16)dlgProc, param, WIN_PROC_32W );
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT32 DIALOG_DoDialogBox( HWND32 hwnd, HWND32 owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    MSG16 msg;
    INT32 retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    EnableWindow32( owner, FALSE );
    ShowWindow32( hwnd, SW_SHOW );

    while (MSG_InternalGetMessage(&msg, hwnd, owner, MSGF_DIALOGBOX, PM_REMOVE,
                                  !(wndPtr->dwStyle & DS_NOIDLEMSG) ))
    {
	if (!IsDialogMessage16( hwnd, &msg))
	{
	    TranslateMessage16( &msg );
	    DispatchMessage16( &msg );
	}
	if (dlgInfo->flags & DF_END) break;
    }
    retval = dlgInfo->idResult;
    EnableWindow32( owner, TRUE );
    dlgInfo->flags |= DF_ENDING;   /* try to stop it being destroyed twice */
    DestroyWindow32( hwnd );
    return retval;
}


/***********************************************************************
 *           DialogBox16   (USER.87)
 */
INT16 WINAPI DialogBox16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                          HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam16   (USER.239)
 */
INT16 WINAPI DialogBoxParam16( HINSTANCE16 hInst, SEGPTR template,
                               HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd = CreateDialogParam16( hInst, template, owner, dlgProc, param);
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParam32A   (USER32.139)
 */
INT32 WINAPI DialogBoxParam32A( HINSTANCE32 hInst, LPCSTR name,
                                HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32A( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParam32W   (USER32.140)
 */
INT32 WINAPI DialogBoxParam32W( HINSTANCE32 hInst, LPCWSTR name,
                                HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32W( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect16   (USER.218)
 */
INT16 WINAPI DialogBoxIndirect16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                  HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam16   (USER.240)
 */
INT16 WINAPI DialogBoxIndirectParam16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                       HWND16 owner, DLGPROC16 dlgProc,
                                       LPARAM param )
{
    HWND16 hwnd;
    LPCVOID ptr;

    if (!(ptr = GlobalLock16( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam16( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock16( dlgTemplate );
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParam32A   (USER32.136)
 */
INT32 WINAPI DialogBoxIndirectParam32A(HINSTANCE32 hInstance, LPCVOID template,
                                       HWND32 owner, DLGPROC32 dlgProc,
                                       LPARAM param )
{
    HWND32 hwnd = CreateDialogIndirectParam32A( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParam32W   (USER32.138)
 */
INT32 WINAPI DialogBoxIndirectParam32W(HINSTANCE32 hInstance, LPCVOID template,
                                       HWND32 owner, DLGPROC32 dlgProc,
                                       LPARAM param )
{
    HWND32 hwnd = CreateDialogIndirectParam32W( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog16   (USER32.173)
 */
BOOL16 WINAPI EndDialog16( HWND16 hwnd, INT16 retval )
{
    return EndDialog32( hwnd, retval );
}


/***********************************************************************
 *           EndDialog32   (USER.88)
 */
WINBOOL WINAPI EndDialog32( HWND32 hwnd, INT32 retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;

    TRACE(dialog, "%04x %d\n", hwnd, retval );

    if( dlgInfo )
    {
        dlgInfo->idResult = retval;
        dlgInfo->flags |= DF_END;
    }
    return TRUE;
}


/***********************************************************************
 *           DIALOG_IsAccelerator
 */
static WINBOOL DIALOG_IsAccelerator( HWND32 hwnd, HWND32 hwndDlg, WPARAM32 vKey )
{
    HWND32 hwndControl = hwnd;
    HWND32 hwndNext;
    WND *wndPtr;
    WINBOOL RetVal = FALSE;
    INT32 dlgCode;

    if (vKey == VK_SPACE)
    {
        dlgCode = SendMessage32A( hwndControl, WM_GETDLGCODE, 0, 0 );
        if (dlgCode & DLGC_BUTTON)
        {
            SendMessage32A( hwndControl, WM_LBUTTONDOWN, 0, 0);
            SendMessage32A( hwndControl, WM_LBUTTONUP, 0, 0);
            RetVal = TRUE;
        }
    }
    else
    {
        do
        {
            wndPtr = WIN_FindWndPtr( hwndControl );
            if (wndPtr != NULL && wndPtr->text != NULL && 
                    (wndPtr->dwStyle & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)
            {
                dlgCode = SendMessage32A( hwndControl, WM_GETDLGCODE, 0, 0 );
                if (dlgCode & (DLGC_BUTTON | DLGC_STATIC))
                {
                    /* find the accelerator key */
                    LPSTR p = wndPtr->text - 2;
                    do
                    {
                        p = strchr( p + 2, '&' );
                    }
                    while (p != NULL && p[1] == '&');

                    /* and check if it's the one we're looking for */
                    if (p != NULL && toupper( p[1] ) == toupper( vKey ) )
                    {
                        if ((dlgCode & DLGC_STATIC) || 
                            (wndPtr->dwStyle & 0x0f) == BS_GROUPBOX )
                        {
                            /* set focus to the control */
                            SendMessage32A( hwndDlg, WM_NEXTDLGCTL,
                                    hwndControl, 1);
                            /* and bump it on to next */
                            SendMessage32A( hwndDlg, WM_NEXTDLGCTL, 0, 0);
                        }
                        else if (dlgCode & 
			    (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))
                        {
                            /* send command message as from the control */
                            SendMessage32A( hwndDlg, WM_COMMAND, 
                                MAKEWPARAM( LOWORD(wndPtr->wIDmenu), 
                                    BN_CLICKED ),
                                (LPARAM)hwndControl );
                        }
                        else
                        {
                            /* click the control */
                            SendMessage32A( hwndControl, WM_LBUTTONDOWN, 0, 0);
                            SendMessage32A( hwndControl, WM_LBUTTONUP, 0, 0);
                        }
                        RetVal = TRUE;
                        break;
                    }
                }
            }
	    hwndNext = GetWindow32( hwndControl, GW_CHILD );
	    if (!hwndNext)
	    {
	        hwndNext = GetWindow32( hwndControl, GW_HWNDNEXT );
	    }
	    while (!hwndNext)
	    {
		hwndControl = GetParent32( hwndControl );
		if (hwndControl == hwndDlg)
		{
		    hwndNext = GetWindow32( hwndDlg, GW_CHILD );
		}
		else
		{
		    hwndNext = GetWindow32( hwndControl, GW_HWNDNEXT );
		}
	    }
            hwndControl = hwndNext;
        }
        while (hwndControl != hwnd);
    }
    return RetVal;
}
 

/***********************************************************************
 *           DIALOG_IsDialogMessage
 */
static WINBOOL DIALOG_IsDialogMessage( HWND32 hwnd, HWND32 hwndDlg,
                                      UINT32 message, WPARAM32 wParam,
                                      LPARAM lParam, WINBOOL *translate,
                                      WINBOOL *dispatch, INT32 dlgCode )
{
    *translate = *dispatch = FALSE;

    if (message == WM_PAINT)
    {
        /* Apparently, we have to handle this one as well */
        *dispatch = TRUE;
        return TRUE;
    }

      /* Only the key messages get special processing */
    if ((message != WM_KEYDOWN) &&
        (message != WM_SYSCHAR) &&
	(message != WM_CHAR))
        return FALSE;

    if (dlgCode & DLGC_WANTMESSAGE)
    {
        *translate = *dispatch = TRUE;
        return TRUE;
    }

    switch(message)
    {
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessage32A( hwndDlg, WM_NEXTDLGCTL,
                                (GetKeyState32(VK_SHIFT) & 0x8000), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                WINBOOL fPrevious = (wParam == VK_LEFT || wParam == VK_UP);
                HWND32 hwndNext = 
                    GetNextDlgGroupItem32 (hwndDlg, GetFocus32(), fPrevious );
                SendMessage32A( hwndDlg, WM_NEXTDLGCTL, hwndNext, 1 );
                return TRUE;
            }
            break;

        case VK_ESCAPE:
            SendMessage32A( hwndDlg, WM_COMMAND, IDCANCEL,
                            (LPARAM)GetDlgItem32( hwndDlg, IDCANCEL ) );
            return TRUE;

        case VK_RETURN:
            {
                DWORD dw = SendMessage16( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
                {
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                    (LPARAM)GetDlgItem32(hwndDlg, LOWORD(dw)));
                }
                else
                {
                    SendMessage32A( hwndDlg, WM_COMMAND, IDOK,
                                    (LPARAM)GetDlgItem32( hwndDlg, IDOK ) );
    
                }
            }
            return TRUE;
        }
        *translate = TRUE;
        break; /* case WM_KEYDOWN */

    case WM_CHAR:
        if (dlgCode & DLGC_WANTCHARS) break;
        /* drop through */

    case WM_SYSCHAR:
        if (DIALOG_IsAccelerator( hwnd, hwndDlg, wParam ))
        {
            /* don't translate or dispatch */
            return TRUE;
        }
        break;
    }

    /* If we get here, the message has not been treated specially */
    /* and can be sent to its destination window. */
    *dispatch = TRUE;
    return TRUE;
}


/***********************************************************************
 *           IsDialogMessage16   (USER.90)
 */
BOOL16 WINAPI WIN16_IsDialogMessage16( HWND16 hwndDlg, SEGPTR msg16 )
{
    LPMSG16 msg = PTR_SEG_TO_LIN(msg16);
    WINBOOL ret, translate, dispatch;
    INT32 dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild16( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessage16( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg16);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage16( msg );
    if (dispatch) DispatchMessage16( msg );
    return ret;
}


BOOL16 WINAPI IsDialogMessage16( HWND16 hwndDlg, LPMSG16 msg )
{
    LPMSG16 msg16 = SEGPTR_NEW(MSG16);
    WINBOOL ret;

    *msg16 = *msg;
    ret = WIN16_IsDialogMessage16( hwndDlg, SEGPTR_GET(msg16) );
    SEGPTR_FREE(msg16);
    return ret;
}

/***********************************************************************
 *           IsDialogMessage32A   (USER32.342)
 */
WINBOOL WINAPI IsDialogMessage32A( HWND32 hwndDlg, LPMSG32 msg )
{
    WINBOOL ret, translate, dispatch;
    INT32 dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild32( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessage32A( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage32( msg );
    if (dispatch) DispatchMessage32A( msg );
    return ret;
}


/***********************************************************************
 *           IsDialogMessage32W   (USER32.343)
 */
WINBOOL WINAPI IsDialogMessage32W( HWND32 hwndDlg, LPMSG32 msg )
{
    WINBOOL ret, translate, dispatch;
    INT32 dlgCode;

    if ((hwndDlg != msg->hwnd) && !IsChild32( hwndDlg, msg->hwnd ))
        return FALSE;

    dlgCode = SendMessage32W( msg->hwnd, WM_GETDLGCODE, 0, (LPARAM)msg);
    ret = DIALOG_IsDialogMessage( msg->hwnd, hwndDlg, msg->message,
                                  msg->wParam, msg->lParam,
                                  &translate, &dispatch, dlgCode );
    if (translate) TranslateMessage32( msg );
    if (dispatch) DispatchMessage32W( msg );
    return ret;
}


/****************************************************************
 *         GetDlgCtrlID16   (USER.277)
 */
INT16 WINAPI GetDlgCtrlID16( HWND16 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/****************************************************************
 *         GetDlgCtrlID32   (USER32.234)
 */
INT32 WINAPI GetDlgCtrlID32( HWND32 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/***********************************************************************
 *           GetDlgItem16   (USER.91)
 */
HWND16 WINAPI GetDlgItem16( HWND16 hwndDlg, INT16 id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == (UINT16)id) return pWnd->hwndSelf;
    return 0;
}


/***********************************************************************
 *           GetDlgItem32   (USER32.235)
 */
HWND32 WINAPI GetDlgItem32( HWND32 hwndDlg, INT32 id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == (UINT16)id) return pWnd->hwndSelf;
    return 0;
}


/*******************************************************************
 *           SendDlgItemMessage16   (USER.101)
 */
LRESULT WINAPI SendDlgItemMessage16( HWND16 hwnd, INT16 id, UINT16 msg,
                                     WPARAM16 wParam, LPARAM lParam )
{
    HWND16 hwndCtrl = GetDlgItem16( hwnd, id );
    if (hwndCtrl) return SendMessage16( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32A   (USER32.452)
 */
LRESULT WINAPI SendDlgItemMessage32A( HWND32 hwnd, INT32 id, UINT32 msg,
                                      WPARAM32 wParam, LPARAM lParam )
{
    HWND32 hwndCtrl = GetDlgItem32( hwnd, id );
    if (hwndCtrl) return SendMessage32A( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32W   (USER32.453)
 */
LRESULT WINAPI SendDlgItemMessage32W( HWND32 hwnd, INT32 id, UINT32 msg,
                                      WPARAM32 wParam, LPARAM lParam )
{
    HWND32 hwndCtrl = GetDlgItem32( hwnd, id );
    if (hwndCtrl) return SendMessage32W( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText16   (USER.92)
 */
void WINAPI SetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR lpString )
{
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32A   (USER32.478)
 */
WINBOOL WINAPI SetDlgItemText32A( HWND32 hwnd, INT32 id, LPCSTR lpString )
{
    return SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32W   (USER32.479)
 */
WINBOOL WINAPI SetDlgItemText32W( HWND32 hwnd, INT32 id, LPCWSTR lpString )
{
    return SendDlgItemMessage32W( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/***********************************************************************
 *           GetDlgItemText16   (USER.93)
 */
INT16 WINAPI GetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR str, UINT16 len )
{
    return (INT16)SendDlgItemMessage16( hwnd, id, WM_GETTEXT,
                                        len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32A   (USER32.237)
 */
INT32 WINAPI GetDlgItemText32A( HWND32 hwnd, INT32 id, LPSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32A( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32W   (USER32.238)
 */
INT32 WINAPI GetDlgItemText32W( HWND32 hwnd, INT32 id, LPWSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32W( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/*******************************************************************
 *           SetDlgItemInt16   (USER.94)
 */
void WINAPI SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    return SetDlgItemInt32( hwnd, (UINT32)(UINT16)id, value, fSigned );
}


/*******************************************************************
 *           SetDlgItemInt32   (USER32.477)
 */
void WINAPI SetDlgItemInt32( HWND32 hwnd, INT32 id, UINT32 value,
                             WINBOOL fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT32)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemInt16   (USER.95)
 */
UINT16 WINAPI GetDlgItemInt16( HWND16 hwnd, INT16 id, BOOL16 *translated,
                               BOOL16 fSigned )
{
    UINT32 result;
    WINBOOL ok;

    if (translated) *translated = FALSE;
    result = GetDlgItemInt32( hwnd, (UINT32)(UINT16)id, &ok, fSigned );
    if (!ok) return 0;
    if (fSigned)
    {
        if (((INT32)result < -32767) || ((INT32)result > 32767)) return 0;
    }
    else
    {
        if (result > 65535) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT16)result;
}


/***********************************************************************
 *           GetDlgItemInt32   (USER32.236)
 */
UINT32 WINAPI GetDlgItemInt32( HWND32 hwnd, INT32 id, WINBOOL *translated,
                               WINBOOL fSigned )
{
    char str[30];
    char * endptr;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!SendDlgItemMessage32A(hwnd, id, WM_GETTEXT, sizeof(str), (LPARAM)str))
        return 0;
    if (fSigned)
    {
        result = strtol( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if (((result == LONG_MIN) || (result == LONG_MAX)) && (errno==ERANGE))
            return 0;
    }
    else
    {
        result = strtoul( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if ((result == ULONG_MAX) && (errno == ERANGE)) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT32)result;
}


/***********************************************************************
 *           CheckDlgButton16   (USER.97)
 */
BOOL16 WINAPI CheckDlgButton16( HWND16 hwnd, INT16 id, UINT16 check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           CheckDlgButton32   (USER32.45)
 */
WINBOOL WINAPI CheckDlgButton32( HWND32 hwnd, INT32 id, UINT32 check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           IsDlgButtonChecked16   (USER.98)
 */
UINT16 WINAPI IsDlgButtonChecked16( HWND16 hwnd, UINT16 id )
{
    return (UINT16)SendDlgItemMessage32A( hwnd, id, BM_GETCHECK32, 0, 0 );
}


/***********************************************************************
 *           IsDlgButtonChecked32   (USER32.344)
 */
UINT32 WINAPI IsDlgButtonChecked32( HWND32 hwnd, UINT32 id )
{
    return (UINT32)SendDlgItemMessage32A( hwnd, id, BM_GETCHECK32, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton16   (USER.96)
 */
BOOL16 WINAPI CheckRadioButton16( HWND16 hwndDlg, UINT16 firstID,
                                  UINT16 lastID, UINT16 checkID )
{
    return CheckRadioButton32( hwndDlg, firstID, lastID, checkID );
}


/***********************************************************************
 *           CheckRadioButton32   (USER32.48)
 */
WINBOOL WINAPI CheckRadioButton32( HWND32 hwndDlg, UINT32 firstID,
                                  UINT32 lastID, UINT32 checkID )
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
	SendMessage32A( pWnd->hwndSelf, BM_SETCHECK32,
                        (pWnd->wIDmenu == checkID), 0 );
        if (pWnd->wIDmenu == lastID) break;
	pWnd = pWnd->next;
    }
    return TRUE;
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER.243) (USER32.233)
 */
DWORD WINAPI GetDialogBaseUnits(void)
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect16   (USER.103)
 */
void WINAPI MapDialogRect16( HWND16 hwnd, LPRECT16 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           MapDialogRect32   (USER32.382)
 */
void WINAPI MapDialogRect32( HWND32 hwnd, LPRECT32 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           GetNextDlgGroupItem16   (USER.227)
 */
HWND16 WINAPI GetNextDlgGroupItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                     BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgGroupItem32( hwndDlg, hwndCtrl, fPrevious );
}


/***********************************************************************
 *           GetNextDlgGroupItem32   (USER32.275)
 */
HWND32 WINAPI GetNextDlgGroupItem32( HWND32 hwndDlg, HWND32 hwndCtrl,
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
 *           GetNextDlgTabItem16   (USER.228)
 */
HWND16 WINAPI GetNextDlgTabItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                   BOOL16 fPrevious )
{
    return (HWND16)GetNextDlgTabItem32( hwndDlg, hwndCtrl, fPrevious );
}


/***********************************************************************
 *           GetNextDlgTabItem32   (USER32.276)
 */
HWND32 WINAPI GetNextDlgTabItem32( HWND32 hwndDlg, HWND32 hwndCtrl,
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

