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
 * FILE:            dll/cpl/intl/time.c
 * PURPOSE:         Time property page
 * PROGRAMMERS:     Eric Kohl
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "intl.h"

static HWND hwndTimeEnum = NULL;

static BOOL CALLBACK
TimeFormatEnumProc(PWSTR lpTimeFormatString)
{
    SendMessageW(hwndTimeEnum,
                CB_ADDSTRING,
                0,
                (LPARAM)lpTimeFormatString);

    return TRUE;
}

static VOID
UpdateTimeSample(HWND hWnd, PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[MAX_SAMPLES_STR_SIZE];

    GetTimeFormatW(pGlobalData->UserLCID, 0, NULL,
                   pGlobalData->szTimeFormat, szBuffer,
                   MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hWnd, IDC_TIMESAMPLE, WM_SETTEXT, 0, (LPARAM)szBuffer);
}


static
VOID
InitTimeFormatCB(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    /* Get the time format */
    SendDlgItemMessageW(hwndDlg, IDC_TIMEFORMAT,
                        CB_LIMITTEXT, MAX_TIMEFORMAT, 0);

    /* Add available time formats to the list */
    hwndTimeEnum = GetDlgItem(hwndDlg, IDC_TIMEFORMAT);
    EnumTimeFormatsW(TimeFormatEnumProc, pGlobalData->UserLCID, 0);

    SendDlgItemMessageW(hwndDlg, IDC_TIMEFORMAT,
                        CB_SELECTSTRING,
                        -1,
                        (LPARAM)pGlobalData->szTimeFormat);
}

static
VOID
InitTimeSeparatorCB(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    SendDlgItemMessageW(hwndDlg, IDC_TIMESEPARATOR,
                        CB_LIMITTEXT, MAX_TIMESEPARATOR, 0);

    SendDlgItemMessageW(hwndDlg, IDC_TIMESEPARATOR,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szTimeSep);

    SendDlgItemMessageW(hwndDlg, IDC_TIMESEPARATOR,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static
VOID
InitAmSymbol(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    int nLen;

    SendDlgItemMessageW(hwndDlg, IDC_TIMEAMSYMBOL,
                        CB_LIMITTEXT, MAX_TIMEAMSYMBOL, 0);

    nLen = wcslen(pGlobalData->szTimeAM);

    SendDlgItemMessageW(hwndDlg, IDC_TIMEAMSYMBOL,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szTimeAM);
    if (nLen != 0)
    {
        SendDlgItemMessageW(hwndDlg, IDC_TIMEAMSYMBOL,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)L"");
    }

    SendDlgItemMessageW(hwndDlg, IDC_TIMEAMSYMBOL,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static
VOID
InitPmSymbol(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    int nLen;

    SendDlgItemMessageW(hwndDlg, IDC_TIMEPMSYMBOL,
                        CB_LIMITTEXT, MAX_TIMEPMSYMBOL, 0);

    nLen = wcslen(pGlobalData->szTimeAM);

    SendDlgItemMessageW(hwndDlg, IDC_TIMEPMSYMBOL,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)pGlobalData->szTimePM);
    if (nLen != 0)
    {
        SendDlgItemMessageW(hwndDlg, IDC_TIMEPMSYMBOL,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)L"");
    }
    SendDlgItemMessageW(hwndDlg, IDC_TIMEPMSYMBOL,
                        CB_SETCURSEL,
                        0, /* Index */
                        0);
}


static
BOOL
GetTimeSetting(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    WCHAR szTimeFormat[MAX_TIMEFORMAT];
    WCHAR szTimeSep[MAX_TIMESEPARATOR];
    WCHAR szTimeAM[MAX_TIMEAMSYMBOL];
    WCHAR szTimePM[MAX_TIMEPMSYMBOL];

    /* Time format */
    GetSelectedComboBoxText(hwndDlg,
                            IDC_TIMEFORMAT,
                            szTimeFormat,
                            MAX_TIMEFORMAT);

    /* Check the time format */
    if (szTimeFormat[0] == L'\0')
    {
        /* TODO: Show error message */

        return FALSE;
    }

    /* Time separator */
    GetSelectedComboBoxText(hwndDlg,
                            IDC_TIMESEPARATOR,
                            szTimeSep,
                            MAX_TIMESEPARATOR);

    /* Check the time separator */
    if (szTimeSep[0] == L'\0')
    {
        /* TODO: Show error message */

        return FALSE;
    }

    /* AM symbol */
    GetSelectedComboBoxText(hwndDlg,
                            IDC_TIMEAMSYMBOL,
                            szTimeAM,
                            MAX_TIMEAMSYMBOL);

    /* PM symbol */
    GetSelectedComboBoxText(hwndDlg,
                            IDC_TIMEPMSYMBOL,
                            szTimePM,
                            MAX_TIMEPMSYMBOL);

    /* Store settings in global data */
    wcscpy(pGlobalData->szTimeFormat, szTimeFormat);
    wcscpy(pGlobalData->szTimeSep, szTimeSep);
    wcscpy(pGlobalData->szTimeAM, szTimeAM);
    wcscpy(pGlobalData->szTimePM, szTimePM);

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
TimePageProc(HWND hwndDlg,
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

            /* Get the time format */
            InitTimeFormatCB(hwndDlg, pGlobalData);

            /* Get the time separator */
            InitTimeSeparatorCB(hwndDlg, pGlobalData);

            /* Get the AM symbol */
            InitAmSymbol(hwndDlg, pGlobalData);

            /* Get the PM symbol */
            InitPmSymbol(hwndDlg, pGlobalData);

            /* Update the time format sample */
            UpdateTimeSample(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TIMEFORMAT:
                case IDC_TIMESEPARATOR:
                case IDC_TIMEAMSYMBOL:
                case IDC_TIMEPMSYMBOL:
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Enable the Apply button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                if (GetTimeSetting(hwndDlg, pGlobalData))
                {
                    pGlobalData->bUserLocaleChanged = TRUE;
                    UpdateTimeSample(hwndDlg, pGlobalData);
                }
            }
            break;
    }

    return FALSE;
}

/* EOF */
