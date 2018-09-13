#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

/***    DlgLine
**
**  Synopsis:
**      bool = DlgLine(hDlg, message, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to current dialog open
**      message - dialog message to be processed
**      wParam  - info about message
**      lParam  - info about message
**
**  Returns:
**
**  Description:
**      This function processes messages for the "LINE" dialog box.
**
**      MESSAGES:
**
**              WM_INITDIALOG - Initialize dialog box
**              WM_COMMAND- Input received
**              WM_HELP - Context-sensitive help
**              WM_CONTEXTMENU - Right click received
**      
*/

INT_PTR
CALLBACK
DlgLine(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int y;

    static DWORD HelpArray[]=
    {
       ID_LINE_LINE, IDH_LINE,
       0, 0
    };

    Unused(lParam);

    switch (message) {

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP, 
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_COMMAND:
        switch (wParam) {

          case IDOK:
            // Retrieve selected item text and compute line nbr

            y = GetDlgItemInt(hDlg, ID_LINE_LINE, NULL, FALSE);

            if (y <= 0) {
                ErrorBox2(hDlg, MB_TASKMODAL, ERR_Goto_Line);
                SetFocus(GetDlgItem(hDlg, ID_LINE_LINE));
            } else {
                GotoLine(curView, y, FALSE);
                EndDialog(hDlg, TRUE);
            }

            return (TRUE);

          case IDCANCEL :
            EndDialog(hDlg, TRUE);
            return (TRUE);

        }
        break;
    }

    return (FALSE);
}                                       /* DlgLine() */
