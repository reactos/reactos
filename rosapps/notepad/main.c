/*
 *  ReactOS notepad
 *
 *  main.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <commctrl.h>
    
#include "main.h"
#include "framewnd.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
UINT nClipboardFormat;
LPCTSTR strClipboardFormat = _T("TODO: SET CORRECT FORMAT");


TCHAR szTitle[MAX_LOADSTRING];
TCHAR szFrameClass[MAX_LOADSTRING];
TCHAR szChildClass[MAX_LOADSTRING];


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wcFrame = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        FrameWndProc,
        0/*cbClsExtra*/,
        0/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szFrameClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class
#if 0
    WNDCLASSEX wcChild = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        ChildWndProc,
        0/*cbClsExtra*/,
        sizeof(HANDLE)/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szChildClass,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_NOTEPAD), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)

    };
    ATOM hChildWndClass = RegisterClassEx(&wcChild); // register child windows class
    hChildWndClass = hChildWndClass; // warning eater
#endif
	hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_NOTEPAD_MENU));

    // Initialize the Windows Common Controls DLL
    InitCommonControls();

    nClipboardFormat = RegisterClipboardFormat(strClipboardFormat);
    if (nClipboardFormat == 0) {
        DWORD dwError = GetLastError();
    }

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                    WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE,
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
    HACCEL hAccel;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_NOTEPAD_FRAME, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_NOTEPAD, szChildClass, MAX_LOADSTRING);
    
    // Store instance handle in our global variable
    hInst = hInstance;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_NOTEPAD);

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
