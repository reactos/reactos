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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/currency.c
 * PURPOSE:         Currency property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

#define POSITIVE_EXAMPLE   _T("123456789.00")
#define NEGATIVE_EXAMPLE   _T("-123456789.00")
#define MAX_FIELD_DIG_SAMPLES       3


static VOID
UpdateExamples(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szBuffer[MAX_FMT_SIZE];

    /* positive example */
    GetCurrencyFormat(pGlobalData->lcid, 0,
                      POSITIVE_EXAMPLE,
                      NULL, szBuffer, MAX_FMT_SIZE);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSSAMPLE), WM_SETTEXT, 0, (LPARAM)szBuffer);

    /* negative example */
    GetCurrencyFormat(pGlobalData->lcid, 0,
                      NEGATIVE_EXAMPLE,
                      NULL, szBuffer, MAX_FMT_SIZE);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGSAMPLE), WM_SETTEXT, 0, (LPARAM)szBuffer);
}


static VOID
InitCurrencySymbols(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szBuffer[MAX_FMT_SIZE];

    /* Set currency symbols */
    GetLocaleInfo(pGlobalData->lcid,
                  LOCALE_SCURRENCY,
                  szBuffer, MAX_FMT_SIZE);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYSYMBOL),
                CB_ADDSTRING,
                0,
                (LPARAM)szBuffer);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYSYMBOL),
                CB_SETCURSEL,
                0, /* index */
                0);
}


static VOID
InitCurrencyPositiveFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szDecimalSep[MAX_FMT_SIZE];
    TCHAR szThousandSep[MAX_FMT_SIZE];
    TCHAR szCurrencySymbol[MAX_FMT_SIZE];
    TCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMT cyFmt;
    INT nPositiveOrder = 0;
    INT ret;
    INT i;


    /* Get positive format */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_ICURRENCY,
                        szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        nPositiveOrder = _ttoi(szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_ICURRDIGITS,
                        szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        cyFmt.NumDigits = _ttoi(szBuffer);
    }
    else
    {
        cyFmt.NumDigits = 0;
    }

    /* Get decimal separator */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONDECIMALSEP,
                        szDecimalSep, MAX_FMT_SIZE);

    /* Get group separator */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONTHOUSANDSEP,
                        szThousandSep, MAX_FMT_SIZE);

    /* Get currency symbol */
    ret = GetLocaleInfo(pGlobalData->lcid,
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
        GetCurrencyFormat(pGlobalData->lcid, 0,
                          _T("1.1"),
                          &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSVALUE),
                    CB_INSERTSTRING,
                    -1,
                    (LPARAM)szBuffer);
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSVALUE),
                CB_SETCURSEL,
                nPositiveOrder,
                0);
}


static VOID
InitCurrencyNegativeFormats(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szDecimalSep[MAX_FMT_SIZE];
    TCHAR szThousandSep[MAX_FMT_SIZE];
    TCHAR szCurrencySymbol[MAX_FMT_SIZE];
    TCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMT cyFmt;
    INT nNegativeOrder = 0;
    INT ret;
    int i;

    /* Get negative format */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_INEGCURR,
                        szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        nNegativeOrder = _ttoi(szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_ICURRDIGITS,
                        szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        cyFmt.NumDigits = _ttoi(szBuffer);
    }
    else
    {
        cyFmt.NumDigits = 0;
    }

    /* Get decimal separator */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONDECIMALSEP,
                        szDecimalSep, MAX_FMT_SIZE);

    /* Get group separator */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONTHOUSANDSEP,
                        szThousandSep, MAX_FMT_SIZE);

    /* Get currency symbol */
    ret = GetLocaleInfo(pGlobalData->lcid,
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
        GetCurrencyFormat(pGlobalData->lcid, 0,
                          _T("-1.1"),
                          &cyFmt, szBuffer, MAX_FMT_SIZE);

        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGVALUE),
                    CB_INSERTSTRING,
                    -1,
                    (LPARAM)szBuffer);
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGVALUE),
                CB_SETCURSEL,
                nNegativeOrder,
                0);
}


static VOID
InitCurrencyDecimalSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szBuffer[MAX_FMT_SIZE];

    /* Get decimal separator */
    GetLocaleInfo(pGlobalData->lcid,
                  LOCALE_SMONDECIMALSEP,
                  szBuffer, MAX_FMT_SIZE);

    /* decimal separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECSEP),
                CB_ADDSTRING,
                0,
                (LPARAM)szBuffer);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECSEP),
                CB_SETCURSEL,
                0, /* index */
                0);
}


/* Initialize the number of fractional digits */
static VOID
InitCurrencyNumFracDigits(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szBuffer[MAX_FMT_SIZE];
    int ret;
    int i;

    /* */
    for (i = 0; i < 10; i++)
    {
        szBuffer[0] = _T('0') + i;
        szBuffer[1] = 0;
        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECNUM),
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_ICURRDIGITS,
                        szBuffer, MAX_FMT_SIZE);
    if (ret != 0)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECNUM),
                    CB_SETCURSEL,
                    _ttoi(szBuffer),
                    0);
    }
    else
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECNUM),
                    CB_SETCURSEL,
                    0,
                    0);
    }
}


/* Initialize the list of group separators */
static VOID
InitCurrencyGroupSeparators(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szBuffer[MAX_FMT_SIZE];

    /* Get group separator */
    GetLocaleInfo(pGlobalData->lcid,
                  LOCALE_SMONTHOUSANDSEP,
                  szBuffer, MAX_FMT_SIZE);

    /* digit group separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPSEP),
                CB_ADDSTRING,
                0,
                (LPARAM)szBuffer);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPSEP),
                CB_SETCURSEL,
                0, /* index */
                0);
}


static VOID
InitDigitGroupCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    TCHAR szThousandSep[MAX_FMT_SIZE];
    TCHAR szGrouping[MAX_FMT_SIZE];
    TCHAR szBuffer[MAX_FMT_SIZE];
    CURRENCYFMT cyFmt;
    INT ret;
    INT i;

    /* Get group separator */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONTHOUSANDSEP,
                        szThousandSep, MAX_FMT_SIZE);

    /* Get grouping */
    ret = GetLocaleInfo(pGlobalData->lcid,
                        LOCALE_SMONGROUPING,
                        szGrouping, MAX_FMT_SIZE);

    /* digit grouping */
    cyFmt.NumDigits = 0;
    cyFmt.LeadingZero = 0;
    cyFmt.lpDecimalSep = _T("");
    cyFmt.lpThousandSep = szThousandSep;
    cyFmt.NegativeOrder = 0;
    cyFmt.lpCurrencySymbol = _T("");
    cyFmt.Grouping = 0;
    GetCurrencyFormat(pGlobalData->lcid, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    cyFmt.Grouping = 3;
    GetCurrencyFormat(pGlobalData->lcid, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    cyFmt.Grouping = 32;
    GetCurrencyFormat(pGlobalData->lcid, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    i = 0;
    if (szGrouping[0] == _T('3'))
    {
        if ((szGrouping[1] == _T(';')) &&
            (szGrouping[2] == _T('2')))
            i = 2;
        else
            i = 1;
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_SETCURSEL,
                i, /* index */
                0);
}


/* Set number of digidts in field  */
static BOOL
SetCurrencyDigNum(HWND hwndDlg, LCID lcid)
{
    LPTSTR szFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES]=
    {
        _T("0;0"),
        _T("3;0"),
        _T("3;2;0")
    };

    int nCurrSel;

    /* Get setted number of digidts in field */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* Save number of digidts in field */
    SetLocaleInfo(lcid, LOCALE_SMONGROUPING, szFieldDigNumSamples[nCurrSel]);

    return TRUE;
}

/* Set currency field separator */
static BOOL
SetCurrencyFieldSep(HWND hwndDlg, LCID lcid)
{
    TCHAR szCurrencyFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency field separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPSEP),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szCurrencyFieldSep);

    /* Save currency field separator */
    SetLocaleInfo(lcid, LOCALE_SMONTHOUSANDSEP, szCurrencyFieldSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetCurrencyFracSymNum(HWND hwndDlg, LCID lcid)
{
    TCHAR szCurrencyFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECNUM),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szCurrencyFracSymNum, DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfo(lcid, LOCALE_ICURRDIGITS, szCurrencyFracSymNum);

    return TRUE;
}

/* Set currency separator */
static BOOL
SetCurrencySep(HWND hwndDlg, LCID lcid)
{
    TCHAR szCurrencySep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency decimal separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECSEP),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szCurrencySep);

    /* TODO: Add check for correctly input */

    /* Save currency separator */
    SetLocaleInfo(lcid, LOCALE_SMONDECIMALSEP, szCurrencySep);

    return TRUE;
}

/* Set negative currency sum format */
static BOOL
SetNegCurrencySumFmt(HWND hwndDlg, LCID lcid)
{
    TCHAR szNegCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGVALUE),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szNegCurrencySumFmt, DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfo(lcid, LOCALE_INEGCURR, szNegCurrencySumFmt);

    return TRUE;
}

/* Set positive currency sum format */
static BOOL
SetPosCurrencySumFmt(HWND hwndDlg, LCID lcid)
{
    TCHAR szPosCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSVALUE),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szPosCurrencySumFmt, DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfo(lcid, LOCALE_ICURRENCY, szPosCurrencySumFmt);

    return TRUE;
}

/* Set currency unit */
static BOOL
SetCurrencyUnit(HWND hwndDlg, LCID lcid)
{
    TCHAR szCurrencyUnit[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency unit */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYSYMBOL),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)(LPCSTR)szCurrencyUnit);

    /* Save currency unit */
    SetLocaleInfo(lcid, LOCALE_SCURRENCY, szCurrencyUnit);

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
                    if (!SetCurrencyDigNum(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetCurrencyUnit(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetPosCurrencySumFmt(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetNegCurrencySumFmt(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetCurrencySep(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetCurrencyFracSymNum(hwndDlg, pGlobalData->lcid))
                        break;

                    if (!SetCurrencyFieldSep(hwndDlg, pGlobalData->lcid))
                        break;

                    UpdateExamples(hwndDlg, pGlobalData);
                }
            }
            break;
    }

    return FALSE;
}

/* EOF */
