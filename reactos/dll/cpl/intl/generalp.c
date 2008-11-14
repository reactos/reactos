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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/generalp.c
 * PURPOSE:         General property page
 * PROGRAMMER:      Eric Kohl
 *                  Klemens Friedl
 *                  Aleksey Bragin
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>

#include "intl.h"
#include "resource.h"

#define SAMPLE_NUMBER   _T("123456789")
#define NO_FLAG         0

HWND hList;
HWND hLocaleList, hGeoList;
BOOL bSpain = FALSE;

static BOOL CALLBACK
LocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    TCHAR lang[255];
    INT index;
    BOOL bNoShow = FALSE;

    lcid = _tcstoul(lpLocale, NULL, 16);

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

/* Update all locale samples */
static VOID
UpdateLocaleSample(HWND hwndDlg, LCID lcidLocale)
{
    TCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get number format sample */
    GetNumberFormat(lcidLocale, NO_FLAG, SAMPLE_NUMBER, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMSAMPLE_EDIT),
                 WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get monetary format sample */
    GetCurrencyFormat(lcidLocale, LOCALE_USE_CP_ACP, SAMPLE_NUMBER, NULL,
        OutBuffer, MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_MONEYSAMPLE_EDIT),
                 WM_SETTEXT, 0, (LPARAM)OutBuffer);

    /* Get time format sample */
    GetTimeFormat(lcidLocale, NO_FLAG, NULL, NULL, OutBuffer, MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_TIMESAMPLE_EDIT),
        WM_SETTEXT,
        0,
        (LPARAM)OutBuffer);

    /* Get short date format sample */
    GetDateFormat(lcidLocale, DATE_SHORTDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_SHORTTIMESAMPLE_EDIT), WM_SETTEXT,
        0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormat(lcidLocale, DATE_LONGDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessage(GetDlgItem(hwndDlg, IDC_FULLTIMESAMPLE_EDIT),
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

/* Sets new locale */
VOID
SetNewLocale(LCID lcid)
{
    // HKCU\\Control Panel\\International\\Locale = 0409 (type=0)
    // HKLM,"SYSTEM\CurrentControlSet\Control\NLS\Language","Default",0x00000000,"0409" (type=0)
    // HKLM,"SYSTEM\CurrentControlSet\Control\NLS\Language","InstallLanguage",0x00000000,"0409" (type=0)

    // Set locale
    HKEY localeKey;
    HKEY langKey;
    DWORD ret;
    TCHAR value[9];
    DWORD valuesize;
    TCHAR ACPPage[9];
    TCHAR OEMPage[9];

    ret = GetLocaleInfo(MAKELCID(lcid, SORT_DEFAULT), LOCALE_IDEFAULTCODEPAGE, OEMPage, sizeof(OEMPage)/sizeof(TCHAR));
    if (ret == 0)
    {
        MessageBox(NULL, _T("Problem reading OEM code page"), _T("Big Problem"), MB_OK);
        return;
    }

    ret = GetLocaleInfo(MAKELCID(lcid, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, ACPPage, sizeof(ACPPage)/sizeof(TCHAR));
    if (ret == 0)
    {
        MessageBox(NULL, _T("Problem reading ANSI code page"), _T("Big Problem"), MB_OK);
        return;
    }

    ret = RegOpenKey(HKEY_CURRENT_USER, _T("Control Panel\\International"), &localeKey);
    if (ret != ERROR_SUCCESS)
    {
        // some serious error
        MessageBox(NULL, _T("Problem opening HKCU\\Control Panel\\International key"),
                   _T("Big Problem"), MB_OK);
        return;
    }

    wsprintf(value, _T("%04X"), (DWORD)lcid);
    valuesize = (_tcslen(value) + 1) * sizeof(TCHAR);

    RegSetValueEx(localeKey, _T("Locale"), 0, REG_SZ, (LPBYTE)value, valuesize);
    RegCloseKey(localeKey);

    ret = RegOpenKey(HKEY_USERS, _T(".DEFAULT\\Control Panel\\International"), &localeKey);
    if (ret != ERROR_SUCCESS)
    {
        // some serious error
        MessageBox(NULL, _T("Problem opening HKU\\.DEFAULT\\Control Panel\\International key"),
                   _T("Big Problem"), MB_OK);
        return;
    }

    wsprintf(value, _T("%04X"), (DWORD)lcid);
    valuesize = (_tcslen(value) + 1) * sizeof(TCHAR);

    RegSetValueEx(localeKey, _T("Locale"), 0, REG_SZ, (BYTE *)value, valuesize);
    RegCloseKey(localeKey);

    // Set language
    ret = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\NLS\\Language"), &langKey);
    if (ret != ERROR_SUCCESS)
    {
        MessageBoxW(NULL, _T("Problem opening HKLM\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language key"),
                    _T("Big Problem"), MB_OK);
        return;
    }

    RegSetValueEx(langKey, _T("Default"), 0, REG_SZ, (BYTE *)value, valuesize );
    RegSetValueEx(langKey, _T("InstallLanguage"), 0, REG_SZ, (BYTE *)value, valuesize );

    RegCloseKey(langKey);


    /* Set language */
    ret = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage"), &langKey);
    if (ret != ERROR_SUCCESS)
    {
        MessageBox(NULL, _T("Problem opening HKLM\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage key"),
                   _T("Big Problem"), MB_OK);
        return;
    }

    RegSetValueExW(langKey, _T("OEMCP"), 0, REG_SZ, (BYTE *)OEMPage, (_tcslen(OEMPage) +1 ) * sizeof(TCHAR));
    RegSetValueExW(langKey, _T("ACP"), 0, REG_SZ, (BYTE *)ACPPage, (_tcslen(ACPPage) +1 ) * sizeof(TCHAR));

    RegCloseKey(langKey);
}

/* Location enumerate procedure */
BOOL
CALLBACK
LocationsEnumProc(GEOID gId)
{
    TCHAR loc[MAX_STR_SIZE];
    INT index;

    GetGeoInfo(gId, GEO_FRIENDLYNAME, loc, MAX_FMT_SIZE, LANG_SYSTEM_DEFAULT);
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

    SendMessage(hGeoList,
                CB_SELECTSTRING,
                (WPARAM) -1,
                (LPARAM)loc);
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
    switch(uMsg)
    {
        case WM_INITDIALOG:
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));
            UpdateLocaleSample(hwndDlg, LOCALE_USER_DEFAULT);
            CreateLocationsList(GetDlgItem(hwndDlg, IDC_LOCATION_COMBO));
            if (IsUnattendedSetupEnabled)
            {
                if (VerifyUnattendLCID(hwndDlg))
                {
                    SetNewLocale(UnattendLCID);
                    PostQuitMessage(0);
                }
                return TRUE;
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

                        UpdateLocaleSample(hwndDlg, NewLcid);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_LOCATION_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                case IDC_SETUP_BUTTON:
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

                         SetupApplet(GetParent(hwndDlg), NewLcid);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                if (lpnm->code == (UINT)PSN_APPLY)
                {
                    /* Apply changes */
                    LCID NewLcid;
                    GEOID NewGeoID;
                    INT iCurSel;

                    PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

                    /* Acquire new value */
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

                    /* Set new locale */
                    SetNewLocale(NewLcid);
                    SetUserGeoID(NewGeoID);
                    SetNonUnicodeLang(hwndDlg, NewLcid);
                }
            }
            break;
    }

    return FALSE;
}


/* EOF */
