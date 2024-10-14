/*
 * Regedit main function
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

BOOL ProcessCmdLine(WCHAR *cmdline);

const WCHAR *reg_class_namesW[] = {L"HKEY_LOCAL_MACHINE", L"HKEY_USERS",
                                   L"HKEY_CLASSES_ROOT", L"HKEY_CURRENT_CONFIG",
                                   L"HKEY_CURRENT_USER", L"HKEY_DYN_DATA"
                                  };

/*******************************************************************************
 * Global Variables:
 */

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
HMENU hPopupMenus;
UINT nClipboardFormat;
LPCWSTR strClipboardFormat = L"TODO: SET CORRECT FORMAT";

#define MAX_LOADSTRING  100
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szFrameClass[MAX_LOADSTRING];
WCHAR szChildClass[MAX_LOADSTRING];


/**
 * PURPOSE: Saves instance handle and creates main window
 *
 * COMMENTS:
 *   In this function, we save the instance handle in a global variable and
 *   create and display the main program window.
 */
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    BOOL AclUiAvailable;
    HMENU hEditMenu;
    INITCOMMONCONTROLSEX icce;
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
    wcFrame.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcFrame.lpszClassName = szFrameClass;

    hFrameWndClass = RegisterClassExW(&wcFrame); /* register frame window class */

    ZeroMemory(&wcChild, sizeof(WNDCLASSEXW));
    wcChild.cbSize = sizeof(WNDCLASSEXW);
    wcChild.lpfnWndProc = ChildWndProc;
    wcChild.cbWndExtra = sizeof(HANDLE);
    wcChild.hInstance = hInstance;
    wcChild.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT));
    wcChild.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcChild.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcChild.lpszClassName = szChildClass;
    wcChild.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_REGEDIT),
                                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON), LR_SHARED);

    RegisterClassExW(&wcChild); /* register child windows class */

    RegisterHexEditorClass(hInstance);

    hMenuFrame = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDR_REGEDIT_MENU));
    hPopupMenus = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDR_POPUP_MENUS));

    /* Initialize the Windows Common Controls DLL */
    /* NOTE: Windows sets 0xFFFF to icce.dwICC but we use better value. */
    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icce);

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
        return FALSE;

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

/*
 * We need to destroy the main menu before destroying the main window
 * to avoid a memory leak.
 */
void DestroyMainMenu()
{
    DestroyMenu(hMenuFrame);
}

void ExitInstance(HINSTANCE hInstance)
{
    UnregisterHexEditorClass(hInstance);

    DestroyMenu(hPopupMenus);
    UnloadAclUiDll();
}

static BOOL InLabelEdit(HWND hWnd, UINT Msg)
{
    HWND hEdit = (HWND)SendMessageW(hWnd, Msg, 0, 0);
    return hEdit && IsWindowVisible(hEdit);
}

static BOOL TranslateChildTabMessage(PMSG msg)
{
    if (msg->message != WM_KEYDOWN) return FALSE;
    if (msg->wParam != VK_TAB) return FALSE;
    if (GetParent(msg->hwnd) != g_pChildWnd->hWnd) return FALSE;
    PostMessageW(hFrameWnd, WM_COMMAND, ID_SWITCH_PANELS, 0);
    return TRUE;
}

static BOOL TranslateRegeditAccelerator(HWND hWnd, HACCEL hAccTable, PMSG msg)
{
    if (msg->message == WM_KEYDOWN)
    {
        if (msg->wParam == VK_DELETE)
        {
            if (g_pChildWnd->hAddressBarWnd == msg->hwnd)
                return FALSE;
            if (InLabelEdit(g_pChildWnd->hTreeWnd, TVM_GETEDITCONTROL) || InLabelEdit(g_pChildWnd->hListWnd, LVM_GETEDITCONTROL))
                return FALSE;
        }
    }
    return TranslateAcceleratorW(hWnd, hAccTable, msg);
}

int WINAPI wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    UNREFERENCED_PARAMETER(hPrevInstance);

    OleInitialize(NULL);

    /* Initialize global strings */
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAY_SIZE(szTitle));
    LoadStringW(hInstance, IDC_REGEDIT_FRAME, szFrameClass, ARRAY_SIZE(szFrameClass));
    LoadStringW(hInstance, IDC_REGEDIT, szChildClass, ARRAY_SIZE(szChildClass));

    if (ProcessCmdLine(GetCommandLineW()))
        return 0;

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
        return 0;

    hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(ID_ACCEL));

    /* Main message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateRegeditAccelerator(hFrameWnd, hAccel, &msg) &&
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
