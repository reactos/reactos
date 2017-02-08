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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "regedit.h"

BOOL ProcessCmdLine(LPWSTR lpCmdLine);

/*******************************************************************************
 * Global Variables:
 */

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
HMENU hPopupMenus = 0;
UINT nClipboardFormat;
LPCWSTR strClipboardFormat = L"TODO: SET CORRECT FORMAT";

#define MAX_LOADSTRING  100
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szFrameClass[MAX_LOADSTRING];
WCHAR szChildClass[MAX_LOADSTRING];


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
    BOOL AclUiAvailable;
    HMENU hEditMenu;

    WNDCLASSEXW wcFrame;
    WNDCLASSEXW wcChild;
    ATOM hFrameWndClass;

    ZeroMemory(&wcFrame, sizeof(WNDCLASSEXW));
    wcFrame.cbSize = sizeof(WNDCLASSEXW);
    wcFrame.lpfnWndProc = FrameWndProc;
    wcFrame.hInstance = hInstance;
    wcFrame.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT));
    wcFrame.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT),
                                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wcFrame.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcFrame.lpszClassName = szFrameClass;

    hFrameWndClass = RegisterClassExW(&wcFrame); /* register frame window class */

    ZeroMemory(&wcChild, sizeof(WNDCLASSEXW));
    wcChild.cbSize = sizeof(WNDCLASSEXW);
    wcChild.lpfnWndProc = ChildWndProc;
    wcChild.cbWndExtra = sizeof(HANDLE);
    wcChild.hInstance = hInstance;
    wcChild.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT));
    wcChild.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcChild.lpszClassName = szChildClass;
    wcChild.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT),
                                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON), LR_SHARED);

    RegisterClassExW(&wcChild); /* register child windows class */

    RegisterHexEditorClass(hInstance);

    hMenuFrame = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDR_REGEDIT_MENU));
    hPopupMenus = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDR_POPUP_MENUS));

    /* Initialize the Windows Common Controls DLL */
    // TODO: Replace this call by InitCommonControlsEx(_something_)
    InitCommonControls();

    hEditMenu = GetSubMenu(hMenuFrame, 1);

    AclUiAvailable = InitializeAclUiDll();
    if(!AclUiAvailable)
    {
        /* hide the Edit/Permissions... menu entry */
        if(hEditMenu != NULL)
        {
            RemoveMenu(hEditMenu, ID_EDIT_PERMISSIONS, MF_BYCOMMAND);
            /* remove the separator after the menu item */
            RemoveMenu(hEditMenu, 4, MF_BYPOSITION);
        }
    }

    if(hEditMenu != NULL)
        SetMenuDefaultItem(hEditMenu, ID_EDIT_MODIFY, MF_BYCOMMAND);

    nClipboardFormat = RegisterClipboardFormatW(strClipboardFormat);
    /* if (nClipboardFormat == 0) {
        DWORD dwError = GetLastError();
    } */

    hFrameWnd = CreateWindowExW(WS_EX_WINDOWEDGE, (LPCWSTR)(UlongToPtr(hFrameWndClass)), szTitle,
                                WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, hMenuFrame, hInstance, NULL/*lpParam*/);

    if (!hFrameWnd)
    {
        return FALSE;
    }

    /* Create the status bar */
    hStatusBar = CreateStatusWindowW(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBT_NOBORDERS,
                                     L"", hFrameWnd, STATUS_WINDOW);
    if (hStatusBar)
    {
        /* Create the status bar panes */
        SetupStatusBar(hFrameWnd, FALSE);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
    }

    LoadSettings();
    UpdateWindow(hFrameWnd);
    return TRUE;
}

/******************************************************************************/

/*
 * We need to destroy the main menu before destroying the main window
 * to avoid a memory leak.
 */

void DestroyMainMenu()
{
    DestroyMenu(hMenuFrame);
}

/******************************************************************************/

void ExitInstance(HINSTANCE hInstance)
{
    UnregisterHexEditorClass(hInstance);

    DestroyMenu(hPopupMenus);
    UnloadAclUiDll();
}

BOOL TranslateChildTabMessage(PMSG msg)
{
    if (msg->message != WM_KEYDOWN) return FALSE;

    /* Allow Ctrl+A on address bar */
    if ((msg->hwnd == g_pChildWnd->hAddressBarWnd) &&
        (msg->message == WM_KEYDOWN) &&
        (msg->wParam == L'A') && (GetKeyState(VK_CONTROL) < 0))
    {
        SendMessageW(msg->hwnd, EM_SETSEL, 0, -1);
        return TRUE;
    }

    if (msg->wParam != VK_TAB) return FALSE;
    if (GetParent(msg->hwnd) != g_pChildWnd->hWnd) return FALSE;
    PostMessageW(g_pChildWnd->hWnd, WM_COMMAND, ID_SWITCH_PANELS, 0);
    return TRUE;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    UNREFERENCED_PARAMETER(hPrevInstance);

    /* Initialize global strings */
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));
    LoadStringW(hInstance, IDC_REGEDIT_FRAME, szFrameClass, COUNT_OF(szFrameClass));
    LoadStringW(hInstance, IDC_REGEDIT, szChildClass, COUNT_OF(szChildClass));

    if (ProcessCmdLine(lpCmdLine))
    {
        return 0;
    }

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }
    /* Store instance handle in our global variable */
    hInst = hInstance;

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow))
    {
        return 0;
    }
    hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(ID_ACCEL));

    /* Main message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hFrameWnd, hAccel, &msg) &&
            !TranslateChildTabMessage(&msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    ExitInstance(hInstance);
    return (int)msg.wParam;
}

/* EOF */
