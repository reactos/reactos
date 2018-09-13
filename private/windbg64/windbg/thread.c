/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Thread.c

Abstract:

    This module contains the functions needed for the Run.Set Thread dialog
    box.

Author:

    David J. Gilman (davegi) 21-Apr-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

/********************* Structs & Defines ************************************/

typedef struct {
    LPTD    lptd;
    BOOL    fFreeze;
    BOOL    fSet;
} THREAD_ACTION, *PTHREAD_ACTION;

/**************************** DATA ******************************************/

static PTHREAD_ACTION      pa = NULL;
static int     ca = 0;
static char    lpstrFreeze[ MAX_MSG_TXT ];
static char    lpstrThaw[ MAX_MSG_TXT ];

static int  iTabs[] = { 12, 30, 70, 90, 110, 130 };
static char szThFmt[] = "%s\t%2d\t%s\t%c %3s\t%s";

/**************************** CODE ******************************************/

BOOL
ThFormatInfo(
    PTHREAD_ACTION    pa,
    LPSTR lpTarget,
    UINT  cch,
    LPTST ptst
    )
/*++

Routine Description:

    This routine will format the data associated with a thread for
    displaying to the user.

Arguments:

    lptd      - Supplies internal handle to thread to be formatted

    lpTarget  - Supplies where to place the formatted string

    cch       - Supplies count of characters in buffer

    ptst      - Returns thread state info in structure pointed to

Return Value:

    TRUE if successful and FALSE otherwise

--*/
{

    XOSD xosd;

    Unreferenced(cch);

    xosd = OSDGetThreadStatus(pa->lptd->lppd->hpid, pa->lptd->htid, ptst);

    if (xosd != xosdNone) {
        return FALSE;
    } else {
        sprintf(lpTarget, szThFmt,
                  (pa->lptd == LptdCur)? " * " : (pa->fSet ? "(*)" : "   "),
                  pa->lptd->itid,
                  ptst->rgchThreadID,
                  pa->lptd->fFrozen? 'F' : 'T',
                  pa->fFreeze ? "(F)" : "(T)",
                  ptst->rgchState);
    }

    return TRUE;
}           /* ThFormatInfo() */


VOID
ThEmptyList(
    void
    )
/*++

Routine Description:

    Destructor for thread list box

Arguments:

    None

Return Value:

    None

--*/
{
    if (pa) {
        free( pa );
        pa = NULL;
    }
    ca = 0;
}


int
ThSetupDlg(
    HWND hDlg,
    BOOL fFirsttime
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    HWND    h = GetDlgItem(hDlg, ID_THREAD_LIST);
    char    rgch[256];
    LPTD    lptd;
    LPTD    lptdSelect = NULL;
    TST     tst;
    int     i, cc;


    Assert( h != NULL);

    i = (int) SendMessage(h, LB_GETCURSEL, 0, 0L);
    SendMessage(h, LB_RESETCONTENT, 0, 0L);

    if (LppdCur) {

        /*
         *  Count the threads in the process.
         */
        for (cc=0,lptd=LppdCur->lptdList; lptd; cc++, lptd=lptd->lptdNext) {
            ;
        }
        if (cc != ca) {
            ca = 0;
        }

        if (!fFirsttime && ca > 0 && i != LB_ERR) {

            lptdSelect = pa[i].lptd;

        } else if (cc) {

            lptdSelect = LptdCur;
            ThEmptyList();

            /*
             * allocate an array to hold state information
             */
            ca = cc;
            pa = (THREAD_ACTION *) malloc(sizeof(THREAD_ACTION)*ca);
            Assert( pa );
            memset(pa, 0, sizeof(THREAD_ACTION)*ca);

            /*
             *  Fill it in with initial information.
             */

            for (i = 0, lptd = LppdCur->lptdList; lptd; i++, lptd = lptd->lptdNext) {
                pa[i].lptd = lptd;
                pa[i].fFreeze = lptd->fFrozen;
                pa[i].fSet = (lptd == LptdCur);
            }
        }

        for (i = 0; i < ca; i++) {

            ThFormatInfo(&pa[i], rgch, sizeof(rgch), &tst);
            SendMessage(h, LB_ADDSTRING, 0, (LPARAM)(LPSTR) rgch);

            if (pa[i].lptd == lptdSelect) {
                SetDlgItemText(hDlg,
                               ID_THREAD_FREEZE,
                               pa[i].fFreeze ? lpstrThaw : lpstrFreeze);
                SendMessage( h, LB_SETCURSEL, i, 0L);
            }
        }
    }

    return ca;
}                    /* ThSetupDlg */




INT_PTR
CALLBACK
DlgThread(
    HWND   hDlg,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function is the dialog procedure for the Thread dialog
    box.  It allows the user to look at the current status of
    all threads in the current process and manuipulate them.

Arguments:

    hDlg    - Handle to dialog window

    msg     - Message to be processed

    wParam  - Info about the mesage

    lParam  - Info about the message

Return Value:

    TRUE if the message was dealt with here and FALSE otherwise

--*/
{
    int   i;

    static DWORD HelpArray[]=
    {
       ID_THREAD_LIST, IDH_SETTHRD,
       ID_THREAD_SELECT, IDH_SELECTTHREAD,
       ID_THREAD_FREEZE, IDH_FREEZE,
       ID_THREAD_FREEZEALL, IDH_FREEZEALL,
       ID_THREAD_THAWALL, IDH_THAWALL,
       0, 0
    };

    UNREFERENCED_PARAMETER( lParam );

    switch ( msg ) {

    case WM_INITDIALOG:

        //
        // Load the text strings for the Thaw/Freeze button once.
        //

        if( lpstrFreeze[ 0 ] == 0 ) {

            Dbg( LoadString(
                    GetModuleHandle( NULL ),
                    DLG_Cols_Thaw,
                    lpstrThaw,
                    MAX_MSG_TXT
                    )
                );

            Dbg( LoadString(
                    GetModuleHandle( NULL ),
                    DLG_Cols_Freeze,
                    lpstrFreeze,
                    MAX_MSG_TXT
                    )
                );
        }

        //
        // set tabs for listbox
        //
        SendMessage(GetDlgItem(hDlg, ID_THREAD_LIST),
            LB_SETTABSTOPS, sizeof(iTabs)/sizeof(*iTabs), (LPARAM)iTabs);

        //
        // now fill listbox and update freeze/thaw button
        //

        if (ThSetupDlg( hDlg, TRUE ) > 0) {
            return TRUE;
        } else {
            // Gray everything
            EnableWindow(GetDlgItem(hDlg, ID_THREAD_FREEZE), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_THREAD_FREEZEALL), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_THREAD_THAWALL), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_THREAD_SELECT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            SetFocus(GetDlgItem(hDlg, IDCANCEL));
            return FALSE;
        }

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;


    case WM_COMMAND:

        switch ( LOWORD(wParam) ) {

        case IDOK:

            //
            // Lock it all in
            //
            for (i = 0; i < ca; i++) {

                if (pa[i].lptd->fFrozen && !pa[i].fFreeze) {

                    if (OSDFreezeThread( LppdCur->hpid,
                                         pa[ i ].lptd->htid,
                                         TRUE
                            ) == xosdNone)
                    {
                        pa[i].lptd->fFrozen = FALSE;
                    }

                } else if (!pa[i].lptd->fFrozen && pa[i].fFreeze) {

                    if (OSDFreezeThread( LppdCur->hpid,
                                         pa[ i ].lptd->htid,
                                         TRUE
                            ) == xosdNone)
                    {
                        pa[i].lptd->fFrozen = TRUE;
                    }
                }
            }

            for (i = 0; i < ca; i++) {
                if (pa[i].fSet && pa[i].lptd != LptdCur) {
                    LptdCur = pa[i].lptd;
                    LppdCur = LptdCur->lppd;
                    UpdateDebuggerState(UPDATE_WINDOWS);
                    break;
                }
            }

            EndDialog( hDlg, TRUE );
            ThEmptyList();
            return TRUE;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            ThEmptyList();
            return TRUE;

        case ID_THREAD_FREEZE:
            i = (int) SendMessage( GetDlgItem( hDlg, ID_THREAD_LIST), LB_GETCURSEL, 0, 0);
            Assert(i != LB_ERR);

            // this could be a freeze or a thaw:
            pa[i].fFreeze = !pa[i].fFreeze;

            ThSetupDlg(hDlg, FALSE);
            return TRUE;

        case ID_THREAD_FREEZEALL:
            for (i=0; i<ca; i++) {
                pa[i].fFreeze = TRUE;
            }
            ThSetupDlg(hDlg, FALSE);
            return TRUE;

        case ID_THREAD_THAWALL:
            for (i=0; i<ca; i++) {
                pa[i].fFreeze = FALSE;
            }
            ThSetupDlg(hDlg, FALSE);
            return TRUE;

        case ID_THREAD_SELECT:
            // clear old current thread
            for (i=0; i<ca; i++) {
                pa[i].fSet = FALSE;
            }

            i = (int) SendMessage( GetDlgItem( hDlg, ID_THREAD_LIST), LB_GETCURSEL, 0, 0);
            pa[i].fSet = TRUE;

            ThSetupDlg(hDlg, FALSE);
            return TRUE;

        case ID_THREAD_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                ThSetupDlg(hDlg, FALSE);
                return TRUE;
            }
            break;

          }

        break;
    }

    return FALSE;
}
