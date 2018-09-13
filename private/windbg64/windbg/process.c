#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

/********************* Structs & Defines ************************************/

typedef struct {
    LPPD        lppd;
    BOOL        fSet;
} A, *PA;

/**************************** DATA ******************************************/

static PA     Pa;
static int    Ca;

static int iTabs[] = { 14, 28, 68, 100, 120 };
static char szPrFmt[] = "%s\t%2d\t%s\t%s";

/**************************** CODE ******************************************/


static BOOL PASCAL
PrFormatInfo(
    PA      pa,
    LPSTR   lpTarget,
    UINT    cch
    )
/*++

Routine Description:

    This routine will format the data associated with a process for
    displaying to the user.

Arguments:

    lppd      - Supplies internal handle to process to be formatted
    lpTarget  - Supplies where to place the formatted string
    cch       - Supplies count of characters in buffer

Return Value:

    TRUE if successful and FALSE otherwise

--*/
{


    PST pst;

    Unreferenced(cch);

    OSDGetProcessStatus(pa->lppd->hpid, &pst);

    sprintf(lpTarget, szPrFmt,
            (pa->lppd == LppdCur)? " * " : (pa->fSet ? "(*)" : "   "),
            pa->lppd->ipid,
            pst.rgchProcessID,
            pst.rgchProcessState);

#if 0
    long        cbPid;
    char        rgb[10];
    char        szStr[100];
    LPSTR       lpstr;

    Unreferenced(cch);

    OSDGetDebugMetric(mtrcPidSize, pa->lppd->hpid, 0, &cbPid );
    OSDGetDebugMetric(mtrcPidValue, pa->lppd->hpid, 0, (long *) &rgb);
    EEFormatMemory(szStr, cbPid*2+1, rgb, cbPid*8, fmtUInt|fmtZeroPad, 16);

    OSDPtrace(osdProcStatus, 0, &lpstr, pa->lppd->hpid, 0);

    sprintf(lpTarget, szPrFmt,
                (pa->lppd == LppdCur)? " * " : (pa->fSet ? "(*)" : "   "),
                pa->lppd->ipid,
                cbPid*2, cbPid*2, szStr,
                lpstr);
#endif

    return TRUE;
}                                       /* PrFormatInfo() */


static VOID PASCAL
PrEmptyList(
    void
    )
/*++

Routine Description:

    Destructor for process list box

Arguments:

    None

Return Value:

    None

--*/
{
    if (Pa) {
        free( Pa );
        Pa = NULL;
    }
    Ca = 0;
}                                       /* PrEmptyList() */


static int PASCAL
PrSetupDlg(
    HWND hDlg,
    BOOL fFirstTime
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    char        rgch[256];
    LPPD        lppd;
    LPPD        lppdSelect = NULL;
    int         i;
    int         cc;
    HWND        h = GetDlgItem(hDlg, ID_PROCESS_LIST);


    Assert( h != NULL);

    i = (int) SendMessage(h, LB_GETCURSEL, 0, 0L);
    SendMessage(h, LB_RESETCONTENT, 0, 0L);

    if (lppd = GetLppdHead()) {

        /*
        **  Count the processes.
        */
        for (cc=0; lppd; lppd = lppd->lppdNext) {
            if (lppd->pstate != psDestroyed) {
                ++cc;
            }
        }
        if (cc != Ca) {
            Ca = 0;
        }

        if (!fFirstTime && Ca > 0 && i != LB_ERR) {

            lppdSelect = Pa[i].lppd;

        } else if (cc) {

            lppdSelect = LppdCur;
            PrEmptyList();

            Ca = cc;
            Pa = (PA) malloc(sizeof(A)*Ca);
            Assert(Pa);
            memset(Pa, 0, sizeof(A)*Ca);

            /*
            **  Fill it in with initial information.
            */

            for (i=0, lppd=GetLppdHead(); lppd; i++, lppd=lppd->lppdNext) {
                if (lppd->pstate == psDestroyed) {
                    continue;
                }
                Pa[i].lppd = lppd;
                Pa[i].fSet = (lppd == LppdCur);
            }
        }

        for (i = 0; i < Ca; i++) {
            PrFormatInfo(&Pa[i], rgch, sizeof(rgch));
            SendMessage(h, LB_ADDSTRING, 0, (LPARAM)(LPSTR) rgch);

            if (Pa[i].lppd == lppdSelect) {
                SendMessage( h, LB_SETCURSEL, i, 0L);
            }
        }

    }
    return Ca;
}                                       /* PrSetupDlg() */



/***    DlgProcess
**
**  Synopsis:
**      bool = DlgProcess(hDlg, msg, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to dialog window
**      msg     - Message to be processed
**      wParam  - Info about the mesage
**      lParam  - Info about the message
**
**  Returns:
**      TRUE if the message was deal with here and FALSE otherwise
**
**  Description:
**      This function is the dialog procecdure for the Set Process dialog
**      box.  It allows the user to look at the current status of
**      all threads in the current process and manuipulate them
*/

INT_PTR
WINAPI
DlgProcess(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int         i;

    static DWORD HelpArray[]=
    {
       ID_PROCESS_LIST, IDH_SETPROCESS,
       ID_PROCESS_SELECT, IDH_SELECTPROCESS,
       0, 0
    };

    switch ( msg ) {
      case WM_INITDIALOG:

        //
        // set tabs for listbox
        //
        SendMessage(GetDlgItem(hDlg, ID_PROCESS_LIST),
            LB_SETTABSTOPS, sizeof(iTabs)/sizeof(*iTabs), (LPARAM)iTabs);

        //
        // now fill listbox and update freeze/thaw button
        //

        if (PrSetupDlg( hDlg, TRUE ) > 0) {
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            return TRUE;
        } else {
            // Gray everything
            EnableWindow(GetDlgItem(hDlg, ID_PROCESS_SELECT), FALSE);
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

        switch ( wParam ) {

          case IDOK:

            for (i = 0; i < Ca; i++) {
                if (Pa[i].fSet && Pa[i].lppd != LppdCur) {
                    LppdCur = Pa[i].lppd;
                    LptdCur = LppdCur->lptdList;
                    UpdateDebuggerState(UPDATE_WINDOWS);
                    break;
                }
            }

            EndDialog(hDlg, TRUE);
            PrEmptyList();
            return TRUE;

          case IDCANCEL:
            EndDialog( hDlg, FALSE );
            PrEmptyList();
            return TRUE;

#ifdef AKENTF
          case ID_PROCESS_FREEZE:

            i = SendMessage( GetDlgItem( hDlg, ID_PROCESS_LIST), LB_GETCURSEL, 0, 0);
            Assert( i != LB_ERR);

            if (Pa[i].fFrozen) {
                if (OSDPtrace(osdFreezeState, FALSE, 0, LppdCur->hpid, htidNull)
                      == xosdNone) {
                    Pa[i].fFrozen = FALSE;
                    Pa[i].lppd->fFrozen = FALSE;
                }
            } else {
                if (OSDPtrace(osdFreezeState, TRUE,  0, LppdCur->hpid, htidNull)
                     == xosdNone) {
                    Pa[i].fFrozen = TRUE;
                    Pa[i].lppd->fFrozen = TRUE;
                }
            }

            PrSetupDlg(hDlg, FALSE);
            return TRUE;


          case ID_PROCESS_FREEZEALL:

            for (i=0; i<Ca; i++) {
                if (!Pa[i].fFrozen
                  && OSDPtrace(osdFreezeState, TRUE,  0, LppdCur->hpid, htidNull)
                      == xosdNone) {
                    Pa[i].fFrozen = TRUE;
                    Pa[i].lppd->fFrozen = TRUE;
                }
            }
            PrSetupDlg(hDlg, FALSE);
            return TRUE;


          case ID_PROCESS_THAWALL:

            for (i=0; i<Ca; i++) {
                if (Pa[i].fFrozen
                  && OSDPtrace(osdFreezeState, FALSE,  0, LppdCur->hpid, htidNull)
                      == xosdNone) {
                    Pa[i].fFrozen = FALSE;
                    Pa[i].lppd->fFrozen = FALSE;
                }
            }
            PrSetupDlg(hDlg, FALSE);
            return TRUE;
#endif

          case ID_PROCESS_SELECT:             // Make current

            // clear old current process
            for (i=0; i<Ca; i++) {
                Pa[i].fSet = FALSE;
            }

            i = (int) SendMessage( GetDlgItem( hDlg, ID_PROCESS_LIST), LB_GETCURSEL, 0, 0);
            Pa[i].fSet = TRUE;

            PrSetupDlg(hDlg, FALSE);
            EnableWindow( GetDlgItem(hDlg, IDOK), Pa[i].lppd != LppdCur );
            return TRUE;

        }
        break;
    }

    return FALSE;
}                                       /* DlgProcess() */
