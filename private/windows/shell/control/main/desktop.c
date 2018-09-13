/** FILE: desktop.c ******** Module Header ********************************
 *
 *  Control panel applet for Desktop configuration.  This file holds
 *  everything to do with Wallpaper and Pattern settings, Icon spacing,
 *  Cursor blink rate, and the sizing grid dialogs in the Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by- Steve Cathcart [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by- Steve Cathcart [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *
 *  NOTES:
 *      12/29/93    Had to change the value of screen saver string that
 *                  is normally stored in registry.  Even though storing
 *                  the full pathname of the screen saver is the right
 *                  thing to do, the registry and floating profile people
 *                  are complaining about it - so now we will only store
 *                  the .SCR (i.e. .EXE) filename.  This may be a problem
 *                  when trying to invoke 16-bit screen savers that have
 *                  a name the same as a 32-bit one - but that is the price.
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"
#include "newexe.h"     // Header file to read 16-bit .EXE's

// Windows sdk
#include <shellapi.h>
#include <winuserp.h>

// Anti Aliased text support

ULONG SetFontEnumeration(ULONG);

// see wingdip.h

#ifndef FE_AA_ON
#define FE_AA_ON 2
#endif

#ifndef FE_AA_DEFAULT
#define FE_AA_DEFAULT 4
#endif

//==========================================================================
//                            Local Definitions
//==========================================================================

#define CXYDESKPATTERN 8

#define SSDLYMIN  1         // minutes now
#define SSDLYMAX  99
#define SSDLYDEF  5

#define WIDTHMIN   1
#define WIDTHMAX  50
#define WIDTHDEF   1

#define GRIDMIN   0
#define GRIDMAX  49
#define GRIDDEF   0

#define ICONMIN   1
#define ICONMAX 512
#define ICONDEF 100

#define NONE_LENGTH 16

// Return values for GetSaverName() function
#define GSN_SS_UNK   0          // Unknow screen saver type (should never happen)
#define GSN_SS_32BIT 1          // 32-bit screen saver
#define GSN_SS_16BIT 2          // 16-bit screen saver

//==========================================================================
//                            External Declarations
//==========================================================================
/* Functions */

//==========================================================================
//                     Local Data Declarations
//==========================================================================
TCHAR szSecure[]         = TEXT("ScreenSaverIsSecure");
TCHAR sz16bitDefSaver[]  = TEXT("black16.scr");

short  CursorRate;
RECT   rCursor;
HBRUSH hbrBkgd;                   /* window background color */
BOOL   bCursorOn;
BOOL   bBkgdOrText;
BOOL   bCaptured;
BOOL   bChanged;
short  nChanged;

WORD patbits[CXYDESKPATTERN];              /* bits for Background pattern */

ARROWVSCROLL avsSSMinutes = {1, -1, 5, -5, SSDLYMAX, SSDLYMIN, SSDLYDEF, SSDLYDEF};

ARROWVSCROLL avsIconGrid = { 1, -1, 5, -5, ICONMAX, ICONMIN, ICONDEF, ICONDEF};

ARROWVSCROLL avsGridGran = { 1, -1, 5, -5, GRIDMAX, GRIDMIN, GRIDDEF, GRIDDEF};

ARROWVSCROLL avsWidthGrid = {1, -1, 5, -5, WIDTHMAX, WIDTHMIN, WIDTHDEF, WIDTHDEF};

ARROWVSCROLL avsCursor   = {-CURSORRANGE / 50, CURSORRANGE / 50, -CURSORRANGE / 4,
                                CURSORRANGE / 4, CURSORMAX, CURSORMIN, 0, 0};

// Defines for flags field
#define SS_NONE         0x10000000L     // (None) screen saver selection
#define SS_16BITDEF     0x00000001L     // 16-bit default scrnsave.scr
#define SS_16BITSCR     0x00000002L     // 16-bit screen saver - *.scr
#define SS_16BITIW      0x00000004L     // 16-bit idlewild screen saver
#define SS_16BIT        0x00000008L     // 16-bit screen saver - other type
#define SS_32BITDEF     0x00000010L     // 32-bit default scrnsave.scr
#define SS_32BITSCR     0x00000020L     // 32-bit screen saver - *.scr
#define SS_32BITIW      0x00000040L     // 32-bit idlewild screen saver
#define SS_32BIT        0x00000080L     // 32-bit screen saver - other type


//  Linked-list structure for screen savers
typedef struct _ssaver
{
    DWORD  dwFlags;                 //  Flags - info on type of scrnsaver
    TCHAR  szSaverPath[PATHMAX];    //  Pathname to executable file
} SSAVER, *PSSAVER;

SSAVER *pssFirst = NULL;
SSAVER *pss16bitDef = NULL;
SSAVER *pss32bitDef = NULL;


//==========================================================================
//                      Local Function Prototypes
//==========================================================================
int    AddSaverName  (HANDLE hCombo, LPTSTR pSaverName, LPTSTR pSaverPath);
BOOL   CheckVal      (HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID);
HBRUSH CreateBkPatBrush      (HWND hDlg);
void   CreateSaverExecString (LPTSTR szTemp, PSSAVER pssSaver, WORD wParam);
void   CursorSpeedInit       (HWND hDlg);
void   FindSavers     (HANDLE hCombo);
void   FreeSavers     (HANDLE hSavers);
int    GetSaverName   (LPTSTR pSaverName, LPTSTR pSaverPath, BOOL bAcceptAny);
void   GridInit       (HWND hDlg);
BOOL   GridOK         (HWND hDlg);
int    lstrpos        (LPTSTR string, TCHAR chr);
void   PatternPaint   (HWND hDlg, HDC hDC, LPRECT lprUpdate);
void   PatternUpdate  (HWND hDlg);
void   ReadPattern    (LPTSTR lpStr);
void   SaverDirSearch (HANDLE hCombo, LPTSTR pPath, LPTSTR pSpec, BOOL bAcceptAny);


void ReadPattern (LPTSTR lpStr)
{
    short    i, val;

    /* Get eight groups of numbers seprated by non-numeric characters. */
    for (i = 0; i < CXYDESKPATTERN; i++)
    {
        val = 0;
        if (*lpStr != TEXT('\0'))
        {
            /* Skip over any non-numeric characters. */
            while (!(*lpStr >= TEXT('0') && *lpStr <= TEXT('9')))
                lpStr++;

            /* Get the next series of digits. */
            while (*lpStr >= TEXT('0') && *lpStr <= TEXT('9'))
                val = (short) (val * 10 + *lpStr++ - TEXT('0'));
        }

        patbits[i] = val;
    }
    return;
}


HBRUSH CreateBkPatBrush (HWND hDlg)
{
    HBITMAP hbmDesktop, hbmMem;
    HDC     hdcScreen, hdcMemSrc, hdcMemDest;
    HBRUSH  hbrPat = NULL;

    if (hbmDesktop = CreateBitmap (CXYDESKPATTERN, CXYDESKPATTERN, 1, 1,
                    (LPTSTR)patbits))
    {
        hdcScreen = GetDC (hDlg);

        if (hdcMemSrc = CreateCompatibleDC (hdcScreen))
        {
            SelectObject (hdcMemSrc, hbmDesktop);

            if (hbmMem = CreateCompatibleBitmap (hdcScreen, CXYDESKPATTERN,
                                                CXYDESKPATTERN))
            {
                if (hdcMemDest = CreateCompatibleDC (hdcScreen))
                {
                    SelectObject (hdcMemDest, hbmMem);
                    SetTextColor (hdcMemDest, GetSysColor (COLOR_BACKGROUND));
                    SetBkColor (hdcMemDest, GetSysColor (COLOR_WINDOWTEXT));
                    BitBlt (hdcMemDest, 0, 0, CXYDESKPATTERN, CXYDESKPATTERN,
                                                hdcMemSrc, 0, 0, SRCCOPY);

                    hbrPat = CreatePatternBrush (hbmMem);

                    /* Clean up */
                    DeleteDC (hdcMemDest);
                }
                DeleteObject (hbmMem);
            }
            DeleteDC (hdcMemSrc);
        }
        ReleaseDC (hDlg, hdcScreen);
        DeleteObject (hbmDesktop);
    }

    return (hbrPat);
}


void PatternPaint (HWND hDlg, HDC hDC, LPRECT lprUpdate)
{
    short    x, y;
    RECT rBox, PatRect;
    HBRUSH hbrBkgd, hbrText;

    GetWindowRect (GetDlgItem (hDlg, IDD_PATTERN), (LPRECT) &PatRect);
    PatRect.top++, PatRect.left++, PatRect.bottom--, PatRect.right--;
    ScreenToClient (hDlg, (LPPOINT) &PatRect.left);
    ScreenToClient (hDlg, (LPPOINT) &PatRect.right);
    if (IntersectRect ((LPRECT) &rBox, (LPRECT)lprUpdate, (LPRECT) &PatRect))
    {
        if (hbrBkgd = CreateSolidBrush (GetNearestColor (hDC,
                                            GetSysColor (COLOR_BACKGROUND))))
        {
            if (hbrText = CreateSolidBrush (GetSysColor (COLOR_WINDOWTEXT)))
            {
                rBox.right = PatRect.left;
                for (x = 0; x < CXYDESKPATTERN; x++)
                {
                    rBox.left = rBox.right;
                    rBox.right = PatRect.left + ((PatRect.right - PatRect.left) * (x + 1)) / CXYDESKPATTERN;
                    rBox.bottom = PatRect.top;
                    for (y = 0; y < CXYDESKPATTERN; y++)
                    {
                        rBox.top = rBox.bottom;
                        rBox.bottom = PatRect.top + ((PatRect.bottom -
                                     PatRect.top) * (y + 1)) / CXYDESKPATTERN;
                        FillRect (hDC, (LPRECT) &rBox,
                            (patbits[y] & (0x01 << 7 - x)) ? hbrText : hbrBkgd);
                    }
                }
                DeleteObject (hbrText);
            }
            DeleteObject (hbrBkgd);
        }
    }
    GetWindowRect (GetDlgItem (hDlg, IDD_PATSAMPLE), (LPRECT) &PatRect);
    PatRect.top++, PatRect.left++, PatRect.bottom--, PatRect.right--;
    ScreenToClient (hDlg, (LPPOINT) &PatRect.left);
    ScreenToClient (hDlg, (LPPOINT) &PatRect.right);
    if (IntersectRect ((LPRECT) &rBox, (LPRECT)lprUpdate, (LPRECT) &PatRect))
    {
        if (hbrBkgd = CreateBkPatBrush (hDlg))
        {
            FillRect (hDC, (LPRECT) &rBox, hbrBkgd);
            DeleteObject (hbrBkgd);
        }
    }
}


void PatternUpdate (HWND hDlg)
{
    RECT rBox;
    HDC hDC = GetDC (hDlg);

    GetWindowRect (GetDlgItem (hDlg, IDD_PATTERN), (LPRECT) &rBox);
    ScreenToClient (hDlg, (LPPOINT) &rBox.left);
    ScreenToClient (hDlg, (LPPOINT) &rBox.right);
    PatternPaint (hDlg, hDC, (LPRECT) &rBox);
    GetWindowRect (GetDlgItem (hDlg, IDD_PATSAMPLE), (LPRECT) &rBox);
    ScreenToClient (hDlg, (LPPOINT) &rBox.left);
    ScreenToClient (hDlg, (LPPOINT) &rBox.right);
    PatternPaint (hDlg, hDC, (LPRECT) &rBox);
    ReleaseDC (hDlg, hDC);
    return;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PatternDlgProc () -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LONG PatternDlgProc (HWND hDlg, UINT wMsg, DWORD wParam, LONG lParam)
{
    HWND  hWndCombo, hWndComboSrc;
    HWND  hDlgParent;
    short nPats;
    short x, y;
    TCHAR szPattern[10];
    TCHAR szPats[81];
    TCHAR szNone[NONE_LENGTH];
    TCHAR szLHS[81], szRHS[81];
    DWORD nOldBkMode;
    RECT  rBox;
    POINT pt;

    PAINTSTRUCT ps;

    switch (wMsg)
    {
    case WM_INITDIALOG:
        bChanged = bCaptured = FALSE;

        GetWindowText (hDlg, szPats, CharSizeOf(szPats));

        hWndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);

        hDlgParent = (HWND) GetWindowLong (hDlg, GWL_HWNDPARENT);

//        hDlgParent = GetWindow (hDlg, GW_OWNER);
        hWndComboSrc = GetDlgItem (hDlgParent, IDD_PATTERNCOMBO);
        for (nPats = (short) (SendMessage (hWndComboSrc, CB_GETCOUNT, 0, 0L) - 1);
            nPats > 0; nPats--)
        {
            SendMessage (hWndComboSrc, CB_GETLBTEXT, nPats, (LPARAM)szPats);
            SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szPats);
        }

        /* This is kind of cute.  Since the NULL selection is index 0 in the parent,
         *   the indexes are 1 less than in the parent, and the user won't be editing
         *   a NULL bitmap, selecting (nPats - 1) highlights the appropriate item, or
         *   no item at all if the index was 0.
         */

        nChanged = (WORD)SendMessage (hWndComboSrc, CB_GETCURSEL, 0L, 0L);
        SendMessage (hWndCombo, CB_SETCURSEL, --nChanged, 0L);
        if (nChanged >= 0)
        {
            SendMessage (hWndCombo, CB_GETLBTEXT, nChanged, (LPARAM)szPats);
            LoadString (hModule, DESKTOP + 1, szPattern, CharSizeOf(szPattern));
            GetPrivateProfileString (szPattern, szPats, szNull, szPats,
                                     CharSizeOf(szPats), szCtlIni);
            ReadPattern (szPats);
        }
        else
        {
            patbits[0] = patbits[1] = patbits[2] = patbits[3] =
                patbits[4] = patbits[5] = patbits[6] = patbits[7] = 0;
        }
        /* Both ADD and CHANGE are always disabled, since no new scheme has been
         * entered and no change to an old scheme has been made.  DELETE is
         * enabled only if an old scheme is selected.
         */
        EnableWindow (GetDlgItem (hDlg, IDD_DELPATTERN), nChanged >= 0);
        EnableWindow (GetDlgItem (hDlg, IDD_ADDPATTERN), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDD_CHANGEPATTERN), FALSE);
        break;

    case WM_PAINT:
        BeginPaint (hDlg, &ps);
        nOldBkMode = SetBkMode (ps.hdc, TRANSPARENT);
        PatternPaint (hDlg,  ps.hdc, &ps.rcPaint);
        SetBkMode (ps.hdc, nOldBkMode);
        EndPaint (hDlg, &ps);
        return FALSE;            // let defdlg proc see this
        break;

    case WM_MOUSEMOVE:
        if (!bCaptured)
            return (FALSE);     /* Let it fall through to the DefDlgProc */

    case WM_LBUTTONDOWN:
        GetWindowRect (GetDlgItem (hDlg, IDD_PATTERN), (LPRECT) &rBox);
        ScreenToClient (hDlg, (LPPOINT) &rBox.left);
        ScreenToClient (hDlg, (LPPOINT) &rBox.right);

        LONG2POINT (lParam, pt);

        if (PtInRect ((LPRECT) &rBox, pt))
        {
            x = (short) (((pt.x - rBox.left) * CXYDESKPATTERN)
                                                / (rBox.right - rBox.left));
            y = (short) (((pt.y - rBox.top) * CXYDESKPATTERN)
                                                / (rBox.bottom - rBox.top));

            nPats = patbits[y];     /* Save old value */
            if (wMsg == WM_LBUTTONDOWN)
            {
                SetCapture (hDlg);
                EnableWindow (GetDlgItem (hDlg, IDD_DELPATTERN), FALSE);
                bChanged = bCaptured = TRUE;
                bBkgdOrText = patbits[y] & (0x01 << (7 - x));
                if (nChanged >= 0 && nChanged ==
                    (short) SendDlgItemMessage (hDlg,IDD_PATTERNCOMBO,
                                                CB_GETCURSEL,0,0L))
                    EnableWindow(GetDlgItem(hDlg,IDD_CHANGEPATTERN),TRUE);
            }
            if (bBkgdOrText)
                patbits[y] &= (~(0x01 << (7 - x)));
            else
                patbits[y] |= (0x01 << (7 - x));
            if (nPats != (short) patbits[y])
                PatternUpdate (hDlg);
        }
        return (FALSE);     /* Let it fall through to the DefDlgProc */
        break;

    case WM_LBUTTONUP:
        if (bCaptured)
        {
            ReleaseCapture ();
            bCaptured = FALSE;
        }
        return (FALSE);     /* Let it fall through to the DefDlgProc */
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_PATTERNCOMBO:
            hWndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                nChanged = x = (short)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0L);
                SendMessage (hWndCombo, CB_GETLBTEXT, x, (LPARAM)szLHS);
                LoadString (hModule, DESKTOP + 1, szPattern, CharSizeOf(szPattern));
                GetPrivateProfileString (szPattern, szLHS, szNull, szPats, CharSizeOf(szPats), szCtlIni);
                ReadPattern (szPats);
                PatternUpdate (hDlg);
                EnableWindow (GetDlgItem (hDlg, IDD_ADDPATTERN), bChanged = FALSE);
                EnableWindow (GetDlgItem (hDlg, IDD_CHANGEPATTERN), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDD_DELPATTERN), TRUE);
            }
            else if (HIWORD(wParam) == CBN_EDITCHANGE)
            {
                if (!(x = (short) SendMessage (hWndCombo, WM_GETTEXTLENGTH, 0, 0L)))
                {
                    EnableWindow (GetDlgItem (hDlg, IDD_DELPATTERN), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDD_CHANGEPATTERN), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDD_ADDPATTERN), FALSE);
                }
                else
                {
                    SendMessage (hWndCombo, WM_GETTEXT, 80, (LPARAM) szLHS);
                    y = (short) SendMessage (hWndCombo, CB_FINDSTRING, (WPARAM)(LONG)-1,
                                             (LPARAM) szLHS);
                    if (y >= 0)
                    {
                        nPats = (short)SendMessage (hWndCombo, CB_GETLBTEXTLEN,
                                                                        y, 0L);
                    }
                    else
                        nPats = --y;

                    EnableWindow (GetDlgItem (hDlg, IDD_DELPATTERN),
                                                !bChanged && (nChanged == y));
                    EnableWindow (GetDlgItem (hDlg, IDD_CHANGEPATTERN),
                                ((nChanged != y) || bChanged) && (x == nPats));
                    EnableWindow (GetDlgItem (hDlg, IDD_ADDPATTERN), x != nPats);
                }
            }
            break;

        case IDD_ADDPATTERN:
        case IDD_CHANGEPATTERN:
            SetDlgItemText (hDlg, IDCANCEL, pszClose);
            hWndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);
            SendMessage (hWndCombo, WM_GETTEXT, 80, (LPARAM)szLHS);

            if (LOWORD(wParam) == IDD_ADDPATTERN)
            {
                nChanged = (short) SendMessage(hWndCombo, CB_ADDSTRING, 0,
                                                (LPARAM)szLHS);
                SendMessage (hWndCombo, CB_SETCURSEL, nChanged, 0L);
            }
// NT's wvsprintf() assumes patbits[] contains word values with dword alignment
//  [stevecat] 8/8/91 - workaround for now by just using sprintf()
//            wvsprintf ((LPSTR)szRHS, (LPSTR)"%d %d %d %d %d %d %d %d",
//                (LPSTR)patbits);

            wsprintf (szRHS, TEXT("%d %d %d %d %d %d %d %d"),
                      patbits[0], patbits[1], patbits[2], patbits[3],
                      patbits[4], patbits[5], patbits[6], patbits[7]);

            LoadString (hModule, DESKTOP+1, szPattern, CharSizeOf(szPattern));
            WritePrivateProfileString (szPattern, szLHS, szRHS, szCtlIni);
            bChanged = FALSE;

            /* Disable the add button so we don't add duplicates */

            EnableWindow (GetDlgItem (hDlg, IDD_ADDPATTERN), FALSE);

            /* Disable the change button since we just saved */

            EnableWindow (GetDlgItem (hDlg, IDD_CHANGEPATTERN), FALSE);

            /* Enable the delete button since we have a current selection */

            EnableWindow(GetDlgItem(hDlg, IDD_DELPATTERN),TRUE);

#ifdef JAPAN
            //Bug#1065
            SetFocus(GetDlgItem(hDlg,IDD_DELPATTERN));
#endif

            break;




        case IDD_DELPATTERN:
            hWndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);
            if ((x = (short)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0L)) >= 0)
            {
                SendMessage (hWndCombo, WM_GETTEXT, 80, (LPARAM)szLHS);
                LoadString (hModule, DESKTOP + 1, szPattern, CharSizeOf(szPattern));
                if (RemoveMsgBox(hDlg, szLHS, REMOVEMSG_PATTERN))
                {
                    SetDlgItemText (hDlg, IDCANCEL, pszClose);
                    SendMessage(hWndCombo, CB_DELETESTRING, x, 0L);
                    WritePrivateProfileString (szPattern, szLHS, NULL,
                                                 szCtlIni);
                    /* Since this will cause the current combobox ]
                       selection to be cancelled, we'd better disable the
                       remove button */
                    EnableWindow (GetDlgItem (hDlg,IDD_DELPATTERN),FALSE);
                }
            }
#ifdef JAPAN
            //Bug#1065
            SetFocus(hWndCombo);
#endif
            break;

        case IDCANCEL:
        case IDOK:

            hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);
            hDlgParent = (HWND) GetWindowLong (hDlg, GWL_HWNDPARENT);
            hWndComboSrc = GetDlgItem(hDlgParent, IDD_PATTERNCOMBO);

            /* Save current selection in parent combobox */

            nChanged = (int)SendMessage (hWndComboSrc, CB_GETCURSEL, 0, 0L);
            SendMessage (hWndComboSrc, CB_GETLBTEXT, nChanged,
                                                (LPARAM) szLHS);

            /* Now rebuild parent combobox */

            SendMessage(hWndComboSrc, CB_RESETCONTENT, 0, 0L);
            for (nPats = (short) SendMessage (hWndCombo,CB_GETCOUNT,
                                             0,0L) - 1;nPats >= 0;
                 nPats--)
            {
               SendMessage(hWndCombo, CB_GETLBTEXT, nPats, (LPARAM)szPats);
               SendMessage(hWndComboSrc, CB_ADDSTRING, 0, (LPARAM)szPats);
            }
            LoadString (hModule, DESKTOP, szNone, CharSizeOf(szNone));
            SendMessage (hWndComboSrc, CB_INSERTSTRING, 0,
                         (LPARAM)szNone);

            if (LOWORD(wParam) == IDOK)
            {
               /* Select new pattern into parent combobox */

               nPats = (short)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0L);
               SendMessage (hWndComboSrc, CB_SETCURSEL, ++nPats, 0L);
            }
            else
            {
               /* Attempt to find old selection in parent combobox */

               nChanged = (int)SendMessage (hWndComboSrc,CB_FINDSTRING,
                                        (WPARAM)(LONG)-1, (LPARAM) szLHS);
               if (nChanged > 0)
                  SendMessage(hWndComboSrc, CB_SETCURSEL,nChanged, 0L);
               else
                  SendMessage(hWndComboSrc, CB_SETCURSEL,0, 0L);
            }
            EndDialog(hDlg, wParam == IDOK);
            break;

        case IDD_HELP:
            goto DoHelp;

        default:
            return (FALSE);
        }
        break;

    default:
        if (wMsg == wHelpMessage)
        {
DoHelp:
        //
        // Note:  The help context has already been set in the parent
        //        window.
        //
            CPHelp (hDlg);
            return(TRUE);
        }
        else
            return (FALSE);
    }
    return (TRUE);
}


BOOL GridOK (HWND hDlg)
{
    TCHAR   szLHS[PATHMAX];
    TCHAR   szBuf[PATHMAX];
    TCHAR   szPat[PATHMAX];
    TCHAR   szPathName[PATHMAX];
    TCHAR   szNone[NONE_LENGTH];
    TCHAR   szDefault[NONE_LENGTH];
    HWND    hwndCombo;
    int     i, wGridGran;
    BOOL    bOK;
    BOOL    bTiled;


    SystemParametersInfo (SPI_SETFASTTASKSWITCH,
                          IsDlgButtonChecked(hDlg, IDD_FASTSWITCH), 0L,
                          SPIF_UPDATEINIFILE);

    SystemParametersInfo (SPI_SETDRAGFULLWINDOWS,
                          IsDlgButtonChecked(hDlg, IDD_FULLDRAG), 0L,
                          SPIF_UPDATEINIFILE);

    {
        // Antialiased text support

        ULONG ul = SetFontEnumeration(0) & ~(FE_AA_ON | FE_AA_DEFAULT);
        if (IsDlgButtonChecked(hDlg, IDD_FS))
        {
            ul |= FE_AA_ON;
        }
        else if (IsDlgButtonChecked(hDlg, IDD_FS_ENHANCED))
        {
            ul |= (FE_AA_ON | FE_AA_DEFAULT);
        }
        SetFontEnumeration(ul);
    }

    LoadString (hModule, DESKTOP, szNone, CharSizeOf(szNone));
    LoadString (hModule, DESKTOP+10, szDefault, CharSizeOf(szDefault));

    //
    //  Get the Grid Granularity setting.
    //

    wGridGran = GetDlgItemInt (hDlg, IDD_GRIDGRAN, (BOOL *) &bOK, FALSE);

    SystemParametersInfo (SPI_SETGRIDGRANULARITY, (WORD) wGridGran, 0L, TRUE);

    //
    // Get the Icon Spacing setting.
    //

    wGridGran = GetDlgItemInt (hDlg, IDD_ICONSPACE, (BOOL *) &bOK, FALSE);

    SystemParametersInfo (SPI_ICONHORIZONTALSPACING, (WORD) wGridGran, 0L, TRUE);
    SystemParametersInfo (SPI_SETICONTITLEWRAP, IsDlgButtonChecked (hDlg, IDD_ICONWRAP), 0L, TRUE);

    //
    //  Get the selected pattern.
    //

    hwndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);

    i = SendMessage (hwndCombo, WM_GETTEXT, PATHMAX, (LPARAM)szBuf);

    LoadString (hModule, DESKTOP + 1, szLHS, CharSizeOf(szLHS));
    GetPrivateProfileString (szLHS, szBuf, szNull, szPat,
                             CharSizeOf(szPat), szCtlIni);
    //
    //  Patterns --> Pattern
    //

    szLHS[lstrlen (szLHS) - 1] = TEXT('\0');

    if (i != 0)
    {
        WriteProfileString (szDesktop, szLHS, szPat);
    }

    //
    //  Get the Tile/Center setting.
    //

    szBuf[0] = TEXT('0');
    szBuf[1] = TEXT('\0');

    if (IsDlgButtonChecked (hDlg, IDD_TILE))
        szBuf[0] = TEXT('1');

    LoadString (hModule, DESKTOP + 5, szLHS, CharSizeOf(szLHS));

    bTiled = (((TCHAR) GetProfileInt (szDesktop, szLHS, 1) + TEXT('0')) != szBuf[0]);

    WriteProfileString (szDesktop, szLHS, szBuf);

    //
    //  Update the screen.  Do this BEFORE changing win.ini, because if the
    //  bitmap cannot be loaded and is evil enough in format to GP Fault the
    //  system, we don't want it to be set within the boot.
    //                                               10 Oct 1989  clarkc
    //
    //  Get the selected wallpaper.
    //  return FALSE if it does not exist
    //

    hwndCombo = GetDlgItem (hDlg, IDD_WALLCOMBO);

    SendMessage (hwndCombo, WM_GETTEXT, PATHMAX, (LPARAM)szBuf);

    //
    //  Make a copy of Wallpaper path for "None" case.  This will give
    //  an initial value to the szPathName string, which is used below
    //  when we set it via SPI calls.
    //

    lstrcpy (szPathName, szBuf);

    /*
     * If bitmap name is not "None" or "Default" then make sure it exists
     */
    if (lstrcmp (szBuf, szNone) && lstrcmp (szBuf, szDefault))
    {
        if (MyOpenSystemFile (szBuf, szPathName, OF_EXIST) == INVALID_HANDLE_VALUE)
        {
            MyMessageBox (hDlg, DESKTOP+9, INITS+1, MB_OK|MB_ICONINFORMATION);
            return (FALSE);
        }
    }

    //
    //  Get the old pattern name, in case the call to SetDeskWallpaper fails.
    //  If so, call SetDeskWallpaper with the old pattern and hope it works.
    //

    LoadString (hModule, DESKTOP + 4, szLHS, CharSizeOf(szLHS));
    GetProfileString (szDesktop, szLHS, szNone, szPat, CharSizeOf(szPat));

    //
    // Attempt write the profile.  If it fails don't change the wallpaper
    //

    if (!WriteProfileString (szDesktop, szLHS, szBuf))
    {
        return TRUE;
    }

    //
    //  if the bitmap or its position has changed
    //

    if ((bTiled || _tcsicmp (szBuf, szPat)) &&
        !SystemParametersInfo (SPI_SETDESKWALLPAPER, 0, (PVOID) szPathName, 0))
    {
        //
        //  Restore the original wallpaper
        //

        WriteProfileString (szDesktop, szLHS, szPat);

        //
        //  Inform user of error displaying bitmap;
        //  I assume it is a memory error, since I verified the file exists
        //  BUG: We can also get here if it is an invalid file.
        //

        MyMessageBox(hDlg, INITS, INITS+1, MB_OK|MB_ICONINFORMATION);
        return FALSE;
    }
    else
    {
        // Send a win.ini change message.
        SendWinIniChange (szDesktop);
        return TRUE;
    }
}

void GridInit (HWND hDlg)
{
    WORD   nSize;
    LPTSTR   pstrT;
    LPTSTR   pszBuffer;
    TCHAR   szBuf[PATHMAX*4];
    TCHAR   szWallPaper[PATHMAX];
    TCHAR   szLHS[20];
    TCHAR   szNone[NONE_LENGTH];
    TCHAR   szDefault[NONE_LENGTH];
    HWND   hwndCombo;
    int    wGridGran;
    HANDLE hPat;
//    HANDLE hTemp;
    short  nCount, i, j;

    //
    // Initialize the Grid Granularity field.
    //

    SystemParametersInfo (SPI_GETGRIDGRANULARITY, 0, (PVOID)(LPINT) &wGridGran, FALSE);
    SetDlgItemInt (hDlg, IDD_GRIDGRAN, wGridGran, FALSE);

    //
    //  Initialize the Desktop Pattern fields.
    //

    hwndCombo = GetDlgItem (hDlg, IDD_PATTERNCOMBO);

    //
    //  Get the patterns out of CONTROL.INI.
    //

    nSize = 512;
    pszBuffer = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(nSize));
    LoadString (hModule, DESKTOP + 1, szLHS, CharSizeOf(szLHS));
    while ((WORD)GetPrivateProfileString (szLHS, NULL, szNull,  pszBuffer, nSize, szCtlIni) == nSize)
    {
        nSize += 512;
        LocalFree ((HANDLE) pszBuffer);
        if (!(pszBuffer = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(nSize))))
            return;
    }

    //
    //  Put the patterns into the combo box.
    //

    pstrT = pszBuffer;
    while (*pstrT)
    {
        // if there's a right-hand side, add it to the list box
//
//  [stevecat] Changed buffer size to work-around a Win32 base bug in
//             GetPrivateProfileString.  It was returning the wrong value
//             when it was passed a buffer of size 2.
//        if (GetPrivateProfileString (szLHS, pstrT, szNull, szBuf, CharSizeOf(szBuf), szCtlIni))
//
        if (GetPrivateProfileString (szLHS, pstrT, szNull, szBuf, CharSizeOf(szBuf), szCtlIni))
            SendMessage (hwndCombo, CB_ADDSTRING, 0, (LPARAM)pstrT);
        while (*pstrT)
            pstrT++;
        pstrT++;
    }
    LocalFree ((HANDLE)pszBuffer);

    szLHS[lstrlen (szLHS) - 1] = TEXT('\0');

    LoadString (hModule, DESKTOP, szNone, CharSizeOf(szNone));
    LoadString (hModule, DESKTOP+10, szDefault, CharSizeOf(szDefault));

    GetProfileString (szDesktop, szLHS, szNone, szBuf, CharSizeOf(szBuf));

    if (!(*szBuf))
        lstrcpy (szBuf, szNone);

    szLHS[lstrlen (szLHS)] = TEXT('s');

    if (hPat = FindRHSIni (szCtlIni, szLHS, szBuf))
    {
        SendMessage (hwndCombo, CB_SELECTSTRING, (WPARAM)(LONG)-1, (LPARAM)(LPTSTR)LocalLock (hPat));
        LocalUnlock (hPat);
        LocalFree (hPat);
    }
    else
    {
        LoadString (hModule, DESKTOP + 13, szLHS, CharSizeOf(szLHS));
        SendMessage (hwndCombo, WM_SETTEXT, 0, (LPARAM)szLHS);
    }

    //
    //  Initialize the Wallpaper Combo.
    //

    LoadString (hModule, DESKTOP + 4, szLHS, CharSizeOf(szLHS));

    GetProfileString (szDesktop, szLHS, szNone, szWallPaper, CharSizeOf(szWallPaper));

    if (!(*szWallPaper))
        lstrcpy (szWallPaper, szNone);

    hwndCombo = GetDlgItem (hDlg, IDD_WALLCOMBO);
    LoadString (hModule, DESKTOP + 6, szLHS, CharSizeOf(szLHS));  /*  "\\*.BMP"  */

    lstrcpy (szBuf, pszSysDir);
    lstrcat (szBuf, szLHS);
    SendMessage (hwndCombo, CB_DIR, 0, (LPARAM)szBuf);

    lstrcpy (szBuf, pszWinDir);
    lstrcat (szBuf, szLHS);
    SendMessage (hwndCombo, CB_DIR, 0, (LPARAM) szBuf);
    SendMessage (hwndCombo, CB_INSERTSTRING, 0, (LPARAM) szNone);
    SendMessage (hwndCombo, CB_INSERTSTRING, 1, (LPARAM) szDefault);

    //
    //  Clean out duplicate items.  This code depends on there being at most
    //  1 duplicate of a given filename, since there were only 2 CB_DIR
    //  messages sent.  It's trivial to change it if needed, but there's also
    //  no point in adding additional tests until it actually is necessary.
    //                                       12 October 1989   clarkc
    //

    nCount = (short) (SendMessage (hwndCombo, CB_GETCOUNT, 0, 0L) - 1);
    for (i = 0; i < nCount; i++)
    {
        SendMessage (hwndCombo, CB_GETLBTEXT, i, (LPARAM)szBuf);
        if ((j = (short) SendMessage (hwndCombo, CB_FINDSTRING, i,
                    (LPARAM)szBuf)) > i)
        {
            SendMessage (hwndCombo, CB_DELETESTRING, j, 0L);
            nCount--;
        }
    }

    SendMessage (hwndCombo, CB_SELECTSTRING, (WPARAM)(LONG)-1, (LPARAM) szWallPaper);
    SendMessage (hwndCombo, WM_SETTEXT, 0, (LPARAM) szWallPaper);

    LoadString (hModule, DESKTOP + 5, szLHS, CharSizeOf(szLHS));
    if (GetProfileInt (szDesktop, szLHS, 1))
        CheckRadioButton (hDlg, IDD_CENTER, IDD_TILE, IDD_TILE);
    else
        CheckRadioButton (hDlg, IDD_CENTER, IDD_TILE, IDD_CENTER);

    SystemParametersInfo(SPI_GETFASTTASKSWITCH, 0, &wGridGran,FALSE);
    CheckDlgButton(hDlg, IDD_FASTSWITCH, wGridGran);

    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &wGridGran,FALSE);
    CheckDlgButton(hDlg, IDD_FULLDRAG, wGridGran);

    {
        // The font smoothing part of the dialog is initially
        // in the WS_DISABLED style. If the display has 16 or
        // more bits per pixel we shall enable the font smoothing

        HDC hdc;
        ULONG ul;
        int idChecked;


        if (hdc = GetDC(hDlg))
        {
            if (GetDeviceCaps(hdc, BITSPIXEL) >= 16)
            {
                HWND hwnd;
                ULONG ul;
                int idChecked;

                if (hwnd = GetDlgItem(hDlg, IDD_FS_GROUP))
                {
                    SetWindowLong(
                        hwnd,
                        GWL_STYLE,
                        GetWindowLong(hwnd, GWL_STYLE) & ~WS_DISABLED
                        );
                }
                if (hwnd = GetDlgItem(hDlg, IDD_FS_NONE))
                {
                    SetWindowLong(
                        hwnd,
                        GWL_STYLE,
                        GetWindowLong(hwnd, GWL_STYLE) & ~WS_DISABLED
                        );
                }
                if (hwnd = GetDlgItem(hDlg, IDD_FS))
                {
                    SetWindowLong(
                        hwnd,
                        GWL_STYLE,
                        GetWindowLong(hwnd, GWL_STYLE) & ~WS_DISABLED
                        );
                }
                if (hwnd = GetDlgItem(hDlg, IDD_FS_ENHANCED))
                {
                    SetWindowLong(
                        hwnd,
                        GWL_STYLE,
                        GetWindowLong(hwnd, GWL_STYLE) & ~WS_DISABLED
                        );
                }
                ul = SetFontEnumeration(0);                 // save state
                switch (ul & (FE_AA_ON | FE_AA_DEFAULT))
                {
                case FE_AA_ON: // minimal font smoothing

                    idChecked = IDD_FS;
                    break;

                case (FE_AA_ON | FE_AA_DEFAULT):   // enhanced font smoothing

                    idChecked = IDD_FS_ENHANCED;
                    break;

                case FE_AA_DEFAULT:            // bogus state FE_AA_ON
                                               // should have been set too!
                    ul &= ~FE_AA_DEFAULT;      // fix and fall through

                default:                       // no font smoothing

                    idChecked = IDD_FS_NONE;
                    break;
                }
                CheckRadioButton(hDlg, IDD_FS_NONE, IDD_FS_ENHANCED, idChecked);
                SetFontEnumeration(ul);                     // restore state
            }
        }
    }

    return;
}

void CursorSpeedInit (HWND hDlg)
{
    TCHAR szLHS[16];
    HWND hCursorScroll, hCursor;

    hCursorScroll = GetDlgItem (hDlg, DESKTOP_BLINK);

    //
    //  get scrollbar values
    //

    LoadString (hModule, DESKTOP + 7, szLHS, CharSizeOf(szLHS));
    CursorRate = (short) (CURSORSUM - GetProfileInt (szWindows, szLHS,
                                                CURSORMIN + CURSORRANGE / 2));

    //
    //  set up scrollbar
    //

    SetScrollRange (hCursorScroll, SB_CTL, CURSORMIN, CURSORMAX, FALSE);
    SetScrollPos (hCursorScroll, SB_CTL, CursorRate, TRUE);

    hCursor = GetDlgItem (hDlg, DESKTOP_CURSOR);
    GetWindowRect (hCursor, &rCursor);
    ScreenToClient (hDlg, (LPPOINT) &rCursor.left);
    ScreenToClient (hDlg, (LPPOINT) &rCursor.right);

    if (!SetTimer (hDlg, BLINK, CURSORSUM - CursorRate, NULL))
    {
        TCHAR    szErrClocks[133];

        LoadString (hModule, INITS + 5,  szErrClocks, CharSizeOf(szErrClocks));
        MessageBox (hDlg, szErrClocks, szCtlPanel, MB_OK | MB_ICONINFORMATION);
    }

    SetCaretBlinkTime ((WORD) (CURSORSUM - CursorRate));
    hbrBkgd = CreateSolidBrush (GetSysColor (COLOR_WINDOW));
    bCursorOn = FALSE;
}


//
// This routine builds the command line string needed to invoke
// the screen saver.  If the input is an Idlewild (*.IW) screen
// saver, then the command line must start with our default
// screen saver (SCRNSAVE.SCR) which will then invoke the Idlewild
// saver (by calling into IWLIB.DLL).  " /s" is appended if we want
// to see a demo of the screens saver, or " /c" if we want to
// configure it.
//
// this is really confused...  [stevecat - old comment from win 3.1]

void CreateSaverExecString (LPTSTR szTemp, PSSAVER pssSaver, WORD wParam)
{
    LPTSTR psz;

    //
    //  Check for IdleWild Screen Saver
    //

    if ((psz = _tcschr (pssSaver->szSaverPath, TEXT('.'))) && !lstrcmpi (psz, TEXT(".IW")))
    {
        //
        // Check to see if this is an 16-bit or 32-bit screen saver first,
        // then use the correct scrnsave.scr to invoke it.
        //

        if (pssSaver->dwFlags & SS_16BIT)
        {
            if (pss16bitDef != NULL)
            {
                lstrcpy (szTemp, pss16bitDef->szSaverPath);
                lstrcat (szTemp, TEXT(" "));
            }
        }
        else
        {
            //
            // Assume it is 32-bit and use old default of scrnsave.scr
            //

            if (pss32bitDef != NULL)
            {
                lstrcpy (szTemp, pss32bitDef->szSaverPath);
                lstrcat (szTemp, TEXT(" "));
            }
            else
                LoadString (hModule, DESKTOP + 17, szTemp, PATHMAX);
        }
        lstrcat (szTemp, pssSaver->szSaverPath);
    }
    else
        lstrcpy (szTemp, pssSaver->szSaverPath);

    if (wParam == DESKTOP_TEST)
        lstrcat (szTemp, TEXT(" /s"));            // save (run the thing)
    else if (wParam == DESKTOP_SETUP)
        lstrcat (szTemp, TEXT(" /c"));            // configure with CP as parent

    // USER will stick on the "/s" before execing the saver
}


BOOL CheckVal(HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID)
{
    WORD nVal;
    BOOL bOK;
    HWND hVal;

    if (wMin > wMax)
    {
        nVal = wMin;
        wMin = wMax;
        wMax = nVal;
    }

    nVal = (WORD)GetDlgItemInt(hDlg, wID, &bOK, FALSE);

    if (!bOK || (nVal < wMin) || (nVal > wMax))
    {
        MyMessageBox(hDlg, wMsgID, INITS+1, MB_OK|MB_ICONINFORMATION, wMin, wMax);
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)(hVal=GetDlgItem(hDlg, wID)), 1L);
//        SendMessage(hVal, EM_SETSEL, NULL, MAKELONG(0, 32767));
        SendMessage (hVal, EM_SETSEL, 0, 32767);
        return(FALSE);
    }

    return(TRUE);
}


BOOL DesktopDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    int     wBorderWidth;
    short   nDelta;
    TCHAR   szTemp[PATHMAX];
    TCHAR   szTemp2[40];
    HDC     hDC;
    int     nVal, nOldVal;
    BOOL    bOK;
    PSSAVER pssSaver;
    WORD    wIndex;
    WORD    nCtlId;

    LPARROWVSCROLL lpAVS;

    static HANDLE hSavers;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            HourGlass (TRUE);

            DragAcceptFiles (hDlg, TRUE);

            //  Init some global pointers for Screen Saver structs
            pssFirst    = NULL;
            pss16bitDef = NULL;
            pss32bitDef = NULL;

            /* This feature requires a mouse
             */
            if (!GetSystemMetrics(SM_MOUSEPRESENT))
                EnableWindow(GetDlgItem(hDlg, IDD_EDITPATTERN), FALSE);

            SendDlgItemMessage (hDlg, DESKTOP_SAVERTIME, EM_LIMITTEXT, 2, 0L);
            SendDlgItemMessage (hDlg, IDD_GRIDGRAN, EM_LIMITTEXT, 2, 0L);
            SendDlgItemMessage (hDlg, DESKTOP_BORDER, EM_LIMITTEXT, 2, 0L);

            SystemParametersInfo (SPI_GETBORDER, 0, (PVOID)(LPINT) &wBorderWidth, FALSE);
            SetDlgItemInt (hDlg, DESKTOP_BORDER, wBorderWidth, TRUE);

            GridInit (hDlg);
            avsIconGrid.bottom = (short) GSM (SM_CXICON);
            SystemParametersInfo (SPI_ICONHORIZONTALSPACING, 0, (PVOID)(LPINT) &nVal, FALSE);
            SetDlgItemInt (hDlg, IDD_ICONSPACE, nVal, TRUE);

            SystemParametersInfo (SPI_GETICONTITLEWRAP, 0, (PVOID)(LPINT) &nVal, FALSE);
            CheckDlgButton (hDlg, IDD_ICONWRAP, nVal);

            /* Get Screen Saver Information.  Added by C. Stevens, Oct. 90 */

            hSavers = GetDlgItem (hDlg, DESKTOP_SAVER);
            FindSavers (hSavers);
            SystemParametersInfo (SPI_GETSCREENSAVETIMEOUT, 0, (PVOID)(LPINT) &nVal, FALSE);
            nVal = (nVal + 59) / 60;
            SetDlgItemInt (hDlg, DESKTOP_SAVERTIME, nVal, TRUE);

            nVal = GetPrivateProfileInt (szBoot, szSecure, 0, szSystemIniPath);
            CheckDlgButton (hDlg, DESKTOP_SAVERPASSWD, nVal);

            // (None) is index 0

            wIndex = (WORD)SendMessage (hSavers, CB_GETCURSEL, 0, 0L);

            if (wIndex == 0)
            {
                EnableWindow (GetDlgItem (hDlg, DESKTOP_SETUP), FALSE);
                EnableWindow (GetDlgItem (hDlg, DESKTOP_TEST), FALSE);
            }

            OddArrowWindow (GetDlgItem (hDlg, DESKTOP_BDRSCROLL));
            OddArrowWindow (GetDlgItem (hDlg, IDD_GRIDGRANSCROLL));
            OddArrowWindow (GetDlgItem (hDlg, IDD_ICONSPACESCROLL));
            OddArrowWindow (GetDlgItem (hDlg, DESKTOP_SAVERSCROLL));
            CursorSpeedInit (hDlg);

            HourGlass (FALSE);
            break;
        }

    case WM_DESTROY:
        DragAcceptFiles (hDlg, FALSE);
        break;

    case WM_DROPFILES:
        DragQueryFile ((HANDLE)wParam, 0, szTemp, 40);
        SetDlgItemText (hDlg, IDD_WALLCOMBO, szTemp);
        DragFinish ((HANDLE)wParam);
        break;

    case WM_HSCROLL:
        switch (LOWORD(wParam))
        {
        case SB_THUMBPOSITION:
            nDelta = 0;
            CursorRate = HIWORD(wParam);
            break;

        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
            return (TRUE);
            break;

        default:
            CursorRate = ArrowVScrollProc (LOWORD(wParam), CursorRate,
                                                (LPARROWVSCROLL) &avsCursor);
            break;
        }

        SetScrollPos ((HWND) lParam, SB_CTL, CursorRate, TRUE);

        /* set system var */
        KillTimer (hDlg, BLINK);
        SetTimer (hDlg, BLINK, CURSORSUM - CursorRate, NULL);

        SetCaretBlinkTime ((WORD) (CURSORSUM - CursorRate));
        break;

    case WM_TIMER:
        if (wParam == BLINK)
        {
            hDC = GetDC (hDlg);
            if (hbrBkgd)
                FillRect (hDC, &rCursor, hbrBkgd);
            if (bCursorOn = !bCursorOn)
                InvertRect (hDC, &rCursor);
            ReleaseDC (hDlg, hDC);
            ValidateRect (hDlg, &rCursor);
        }
        break;

    case WM_VSCROLL:
        nCtlId = HIWORD (wParam) - (DESKTOP_BDRSCROLL - DESKTOP_BORDER);
        if (LOWORD(wParam) == SB_ENDSCROLL)
        {
            SendDlgItemMessage (hDlg, nCtlId, EM_SETSEL, 0, 32767);
            break;
        }
        switch (nCtlId)
        {

        /* Screen Saver scrollbar enabled by C. Stevens, Oct. 90 */

        case DESKTOP_SAVERTIME :
            lpAVS = (LPARROWVSCROLL) &avsSSMinutes;
            break;

        case DESKTOP_BORDER:
            lpAVS = (LPARROWVSCROLL) &avsWidthGrid;
            break;

        case IDD_GRIDGRAN:
            lpAVS = (LPARROWVSCROLL) &avsGridGran;
            break;

        case IDD_ICONSPACE:
            lpAVS = (LPARROWVSCROLL) &avsIconGrid;
            break;

        default:
            return (FALSE);
            break;
        }

        nOldVal = nVal = GetDlgItemInt (hDlg, nCtlId, &bOK, FALSE);
        if (!bOK && (( nVal < lpAVS->bottom) || (nVal > lpAVS->top)))
        {
            nVal = (int) lpAVS->thumbpos;
        }
        else
            nVal = (int) ArrowVScrollProc (LOWORD(wParam), (short) nVal, lpAVS);

        if ((nOldVal != nVal) || !bOK)
        {
            SetDlgItemInt (hDlg, nCtlId, nVal, FALSE);
        }
        SetFocus (GetDlgItem (hDlg, nCtlId));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_EDITPATTERN:
            {
                DWORD dwSave;

                dwSave = dwContext;
                dwContext = IDH_DLG_PATTERN;

                DialogBox (hModule, (LPTSTR) MAKEINTRESOURCE(DLG_PATTERN), hDlg,
                                                    (DLGPROC)PatternDlgProc);
                dwContext = dwSave;
            }
            break;

        case IDD_CENTER:
        case IDD_TILE:
            CheckRadioButton (hDlg, IDD_CENTER, IDD_TILE, LOWORD(wParam));
            break;

        case PUSH_OK:
            {
                wIndex = (WORD) SendMessage (hSavers, CB_GETCURSEL, 0, 0L);

                /* Only check SAVERTIME if there is a screen saver selected
                 */
                if (wIndex && !CheckVal(hDlg, DESKTOP_SAVERTIME, SSDLYMIN, SSDLYMAX,
                                                                DESKTOP+14))
                    break;
                if (!CheckVal(hDlg, DESKTOP_BORDER, WIDTHMIN, WIDTHMAX,
                                                                DESKTOP+8 ))
                    break;
                if (!CheckVal(hDlg, IDD_GRIDGRAN   , GRIDMIN , GRIDMAX ,
                                                                DESKTOP+2 ))
                    break;
                if (!CheckVal(hDlg, IDD_ICONSPACE  , avsIconGrid.top ,
                                            avsIconGrid.bottom , DESKTOP+12))
                    break;

                HourGlass (TRUE);

                if (!GridOK (hDlg))
                    break;

                /* Write the Border Width */
                nVal = GetDlgItemInt (hDlg, DESKTOP_BORDER, (BOOL *) &bOK, FALSE);
                if (bOK)
                {
                    SystemParametersInfo (SPI_SETBORDER, (WORD) nVal, 0L, TRUE);
                }
                /* Write the cursor  Speed value */
                MyItoa (CURSORSUM - CursorRate, szTemp, 10);
                LoadString (hModule, DESKTOP + 7, szTemp2, CharSizeOf(szTemp2));
                WriteProfileString (szWindows, szTemp2, szTemp);

                /* Update the Screen Saver Info. Added by C. Stevens, Oct. 90 */

                wIndex = (WORD) SendMessage (hSavers, CB_GETCURSEL, 0, 0L);
                pssSaver = (PSSAVER) SendMessage (hSavers, CB_GETITEMDATA, wIndex, 0L);

                if (pssSaver && (pssSaver != (PSSAVER)CB_ERR) &&
                    (pssSaver != (PSSAVER)CB_ERRSPACE))
                {
                    CreateSaverExecString (szTemp, pssSaver, 0);
                    WritePrivateProfileString (szBoot, szSCRNSAVEEXE, szTemp, szSystemIniPath);

                    //  Set the "ScreenSaverIsSecure" value for password
                    //  protection of Screen Saver

                    if (!((pssSaver->dwFlags & SS_16BIT) ||
                          (pssSaver->dwFlags & SS_16BITDEF)))
                    {
                        // This is NOT a 16-bit screens saver, and therefore
                        // it may have Password Protection
                        if (IsDlgButtonChecked (hDlg, DESKTOP_SAVERPASSWD))
                        {
                            WritePrivateProfileString (szBoot, szSecure, TEXT("1"),
                                                       szSystemIniPath);
                        }
                        else
                        {
                            WritePrivateProfileString (szBoot, szSecure, TEXT("0"),
                                                       szSystemIniPath);
                        }
                    }
                    else
                    {
                        //  This is a 16-bit screen saver, it cannot have
                        //  Password protection
                        WritePrivateProfileString (szBoot, szSecure, TEXT("0"),
                                                   szSystemIniPath);
                    }
#if 0
                    if (lstrcmp (pssSaver->szSaverPath, szNone))
                        SystemParametersInfo (SPI_SETSCREENSAVEACTIVE, TRUE, 0L, TRUE);
                    else
                        SystemParametersInfo (SPI_SETSCREENSAVEACTIVE, FALSE, 0L, TRUE);
#else
                    // use the fact that (None) is at index 0
                    SystemParametersInfo (SPI_SETSCREENSAVEACTIVE, (WORD) (wIndex != 0), 0L, TRUE);
#endif
                }

                nVal = GetDlgItemInt (hDlg, DESKTOP_SAVERTIME, (BOOL *) &bOK, FALSE);
                nVal *= 60;             // convert to seconds

                //  Tell user to read pattern out of win.ini
                SystemParametersInfo (SPI_SETDESKPATTERN, (UINT)-1, 0L, FALSE);

                //
                //  Note that the last SystemParametersInfo call must have the
                //  SPIF_SENDWININICHANGE flag.
                //

                SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, nVal, NULL,
                                     SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

                HourGlass (FALSE);
            }

            // fall through...

        case PUSH_CANCEL:
            if (hbrBkgd)
                DeleteObject (hbrBkgd);
            KillTimer (hDlg, BLINK);

#ifdef JAPAN
            //Rest Cursor Speed yutakas 1992.10.21
            {
                TCHAR szLHS[16];

                LoadString(hModule, DESKTOP+7, szLHS, CharSizeOf(szLHS));
                SetCaretBlinkTime(GetProfileInt(szWindows, szLHS,
                                        CURSORMIN + CURSORRANGE/2));
            }
#endif

            // Free memory handles associated with screen savers

            FreeSavers (hSavers);
            EndDialog (hDlg, 0L);
            break;

            /* The following cases added to provide screen saver support.
              Added by C. Stevens, Oct. 90 */

        case DESKTOP_TEST:
            // run the thing now
        case DESKTOP_SETUP:
            // configure the thing
            {
                MSG         Msg;
                STARTUPINFO StartupInfo;
                PROCESS_INFORMATION ProcessInformation;
                BOOL b;
                HWND hwndFocus;

                wIndex = (WORD) SendMessage (hSavers, CB_GETCURSEL, 0, 0L);
                pssSaver = (PSSAVER) SendMessage (hSavers, CB_GETITEMDATA,
                                                  wIndex, 0L);
                if (pssSaver)
                {
                    if (wIndex == 0)
                        // (None) is at index zero
                        break;
#if 0
                    /* Do not execute "(None)" */
                    if (!lstrcmp (pssSaver->szSaverPath, szNone))
                        break;
#endif
                    CreateSaverExecString (szTemp, pssSaver, LOWORD(wParam));

//                    WinExec (szTemp, SW_SHOW);

                    // Create screen saver process
                    memset (&StartupInfo, 0, sizeof(StartupInfo));
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.wShowWindow = SW_SHOW;

                    // start the screen saver at normal priority so
                    // it'll run without being pre-empted so the user
                    // can check it out.

                    b = CreateProcess ( NULL,
                                        szTemp,         // CommandLine
                                        NULL,
                                        NULL,
                                        FALSE,
                                        NORMAL_PRIORITY_CLASS,
                                        NULL,
                                        NULL,
                                        &StartupInfo,
                                        &ProcessInformation
                                        );
                    // If process creation successful, wait for it to
                    // complete before continuing
                    if ( b )
                    {
                        // Save keyboard focus
                        hwndFocus = GetFocus ();

                        // Wait until other process ready for User input
                        WaitForInputIdle (ProcessInformation.hProcess,
                                          INFINITE);


                        // HACK: Clean-events prior to attaching thread-input.
                        // There is a known problem where the deactivation
                        // is processed after the queues have been attached,
                        // and this causes the screen-saver to terminate
                        // prematurely.  Will look at this after PPC.  We
                        // were just lucky lucky timing worked out before.
                        //
                        // 12-08-94 : ChrisWil.
                        //
                        while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
                        {
                            TranslateMessage (&Msg);
                            DispatchMessage (&Msg);
                        }


                        //  Force USER to treat Control Panel and Screen Saver
                        //  process as a single process for input
                        AttachThreadInput (GetCurrentThreadId (),
                                           ProcessInformation.dwThreadId,
                                           TRUE);

                        EnableWindow (hDlg, FALSE);
                        while (MsgWaitForMultipleObjects (
                                              1,
                                              &ProcessInformation.hProcess,
                                              FALSE,
                                              (DWORD)(LONG)-1,
                                              QS_ALLEVENTS | QS_SENDMESSAGE) != 0)
                        {
                            // This message loop is a duplicate of main
                            // message loop with the exception of using
                            // PeekMessage instead of waiting inside of
                            // GetMessage.  Process wait will actually
                            // be done in MsgWaitForMultipleObjects api.
                            //
                            while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
                            {
                                TranslateMessage (&Msg);
                                DispatchMessage (&Msg);
                            }

                        }

                        //  Uncouple Control Panel from screen saver thread
                        AttachThreadInput (GetCurrentThreadId (),
                                           ProcessInformation.dwThreadId,
                                           FALSE);

                        // Close handles and re-enable window
                        CloseHandle (ProcessInformation.hProcess);
                        CloseHandle (ProcessInformation.hThread);
                        EnableWindow (hDlg, TRUE);

                        // Bring Desktop window to foreground, always
                        SetForegroundWindow (hDlg);

                        // Restore keyboard focus in desktop applet
                        SetFocus (hwndFocus);
                    }
                }
                break;
            }

        case DESKTOP_SAVER:
            {
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {

                    /* If user has selected "(None)", disable test and
                       setup buttons */

                    wIndex = (WORD) SendMessage (hSavers, CB_GETCURSEL, 0, 0L);
                    pssSaver = (PSSAVER) SendMessage (hSavers, CB_GETITEMDATA,
                                                      wIndex, 0L);
#if 0
                    SendMessage (hSavers, CB_GETLBTEXT, wIndex, (LPARAM) szTemp);

                    if (lstrcmp (szTemp, szNone))
                    {
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_TEST), TRUE);
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_SETUP), TRUE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_TEST), FALSE);
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_SETUP), FALSE);
                    }
#else
                    EnableWindow (GetDlgItem (hDlg, DESKTOP_TEST), wIndex);
                    EnableWindow (GetDlgItem (hDlg, DESKTOP_SETUP), wIndex);

                    //  Disable password control if 16-bit screen saver

                    if ((pssSaver->dwFlags & SS_16BIT) || (pssSaver->dwFlags & SS_16BITDEF))
                    {
                        // Clear and disable "Password Protected" checkbox
                        CheckDlgButton (hDlg, DESKTOP_SAVERPASSWD, 0);
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_SAVERPASSWD), FALSE);
                    }
                    else
                    {
                        // This is NOT a 16-bit screen saver
                        EnableWindow (GetDlgItem (hDlg, DESKTOP_SAVERPASSWD), TRUE);
                    }
#endif
                    /* if user has not selected (none) and the delay is 0,
                     * set it to 2
                     */
                    if (SendDlgItemMessage(hDlg, LOWORD(wParam), CB_GETCURSEL, 0, 0L)!=0
                        && GetDlgItemInt(hDlg, DESKTOP_SAVERTIME, &bOK, FALSE)==0
                        && bOK)
                      SetDlgItemInt(hDlg, DESKTOP_SAVERTIME, 2, FALSE);
                }
                break;
            }

        case IDD_FS_NONE:
        case IDD_FS:
        case IDD_FS_ENHANCED:

            CheckRadioButton(hDlg, IDD_FS_NONE, IDD_FS_ENHANCED, LOWORD(wParam));
            break;

        case IDD_HELP:
          goto DoHelp;

        default:
          break;
        }
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
        }
        else
            return FALSE;
        break;
    }
  return(TRUE);
}


/* The following function searches the Windows directory and System
   subdirectory for Screen Savers.  These files are identified by the
   extension contained in SAVERSPEC (currently defined as "*.SCR").  The
   function SaverDirSearch () is called to search the subdirectories and
   add any screen savers to the Screen Saver Combobox.

   Written by C. Stevens, Oct. 90

   PARAMETERS:

   hCombo - Handle to the Screen Saver Combobox */

void FindSavers (HANDLE hCombo)
{
    TCHAR   szSaverPath[120];   /* Path to currently selected Screen Saver */
    TCHAR   szSaverSpec[10];    /* Pattern to match during file search */
    LPTSTR  pszComboPath;       /* Path associated with current Combobox entry */
    WORD    wCount;             /* Number of entries in Combobox */
    WORD    wIndex;             /* Current Combobox entry */
    WORD    wIndexBackup;       /* Backup value for Current Combobox entry */

    register short  i;         /* Loop index */
    TCHAR   *pch;              /* Pointer into szSaverPath */
    TCHAR    szNone[NONE_LENGTH];
    UINT     uError;
    SSAVER  *pssSaver;          /* Handle associated with Combobox entry */
    BOOL     bIdleWild;
    HWND     hDlg;


    LoadString (hModule, DESKTOP, szNone, CharSizeOf(szNone));

    //  Get screen saver extension to search for
    LoadString (hModule, DESKTOP + 15, szSaverSpec, CharSizeOf(szSaverSpec));

    //  Disable WIN32 Error Popup Message box on attempt to load 16-bit .SCR's
    uError = SetErrorMode (SEM_FAILCRITICALERRORS);

    /* Search directories for files with Screen Saver extension */

    SaverDirSearch (hCombo, pszSysDir, szSaverSpec, FALSE);
    SaverDirSearch (hCombo, pszWinDir, szSaverSpec, FALSE);

    /* Search for IWLIB.DLL in current path (windows and windows\system are
     * included in this search)
     */
    //////////////////////////////////////////////////////////////////////////
    //+++ FUTURE - 32-bit
    //    Look for 32-bit IWLIB(32).DLL in Windows and System32 dirs, if not
    //    found, make sure 16-bit def scrnsave.scr available before listing
    //    *.IW screen savers.
    //////////////////////////////////////////////////////////////////////////

    //  Look for IWLIB.DLL for IdleWild screen saver support and
    //  also make sure 16-bit scrnsave.scr is available before listing these

    LoadString (hModule, DESKTOP + 16, szSaverSpec, CharSizeOf(szSaverSpec));
    if (MyOpenFile (TEXT("IWLIB.DLL"), szSaverPath, OF_EXIST) != INVALID_HANDLE_VALUE &&
        (pss16bitDef != NULL))
    {
        CharUpper (szSaverPath);

        if (!(pch = _tcsrchr (szSaverPath, TEXT('\\'))))
        {
            if (pch = _tcsrchr (szSaverPath, TEXT(':')))
                ++pch;
            else
                pch = szSaverPath;
        }
        *pch = TEXT('\0');

        SaverDirSearch (hCombo, szSaverPath, szSaverSpec, TRUE);
    }

    //  Restore prior error mode
    SetErrorMode (uError);

    /* Add "(None)" option to disable screen save, this will be the first
      item in the list box (index 0) because it is sorted! */

    //////////////////////////////////////////////////////////////////////////
    //  Create a dummy SS struct for NONE screen saver entry
    //////////////////////////////////////////////////////////////////////////

    // We found a valid SS file
    //
    // Allocate memory for its' data struct, init fields,
    // save path, set flags and put in ComboBox

    pssSaver = (SSAVER *) AllocMem (sizeof(SSAVER));

    if (!pssSaver)
        return;

    pssSaver->dwFlags = SS_NONE;

    lstrcpy (pssSaver->szSaverPath, szNone);

    // Always force "(None)" entry to top of list
    wIndex = (WORD) SendMessage (hCombo, CB_INSERTSTRING, 0, (LPARAM)szNone);
    SendMessage (hCombo, CB_SETITEMDATA, wIndex, (LPARAM) pssSaver);

    // Attempt to find the current screen saver in the listbox and set this
    // as the current listbox selection

    GetPrivateProfileString (szBoot, szSCRNSAVEEXE, szNone, szSaverPath, CharSizeOf(szSaverPath), szSystemIniPath);

    //  Convert to uppercase for case-insensitive comparison later
    //  Note:  Uppercase is a better choice in UNICODE environments

    _tcsupr (szSaverPath);

    //  Check if this is an IdleWild screen saver
    if (_tcsstr (szSaverPath, TEXT(".IW")))
        bIdleWild = TRUE;
    else
        bIdleWild = FALSE;

    //  Create a backup value in case 32-bit SS not found
    wIndexBackup = 0;

    wCount = (WORD) SendMessage (hCombo, CB_GETCOUNT, 0, 0L);
    for (i = 0; i < (short) wCount; i++)
    {
        pssSaver = (PSSAVER) SendMessage (hCombo, CB_GETITEMDATA, i, 0L);
        if (pssSaver)
        {
            //  Get struct pointer

            //  First check to see if this is an IdleWild screen saver.
            //  If it is, ignore matches on 16/32BITDEF scrnsave.scr
            //  and continue search for matching *.IW file/path.

            if (bIdleWild && ((pssSaver->dwFlags & SS_32BITDEF) ||
                              (pssSaver->dwFlags & SS_16BITDEF)))
            {
                continue;
            }

            pszComboPath = (LPTSTR) pssSaver->szSaverPath;

            if (pszComboPath)
            {
                //  Try finding args both ways:
                //
                //  1st case: szSaverPath may only be a *.scr file and
                //            pszComboPath is always a fully qualified
                //            pathname of screen saver
                //
                //  2nd case: szSaverPath may be the full path name, including
                //            the default scrnsave.scr in order to invoke an
                //            IdleWild screen saver.  In this case, pszComboPath
                //            would be a substring of the "Exec string" for that
                //            screen saver
                if (_tcsstr (pszComboPath, szSaverPath) ||
                    _tcsstr (szSaverPath, pszComboPath))
                {
                    // Check to see if this is the 32-bit version of screen saver
                    if ((pssSaver->dwFlags & SS_32BIT) ||
                        (pssSaver->dwFlags & SS_32BITDEF))
                    {
                        wIndex = i;
                        break;
                    }
                    else
                    {
                        wIndexBackup = i;
                    }
                }
            }
        }
    }

    // This applies if we could not find the 32-bit version of screen saver
    if (wIndex == 0)
        wIndex = wIndexBackup;

    // Set the default selection (incl. "Password Protect" control)

    SendMessage (hCombo, CB_SETCURSEL, wIndex, 0L);

    pssSaver = (PSSAVER) SendMessage (hCombo, CB_GETITEMDATA, wIndex, 0L);

    hDlg = GetParent (hCombo);

    if ((pssSaver->dwFlags & SS_16BIT) || (pssSaver->dwFlags & SS_16BITDEF))
    {
        // Clear and disable "Password Protected" checkbox
        CheckDlgButton (hDlg, DESKTOP_SAVERPASSWD, 0);
        EnableWindow (GetDlgItem (hDlg, DESKTOP_SAVERPASSWD), FALSE);
    }
    else
    {
        // This is NOT a 16-bit screen saver
        EnableWindow (GetDlgItem (hDlg, DESKTOP_SAVERPASSWD), TRUE);
    }
}


#if 0       //+++ [stevecat]
/* The following function adds a Screen Saver entry to the Screen Saver
   Combobox.  Each Combobox entry contains the description of the Screen
   Saver in its text field, and a handle used to reference the pathname
   of the Screen Saver executable.

   Written by C. Stevens, Oct. 90

   PARAMETERS:

   hCombo     - Handle to the Screen Saver Combobox
   pSaverName - Description of the Screen Saver
   pSaverPath - Pathname of the Screen Saver executable

   RETURN:  This function returns the index of the Combobox item just
            created.  The function returns -1 if unsuccessful */

int AddSaverName(HANDLE hCombo, PSSAVER pSaver, LPTSTR pSaverPath)
{
    HANDLE hSaverItem;    /* Handle to memory for Screen Saver description */
    LPTSTR pComboItem;    /* Combobox copy of Screen Saver description */
    int    Index = -1;    /* Index of current Combobox entry */
    SSAVER *pssSaver;

    //  Allocate struct pointer
    hSaverItem = pssSaver = (SSAVER *) AllocMem (sizeof(SSAVER));
    pssSaver->dwFlags = 0L;
    pssSaver->szSaverPath[0] = TEXT('\0');

    if (hSaverItem)
    {
        pComboItem = (LPTSTR) pssSaver->szSaverPath;
        if (pComboItem)
        {
            lstrcpy (pComboItem, pSaverPath);

            // Make path name case insensitive (for later comparison)
            //  Note:  Uppercase is a better choice in UNICODE environments

            _tcsupr (pComboItem);
#ifdef JAPAN
            Index = SendMessage( hCombo, CB_INSERTSTRING, 0, (LPARAM)pSaverName);
#else
            Index = SendMessage (hCombo, CB_ADDSTRING, 0, (LPARAM)pSaverName);
#endif

            SendMessage (hCombo, CB_SETITEMDATA, Index, (LPARAM) hSaverItem);
        }
//+++        LocalUnlock (hSaverItem);
    }
    return Index;
}
#endif      //+++ [stevecat]


/* The following function extracts a description of the Screen Saver from
   the DESCRIPTION field of the executable.

   Stolen from Control Panel - CPFONT3.C, Oct. 90

   PARAMETERS:

   pSaverName - The Screen Saver description extracted from the
                   DESCRIPTION field of the exeutable
   pSaverPath - Path of the Screen Saver executable file
   bAcceptAny - Flag indicating accept any description string

   RETURN:  This function returns TRUE when a description is successfully
            extracted from the Screen Saver executable

            0 - Cannot extract description from Screen Saver executable
            1 - Success on 32-bit Screen Saver executable
            2 - Success on 16-bit Screen Saver executable
*/

int GetSaverName (LPTSTR pSaverName, LPTSTR pSaverPath, BOOL bAcceptAny)
{
    LPTSTR  pLastBackSlash;
    HANDLE  hSaver;

    if (hSaver = LoadLibrary (pSaverPath))
    {
        if (!LoadString (hSaver, SAVERDESC, pSaverName, PATHMAX))
        {
            // In case of failure use the base filename as a description
            // ...get rid of filename extension
            pLastBackSlash = _tcsrchr(pSaverPath, TEXT('.'));
            pLastBackSlash = TEXT('\0');

            if (pLastBackSlash = _tcsrchr(pSaverPath, TEXT('\\')))
                lstrcpy (pSaverName, pLastBackSlash+1);
            else
                lstrcpy (pSaverName, pSaverPath);
        }
        FreeLibrary (hSaver);
        return GSN_SS_32BIT;
    }
    else
    {
        // 16-bit screen saver .EXE support code

        //  Verify 16-bit .EXE format and extract name os SS from it

        BOOL bResult = FALSE;    /* Description successfully extracted */
        struct exe_hdr oeHeader; /* Old style executable header structure */
        struct new_exe neHeader; /* New style executable header structure */
        HANDLE fh;               /* File handle to Screen Saver executable */
        BYTE cBytes;             /* Byte count (ANSI string length) */
        TCHAR szSaverName[120];  /* local Screen Saver description */
        long lNewHeader;         /* Byte count (structure length) */
        int nPos;                /* Used to strip extra crap from pSaverName */

        /* We always set pSaverName if we return TRUE */
        *pSaverName = TEXT('\0');

        if ((fh = MyOpenFile(pSaverPath, NULL, OF_READ)) != INVALID_HANDLE_VALUE)
        {
            /* Read old style header structure */

            MyByteReadFile (fh, &oeHeader, sizeof(oeHeader));

            /* Check for old style header */

            if (oeHeader.e_magic == EMAGIC && oeHeader.e_lfanew)
                lNewHeader = oeHeader.e_lfanew;
            else
                lNewHeader = 0L;

            MyFileSeek (fh, lNewHeader, 0);

            /* Read new style header structure */

            MyByteReadFile (fh, &neHeader, sizeof(neHeader));

            /* Verify new style header */

            if (neHeader.ne_magic == NEMAGIC)
            {
                /* Find file description location */

                MyFileSeek (fh, neHeader.ne_nrestab, 0);

                /* Read description size */

                MyByteReadFile (fh, &cBytes, 1);
                cBytes = ((int) cBytes < 119) ? cBytes : (BYTE) 119;

                /* Read file description */

                MyAnsiReadFile (fh, CP_ACP, szSaverName, cBytes);
                szSaverName[cBytes] = (TCHAR) 0;

                /* Verify SCRNSAVE before description and convert to ANSI */

                if (szSaverName[0] != TEXT('\0'))
                {
                    if (!_tcsncmp (szSaverName, TEXT("SCRNSAVE"), 8))
                    {
                        nPos = lstrpos (&szSaverName[8], TEXT(':'));

#ifdef UNICODE
                        lstrcpy (pSaverName, &szSaverName[9 + nPos]);
#else
                        OemToAnsi (&szSaverName[9 + nPos], pSaverName);
#endif

                        /* Strip extra crap from description */

                        nPos = lstrpos (pSaverName, TEXT(':'));
                        if (nPos)
                           pSaverName[nPos] = (TCHAR) 0;

                        StripBlanks (pSaverName);
                        bResult = TRUE;
                    }
                    else if (bAcceptAny)
                    {
#ifdef UNICODE
                        lstrcpy (pSaverName, szSaverName);
#else
                        OemToAnsi(szSaverName, pSaverName);
#endif
                        bResult = TRUE;
                    }
                }
            }
            CloseHandle (fh);
        }

        //  Postfix " (16-bit)" to description
        if (bResult)
        {
            LoadString (hModule, DESKTOP + 18, szSaverName, CharSizeOf(szSaverName));
            lstrcat (pSaverName, szSaverName);
        }

        return bResult ? GSN_SS_16BIT : GSN_SS_UNK;
    }

    return GSN_SS_UNK;
}


/* The following function searches a directory (non-recursively) for files
   matching SAVERSPEC (currently defined as "*.SCR").  The description of
   each Screen Saver is found by calling GetSaverName ().  The Screen Saver
   is added to the Combobox by calling AddSaverName ().

   Written by C. Stevens, Oct. 90

   PARAMETERS:

   hCombo     - Handle to Screen Saver Combobox
   lpszPath   - Directory in which to search for files
   lpszSpec   - Pattern to match for file search
   bAcceptAny - Flag indicating to accept any saver name */

void SaverDirSearch (HANDLE hCombo, LPTSTR pPath, LPTSTR pSpec, BOOL bAcceptAny)
{
    BOOL   bFound;                 // Flag indicating file match found
    TCHAR  szNewPath[PATHMAX];     // Screen Saver path and filename
    TCHAR  szMessage[PATHMAX];
    TCHAR  szDesc[128];            // Description string storage
    HANDLE fFile;
    int    rc, Index;
    SSAVER *pssSaver;

    WIN32_FIND_DATA FindFileData;


    CharUpper (pPath);
    CharUpper (pSpec);

    /* Concatenate file spec onto path string */

    lstrcpy (szNewPath, pPath);

    BackslashTerm (szNewPath);

    /* Search for the first file satisfying the file specs */

    lstrcpy (szMessage, szNewPath);
    lstrcat (szMessage, pSpec);

    if ((fFile = FindFirstFile (szMessage, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        bFound = TRUE;

        while (bFound)
        {
            /* Add the file to the ComboBox */
            if (FindFileData.cFileName[0] != TEXT('.'))
            {
                //  Create fully qualified pathname

                lstrcpy (szMessage, szNewPath);
                lstrcat (szMessage, FindFileData.cFileName);

                if (rc = GetSaverName (szDesc, szMessage, bAcceptAny))
                {
                    // Determine SS type, set flags, add item to combobox
                    //
                    // We found a valid SS file
                    //
                    // Allocate memory for its' data struct, init fields,
                    // save path, set flags and put in ComboBox

                    pssSaver = (SSAVER *) AllocMem (sizeof(SSAVER));

                    if (!pssSaver)
                        return;

                    pssSaver->dwFlags = 0L;
                    pssSaver->szSaverPath[0] = TEXT('\0');

                    //  Find "Default" screen savers

                    switch (rc)
                    {
                       case GSN_SS_32BIT:     // 32-bit Screen Saver
                           if (!lstrcmpi (FindFileData.cFileName, TEXT("SCRNSAVE.SCR")))
                           {
                               pssSaver->dwFlags = SS_32BITDEF;
                               pss32bitDef = pssSaver;
                           }
                           else
                               pssSaver->dwFlags = SS_32BIT;
                           break;

                       case GSN_SS_16BIT:     // 16-bit Screen Saver
                           if (!lstrcmpi (FindFileData.cFileName, sz16bitDefSaver))
                           {
                               pssSaver->dwFlags = SS_16BITDEF;
                               pss16bitDef = pssSaver;
                           }
                           else
                               pssSaver->dwFlags = SS_16BIT;
                           break;

                       default:
                           pssSaver->dwFlags = SS_NONE;
                           break;
                    }

                    // Make path name case insensitive (for later comparison)
                    //  Note:  Uppercase is a better choice in UNICODE environments

                    // [stevecat] 12/29/93 - Changed to NOT store full pathname of
                    // of SS in registry because floating user profiles and the
                    // registry (for machine independence) do not like them.
                    // _tcsupr (szMessage);
                    // lstrcpy (pssSaver->szSaverPath, szMessage);

                    _tcsupr (FindFileData.cFileName);
                    lstrcpy (pssSaver->szSaverPath, FindFileData.cFileName);

                    Index = SendMessage (hCombo, CB_ADDSTRING, 0, (LPARAM)szDesc);
                    SendMessage (hCombo, CB_SETITEMDATA, Index, (LPARAM) pssSaver);
                }
            }
            /* Search for next file */
            bFound = FindNextFile (fFile, &FindFileData);
        }
        FindClose (fFile);
    }
}

/*============================================================================
;
; FreeSavers
;
; The following function frees the local handles associated with strings
; stored in the screen savers combobox.
;
; Parameters:
;
; hSavers - Handle to screen saver combobox
;
; Return Value: None
;
============================================================================*/

void FreeSavers(HANDLE hSavers)
{
    WORD wCount;     /* Number of items in combobox */
    register WORD i; /* Counter */
    HANDLE hTemp;    /* Handle stored as combobox itemdata */

    wCount = (WORD) SendMessage (hSavers, CB_GETCOUNT, 0, 0L);
    for (i = 0; i < wCount; i++)
    {
        hTemp = (HANDLE) SendMessage (hSavers, CB_GETITEMDATA, i, 0L);
        if (hTemp)
            FreeMem ((LPVOID) hTemp, sizeof(SSAVER));
    }
}


int lstrpos(LPTSTR string, TCHAR chr)
{
    LPTSTR lpTmp;

    if (!(lpTmp = _tcschr(string, chr)))
        return(-1);
    else
        return (lpTmp - string);
}




