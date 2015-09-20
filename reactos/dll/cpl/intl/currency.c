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
#define MAX_FIELD_DIG_SAMPLES       3


static VOID
UpdateExamples(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];

    /* Positive example */
    GetCurrencyFormatW(pGlobalData->lcid, 0,
                       POSITIVE_EXAMPLE,
                       NULL, szBuffer, MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSSAMPLE, WM_SETTEXT, 0, (LPARAM)szBuffer);

    /* Negative example */
    GetCurrencyFormatW(pGlobalData->lcid, 0,
                       NEGATIVE_EXAMPLE,
                       NULL, szBuffer, MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGSAMPLE, WM_SETTEXT, 0, (LPARAM)szBuffer);
}


static VOID
InitCurrencySymbols(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_LIMITTEXT,
                        MAX_CURRENCYSYMBOL,
                        0);

    /* Set currency symbols */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SCURRENCY,
                   szBuffer, MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)szBuffer);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static VOID
InitCurrencyPositiveFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szDecimalSep[MAX_FMT_SIZE];
    WCHAR szThousandSep[MAX_FMT_SIZE];
    WCHAR szCurrencySymbol[MAX_FMT_SIZE];
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    INT nPositiveOrder = 0;
    INT ret;
    INT i;


    /* Get positive format */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_ICURRENCY,
                         szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        nPositiveOrder = _wtoi(szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_ICURRDIGITS,
                         szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        cyFmt.NumDigits = _wtoi(szBuffer);
    }
    else
    {
        cyFmt.NumDigits = 0;
    }

    /* Get decimal separator */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SMONDECIMALSEP,
                         szDecimalSep, MAX_FMT_SIZE);

    /* Get group separator */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SMONTHOUSANDSEP,
                         szThousandSep, MAX_FMT_SIZE);

    /* Get currency symbol */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SCURRENCY,
                         szCurrencySymbol, MAX_FMT_SIZE);

    /* positive currency values */
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = szDecimalSep;
    cyFmt.lpThousandSep = szThousandSep;
    cyFmt.lpCurrencySymbol = szCurrencySymbol;
    cyFmt.NegativeOrder = 0;

    for (i = 0; i < 4; i++)
    {
        cyFmt.PositiveOrder = i;
        GetCurrencyFormatW(pGlobalData->lcid, 0,
                           L"1.1",
                           &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                            CB_INSERTSTRING,
                            -1,
                            (LPARAM)szBuffer);
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                        CB_SETCURSEL,
                        nPositiveOrder,
                        0);
}


static VOID
InitCurrencyNegativeFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szDecimalSep[MAX_FMT_SIZE];
    WCHAR szThousandSep[MAX_FMT_SIZE];
    WCHAR szCurrencySymbol[MAX_FMT_SIZE];
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    INT nNegativeOrder = 0;
    INT ret;
    int i;

    /* Get negative format */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_INEGCURR,
                         szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        nNegativeOrder = _wtoi(szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_ICURRDIGITS,
                         szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        cyFmt.NumDigits = _wtoi(szBuffer);
    }
    else
    {
        cyFmt.NumDigits = 0;
    }

    /* Get decimal separator */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SMONDECIMALSEP,
                         szDecimalSep, MAX_FMT_SIZE);

    /* Get group separator */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SMONTHOUSANDSEP,
                         szThousandSep, MAX_FMT_SIZE);

    /* Get currency symbol */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_SCURRENCY,
                         szCurrencySymbol, MAX_FMT_SIZE);

    /* negative currency values */
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = szDecimalSep;
    cyFmt.lpThousandSep = szThousandSep;
    cyFmt.lpCurrencySymbol = szCurrencySymbol;
    cyFmt.PositiveOrder = 0;

    for (i = 0; i < 16; i++)
    {
        cyFmt.NegativeOrder = i;
        GetCurrencyFormatW(pGlobalData->lcid, 0,
                           L"-1.1",
                           &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                            CB_INSERTSTRING,
                            -1,
                            (LPARAM)szBuffer);
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                        CB_SETCURSEL,
                        nNegativeOrder,
                        0);
}


static VOID
InitCurrencyDecimalSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        CB_LIMITTEXT,
                        MAX_CURRENCYDECSEP,
                        0);

    /* Get decimal separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SMONDECIMALSEP,
                   szBuffer, MAX_FMT_SIZE);

    /* Decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)szBuffer);

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
    int ret;
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

    /* Get number of fractional digits */
    ret = GetLocaleInfoW(pGlobalData->lcid,
                         LOCALE_ICURRDIGITS,
                         szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                            CB_SETCURSEL,
                            _wtoi(szBuffer),
                            0);
    }
    else
    {
        SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                            CB_SETCURSEL,
                            0,
                            0);
    }
}


/* Initialize the list of group separators */
static VOID
InitCurrencyGroupSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_FMT_SIZE];

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_LIMITTEXT,
                        MAX_CURRENCYGRPSEP,
                        0);

    /* Get group separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SMONTHOUSANDSEP,
                   szBuffer, MAX_FMT_SIZE);

    /* Digit group separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)szBuffer);

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static VOID
InitDigitGroupCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szThousandSep[MAX_FMT_SIZE];
    WCHAR szGrouping[MAX_FMT_SIZE];
    WCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMTW cyFmt;
    INT i;

    /* Get group separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SMONTHOUSANDSEP,
                   szThousandSep, MAX_FMT_SIZE);

    /* Get grouping */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SMONGROUPING,
                   szGrouping, MAX_FMT_SIZE);

    /* Digit grouping */
    cyFmt.NumDigits = 0;
    cyFmt.LeadingZero = 0;
    cyFmt.lpDecimalSep = L"";
    cyFmt.lpThousandSep = szThousandSep;
    cyFmt.PositiveOrder = 0;
    cyFmt.NegativeOrder = 0;
    cyFmt.lpCurrencySymbol = L"";
    cyFmt.Grouping = 0;
    GetCurrencyFormatW(pGlobalData->lcid, 0,
                       L"123456789",
                       &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                        CB_INSERTSTRING,
                        -1,
                        (LPARAM)szBuffer);

    cyFmt.Grouping = 3;
    GetCurrencyFormatW(pGlobalData->lcid, 0,
                       L"123456789",
                       &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                        CB_INSERTSTRING,
                        -1,
                        (LPARAM)szBuffer);

    cyFmt.Grouping = 32;
    GetCurrencyFormatW(pGlobalData->lcid, 0,
                       L"123456789",
                       &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                        CB_INSERTSTRING,
                        -1,
                        (LPARAM)szBuffer);

    i = 0;
    if (szGrouping[0] == L'3')
    {
        if ((szGrouping[1] == L';') &&
            (szGrouping[2] == L'2'))
            i = 2;
        else
            i = 1;
    }

    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                        CB_SETCURSEL,
                        i, /* Index */
                        0);
}


/* Set number of digits in field  */
static BOOL
SetCurrencyDigNum(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    PWSTR szFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES]=
    {
        L"0;0",
        L"3;0",
        L"3;2;0"
    };

    int nCurrSel;

    /* Get setted number of digits in field */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPNUM,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Save number of digits in field */
    if (nCurrSel != CB_ERR)
        SetLocaleInfoW(pGlobalData->lcid, LOCALE_SMONGROUPING, szFieldDigNumSamples[nCurrSel]);

    return TRUE;
}

/* Set currency field separator */
static BOOL
SetCurrencyFieldSep(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szCurrencyFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency field separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYGRPSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szCurrencyFieldSep);

    /* Save currency field separator */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SMONTHOUSANDSEP, szCurrencyFieldSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetCurrencyFracSymNum(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szCurrencyFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECNUM,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Convert to wide char */
    _itow(nCurrSel, szCurrencyFracSymNum, DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_ICURRDIGITS, szCurrencyFracSymNum);

    return TRUE;
}

/* Set currency separator */
static BOOL
SetCurrencySep(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szCurrencySep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYDECSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szCurrencySep);

    /* TODO: Add check for correctly input */

    /* Save currency separator */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SMONDECIMALSEP, szCurrencySep);

    return TRUE;
}

/* Set negative currency sum format */
static BOOL
SetNegCurrencySumFmt(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szNegCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYNEGVALUE,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Convert to wide char */
    _itow(nCurrSel, szNegCurrencySumFmt, DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_INEGCURR, szNegCurrencySumFmt);

    return TRUE;
}

/* Set positive currency sum format */
static BOOL
SetPosCurrencySumFmt(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szPosCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_CURRENCYPOSVALUE,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Convert to wide char */
    _itow(nCurrSel, szPosCurrencySumFmt, DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_ICURRENCY, szPosCurrencySumFmt);

    return TRUE;
}

/* Set currency unit */
static BOOL
SetCurrencyUnit(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szCurrencyUnit[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency unit */
    SendDlgItemMessageW(hwndDlg, IDC_CURRENCYSYMBOL,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)(PCWSTR)szCurrencyUnit);

    /* Save currency unit */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SCURRENCY, szCurrencyUnit);

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
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;
                /* If push apply button */
                if (lpnm->code == (UINT)PSN_APPLY)
                {
                    if (!SetCurrencyDigNum(hwndDlg, pGlobalData))
                        break;

                    if (!SetCurrencyUnit(hwndDlg, pGlobalData))
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

                    UpdateExamples(hwndDlg, pGlobalData);
                }
            }
            break;
    }
    return FALSE;
}

/* EOF */
