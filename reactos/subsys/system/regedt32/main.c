/*
 *  ReactOS regedt32
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
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HINSTANCE hInst;
HACCEL    hAccel;
HWND      hFrameWnd;
HWND      hMDIClient;
HWND      hStatusBar;
HMENU     hMenuFrame;

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
    WNDCLASSEX wcFrame = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        FrameWndProc,
        0/*cbClsExtra*/,
        0/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDT32)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szFrameClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDT32), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class

    WNDCLASSEX wcChild = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE/*style*/,
        ChildWndProc,
        0/*cbClsExtra*/,
        sizeof(HANDLE)/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDT32)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szChildClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDT32), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)

    };
    ATOM hChildWndClass = RegisterClassEx(&wcChild); // register child windows class

    hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_REGEDT32_MENU));
//    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REGEDT32));

    // Initialize the Windows Common Controls DLL
    InitCommonControls();

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, hMenuFrame, hInstance, NULL/*lpParam*/);
    if (!hFrameWnd) {
        return FALSE;
    }

    // Create the status bar
    hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
                                    _T(""), hFrameWnd, STATUS_WINDOW);
    if (hStatusBar) {
        // Create the status bar panes
        SetupStatusBar(hFrameWnd, FALSE);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
    }
    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

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
//    HACCEL hAccel;
    HWND hMDIClient;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDT32_FRAME, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDT32, szChildClass, MAX_LOADSTRING);
    
    // Store instance handle in our global variable
    hInst = hInstance;

    // Load our settings from the registry
    LoadSettings();

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
//    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_REGEDT32);
    hMDIClient = GetWindow(hFrameWnd, GW_CHILD);

    // Main message loop:
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateMDISysAccel(hMDIClient, &msg) &&
            !TranslateAccelerator(hFrameWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    // Save our settings to the registry
    SaveSettings();
    ExitInstance();
    return msg.wParam;
}
