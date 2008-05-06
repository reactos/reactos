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
/*
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/time.c
 * PURPOSE:         Time property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <malloc.h>

#include "intl.h"
#include "resource.h"

static HWND hwndEnum = NULL;

static BOOL CALLBACK
TimeFormatEnumProc(LPTSTR lpTimeFormatString)
{
    SendMessage(hwndEnum,
                CB_ADDSTRING,
                0,
                (LPARAM)lpTimeFormatString);

    return TRUE;
}

static VOID
UpdateTimeSample(HWND hWnd, LCID lcid)
{
    TCHAR szBuffer[80];

    GetTimeFormat(lcid, 0, NULL, NULL, szBuffer, 80);
    SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szBuffer);
}


static VOID
GetSelectedComboEntry(HWND hwndDlg, DWORD dwIdc, TCHAR *Buffer, UINT uSize)
{
    int nIndex;
    HWND hChildWnd;

    /* get handle to time format control */
    hChildWnd = GetDlgItem(hwndDlg, dwIdc);
    /* Get index to selected time format */
    nIndex = SendMessage(hChildWnd, CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
        /* no selection? get content of the edit control */
        SendMessage(hChildWnd, WM_GETTEXT, uSize, (LPARAM)Buffer);
    else {
        LPTSTR tmp;
        UINT   uReqSize;

        /* get requested size, including the null terminator;
         * it shouldn't be required because the previous CB_LIMITTEXT,
         * but it would be better to check it anyways */
        uReqSize = SendMessage(hChildWnd, CB_GETLBTEXTLEN, (WPARAM)nIndex, 0) + 1;
        /* allocate enought space, to be more safe */
        tmp = (LPTSTR)_alloca(uReqSize*sizeof(TCHAR));
        /* get selected time format text */
        SendMessage(hChildWnd, CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)tmp);
        /* finally, copy the result into the output */
        _tcsncpy(Buffer, tmp, uSize);
    }
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
        {
            TCHAR Buffer[80];
            int nLen;

            pGlobalData = (PGLOBALDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            /* Update the time format sample */
            UpdateTimeSample(GetDlgItem(hwndDlg, IDC_TIMESAMPLE), pGlobalData->lcid);

            /* Get the time format (max. 80 characters) */
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEFORMAT),
                        CB_LIMITTEXT, 80, 0);

            /* Add available time formats to the list */
            hwndEnum = GetDlgItem(hwndDlg, IDC_TIMEFORMAT);
            EnumTimeFormats(TimeFormatEnumProc, pGlobalData->lcid, 0);

            GetLocaleInfo(pGlobalData->lcid, LOCALE_STIMEFORMAT, Buffer, sizeof(Buffer)/sizeof(TCHAR));
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEFORMAT),
                        CB_SELECTSTRING,
                        -1,
                        (LPARAM)Buffer);

            /* Get the time separator (max. 4 characters) */
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                        CB_LIMITTEXT, 4, 0);
            GetLocaleInfo(pGlobalData->lcid, LOCALE_STIME, Buffer, sizeof(Buffer)/sizeof(TCHAR));
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                        CB_ADDSTRING,
                        0,
                        (LPARAM)Buffer);
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                        CB_SETCURSEL,
                        0, /* index */
                        0);

            /* Get the AM symbol (max. 9 characters) */
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                        CB_LIMITTEXT, 9, 0);
            nLen = GetLocaleInfo(pGlobalData->lcid, LOCALE_S1159, Buffer, 80);
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                        CB_ADDSTRING,
                        0,
                        (LPARAM)Buffer);
            if (nLen != 0)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                            CB_ADDSTRING,
                            0,
                            (LPARAM)_T(""));
            }
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                        CB_SETCURSEL,
                        0, /* index */
                        0);

            /* Get the PM symbol (max. 9 characters) */
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                        CB_LIMITTEXT, 9, 0);
            nLen = GetLocaleInfo(pGlobalData->lcid, LOCALE_S2359, Buffer, 80);
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                        CB_ADDSTRING,
                        0,
                        (LPARAM)Buffer);
            if (nLen != 0)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                           CB_ADDSTRING,
                           0,
                           (LPARAM)_T(""));
            }
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                        CB_SETCURSEL,
                        0, /* index */
                        0);
        }
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
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            if (lpnm->code == (UINT)PSN_APPLY)
            {
                TCHAR Buffer[80];

                /* get selected/typed time format text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEFORMAT, Buffer, sizeof(Buffer)/sizeof(TCHAR));

                /* Set time format */
                SetLocaleInfo(pGlobalData->lcid, LOCALE_STIMEFORMAT, Buffer);

                /* get selected/typed time separator text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMESEPARATOR, Buffer, sizeof(Buffer)/sizeof(TCHAR));

                /* Set time separator */
                SetLocaleInfo(pGlobalData->lcid, LOCALE_STIME, Buffer);

                /* get selected/typed AM symbol text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEAMSYMBOL, Buffer, sizeof(Buffer)/sizeof(TCHAR));

                /* Set the AM symbol */
                SetLocaleInfo(pGlobalData->lcid, LOCALE_S1159, Buffer);

                /* get selected/typed PM symbol text */
                GetSelectedComboEntry(hwndDlg, IDC_TIMEPMSYMBOL, Buffer, sizeof(Buffer)/sizeof(TCHAR));

                /* Set the PM symbol */
                SetLocaleInfo(pGlobalData->lcid, LOCALE_S2359, Buffer);

                /* Update the time format sample */
                UpdateTimeSample(GetDlgItem(hwndDlg, IDC_TIMESAMPLE), pGlobalData->lcid);
            }
        }
        break;
    }

    return FALSE;
}

/* EOF */
