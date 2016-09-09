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
 * FILE:            dll/cpl/intl/numbers.c
 * PURPOSE:         Numbers property page
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

#define SAMPLE_NUMBER               L"123456789"
#define SAMPLE_NEG_NUMBER           L"-123456789"
#define MAX_NUM_SEP_SAMPLES         2
#define MAX_FRAC_NUM_SAMPLES        10
#define MAX_FIELD_SEP_SAMPLES       1
#define MAX_FIELD_DIG_SAMPLES       3
#define MAX_NEG_SIGN_SAMPLES        1
#define MAX_NEG_NUMBERS_SAMPLES     5
#define MAX_LEAD_ZEROES_SAMPLES     2
#define MAX_LIST_SEP_SAMPLES        1
#define MAX_UNITS_SYS_SAMPLES       2

static PWSTR lpNumSepSamples[MAX_NUM_SEP_SAMPLES] =
    {L",", L"."};
static PWSTR lpFieldSepSamples[MAX_FIELD_SEP_SAMPLES] =
    {L" "};
static PWSTR lpFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES] =
    {L"0;0", L"3;0", L"3;2;0"};
static PWSTR lpNegSignSamples[MAX_NEG_SIGN_SAMPLES] =
    {L"-"};
static PWSTR lpNegNumFmtSamples[MAX_NEG_NUMBERS_SAMPLES] =
    {L"(1,1)", L"-1,1", L"- 1,1", L"1,1-", L"1,1 -"};
static PWSTR lpLeadNumFmtSamples[MAX_LEAD_ZEROES_SAMPLES] =
    {L",7", L"0,7"};
static PWSTR lpListSepSamples[MAX_LIST_SEP_SAMPLES] =
    {L";"};


/* Init num decimal separator control box */
static VOID
InitNumDecimalSepCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                        CB_LIMITTEXT,
                        MAX_NUMDECIMALSEP - 1,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of decimal separators */
    for (nCBIndex = 0; nCBIndex < MAX_NUM_SEP_SAMPLES; nCBIndex++)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lpNumSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szNumDecimalSep);

    /* If it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                            CB_ADDSTRING,
                            MAX_NUM_SEP_SAMPLES,
                            (LPARAM)pGlobalData->szNumDecimalSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szNumDecimalSep);
    }
}

/* Init number of fractional symbols control box */
static VOID
InitNumOfFracSymbCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szFracCount[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNDIGDEC,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of fractional symbols */
    for (nCBIndex = 0; nCBIndex < MAX_FRAC_NUM_SAMPLES; nCBIndex++)
    {
        /* Convert to wide char */
        _itow(nCBIndex, szFracCount, DECIMAL_RADIX);

        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNDIGDEC,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szFracCount);
    }

    /* Set current item to value from registry */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNDIGDEC,
                        CB_SETCURSEL,
                        (WPARAM)pGlobalData->nNumDigits,
                        (LPARAM)0);
}

/* Init field separator control box */
static VOID
InitNumFieldSepCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                        CB_LIMITTEXT,
                        MAX_NUMTHOUSANDSEP - 1,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of field separators */
    for (nCBIndex = 0; nCBIndex < MAX_FIELD_SEP_SAMPLES; nCBIndex++)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lpFieldSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szNumThousandSep);

    /* If it is not success, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szNumThousandSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szNumThousandSep);
    }
}

/* Init number of digits in field control box */
static VOID
InitFieldDigNumCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    PWSTR pszFieldDigNumSmpl;
    INT nCBIndex;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of field digits num */
    for (nCBIndex = 0; nCBIndex < MAX_FIELD_DIG_SAMPLES; nCBIndex++)
    {
        pszFieldDigNumSmpl = InsSpacesFmt(SAMPLE_NUMBER, lpFieldDigNumSamples[nCBIndex]);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pszFieldDigNumSmpl);
        free(pszFieldDigNumSmpl);
    }

    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                        CB_SETCURSEL,
                        (WPARAM)pGlobalData->nNumGrouping,
                        (LPARAM)0);
}

/* Init negative sign control box */
static VOID
InitNegSignCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                        CB_LIMITTEXT,
                        MAX_NUMNEGATIVESIGN - 1,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of signs */
    for (nCBIndex = 0; nCBIndex < MAX_NEG_SIGN_SAMPLES; nCBIndex++)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lpNegSignSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szNumNegativeSign);

    /* If  it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szNumNegativeSign);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szNumNegativeSign);
    }
}

/* Init negative numbers format control box */
static VOID
InitNegNumFmtCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szNewSample[MAX_SAMPLES_STR_SIZE];
    PWSTR pszResultStr;
    INT nCBIndex;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of negative numbers formats */
    for (nCBIndex = 0; nCBIndex < MAX_NEG_NUMBERS_SAMPLES; nCBIndex++)
    {
        /* Replace standard separator to setted */
        pszResultStr = ReplaceSubStr(lpNegNumFmtSamples[nCBIndex],
                                     pGlobalData->szNumDecimalSep,
                                     L",");
        wcscpy(szNewSample, pszResultStr);
        free(pszResultStr);

        /* Replace standard negative sign to setted */
        pszResultStr = ReplaceSubStr(szNewSample,
                                     pGlobalData->szNumNegativeSign,
                                     L"-");
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pszResultStr);
        free(pszResultStr);
    }

    /* Set current item to value from registry */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                        CB_SETCURSEL,
                        (WPARAM)pGlobalData->nNumNegFormat,
                        (LPARAM)0);
}

/* Init leading zeroes control box */
static VOID
InitLeadingZeroesCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    PWSTR pszResultStr;
    INT nCBIndex;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDISPLEADZER,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create list of standard leading zeroes formats */
    for (nCBIndex = 0; nCBIndex < MAX_LEAD_ZEROES_SAMPLES; nCBIndex++)
    {
        pszResultStr = ReplaceSubStr(lpLeadNumFmtSamples[nCBIndex],
                                     pGlobalData->szNumDecimalSep,
                                     L",");
        SendDlgItemMessage(hwndDlg, IDC_NUMBERSDISPLEADZER,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)pszResultStr);
        free(pszResultStr);
    }

    /* Set current item to value from registry */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDISPLEADZER,
                        CB_SETCURSEL,
                        (WPARAM)pGlobalData->nNumLeadingZero,
                        (LPARAM)0);
}

static VOID
InitListSepCB(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                        CB_LIMITTEXT,
                        MAX_NUMLISTSEP - 1,
                        0);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create standard list of signs */
    for (nCBIndex = 0; nCBIndex < MAX_LIST_SEP_SAMPLES; nCBIndex++)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lpListSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pGlobalData->szNumListSep);

    /* If it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pGlobalData->szNumListSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pGlobalData->szNumListSep);
    }
}

/* Init system of units control box */
static VOID
InitUnitsSysCB(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szUnitName[128];
    INT nCBIndex;

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSMEASSYS,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Create list of standard system of units */
    for (nCBIndex = 0; nCBIndex < MAX_UNITS_SYS_SAMPLES; nCBIndex++)
    {
        LoadStringW(hApplet, IDS_METRIC + nCBIndex, szUnitName, 128);

        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSMEASSYS,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szUnitName);
    }

    /* Set current item to value from registry */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSMEASSYS,
                        CB_SETCURSEL,
                        (WPARAM)pGlobalData->nNumMeasure,
                        (LPARAM)0);
}

/* Update all numbers locale samples */
static VOID
UpdateNumSamples(HWND hwndDlg,
                 PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];
    NUMBERFMT NumberFormat;

    NumberFormat.NumDigits = pGlobalData->nNumDigits;
    NumberFormat.LeadingZero = pGlobalData->nNumLeadingZero;
    NumberFormat.Grouping = GroupingFormats[pGlobalData->nNumGrouping].nInteger;
    NumberFormat.lpDecimalSep = pGlobalData->szNumDecimalSep;
    NumberFormat.lpThousandSep = pGlobalData->szNumThousandSep;
    NumberFormat.NegativeOrder = pGlobalData->nNumNegFormat;

    /* Get positive number format sample */
    GetNumberFormatW(pGlobalData->UserLCID,
                     0,
                     SAMPLE_NUMBER,
                     &NumberFormat,
                     OutBuffer,
                     MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSPOSSAMPLE,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);

    /* Get positive number format sample */
    GetNumberFormatW(pGlobalData->UserLCID,
                     0,
                     SAMPLE_NEG_NUMBER,
                     &NumberFormat,
                     OutBuffer,
                     MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNEGSAMPLE,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);
}

/* Set num decimal separator */
static BOOL
SetNumDecimalSep(HWND hwndDlg,
                 PGLOBALDATA pGlobalData)
{
    /* Get setted decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                        WM_GETTEXT,
                        (WPARAM)MAX_NUMDECIMALSEP,
                        (LPARAM)pGlobalData->szNumDecimalSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetFracSymNum(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNDIGDEC,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nNumDigits = nCurrSel;

    return TRUE;
}

/* Set field separator */
static BOOL
SetNumFieldSep(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    /* Get thousand separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                        WM_GETTEXT,
                        (WPARAM)MAX_NUMTHOUSANDSEP,
                        (LPARAM)pGlobalData->szNumThousandSep);

    return TRUE;
}

/* Set number of digits in field */
static BOOL
SetFieldDigNum(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted negative sum format */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nNumGrouping = nCurrSel;

    return TRUE;
}

/* Set negative sign */
static BOOL
SetNumNegSign(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    /* Get setted negative sign */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                        WM_GETTEXT,
                        (WPARAM)MAX_NUMNEGATIVESIGN,
                        (LPARAM)pGlobalData->szNumNegativeSign);

    return TRUE;
}

/* Set negative sum format */
static BOOL
SetNegSumFmt(HWND hwndDlg,
             PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted negative sum format */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nNumNegFormat = nCurrSel;

    return TRUE;
}

/* Set leading zero */
static BOOL
SetNumLeadZero(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted leading zero format */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDISPLEADZER,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nNumLeadingZero = nCurrSel;

    return TRUE;
}

/* Set elements list separator */
static BOOL
SetNumListSep(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    /* Get setted list separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_NUMLISTSEP,
                        (LPARAM)pGlobalData->szNumListSep);

    return TRUE;
}

/* Set units system */
static BOOL
SetNumUnitsSys(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    INT nCurrSel;

    /* Get setted units system */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSMEASSYS,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);
    if (nCurrSel == CB_ERR)
        return FALSE;

    pGlobalData->nNumMeasure = nCurrSel;

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
NumbersPageProc(HWND hwndDlg,
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

            InitNumDecimalSepCB(hwndDlg, pGlobalData);
            InitNumOfFracSymbCB(hwndDlg, pGlobalData);
            InitNumFieldSepCB(hwndDlg, pGlobalData);
            InitFieldDigNumCB(hwndDlg, pGlobalData);
            InitNegSignCB(hwndDlg, pGlobalData);
            InitNegNumFmtCB(hwndDlg, pGlobalData);
            InitLeadingZeroesCB(hwndDlg, pGlobalData);
            InitListSepCB(hwndDlg, pGlobalData);
            InitUnitsSysCB(hwndDlg, pGlobalData);
            UpdateNumSamples(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_NUMBERDSYMBOL:
                case IDC_NUMBERSNDIGDEC:
                case IDC_NUMBERSDIGITGRSYM:
                case IDC_NUMBERSDGROUPING:
                case IDC_NUMBERSNSIGNSYM:
                case IDC_NUMBERSNNUMFORMAT:
                case IDC_NUMBERSDISPLEADZER:
                case IDC_NUMBERSLSEP:
                case IDC_NUMBERSMEASSYS:
                    if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Set "Apply" button enabled */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
            }
            break;

        case WM_NOTIFY:
            /* If push apply button */
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                if (!SetNumDecimalSep(hwndDlg, pGlobalData))
                    break;

                if (!SetFracSymNum(hwndDlg, pGlobalData))
                    break;

                if (!SetNumFieldSep(hwndDlg, pGlobalData))
                    break;

                if (!SetFieldDigNum(hwndDlg, pGlobalData))
                    break;

                if (!SetNumNegSign(hwndDlg, pGlobalData))
                    break;

                if (!SetNegSumFmt(hwndDlg, pGlobalData))
                    break;

                if (!SetNumLeadZero(hwndDlg, pGlobalData))
                    break;

                if (!SetNumListSep(hwndDlg, pGlobalData))
                    break;

                if (!SetNumUnitsSys(hwndDlg, pGlobalData))
                    break;

                pGlobalData->fUserLocaleChanged = TRUE;

                UpdateNumSamples(hwndDlg, pGlobalData);
            }
            break;
    }
    return FALSE;
}

/* EOF */
