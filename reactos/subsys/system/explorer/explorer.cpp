/*
 * Copyright 2003 Martin Fuchs
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


 //
 // Explorer clone
 //
 // explorer.cpp
 //
 // Martin Fuchs, 23.07.2003
 //
 // Credits: Thanks to Leon Finker for his explorer window example
 //


#include "utility/utility.h"
#include "utility/shellclasses.h"

#include "explorer.h"
#include "globals.h"

#include "explorer_intres.h"
#include "externals.h"


ExplorerGlobals g_Globals;


ExplorerGlobals::ExplorerGlobals()
{
	_hInstance = 0;
	_hframeClass = 0;
	_cfStrFName = 0;
	_hMainWnd = 0;
	_prescan_nodes = false;
	_desktop_mode = false;
}


ResString::ResString(UINT nid)
{
	TCHAR buffer[BUFFER_LEN];

	int len = LoadString(g_Globals._hInstance, nid, buffer, sizeof(buffer)/sizeof(TCHAR));

	assign(buffer, len);
}


void explorer_show_frame(HWND hwndDesktop, int cmdshow)
{
	if (g_Globals._hMainWnd)
		return;

	g_Globals._prescan_nodes = false;

	HMENU hMenuFrame = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(IDM_MAINFRAME));

	 // create main window
	g_Globals._hMainWnd = Window::Create(WINDOW_CREATOR(MainFrame), 0,
					(LPCTSTR)(int)g_Globals._hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					0/*hwndDesktop*/, hMenuFrame);

	ShowWindow(g_Globals._hMainWnd, cmdshow);

	UpdateWindow(g_Globals._hMainWnd);

	 // Open the first child window after initialiszing the whole application
	PostMessage(g_Globals._hMainWnd, WM_OPEN_WINDOW, 0, 0);
}


static void InitInstance(HINSTANCE hinstance)
{
	g_Globals._hInstance = hinstance;

	setlocale(LC_COLLATE, "");	// set collating rules to local settings for compareName


	 // register frame window class

	WindowClass wcFrame(CLASSNAME_FRAME);

	wcFrame.hIcon		  = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_EXPLORER));
	wcFrame.hCursor 	  = LoadCursor(0, IDC_ARROW);
	wcFrame.hIconSm 	  = (HICON)LoadImage(hinstance,
											 MAKEINTRESOURCE(IDI_EXPLORER),
											 IMAGE_ICON,
											 GetSystemMetrics(SM_CXSMICON),
											 GetSystemMetrics(SM_CYSMICON),
											 LR_SHARED);

	g_Globals._hframeClass = wcFrame.Register();


	 // register child windows class

	WindowClass wcChild(CLASSNAME_CHILDWND);

	wcChild.style		  = CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW;
	wcChild.hCursor 	  = LoadCursor(0, IDC_ARROW);

	wcChild.Register();


	 // register tree windows class

	WindowClass wcTreeChild(CLASSNAME_WINEFILETREE);

	wcTreeChild.style	  = CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW;
	wcTreeChild.hCursor   = LoadCursor(0, IDC_ARROW);

	wcTreeChild.Register();


	g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hinstance, HWND hwndDesktop, int cmdshow)
{
	 // initialize COM and OLE
	OleInit usingCOM;

	 // initialize Common Controls library
	CommonControlInit usingCmnCtrl(ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES);

	try {
		MSG msg;

		InitInstance(hinstance);

		if (hwndDesktop)
			g_Globals._desktop_mode = true;

		if (cmdshow != SW_HIDE) {
#ifndef _ROS_	// don't maximize if being called from the ROS desktop
			if (cmdshow == SW_SHOWNORMAL)
					/*TODO: read window placement from registry */
				cmdshow = SW_MAXIMIZE;
#endif

			explorer_show_frame(hwndDesktop, cmdshow);
		}

		while(GetMessage(&msg, 0, 0, 0)) {
			if (g_Globals._hMainWnd && SendMessage(g_Globals._hMainWnd, WM_TRANSLATE_MSG, 0, (LPARAM)&msg))
				continue;

			TranslateMessage(&msg);

			try {
				DispatchMessage(&msg);
			} catch(COMException& e) {
				HandleException(e, g_Globals._hMainWnd);
			}
		}

		return msg.wParam;

	} catch(COMException& e) {
		HandleException(e, g_Globals._hMainWnd);
	}

	return -1;
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	 // create desktop window and task bar only, if there is no other shell and we are
	 // the first explorer instance
	BOOL startup_desktop = !IsAnyDesktopRunning();

	 // If there is given the command line option "-desktop", create desktop window anyways
	if (!lstrcmp(lpCmdLine,TEXT("-desktop")))
		startup_desktop = TRUE;

	if (!lstrcmp(lpCmdLine,TEXT("-noexplorer"))) {
		nShowCmd = SW_HIDE;
		startup_desktop = TRUE;
	}

	HWND hwndDesktop = 0;

	if (startup_desktop)
	{
		hwndDesktop = create_desktop_window(hInstance);

		 // Initialize the explorer bar
		HWND hwndExplorerBar = InitializeExplorerBar(hInstance);

		 // Load plugins
//		LoadAvailablePlugIns(hwndExplorerBar);

#ifndef _DEBUG	//MF: disabled for debugging
		{
		char* argv[] = {""};
		startup(1, argv);
		}
#endif
	}

	int ret = explorer_main(hInstance, hwndDesktop, nShowCmd);

//	ReleaseAvailablePlugIns();

	return ret;
}
