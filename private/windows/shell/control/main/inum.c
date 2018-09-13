/** FILE: inum.c *********** Module Header ********************************
 *
 *  Control panel applet for International configuration.  This file holds
 *  everything to do with the Number dialog box inside the International
 *  Dialog in Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================


//==========================================================================
//                            External Declarations
//==========================================================================

extern TCHAR    *pszNegNumPat[];


//==========================================================================
//                            Local Data Declarations
//==========================================================================


//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

BOOL ExistDigits(
    TCHAR *pszString)
{
    while (*pszString)
    {
        if (_istdigit(*pszString))
        {
            return (TRUE);
        }
        pszString++;
    }
    return (FALSE);
}


void SetupNegativeNumCB(
    HWND hCB,
    TCHAR *szThouSep,
    TCHAR *szDecSep,
    int nDecimal)
{
    TCHAR   szFormat[30];
    TCHAR   szTemp[20];
    TCHAR   *pch;
    TCHAR   ch;
    int     i;


    if (nDecimal > MAX_DEC_DIGITS)
    {
        nDecimal = MAX_DEC_DIGITS;
    }
    ch = (TCHAR) (TEXT('0') + nDecimal);

    if (nDecimal)
    {
        wsprintf (szTemp, TEXT("1%s234%s"), szThouSep, szDecSep);
        for (i = 0, pch = szTemp + lstrlen (szTemp); i < nDecimal; i++)
        {
            *pch++ = ch;
        }
        *pch = TEXT('\0');
    }
    else
    {
        wsprintf (szTemp, TEXT("1%s234"), szThouSep);
    }

    SendMessage (hCB, CB_RESETCONTENT, 0, 0L);

    for (i = 0; i < NUM_NEGNUM_PAT; i++)
    {
        wsprintf(szFormat, pszNegNumPat[i], (LPTSTR)szTemp);

        SendMessage(hCB, CB_ADDSTRING, 0, (LONG)(LPTSTR)szFormat);
    }

    return;
}


BOOL APIENTRY NumIntlDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HWND  hCB;
    BOOL bOK;
    int  nDigits;
    short nIndex;
    TCHAR szThousand[2];
    TCHAR szDecimal[2];
    static BOOL bSepChange;


    switch (message)
    {
    case WM_INITDIALOG:
        HourGlass (TRUE);

        CheckRadioButton (hDlg, NUM_NOLEAD0, NUM_LEAD0, NUM_NOLEAD0 + Current.iLzero);

        SetDlgItemText (hDlg, NUM_1000SEP, Current.sThousand);
        SendDlgItemMessage (hDlg, NUM_1000SEP, EM_LIMITTEXT, 1, 0L);

        SetDlgItemText (hDlg, NUM_DECSEP, Current.sDecimal);
        SendDlgItemMessage (hDlg, NUM_DECSEP, EM_LIMITTEXT, 1, 0L);

        SetDlgItemInt (hDlg, NUM_DECDIGITS, Current.iDigits, FALSE);
        SendDlgItemMessage (hDlg, NUM_DECDIGITS, EM_LIMITTEXT, 1, 0L);

        hCB = GetDlgItem (hDlg, NUM_NEG);
        SetupNegativeNumCB (hCB, Current.sThousand, Current.sDecimal, Current.iDigits);
        SendMessage (hCB, CB_SETCURSEL, Current.iNegNumber, 0L);

        bSepChange = FALSE;
        HourGlass (FALSE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case NUM_NOLEAD0:
        case NUM_LEAD0:
            CheckRadioButton (hDlg, NUM_NOLEAD0, NUM_LEAD0, LOWORD(wParam));
            break;

        case NUM_DECDIGITS:
        case NUM_1000SEP:
        case NUM_DECSEP:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bSepChange = TRUE;
            }
            else if (bSepChange && (HIWORD(wParam) == EN_KILLFOCUS))
            {
                bSepChange = FALSE;

                nDigits = GetDlgItemInt (hDlg, NUM_DECDIGITS, &bOK, FALSE);

                if (!GetDlgItemText(hDlg, NUM_1000SEP, szThousand, CharSizeOf(Current.sThousand)))
                {
                    *szThousand = 0;
                }
                if (!GetDlgItemText(hDlg, NUM_DECSEP, szDecimal, CharSizeOf(Current.sDecimal))
                    && (nDigits != 0))
                {
                    break;
                }

                hCB = GetDlgItem (hDlg, NUM_NEG);
                nIndex = (short) SendMessage (hCB, CB_GETCURSEL, 0L, 0L);
                SetupNegativeNumCB (hCB, szThousand, szDecimal, nDigits);
                SendMessage (hCB, CB_SETCURSEL, nIndex, 0L);
            }
            break;

        case NUM_NEG:
            break;

        case PUSH_OK:
            nDigits = GetDlgItemInt (hDlg, NUM_DECDIGITS, &bOK, FALSE);
            if (!bOK)
            {
                LoadString (hModule, INTL + 1, szGenErr, CharSizeOf(szGenErr));
                MessageBox (hDlg, szGenErr, szCtlPanel, MB_OK | MB_ICONINFORMATION);
                SetFocus (GetDlgItem (hDlg, NUM_DECDIGITS));
                SendDlgItemMessage (hDlg, NUM_DECDIGITS, EM_SETSEL, 0, 32767);
                break;
            }

            if ((!GetDlgItemText (hDlg, NUM_DECSEP, szDecimal, CharSizeOf(szDecimal))
                 && (nDigits != 0)) || ExistDigits(szDecimal))
            {
                /*
                 *  Null decimal separator only valid if number of decimal
                 *  digits is zero.
                 *
                 *  Digits are invalid in separator.
                 */
                LoadString (hModule, INTL, szGenErr, CharSizeOf(szGenErr));
                MessageBox (hDlg, szGenErr, szCtlPanel, MB_OK | MB_ICONINFORMATION);
                SetFocus (GetDlgItem (hDlg, NUM_DECSEP));
                break;
            }

            GetDlgItemText (hDlg, NUM_1000SEP, szThousand, CharSizeOf(Current.sThousand));
            if (ExistDigits(szThousand))
            {
                /*
                 *  Digits are invalid in separator.
                 */
                LoadString (hModule, INTL + 14, szGenErr, CharSizeOf(szGenErr));
                MessageBox (hDlg, szGenErr, szCtlPanel, MB_OK | MB_ICONINFORMATION);
                SetFocus (GetDlgItem (hDlg, NUM_1000SEP));
                break;
            }

            Current.iLzero = IsDlgButtonChecked (hDlg, NUM_LEAD0) ? TRUE : FALSE;

            Current.iDigits = nDigits;
            lstrcpy (Current.sDecimal, szDecimal);
            lstrcpy (Current.sThousand, szThousand);

            Current.iNegNumber = SendMessage (GetDlgItem (hDlg, NUM_NEG),
                                              CB_GETCURSEL, 0L, 0L);

        case PUSH_CANCEL:
            EndDialog (hDlg, 0L);
            break;
        }
        break;
    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return (TRUE);
        }
        else
        {
            return (FALSE);
        }
        break;
    }

    return (TRUE);

    UNREFERENCED_PARAMETER(lParam);
}

