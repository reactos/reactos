/*
 *  ReactOS regedt32
 *
 *  regedt32.cpp
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
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
	
#include "resource.h"
#include <shellapi.h>
//#include <winspool.h>

#define STATUS_WINDOW   2001
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                            // current instance

HWND hMainWnd;                   // Main Window
HWND hStatusWnd;                 // Status Bar Window
HWND hTabWnd;                    // Tab Control Window

int  nMinimumWidth;              // Minimum width of the dialog (OnSize()'s cx)
int  nMinimumHeight;             // Minimum height of the dialog (OnSize()'s cy)

int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

TCHAR szTitle[MAX_LOADSTRING];              // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];        // The title bar text
TCHAR szFrameClass[MAX_LOADSTRING];         // The title bar text

// Foward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
ATOM                MyRegisterClass2(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FrameWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDT32, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDT32_FRAME, szFrameClass, MAX_LOADSTRING);
    
    MyRegisterClass(hInstance);
    MyRegisterClass2(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_REGEDT32);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_REGEDT32);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)SS_BLACKRECT/*(COLOR_WINDOW+1)*/;
//  wcex.lpszMenuName   = (LPCSTR)IDC_REGEDT32;
//  wcex.lpszMenuName   = (LPCSTR)IDR_REGEDIT_MENU;
    wcex.lpszMenuName   = (LPCSTR)IDR_REGEDT32_MENU;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);
    return RegisterClassEx(&wcex);
}

ATOM MyRegisterClass2(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)FrameWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_REGEDT32);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = (LPCSTR)IDR_REGEDIT_MENU;
//  wcex.lpszMenuName   = (LPCSTR)IDR_REGEDT32_MENU;
    wcex.lpszClassName  = szFrameClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);
    return RegisterClassEx(&wcex);
}

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
    int nParts[3];

    // Initialize the Windows Common Controls DLL
    InitCommonControls();

    hInst = hInstance; // Store instance handle in our global variable
    hMainWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hMainWnd) {
        return FALSE;
    }

    // Get the minimum window sizes
//    GetWindowRect(hMainWnd, &rc);
//    nMinimumWidth = (rc.right - rc.left);
//    nMinimumHeight = (rc.bottom - rc.top);

    // Create the status bar
    hStatusWnd = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
		                            "", hMainWnd, STATUS_WINDOW);
    if (!hStatusWnd)
        return FALSE;

    // Create the status bar panes
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = 400;
    SendMessage(hStatusWnd, SB_SETPARTS, 3, (long)nParts);


    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    return TRUE;
}


// OnSize()
// This function handles all the sizing events for the application
// It re-sizes every window, and child window that needs re-sizing
void OnSize(UINT nType, int cx, int cy)
{
    int     nParts[3];
    int     nXDifference;
    int     nYDifference;
    RECT    rc;

    if (nType == SIZE_MINIMIZED)
        return;

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    // Update the status bar size
    GetWindowRect(hStatusWnd, &rc);
    SendMessage(hStatusWnd, WM_SIZE, nType, MAKELPARAM(cx, cy + (rc.bottom - rc.top)));

    // Update the status bar pane sizes
    nParts[0] = bInMenuLoop ? -1 : 100;
    nParts[1] = 210;
    nParts[2] = cx;
    SendMessage(hStatusWnd, SB_SETPARTS, bInMenuLoop ? 1 : 3, (long)nParts);
}

void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    // Update the status bar pane sizes
    nParts = -1;
    SendMessage(hStatusWnd, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

void OnExitMenuLoop(HWND hWnd)
{
    RECT  rc;
    int   nParts[3];
    TCHAR text[260];

    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(hStatusWnd, SB_SETPARTS, 3, (long)nParts);
    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)_T(""));
//    wsprintf(text, _T("CPU Usage: %3d%%"), PerfDataGetProcessorUsage());
//    SendMessage(hStatusWnd, SB_SETTEXT, 1, (LPARAM)text);
//    wsprintf(text, _T("Processes: %d"), PerfDataGetProcessCount());
//    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text);
}

void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    strcpy(str, TEXT(""));
    if (LoadString(hInst, nItemID, str, 100)) {
        // load appropriate string
        LPTSTR lpsz = str;
        // first newline terminates actual string
        lpsz = _tcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)str);
}


LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR szHello[MAX_LOADSTRING];
    LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

    switch (message) {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId) {
            case IDM_ABOUT:
//            DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				{
                HICON hIcon = LoadIcon(hInst, (LPCTSTR)IDI_REGEDT32);
                ShellAbout(hWnd, szTitle, "FrameWndProc", hIcon);
                //if (hIcon) DestroyIcon(hIcon); // NOT REQUIRED
                }
            break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        RECT rt;
        GetClientRect(hWnd, &rt);
        DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    //TCHAR szHello[MAX_LOADSTRING];
    //LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

    switch (message) {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId) {
        case ID_REGISTRY_PRINTERSETUP:
            //PRINTDLG pd;
            //PrintDlg(&pd);
            //PAGESETUPDLG psd;
            //PageSetupDlg(&psd);
            break;
        case ID_REGISTRY_OPENLOCAL:
			{
            HWND hChildWnd;
//            hChildWnd = CreateWindow(szFrameClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
//                                   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, NULL, hInst, NULL);
            hChildWnd = CreateWindow(szFrameClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
                                     0, 0, 150, 170, hWnd, NULL, hInst, NULL);
			if (hChildWnd) {
				ShowWindow(hChildWnd, 1);
				UpdateWindow(hChildWnd);
			}
            }
            break;
        case IDM_ABOUT:
//            DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
            {
            HICON hIcon = LoadIcon(hInst, (LPCTSTR)IDI_REGEDT32);
            ShellAbout(hWnd, szTitle, "", hIcon);
            //if (hIcon) DestroyIcon(hIcon); // NOT REQUIRED
            }
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_SIZE:
        // Handle the window sizing in it's own function
        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        RECT rt;
        GetClientRect(hWnd, &rt);
        //DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER:
        break;

    case WM_ENTERMENULOOP:
        OnEnterMenuLoop(hWnd);
        break;
    case WM_EXITMENULOOP:
        OnExitMenuLoop(hWnd);
        break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}
