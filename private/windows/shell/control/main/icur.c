/** FILE: icur.c *********** Module Header ********************************
 *
 *  Control panel applet for International configuration.  This file holds
 *  everything to do with the Currency dialog box inside the International
 *  Dialog of Control Panel.
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

extern TCHAR    *pszCurPat[];
extern TCHAR    *pszNegCurPat[];
extern TCHAR    *pszSymPlacement[];


//==========================================================================
//                            Local Data Declarations
//==========================================================================


//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

void SetupPlacementCB(
    HWND hCB,
    TCHAR *szSymbol)
{
    TCHAR    szFormat[20];
    short    i;

    SendMessage (hCB, CB_RESETCONTENT, 0, 0L);
    for (i = 0; i < NUM_SYM_PAT; i++)
    {
        wsprintf ((LPTSTR)szFormat, (LPTSTR)pszSymPlacement[i], (LPTSTR)szSymbol);
        SendMessage (hCB, CB_ADDSTRING, 0L, (LONG)(LPTSTR)szFormat);
    }
}


void SetupNegativeCB(
    HWND hCB,
    TCHAR *szSymbol,
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
        wsprintf (szTemp, TEXT("123%s"), szDecSep);
        for (i = 0, pch = szTemp + lstrlen (szTemp); i < nDecimal; i++)
        {
            *pch++ = ch;
        }
        *pch = TEXT('\0');
    }
    else
    {
        lstrcpy (szTemp, TEXT("123"));
    }

    SendMessage (hCB, CB_RESETCONTENT, 0, 0L);

    for (i = 0; i < NUM_NEG_PAT; i++)
    {
        // flip the format for these guys
        if ((i < 4) || (i == 9) || (i == 11) || (i == 12) || (i == 14))
        {
            wsprintf(szFormat, pszNegCurPat[i], (LPTSTR)szSymbol, (LPTSTR)szTemp);
        }
        else
        {
            wsprintf(szFormat, pszNegCurPat[i], (LPTSTR)szTemp, (LPTSTR)szSymbol);
        }

        SendMessage(hCB, CB_ADDSTRING, 0, (LONG)(LPTSTR)szFormat);
    }

    return;
}


BOOL APIENTRY CurIntlDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HWND  hCB;
    int   nDec;
    short nIndex;
    BOOL  bOK;
    TCHAR  szTemp[20];
    TCHAR  szDecimal[20];
    TCHAR  szThousand[20];
    static BOOL bSymbolChange;


    switch (message)
    {
    case WM_INITDIALOG:
        HourGlass (TRUE);

        SetDlgItemText (hDlg, CUR_SYMBOL, Current.sCurrency);
        SendDlgItemMessage (hDlg, CUR_SYMBOL, EM_LIMITTEXT,
                            CharSizeOf(Current.sCurrency) - 1, 0L);

        SetDlgItemText (hDlg, CUR_1000SEP, Current.sMonThousand);
        SendDlgItemMessage (hDlg, CUR_1000SEP, EM_LIMITTEXT, 1, 0L);

        SetDlgItemText (hDlg, CUR_DECSEP, Current.sMonDecimal);
        SendDlgItemMessage (hDlg, CUR_DECSEP, EM_LIMITTEXT, 1, 0L);

        SetDlgItemInt (hDlg, CUR_DECDIGITS, Current.iCurDec, FALSE);

        hCB = GetDlgItem (hDlg, CUR_FORMAT1);
        SetupPlacementCB (hCB, Current.sCurrency);
        SendMessage (hCB, CB_SETCURSEL, Current.iCurFmt, 0L);

        hCB = GetDlgItem (hDlg, CUR_NEG);
        SetupNegativeCB (hCB, Current.sCurrency, Current.sMonDecimal, Current.iCurDec);
        SendMessage (hCB, CB_SETCURSEL, Current.iNegCur, 0L);

        bSymbolChange = FALSE;
        HourGlass (FALSE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case CUR_DECDIGITS:
        case CUR_DECSEP:
        case CUR_SYMBOL:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bSymbolChange = TRUE;
            }
            else if (bSymbolChange && (HIWORD(wParam) == EN_KILLFOCUS))
            {
                bSymbolChange = FALSE;

                nDec = GetDlgItemInt (hDlg, CUR_DECDIGITS, &bOK, FALSE);

                if (!GetDlgItemText(hDlg, CUR_SYMBOL, szTemp, CharSizeOf(Current.sCurrency)))
                {
                    break;
                }
                if (!GetDlgItemText(hDlg, CUR_DECSEP, szDecimal, CharSizeOf(Current.sMonDecimal))
                    && (nDec != 0))
                {
                    break;
                }

                hCB = GetDlgItem (hDlg, CUR_NEG);
                nIndex = (short) SendMessage (hCB, CB_GETCURSEL, 0L, 0L);
                SetupNegativeCB (hCB, szTemp, szDecimal, nDec);
                SendMessage (hCB, CB_SETCURSEL, nIndex, 0L);

                hCB = GetDlgItem (hDlg, CUR_FORMAT1);
                nIndex = (short) SendMessage (hCB, CB_GETCURSEL, 0L, 0L);
                SetupPlacementCB (hCB, szTemp);
                SendMessage (hCB, CB_SETCURSEL, nIndex, 0L);
            }
            break;

        case CUR_FORMAT1:
        case CUR_NEG:
        case CUR_1000SEP:
            break;

        case PUSH_OK:
            nDec = GetDlgItemInt (hDlg, CUR_DECDIGITS, &bOK, FALSE);
            if (!bOK)
            {
                MyMessageBox (hDlg, INTL+4, INITS+1, MB_OK | MB_ICONINFORMATION);
                SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, CUR_DECDIGITS), 1L);
                SendDlgItemMessage (hDlg, CUR_DECDIGITS, EM_SETSEL, 0, 32767);
                break;
            }

            if ((!GetDlgItemText (hDlg, CUR_DECSEP, szDecimal, CharSizeOf(Current.sMonDecimal))
                 && (nDec != 0)) || ExistDigits(szDecimal))
            {
                /*
                 *  Null decimal separator only valid if number of decimal
                 *  digits is zero.
                 */
                MyMessageBox (hDlg, INTL, INITS+1, MB_OK | MB_ICONINFORMATION);
                SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, CUR_DECSEP), 1L);
                break;
            }

            if (!GetDlgItemText(hDlg, CUR_SYMBOL, szTemp, CharSizeOf(Current.sCurrency)) ||
                ExistDigits(szTemp))
            {
                MyMessageBox (hDlg, INTL+3, INITS+1, MB_OK | MB_ICONINFORMATION);
                SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, CUR_SYMBOL), 1L);
                break;
            }

            GetDlgItemText (hDlg, CUR_1000SEP, szThousand, CharSizeOf(Current.sMonThousand));
            if (ExistDigits(szThousand))
            {
                MyMessageBox (hDlg, INTL+14, INITS+1, MB_OK | MB_ICONINFORMATION);
                SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, CUR_1000SEP), 1L);
                break;
            }

            lstrcpy (Current.sMonDecimal, szDecimal);
            lstrcpy(Current.sCurrency, szTemp);
            lstrcpy (Current.sMonThousand, szThousand);

            Current.iCurDec = nDec;
            Current.iCurFmt = SendMessage (GetDlgItem (hDlg, CUR_FORMAT1),
                                           CB_GETCURSEL, 0L, 0L);
            Current.iNegCur = SendMessage (GetDlgItem (hDlg, CUR_NEG),
                                           CB_GETCURSEL, 0L, 0L);
            // fall through...

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