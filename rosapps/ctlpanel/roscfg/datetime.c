/*
 *  ReactOS control
 *
 *  datetime.c
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
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <math.h>
//#include <windowsx.h>
#include "main.h"
#include "system.h"
#include "assert.h"
#include "trace.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

#define DLGPROC_RESULT BOOL
//#define DLGPROC_RESULT LRESULT 

extern HMODULE hModule;

typedef LRESULT(WINAPI *t_fnOldWndProc)(HWND,UINT,WPARAM,LPARAM);
//FARPROC lpfnOldWndProc;
t_fnOldWndProc lpfnOldWndProc;

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void SetIsoTropic(HDC hdc, int cxClient, int cyClient)
{
    SetMapMode(hdc, MM_ISOTROPIC);
    SetWindowExtEx(hdc, 1000, 1000, NULL);
    SetViewportExtEx(hdc, cxClient / 2, -cyClient / 2, NULL);
    SetViewportOrgEx(hdc, cxClient / 2,  cyClient / 2, NULL);
}

#define TWOPI (2*3.14159)

static void RotatePoint(POINT pt[], int iNum, int iAngle)
{
    int i;
    POINT ptTemp;
    for (i = 0; i < iNum; i++) {
        ptTemp.x = (int)(pt[i].x * cos(TWOPI * iAngle / 360) +
                         pt[i].y * sin(TWOPI * iAngle / 360));
        ptTemp.y = (int)(pt[i].y * cos(TWOPI * iAngle / 360) -
                         pt[i].x * sin(TWOPI * iAngle / 360));
        pt[i] = ptTemp;
    }
}

static void DrawClock(HDC hdc)
{
    int iAngle;
    POINT pt[3];

    for (iAngle = 0; iAngle < 360; iAngle += 6) {
        pt[0].x = 0;
        pt[0].y = 900;

        RotatePoint(pt, 1, iAngle);

        pt[2].x = pt[2].y = iAngle % 5 ? 23 : 100;

        pt[0].x -= pt[2].x / 2; 
        pt[0].y -= pt[2].y / 2; 

        pt[1].x = pt[0].x + pt[2].x; 
        pt[1].y = pt[0].y + pt[2].y; 

        SelectObject(hdc, GetStockObject(BLACK_BRUSH));
        Ellipse(hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
    }
}

static void DrawHands(HDC hdc, struct tm* datetime, BOOL bChange)
{
    static POINT pt[3][5] = { 0, -150, 100, 0, 0, 600, -100, 0, 0, -150,
                              0, -200,  50, 0, 0, 800,  -50, 0, 0, -200,
                              0,    0,   0, 0, 0,   0,    0, 0, 0, -800 };
    int i, iAngle[3];
    POINT ptTemp[3][5];

    iAngle[0] = (datetime->tm_hour * 30) % 360 + datetime->tm_min / 2;
    iAngle[1] = datetime->tm_min * 6;
    iAngle[2] = datetime->tm_sec * 6;

    memcpy(ptTemp, pt, sizeof(pt));
    
    for (i = bChange ? 0 : 2; i < 3; i++) {
        RotatePoint(ptTemp[i], 5, iAngle[i]);
        Polyline(hdc, ptTemp[i], 5);
    }
}

DLGPROC_RESULT CALLBACK AnalogClockFaceWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int cxClient, cyClient;
    static struct tm dtPrevious;
    BOOL bChange;
    HDC hdc;
    PAINTSTRUCT ps;
    time_t lTime;
    struct tm * datetime;
    RECT rc;

    switch (message) {
    case WM_CREATE:
        GetClientRect(hWnd, &rc);
        //GetWindowRect(hWnd, &rc);
        cxClient = rc.right - rc.left;
        cyClient = rc.bottom - rc.top;
        time(&lTime);
        datetime = localtime(&lTime);
        dtPrevious = *datetime;
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        SetIsoTropic(hdc, cxClient, cyClient);
        DrawClock(hdc);
        DrawHands(hdc, &dtPrevious, TRUE);
        EndPaint(hWnd, &ps);
        return 0;

    case WM_TIMER:
        time(&lTime);
        datetime = localtime(&lTime);
        bChange = datetime->tm_hour != dtPrevious.tm_hour ||
                  datetime->tm_min  != dtPrevious.tm_min;
        hdc = GetDC(hWnd);
        SetIsoTropic(hdc, cxClient, cyClient);

        SelectObject(hdc, GetStockObject(WHITE_PEN));
        {
        //HPEN hPen = CreatePen(PS_SOLID, 0, RGB(255,0,0));
        //hPen = SelectObject(hdc, hPen);
        DrawHands(hdc, &dtPrevious, bChange);
        SelectObject(hdc, GetStockObject(BLACK_PEN));
        DrawHands(hdc, datetime, TRUE);
        ReleaseDC(hWnd, hdc);
        dtPrevious = *datetime;
        //hPen = SelectObject(hdc, hPen);
        //DeleteObject(hPen);
        }
        return 0;

    case WM_SIZE:
        cxClient = LOWORD(wParam);
        cyClient = HIWORD(wParam);
        return 0;

    }
    return CallWindowProc(lpfnOldWndProc, hWnd, message, wParam, lParam);
}

static LRESULT OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, int nDlgCtrlID)
{
//    TRACE(_T("OnCommand(%u, %08X, %08X)\n"), nDlgCtrlID, wParam, lParam);
        // Handle the button clicks
/*
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        break;
 */
    return 0L;
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
    TCHAR buffer[20];
    struct tm *newtime;
    time_t aclock;
    HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIME_EDIT);
    time(&aclock);                 /* Get time in seconds */
    newtime = localtime(&aclock);  /* Convert time to struct tm form */
    swprintf(buffer, _T("%02u:%02u:%02u"), newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
    SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer);
    {
    HWND hClockFace = GetDlgItem(hDlg, IDC_CPL_DATETIME_CLOCKFACE);
    SendMessage(hClockFace, WM_TIMER, 0, 0);
    }
}

static BOOL SetEarthMapOffset(HWND hDlg)
{
//    int i;
    HWND hImage = GetDlgItem(hDlg, IDC_CPL_DATETIME_EARTHMAP_IMAGE);
    //SendMessage(hImage, CB_RESETCONTENT, 0, 0); 
    return TRUE;
}

static BOOL LoadDateTimeMonthCombo(HWND hDlg)
{
    TCHAR buffer[20];
    struct tm *newtime;
    time_t aclock;
    int i;
    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_MONTH_COMBO);
    HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_YEAR_EDIT);

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0); 
    time(&aclock);                 /* Get time in seconds */
    newtime = localtime(&aclock);  /* Convert time to struct tm form */
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
    SendMessage(hCombo, CB_SETCURSEL, newtime->tm_mon, 0);
//    _itot(newtime->tm_year + 1900, buffer, 10);
//    SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer);
    SendMessage(GetDlgItem(hDlg, IDC_CPL_DATETIME_YEAR_EDIT), WM_SETTEXT, 0, (LPARAM)_itot(newtime->tm_year + 1900, buffer, 10));
    return TRUE;
}

//    IDC_CPL_DATETIME_DAYLIGHT_SAVINGS_CHECK

static BOOL ReadTimeZoneRegKey(HWND hDlg, HKEY hNewKey, TCHAR* Name)
{
    DWORD dwCount = 0L;
    HKEY hSubKey = 0;
    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIMEZONE_COMBO);
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
//            if (RegQueryValueEx(hSubKey, NULL, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) {
//                AddEntryToList(hwndLV, _T("(Default)"), dwValType, ValBuf, dwValSize);
//            }
//            dwValSize = max_val_size;
            while (RegEnumValue(hSubKey, dwIndex, ValName, &dwValNameLen, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) {
                ValBuf[dwValSize] = _T('\0');
                //AddEntryToList(hwndLV, ValName, dwValType, ValBuf, dwValSize);
                if (_tcsstr(ValName, _T("Display"))) {
                    //TRACE(_T("  %u %s = %s\n"), dwIndex, ValName, ValBuf);
                    int retval = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)ValBuf);
                    if (retval == CB_ERR) {
                        MessageBox(hDlg, _T("CB_ERR: failed to add timezone item to combo box"), _T("Date Time Applet"), MB_ICONERROR);
                        return FALSE;
                    } else {
                        if (retval == CB_ERRSPACE) {
                            MessageBox(hDlg, _T("CB_ERRSPACE: failed to add timezone item to combo box"), _T("Date Time Applet"), MB_ICONERROR);
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
         TRACE(_T("ReadTimeZoneRegKey() ERROR: failed to open registry data %s\n"), Name);
    }
    return TRUE;
}

static BOOL LoadTimeZoneCombo(HWND hDlg)
{
    HKEY hKey = HKEY_LOCAL_MACHINE;
    TCHAR keyPath[1000];
    int keyPathLen = 0;

    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIMEZONE_COMBO);
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0); 

    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode;
        _tcscpy(keyPath, _T("SOFTWARE\\ReactWare\\Windows NT\\CurrentVersion\\Time Zones"));
        errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            DWORD dwIndex = 0L;
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                ReadTimeZoneRegKey(hDlg, hNewKey, Name);
                cName = MAX_NAME_LEN;
                ++dwIndex;
            }
            RegCloseKey(hNewKey);
        } else {
            TRACE(_T("LoadTimeZoneCombo() ERROR: failed to open registry key %s\n"), keyPath);
        }

    } else {
        TRACE(_T("LoadTimeZoneCombo() ERROR: no registry root key\n"));
    }
    return TRUE;
}

#define ID_CPL_DATETIME_TIMER_ID 34
#define ID_CPL_DATETIME_TIMER_PERIOD 1000
static int nTimerID;

static BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    TCHAR StandardNameBuf[100];
    TCHAR keyPath[1000];
    HKEY hNewKey;
    LONG errCode;

    TRACE(_T("OnInitDialog(%08X, %08X, %08X)\n"), hDlg, wParam, lParam);

    nTimerID = SetTimer(hDlg, ID_CPL_DATETIME_TIMER_ID, ID_CPL_DATETIME_TIMER_PERIOD, NULL);
    if (!nTimerID) {
        return FALSE;
    }

    if (lParam == IDD_CPL_DATETIME_DATETIME_PAGE) {

        HWND hClockFace = GetDlgItem(hDlg, IDC_CPL_DATETIME_CLOCKFACE);
        //lpfnOldWndProc = (FARPROC)SetWindowLong(hClockFace, GWL_WNDPROC, (DWORD)AnalogClockFaceWndProc);
        lpfnOldWndProc = (t_fnOldWndProc)SetWindowLong(hClockFace, GWL_WNDPROC, (DWORD)AnalogClockFaceWndProc);

        SendMessage(hClockFace, WM_CREATE, 0, 0);

        //HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_CURRENT_TIMEZONE_EDIT);
        LoadDateTimeMonthCombo(hDlg);
        //SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)StandardNameBuf);
    } else 
    if (lParam == IDD_CPL_DATETIME_TIMEZONE_PAGE) {
        //HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIMEZONE_COMBO);
        LoadTimeZoneCombo(hDlg);
//        SendMessage(hCombo, CB_SETCURSEL, newtime->tm_mon, 0);
        //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)StandardNameBuf);
        //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)_T("(UTC+10:00) Australia New South Wales"));
    }

    StandardNameBuf[0] = _T('\0');
    //_tcscpy(keyPath, _T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"));
    _tcscpy(keyPath, _T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInfo"));
    errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hNewKey);
    if (errCode == ERROR_SUCCESS) {
        DWORD dwValSize = 100;
        DWORD dwValType;
        if (RegQueryValueEx(hNewKey, _T("StandardName"), NULL, &dwValType, (LPBYTE)StandardNameBuf, &dwValSize) == ERROR_SUCCESS) {
        }
        RegCloseKey(hNewKey);

        if (lParam == IDD_CPL_DATETIME_DATETIME_PAGE) {
            HWND hEdit = GetDlgItem(hDlg, IDC_CPL_DATETIME_CURRENT_TIMEZONE_EDIT);
            SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)StandardNameBuf);
        }

        _tcscpy(keyPath, _T("SOFTWARE\\ReactWare\\Windows NT\\CurrentVersion\\Time Zones\\"));
        _tcscat(keyPath, StandardNameBuf);

        TRACE(_T("OnInitDialog() keyPath %s\n"), keyPath);

        errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            dwValSize = 100;
            if (RegQueryValueEx(hNewKey, _T("Display"), NULL, &dwValType, (LPBYTE)StandardNameBuf, &dwValSize) == ERROR_SUCCESS) {
                if (lParam == IDD_CPL_DATETIME_TIMEZONE_PAGE) {
                    HWND hCombo = GetDlgItem(hDlg, IDC_CPL_DATETIME_TIMEZONE_COMBO);
                    //SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)StandardNameBuf);
                    SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)_T("(UTC+10:00) Australia New South Wales"));
                }
            }
            RegCloseKey(hNewKey);
        }
    } else {
        TRACE(_T("LoadTimeZoneCombo() ERROR: failed to open registry key %s\n"), keyPath);
    }

    return TRUE;
}

DLGPROC_RESULT CALLBACK DateTimeAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//    LONG nDlgCtrlID = GetWindowLong(hDlg, GWL_ID);
    LONG nDlgCtrlID = GetWindowLong(hDlg, GWL_USERDATA);


    switch (message) {
    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        return OnInitDialog(hDlg, wParam, lParam);

    case WM_DESTROY:
        if (nTimerID) KillTimer(hDlg, nTimerID);
        break;

    case WM_COMMAND:
        return OnCommand(hDlg, wParam, lParam, nDlgCtrlID);

    case WM_TIMER:
        OnTimer(hDlg);
        break;

    case WM_NOTIFY:
        return OnNotify(hDlg, wParam, lParam, nDlgCtrlID);
    }
    return 0;
}
