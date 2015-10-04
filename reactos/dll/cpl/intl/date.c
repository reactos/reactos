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
 * FILE:            dll/cpl/intl/date.c
 * PURPOSE:         Date property page
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

/* GLOBALS ******************************************************************/

#define YEAR_STR_MAX_SIZE        5
#define MAX_SHRT_DATE_SEPARATORS 3
#define STD_DATE_SEP             L"."
#define YEAR_DIFF                (99)
#define MAX_YEAR                 (9999)

static HWND hwndEnum = NULL;

/* FUNCTIONS ****************************************************************/

/* If char is 'y' or 'M' or 'd' return TRUE, else FALSE */
BOOL
isDateCompAl(WCHAR alpha)
{
    if ((alpha == L'y') || (alpha == L'M') || (alpha == L'd') || (alpha == L' '))
        return TRUE;
    else
        return FALSE;
}

/* Find first date separator in string */
LPTSTR
FindDateSep(const WCHAR *szSourceStr)
{
    PWSTR pszFoundSep;
    UINT nDateCompCount=0;
    UINT nDateSepCount=0;

    pszFoundSep = (LPWSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    if(!pszFoundSep)
        return NULL;

    wcscpy(pszFoundSep,STD_DATE_SEP);

    while (nDateCompCount < wcslen(szSourceStr))
    {
        if (!isDateCompAl(szSourceStr[nDateCompCount]) && (szSourceStr[nDateCompCount] != L'\''))
        {
            while (!isDateCompAl(szSourceStr[nDateCompCount]) && (szSourceStr[nDateCompCount] != L'\''))
            {
                pszFoundSep[nDateSepCount++] = szSourceStr[nDateCompCount];
                nDateCompCount++;
            }

            pszFoundSep[nDateSepCount] = L'\0';
            return pszFoundSep;
        }

        nDateCompCount++;
    }

    return pszFoundSep;
}

/* Replace given template in source string with string to replace and return received string */


/* Setted up short date separator to registry */
static BOOL
SetShortDateSep(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szShortDateSep[MAX_SAMPLES_STR_SIZE];
    INT nSepStrSize;
    INT nSepCount;

    /* Get separator */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szShortDateSep);

    /* Get separator string size */
    nSepStrSize = wcslen(szShortDateSep);

    /* Check date components */
    for (nSepCount = 0; nSepCount < nSepStrSize; nSepCount++)
    {
        if (iswalnum(szShortDateSep[nSepCount]) || (szShortDateSep[nSepCount] == L'\''))
        {
            PrintErrorMsgBox(IDS_ERROR_SYMBOL_SEPARATE);
            return FALSE;
        }
    }

    /* Save date separator */
    wcscpy(pGlobalData->szDateSep, szShortDateSep);

    return TRUE;
}

/* Setted up short date format to registry */
static BOOL
SetShortDateFormat(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szShortDateFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR szShortDateSep[MAX_SAMPLES_STR_SIZE];
    WCHAR szFoundDateSep[MAX_SAMPLES_STR_SIZE];
    PWSTR pszResultStr;
    PWSTR pszFoundSep;
    BOOL OpenApostFlg = FALSE;
    INT nFmtStrSize;
    INT nDateCompCount;

    /* Get format */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szShortDateFmt);

    /* Get separator */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szShortDateSep);

    /* Get format-string size */
    nFmtStrSize = wcslen(szShortDateFmt);

    /* Check date components */
    for (nDateCompCount = 0; nDateCompCount < nFmtStrSize; nDateCompCount++)
    {
        if (szShortDateFmt[nDateCompCount] == L'\'')
        {
            OpenApostFlg = !OpenApostFlg;
        }

        if (iswalnum(szShortDateFmt[nDateCompCount]) &&
            !isDateCompAl(szShortDateFmt[nDateCompCount]) &&
            !OpenApostFlg)
        {
            PrintErrorMsgBox(IDS_ERROR_SYMBOL_FORMAT_SHORT);
            return FALSE;
        }

    }

    if (OpenApostFlg)
    {
        PrintErrorMsgBox(IDS_ERROR_SYMBOL_FORMAT_SHORT);
        return FALSE;
    }

    pszFoundSep = FindDateSep(szShortDateFmt);

    /* Substring replacement of separator */
    wcscpy(szFoundDateSep, pszFoundSep);
    pszResultStr = ReplaceSubStr(szShortDateFmt, szShortDateSep, szFoundDateSep);
    wcscpy(szShortDateFmt, pszResultStr);
    free(pszResultStr);

    if (pszFoundSep)
        free(pszFoundSep);

    /* Save short date format */
    wcscpy(pGlobalData->szShortDateFormat, szShortDateFmt);

    return TRUE;
}

/* Setted up long date format to registry */
static BOOL
SetLongDateFormat(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szLongDateFmt[MAX_SAMPLES_STR_SIZE];
    BOOL OpenApostFlg = FALSE;
    INT nFmtStrSize;
    INT nDateCompCount;

    /* Get format */
    SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szLongDateFmt);

    /* Get format string size */
    nFmtStrSize = wcslen(szLongDateFmt);

    /* Check date components */
    for (nDateCompCount = 0; nDateCompCount < nFmtStrSize; nDateCompCount++)
    {
        if (szLongDateFmt[nDateCompCount] == L'\'')
        {
            OpenApostFlg = !OpenApostFlg;
        }

        if (iswalnum(szLongDateFmt[nDateCompCount]) &&
            !isDateCompAl(szLongDateFmt[nDateCompCount]) &&
            !OpenApostFlg)
        {
            PrintErrorMsgBox(IDS_ERROR_SYMBOL_FORMAT_LONG);
            return FALSE;
        }

    }

    if (OpenApostFlg)
    {
        PrintErrorMsgBox(IDS_ERROR_SYMBOL_FORMAT_LONG);
        return FALSE;
    }

    /* Save long date format */
    wcscpy(pGlobalData->szLongDateFormat, szLongDateFmt);

    return TRUE;
}

/* Init short date separator control box */
static VOID
InitShortDateSepSamples(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    PWSTR ShortDateSepSamples[MAX_SHRT_DATE_SEPARATORS] =
    {
        L".",
        L"/",
        L"-"
    };
    INT nCBIndex;
    INT nRetCode;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of separators */
    for (nCBIndex = 0; nCBIndex < MAX_SHRT_DATE_SEPARATORS; nCBIndex++)
    {
        SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)ShortDateSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szDateSep);

    /* If it is not successful, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szDateSep);
        SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szDateSep);
    }
}

static BOOL CALLBACK
ShortDateFormatEnumProc(PWSTR lpTimeFormatString)
{
    SendMessageW(hwndEnum,
                 CB_ADDSTRING,
                 0,
                 (LPARAM)lpTimeFormatString);

    return TRUE;
}

/* Init short date control box */
VOID
InitShortDateCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nRetCode;

    /* Limit text lengths */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                        CB_LIMITTEXT,
                        MAX_SHORTDATEFORMAT,
                        0);
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESEP_COMBO,
                        CB_LIMITTEXT,
                        MAX_DATESEPARATOR,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Enumerate short date formats */
    hwndEnum = GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO);
    EnumDateFormatsW(ShortDateFormatEnumProc, pGlobalData->UserLCID, DATE_SHORTDATE);

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szShortDateFormat);

    /* If it is not successful, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szShortDateFormat);
        SendDlgItemMessageW(hwndDlg, IDC_SHRTDATEFMT_COMBO,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szShortDateFormat);
    }
}

/* Init long date control box */
static VOID
InitLongDateCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                        CB_LIMITTEXT,
                        MAX_LONGDATEFORMAT,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Enumerate short long formats */
    hwndEnum = GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO);
    EnumDateFormatsW(ShortDateFormatEnumProc, pGlobalData->UserLCID, DATE_LONGDATE);

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szLongDateFormat);

    /* If it is not successful, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szLongDateFormat);
        SendDlgItemMessageW(hwndDlg, IDC_LONGDATEFMT_COMBO,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szLongDateFormat);
    }
}

/* Set up max date value to registry */
static VOID
SetMaxDate(HWND hwndDlg, LCID lcid)
{
    WCHAR szMaxDateVal[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;
    INT nSpinVal;

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Get spin value */
    nSpinVal=LOWORD(SendMessageW(hWndYearSpin,
                    UDM_GETPOS,
                    0,
                    0));

    /* convert to wide char */
    _itow(nSpinVal, szMaxDateVal, DECIMAL_RADIX);

    /* Save max date value */
    SetCalendarInfoW(lcid,
                    CAL_GREGORIAN,
                    48 , /* CAL_ITWODIGITYEARMAX */
                    (PCWSTR)szMaxDateVal);
}

/* Get max date value from registry set */
static INT
GetMaxDate(LCID lcid)
{
    INT nMaxDateVal = 0;

    GetCalendarInfoW(lcid,
                     CAL_GREGORIAN,
                     CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                     NULL,
                     0, /* ret type - number */
                     (LPDWORD)&nMaxDateVal);

    return nMaxDateVal;
}

/* Set's MIN data edit control value to MAX-99 */
static VOID
SetMinDate(HWND hwndDlg)
{
    WCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;
    INT nSpinVal;

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Get spin value */
    nSpinVal = LOWORD(SendMessageW(hWndYearSpin,
                                   UDM_GETPOS,
                                   0,
                                   0));

    /* Set min year value */
    wsprintf(OutBuffer, L"%d", (DWORD)nSpinVal - YEAR_DIFF);
    SendDlgItemMessageW(hwndDlg, IDC_FIRSTYEAR_EDIT,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);
}

/* Init spin control */
static VOID
InitMinMaxDateSpin(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    HWND hWndYearSpin;

    /* Limit text lengths */
    SendDlgItemMessageW(hwndDlg, IDC_FIRSTYEAR_EDIT,
                EM_LIMITTEXT,
                MAX_YEAR_EDIT,
                0);
    SendDlgItemMessageW(hwndDlg, IDC_SECONDYEAR_EDIT,
                EM_LIMITTEXT,
                MAX_YEAR_EDIT,
                0);

    hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Init max date value */
    wsprintf(OutBuffer, L"%04d", (DWORD)GetMaxDate(pGlobalData->UserLCID));
    SendDlgItemMessageW(hwndDlg, IDC_SECONDYEAR_EDIT,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);

    /* Init min date value */
    wsprintf(OutBuffer, L"%04d", (DWORD)GetMaxDate(pGlobalData->UserLCID) - YEAR_DIFF);
    SendDlgItemMessageW(hwndDlg, IDC_FIRSTYEAR_EDIT,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);

    /* Init updown control */
    /* Set bounds */
    SendMessageW(hWndYearSpin,
                 UDM_SETRANGE,
                 0,
                 MAKELONG(MAX_YEAR,YEAR_DIFF));

    /* Set current value */
    SendMessageW(hWndYearSpin,
                 UDM_SETPOS,
                 0,
                 MAKELONG(GetMaxDate(pGlobalData->UserLCID),0));
}

/* Update all date locale samples */
static VOID
UpdateDateLocaleSamples(HWND hwndDlg,
                        PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[MAX_SAMPLES_STR_SIZE];

    /* Get short date format sample */
    GetDateFormatW(pGlobalData->UserLCID, DATE_SHORTDATE, NULL, NULL, OutBuffer,
                   MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_SHRTDATESAMPLE_EDIT, WM_SETTEXT,
                        0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormatW(pGlobalData->UserLCID, DATE_LONGDATE, NULL, NULL, OutBuffer,
                   MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_LONGDATESAMPLE_EDIT,
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

            InitMinMaxDateSpin(hwndDlg, pGlobalData);
            UpdateDateLocaleSamples(hwndDlg, pGlobalData);
            InitShortDateCB(hwndDlg, pGlobalData);
            InitLongDateCB(hwndDlg, pGlobalData);
            InitShortDateSepSamples(hwndDlg, pGlobalData);
            /* TODO: Add other calendar types */
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SECONDYEAR_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        SetMinDate(hwndDlg);
                    }
                    break;

                case IDC_SCR_MAX_YEAR:
                    /* Set "Apply" button enabled */
                    /* FIXME */
                    //PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_CALTYPE_COMBO:
                case IDC_HIJCHRON_COMBO:
                case IDC_SHRTDATEFMT_COMBO:
                case IDC_LONGDATEFMT_COMBO:
                case IDC_SHRTDATESEP_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Set "Apply" button enabled */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                if (!SetLongDateFormat(hwndDlg, pGlobalData))
                    break;

                if (!SetShortDateFormat(hwndDlg, pGlobalData))
                    break;

                if (!SetShortDateSep(hwndDlg, pGlobalData))
                    break;

                pGlobalData->fUserLocaleChanged = TRUE;

                SetMaxDate(hwndDlg, pGlobalData->UserLCID);
                InitShortDateCB(hwndDlg, pGlobalData);
                UpdateDateLocaleSamples(hwndDlg, pGlobalData);
            }
            break;
    }

    return FALSE;
}

/* EOF */
