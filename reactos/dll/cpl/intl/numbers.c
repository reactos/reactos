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
    WCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                        CB_LIMITTEXT,
                        MAX_NUMBERDSYMBOL,
                        0);

    /* Get current decimal separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SDECIMAL,
                   szNumSep,
                   MAX_SAMPLES_STR_SIZE);

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
                                   (LPARAM)(LPCSTR)szNumSep);

    /* If it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                            CB_ADDSTRING,
                            MAX_NUM_SEP_SAMPLES,
                            (LPARAM)szNumSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)szNumSep);
    }
}

/* Init number of fractional symbols control box */
static VOID
InitNumOfFracSymbCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szFracNum[MAX_SAMPLES_STR_SIZE];
    WCHAR szFracCount[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;

    /* Get current number of fractional symbols */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_IDIGITS,
                   szFracNum,
                   MAX_SAMPLES_STR_SIZE);

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
                        (WPARAM)_wtoi(szFracNum),
                        (LPARAM)0);
}

/* Init field separator control box */
static VOID
InitNumFieldSepCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szFieldSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                        CB_LIMITTEXT,
                        MAX_NUMBERSDIGITGRSYM,
                        0);

    /* Get current field separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_STHOUSAND,
                   szFieldSep,
                   MAX_SAMPLES_STR_SIZE);

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
                                   (LPARAM)szFieldSep);

    /* If it is not success, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szFieldSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)szFieldSep);
    }
}

/* Init number of digits in field control box */
static VOID
InitFieldDigNumCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szFieldDigNum[MAX_SAMPLES_STR_SIZE];
    PWSTR pszFieldDigNumSmpl;
    INT nCBIndex;
    INT nRetCode;

    /* Get current field digits num */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SGROUPING,
                   szFieldDigNum,
                   MAX_SAMPLES_STR_SIZE);

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

    pszFieldDigNumSmpl = InsSpacesFmt(SAMPLE_NUMBER, szFieldDigNum);
    /* Set current item to value from registry */
    nRetCode = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                                   CB_SELECTSTRING,
                                   -1,
                                   (LPARAM)pszFieldDigNumSmpl);

    /* If it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)pszFieldDigNumSmpl);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)pszFieldDigNumSmpl);
    }

    free(pszFieldDigNumSmpl);
}

/* Init negative sign control box */
static VOID
InitNegSignCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szNegSign[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                        CB_LIMITTEXT,
                        MAX_NUMBERSNSIGNSYM,
                        0);

    /* Get current negative sign */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SNEGATIVESIGN,
                   szNegSign,
                   MAX_SAMPLES_STR_SIZE);

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
                                   (LPARAM)szNegSign);

    /* If  it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szNegSign);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)szNegSign);
    }
}

/* Init negative numbers format control box */
static VOID
InitNegNumFmtCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szNegNumFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    WCHAR szNegSign[MAX_SAMPLES_STR_SIZE];
    WCHAR szNewSample[MAX_SAMPLES_STR_SIZE];
    PWSTR pszResultStr;
    INT nCBIndex;

    /* Get current negative numbers format */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_INEGNUMBER,
                   szNegNumFmt,
                   MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SDECIMAL,
                   szNumSep,
                   MAX_SAMPLES_STR_SIZE);

    /* Get current negative sign */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SNEGATIVESIGN,
                   szNegSign,
                   MAX_SAMPLES_STR_SIZE);

    /* Create standard list of negative numbers formats */
    for (nCBIndex = 0; nCBIndex < MAX_NEG_NUMBERS_SAMPLES; nCBIndex++)
    {
        /* Replace standard separator to setted */
        pszResultStr = ReplaceSubStr(lpNegNumFmtSamples[nCBIndex],
                                     szNumSep,
                                     L",");
        wcscpy(szNewSample, pszResultStr);
        free(pszResultStr);

        /* Replace standard negative sign to setted */
        pszResultStr = ReplaceSubStr(szNewSample,
                                     szNegSign,
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
                        (WPARAM)_wtoi(szNegNumFmt),
                        (LPARAM)0);
}

/* Init leading zeroes control box */
static VOID
InitLeadingZeroesCB(HWND hwndDlg, PGLOBALDATA pGlobalData)
{
    WCHAR szLeadNumFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    PWSTR pszResultStr;
    INT nCBIndex;

    /* Get current leading zeroes format */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_ILZERO,
                   szLeadNumFmt,
                   MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDISPLEADZER,
                        CB_RESETCONTENT,
                        (WPARAM)0,
                        (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SDECIMAL,
                   szNumSep,
                   MAX_SAMPLES_STR_SIZE);

    /* Create list of standard leading zeroes formats */
    for (nCBIndex = 0; nCBIndex < MAX_LEAD_ZEROES_SAMPLES; nCBIndex++)
    {
        pszResultStr = ReplaceSubStr(lpLeadNumFmtSamples[nCBIndex],
                                     szNumSep,
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
                        (WPARAM)_wtoi(szLeadNumFmt),
                        (LPARAM)0);
}

static VOID
InitListSepCB(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    WCHAR szListSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Limit text length */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                        CB_LIMITTEXT,
                        MAX_NUMBERSLSEP,
                        0);

    /* Get current list separator */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_SLIST,
                   szListSep,
                   MAX_SAMPLES_STR_SIZE);

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
                                   (LPARAM)szListSep);

    /* If it is not successful, add new values to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szListSep);
        SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                            CB_SELECTSTRING,
                            -1,
                            (LPARAM)szListSep);
    }
}

/* Init system of units control box */
static VOID
InitUnitsSysCB(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szUnitsSys[MAX_SAMPLES_STR_SIZE];
    WCHAR szUnitName[128];
    INT nCBIndex;

    /* Get current system of units */
    GetLocaleInfoW(pGlobalData->lcid,
                   LOCALE_IMEASURE,
                   szUnitsSys,
                   MAX_SAMPLES_STR_SIZE);

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
                        (WPARAM)_wtoi(szUnitsSys),
                        (LPARAM)0);
}

/* Update all numbers locale samples */
static VOID
UpdateNumSamples(HWND hwndDlg,
                 PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get positive number format sample */
    GetNumberFormatW(pGlobalData->lcid,
                     0,
                     SAMPLE_NUMBER,
                     NULL,
                     OutBuffer,
                     MAX_FMT_SIZE);

    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSPOSSAMPLE,
                        WM_SETTEXT,
                        0,
                        (LPARAM)OutBuffer);

    /* Get positive number format sample */
    GetNumberFormatW(pGlobalData->lcid,
                     0,
                     SAMPLE_NEG_NUMBER,
                     NULL,
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
    WCHAR szDecimalSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted decimal separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERDSYMBOL,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szDecimalSep);

    /* Save decimal separator */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SDECIMAL, szDecimalSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetFracSymNum(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    WCHAR szFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNDIGDEC,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Convert to wide char */
    _itow(nCurrSel, szFracSymNum, DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_IDIGITS, szFracSymNum);

    return TRUE;
}

/* Set field separator */
static BOOL
SetNumFieldSep(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted field separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDIGITGRSYM,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szFieldSep);

    /* Save field separator */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_STHOUSAND, szFieldSep);

    return TRUE;
}

/* Set number of digits in field  */
static BOOL
SetFieldDigNum(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szFieldDigNum[MAX_SAMPLES_STR_SIZE];

    /* Get setted number of digidts in field */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDGROUPING,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szFieldDigNum);

    /* Save number of digits in field */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SGROUPING, szFieldDigNum);

    return TRUE;
}

/* Set negative sign */
static BOOL
SetNumNegSign(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    WCHAR szNegSign[MAX_SAMPLES_STR_SIZE];

    /* Get setted negative sign */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNSIGNSYM,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szNegSign);

    /* Save negative sign */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SNEGATIVESIGN, szNegSign);

    return TRUE;
}

/* Set negative sum format */
static BOOL
SetNegSumFmt(HWND hwndDlg,
             PGLOBALDATA pGlobalData)
{
    WCHAR szNegSumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted negative sum format */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSNNUMFORMAT,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel, szNegSumFmt,DECIMAL_RADIX);

    /* Save negative sum format */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_INEGNUMBER, szNegSumFmt);

    return TRUE;
}

/* Set leading zero */
static BOOL
SetNumLeadZero(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szLeadZero[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted leading zero format */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSDISPLEADZER,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* Convert to wide char */
    _itow(nCurrSel, szLeadZero, DECIMAL_RADIX);

    /* Save leading zero format */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_ILZERO, szLeadZero);

    return TRUE;
}

/* Set elements list separator */
static BOOL
SetNumListSep(HWND hwndDlg,
              PGLOBALDATA pGlobalData)
{
    WCHAR szListSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted list separator */
    SendDlgItemMessageW(hwndDlg, IDC_NUMBERSLSEP,
                        WM_GETTEXT,
                        (WPARAM)MAX_SAMPLES_STR_SIZE,
                        (LPARAM)szListSep);

    /* Save list separator */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_SLIST, szListSep);

    return TRUE;
}

/* Set units system */
static BOOL
SetNumUnitsSys(HWND hwndDlg,
               PGLOBALDATA pGlobalData)
{
    WCHAR szUnitsSys[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted units system */
    nCurrSel = SendDlgItemMessageW(hwndDlg, IDC_NUMBERSMEASSYS,
                                   CB_GETCURSEL,
                                   (WPARAM)0,
                                   (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel, szUnitsSys, DECIMAL_RADIX);

    /* Save units system */
    SetLocaleInfoW(pGlobalData->lcid, LOCALE_IMEASURE, szUnitsSys);

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

                UpdateNumSamples(hwndDlg, pGlobalData);
            }
            break;
    }
    return FALSE;
}

/* EOF */
