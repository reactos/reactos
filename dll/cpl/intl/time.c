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

        GetLocaleInfo(pGlobalData->lcid, LOCALE_STIMEFORMAT, Buffer, 80);
        SendMessage(GetDlgItem(hwndDlg, IDC_TIMEFORMAT),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)Buffer);

        /* Get the time separator (max. 4 characters) */
        SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                    CB_LIMITTEXT, 4, 0);
        GetLocaleInfo(pGlobalData->lcid, LOCALE_STIME, Buffer, 80);
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
          int nIndex;

          /* Set time format */
          nIndex = SendMessage(GetDlgItem(hwndDlg, IDC_TIMEFORMAT),
                               CB_GETCURSEL, 0, 0);
          SendMessage(GetDlgItem(hwndDlg, IDC_TIMEFORMAT),
                      CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)Buffer);
          SetLocaleInfo(pGlobalData->lcid, LOCALE_STIMEFORMAT, Buffer);

          /* Set time separator */
          nIndex = SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                               CB_GETCURSEL, 0, 0);
          SendMessage(GetDlgItem(hwndDlg, IDC_TIMESEPARATOR),
                      CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)Buffer);
          SetLocaleInfo(pGlobalData->lcid, LOCALE_STIME, Buffer);

          /* Set the AM symbol */
          nIndex = SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                               CB_GETCURSEL, 0, 0);
          if (nIndex != CB_ERR)
          {
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEAMSYMBOL),
                        CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)Buffer);
            SetLocaleInfo(pGlobalData->lcid, LOCALE_S1159, Buffer);
          }
          else
          {
            SetLocaleInfo(pGlobalData->lcid, LOCALE_S1159, _T(""));
          }

          /* Set the PM symbol */
          nIndex = SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                               CB_GETCURSEL, 0, 0);
          if (nIndex != CB_ERR)
          {
            SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPMSYMBOL),
                        CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)Buffer);
            SetLocaleInfo(pGlobalData->lcid, LOCALE_S2359, Buffer);
          }
          else
          {
            SetLocaleInfo(pGlobalData->lcid, LOCALE_S2359, _T(""));
          }

          /* Update the time format sample */
          UpdateTimeSample(GetDlgItem(hwndDlg, IDC_TIMESAMPLE), pGlobalData->lcid);
        }
      }
      break;
    }

    return FALSE;
}

/* EOF */
