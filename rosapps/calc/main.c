/*
 *  ReactOS calc
 *
 *  calc.c
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
#include <tchar.h>

#include "main.h"
#include "button.h"
#include "settings.h"


// Global Variables:
HINSTANCE hInst;
HWND hDlgWnd;
TCHAR szTitle[MAX_LOADSTRING];
CALC_TYPES CalcType;
HACCEL hAccel;

BOOL bDigitGrouping = FALSE;


#define BEGIN_CMD_MAP(a) switch(##a) {
#define CMD_MAP_ENTRY(a, b) case a: b(); break;
#define END_CMD_MAP(a) }


BOOL OnCreate(HWND hWnd)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hDlgWnd);
    hViewMenu = GetSubMenu(hMenu, ID_MENU_VIEW);
    if (bDigitGrouping) {
        CheckMenuItem(hViewMenu, ID_VIEW_DIGIT_GROUPING, MF_BYCOMMAND|MF_CHECKED);
    }

    //SendMessage(hDlgWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_CALC)));

    // Initialize the Windows Common Controls DLL
    //InitCommonControls();

    return TRUE;
}

void OnEditCopy(void)
{
    SendMessage(GetDlgItem(hDlgWnd, IDC_RESULT), WM_COMMAND, MAKELONG(ID_EDIT_COPY, 0), 0);
}

void OnEditPaste(void)
{
    SendMessage(GetDlgItem(hDlgWnd, IDC_RESULT), WM_COMMAND, MAKELONG(ID_EDIT_PASTE, 0), 0);
}

void OnViewStandard(void)
{
    CalcType = STANDARD;
    DestroyWindow(hDlgWnd);
}

void OnViewScientific(void)
{
    CalcType = SCIENTIFIC;
    DestroyWindow(hDlgWnd);
}

void OnViewDigitGrouping(void)
{
    HMENU hMenu = GetMenu(hDlgWnd);
    HMENU hViewMenu = GetSubMenu(hMenu, ID_MENU_VIEW);
    bDigitGrouping = !bDigitGrouping;
    if (bDigitGrouping) {
        CheckMenuItem(hViewMenu, ID_VIEW_DIGIT_GROUPING, MF_BYCOMMAND|MF_CHECKED);
    } else {
        CheckMenuItem(hViewMenu, ID_VIEW_DIGIT_GROUPING, MF_BYCOMMAND|MF_UNCHECKED);
    }
}

void OnViewHex(void)
{
}

void OnViewDecimal(void)
{
}

void OnViewOctal(void)
{
}

void OnViewBinary(void)
{
}

void OnViewDegrees(void)
{
}

void OnViewRadians(void)
{
}

void OnViewGrads(void)
{
}

void OnHelpTopics(void)
{
    WinHelp(hDlgWnd, _T("calc"), HELP_CONTENTS, 0);
}

void OnHelpAbout(void)
{
}

// Message handler for dialog box.
LRESULT CALLBACK CalcWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        hDlgWnd = hDlg;
        InitColours();
        return OnCreate(hDlg);

    case WM_DRAWITEM: 
        DrawItem((LPDRAWITEMSTRUCT)lParam);
        return TRUE; 

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        if (HIWORD(wParam) == BN_CLICKED) { 
            switch (LOWORD(wParam)) { 
//            case IDC_OWNERDRAW: 
//                 // application-defined processing 
//                 break; 
            } 
        } 
            
        BEGIN_CMD_MAP(LOWORD(wParam))
            CMD_MAP_ENTRY(ID_EDIT_COPY,           OnEditCopy)
            CMD_MAP_ENTRY(ID_EDIT_PASTE,          OnEditPaste)
            CMD_MAP_ENTRY(ID_VIEW_STANDARD,       OnViewStandard)
            CMD_MAP_ENTRY(ID_VIEW_SCIENTIFIC,     OnViewScientific)
            CMD_MAP_ENTRY(ID_VIEW_DIGIT_GROUPING, OnViewDigitGrouping)
            CMD_MAP_ENTRY(ID_VIEW_HEX,            OnViewHex)
            CMD_MAP_ENTRY(ID_VIEW_DECIMAL,        OnViewDecimal)
            CMD_MAP_ENTRY(ID_VIEW_OCTAL,          OnViewOctal)
            CMD_MAP_ENTRY(ID_VIEW_BINARY,         OnViewBinary)
            CMD_MAP_ENTRY(ID_VIEW_DEGREES,        OnViewDegrees)
            CMD_MAP_ENTRY(ID_VIEW_RADIANS,        OnViewRadians)
            CMD_MAP_ENTRY(ID_VIEW_GRADS,          OnViewGrads)
            CMD_MAP_ENTRY(ID_HELP_TOPICS,         OnHelpTopics)
            CMD_MAP_ENTRY(ID_HELP_ABOUT,          OnHelpAbout)
        END_CMD_MAP(0)
        break;
/*
    case WM_KEYUP:
    case WM_KEYDOWN:
        {
        MSG msg;
        msg.hwnd = hDlg;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
//        msg.time; 
//        msg.pt; 
        if (TranslateAccelerator(hDlg, hAccel, &msg)) return 0;
        }
        break;
 */
    case WM_NOTIFY:
        {
        int     idctrl;
        LPNMHDR pnmh;
        idctrl = (int)wParam;
        pnmh = (LPNMHDR)lParam;
/*
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB) &&
            (pnmh->code == TCN_SELCHANGE))
        {
            _OnTabWndSelChange();
        }
 */
        }
        break;

     case WM_DESTROY:
        WinHelp(hDlgWnd, _T("regedit"), HELP_QUIT, 0);
        return DefWindowProc(hDlg, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    CALC_TYPES CurrentCalcType;

    // Initialize global variables
    hInst = hInstance;
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    // Load our settings from the registry
    LoadSettings();

    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDR_ACCELERATOR);

    do {
        CurrentCalcType = CalcType;
        switch (CalcType) {
        case HP42S:
            break;
        case SCIENTIFIC:
            DialogBox(hInst, (LPCTSTR)IDD_SCIENTIFIC, NULL, (DLGPROC)CalcWndProc);
            break;
        case STANDARD:
        default:
            DialogBox(hInst, (LPCTSTR)IDD_STANDARD, NULL, (DLGPROC)CalcWndProc);
            break;
        }
    } while (CalcType != CurrentCalcType);
 
    // Save our settings to the registry
    SaveSettings();
    return 0;
}

