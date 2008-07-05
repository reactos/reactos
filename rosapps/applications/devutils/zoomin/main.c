/*
 *  ReactOS zoomin
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "framewnd.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HWND hFrameWnd;
HMENU hMenuFrame;

TCHAR szTitle[MAX_LOADSTRING];


////////////////////////////////////////////////////////////////////////////////
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: creates main window
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
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ZOOMIN)),
        LoadCursor(0, IDC_ARROW),
        0,//(HBRUSH)(COLOR_BTNFACE+1),
        0/*lpszMenuName*/,
        WNDCLASS_ZOOMIN,
        (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ZOOMIN), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class

	hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_ZOOMIN_MENU));

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)UlongToPtr(hFrameWndClass), szTitle,
                    WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE | WS_VSCROLL,
                    CW_USEDEFAULT, CW_USEDEFAULT, 250, 250,
                    NULL, hMenuFrame, hInstance, NULL/*lpParam*/);

    if (!hFrameWnd) {
        return FALSE;
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

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ZOOMIN));

    // Main message loop:
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}
