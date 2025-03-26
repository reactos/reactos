/*
 * Star field screensaver
 *
 * Copyright 2011 Carlo Bramini
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>

#include "resource.h"
#include "settings.h"

#define SIZEOF(_v)  (sizeof(_v) / sizeof(*_v))

// Options for the starfield
SSSTARS Settings;

// Factory default settings.
static const SSSTARS FactoryDefaults = {
    MAX_STARS,
    20,
    ROTATION_PERIODIC,

    TRUE,
    TRUE,
    TRUE,
    TRUE
};

static const DWORD RotoStrings[] = {
    IDS_ROTATION_NONE,
    IDS_ROTATION_LINEAR,
    IDS_ROTATION_PERIODIC
};

static DWORD QueryDWORD(HKEY hKey, LPCTSTR pszValueName, DWORD Default)
{
    DWORD dwData, dwType, cbData;
    LONG  lRes;

    dwType = REG_DWORD;
    cbData = sizeof(DWORD);

    lRes = RegQueryValueEx(
                hKey,
                pszValueName,
                NULL,
                &dwType,
                (LPBYTE)&dwData,
                &cbData);

    if (lRes != ERROR_SUCCESS || dwType != REG_DWORD)
        return Default;

    return dwData;
}

static void SaveDWORD(HKEY hKey, LPCTSTR pszValueName, DWORD dwValue)
{
    RegSetValueEx(hKey, pszValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
}


void LoadSettings(void)
{
    HKEY  hKey;
    LONG  lRes;

    Settings = FactoryDefaults;

    lRes = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            _T("Software\\Microsoft\\ScreenSavers\\Ssstars"),
            0,
            _T(""),
            0,
            KEY_READ,
            NULL,
            &hKey,
            NULL);

    if (lRes != ERROR_SUCCESS)
        return;

    Settings.uiNumStars = QueryDWORD(hKey, _T("NumberOfStars"),  Settings.uiNumStars);
    Settings.uiSpeed    = QueryDWORD(hKey, _T("Speed"),          Settings.uiSpeed);
    Settings.uiRotation = QueryDWORD(hKey, _T("TypeOfRotation"), Settings.uiRotation);

    Settings.bDoBlending      = QueryDWORD(hKey, _T("DoBlending"),      Settings.bDoBlending);
    Settings.bFinePerspective = QueryDWORD(hKey, _T("FinePerspective"), Settings.bFinePerspective);
    Settings.bEnableFiltering = QueryDWORD(hKey, _T("EnableFiltering"), Settings.bEnableFiltering);
    Settings.bSmoothShading   = QueryDWORD(hKey, _T("SmoothShading"),   Settings.bSmoothShading);

    // Check the number of stars to be in range
    if (Settings.uiNumStars < MIN_STARS)
        Settings.uiNumStars = MIN_STARS;
    else
    if (Settings.uiNumStars > MAX_STARS)
        Settings.uiNumStars = MAX_STARS;

    // Check the speed to be in range
    if (Settings.uiSpeed < MIN_SPEED)
        Settings.uiSpeed = MIN_SPEED;
    else
    if (Settings.uiSpeed > MAX_SPEED)
        Settings.uiSpeed = MAX_SPEED;

    // Check type of rotation to be in range
    if (Settings.uiRotation != ROTATION_NONE &&
        Settings.uiRotation != ROTATION_LINEAR &&
        Settings.uiRotation != ROTATION_PERIODIC)
        Settings.uiRotation = ROTATION_PERIODIC;

    RegCloseKey(hKey);
}

void SaveSettings(void)
{
    HKEY  hKey;
    LONG  lRes;

    lRes = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            _T("Software\\Microsoft\\ScreenSavers\\Ssstars"),
            0,
            _T(""),
            0,
            KEY_WRITE,
            NULL,
            &hKey,
            NULL);

    if (lRes != ERROR_SUCCESS)
        return;

    SaveDWORD(hKey, _T("NumberOfStars"),  Settings.uiNumStars);
    SaveDWORD(hKey, _T("Speed"),          Settings.uiSpeed);
    SaveDWORD(hKey, _T("TypeOfRotation"), Settings.uiRotation);

    SaveDWORD(hKey, _T("DoBlending"),      Settings.bDoBlending);
    SaveDWORD(hKey, _T("FinePerspective"), Settings.bFinePerspective);
    SaveDWORD(hKey, _T("EnableFiltering"), Settings.bEnableFiltering);
    SaveDWORD(hKey, _T("SmoothShading"),   Settings.bSmoothShading);

    RegCloseKey(hKey);
}

static void SetupControls(HWND hWnd)
{
    TCHAR     Strings[256];
    HINSTANCE hInstance;
    UINT      x, gap;
    LOGFONT   lf;
    HFONT     hFont;
    HBITMAP   hCosmos;
    HDC       hDC, hMemDC;
    HGDIOBJ   hOldBmp, hOldFnt;
    SIZE      sizeReactOS;
    SIZE      sizeStarfield;
    BITMAP    bm;

    hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    SendDlgItemMessage(hWnd, IDC_SLIDER_NUM_OF_STARS, TBM_SETRANGE, FALSE, MAKELPARAM(MIN_STARS, MAX_STARS));

    SendDlgItemMessage(hWnd, IDC_SLIDER_SPEED, TBM_SETRANGE, FALSE, MAKELPARAM(1, 100));

    for (x = 0; x < ROTATION_ITEMS; x++)
    {
        LoadString(hInstance, RotoStrings[x], Strings, sizeof(Strings)/sizeof(TCHAR));
        SendDlgItemMessage(hWnd, IDC_COMBO_ROTATION, CB_ADDSTRING, 0, (LPARAM)Strings);
    }

    hCosmos = LoadImage(hInstance, MAKEINTRESOURCE(IDB_COSMOS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);

    hDC = GetDC(hWnd);
    hMemDC = CreateCompatibleDC(hDC);

    // Create the font for the title
    ZeroMemory(&lf, sizeof(lf));

    lf.lfWeight  = FW_THIN;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfHeight  = 36;
    _tcscpy(lf.lfFaceName, _T("Tahoma"));

    hFont = CreateFontIndirect(&lf);

    hOldBmp = SelectObject(hMemDC, hCosmos);
    hOldFnt = SelectObject(hMemDC, hFont);

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(0xFF, 0xFF, 0xFF));

    x = LoadString(hInstance, IDS_DESCRIPTION, Strings, sizeof(Strings)/sizeof(TCHAR));

    GetTextExtentPoint32(hMemDC, _T("ReactOS"), 7, &sizeReactOS);
    GetTextExtentPoint32(hMemDC, Strings,       x, &sizeStarfield);

    GetObject(hCosmos, sizeof(BITMAP), &bm);

    gap = bm.bmHeight - sizeReactOS.cy - sizeStarfield.cy;

    TextOut(hMemDC, 16, gap * 2 / 5, _T("ReactOS"), 7);
    TextOut(hMemDC, 16, gap * 3 / 5 + sizeReactOS.cy, Strings, x);

    SelectObject(hMemDC, hOldBmp);
    SelectObject(hMemDC, hOldFnt);

    DeleteObject(hFont);

    DeleteDC(hMemDC);
    ReleaseDC(hWnd, hDC);

    SendDlgItemMessage(hWnd, IDC_IMAGE_COSMOS, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hCosmos);
}

static void ApplySettings(HWND hWnd)
{
    SendDlgItemMessage(hWnd, IDC_SLIDER_NUM_OF_STARS, TBM_SETPOS, TRUE, Settings.uiNumStars);
    SetDlgItemInt(hWnd, IDC_TEXT_NUM_OF_STARS, Settings.uiNumStars, FALSE);

    SendDlgItemMessage(hWnd, IDC_SLIDER_SPEED, TBM_SETPOS, TRUE, Settings.uiSpeed);
    SetDlgItemInt(hWnd, IDC_TEXT_SPEED, Settings.uiSpeed, FALSE);

    SendDlgItemMessage(hWnd, IDC_COMBO_ROTATION, CB_SETCURSEL, (WPARAM)Settings.uiRotation, 0);

    SendDlgItemMessage(hWnd, IDC_CHECK_DOBLENDING,  BM_SETCHECK, (WPARAM)Settings.bDoBlending, 0);
    SendDlgItemMessage(hWnd, IDC_CHECK_PERSPECTIVE, BM_SETCHECK, (WPARAM)Settings.bFinePerspective, 0);
    SendDlgItemMessage(hWnd, IDC_CHECK_FILTERING,   BM_SETCHECK, (WPARAM)Settings.bEnableFiltering, 0);
    SendDlgItemMessage(hWnd, IDC_CHECK_SHADING,     BM_SETCHECK, (WPARAM)Settings.bSmoothShading, 0);
}

static void ReadSettings(HWND hWnd)
{
    Settings.uiNumStars = SendDlgItemMessage(hWnd, IDC_SLIDER_NUM_OF_STARS, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_NUM_OF_STARS, Settings.uiNumStars, FALSE);

    Settings.uiSpeed = SendDlgItemMessage(hWnd, IDC_SLIDER_SPEED, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_SPEED, Settings.uiSpeed, FALSE);

    Settings.uiRotation = SendDlgItemMessage(hWnd, IDC_COMBO_ROTATION, CB_GETCURSEL, 0, 0);

    Settings.bDoBlending      = SendDlgItemMessage(hWnd, IDC_CHECK_DOBLENDING,  BM_GETCHECK, 0, 0);
    Settings.bFinePerspective = SendDlgItemMessage(hWnd, IDC_CHECK_PERSPECTIVE, BM_GETCHECK, 0, 0);
    Settings.bEnableFiltering = SendDlgItemMessage(hWnd, IDC_CHECK_FILTERING,   BM_GETCHECK, 0, 0);
    Settings.bSmoothShading   = SendDlgItemMessage(hWnd, IDC_CHECK_SHADING,     BM_GETCHECK, 0, 0);
}

static BOOL OnCommandAbout(HWND hWnd)
{
    HINSTANCE hInstance;
    HICON     hIcon;
    TCHAR     szAppName[256];
    TCHAR     szAuthor[256];
    TCHAR     szLicense[1024];

    hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STARFIELD));

    LoadString(hInstance, IDS_DESCRIPTION, szAppName, SIZEOF(szAppName));
    LoadString(hInstance, IDS_AUTHOR,      szAuthor,  SIZEOF(szAuthor));
    LoadString(hInstance, IDS_LICENSE,     szLicense, SIZEOF(szLicense));

    _tcscat(szAppName, _T("#"));
    _tcscat(szAppName, szAuthor);

    ShellAbout(hWnd, szAppName, szLicense, hIcon);

    return TRUE;
}

//
// Dialogbox procedure for Configuration window
//
BOOL CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        LoadSettings();
        SetupControls(hDlg);
        ApplySettings(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD( wParam )) {

        case IDOK:
            // Write configuration
            SaveSettings();

            // Fall down...
        case IDCANCEL:
            EndDialog( hDlg, LOWORD( wParam ));
            return TRUE;

        case IDC_BUTTON_ABOUT:
            return OnCommandAbout(hDlg);
        }

    case WM_HSCROLL:
        ReadSettings(hDlg);
        return TRUE;

    }

    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    InitCommonControls();

    return TRUE;
}

