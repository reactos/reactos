/*
 * Regedit main function
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include <fcntl.h>

#define REGEDIT_DECLARE_FUNCTIONS
#include "main.h"


BOOL ProcessCmdLine(LPSTR lpCmdLine);


/*******************************************************************************
 * Global Variables:
 */

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
UINT nClipboardFormat;
LPCTSTR strClipboardFormat = _T("TODO: SET CORRECT FORMAT");


TCHAR szTitle[MAX_LOADSTRING];
TCHAR szFrameClass[MAX_LOADSTRING];
TCHAR szChildClass[MAX_LOADSTRING];


/*******************************************************************************
 *
 *   FUNCTION: DynamicBind( void )
 *
 *   PURPOSE: Binds all functions dependent on user32.dll
 */
static BOOL DynamicBind( void )
{
    HMODULE dll;

#define d(x)                                                             \
    p##x = (typeof (x) ) GetProcAddress( dll, #x );                      \
    if( ! p##x )                                                         \
    {                                                                    \
        fprintf(stderr,"failed to bind function at line %d\n",__LINE__); \
        return FALSE;                                                    \
    }                                                                    \


    dll = LoadLibrary("user32");
    if( !dll )
        return FALSE;

    d(BeginDeferWindowPos)
    d(BeginPaint)
    d(CallWindowProcA)
    d(CheckMenuItem)
    d(CloseClipboard)
    d(CreateWindowExA)
    d(DefWindowProcA)
    d(DeferWindowPos)
    d(DestroyMenu)
    d(DestroyWindow)
    d(DialogBoxParamA)
    d(DispatchMessageA)
    d(EmptyClipboard)
    d(EndDeferWindowPos)
    d(EndDialog)
    d(EndPaint)
    d(FillRect)
    d(GetCapture)
    d(GetClientRect)
    d(GetCursorPos)
    d(GetDC)
    d(GetDlgItem)
    d(GetMenu)
    d(GetMessageA)
    d(GetSubMenu)
    d(GetSystemMetrics)
    d(InvertRect)
    d(IsWindowVisible)
    d(LoadAcceleratorsA)
    d(LoadBitmapA)
    d(LoadCursorA)
    d(LoadIconA)
    d(LoadImageA)
    d(LoadMenuA)
    d(LoadStringA)
    d(MessageBeep)
    d(MoveWindow)
    d(OpenClipboard)
    d(PostQuitMessage)
    d(RegisterClassExA)
    d(RegisterClipboardFormatA)
    d(ReleaseCapture)
    d(ReleaseDC)
    d(ScreenToClient)
    d(SendMessageA)
    d(SetCapture)
    d(SetCursor)
    d(SetFocus)
    d(SetWindowLongA)
    d(SetWindowTextA)
    d(ShowWindow)
    d(TranslateAccelerator)
    d(TranslateMessage)
    d(UpdateWindow)
    d(WinHelpA)
    d(wsprintfA)

    dll = LoadLibrary("gdi32");
    if( !dll )
        return FALSE;

    d(DeleteDC)
    d(DeleteObject)
    d(GetStockObject)

    dll = LoadLibrary("comctl32");
    if( !dll )
        return FALSE;

    d(CreateStatusWindowA)
    d(ImageList_Add)
    d(ImageList_Create)
    d(ImageList_GetImageCount)
    d(InitCommonControls)

    dll = LoadLibrary("comdlg32");
    if( !dll )
        return FALSE;

    d(CommDlgExtendedError)
    d(GetOpenFileNameA)
    d(GetSaveFileNameA)
    d(PrintDlgA)

    return TRUE;
}

/*******************************************************************************
 *
 *
 *   FUNCTION: InitInstance(HANDLE, int)
 *
 *   PURPOSE: Saves instance handle and creates main window
 *
 *   COMMENTS:
 *
 *        In this function, we save the instance handle in a global variable and
 *        create and display the main program window.
 */

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
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); /* register frame window class */

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
    ATOM hChildWndClass = RegisterClassEx(&wcChild); /* register child windows class */
    hChildWndClass = hChildWndClass; /* warning eater */

	hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_REGEDIT_MENU));

    /* Initialize the Windows Common Controls DLL */
    InitCommonControls();

    nClipboardFormat = RegisterClipboardFormat(strClipboardFormat);
    /* if (nClipboardFormat == 0) {
        DWORD dwError = GetLastError();
    } */

    hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, hMenuFrame, hInstance, NULL/*lpParam*/);

    if (!hFrameWnd) {
        return FALSE;
    }

    /* Create the status bar */
    hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS,
                                    _T(""), hFrameWnd, STATUS_WINDOW);
    if (hStatusBar) {
        /* Create the status bar panes */
        SetupStatusBar(hFrameWnd, FALSE);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
    }
    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    return TRUE;
}

/******************************************************************************/

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
/*
    int hCrt;
    FILE *hf;
    AllocConsole();
    hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    hf = _fdopen(hCrt, "w");
    *stdout = *hf;
    setvbuf(stdout, NULL, _IONBF, 0);

	wprintf(L"command line exit, hInstance = %d\n", hInstance);
	getch();
	FreeConsole();
    return 0;
 */

    if (ProcessCmdLine(lpCmdLine)) {
        return 0;
    }

    if (!DynamicBind()) {
        return 0;
    }

    /* Initialize global strings */
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDIT_FRAME, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REGEDIT, szChildClass, MAX_LOADSTRING);

    /* Store instance handle in our global variable */
    hInst = hInstance;

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_REGEDIT);

    /* Main message loop */
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    ExitInstance();
    return msg.wParam;
}
