/*
 *  ReactOS Application
 *
 *  main.c
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "main.h"
#include "framewnd.h"
#include "childwnd.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HINSTANCE hInst;
HACCEL    hAccel;
HWND      hFrameWnd;
HWND      hMDIClient;
HMENU     hMenuFrame;
HWND      hStatusBar;
HWND      hToolBar;
HFONT     hFont;

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szFrameClass[MAX_LOADSTRING];
TCHAR szChildClass[MAX_LOADSTRING];


////////////////////////////////////////////////////////////////////////////////
//
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
//    ChildWnd* pChildWnd;

    WNDCLASSEX wcFrame = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        FrameWndProc,
        0/*cbClsExtra*/,
        0/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MDI_APP)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szFrameClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MDI_APP), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class
#if 0
    WNDCLASS wcChild = {
        CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW,
        ChildWndProc,
        0/*cbClsExtra*/,
        0/*cbWndExtra*/,
        hInstance,
        0/*hIcon*/,
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szChildClass
    };
    ATOM hChildWndClass = RegisterClass(&wcChild); // register child windows class
#else
    WNDCLASSEX wcChild = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        ChildWndProc,
        0/*cbClsExtra*/,
        sizeof(HANDLE)/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MDI_APP)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szChildClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDC_MDI_APP), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)

    };
    ATOM hChildWndClass = RegisterClassEx(&wcChild); // register child windows class
#endif

    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_MDI_APP));
    HMENU hMenuOptions = GetSubMenu(hMenu, ID_OPTIONS_MENU);
    HMENU hChildMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_MDI_APP_CHILD));

    INITCOMMONCONTROLSEX icc = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_BAR_CLASSES
    };

    HDC hdc = GetDC(0);

    hMenuFrame = hMenu;
//  hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);
    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MDI_APP));
    hFont = CreateFont(-MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("MS Sans Serif"));
    ReleaseDC(0, hdc);

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
//  hFrameWnd = CreateWindow(szFrameClass, szTitle,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL/*hWndParent*/, hMenuFrame, hInstance, NULL/*lpParam*/);
    if (!hFrameWnd) {
        return FALSE;
    }

    if (InitCommonControlsEx(&icc))
    {
        int nParts[3];
        TBBUTTON toolbarBtns[] = {
            {0, 0, 0, TBSTYLE_SEP},
            {0, ID_WINDOW_NEW_WINDOW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
            {1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
            {2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, TBSTYLE_BUTTON},
            {3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
            {4, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
            {5, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        };

        hToolBar = CreateToolbarEx(hFrameWnd, WS_CHILD|WS_VISIBLE,
            IDC_TOOLBAR, 2, hInstance, IDB_TOOLBAR, toolbarBtns,
            sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));
        CheckMenuItem(hMenuOptions, ID_OPTIONS_TOOLBAR, MF_BYCOMMAND|MF_CHECKED);

        // Create the status bar
        hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
                                        "", hFrameWnd, IDC_STATUSBAR);
        if (!hStatusBar)
            return FALSE;
        CheckMenuItem(hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

        // Create the status bar panes
        nParts[0] = 100;
        nParts[1] = 210;
        nParts[2] = 400;
        SendMessage(hStatusBar, SB_SETPARTS, 3, (long)nParts);
    } else {
        CheckMenuItem(hMenuOptions, ID_OPTIONS_TOOLBAR, MF_BYCOMMAND|MF_GRAYED);
        CheckMenuItem(hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_GRAYED);
	}

    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    UpdateStatusBar();
    return TRUE;
}

void UpdateStatusBar(void)
{
    TCHAR text[260];
    DWORD size;

    size = sizeof(text)/sizeof(TCHAR);
    GetUserName(text, &size);
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)text);
    size = sizeof(text)/sizeof(TCHAR);
    GetComputerName(text, &size);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
}


static int g_foundPrevInstance = 0;

// search for already running win[e]files
static BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam)
{
    TCHAR cls[128];

    GetClassName(hWnd, cls, 128);
    if (!lstrcmp(cls, (LPCTSTR)lParam)) {
        g_foundPrevInstance++;
        return FALSE;
    }
    return TRUE;
}


void ExitInstance(void)
{
    DestroyMenu(hMenuFrame);
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;
    HWND hMDIClient;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MDI_APP, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MDI_APP_CHILD, szChildClass, MAX_LOADSTRING);
    
    // Allow only one running instance
    EnumWindows(EnumWndProc, (LPARAM)szFrameClass);
    if (g_foundPrevInstance)
        return 1;

    // Store instance handle in our global variable
    hInst = hInstance;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_MDI_APP);
    hMDIClient = GetWindow(hFrameWnd, GW_CHILD);

    // Main message loop:
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) { 
        if (!TranslateMDISysAccel(hMDIClient, &msg) && 
            !TranslateAccelerator(hFrameWnd/*hwndFrame*/, hAccel, &msg)) { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
    } 
    ExitInstance();
    return msg.wParam;
}
