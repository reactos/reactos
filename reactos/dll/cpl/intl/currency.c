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


typedef struct _GLOBAL_DATA
{
    TCHAR szCurrencySymbol[6];
    TCHAR szDecimalSep[4];
    TCHAR szThousandSep[4];
    TCHAR szGrouping[10];

    int PositiveOrder;
    int NegativeOrder;
    int NumDigits;

} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
GetInitialCurrencyValues(PGLOBAL_DATA pGlobalData)
{
    TCHAR szBuffer[256];
    int ret;

    /* Get currency symbol */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_SCURRENCY,
                        pGlobalData->szCurrencySymbol, 6);

    /* Get positive format */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_ICURRENCY,
                        szBuffer, 256);
    if (ret != 0)
    {
        pGlobalData->PositiveOrder = _ttoi(szBuffer);
    }

    /* Get negative format */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_INEGCURR,
                        szBuffer, 256);
    if (ret != 0)
    {
        pGlobalData->NegativeOrder = _ttoi(szBuffer);
    }

    /* Get number of fractional digits */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_ICURRDIGITS,
                        szBuffer, 256);
    if (ret != 0)
    {
        pGlobalData->NumDigits = _ttoi(szBuffer);
    }

    /* Get decimal separator */
    ret = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                         LOCALE_SMONDECIMALSEP,
                         pGlobalData->szDecimalSep, 4);

    /* Get group separator */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_SMONTHOUSANDSEP,
                        pGlobalData->szThousandSep, 4);

    /* Get grouping */
    ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_SMONGROUPING,
                        pGlobalData->szGrouping, 10);

}


static VOID
UpdateExamples(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    TCHAR szBuffer[256];
    CURRENCYFMT cyFmt;

    cyFmt.NumDigits = pGlobalData->NumDigits;
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = pGlobalData->szDecimalSep;
    cyFmt.lpThousandSep = pGlobalData->szThousandSep;
    cyFmt.PositiveOrder = pGlobalData->PositiveOrder;
    cyFmt.NegativeOrder = pGlobalData->NegativeOrder;
    cyFmt.lpCurrencySymbol = pGlobalData->szCurrencySymbol;

    /* positive example */
    GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                      _T("123456789.00"),
                      &cyFmt, szBuffer, 256);

    SetDlgItemText(hwndDlg,
                   IDC_CURRENCYPOSSAMPLE,
                   szBuffer);

    /* negative example */
    GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                      _T("-123456789.00"),
                      &cyFmt, szBuffer, 256);

    SetDlgItemText(hwndDlg,
                   IDC_CURRENCYNEGSAMPLE,
                   szBuffer);
}


static VOID
OnInitDialog(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    TCHAR szBuffer[256];
    CURRENCYFMT cyFmt;
    int i;

    GetInitialCurrencyValues(pGlobalData);

    /* Set currency symbol */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYSYMBOL),
                CB_ADDSTRING,
                0,
                (LPARAM)pGlobalData->szCurrencySymbol);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYSYMBOL),
                CB_SETCURSEL,
                0, /* index */
                0);


    /* positive currency values */
    cyFmt.NumDigits = pGlobalData->NumDigits;
    cyFmt.LeadingZero = 0;
    cyFmt.Grouping = 3;
    cyFmt.lpDecimalSep = pGlobalData->szDecimalSep;
    cyFmt.lpThousandSep = pGlobalData->szThousandSep;
    cyFmt.NegativeOrder = 0;
    cyFmt.lpCurrencySymbol = pGlobalData->szCurrencySymbol;

    for (i = 0; i < 4; i++)
    {
        cyFmt.PositiveOrder = i;
        GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                          _T("1.1"),
                          &cyFmt, szBuffer, 256);

        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSVALUE),
                    CB_INSERTSTRING,
                    -1,
                    (LPARAM)szBuffer);
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYPOSVALUE),
                CB_SETCURSEL,
                pGlobalData->PositiveOrder,
                0);

    /* negative currency values */
    cyFmt.PositiveOrder = 0;
    for (i = 0; i < 16; i++)
    {
        cyFmt.NegativeOrder = i;
        GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                          _T("-1.1"),
                          &cyFmt, szBuffer, 256);

        SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGVALUE),
                    CB_INSERTSTRING,
                    -1,
                    (LPARAM)szBuffer);
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYNEGVALUE),
                CB_SETCURSEL,
                pGlobalData->NegativeOrder, /* index */
                0);

    /* decimal separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECSEP),
                CB_ADDSTRING,
                0,
                (LPARAM)pGlobalData->szDecimalSep);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECSEP),
                CB_SETCURSEL,
                0, /* index */
                0);


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

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYDECNUM),
                CB_SETCURSEL,
                pGlobalData->NumDigits, /* index */
                0);


    /* digit group separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPSEP),
                CB_ADDSTRING,
                0,
                (LPARAM)pGlobalData->szThousandSep);

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPSEP),
                CB_SETCURSEL,
                0, /* index */
                0);

    /* digit grouping */
    cyFmt.NumDigits = 0;
    cyFmt.LeadingZero = 0;
    cyFmt.lpDecimalSep = _T("");
    cyFmt.lpThousandSep = pGlobalData->szThousandSep;
    cyFmt.NegativeOrder = 0;
    cyFmt.lpCurrencySymbol = _T("");
    cyFmt.Grouping = 0;
    GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, 256);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    cyFmt.Grouping = 3;
    GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, 256);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    cyFmt.Grouping = 32;
    GetCurrencyFormat(LOCALE_USER_DEFAULT, 0,
                      _T("123456789"),
                      &cyFmt, szBuffer, 256);
    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_INSERTSTRING,
                -1,
                (LPARAM)szBuffer);

    i = 0;
    if (pGlobalData->szGrouping[0] == _T('3'))
    {
        if ((pGlobalData->szGrouping[1] == _T(';')) &&
            (pGlobalData->szGrouping[2] == _T('2')))
            i = 2;
        else
            i = 1;
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_CURRENCYGRPNUM),
                CB_SETCURSEL,
                i, /* index */
                0);

    /* Show the examples */
    UpdateExamples(hwndDlg, pGlobalData);
}


/* Property page dialog callback */
INT_PTR CALLBACK
CurrencyPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);
            OnInitDialog(hwndDlg, pGlobalData);
            break;


        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}

/* EOF */
