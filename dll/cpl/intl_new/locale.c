/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/locale.h
 * PURPOSE:         Regional parameters page
 * PROGRAMMERS:     Eric Kohl
 *                  Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define SAMPLE_NUMBER   L"123456789"
#define NO_FLAG         0

HWND hLocaleList, hGeoList;

/* FUNCTIONS ****************************************************************/

/* Location enumerate procedure */
BOOL
CALLBACK
LocationsEnumProc(GEOID gId)
{
    TCHAR loc[MAX_STR_SIZE];
    int index;

    GetGeoInfo(gId, GEO_FRIENDLYNAME, loc, MAX_FMT_SIZE, LANG_SYSTEM_DEFAULT);
    index = (int) SendMessageW(hGeoList,
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
VOID
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
               MAX_FMT_SIZE,
               LANG_SYSTEM_DEFAULT);

    SendMessageW(hGeoList,
                 CB_SELECTSTRING,
                 (WPARAM) -1,
                 (LPARAM)loc);
}

/* Locale enumerate procedure */
BOOL
CALLBACK
LocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    TCHAR lang[MAX_STR_SIZE];
    int index;

    lcid = wcstoul(lpLocale, NULL, 16);

    GetLocaleInfo(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang));
    index = (int) SendMessageW(hLocaleList,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)lang);

    SendMessageW(hLocaleList,
                 CB_SETITEMDATA,
                 (WPARAM) index,
                 (LPARAM)lcid);

    return TRUE;
}

/* Enumerate all installed locale identifiers */
static
VOID
CreateLanguagesList(HWND hWnd)
{
    TCHAR langSel[MAX_STR_SIZE];

    hLocaleList = hWnd;

    EnumSystemLocalesW(LocalesEnumProc, LCID_INSTALLED);

    /* Select current locale */
    GetLocaleInfo(GetUserDefaultLCID(),
                  LOCALE_SLANGUAGE,
                  langSel,
                  sizeof(langSel));

    SendMessageW(hLocaleList,
                 CB_SELECTSTRING,
                 (WPARAM) -1,
                 (LPARAM)langSel);
}


/* Update all locale samples */
static
VOID
UpdateLocaleSample(HWND hwndDlg, LCID lcidLocale)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get number format sample */
    GetNumberFormatW(lcidLocale, NO_FLAG, SAMPLE_NUMBER, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMSAMPLE_EDIT),
                 WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get monetary format sample */
    GetCurrencyFormatW(lcidLocale, LOCALE_USE_CP_ACP, SAMPLE_NUMBER, NULL,
        OutBuffer, MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_MONEYSAMPLE_EDIT),
                 WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get time format sample */
    GetTimeFormatW(lcidLocale, NO_FLAG, NULL, NULL, OutBuffer, MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESAMPLE_EDIT),
        WM_SETTEXT,
        0,
        (LPARAM)OutBuffer);

    /* Get short date format sample */
    GetDateFormatW(lcidLocale, DATE_SHORTDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHORTTIMESAMPLE_EDIT), WM_SETTEXT,
        0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormatW(lcidLocale, DATE_LONGDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_FULLTIMESAMPLE_EDIT),
        WM_SETTEXT, 0, (LPARAM)OutBuffer);
}

/* Setting up new locale */
static
VOID
SetNewLocale(LCID LcidLocale)
{
    int RetCode;
    TCHAR OutBuff[MAX_STR_SIZE];
    int LocaleCounter;
    HKEY LocaleKey;
    TCHAR Value[9];
    DWORD ValueSize;

    //SetUserDefaultLCID(LcidLocale); // Not implemented in MS :(

    if(!IsValidLocale(LcidLocale, LCID_INSTALLED))
    {
        MessageBoxW(NULL, L"Invalid locale.", L"Critical error",
            MB_OK | MB_ICONERROR);
        return;
    }

    //HACKHACK: I'm sure there is a better way to do this
    for(LocaleCounter = LOCALE_ILANGUAGE;
        LocaleCounter <= LOCALE_SISO3166CTRYNAME; LocaleCounter++)
    {
        RetCode = GetLocaleInfoW(LcidLocale,
                                 (LCTYPE)LocaleCounter,
                                 OutBuff,
                                 sizeof(OutBuff));
        if(RetCode == ERROR_INVALID_PARAMETER)
            continue;
        SetLocaleInfoW(LOCALE_USER_DEFAULT, (LCTYPE)LocaleCounter, OutBuff);
    }

    RetCode = RegOpenKeyW(HKEY_CURRENT_USER,
                          L"Control Panel\\International",
                          &LocaleKey);

    if(RetCode != ERROR_SUCCESS)
    {
        MessageBoxW(NULL,
                    L"Problem opening HKCU\\Control Panel\\International key",
                    L"Critical error", MB_OK | MB_ICONERROR);
        return;
    }

    wsprintf(Value, L"%04X", (DWORD)LcidLocale);
    ValueSize = (DWORD) (wcslen(Value) + 1) * sizeof(WCHAR);

    RegSetValueExW(LocaleKey, L"Locale", 0, REG_SZ, (BYTE *)Value, ValueSize);
    RegCloseKey(LocaleKey);
}

/* Regional Parameters page dialog callback */
INT_PTR
CALLBACK
RegOptsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int CurrSel;
    LCID NewLCID;
    GEOID NewLocation;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            CreateLocationsList(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO));
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_SETREG_COMBO));
            /* Update locale samples */
            UpdateLocaleSample(hwndDlg, LOCALE_USER_DEFAULT);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                /* If setup button pressed */
                case IDC_SETUP_BUTTON:
                    SetupApplet(hwndDlg, uMsg, wParam, lParam);
                break;

                /* If selected other locale */
                case IDC_SETREG_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        CurrSel = (int) SendMessageW((HWND)lParam, CB_GETCURSEL,
                            0, 0);
                        NewLCID = (LCID) SendMessageW((HWND)lParam, CB_GETITEMDATA,
                            CurrSel, 0);
                        UpdateLocaleSample(hwndDlg,
                                           MAKELCID(NewLCID, SORT_DEFAULT));
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                break;

                /* If changed location */
                case IDC_LOCATION_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Set "Apply" button enabled */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                break;
            }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            /* If push apply button */
            if (lpnm->code == (UINT)PSN_APPLY)
            {
                /* Set locale */
                CurrSel = (int) SendMessageW(GetDlgItem(hwndDlg, IDC_SETREG_COMBO),
                    CB_GETCURSEL, 0, 0);
                NewLCID = (LCID) SendMessageW(GetDlgItem(hwndDlg, IDC_SETREG_COMBO),
                    CB_GETITEMDATA, CurrSel, 0);
                if (NewLCID == (LCID)CB_ERR) break;

                SetNewLocale(MAKELCID(NewLCID, SORT_DEFAULT));

                /* Set geo location */
                CurrSel = (int) SendMessageW(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO),
                               CB_GETCURSEL,
                               0,
                               0);
                NewLocation = (GEOID) SendMessageW(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO),
                              CB_GETITEMDATA,
                              CurrSel,
                              0);
                SetUserGeoID(NewLocation);
            }
        }
        break;
    }
    return FALSE;
}

/* EOF */
