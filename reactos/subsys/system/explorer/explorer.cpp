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

#include "explorer.h"
#include "desktop/desktop.h"

#include "globals.h"
#include "externals.h"

#include "explorer_intres.h"

#include <locale.h>


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


ResIcon::ResIcon(UINT nid)
{
	_hIcon = LoadIcon(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}

SmallIcon::SmallIcon(UINT nid)
{
	_hIcon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}


ResBitmap::ResBitmap(UINT nid)
{
	_hBmp = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}


void explorer_show_frame(HWND hwndDesktop, int cmdshow)
{
	if (g_Globals._hMainWnd)
		return;

	g_Globals._prescan_nodes = false;

	 // create main window
	HWND hMainFrame = MainFrame::Create();

	if (hMainFrame) {
		g_Globals._hMainWnd = hMainFrame;

		ShowWindow(hMainFrame, cmdshow);
		UpdateWindow(hMainFrame);

		 // Open the first child window after initializing the whole application
		PostMessage(hMainFrame, PM_OPEN_WINDOW, TRUE/*mode_explore*/, 0);
	}
}


static void InitInstance(HINSTANCE hInstance)
{
	setlocale(LC_COLLATE, "");	// set collating rules to local settings for compareName

	 // register frame window class
	g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME,IDI_EXPLORER);

	 // register child windows class
	WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	 // register tree windows class
	WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hInstance, HWND hwndDesktop, int cmdshow)
{
	 // initialize COM and OLE
	OleInit usingCOM;

	 // initialize Common Controls library
	CommonControlInit usingCmnCtrl;

	try {
		InitInstance(hInstance);
	} catch(COMException& e) {
		HandleException(e, hwndDesktop);
		return -1;
	}

	if (hwndDesktop)
		g_Globals._desktop_mode = true;

	if (cmdshow != SW_HIDE) {
/*	// don't maximize if being called from the ROS desktop
		if (cmdshow == SW_SHOWNORMAL)
				//TODO: read window placement from registry
			cmdshow = SW_MAXIMIZE;
*/

		explorer_show_frame(hwndDesktop, cmdshow);
	}

	return Window::MessageLoop();
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	 // create desktop window and task bar only, if there is no other shell and we are
	 // the first explorer instance
	BOOL startup_desktop = !IsAnyDesktopRunning();

	bool autostart = true;

#ifdef _DEBUG	//MF: disabled for debugging
	autostart = false;
#endif

	 // If there is given the command line option "-desktop", create desktop window anyways
	if (_tcsstr(lpCmdLine,TEXT("-desktop")))
		startup_desktop = TRUE;

	if (_tcsstr(lpCmdLine,TEXT("-nodesktop")))
		startup_desktop = FALSE;

	if (_tcsstr(lpCmdLine,TEXT("-noexplorer"))) {
		nShowCmd = SW_HIDE;
		startup_desktop = TRUE;
	}

	if (!_tcsstr(lpCmdLine,TEXT("-noautostart")))
		autostart = false;

	g_Globals._hInstance = hInstance;

	HWND hwndDesktop = 0;

	if (startup_desktop)
	{
		hwndDesktop = DesktopWindow::Create();

		if (autostart)
		{
			char* argv[] = {""};
			startup(1, argv);
		}
	}

	int ret = explorer_main(hInstance, hwndDesktop, nShowCmd);

	return ret;
}
