/*
 *  ReactOS winfile
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
#include "settings.h"
#include "framewnd.h"
#include "childwnd.h"
#include "drivebar.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//
UINT OemCodePage;
UINT AnsiCodePage;
LCID UserDefaultLCID;

HINSTANCE hInst;
#ifdef USE_GLOBAL_STRUCT
WINFILE_GLOBALS Globals;
#else
HINSTANCE   hInstance;
HACCEL	    hAccel;
HWND		hMainWnd;
HMENU		hMenuFrame;
HMENU		hWindowsMenu;
HMENU		hLanguageMenu;
HMENU		hMenuView;
HMENU		hMenuOptions;
HWND		hMDIClient;
HWND		hStatusBar;
HWND		hToolBar;
HWND		hDriveBar;
HFONT		hFont;
HWND        hDriveCombo;


TCHAR		num_sep;
SIZE		spaceSize;
HIMAGELIST  himl;

TCHAR		drives[BUFFER_LEN];
BOOL		prescan_node;	//TODO

LPCSTR	    lpszLanguage;
UINT		wStringTableOffset;
#endif

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
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINFILE)),
		LoadCursor(0, IDC_ARROW),
		0/*hbrBackground*/,
		0/*lpszMenuName*/,
		szFrameClass,
		(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_WINFILE), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
	};
	ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class

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

	ATOM hChildClass = RegisterClass(&wcChild); // register child windows class

	HMENU hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_WINFILE));
	HMENU hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);

	CLIENTCREATESTRUCT ccs = {
		hMenuWindow, IDW_FIRST_CHILD
	};

	INITCOMMONCONTROLSEX icc = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_BAR_CLASSES | ICC_USEREX_CLASSES
	};

//    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
//    icex.dwICC = ICC_USEREX_CLASSES;


//	TCHAR path[MAX_PATH];

	HDC hdc = GetDC(0);

//	hMenuFrame = hMenuFrame;
	Globals.hMenuView = GetSubMenu(hMenuFrame, 3);
	Globals.hMenuOptions = GetSubMenu(hMenuFrame, 4);
	Globals.hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINFILE));
	Globals.hFont = CreateFont(-MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("MS Sans Serif"));
	ReleaseDC(0, hdc);

	Globals.hMainWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
		            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					0/*hWndParent*/, hMenuFrame, hInstance, NULL/*lpParam*/);
    if (!Globals.hMainWnd) {
        return FALSE;
    }

    if (InitCommonControlsEx(&icc))	{
//		TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP};
		TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP};
//		int btn = 1;
//		PTSTR p;

        {
#define DRIVEBOX_WIDTH  200
#define DRIVEBOX_HEIGHT 8
/*
typedef struct _TBBUTTON {
    int iBitmap; 
    int idCommand; 
    BYTE fsState; 
    BYTE fsStyle; 
    DWORD dwData; 
    INT_PTR iString; 
} TBBUTTON, NEAR* PTBBUTTON, FAR* LPTBBUTTON; 
 */
		TBBUTTON toolbarBtns[] = {
			{DRIVEBOX_WIDTH+10, 0, 0, TBSTYLE_SEP},
			{0, 0, 0, TBSTYLE_SEP},

//			{1, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_FILE_OPEN }, 
//			{2, ID_FILE_MOVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_FILE_MOVE}, 
//			{3, ID_FILE_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_FILE_COPY}, 
//			{4, ID_FILE_COPY_CLIPBOARD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_FILE_COPY_CLIPBOARD}, 
			{5, ID_FILE_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{6, ID_FILE_RENAME, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{7, ID_FILE_PROPERTIES, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{8, ID_FILE_COMPRESS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{9, ID_FILE_UNCOMPRESS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{10, ID_FILE_RUN, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{10, ID_FILE_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{11, ID_FILE_ASSOCIATE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{12, ID_FILE_CREATE_DIRECTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{13, ID_FILE_SEARCH, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{14, ID_FILE_SELECT_FILES, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{16, ID_FILE_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{15, ID_DISK_COPY_DISK, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{16, ID_DISK_LABEL_DISK, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{17, ID_DISK_FORMAT_DISK, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{18, ID_DISK_CONNECT_NETWORK_DRIVE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{19, ID_DISK_DISCONNECT_NETWORK_DRIVE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{20, ID_DISK_SHARE_AS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{21, ID_DISK_STOP_SHARING, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{24, ID_DISK_SELECT_DRIVE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{25, ID_TREE_EXPAND_ONE_LEVEL, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{26, ID_TREE_EXPAND_BRANCH, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{27, ID_TREE_EXPAND_ALL, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{28, ID_TREE_COLLAPSE_BRANCH, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{22, ID_TREE_INDICATE_EXPANDABLE_BRANCHES, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{30, ID_VIEW_TREE_DIRECTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{31, ID_VIEW_TREE_ONLY, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{32, ID_VIEW_DIRECTORY_ONLY, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{33, ID_VIEW_SPLIT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{23, ID_VIEW_NAME, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{24, ID_VIEW_ALL_FILE_DETAILS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{25, ID_VIEW_PARTIAL_DETAILS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{37, ID_VIEW_SORT_BY_NAME, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{38, ID_VIEW_SORT_BY_TYPE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{39, ID_VIEW_SORT_BY_SIZE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{40, ID_VIEW_SORT_BY_DATE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{26, ID_VIEW_BY_FILE_TYPE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{27, ID_OPTIONS_CONFIRMATION, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{28, ID_OPTIONS_FONT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{29, ID_OPTIONS_CUSTOMISE_TOOLBAR, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{45, ID_OPTIONS_TOOLBAR, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{46, ID_OPTIONS_DRIVEBAR, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{47, ID_OPTIONS_STATUSBAR, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{30, ID_OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{31, ID_OPTIONS_MINIMISE_ON_USE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{32, ID_OPTIONS_SAVE_ON_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{33, ID_SECURITY_PERMISSIONS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{34, ID_SECURITY_AUDITING, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{35, ID_SECURITY_OWNER, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{36, ID_WINDOW_NEW_WINDOW, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{37, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{38, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{39, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{40, ID_WINDOW_ARRANGE_ICONS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{41, ID_WINDOW_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
			{42, ID_HELP_CONTENTS, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{61, ID_HELP_SEARCH_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{62, ID_HELP_HOW_TO_USE_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
//			{63, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON}, 


//			{0, ID_WINDOW_NEW_WINDOW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
//			{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
//			{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, TBSTYLE_BUTTON},
//			{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
//			{4, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
//			{5, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		};

//		Globals.hToolBar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT,
		Globals.hToolBar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|CCS_ADJUSTABLE,
			IDW_TOOLBAR, 2, hInstance, IDB_TOOLBAR, toolbarBtns,
			sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));
		CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_TOOLBAR, MF_BYCOMMAND|MF_CHECKED);

        {
            // Create the edit control. Notice that the parent of
            // the toolbar, is used as the parent of the edit control.    
            //hWndEdit = CreateWindowEx(0L, WC_COMBOBOXEX, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE 
#if 0
            Globals.hDriveCombo = CreateWindowEx(0L, _T("ComboBox"), NULL, 
                WS_CHILD | WS_BORDER | WS_VISIBLE | CBS_DROPDOWNLIST | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE, 
                10, 0, DRIVEBOX_WIDTH, DRIVEBOX_HEIGHT, Globals.hMainWnd, (HMENU)IDW_DRIVEBOX, hInstance, 0);
#else
    Globals.hDriveCombo = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
					WS_CHILD | WS_BORDER | WS_VISIBLE | CBS_DROPDOWN,
					// No size yet--resize after setting image list.
					10,     // Vertical position of Combobox
					0,      // Horizontal position of Combobox
					200,    // Sets the width of Combobox
					100,    // Sets the height of Combobox
					Globals.hMainWnd,
					(HMENU)IDW_DRIVEBOX,
					hInstance,
					NULL);
#endif
            // Set the toolbar window as the parent of the edit control
            // window. You must set the toolbar as the parent of the edit
            // control for it to appear embedded in the toolbar.
            SetParent(Globals.hDriveCombo, Globals.hToolBar);    
        }
		}

    // Create the drive bar
        Globals.hDriveBar = CreateToolbarEx(Globals.hMainWnd, 
			        WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_FLAT|TBSTYLE_LIST|TBSTYLE_WRAPABLE,
//                    WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|TBSTYLE_WRAPABLE,
					IDW_DRIVEBAR, 2, hInstance, IDB_DRIVEBAR, 
					&drivebarBtn, 1/*iNumButtons*/, 
					25/*dxButton*/, 16/*dyButton*/, 
					0/*dxBitmap*/, 0/*dyBitmap*/, sizeof(TBBUTTON));
//					16/*dxButton*/, 13/*dyButton*/, 
//					16/*dxBitmap*/, 13/*dyBitmap*/, sizeof(TBBUTTON));
		CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_DRIVEBAR, MF_BYCOMMAND|MF_CHECKED);
        ConfigureDriveBar(Globals.hDriveBar);

        // Create the status bar
        Globals.hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
                                    _T(""), Globals.hMainWnd, IDW_STATUS_WINDOW);
        if (!Globals.hStatusBar)
            return FALSE;
    	CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

        // Create the status bar panes
    	SetupStatusBar(FALSE);
	}

#if 0
	//Globals.hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, Globals.Globals.hMainWnd, IDW_STATUSBAR);
	//CheckMenuItem(Globals.Globals.hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
/* CreateStatusWindow does not accept WS_BORDER */
/* Globals.hstatusbar = CreateWindowEx(WS_EX_NOPARENTNOTIFY, STATUSCLASSNAME, 0,
					WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CCS_NODIVIDER, 0,0,0,0,
					Globals.Globals.hMainWnd, (HMENU)IDW_STATUSBAR, hinstance, 0);*/
    //TODO: read paths and window placements from registry
	GetCurrentDirectory(MAX_PATH, path);
	child = alloc_child_window(path);
	child->pos.showCmd = SW_SHOWMAXIMIZED;
	child->pos.rcNormalPosition.left = 0;
	child->pos.rcNormalPosition.top = 0;
	child->pos.rcNormalPosition.right = 320;
	child->pos.rcNormalPosition.bottom = 280;
	if (!create_child_window(child))
		free(child);
	SetWindowPlacement(child->hWnd, &child->pos);
	Globals.himl = ImageList_LoadBitmap(Globals.hInstance, MAKEINTRESOURCE(IDB_IMAGES), 16, 0, RGB(0,255,0));
	Globals.prescan_node = FALSE;
#endif

    ShowWindow(Globals.hMainWnd, nCmdShow);
    UpdateWindow(Globals.hMainWnd);
	UpdateStatusBar();
    return TRUE;
}
        
////////////////////////////////////////////////////////////////////////////////

void SetupStatusBar(BOOL bResize)
{
    int nParts[4];
//		int parts[] = {300, 500};
//		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
//		SendMessage(Globals.hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);

    // Create the status bar panes
    nParts[0] = 350;
    nParts[1] = 700;
    nParts[2] = 800;
    nParts[3] = 900;

	if (bResize)
		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 4, (long)nParts);
}

void UpdateStatusBar(void)
{
//  TCHAR text[260];
//	DWORD size;

//	size = sizeof(text)/sizeof(TCHAR);
//	GetUserName(text, &size);
//  SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
//	size = sizeof(text)/sizeof(TCHAR);
//	GetComputerName(text, &size);
//  SendMessage(Globals.hStatusBar, SB_SETTEXT, 1, (LPARAM)text);
}


static int g_foundPrevInstance = 0;

// search for already running instances
static BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lparam)
{
	TCHAR cls[128];

	GetClassName(hWnd, cls, 128);
	if (!lstrcmp(cls, (LPCTSTR)lparam)) {
		g_foundPrevInstance++;
        SetForegroundWindow(hWnd);
		return FALSE;
	}
	return TRUE;
}


void ExitInstance(void)
{
    if (Globals.himl)
	    ImageList_Destroy(Globals.himl);
}

/*
struct _cpinfo { 
  UINT MaxCharSize; 
  BYTE DefaultChar[MAX_DEFAULTCHAR]; 
  BYTE LeadByte[MAX_LEADBYTES]; 
} CPINFO, *LPCPINFO; 
 */

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;
    HWND hMDIClient;

    CPINFO CPinfo;

    OemCodePage = GetOEMCP();
    AnsiCodePage = GetACP();
    UserDefaultLCID = GetUserDefaultLCID();
    if (GetCPInfo(UserDefaultLCID, &CPinfo)) {

    }

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_WINFILE, szFrameClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_WINFILE_CHILD, szChildClass, MAX_LOADSTRING);

	// Allow only one running instance
    EnumWindows(EnumWndProc, (LPARAM)szFrameClass);
    if (g_foundPrevInstance) {
		return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    LoadSettings();

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

//    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_WINFILE);
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDR_ACCELERATOR1);
    
    hMDIClient = GetWindow(Globals.hMainWnd, GW_CHILD);

    // Main message loop:
	while (GetMessage(&msg, (HWND)NULL, 0, 0)) { 
        if (!TranslateMDISysAccel(hMDIClient, &msg) && 
            !TranslateAccelerator(Globals.hMainWnd/*hwndFrame*/, hAccel, &msg)) { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
		} 
	} 

    SaveSettings();
	ExitInstance();
    return msg.wParam;
}
