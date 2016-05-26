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
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

static HWND hwndEnum = NULL;

static BOOL CALLBACK
TimeFormatEnumProc(PWSTR lpTimeFormatString)
{
    SendMessageW(hwndEnum,
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


static VOID
GetSelectedComboEntry(HWND hwndDlg, DWORD dwIdc, WCHAR *Buffer, UINT uSize)
{
    int nIndex;
    HWND hChildWnd;

    /* Get handle to time format control */
    hChildWnd = GetDlgItem(hwndDlg, dwIdc);
    /* Get index to selected time format */
    nIndex = SendMessageW(hChildWnd, CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
        /* No selection? Get content of the edit control */
        SendMessageW(hChildWnd, WM_GETTEXT, uSize, (LPARAM)Buffer);
    else {
        PWSTR tmp;
        UINT   uReqSize;

        /* Get requested size, including the null terminator;
         * it shouldn't be required because the previous CB_LIMITTEXT,
         * but it would be better to check it anyways */
        uReqSize = SendMessageW(hChildWnd, CB_GETLBTEXTLEN, (WPARAM)nIndex, 0) + 1;
        /* Allocate enough space to be more safe */
        tmp = (PWSTR)_alloca(uReqSize*sizeof(WCHAR));
        /* Get selected time format text */
        SendMessageW(hChildWnd, CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)tmp);
        /* Finally, copy the result into the output */
        wcsncpy(Buffer, tmp, uSize);
    }
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
    hwndEnum = GetDlgItem(hwndDlg, IDC_TIMEFORMAT);
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
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                /* Get selected/typed time format text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEFORMAT,
                                      pGlobalData->szTimeFormat, 
                                      MAX_TIMEFORMAT);

                /* Get selected/typed time separator text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMESEPARATOR,
                                      pGlobalData->szTimeSep,
                                      MAX_TIMESEPARATOR);

                /* Get selected/typed AM symbol text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEAMSYMBOL,
                                      pGlobalData->szTimeAM,
                                      MAX_TIMEAMSYMBOL);

                /* Get selected/typed PM symbol text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEPMSYMBOL,
                                      pGlobalData->szTimePM,
                                      MAX_TIMEPMSYMBOL);

                pGlobalData->fUserLocaleChanged = TRUE;

                /* Update the time format sample */
                UpdateTimeSample(hwndDlg, pGlobalData);
            }
            break;
    }

    return FALSE;
}

/* EOF */
