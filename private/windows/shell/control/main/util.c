/** FILE: util.c *********** Module Header ********************************
 *
 *  Control panel utility library routines. This file contains string,
 *  cursor and SendWinIniChange() routines.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  15:30 on Thur  03 May 1994  -by-  Steve Cathcart   [stevecat]
 *        Increased  MyMessageBox buffers, Restart dialog changes
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *************************************************************************/

//==========================================================================
//                                Include files
//==========================================================================

#include <nt.h>    // For shutdown privilege.
#include <ntrtl.h>
#include <nturtl.h>

#include "main.h"

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


BOOL APIENTRY WantArrows (HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    switch (message)
    {
    case WM_GETDLGCODE:

        return DLGC_WANTARROWS | DLGC_WANTCHARS;
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
        return (SendMessage (GetParent (hWnd), message, wParam, lParam));
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        return (SendMessage (GetParent(hWnd),
                             WM_USER + 0x0401 + message - WM_SETFOCUS,
                             wParam, (LPARAM) hWnd));
        break;

    default:
        return (CallWindowProc (lpprocStatic, hWnd, message, wParam, lParam));
   }
}

int MyMessageBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...)
{
    TCHAR szText[4*PATHMAX], szCaption[2*PATHMAX];
    int ival;
    va_list parg;

    va_start(parg, wType);

    if (wText == INITS)
        goto NoMem;

    if (!LoadString (hModule, wText, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    wvsprintf (szText, szCaption, parg);

    if (!LoadString (hModule, wCaption, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    if ((ival = MessageBox (hWnd, szText, szCaption, wType)) == 0)
        goto NoMem;

    va_end(parg);

    return (ival);

NoMem:
    va_end(parg);

    ErrMemDlg (hWnd);
    return 0;
}


/* This does what is necessary to bring up a dialog box
 */
int DoDialogBoxParam (int nDlg, HWND hParent, DLGPROC lpProc,
                                        DWORD dwHelpContext, DWORD dwParam)
{
    DWORD dwSave;

    dwSave = dwContext;
    dwContext = dwHelpContext;
    nDlg = DialogBoxParam (hModule, (LPTSTR) MAKEINTRESOURCE(nDlg), hParent, lpProc, dwParam);
    dwContext = dwSave;

    if (nDlg == -1)
        MyMessageBox (hParent, INITS, INITS+1, IDOK);

    return(nDlg);
}

// Turn hourglass on or off

void HourGlass (BOOL bOn)
{
    if (!GetSystemMetrics (SM_MOUSEPRESENT))
        ShowCursor (bOn);

    SetCursor (LoadCursor (NULL, bOn ? IDC_WAIT : IDC_ARROW));
}


LPTSTR strscan (LPTSTR pszString, LPTSTR pszTarget)
{
    LPTSTR psz;

    if (psz = _tcsstr (pszString, pszTarget))
        return (psz);
    else
        return (pszString + lstrlen (pszString));
}


/* StripBlanks()
   Strips leading and trailing blanks from a string.
   Alters the memory where the string sits.
*/

void StripBlanks (LPTSTR pszString)
{
    LPTSTR  pszPosn;

    /* strip leading blanks */
    pszPosn = pszString;
    while (*pszPosn == TEXT(' '))
        pszPosn++;

    if (pszPosn != pszString);
        lstrcpy (pszString, pszPosn);

    /* strip trailing blanks */
    if ((pszPosn = pszString + lstrlen (pszString)) != pszString)
    {
       pszPosn = CharPrev (pszString, pszPosn);
       while(*pszPosn == TEXT(' '))
           pszPosn = CharPrev (pszString, pszPosn);
       pszPosn = CharNext (pszPosn);
       *pszPosn = TEXT('\0');
    }
}

void SendWinIniChange (LPTSTR lpSection)
{
// NOTE: We have (are) gone through several iterations of which USER
//       api is the correct one to use.  The main problem for the Control
//       Panel is to avoid being HUNG if another app (top-level window)
//       is HUNG.  Another problem is that we pass a pointer to a message
//       string in our address space.  SendMessage will 'thunk' this properly
//       for each window, but PostMessage and SendNotifyMessage will not.
//       That finally brings us to try to use SendMessageTimeout(). 9/21/92
//
// Try SendNotifyMessage in build 260 or later - kills earlier builds
//    SendNotifyMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//    PostMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//  [stevecat] 4/4/92
//
//    SendMessage ((HWND)-1, WM_WININICHANGE, 0L, (LPARAM)lpSection);

    //  NOTE: The final parameter (LPDWORD lpdwResult) must be NULL

    SendMessageTimeout ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection,
                                            SMTO_ABORTIFHUNG, 1000, NULL);

//    if (iSection == WININIFONTS)
    if (!lstrcmpi (lpSection, TEXT("fonts")))
    {
        SendMessageTimeout ((HWND)-1, WM_FONTCHANGE, 0, 0L,
                                            SMTO_ABORTIFHUNG, 1000, NULL);

//        SendMessage ((HWND)-1, WM_FONTCHANGE, 0, 0L);
//        SendNotifyMessage ((HWND)-1, WM_FONTCHANGE, 0, 0L);
//        PostMessage ((HWND)-1, WM_FONTCHANGE, 0, 0L);
    }
}

int strpos (LPTSTR string, TCHAR chr)
{
    LPTSTR lpTmp;

    if (!(lpTmp = _tcschr (string, chr)))
        return(-1);
    else
        return (lpTmp - string);
}

/*============================================================================
;
; RestartDlg
;
; The following function is the dialog procedure for bringing up a system
; restart message.  This dialog is called whenever the user is advised to
; reboot the system.  The dialog contains an IDOK and IDCANCEL button, which
; instructs the function whether to restart windows immediately.
;
; Parameters:
;
; lParam - The LOWORD portion contains an index to the resource string
;          used to compose the restart message.  This string is inserted
;          before the string IDS_RESTART.
;
; Return Value: The usual dialog return value.
;
============================================================================*/

BOOL RestartDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR szMessage[200];
    TCHAR szTemp[100];
    BOOLEAN PreviousPriv;

    switch (message)
    {

        case WM_INITDIALOG:

            /* Set up the restart message */

            LoadString (hModule, LOWORD(lParam), szMessage, CharSizeOf(szMessage));
            if (LoadString (hModule, IDS_RESTART, szTemp, CharSizeOf(szTemp)))
                lstrcat (szMessage, szTemp);

            SetDlgItemText (hDlg, RESTART_TEXT, szMessage);
            break;

        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                case IDOK:
                    RtlAdjustPrivilege (SE_SHUTDOWN_PRIVILEGE,
                                        TRUE, FALSE, &PreviousPriv);
                    ExitWindowsEx (EWX_REBOOT | EWX_SHUTDOWN, (DWORD)(-1));
                    break;

                case IDCANCEL:
                    EndDialog (hDlg, 0L);
                    break;

                default:
                    return (FALSE);
            }
            return (TRUE);

        default:
            return (FALSE);
    }

    return (FALSE);
}

void BorderRect (HDC hDC, LPRECT lpRect, HBRUSH hBrush)
{
    int    cx;
    int    cy;
    HBRUSH hbrSave;
    extern int cxBorder,cyBorder;

    cx = lpRect->right - lpRect->left;     /* rectangle width */
    cy = lpRect->bottom - lpRect->top;     /* rectangle height */

    hbrSave = SelectObject (hDC, hBrush);

    PatBlt (hDC, lpRect->left, lpRect->top, cx, cyBorder, PATCOPY);
    PatBlt (hDC, lpRect->left, lpRect->bottom - cyBorder, cx, cyBorder, PATCOPY);
    PatBlt (hDC, lpRect->left, lpRect->top, cxBorder, cy, PATCOPY);
    PatBlt (hDC, lpRect->right - cxBorder, lpRect->top, cxBorder, cy, PATCOPY);

    SelectObject (hDC, hbrSave);    /* restore previous hbrush */
}

