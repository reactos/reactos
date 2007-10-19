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
 * FILE:            lib/cpl/intl/date.c
 * PURPOSE:         Date property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define YEAR_STR_MAX_SIZE        4
#define MAX_SHRT_DATE_SEPARATORS 3
#define STD_DATE_SEP             _T(".")
#define YEAR_DIFF                (99)
#define MAX_YEAR                 (9999)

static HWND hwndEnum = NULL;

/* FUNCTIONS ****************************************************************/

/* if char is 'y' or 'M' or 'd' return TRUE, else FALSE */
BOOL
isDateCompAl(TCHAR alpha)
{
    if ((alpha == _T('y')) || (alpha == _T('M')) || (alpha == _T('d')) || (alpha == _T(' ')))
        return TRUE;
    else
        return FALSE;
}

/* Find first date separator in string */
LPTSTR
FindDateSep(const TCHAR *szSourceStr)
{
    LPTSTR pszFoundSep;
    INT nDateCompCount=0;
    INT nDateSepCount=0;

    pszFoundSep = (LPTSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(TCHAR));

    _tcscpy(pszFoundSep,STD_DATE_SEP);

    while (nDateCompCount < _tcslen(szSourceStr))
    {
        if (!isDateCompAl(szSourceStr[nDateCompCount]) && (szSourceStr[nDateCompCount] != _T('\'')))
        {
            while (!isDateCompAl(szSourceStr[nDateCompCount]) && (szSourceStr[nDateCompCount] != _T('\'')))
            {
                pszFoundSep[nDateSepCount++] = szSourceStr[nDateCompCount];
                nDateCompCount++;
            }

            pszFoundSep[nDateSepCount] = _T('\0');
            return pszFoundSep;
        }

        nDateCompCount++;
    }

    return pszFoundSep;
}

/* Replace given template in source string with string to replace and return recieved string */


/* Setted up short date separator to registry */
static BOOL
SetShortDateSep(HWND hwndDlg, LCID lcid)
{
    TCHAR szShortDateSep[MAX_SAMPLES_STR_SIZE];
    INT nSepStrSize;
    INT nSepCount;

    /* Get setted separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szShortDateSep);

    /* Get setted separator string size */
    nSepStrSize = _tcslen(szShortDateSep);

    /* Check date components */
    for (nSepCount = 0; nSepCount < nSepStrSize; nSepCount++)
    {
        if (_istalnum(szShortDateSep[nSepCount]) || (szShortDateSep[nSepCount] == _T('\'')))
        {
            MessageBox(NULL,
                       _T("Entered short date separator contain incorrect symbol"),
                       _T("Error"), MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    /* Save date separator */
    SetLocaleInfo(lcid, LOCALE_SDATE, szShortDateSep);

    return TRUE;
}

/* Setted up short date format to registry */
static BOOL
SetShortDateFormat(HWND hwndDlg, LCID lcid)
{
    TCHAR szShortDateFmt[MAX_SAMPLES_STR_SIZE];
    TCHAR szShortDateSep[MAX_SAMPLES_STR_SIZE];
    TCHAR szFindedDateSep[MAX_SAMPLES_STR_SIZE];
    LPTSTR pszResultStr;
    BOOL OpenApostFlg = FALSE;
    INT nFmtStrSize;
    INT nDateCompCount;

    /* Get setted format */
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szShortDateFmt);

    /* Get setted separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szShortDateSep);

    /* Get setted format-string size */
    nFmtStrSize = _tcslen(szShortDateFmt);

    /* Check date components */
    for (nDateCompCount = 0; nDateCompCount < nFmtStrSize; nDateCompCount++)
    {
        if (szShortDateFmt[nDateCompCount] == _T('\''))
        {
            OpenApostFlg = !OpenApostFlg;
        }

        if (_istalnum(szShortDateFmt[nDateCompCount]) &&
            !isDateCompAl(szShortDateFmt[nDateCompCount]) &&
            !OpenApostFlg)
        {
            MessageBox(NULL,
                       _T("Entered short date format contain incorrect symbol"),
                       _T("Error"), MB_OK | MB_ICONERROR);
            return FALSE;
        }

    }

    if (OpenApostFlg)
    {
        MessageBoxW(NULL,
                    _T("Entered short date format contain incorrect symbol"),
                    _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* substring replacement of separator */
    _tcscpy(szFindedDateSep, FindDateSep(szShortDateFmt));
    pszResultStr = ReplaceSubStr(szShortDateFmt, szShortDateSep, szFindedDateSep);
    _tcscpy(szShortDateFmt, pszResultStr);
    free(pszResultStr);

    /* Save short date format */
    SetLocaleInfo(lcid, LOCALE_SSHORTDATE, szShortDateFmt);

    return TRUE;
}

/* Setted up long date format to registry */
static BOOL
SetLongDateFormat(HWND hwndDlg, LCID lcid)
{
    TCHAR szLongDateFmt[MAX_SAMPLES_STR_SIZE];
    BOOL OpenApostFlg = FALSE;
    INT nFmtStrSize;
    INT nDateCompCount;

    /* Get setted format */
    SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szLongDateFmt);

    /* Get setted format string size */
    nFmtStrSize = _tcslen(szLongDateFmt);

    /* Check date components */
    for (nDateCompCount = 0; nDateCompCount < nFmtStrSize; nDateCompCount++)
    {
        if (szLongDateFmt[nDateCompCount] == _T('\''))
        {
            OpenApostFlg = !OpenApostFlg;
        }

        if (_istalnum(szLongDateFmt[nDateCompCount]) &&
            !isDateCompAl(szLongDateFmt[nDateCompCount]) &&
            !OpenApostFlg)
        {
            MessageBox(NULL,
                       _T("Entered long date format contain incorrect symbol"),
                       _T("Error"), MB_OK | MB_ICONERROR);
            return FALSE;
        }

    }

    if (OpenApostFlg)
    {
        MessageBoxW(NULL,
                    _T("Entered long date format contain incorrect symbol"),
                    _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* Save short date format */
    SetLocaleInfo(lcid, LOCALE_SLONGDATE, szLongDateFmt);

    return TRUE;
}

/* Init short date separator control box */
static VOID
InitShortDateSepSamples(HWND hwndDlg, LCID lcid)
{
    LPTSTR ShortDateSepSamples[MAX_SHRT_DATE_SEPARATORS] =
    {
        _T("."),
        _T("/"),
        _T("-")
    };
    TCHAR szShortDateSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current short date separator */
    GetLocaleInfo(lcid,
                  LOCALE_SDATE,
                  szShortDateSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standart list of separators */
    for (nCBIndex = 0; nCBIndex < MAX_SHRT_DATE_SEPARATORS; nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                     CB_ADDSTRING,
                     0,
                     (LPARAM)ShortDateSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szShortDateSep);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szShortDateSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)szShortDateSep);
    }
}

static BOOL CALLBACK
ShortDateFormatEnumProc(LPTSTR lpTimeFormatString)
{
    SendMessage(hwndEnum,
                CB_ADDSTRING,
                0,
                (LPARAM)lpTimeFormatString);

    return TRUE;
}

/* Init short date control box */
VOID
InitShortDateCB(HWND hwndDlg, LCID lcid)
{
    TCHAR szShortDateFmt[MAX_SAMPLES_STR_SIZE];
    INT nRetCode;

    /* Get current short date format */
    GetLocaleInfo(lcid,
                  LOCALE_SSHORTDATE,
                  szShortDateFmt,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Enumerate short date formats */
    hwndEnum = GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO);
    EnumDateFormats(ShortDateFormatEnumProc, lcid, DATE_SHORTDATE);

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szShortDateFmt);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szShortDateFmt);
        SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szShortDateFmt);
    }
}

/* Init long date control box */
static VOID
InitLongDateCB(HWND hwndDlg, LCID lcid)
{
    TCHAR szLongDateFmt[MAX_SAMPLES_STR_SIZE];
    INT nRetCode;

    /* Get current long date format */
    GetLocaleInfo(lcid,
                  LOCALE_SLONGDATE,
                  szLongDateFmt,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Enumerate short long formats */
    hwndEnum = GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO);
    EnumDateFormats(ShortDateFormatEnumProc, lcid, DATE_LONGDATE);

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szLongDateFmt);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szLongDateFmt);
        SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szLongDateFmt);
    }
}

/* Set up max date value to registry */
static VOID
SetMaxDate(HWND hwndDlg, LCID lcid)
{
    TCHAR szMaxDateVal[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;
    INT nSpinVal;

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Get spin value */
    nSpinVal=LOWORD(SendMessage(hWndYearSpin,
                    UDM_GETPOS,
                    0,
                    0));

    /* convert to wide char */
    _itot(nSpinVal, szMaxDateVal, DECIMAL_RADIX);

    /* Save max date value */
    SetCalendarInfo(lcid,
                    CAL_GREGORIAN,
                    48 , /* CAL_ITWODIGITYEARMAX */
                    (LPCTSTR)&szMaxDateVal);
}

/* Get max date value from registry set */
static INT
GetMaxDate(LCID lcid)
{
    INT nMaxDateVal;

    GetCalendarInfo(lcid,
                    CAL_GREGORIAN,
                    CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                    NULL,
                    0, /* ret type - number */
                    (LPDWORD)&nMaxDateVal);

    return nMaxDateVal;
}

/* Set's MIN data edit control value to MAX-99 */
static VOID
SetMinData(HWND hwndDlg)
{
    TCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;
    INT nSpinVal;

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Get spin value */
    nSpinVal = LOWORD(SendMessage(hWndYearSpin,
                                  UDM_GETPOS,
                                  0,
                                  0));

    /* Set min year value */
    wsprintf(OutBuffer, _T("%d"), (DWORD)nSpinVal - YEAR_DIFF);
    SendMessage(GetDlgItem(hwndDlg, IDC_FIRSTYEAR_EDIT),
                WM_SETTEXT,
                0,
                (LPARAM)OutBuffer);
}

/* Init spin control */
static VOID
InitMinMaxDateSpin(HWND hwndDlg, LCID lcid)
{
    TCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Init max date value */
    wsprintf(OutBuffer, _T("%04d"), (DWORD)GetMaxDate(lcid));
    SendMessage(GetDlgItem(hwndDlg, IDC_SECONDYEAR_EDIT),
                WM_SETTEXT,
                0,
                (LPARAM)OutBuffer);

    /* Init min date value */
    wsprintf(OutBuffer, _T("%04d"), (DWORD)GetMaxDate(lcid) - YEAR_DIFF);
    SendMessage(GetDlgItem(hwndDlg, IDC_FIRSTYEAR_EDIT),
                WM_SETTEXT,
                0,
                (LPARAM)OutBuffer);

    /* Init updown control */
    /* Set bounds */
    SendMessage(hWndYearSpin,
                UDM_SETRANGE,
                0,
                MAKELONG(MAX_YEAR,YEAR_DIFF));

    /* Set current value */
    SendMessage(hWndYearSpin,
                UDM_SETPOS,
                0,
                MAKELONG(GetMaxDate(lcid),0));
}

/* Update all date locale samples */
static VOID
UpdateDateLocaleSamples(HWND hwndDlg,
                        LCID lcidLocale)
{
    TCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get short date format sample */
    GetDateFormat(lcidLocale, DATE_SHORTDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_SHRTDATESAMPLE_EDIT), WM_SETTEXT,
        0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormat(lcidLocale, DATE_LONGDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_LONGDATESAMPLE_EDIT),
        WM_SETTEXT, 0, (LPARAM)OutBuffer);
}

/* Property page dialog callback */
INT_PTR CALLBACK
DatePageProc(HWND hwndDlg,
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

            InitMinMaxDateSpin(hwndDlg, pGlobalData->lcid);
            UpdateDateLocaleSamples(hwndDlg, pGlobalData->lcid);
            InitShortDateCB(hwndDlg, pGlobalData->lcid);
            InitLongDateCB(hwndDlg, pGlobalData->lcid);
            InitShortDateSepSamples(hwndDlg, pGlobalData->lcid);
            /* TODO: Add other calendar types */
            break;

        case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case IDC_SECONDYEAR_EDIT:
            {
                if(HIWORD(wParam)==EN_CHANGE)
                {
                    SetMinData(hwndDlg);
                }
            }
            case IDC_SCR_MAX_YEAR:
            {
				/* Set "Apply" button enabled */
				/* FIXME */
				//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;
            case IDC_CALTYPE_COMBO:
            case IDC_HIJCHRON_COMBO:
            case IDC_SHRTDATEFMT_COMBO:
            case IDC_SHRTDATESEP_COMBO:
            case IDC_LONGDATEFMT_COMBO:
            {
                if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                {
                    /* Set "Apply" button enabled */
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            break;
        }
    }
    break;
    case WM_NOTIFY:
    {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        /* If push apply button */
        if (lpnm->code == (UINT)PSN_APPLY)
        {
            SetMaxDate(hwndDlg, pGlobalData->lcid);
            if(!SetShortDateSep(hwndDlg, pGlobalData->lcid)) break;
            if(!SetShortDateFormat(hwndDlg, pGlobalData->lcid)) break;
            if(!SetLongDateFormat(hwndDlg, pGlobalData->lcid)) break;
            InitShortDateCB(hwndDlg, pGlobalData->lcid);
            /* FIXME: */
            //Sleep(15);
            UpdateDateLocaleSamples(hwndDlg, pGlobalData->lcid);
        }
    }
    break;
    }

    return FALSE;
}

/* EOF */
