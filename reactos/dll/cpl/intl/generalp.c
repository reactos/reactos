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

#define SAMPLE_NUMBER   _T("123456789")
#define NO_FLAG         0

typedef struct
{
    LCTYPE lcType;
    PWSTR  pKeyName;
} LOCALE_KEY_DATA, *PLOCALE_KEY_DATA;

HWND hList;
HWND hLocaleList, hGeoList;
BOOL bSpain = FALSE;

LOCALE_KEY_DATA LocaleKeyData[] =
{
    {LOCALE_ICALENDARTYPE, L"iCalendarType"},
    {LOCALE_ICOUNTRY, L"iCountry"},
    {LOCALE_ICURRDIGITS, L"iCurrDigits"},
    {LOCALE_ICURRENCY, L"iCurrency"},
    {LOCALE_IDATE, L"iDate"},
    {LOCALE_IDIGITS, L"iDigits"},
    {LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek"},
    {LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear"},
    {LOCALE_ILZERO, L"iLZero"},
    {LOCALE_IMEASURE, L"iMeasure"},
    {LOCALE_INEGCURR, L"iNegCurr"},
    {LOCALE_INEGNUMBER, L"iNegNumber"},
    {LOCALE_ITIME, L"iTime"},
    {LOCALE_ITIMEMARKPOSN, L"iTimePrefix"},
    {LOCALE_ITLZERO, L"iTLZero"},
    {LOCALE_IDIGITSUBSTITUTION, L"NumShape"},
    {LOCALE_S1159, L"s1159"},
    {LOCALE_S2359, L"s2359"},
    {LOCALE_SCOUNTRY, L"sCountry"},
    {LOCALE_SCURRENCY, L"sCurrency"},
    {LOCALE_SDATE, L"sDate"},
    {LOCALE_SDECIMAL, L"sDecimal"},
    {LOCALE_SGROUPING, L"sGrouping"},
    {LOCALE_SABBREVLANGNAME, L"sLanguage"},
    {LOCALE_SLIST, L"sList"},
    {LOCALE_SLONGDATE, L"sLongDate"},
    {LOCALE_SMONDECIMALSEP, L"sMonDecimalSep"},
    {LOCALE_SMONGROUPING, L"sMonGrouping"},
    {LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep"},
    {LOCALE_SNATIVEDIGITS, L"sNativeDigits"},
    {LOCALE_SNEGATIVESIGN, L"sNegativeSign"},
    {LOCALE_SPOSITIVESIGN, L"sPositiveSign"},
    {LOCALE_SSHORTDATE, L"sShortDate"},
    {LOCALE_STHOUSAND, L"sThousand"},
    {LOCALE_STIME, L"sTime"},
    {LOCALE_STIMEFORMAT, L"sTimeFormat"}
};


static BOOL CALLBACK
LocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    TCHAR lang[255];
    INT index;
    BOOL bNoShow = FALSE;

    lcid = _tcstoul(lpLocale, NULL, 16);

    /* Display only languages with installed support */
    if (!IsValidLocale(lcid, LCID_INSTALLED))
        return TRUE;

    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        if (bSpain == FALSE)
        {
            LoadString(hApplet, IDS_SPAIN, lang, 255);
            bSpain = TRUE;
        }
        else
        {
            bNoShow = TRUE;
        }
    }
    else
    {
        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang)/sizeof(TCHAR));
    }

    if (bNoShow == FALSE)
    {
    index = SendMessage(hList,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)lang);

    SendMessage(hList,
                CB_SETITEMDATA,
                index,
                (LPARAM)lcid);
    }

    return TRUE;
}


PWSTR
GetLocaleString(
    PWSTR *pLocaleArray,
    LCTYPE lcType)
{
    DWORD dwDataCount, i;

    dwDataCount = sizeof(LocaleKeyData) / sizeof(LOCALE_KEY_DATA);
    for (i = 0; i < dwDataCount; i++)
    {
        if (LocaleKeyData[i].lcType == lcType)
            return pLocaleArray[i];
    }

    return NULL;
}


/* Update all locale samples */
static
VOID
UpdateLocaleSample(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    WCHAR OutBuffer[MAX_SAMPLES_STR_SIZE];

    /* Get number format sample */
    GetNumberFormatW(pGlobalData->lcid, NO_FLAG, SAMPLE_NUMBER, NULL,
                     OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_NUMSAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get monetary format sample */
    GetCurrencyFormatW(pGlobalData->lcid, NO_FLAG, SAMPLE_NUMBER, NULL,
                       OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_MONEYSAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get time format sample */
    GetTimeFormatW(pGlobalData->lcid, NO_FLAG, NULL, NULL,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_TIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
    ZeroMemory(OutBuffer, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    /* Get short date format sample */
    GetDateFormatW(pGlobalData->lcid, DATE_SHORTDATE, NULL, NULL,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_SHORTTIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormatW(pGlobalData->lcid, DATE_LONGDATE, NULL, NULL,
                   OutBuffer, MAX_SAMPLES_STR_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_FULLTIMESAMPLE_EDIT,
                        WM_SETTEXT, 0, (LPARAM)OutBuffer);
}

static VOID
CreateLanguagesList(HWND hwnd)
{
    TCHAR langSel[255];

    hList = hwnd;
    bSpain = FALSE;
    EnumSystemLocales(LocalesEnumProc, LCID_SUPPORTED);

    /* Select current locale */
    /* or should it be System and not user? */
    GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, sizeof(langSel)/sizeof(TCHAR));

    SendMessage(hList,
                CB_SELECTSTRING,
                -1,
                (LPARAM)langSel);
}


BOOL
LoadCurrentLocale(
    PGLOBALDATA pGlobalData)
{
    WCHAR szValue[9];
    PWSTR ptr;
    HKEY hLocaleKey;
    DWORD ret;
    DWORD dwSize;
    DWORD i;

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

    pGlobalData->dwLocaleCount = sizeof(LocaleKeyData) / sizeof(LOCALE_KEY_DATA);

    pGlobalData->pLocaleArray = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                          pGlobalData->dwLocaleCount * sizeof(PWSTR));
    if (pGlobalData->pLocaleArray == NULL)
    {
        RegCloseKey(hLocaleKey);
        return FALSE;
    }

    dwSize = 9 * sizeof(WCHAR);
    RegQueryValueExW(hLocaleKey,
                     L"Locale",
                     NULL,
                     NULL,
                     (PBYTE)szValue,
                     &dwSize);
    pGlobalData->lcid = (LCID)wcstoul(szValue, &ptr, 16);

    for (i = 0; i < pGlobalData->dwLocaleCount; i++)
    {
        RegQueryValueExW(hLocaleKey,
                         LocaleKeyData[i].pKeyName,
                         NULL,
                         NULL,
                         NULL,
                         &dwSize);
        if (dwSize > 0)
        {
            pGlobalData->pLocaleArray[i] = HeapAlloc(GetProcessHeap(), 0, dwSize);
            if (pGlobalData->pLocaleArray[i])
            {
                RegQueryValueExW(hLocaleKey,
                                 LocaleKeyData[i].pKeyName,
                                 NULL,
                                 NULL,
                                 (LPVOID)pGlobalData->pLocaleArray[i],
                                 &dwSize);
            }
        }
    }

    RegCloseKey(hLocaleKey);

    return TRUE;
}


VOID
FreeCurrentLocale(
    PGLOBALDATA pGlobalData)
{
    DWORD i;

    if (pGlobalData == NULL || pGlobalData->pLocaleArray == NULL)
        return;

    for (i = 0; i < pGlobalData->dwLocaleCount; i++)
    {
        if (pGlobalData->pLocaleArray[i])
            HeapFree(GetProcessHeap(), 0, pGlobalData->pLocaleArray[i]);
    }
    HeapFree(GetProcessHeap(), 0, pGlobalData->pLocaleArray);
}


VOID
SetNewLocale(
    PGLOBALDATA pGlobalData,
    LCID lcid)
{
    DWORD i, dwSize;

    pGlobalData->lcid = lcid;

    for (i = 0; i < pGlobalData->dwLocaleCount; i++)
    {
        if (pGlobalData->pLocaleArray[i])
        {
            HeapFree(GetProcessHeap(), 0, pGlobalData->pLocaleArray[i]);
            pGlobalData->pLocaleArray[i] = NULL;
        }

        dwSize = GetLocaleInfo(lcid,
                               LocaleKeyData[i].lcType | LOCALE_NOUSEROVERRIDE,
                               NULL,
                               0);
        if (dwSize > 0)
        {
            pGlobalData->pLocaleArray[i] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
            if (pGlobalData->pLocaleArray[i])
            {
                GetLocaleInfo(lcid,
                              LocaleKeyData[i].lcType | LOCALE_NOUSEROVERRIDE,
                              pGlobalData->pLocaleArray[i],
                              dwSize);
            }
        }
    }
}


/* Sets new locale */
VOID
SaveCurrentLocale(
    PGLOBALDATA pGlobalData)
{
    // HKCU\\Control Panel\\International\\Locale = 0409 (type=0)
    // HKLM,"SYSTEM\CurrentControlSet\Control\NLS\Language","Default",0x00000000,"0409" (type=0)
    // HKLM,"SYSTEM\CurrentControlSet\Control\NLS\Language","InstallLanguage",0x00000000,"0409" (type=0)

    // Set locale
    HKEY localeKey;
    DWORD ret;
    WCHAR value[9];
    DWORD valuesize;
    DWORD i;

    ret = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\International",
                        0, KEY_READ | KEY_WRITE, &localeKey);
    if (ret != ERROR_SUCCESS)
    {
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return;
    }

    wsprintf(value, L"%08x", (DWORD)pGlobalData->lcid);
    valuesize = (wcslen(value) + 1) * sizeof(WCHAR);

    ret = RegSetValueExW(localeKey, L"Locale", 0, REG_SZ, (PBYTE)value, valuesize);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(localeKey);
        PrintErrorMsgBox(IDS_ERROR_INT_KEY_REG);
        return;
    }

    for (i = 0; i < pGlobalData->dwLocaleCount; i++)
    {
        RegSetValueExW(localeKey,
                       LocaleKeyData[i].pKeyName,
                       0,
                       REG_SZ,
                       (PBYTE)pGlobalData->pLocaleArray[i],
                       (wcslen(pGlobalData->pLocaleArray[i]) + 1) * sizeof(WCHAR));
    }

    /* Flush and close the locale key */
    RegFlushKey(localeKey);
    RegCloseKey(localeKey);

    /* Set the new locale for the current process */
    NtSetDefaultLocale(TRUE, pGlobalData->lcid);

}

/* Location enumerate procedure */
BOOL
CALLBACK
LocationsEnumProc(GEOID gId)
{
    TCHAR loc[MAX_STR_SIZE];
    INT index;

    if (GetGeoInfo(gId, GEO_FRIENDLYNAME, loc, MAX_STR_SIZE, LANG_SYSTEM_DEFAULT) == 0)
        return TRUE;

    index = (INT)SendMessage(hGeoList,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)loc);

    SendMessage(hGeoList,
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
    TCHAR loc[MAX_STR_SIZE];

    hGeoList = hWnd;

    EnumSystemGeoID(GEOCLASS_NATION, 0, LocationsEnumProc);

    /* Select current location */
    userGeoID = GetUserGeoID(GEOCLASS_NATION);
    GetGeoInfo(userGeoID,
               GEO_FRIENDLYNAME,
               loc,
               MAX_STR_SIZE,
               LANG_SYSTEM_DEFAULT);

    SendMessage(hGeoList,
                CB_SELECTSTRING,
                (WPARAM) -1,
                (LPARAM)loc);

    return userGeoID;
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
            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBALDATA));
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
                        pGlobalData->fUserLocaleChanged = TRUE;

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
                        pGlobalData->fGeoIdChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_SETUP_BUTTON:
                    SetupApplet(GetParent(hwndDlg), pGlobalData);
                    break;
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                if (lpnm->code == (UINT)PSN_APPLY)
                {
                    /* Apply changes */
                    PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

                    /* Set new locale */
                    if (pGlobalData->fUserLocaleChanged == TRUE)
                    {
                        SaveCurrentLocale(pGlobalData);
                        pGlobalData->fUserLocaleChanged = FALSE;
                    }

                    /* Set new GEO ID */
                    if (pGlobalData->fGeoIdChanged == TRUE)
                    {
                        SetUserGeoID(pGlobalData->geoid);
                        pGlobalData->fGeoIdChanged = FALSE;
                    }

                    AddNewKbLayoutsByLcid(pGlobalData->lcid);
                }
            }
            break;

        case WM_DESTROY:
            if (pGlobalData)
            {
                FreeCurrentLocale(pGlobalData);
                HeapFree(GetProcessHeap(), 0, pGlobalData);
            }
            break;
    }

    return FALSE;
}

/* EOF */
