/*
 *  ReactOS regedit
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
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;

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
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szFrameClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class

    WNDCLASSEX wcChild = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        ChildWndProc,
        0/*cbClsExtra*/,
        sizeof(HANDLE)/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szChildClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)

    };
    ATOM hChildWndClass = RegisterClassEx(&wcChild); // register child windows class

/*
    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)FrameWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_REGEDIT);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)SS_BLACKRECT;
//    wcex.lpszMenuName   = (LPCSTR)IDC_REGEDIT;
    wcex.lpszMenuName   = (LPCSTR)IDR_REGEDIT_MENU;
    wcex.lpszClassName  = szFrameClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);
    RegisterClassEx(&wcex);
 */

	hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_REGEDIT_MENU));

    // Initialize the Windows Common Controls DLL
    InitCommonControls();

    hInst = hInstance; // Store instance handle in our global variable
//    hFrameWnd = CreateWindow(szFrameClass, szTitle, WS_OVERLAPPEDWINDOW,
//                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, _T("regedit application"),
                    WS_OVERLAPPEDWINDOW,
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
        int nParts[3];
        nParts[0] = 100;
        nParts[1] = 210;
        nParts[2] = 400;
        SendMessage(hStatusBar, SB_SETPARTS, 3, (long)nParts);
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
    HACCEL hAccel;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDIT_FRAME, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDIT, szChildClass, MAX_LOADSTRING);
    
    // Store instance handle in our global variable
    hInst = hInstance;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_REGEDIT);

    // Main message loop:
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    ExitInstance();
    return msg.wParam;
}
