/*
 *  ReactOS
 *  Copyright (C) 2004, 2005 ReactOS Team
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
 * FILE:            dll/cpl/intl/generalp.c
 * PURPOSE:         General property page
 * PROGRAMMER:      Eric Kohl
 *                  Klemens Friedl
 *                  Aleksey Bragin
 */

#include "intl.h"

#include <debug.h>

#define SAMPLE_NUMBER   L"123456789"

#define NUM_SHEETS      4

#define MAX_FIELD_DIG_SAMPLES       3


HWND hList;
HWND hLocaleList, hGeoList;
BOOL isSpain = FALSE;

GROUPINGDATA
GroupingFormats[MAX_GROUPINGFORMATS] =
{
    {0, L"0;0"},
    {3, L"3;0"},
    {32, L"3;2;0"}
};

static BOOL CALLBACK
GeneralPropertyPageLocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    WCHAR lang[255];
    INT index;
    BOOL bNoShow = FALSE;

    lcid = wcstoul(lpLocale, NULL, 16);

    /* Display only languages with installed support */
    if (!IsValidLocale(lcid, LCID_INSTALLED))
        return TRUE;

    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        if (isSpain == FALSE)
        {
            LoadStringW(hApplet, IDS_SPAIN, lang, 255);
            isSpain = TRUE;
        }
        else
        {
            bNoShow = TRUE;
        }
    }
    else
    {
        GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang)/sizeof(WCHAR));
    }

    if (bNoShow == FALSE)
    {
    index = SendMessageW(hList,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)lang);

    SendMessageW(hList,
                 CB_SETITEMDATA,
                 index,
                 (LPARAM)lcid);
    }

    return TRUE;
}


/* Update all locale samples */
static
VOID
UpdateLocaleSample(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[MAX_SAMPLES_STR_SIZE];
    NUMBERFMT NumberFormat;
    CURRENCYFMTW CurrencyFormat;

    NumberFormat.NumDigits = pGlobalData->nNumDigits;
    NumberFormat.LeadingZero = pGlobalData->nNumLeadingZero;
    NumberFormat.Grouping = GroupingFormats[pGlobalData->nNumGrouping].nInteger;
    NumberFormat.lpDecimalSep = pGlobalData->szNumDecimalSep;
    NumberFormat.lpThousandSep = pGlobalData->szNumThousandSep;
    NumberFormat.NegativeOrder = pGlobalData->nNumNegFormat;

    CurrencyFormat.NumDigits = pGlobalData->nCurrDigits;
    CurrencyFormat.LeadingZero = pGlobalData->nNumLeadingZero;
    CurrencyFormat.Grouping = GroupingFormats[pGlobalData->nCurrGrouping].nInteger;
    CurrencyFormat.lpDecimalSep = pGlobalData->szCurrDecimalSep;
    CurrencyFormat.lpThousandSep = pGlobalData->szCurrThousandSep;
    CurrencyFormat.NegativeOrder = pGlobalData->nCurrNegFormat;
    CurrencyFormat.PositiveOrder = pGlobalData->nCurrPosFormat;
    CurrencyFormat.lpCurrencySymbol = pGlobalData->szCurrSymbol;

    /* Get number format sample */
    GetNumberFormatW(pGlobalData->UserLCID, 0, SAMPLE_NUMBER,
                     &NumberFormat,
                     OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_NUMSAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get monetary format sample */
    GetCurrencyFormatW(pGlobalData->UserLCID, 0, SAMPLE_NUMBER,
                       &CurrencyFormat,
                       OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_MONEYSAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get time format sample */
    GetTimeFormatW(pGlobalData->UserLCID, 0, NULL,
                   pGlobalData->szTimeFormat,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_TIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get short date format sample */
    GetDateFormatW(pGlobalData->UserLCID, 0, NULL,
                   pGlobalData->szShortDateFormat,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_SHORTTIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormatW(pGlobalData->UserLCID, 0, NULL,
                   pGlobalData->szLongDateFormat,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_FULLTIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
}

static VOID
CreateLanguagesList(HWND hwnd)
{
    WCHAR langSel[255];

    hList = hwnd;
    isSpain = FALSE;
    EnumSystemLocalesW(GeneralPropertyPageLocalesEnumProc, LCID_SUPPORTED);

    /* Select current locale */
    /* or should it be System and not user? */
    GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, sizeof(langSel)/sizeof(WCHAR));

    SendMessageW(hList,
                 CB_SELECTSTRING,
                 -1,
                 (LPARAM)langSel);
}


BOOL
LoadCurrentLocale(
    PGLOBALDATA pGlobalData)
{
    WCHAR szBuffer[16];
    PWSTR ptr;
    HKEY hLocaleKey;
    DWORD ret;
    DWORD dwSize;

    ret = RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"Control Panel\\International",
                        0,
                        KEY_READ,
                        &hLocaleKey);
    if (ret != ERROR_SUCCESS)
    {
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return FALSE;
    }

    dwSize = 9 * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"Locale",
                     NULL,
                     NULL,
                     (PBYTE)szBuffer,
                     &dwSize);
    pGlobalData->UserLCID = (LCID)wcstoul(szBuffer, &ptr, 16);

    /* Number */
    dwSize = MAX_NUMDECIMALSEP * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sDecimal",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumDecimalSep,
                     &dwSize);

    dwSize = MAX_NUMTHOUSANDSEP * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sThousand",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumThousandSep,
                     &dwSize);

    dwSize = MAX_NUMNEGATIVESIGN * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sNegativeSign",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumNegativeSign,
                     &dwSize);

    dwSize = MAX_NUMPOSITIVESIGN * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sPositiveSign",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumPositiveSign,
                     &dwSize);

    dwSize = MAX_NUMLISTSEP * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sList",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumListSep,
                     &dwSize);

    dwSize = MAX_NUMNATIVEDIGITS * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sNativeDigits",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szNumNativeDigits,
                     &dwSize);

    pGlobalData->nNumNegFormat = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iNegNumber",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nNumNegFormat = _wtoi(szBuffer);

    pGlobalData->nNumDigits = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iDigits",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nNumDigits = _wtoi(szBuffer);

    pGlobalData->nNumLeadingZero = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iLZero",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nNumLeadingZero = _wtoi(szBuffer);

    pGlobalData->nNumMeasure = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iMeasure",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nNumMeasure = _wtoi(szBuffer);

    pGlobalData->nNumShape = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"NumShape",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nNumShape = _wtoi(szBuffer);

    pGlobalData->nNumGrouping = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"sGrouping",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
    {
        pGlobalData->nNumGrouping = 0;
        if (szBuffer[0] == L'3')
        {
            if ((szBuffer[1] == L';') &&
                (szBuffer[2] == L'2'))
                pGlobalData->nNumGrouping = 2;
            else
                pGlobalData->nNumGrouping = 1;
        }
    }

    /* Currency */
    dwSize = MAX_CURRSYMBOL * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sCurrency",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szCurrSymbol,
                     &dwSize);

    dwSize = MAX_CURRDECIMALSEP * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sMonDecimalSep",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szCurrDecimalSep,
                     &dwSize);

    dwSize = MAX_CURRTHOUSANDSEP * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sMonThousandSep",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szCurrThousandSep,
                     &dwSize);

    pGlobalData->nCurrGrouping = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"sMonGrouping",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
    {
        pGlobalData->nCurrGrouping = 0;
        if (szBuffer[0] == L'3')
        {
            if ((szBuffer[1] == L';') &&
                (szBuffer[2] == L'2'))
                pGlobalData->nCurrGrouping = 2;
            else
                pGlobalData->nCurrGrouping = 1;
        }
    }

    pGlobalData->nCurrPosFormat = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iCurrency",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nCurrPosFormat = _wtoi(szBuffer);

    pGlobalData->nCurrNegFormat = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iNegCurr",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nCurrNegFormat = _wtoi(szBuffer);

    pGlobalData->nCurrDigits = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iCurrDigits",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nCurrDigits = _wtoi(szBuffer);

    /* Time */
    dwSize = MAX_TIMEFORMAT * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sTimeFormat",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szTimeFormat,
                     &dwSize);

    dwSize = MAX_TIMESEPARATOR * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sTime",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szTimeSep,
                     &dwSize);

    dwSize = MAX_TIMEAMSYMBOL * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"s1159",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szTimeAM,
                     &dwSize);

    dwSize = MAX_TIMEPMSYMBOL * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"s2359",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szTimePM,
                     &dwSize);

    pGlobalData->nTime = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iTime",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nTime = _wtoi(szBuffer);

    pGlobalData->nTimePrefix = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iTimePrefix",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nTimePrefix = _wtoi(szBuffer);

    pGlobalData->nTimeLeadingZero = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iTLZero",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nTimeLeadingZero = _wtoi(szBuffer);

    /* Date */
    dwSize = MAX_LONGDATEFORMAT * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sLongDate",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szLongDateFormat,
                     &dwSize);

    dwSize = MAX_SHORTDATEFORMAT * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sShortDate",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szShortDateFormat,
                     &dwSize);

    dwSize = MAX_DATESEPARATOR * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sDate",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szDateSep,
                     &dwSize);

    pGlobalData->nFirstDayOfWeek = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iFirstDayOfWeek",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nFirstDayOfWeek = _wtoi(szBuffer);

    pGlobalData->nFirstWeekOfYear = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iFirstWeekOfYear",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nFirstWeekOfYear = _wtoi(szBuffer);

    pGlobalData->nDate = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iDate",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nDate = _wtoi(szBuffer);

    pGlobalData->nCalendarType = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iCalendarType",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nCalendarType = _wtoi(szBuffer);

    /* Misc */
    dwSize = MAX_MISCCOUNTRY * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sCountry",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szMiscCountry,
                     &dwSize);

    dwSize = MAX_MISCLANGUAGE * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"sLanguage",
                     NULL,
                     NULL,
                     (PBYTE)pGlobalData->szMiscLanguage,
                     &dwSize);

    pGlobalData->nMiscCountry = 0;
    dwSize = 16 * sizeof(WCHAR);
    if (RegQueryValueExW(hLocaleKey,
                         L"iCountry",
                         NULL,
                         NULL,
                         (PBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS)
        pGlobalData->nMiscCountry = _wtoi(szBuffer);

    RegCloseKey(hLocaleKey);

    return TRUE;
}


VOID
SetNewLocale(
    PGLOBALDATA pGlobalData,
    LCID lcid)
{
    WCHAR szBuffer[16];

    pGlobalData->UserLCID = lcid;

    /* Number */
    GetLocaleInfo(lcid,
                  LOCALE_SDECIMAL | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumDecimalSep,
                  MAX_NUMDECIMALSEP);

    GetLocaleInfo(lcid,
                  LOCALE_STHOUSAND | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumThousandSep,
                  MAX_NUMTHOUSANDSEP);

    GetLocaleInfo(lcid,
                  LOCALE_SNEGATIVESIGN | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumNegativeSign,
                  MAX_NUMNEGATIVESIGN);

    GetLocaleInfo(lcid,
                  LOCALE_SPOSITIVESIGN | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumPositiveSign,
                  MAX_NUMPOSITIVESIGN);

    GetLocaleInfo(lcid,
                  LOCALE_SLIST | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumListSep,
                  MAX_NUMLISTSEP);

    GetLocaleInfo(lcid,
                  LOCALE_SNATIVEDIGITS | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szNumNativeDigits,
                  MAX_NUMNATIVEDIGITS);

    GetLocaleInfo(lcid,
                  LOCALE_INEGNUMBER | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumNegFormat = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_IDIGITS | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumDigits = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_ILZERO | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumLeadingZero = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_IMEASURE | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumMeasure = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_IDIGITSUBSTITUTION | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumShape = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_SGROUPING | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nNumGrouping = 0;
    if (szBuffer[0] == L'3')
    {
        if ((szBuffer[1] == L';') &&
            (szBuffer[2] == L'2'))
            pGlobalData->nNumGrouping = 2;
        else
            pGlobalData->nNumGrouping = 1;
    }

    /* Currency */
    GetLocaleInfo(lcid,
                  LOCALE_SCURRENCY | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szCurrSymbol,
                  MAX_CURRSYMBOL);

    GetLocaleInfo(lcid,
                  LOCALE_SMONDECIMALSEP | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szCurrDecimalSep,
                  MAX_CURRDECIMALSEP);

    GetLocaleInfo(lcid,
                  LOCALE_SMONTHOUSANDSEP | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szCurrThousandSep,
                  MAX_CURRTHOUSANDSEP);

    GetLocaleInfo(lcid,
                  LOCALE_SMONGROUPING | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nCurrGrouping = 0;
    if (szBuffer[0] == L'3')
    {
        if ((szBuffer[1] == L';') &&
            (szBuffer[2] == L'2'))
            pGlobalData->nCurrGrouping = 2;
        else
            pGlobalData->nCurrGrouping = 1;
    }

    GetLocaleInfo(lcid,
                  LOCALE_ICURRENCY | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nCurrPosFormat = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_INEGCURR | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nCurrNegFormat = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_ICURRDIGITS | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nCurrDigits = _wtoi(szBuffer);

    /* Time */
    GetLocaleInfo(lcid,
                  LOCALE_STIMEFORMAT | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szTimeFormat,
                  MAX_TIMEFORMAT);

    GetLocaleInfo(lcid,
                  LOCALE_STIME | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szTimeSep,
                  MAX_TIMESEPARATOR);

    GetLocaleInfo(lcid,
                  LOCALE_S1159 | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szTimeAM,
                  MAX_TIMEAMSYMBOL);

    GetLocaleInfo(lcid,
                  LOCALE_S2359 | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szTimePM,
                  MAX_TIMEPMSYMBOL);

    GetLocaleInfo(lcid,
                  LOCALE_ITIME | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nTime = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_ITIMEMARKPOSN | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nTimePrefix = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_ITLZERO | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nTimeLeadingZero = _wtoi(szBuffer);

    /* Date */
    GetLocaleInfo(lcid,
                  LOCALE_SLONGDATE | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szLongDateFormat,
                  MAX_LONGDATEFORMAT);

    GetLocaleInfo(lcid,
                  LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szShortDateFormat,
                  MAX_SHORTDATEFORMAT);

    GetLocaleInfo(lcid,
                  LOCALE_SDATE | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szDateSep,
                  MAX_DATESEPARATOR);

    GetLocaleInfo(lcid,
                  LOCALE_IFIRSTDAYOFWEEK | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nFirstDayOfWeek = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_IFIRSTWEEKOFYEAR | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nFirstWeekOfYear = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_IDATE | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nDate = _wtoi(szBuffer);

    GetLocaleInfo(lcid,
                  LOCALE_ICALENDARTYPE | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nCalendarType = _wtoi(szBuffer);

    /* Misc */
    GetLocaleInfo(lcid,
                  LOCALE_SCOUNTRY | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szMiscCountry,
                  MAX_MISCCOUNTRY);

    GetLocaleInfo(lcid,
                  LOCALE_SLANGUAGE | LOCALE_NOUSEROVERRIDE,
                  pGlobalData->szMiscLanguage,
                  MAX_MISCLANGUAGE);

    GetLocaleInfo(lcid,
                  LOCALE_ICOUNTRY | LOCALE_NOUSEROVERRIDE,
                  szBuffer,
                  sizeof(szBuffer) / sizeof(WCHAR));
    pGlobalData->nMiscCountry = _wtoi(szBuffer);
}


static
VOID
SaveUserLocale(
    PGLOBALDATA pGlobalData,
    HKEY hLocaleKey)
{
    WCHAR szBuffer[16];

    wsprintf(szBuffer, L"%08lx", (DWORD)pGlobalData->UserLCID);
    RegSetValueExW(hLocaleKey,
                   L"Locale",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    /* Number */
    RegSetValueExW(hLocaleKey,
                   L"sDecimal",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumDecimalSep,
                   (wcslen(pGlobalData->szNumDecimalSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sThousand",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumThousandSep,
                   (wcslen(pGlobalData->szNumThousandSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sNegativeSign",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumNegativeSign,
                   (wcslen(pGlobalData->szNumNegativeSign) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sPositiveSign",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumPositiveSign,
                   (wcslen(pGlobalData->szNumPositiveSign) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sGrouping",
                   0,
                   REG_SZ,
                   (PBYTE)GroupingFormats[pGlobalData->nNumGrouping].pszString,
                   (wcslen(GroupingFormats[pGlobalData->nNumGrouping].pszString) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sList",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumListSep,
                   (wcslen(pGlobalData->szNumListSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sNativeDigits",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szNumNativeDigits,
                   (wcslen(pGlobalData->szNumNativeDigits) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nNumNegFormat,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iNegNumber",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nNumDigits,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iDigits",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nNumLeadingZero,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iLZero",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nNumMeasure,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iMeasure",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nNumShape,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"NumShape",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    /* Currency */
    RegSetValueExW(hLocaleKey,
                   L"sCurrency",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szCurrSymbol,
                   (wcslen(pGlobalData->szCurrSymbol) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sMonDecimalSep",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szCurrDecimalSep,
                   (wcslen(pGlobalData->szCurrDecimalSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sMonThousandSep",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szCurrThousandSep,
                   (wcslen(pGlobalData->szCurrThousandSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sMonGrouping",
                   0,
                   REG_SZ,
                   (PBYTE)GroupingFormats[pGlobalData->nCurrGrouping].pszString,
                   (wcslen(GroupingFormats[pGlobalData->nCurrGrouping].pszString) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nCurrPosFormat,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iCurrency",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nCurrNegFormat,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iNegCurr",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nCurrDigits,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iCurrDigits",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    /* Time */
    RegSetValueExW(hLocaleKey,
                   L"sTimeFormat",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szTimeFormat,
                   (wcslen(pGlobalData->szTimeFormat) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sTime",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szTimeSep,
                   (wcslen(pGlobalData->szTimeSep) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"s1159",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szTimeAM,
                   (wcslen(pGlobalData->szTimeAM) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"s2359",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szTimePM,
                   (wcslen(pGlobalData->szTimePM) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nTime,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iTime",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nTimePrefix,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iTimePrefix",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nTimeLeadingZero,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iTLZero",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    /* Date */
    RegSetValueExW(hLocaleKey,
                   L"sLongDate",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szLongDateFormat,
                   (wcslen(pGlobalData->szLongDateFormat) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sShortDate",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szShortDateFormat,
                   (wcslen(pGlobalData->szShortDateFormat) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sDate",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szDateSep,
                   (wcslen(pGlobalData->szDateSep) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nFirstDayOfWeek,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iFirstDayOfWeek",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nFirstWeekOfYear,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iFirstWeekOfYear",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nDate,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iDate",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nCalendarType,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iCalendarType",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    /* Misc */
    RegSetValueExW(hLocaleKey,
                   L"sCountry",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szMiscCountry,
                   (wcslen(pGlobalData->szMiscCountry) + 1) * sizeof(WCHAR));

    RegSetValueExW(hLocaleKey,
                   L"sLanguage",
                   0,
                   REG_SZ,
                   (PBYTE)pGlobalData->szMiscLanguage,
                   (wcslen(pGlobalData->szMiscLanguage) + 1) * sizeof(WCHAR));

    _itow(pGlobalData->nMiscCountry,
          szBuffer, DECIMAL_RADIX);
    RegSetValueExW(hLocaleKey,
                   L"iCountry",
                   0,
                   REG_SZ,
                   (PBYTE)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));
}


/* Sets new locale */
VOID
SaveCurrentLocale(
    PGLOBALDATA pGlobalData)
{
    HKEY hLocaleKey;
    DWORD ret;

    if (pGlobalData->bApplyToDefaultUser)
    {
        ret = RegOpenKeyExW(HKEY_USERS,
                            L".DEFAULT\\Control Panel\\International",
                            0,
                            KEY_WRITE,
                            &hLocaleKey);
        if (ret != ERROR_SUCCESS)
        {
            PrintErrorMsgBox(IDS_ERROR_DEF_INT_KEY_REG);
            return;
        }

        SaveUserLocale(pGlobalData, hLocaleKey);

        /* Flush and close the locale key */
        RegFlushKey(hLocaleKey);
        RegCloseKey(hLocaleKey);
    }

    ret = RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"Control Panel\\International",
                        0,
                        KEY_WRITE,
                        &hLocaleKey);
    if (ret != ERROR_SUCCESS)
    {
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return;
    }

    SaveUserLocale(pGlobalData, hLocaleKey);

    /* Flush and close the locale key */
    RegFlushKey(hLocaleKey);
    RegCloseKey(hLocaleKey);

    /* Set the new locale for the current process */
    NtSetDefaultLocale(TRUE, pGlobalData->UserLCID);
}

/* Location enumerate procedure */
BOOL
CALLBACK
LocationsEnumProc(GEOID gId)
{
    WCHAR loc[MAX_STR_SIZE];
    INT index;

    if (GetGeoInfoW(gId, GEO_FRIENDLYNAME, loc, MAX_STR_SIZE, GetThreadLocale()) == 0)
        return TRUE;

    index = (INT)SendMessageW(hGeoList,
                              CB_ADDSTRING,
                              0,
                              (LPARAM)loc);

    SendMessageW(hGeoList,
                 CB_SETITEMDATA,
                 index,
                 (LPARAM)gId);

    return TRUE;
}

/* Enumerate all system locations identifiers */
static
GEOID
CreateLocationsList(HWND hWnd)
{
    GEOID userGeoID;
    WCHAR loc[MAX_STR_SIZE];

    hGeoList = hWnd;

    EnumSystemGeoID(GEOCLASS_NATION, 0, LocationsEnumProc);

    /* Select current location */
    userGeoID = GetUserGeoID(GEOCLASS_NATION);
    GetGeoInfoW(userGeoID,
                GEO_FRIENDLYNAME,
                loc,
                MAX_STR_SIZE,
                GetThreadLocale());

    SendMessageW(hGeoList,
                 CB_SELECTSTRING,
                 (WPARAM) -1,
                 (LPARAM)loc);

    return userGeoID;
}

VOID
SaveGeoID(
    PGLOBALDATA pGlobalData)
{
    HKEY hGeoKey;
    WCHAR value[15];
    DWORD valuesize;
    DWORD ret;

    wsprintf(value, L"%lu", (DWORD)pGlobalData->geoid);
    valuesize = (wcslen(value) + 1) * sizeof(WCHAR);

    if (pGlobalData->bApplyToDefaultUser)
    {
        ret = RegOpenKeyExW(HKEY_USERS,
                            L".DEFAULT\\Control Panel\\International\\Geo",
                            0,
                            KEY_WRITE,
                            &hGeoKey);
        if (ret != ERROR_SUCCESS)
        {
            PrintErrorMsgBox(IDS_ERROR_DEF_INT_KEY_REG);
            return;
        }

        ret = RegSetValueExW(hGeoKey,
                             L"Nation",
                             0,
                             REG_SZ,
                             (PBYTE)value,
                             valuesize);

        RegFlushKey(hGeoKey);
        RegCloseKey(hGeoKey);

        if (ret != ERROR_SUCCESS)
        {
            PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
            return;
        }
    }

    ret = RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"Control Panel\\International\\Geo",
                        0,
                        KEY_WRITE,
                        &hGeoKey);
    if (ret != ERROR_SUCCESS)
    {
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return;
    }

    ret = RegSetValueExW(hGeoKey,
                         L"Nation",
                         0,
                         REG_SZ,
                         (PBYTE)value,
                         valuesize);

    RegFlushKey(hGeoKey);
    RegCloseKey(hGeoKey);

    if (ret != ERROR_SUCCESS)
    {
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return;
    }
}

DWORD
VerifyUnattendLCID(HWND hwndDlg)
{
    LRESULT lCount, lIndex, lResult;

    lCount = SendMessage(hList, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if (lCount == CB_ERR)
    {
        return 0;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        lResult = SendMessage(hList, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
        if (lResult == CB_ERR)
        {
            continue;
        }

        if (lResult == (LRESULT)UnattendLCID)
        {
            SendMessage(hList, CB_SETCURSEL, (WPARAM)lIndex, (LPARAM)0);
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            return 1;
        }
    }

    return 0;
}


static
VOID
InitPropSheetPage(
    PROPSHEETPAGEW *psp,
    WORD idDlg,
    DLGPROC DlgProc,
    PGLOBALDATA pGlobalData)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGEW));
    psp->dwSize = sizeof(PROPSHEETPAGEW);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
    psp->lParam = (LPARAM)pGlobalData;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

INT_PTR
APIENTRY
CustomizeLocalePropertySheet(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    PROPSHEETPAGEW PsPage[NUM_SHEETS + 1];
    PROPSHEETHEADERW psh;
    WCHAR Caption[MAX_STR_SIZE];

    LoadStringW(hApplet, IDS_CUSTOMIZE_TITLE, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCE(IDC_CPLICON);
    psh.pszCaption = Caption;
    psh.nPages = (sizeof(PsPage) / sizeof(PROPSHEETPAGE)) - 1;
    psh.nStartPage = 0;
    psh.ppsp = PsPage;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&PsPage[0], IDD_NUMBERSPAGE, NumbersPageProc, pGlobalData);
    InitPropSheetPage(&PsPage[1], IDD_CURRENCYPAGE, CurrencyPageProc, pGlobalData);
    InitPropSheetPage(&PsPage[2], IDD_TIMEPAGE, TimePageProc, pGlobalData);
    InitPropSheetPage(&PsPage[3], IDD_DATEPAGE, DatePageProc, pGlobalData);

    if (IsSortPageNeeded(pGlobalData->UserLCID))
    {
        psh.nPages++;
        InitPropSheetPage(&PsPage[4], IDD_SORTPAGE, SortPageProc, pGlobalData);
    }

    return PropertySheetW(&psh);
}


/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
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

            if (pGlobalData)
            {
                LoadCurrentLocale(pGlobalData);

                CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));
                UpdateLocaleSample(hwndDlg, pGlobalData);
                pGlobalData->geoid = CreateLocationsList(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO));
                if (IsUnattendedSetupEnabled)
                {
                    if (VerifyUnattendLCID(hwndDlg))
                    {
                        SetNewLocale(pGlobalData, UnattendLCID);
                        SaveCurrentLocale(pGlobalData);
                        PostQuitMessage(0);
                    }
                    else
                    {
                        DPRINT1("VerifyUnattendLCID failed\n");
                    }
                    return TRUE;
                }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_LANGUAGELIST:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LCID NewLcid;
                        INT iCurSel;

                        iCurSel = SendMessage(hList,
                                              CB_GETCURSEL,
                                              0,
                                              0);
                        if (iCurSel == CB_ERR)
                            break;

                        NewLcid = SendMessage(hList,
                                              CB_GETITEMDATA,
                                              iCurSel,
                                              0);
                        if (NewLcid == (LCID)CB_ERR)
                            break;

                        SetNewLocale(pGlobalData, NewLcid);
                        UpdateLocaleSample(hwndDlg, pGlobalData);
                        pGlobalData->bUserLocaleChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_LOCATION_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        GEOID NewGeoID;
                        INT iCurSel;

                        iCurSel = SendMessage(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO),
                                              CB_GETCURSEL,
                                              0,
                                              0);
                        if (iCurSel == CB_ERR)
                            break;

                        NewGeoID = SendMessage(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO),
                                               CB_GETITEMDATA,
                                               iCurSel,
                                               0);
                        if (NewGeoID == (GEOID)CB_ERR)
                            break;

                        pGlobalData->geoid = NewGeoID;
                        pGlobalData->bGeoIdChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_SETUP_BUTTON:
                    if (CustomizeLocalePropertySheet(GetParent(hwndDlg), pGlobalData) > 0)
                    {
                        UpdateLocaleSample(hwndDlg, pGlobalData);
                        pGlobalData->bUserLocaleChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                /* Apply changes */
                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

                /* Set new locale */
                if (pGlobalData->bUserLocaleChanged == TRUE)
                {
                    SaveCurrentLocale(pGlobalData);
                    pGlobalData->bUserLocaleChanged = FALSE;
                }

                /* Set new GEO ID */
                if (pGlobalData->bGeoIdChanged == TRUE)
                {
                    SaveGeoID(pGlobalData);
                    pGlobalData->bGeoIdChanged = FALSE;
                }

                AddNewKbLayoutsByLcid(pGlobalData->UserLCID);

                /* Post WM_WININICHANGE messages to system */
                PostMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)L"intl");
            }
            break;
    }

    return FALSE;
}

/* EOF */
