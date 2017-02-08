/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/currency.c
 * PURPOSE:         Currency property page
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

#define POSITIVE_EXAMPLE   L"123456789.00"
#define NEGATIVE_EXAMPLE   L"-123456789.00"


static VOID
UpdateExamples(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW CurrencyFormat;

    CurrencyFormat.NumDigits = pGlobalData->nCurrDigits;
    CurrencyFormat.LeadingZero = pGlobalData->nNumLeadingZero;
    CurrencyFormat.Grouping = GroupingFormats[pGlobalData->nCurrGrouping].nInteger;
    CurrencyFormat.lpDecimalSep = pGlobalData->szCurrDecimalSep;
    CurrencyFormat.lpThousandSep = pGlobalData->szCurrThousandSep;
    CurrencyFormat.NegativeOrder = pGlobalData->nCurrNegFormat;
    CurrencyFormat.PositiveOrder = pGlobalData->nCurrPosFormat;
    CurrencyFormat.lpCurrencySymbol = pGlobalData->szCurrSymbol;

    /* Positive example */
    GetCurrencyFormatW(pGlobalData->UserLCID, 0,
                       POSITIVE_EXAMPLE,
                       &CurrencyFormat, szBuffer, MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSSAMPLE, WM_SETTEXT, 0, (LPARAM)szBuffer);

    /* Negative example */
    GetCurrencyFormatW(pGlobalData->UserLCID, 0,
                       NEGATIVE_EXAMPLE,
                       &CurrencyFormat, szBuffer, MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGSAMPLE, WM_SETTEXT, 0, (LPARAM)szBuffer);
}


static VOID
InitCurrencySymbols(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_LIMITTEXT,
                        MAX_CURRSYMBOL - 1,
                        0);

    /* Set currency symbols */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szCurrSymbol);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static VOID
InitCurrencyPositiveFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    INT i;

    /* positive currency values */
    cyFmt.NumDigits = pGlobalData->nCurrDigits;
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = pGlobalData->szCurrDecimalSep;
    cyFmt.lpThousandSep = pGlobalData->szCurrThousandSep;
    cyFmt.lpCurrencySymbol = pGlobalData->szCurrSymbol;
    cyFmt.NegativeOrder = 0;

    for (i = 0; i < 4; i++)
    {
        cyFmt.PositiveOrder = i;
        GetCurrencyFormatW(pGlobalData->UserLCID, 0,
                           L"1.1",
                           &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                            CB_INSERTSTRING,
                            -1,
                            (LPARAM)szBuffer);
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                        CB_SETCURSEL,
                        pGlobalData->nCurrPosFormat,
                        0);
}


static VOID
InitCurrencyNegativeFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    int i;

    /* negative currency values */
    cyFmt.NumDigits = pGlobalData->nCurrDigits;
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = pGlobalData->szCurrDecimalSep;
    cyFmt.lpThousandSep = pGlobalData->szCurrThousandSep;
    cyFmt.lpCurrencySymbol = pGlobalData->szCurrSymbol;
    cyFmt.PositiveOrder = 0;

    for (i = 0; i < 16; i++)
    {
        cyFmt.NegativeOrder = i;
        GetCurrencyFormatW(pGlobalData->UserLCID, 0,
                           L"-1.1",
                           &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                            CB_INSERTSTRING,
                            -1,
                            (LPARAM)szBuffer);
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                        CB_SETCURSEL,
                        pGlobalData->nCurrNegFormat,
                        0);
}


static VOID
InitCurrencyDecimalSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        CB_LIMITTEXT,
                        MAX_CURRDECIMALSEP - 1,
                        0);

    /* Decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szCurrDecimalSep);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


/* Initialize the number of fractional digits */
static VOID
InitCurrencyNumFracDigits(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];
    int i;

    /* Create standard list of fractional symbols */
    for (i = 0; i < 10; i++)
    {
        szBuffer[0] = L'0' + i;
        szBuffer[1] = 0;
        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szBuffer);
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                        CB_SETCURSEL,
                        pGlobalData->nCurrDigits,
                        0);
}


/* Initialize the list of group separators */
static VOID
InitCurrencyGroupSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_LIMITTEXT,
                        MAX_CURRTHOUSANDSEP - 1,
                        0);

    /* Digit group separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szCurrThousandSep);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static VOID
InitDigitGroupCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    INT i;

    /* Digit grouping */
    cyFmt.NumDigits = 0;
    cyFmt.LeadingZero = 0;
    cyFmt.lpDecimalSep = L"";
    cyFmt.lpThousandSep = pGlobalData->szCurrThousandSep;
    cyFmt.PositiveOrder = 0;
    cyFmt.NegativeOrder = 0;
    cyFmt.lpCurrencySymbol = L"";

    for (i = 0 ; i < MAX_GROUPINGFORMATS ; i++)
    {
       cyFmt.Grouping = GroupingFormats[i].nInteger;

       GetCurrencyFormatW(pGlobalData->UserLCID, 0,
                          L"123456789",
                          &cyFmt, szBuffer, MAX_FMT_SIZE);
       SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                           CB_INSERTSTRING,
                           -1,
                           (LPARAM)szBuffer);
    }
}


/* Set number of digits in field  */
static BOOL
SetCurrencyDigNum(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted number of digits in field */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Save number of digits in field */
    if (nCurrSel != CB_ERR)
        pGlobalData->nCurrGrouping = nCurrSel;

    return TRUE;
}

/* Set currency field separator */
static BOOL
SetCurrencyFieldSep(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Get setted currency field separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)pGlobalData->szCurrThousandSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetCurrencyFracSymNum(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nCurrDigits = nCurrSel;

    return TRUE;
}

/* Set currency separator */
static BOOL
SetCurrencySep(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Get setted currency decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)pGlobalData->szCurrDecimalSep);

    return TRUE;
}

/* Set negative currency sum format */
static BOOL
SetNegCurrencySumFmt(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nCurrNegFormat = nCurrSel;

    return TRUE;
}

/* Set positive currency sum format */
static BOOL
SetPosCurrencySumFmt(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nCurrPosFormat = nCurrSel;

    return TRUE;
}

/* Set currency symbol */
static BOOL
SetCurrencySymbol(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    /* Get setted currency unit */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)(PCWSTR)pGlobalData->szCurrSymbol);

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
CurrencyPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PGLOBALDATA pGlobalData;

    pGlobalData = (PGLOBALDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBALDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            InitCurrencySymbols(hwndDlg, pGlobalData);
            InitCurrencyPositiveFormats(hwndDlg, pGlobalData);
            InitCurrencyNegativeFormats(hwndDlg, pGlobalData);
            InitCurrencyDecimalSeparators(hwndDlg, pGlobalData);
            InitCurrencyNumFracDigits(hwndDlg, pGlobalData);
            InitCurrencyGroupSeparators(hwndDlg, pGlobalData);
            InitDigitGroupCB(hwndDlg, pGlobalData);
            UpdateExamples(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CURRENCYSYMBOL:
                case IDC_CURRENCYPOSVALUE:
                case IDC_CURRENCYNEGVALUE:
                case IDC_CURRENCYDECSEP:
                case IDC_CURRENCYDECNUM:
                case IDC_CURRENCYGRPSEP:
                case IDC_CURRENCYGRPNUM:
                    if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Set "Apply" button enabled */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                if (!SetCurrencySymbol(hwndDlg, pGlobalData))
                    break;

                if (!SetCurrencyDigNum(hwndDlg, pGlobalData))
                    break;

                if (!SetPosCurrencySumFmt(hwndDlg, pGlobalData))
                    break;

                if (!SetNegCurrencySumFmt(hwndDlg, pGlobalData))
                    break;

                if (!SetCurrencySep(hwndDlg, pGlobalData))
                    break;

                if (!SetCurrencyFracSymNum(hwndDlg, pGlobalData))
                    break;

                if (!SetCurrencyFieldSep(hwndDlg, pGlobalData))
                    break;

                pGlobalData->fUserLocaleChanged = TRUE;

                UpdateExamples(hwndDlg, pGlobalData);
            }
            break;
    }
    return FALSE;
}

/* EOF */
