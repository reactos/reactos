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

#include <locale.h>	// for setlocale()

#ifndef __WINE__
#include <io.h>		// for dup2()
#include <fcntl.h>	// for _O_RDONLY
#endif


extern "C" int initialize_gdb_stub();	// start up GDB stub


ExplorerGlobals g_Globals;


ExplorerGlobals::ExplorerGlobals()
{
	_hInstance = 0;
	_hframeClass = 0;
	_cfStrFName = 0;
	_hMainWnd = 0;
	_prescan_nodes = false;
	_desktop_mode = false;
	_log = NULL;
#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	_SHRestricted = 0;
#endif
}


void _log_(LPCTSTR txt)
{
	FmtString msg(TEXT("%s\n"), txt);

	if (g_Globals._log)
		_fputts(msg, g_Globals._log);

	OutputDebugString(msg);
}


ResString::ResString(UINT nid)
{
	TCHAR buffer[BUFFER_LEN];

	int len = LoadString(g_Globals._hInstance, nid, buffer, sizeof(buffer)/sizeof(TCHAR));

	super::assign(buffer, len);
}


ResIcon::ResIcon(UINT nid)
{
	_hIcon = LoadIcon(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}

SmallIcon::SmallIcon(UINT nid)
{
	_hIcon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

ResIconEx::ResIconEx(UINT nid, int w, int h)
{
	_hIcon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, w, h, LR_SHARED);
}


void SetWindowIcon(HWND hwnd, UINT nid)
{
	HICON hIcon = ResIcon(nid);
	Window_SetIcon(hwnd, ICON_BIG, hIcon);

	HICON hIconSmall = SmallIcon(nid);
	Window_SetIcon(hwnd, ICON_SMALL, hIconSmall);
}


ResBitmap::ResBitmap(UINT nid)
{
	_hBmp = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}


void explorer_show_frame(HWND hwndDesktop, int cmdshow, LPTSTR lpCmdLine)
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

		bool valid_dir = false;

		if (lpCmdLine) {
			DWORD attribs = GetFileAttributes(lpCmdLine);

			if (attribs!=INVALID_FILE_ATTRIBUTES && (attribs&FILE_ATTRIBUTE_DIRECTORY))
				valid_dir = true;
			else if (*lpCmdLine==':' || *lpCmdLine=='"')
				valid_dir = true;
		}

		 // Open the first child window after initializing the application
		if (valid_dir)
			PostMessage(hMainFrame, PM_OPEN_WINDOW, 0, (LPARAM)lpCmdLine);
		else
			PostMessage(hMainFrame, PM_OPEN_WINDOW, OWM_EXPLORE|OWM_DETAILS, 0);
	}
}


 /// "About Explorer" Dialog
struct ExplorerAboutDlg : public
			CtlColorParent<
				OwnerDrawParent<Dialog>
			>
{
	typedef CtlColorParent<
				OwnerDrawParent<Dialog>
			> super;

	ExplorerAboutDlg(HWND hwnd)
	 :	super(hwnd)
	{
		SetWindowIcon(hwnd, IDI_REACTOS);

		new FlatButton(hwnd, IDOK);

		_hfont = CreateFont(20, 0, 0, 0, FW_BOLD, TRUE, 0, 0, 0, 0, 0, 0, 0, TEXT("Sans Serif"));
		new ColorStatic(hwnd, IDC_ROS_EXPLORER, RGB(32,32,128), 0, _hfont);

		new HyperlinkCtrl(hwnd, IDC_WWW);

		CenterWindow(hwnd);
	}

	~ExplorerAboutDlg()
	{
		DeleteObject(_hfont);
	}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case WM_PAINT:
			Paint();
			break;

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}

		return 0;
	}

	void Paint()
	{
		PaintCanvas canvas(_hwnd);

		HICON hicon = (HICON) LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(IDI_REACTOS_BIG), IMAGE_ICON, 0, 0, LR_SHARED);

		DrawIconEx(canvas, 20, 10, hicon, 0, 0, 0, 0, DI_NORMAL);
	}

protected:
	HFONT	_hfont;
};

void explorer_about(HWND hwndParent)
{
	Dialog::DoModal(IDD_ABOUT_EXPLORER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}


static void InitInstance(HINSTANCE hInstance)
{
	CONTEXT("InitInstance");

	setlocale(LC_COLLATE, "");	// set collating rules to local settings for compareName

	 // register frame window class
	g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME,IDI_EXPLORER);

	 // register child windows class
	WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	 // register tree windows class
	WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hInstance, HWND hwndDesktop, LPTSTR lpCmdLine, int cmdshow)
{
	CONTEXT("explorer_main");

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
				///@todo read window placement from registry
			cmdshow = SW_MAXIMIZE;
*/

		explorer_show_frame(hwndDesktop, cmdshow, lpCmdLine);
	}

	return Window::MessageLoop();
}


 // MinGW does not provide a Unicode startup routine, so we have to implement an own.
#if defined(__MINGW32__) && defined(UNICODE)

#define _tWinMain wWinMain
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);

int main(int argc, char* argv[])
{
	CONTEXT("main");

	STARTUPINFO startupinfo;
	int nShowCmd = SW_SHOWNORMAL;

	GetStartupInfo(&startupinfo);

	if (startupinfo.dwFlags & STARTF_USESHOWWINDOW)
		nShowCmd = startupinfo.wShowWindow;

	LPWSTR cmdline = GetCommandLineW();

	while(*cmdline && !_istspace(*cmdline))
		++cmdline;

	return wWinMain(GetModuleHandle(NULL), 0, cmdline, nShowCmd);
}

#endif	// __MINGW && UNICODE


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	CONTEXT("WinMain()");

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
	else if (_tcsstr(lpCmdLine,TEXT("-nodesktop")))
		startup_desktop = FALSE;

	 // Don't display cabinet window in desktop mode
	if (startup_desktop && !_tcsstr(lpCmdLine,TEXT("-explorer")))
		nShowCmd = SW_HIDE;

	if (_tcsstr(lpCmdLine,TEXT("-noautostart")))
		autostart = false;

#ifndef __WINE__
	if (_tcsstr(lpCmdLine,TEXT("-console"))) {
		AllocConsole();

		_dup2(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY), 0);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0), 1);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), 0), 2);

		g_Globals._log = fdopen(1, "w");
		setvbuf(g_Globals._log, 0, _IONBF, 0);

		LOG(TEXT("starting explorer debug log\n"));
	}
#endif

	bool use_gdb_stub = false;	// !IsDebuggerPresent();

	if (_tcsstr(lpCmdLine,TEXT("-debug")))
		use_gdb_stub = true;

	if (_tcsstr(lpCmdLine,TEXT("-break"))) {
		LOG(TEXT("debugger breakpoint"));
#ifdef _MSC_VER
		__asm int 3
#else
		asm("int3");
#endif
	}

	 // activate GDB remote debugging stub if no other debugger is running
	if (use_gdb_stub) {
		LOG(TEXT("waiting for debugger connection...\n"));

		initialize_gdb_stub();
	}

	g_Globals._hInstance = hInstance;
#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	g_Globals._SHRestricted = (DWORD(STDAPICALLTYPE*)(RESTRICTIONS)) GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "SHRestricted");
#endif

	 // initialize COM and OLE before creating the desktop window
	OleInit usingCOM;

	HWND hwndDesktop = 0;

	if (startup_desktop)
	{
		hwndDesktop = DesktopWindow::Create();

		if (autostart)
		{
			char* argv[] = {"", "s"};	// call startup routine in SESSION_START mode
			startup(2, argv);
		}
	}

	/**TODO fix command line handling */
	if (*lpCmdLine=='"' && lpCmdLine[_tcslen(lpCmdLine)-1]=='"') {
		++lpCmdLine;
		lpCmdLine[_tcslen(lpCmdLine)-1] = '\0';
	}

	int ret = explorer_main(hInstance, hwndDesktop, lpCmdLine, nShowCmd);

	return ret;
}
