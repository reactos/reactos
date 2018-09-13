/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** sciproc.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    CalcWndProc--Main window procedure.                             ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    SetRadix,                                                       ***/
/***    ProcessCommands.                                                ***/
/***                                                                    ***/
/*** Last modification Fri  08-Dec-1989                                 ***/
/*** -by- Amit Chatterjee. [amitc]                                      ***/
/*** Last modification July-21-1994                                     ***/
/*** -by- Arthur Bierer [t-arthb] or abierer@ucsd.edu                   ***/
/***                                                                    ***/
/*** Modified WM_PAINT processing to display ghnoLastNum rather than    ***/
/*** ghnoNum if the last key hit was an operator.                       ***/
/***                                                                    ***/
/**************************************************************************/

#include "scicalc.h"
#include "calchelp.h"

extern HWND     hStatBox;
extern HBRUSH   hBrushBk;
extern BOOL     bFocus, bError;
extern TCHAR    szDec[5], *rgpsz[CSTRINGS];
extern HNUMOBJ  ghnoNum, ghnoLastNum;
extern INT      nTempCom ;
extern INT      gnPendingError ;
extern BOOL     gbRecord;


BOOL FireUpPopupMenu( HWND hwnd, HINSTANCE hInstanceWin, LPARAM lParam)
{
    HMENU hmenu;

    if ((hmenu = LoadMenu(hInstanceWin, MAKEINTRESOURCE(IDM_HELPPOPUP))))
    {
        int cmd = TrackPopupMenuEx(GetSubMenu(hmenu, 0),
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
            LOWORD(lParam), HIWORD(lParam), hwnd, NULL);
        DestroyMenu(hmenu);
        return ( cmd == HELP_CONTEXTPOPUP ) ? TRUE : FALSE;

    }
    else
        return FALSE;
}

LRESULT APIENTRY CalcWndProc (
HWND           hWnd,
UINT           iMessage,
WPARAM         wParam,
LPARAM         lParam)
{
    INT         nID, nTemp;       /* Return value from GetKey & temp.  */
    HANDLE      hTempBrush; // a brush to play with in WM_CTLCOLORSTATIC

    switch (iMessage)
    {
        case WM_INITMENUPOPUP:
            /* Gray out the PASTE option if CF_TEXT is not available.     */
            /* nTemp is used here so we only call EnableMenuItem once.    */
            if (!IsClipboardFormatAvailable(CF_TEXT))
                nTemp=MF_GRAYED | MF_DISABLED;
            else
                nTemp=MF_ENABLED;

            EnableMenuItem(GetMenu(hWnd),IDM_PASTE, nTemp);
            break;

        case WM_CONTEXTMENU:
            // If the user clicked on the dialog face and not one of the
            // buttons then do nothing.  If the id of the button is IDC_STATIC
            // then do nothing. 

            if ( (HWND)wParam == g_hwndDlg )
            {
                // check for clicks on disabled buttons.  These aren't seen 
                // by WindowFromPoint but are seen by ChildWindowFromPoint.
                // As a result, the value of wParam will be g_hwndDlg 
                // if the WM_RBUTTONUP event occured on a disabled button.

                POINT pt;
                HWND  hwnd;

                // convert from short values to long values
                pt.x = MAKEPOINTS(lParam).x;   
                pt.y = MAKEPOINTS(lParam).y;

                // then convert to client coordinates
                ScreenToClient( g_hwndDlg, &pt );  

                hwnd = ChildWindowFromPoint( g_hwndDlg, pt );

                if ( !hwnd || (hwnd == g_hwndDlg) || 
                     (IDC_STATIC == GetDlgCtrlID( hwnd )))
                {
                    return (DefWindowProc(hWnd, iMessage, wParam, lParam));
                }

                wParam = (WPARAM)hwnd;
            }

            if ( FireUpPopupMenu( g_hwndDlg, hInst, lParam ) )
            {
                nID = GetDlgCtrlID( (HWND)wParam );

                WinHelp((HWND) wParam, rgpsz[IDS_HELPFILE], HELP_CONTEXTPOPUP,
                        GetHelpID( nID ));
            }
            break;

        case WM_HELP:
            HtmlHelp(GetDesktopWindow(), rgpsz[IDS_CHMHELPFILE], HH_DISPLAY_TOPIC, 0L);
            return 0;

        case WM_COMMAND: /* Interpret all buttons on calculator.          */
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier

            // the accelerator table feeds us IDC_MOD in response to the 
            // "%" key.  This same accelerator is used for the percent function
            // in Standard view so translate here.

            if ( (wID == IDC_MOD) && (nCalc == 1) )
                wID = IDC_PERCENT;

            // when we get an accelerator keystroke we fake a button press to provide feedback
            if ( wNotifyCode == 1 )
            {
                // For an accelerator the hwnd is not passed in the lParam so ask the dialog
                HWND hwndCtl = GetDlgItem( g_hwndDlg, wID );
                SendMessage( hwndCtl, BM_SETSTATE, 1, 0 );  // push the button down
                Sleep( 20 );                                // wait a bit
                SendMessage( hwndCtl, BM_SETSTATE, 0, 0 );  // push the button up
            }

            // we turn on notify for the text controls to automate the handling of context
            // help but we don't care about any commands we recieve from these controls. As
            // a result, only process commands that are not from a text control.
            if ( (wID != IDC_DISPLAY) && (wID != IDC_MEMTEXT) && (wID != IDC_PARTEXT) )
                ProcessCommands(wID);
            break;
        }

        case WM_CLOSE:
            if ( hStatBox )
            {
                SendMessage(hStatBox, WM_CLOSE, 0, 0L) ;
                hStatBox = NULL;
            }

            DestroyWindow(g_hwndDlg);
            KillTimeCalc();
            WinHelp(g_hwndDlg, rgpsz[IDS_HELPFILE], HELP_QUIT, 0L);
            PostQuitMessage(0);
            break;

        case WM_SYSCOMMAND:
            if ( (wParam & 0xFFF0) == SC_CLOSE )
            {
                PostQuitMessage(0);
            }
            return (DefWindowProc(hWnd, iMessage, wParam, lParam));

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            RECT & rect = pdis->rcItem;
            UINT & nState = pdis->itemState;
            HDC & hdc = pdis->hDC;

            int iBtnID = (int)wParam;
            LPCTSTR psz = rgpsz[INDEXFROMID(iBtnID)];

            SetTextColor( hdc, (nState & ODS_DISABLED)?GetSysColor(COLOR_GRAYTEXT):GetKeyColor( iBtnID ) );
            DrawEdge( hdc, &rect, (nState & ODS_SELECTED)?EDGE_SUNKEN:EDGE_RAISED, BF_RECT );
            DrawText( hdc, psz, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
            break;
        }

        case WM_CTLCOLORSTATIC:
            // get the Control's id from its handle in lParam
            if ( IDC_DISPLAY == GetWindowID( (HWND) lParam) )
            {
                // we set this window to a white backround
                hTempBrush = GetSysColorBrush( COLOR_WINDOW );
                SetBkColor( (HDC) wParam, GetSysColor( COLOR_WINDOW ) );
                SetTextColor( (HDC) wParam, GetSysColor( COLOR_WINDOWTEXT ) );

                return (LRESULT) hTempBrush;
            }
            return (DefWindowProc(hWnd, iMessage, wParam, lParam));

        case WM_SETTINGCHANGE:
            if (lParam!=0)
            {
                // we only care about changes to color and internation settings, ignore all others
                if (lstrcmp((LPCTSTR)lParam, TEXT("colors")) &&
                        lstrcmp((LPCTSTR)lParam, TEXT("intl")))
                    break;
            }

            // Always call if lParam==0.  This is simply for safety and isn't strictly needed
            InitSciCalc (FALSE);
            break;

        case WM_SIZE:
            {
                HWND hwndSizer;

                nTemp=SW_SHOW;
                if (wParam==SIZEICONIC)
                    nTemp=SW_HIDE;

                if (hStatBox!=0 && (wParam==SIZEICONIC || wParam==SIZENORMAL))
                    ShowWindow(hStatBox, nTemp);

                // A special control has been added to both dialogs with an ID of
                // IDC_SIZERCONTROL.  This control is possitioned such that the bottom of
                // the control determines the height of the dialog.  If a really large menu
                // font is selected then the menu might wrap to two lines, which exposes a
                // bug in Windows that causes the client area to be too small.  By checking
                // that IDC_SIZERCONTROL is fully visible we can compensate for this bug.
                hwndSizer = GetDlgItem( g_hwndDlg, IDC_SIZERCONTROL );
                if ( hwndSizer )
                {
                    RECT rc;
                    int iDelta;
                    GetClientRect( hwndSizer, &rc );
                    MapWindowPoints( hwndSizer, g_hwndDlg, (LPPOINT)&rc, 2 );

                    // if the difference between the current height of the client area
                    // (MAKEPOINTS(lParam).y) and the desired height of the client
                    // area (rc.bottom) is non-zero then we must adjust the size of the
                    // client area.  This will enlarge the client area if you switch
                    // from a regular menu font to a jumbo menu font and shrink the
                    // client area if you switch from a jumbo menu font to a regular
                    // menu font.
                    iDelta = rc.bottom - HIWORD(lParam);
                    if ( iDelta )
                    {
                        GetWindowRect( g_hwndDlg, &rc );
                        SetWindowPos( g_hwndDlg, NULL,
                            0, 0,                       // these are ingored due to SWP_NOMOVE
                            rc.right-rc.left,           // the width remains the same
                            rc.bottom-rc.top+iDelta,    // the heigth changes by iDelta
                            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
                        return 0;
                    }
                }
            }
            /* Fall through.                                              */

        default:
            return (DefWindowProc(hWnd, iMessage, wParam, lParam));
    }

    return 0L;
}
