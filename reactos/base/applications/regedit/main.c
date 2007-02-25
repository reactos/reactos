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

#include <regedit.h>

BOOL ProcessCmdLine(LPSTR lpCmdLine);


/*******************************************************************************
 * Global Variables:
 */

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;
HMENU hPopupMenus = 0;
UINT nClipboardFormat;
LPCTSTR strClipboardFormat = _T("TODO: SET CORRECT FORMAT");
const TCHAR g_szGeneralRegKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit");


#define MAX_LOADSTRING  100
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szFrameClass[MAX_LOADSTRING];
TCHAR szChildClass[MAX_LOADSTRING];


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
    TCHAR szBuffer[256];

    WNDCLASSEX wcFrame;
    WNDCLASSEX wcChild;
    ATOM hFrameWndClass;
    ATOM hChildWndClass;

    ZeroMemory(&wcFrame, sizeof(WNDCLASSEX));
    wcFrame.cbSize = sizeof(WNDCLASSEX);
    wcFrame.style = CS_HREDRAW | CS_VREDRAW;
    wcFrame.lpfnWndProc = FrameWndProc;
    wcFrame.hInstance = hInstance;
    wcFrame.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT));
    wcFrame.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT),
                                       IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                       GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wcFrame.hCursor = LoadCursor(0, IDC_ARROW); 
    wcFrame.lpszClassName = szFrameClass;

    hFrameWndClass = RegisterClassEx(&wcFrame); /* register frame window class */

    ZeroMemory(&wcChild, sizeof(WNDCLASSEX));
    wcChild.cbSize = sizeof(WNDCLASSEX);
    wcChild.style = CS_HREDRAW | CS_VREDRAW;
    wcChild.lpfnWndProc = ChildWndProc;
    wcChild.cbWndExtra = sizeof(HANDLE);
    wcChild.hInstance = hInstance;
    wcChild.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGEDIT));
    wcChild.hCursor = LoadCursor(0, IDC_ARROW),
    wcChild.lpszClassName =  szChildClass,
    wcChild.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REGEDIT), IMAGE_ICON,
                                              GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);

    hChildWndClass = RegisterClassEx(&wcChild); /* register child windows class */

    RegisterHexEditorClass(hInstance);

    hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_REGEDIT_MENU));
    hPopupMenus = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENUS));

    /* Initialize the Windows Common Controls DLL */
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

    nClipboardFormat = RegisterClipboardFormat(strClipboardFormat);
    /* if (nClipboardFormat == 0) {
        DWORD dwError = GetLastError();
    } */

    hFrameWnd = CreateWindowEx(WS_EX_WINDOWEDGE, (LPCTSTR)(UlongToPtr(hFrameWndClass)), szTitle,
                               WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
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

    /* Restore position */
    if (RegQueryStringValue(HKEY_CURRENT_USER, g_szGeneralRegKey,
        _T("LastKey"),
        szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])) == ERROR_SUCCESS)
    {
        SelectNode(g_pChildWnd->hTreeWnd, szBuffer);
    }

    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    return TRUE;
}

/******************************************************************************/

/* we need to destroy the main menu before destroying the main window
   to avoid a memory leak */

void DestroyMainMenu() {
	DestroyMenu(hMenuFrame);
}

/******************************************************************************/

void ExitInstance(HINSTANCE hInstance)
{
    UnregisterHexEditorClass(hInstance);
    
    DestroyMenu(hPopupMenus);
    UnloadAclUiDll();
}

BOOL TranslateChildTabMessage(MSG *msg)
{
  if (msg->message != WM_KEYDOWN) return FALSE;
  if (msg->wParam != VK_TAB) return FALSE;
  if (GetParent(msg->hwnd) != g_pChildWnd->hWnd) return FALSE;
  PostMessage(g_pChildWnd->hWnd, WM_COMMAND, ID_SWITCH_PANELS, 0);
  return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    UNREFERENCED_PARAMETER(hPrevInstance);

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
    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCEL));

    /* Main message loop */
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(hFrameWnd, hAccel, &msg)
            && !TranslateChildTabMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    ExitInstance(hInstance);
    return (int) msg.wParam;
}
