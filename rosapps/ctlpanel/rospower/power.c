/*
 *  ReactOS rospower control panel applet
 *
 *  power.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include <time.h>

#include <windowsx.h>
#include "main.h"
#include "system.h"

#include "assert.h"
#include "trace.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

extern HMODULE hModule;
/*
HWND hSystemGeneralPage;
HWND hSystemNetworkPage;
HWND hSystemHardwarePage;
HWND hSystemUsersPage;
HWND hSystemAdvancedPage;
 */

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

#define DLGPROC_RESULT BOOL
//#define DLGPROC_RESULT LRESULT 

static HWND OnCreate(HWND hDlg, LONG lData)
{
//    TCHAR buffer[50];
    HWND hTabWnd = NULL;
//    RECT rc;

    //SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_TASKMANAGER)));

    // Create tab pages
//    hTabWnd = GetDlgItem(hDlg, IDC_CPL_DIALOG_TAB);
//    if (lData >= 0 && lData < GetCountProc()) {
//        CreateTabPages(hDlg, &CPlAppletDlgPagesData[lData]);
//    }
//    LoadString(hModule, CPlAppletDlgPagesData[lData].nDlgTitleId, buffer, sizeof(buffer)/sizeof(TCHAR));
//    SetWindowText(hDlg, buffer);
//    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 0);

    return hTabWnd;
}

static LRESULT OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, int nDlgCtrlID)
{
    TRACE(_T("OnCommand(%u, %08X, %08X)\n"), nDlgCtrlID, wParam, lParam);

    switch (LOWORD(wParam)) {
    case ID_APPLY:
        break;
    case IDC_SAVEAS_BUTTON:
        break;
    case IDC_DELETE_BUTTON:
        break;
    case IDC_CPL_POWER_UPS_ABOUT_BUTTON:
        MessageBox(hDlg, _T("IDC_CPL_POWER_UPS_ABOUT_BUTTON"), _T("Power Config Applet"), MB_ICONEXCLAMATION);
        return TRUE;
        break;
    case IDC_CPL_POWER_UPS_CONFIGURE_BUTTON:
        //MessageBox(hDlg, _T("IDC_CPL_POWER_UPS_CONFIGURE_BUTTON"), _T("Power Config Applet"), MB_ICONEXCLAMATION);
        break;
    case IDC_CPL_POWER_UPS_SELECT_BUTTON:
        //MessageBox(hDlg, _T("IDC_CPL_POWER_UPS_SELECT_BUTTON"), _T("Power Config Applet"), MB_ICONEXCLAMATION);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


static LRESULT OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam, int nDlgCtrlID)
{
    TRACE(_T("OnNotify(%u, %08X, %08X)\n"), nDlgCtrlID, wParam, lParam);
/*
//    int idctrl = (int)wParam;
//    HWND hTabWnd = GetDlgItem(hWnd, pPagesData->nTabCtrlId);
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if ((pnmh->hwndFrom == hTabWnd) && (pnmh->code == TCN_SELCHANGE)) {
//        (pnmh->idFrom == IDD_CPL_TABBED_DIALOG) &&
        TCITEM item;
        int i;
        int nActiveTabPage = TabCtrl_GetCurSel(hTabWnd);
        int nPageCount = TabCtrl_GetItemCount(hTabWnd);
        for (i = 0; i < nPageCount; i++) {
            item.mask = TCIF_PARAM;
            if (TabCtrl_GetItem(hTabWnd, i, &item)) {
                if ((HWND)item.lParam) {
                    if (i == nActiveTabPage) {
                        ShowWindow((HWND)item.lParam, SW_SHOW);
                        //BringWindowToTop((HWND)item.lParam);
                        //TabCtrl_SetCurFocus(hTabWnd, i);
                    } else {
                        ShowWindow((HWND)item.lParam, SW_HIDE);
                    }
                }
            }
        }
    }
//    assert(0);
 */
    return 0L;
}

static void OnTimer(HWND hDlg)
{
#if 0
    TCHAR buffer[20];
    struct tm *newtime;
    time_t aclock;
    HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIME_EDIT);
    time(&aclock);                 /* Get time in seconds */
    newtime = localtime(&aclock);  /* Convert time to struct tm form */
    swprintf(buffer, _T("%02u:%02u:%02u"), newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
    SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer);
#endif
}

static BOOL LoadPowerSchemesCombo2(HWND hDlg)
{
//    TCHAR buffer[20];
//    struct tm *newtime;
//    time_t aclock;
//    int i;
    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_POWER_SCHEMES_COMBO);
//    HWND hEdit = GetDlgItem(hDlg, IDC_CPL_POWER_SCHEMES_COMBO);

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0); 
//    time(&aclock);                 /* Get time in seconds */
//    newtime = localtime(&aclock);  /* Convert time to struct tm form */
/*
    for (i = IDS_CPL_DATETIME_MONTH_1; i <= IDS_CPL_DATETIME_MONTH_12; i++) {
        TCHAR buffer[34];
        if (LoadString(hModule, i, buffer, sizeof(buffer)/sizeof(TCHAR))) {
            int retval = SendMessage(hCombo, CB_INSERTSTRING, -1, (LPARAM)buffer);
            if (retval == CB_ERR) {
                MessageBox(hDlg, _T("CB_ERR: failed to add month item to combo box"), _T("Date Time Applet"), MB_ICONERROR);
                return FALSE;
            } else
            if (retval == CB_ERRSPACE) {
                MessageBox(hDlg, _T("CB_ERRSPACE: failed to add month item to combo box"), _T("Date Time Applet"), MB_ICONERROR);
                return FALSE;
            } else {
            }
        }
    }
 */
//    SendMessage(hCombo, CB_SETCURSEL, newtime->tm_mon, 0);
////    _itot(newtime->tm_year + 1900, buffer, 10);
////    SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer);
//    SendMessage(GetDlgItem(hDlg, IDC_CPL_POWER_UPS_MODEL_EDIT), WM_SETTEXT, 0, (LPARAM)_itot(newtime->tm_year + 1900, buffer, 10));
    return TRUE;
}
/*
#define IDC_CPL_POWER_SCHEMES_STANDBY_COMBO          1010
#define IDC_CPL_POWER_SCHEMES_MONITOROFF_COMBO       1011
#define IDC_CPL_POWER_SCHEMES_DISKOFF_COMBO          1012
#define IDC_CPL_POWER_SCHEMES_HIBERNATE_COMBO        1013
#define IDC_CPL_POWER_SCHEMES_COMBO                  1014
#define IDC_CPL_POWER_HIBERNATE_DISK_AVAIL_EDIT      1015
#define IDC_CPL_POWER_HIBERNATE_ENABLE_CHECK         1016
#define IDC_CPL_POWER_HIBERNATE_DISK_REQ_EDIT        1017
#define IDC_CPL_POWER_ADVANCED_RESUME_PASSWORD_CHECK 1018
#define IDC_CPL_POWER_ADVANCED_POWER_COMBO           1019
#define IDC_CPL_POWER_ADVANCED_SLEEP_COMBO           1020
#define IDC_CPL_POWER_ADVANCED_TASKBAR_ICON_CHECK    1021
#define IDC_CPL_POWER_UPS_POWER_SOURCE_EDIT          1022
#define IDC_CPL_POWER_UPS_ABOUT_BUTTON               1023
#define IDC_CPL_POWER_UPS_ESTIMATED_RUNTIME_EDIT     1024
#define IDC_CPL_POWER_UPS_ESTIMATED_CAPACITY_EDIT    1025
#define IDC_CPL_POWER_UPS_BATTERY_STATE_EDIT         1026
#define IDC_CPL_POWER_UPS_MANUFACTURER_EDIT          1027
#define IDC_CPL_POWER_UPS_MODEL_EDIT                 1028
#define IDC_CPL_POWER_UPS_SERVICE_STATUS_EDIT        1029
#define IDC_CPL_POWER_UPS_CONFIGURE_BUTTON           1030
#define IDC_CPL_POWER_UPS_SELECT_BUTTON              1031
 */

static BOOL ReadPowerPoliciesRegKey(HWND hDlg, HKEY hNewKey, TCHAR* Name)
{
    DWORD dwCount = 0L;
    HKEY hSubKey = 0;
    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_POWER_SCHEMES_COMBO);
    LONG errCode = errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hSubKey);
    if (errCode == ERROR_SUCCESS) {
        DWORD max_sub_key_len, max_val_name_len, max_val_size, val_count;
        /* get size information and resize the buffers if necessary */
        errCode = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL,
                    &max_sub_key_len, NULL, &val_count, &max_val_name_len, &max_val_size, NULL, NULL);
        if (errCode == ERROR_SUCCESS) {
            TCHAR* ValName = malloc(++max_val_name_len * sizeof(TCHAR));
            DWORD dwValNameLen = max_val_name_len;
            BYTE* ValBuf = malloc(++max_val_size);
            DWORD dwValSize = max_val_size;
            DWORD dwIndex = 0L;
            DWORD dwValType;
            while (RegEnumValue(hSubKey, dwIndex, ValName, &dwValNameLen, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) {
                ValBuf[dwValSize] = _T('\0');
                if (_tcsstr(ValName, _T("Name"))) {
                    //TRACE(_T("  %u %s = %s\n"), dwIndex, ValName, ValBuf);
                    int retval = SendMessage(hCombo, CB_INSERTSTRING, -1, (LPARAM)ValBuf);
                    if (retval == CB_ERR) {
                        MessageBox(hDlg, _T("CB_ERR: failed to add power policy item to combo box"), _T("Power Config Applet"), MB_ICONERROR);
                        return FALSE;
                    } else {
                        if (retval == CB_ERRSPACE) {
                            MessageBox(hDlg, _T("CB_ERRSPACE: failed to add power policy item to combo box"), _T("Power Config Applet"), MB_ICONERROR);
                            return FALSE;
                        } else {
                        }
                    }
                    dwValNameLen = max_val_name_len;
                    dwValSize = max_val_size;
                    dwValType = 0L;
                    ++dwIndex;
                }
            }
            free(ValBuf);
            free(ValName);
        }
        RegCloseKey(hSubKey);
    } else {
         TRACE(_T("ReadPowerPoliciesRegKey() ERROR: failed to open registry data %s\n"), Name);
    }
    return TRUE;
}

static BOOL LoadPowerSchemesCombo(HWND hDlg)
{
    HKEY hKey = HKEY_CURRENT_USER;
    TCHAR keyPath[1000];
    int keyPathLen = 0;

    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_POWER_SCHEMES_COMBO);
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0); 

    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode;
        _tcscpy(keyPath, _T("Control Panel\\PowerCfg\\PowerPolicies"));
        errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            DWORD dwIndex = 0L;
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                ReadPowerPoliciesRegKey(hDlg, hNewKey, Name);
                cName = MAX_NAME_LEN;
                ++dwIndex;
            }
            RegCloseKey(hNewKey);
        } else {
            TRACE(_T("LoadPowerSchemesCombo() ERROR: failed to open registry key %s\n"), keyPath);
            MessageBox(hDlg, _T("ERROR: failed to open registry key"), _T("Power Config Applet"), MB_ICONERROR);
        }

        _tcscpy(keyPath, _T("Control Panel\\PowerCfg"));
        errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR buffer[32];
            DWORD dwSize = sizeof(buffer)/sizeof(buffer[0]);
            errCode = RegQueryValueEx(hNewKey, _T("CurrentPowerPolicy"), NULL, NULL, (LPBYTE)buffer, &dwSize);
            if (errCode == ERROR_SUCCESS) {
                SendMessage(hCombo, CB_SETCURSEL, _ttoi(buffer), 0);
            }
            RegCloseKey(hNewKey);
        } else {
            TRACE(_T("LoadPowerSchemesCombo() ERROR: failed to open registry key %s\n"), keyPath);
            MessageBox(hDlg, _T("ERROR: failed to open registry key"), _T("Power Config Applet"), MB_ICONERROR);
        }


    } else {
        MessageBox(hDlg, _T("ERROR: no registry root key"), _T("Power Config Applet"), MB_ICONERROR);
    }
    return TRUE;
}

#define ID_CPL_POWERCFG_TIMER_ID 36
#define ID_CPL_POWERCFG_TIMER_PERIOD 1000
static int nTimerID;

static BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
//    BYTE StandardNameBuf[100];
//    TCHAR keyPath[1000];
//    HKEY hNewKey;
//    LONG errCode;

    TRACE(_T("OnInitDialog(%08X, %08X, %08X)\n"), hDlg, wParam, lParam);

//    nTimerID = SetTimer(hDlg, ID_CPL_POWERCFG_TIMER_ID, ID_CPL_POWERCFG_TIMER_PERIOD, NULL);
//    if (!nTimerID) {
//        return FALSE;
//    }

    if (lParam == IDD_CPL_POWER_SCHEMES_PAGE) {
        //HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_CURRENT_TIMEZONE_EDIT);
        LoadPowerSchemesCombo(hDlg);
        //SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)StandardNameBuf);
    } else 
    if (lParam == IDD_CPL_POWER_ADVANCED_PAGE) {
        //HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIMEZONE_COMBO);
        LoadPowerSchemesCombo2(hDlg);
//        SendMessage(hCombo, CB_SETCURSEL, newtime->tm_mon, 0);
        //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)StandardNameBuf);
        //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)_T("(UTC+10:00) Australia New South Wales"));
    } else 
    if (lParam == IDD_CPL_POWER_HIBERNATE_PAGE) {
    } else 
    if (lParam == IDD_CPL_POWER_UPS_PAGE) {
    } else 
    if (lParam == IDD_CPL_POWER_HARDWARE_PAGE) {
    }
/*
    StandardNameBuf[0] = _T('\0');
    //_tcscpy(keyPath, _T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"));
    _tcscpy(keyPath, _T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInfo"));
    errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hNewKey);
    if (errCode == ERROR_SUCCESS) {
        DWORD dwValSize = 100;
        DWORD dwValType;
        if (RegQueryValueEx(hNewKey, _T("StandardName"), NULL, &dwValType, StandardNameBuf, &dwValSize) == ERROR_SUCCESS) {
        }
        RegCloseKey(hNewKey);

        if (lParam == IDD_CPL_POWER_SCHEMES_PAGE) {
            HWND hEdit = GetDlgItem(hDlg, IDC_CPL_POWER_HIBERNATE_DISK_REQ_EDIT);
            SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)StandardNameBuf);
        }

        _tcscpy(keyPath, _T("SOFTWARE\\ReactWare\\Windows NT\\CurrentVersion\\Time Zones\\"));
        _tcscat(keyPath, StandardNameBuf);

        TRACE(_T("OnInitDialog() keyPath %s\n"), keyPath);

        errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            dwValSize = 100;
            if (RegQueryValueEx(hNewKey, _T("Display"), NULL, &dwValType, StandardNameBuf, &dwValSize) == ERROR_SUCCESS) {
                if (lParam == IDD_CPL_POWER_ADVANCED_PAGE) {
                    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_POWER_SCHEMES_MONITOROFF_COMBO);
                    //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)StandardNameBuf);
                    SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)_T("(UTC+10:00) Australia New South Wales"));
                }
            }
            RegCloseKey(hNewKey);
        }
    } else {
        TRACE(_T("LoadPowerSchemesCombo() ERROR: failed to open registry key %s\n"), keyPath);
        MessageBox(hDlg, _T("ERROR: failed to open registry key"), _T("Power Config Applet"), MB_ICONERROR);
    }
 */
    return TRUE;
}

DLGPROC_RESULT CALLBACK CPlPowerAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//    LONG nDlgCtrlID = GetWindowLong(hDlg, GWL_ID);
    LONG nDlgCtrlID = GetWindowLong(hDlg, DWL_USER);


    switch (message) {
    case WM_INITDIALOG:
        SetWindowLong(hDlg, DWL_USER, lParam);
        return OnInitDialog(hDlg, wParam, lParam);

    case WM_DESTROY:
        if (nTimerID) KillTimer(hDlg, nTimerID);
        break;

    case WM_COMMAND:
        return OnCommand(hDlg, wParam, lParam, nDlgCtrlID);
        // Handle the button clicks
/*
        switch (LOWORD(wParam)) {
//        case IDC_ENDTASK:
//            break;
//        }
        case IDOK:
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        break;
 */
    case WM_TIMER:
        OnTimer(hDlg);
        break;

    case WM_NOTIFY:
        return OnNotify(hDlg, wParam, lParam, nDlgCtrlID);
        //break;
    }
    return 0;
}

