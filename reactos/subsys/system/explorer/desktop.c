/*
    Desktop creation example

    Silver Blade (silverblade_uk@hotmail.com)
    5th July 2003
*/

#include <stdio.h>
#include <windows.h>

#include "include/explorer.h"


const TCHAR DesktopClassName[] = TEXT("DesktopWindow");


HWND (WINAPI*GetShellWindow)();
void (WINAPI*SetShellWindow)(HWND);


LRESULT CALLBACK DeskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE :
        {
            // Over-ride close. We need to close desktop some other way.
            break;
        }

        case WM_PAINT :
        {
            // We'd want to draw the desktop wallpaper here. Need to
            // maintain a copy of the wallpaper in an off-screen DC and then
            // bitblt (or stretchblt?) it to the screen appropriately. For
            // now, though, we'll just draw some text.

            PAINTSTRUCT ps;
            HDC DesktopDC = BeginPaint(hwnd, &ps);

            TCHAR Text [] = TEXT("ReactOS 0.1.2 Desktop Example\nby Silver Blade");

            int Width, Height;

            Width = GetSystemMetrics(SM_CXSCREEN);
            Height = GetSystemMetrics(SM_CYSCREEN);

            // This next part could be improved by working out how much
            // space the text actually needs...

			{
			RECT r;
            r.left = Width - 260;
            r.top = Height - 80;
            r.right = r.left + 250;
            r.bottom = r.top + 40;

            SetTextColor(DesktopDC, 0x00ffffff);
            SetBkMode(DesktopDC, TRANSPARENT);
            DrawText(DesktopDC, Text, -1, &r, DT_RIGHT);
			}

            EndPaint(hwnd, &ps);

			break;
        }

		case WM_LBUTTONDBLCLK:
			ShowFileMgr(hwnd, SW_SHOWNORMAL);
			break;

		case WM_DESTROY:
			if (SetShellWindow)
				SetShellWindow(0);
			break;

		default:
		    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

	return 0;
}


BOOL IsAnyDesktopRunning()
{
	POINT pt;
	HINSTANCE shell32 = GetModuleHandle(TEXT("user32"));

	GetShellWindow = (HWND(WINAPI*)()) GetProcAddress(shell32, "GetShellWindow");
	SetShellWindow = (void(WINAPI*)(HWND)) GetProcAddress(shell32, "SetShellWindow");

	if (GetShellWindow)
		return GetShellWindow() != 0;

	pt.x = 0;
	pt.y = 0;

	return WindowFromPoint(pt) != GetDesktopWindow();
}


int main(int argc, char *argv[])
{
    int Width, Height;
    WNDCLASSEX wc;
    HWND hwndDesktop = 0;

	STARTUPINFO startupinfo;
	int nCmdShow = SW_SHOWNORMAL;

	HINSTANCE hInstance = GetModuleHandle(NULL);

	 // create desktop window and task bar only, if there is no other shell and we are
	 // the first explorer instance (just in case SetShellWindow() is not supported by the OS)
	BOOL startup_desktop = !IsAnyDesktopRunning() && !find_window_class(DesktopClassName);

	if (argc>1 && !strcmp(argv[1],"-desktop"))
		startup_desktop = TRUE;

	if (startup_desktop)
	{
		HWND hwndExplorerBar;

		wc.cbSize       = sizeof(WNDCLASSEX);
		wc.style        = CS_DBLCLKS;
		wc.lpfnWndProc  = &DeskWndProc;
		wc.cbClsExtra   = 0;
		wc.cbWndExtra   = 0;
		wc.hInstance    = hInstance;
		wc.hIcon        = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName= DesktopClassName;
		wc.hIconSm      = NULL;

		if (!RegisterClassEx(&wc))
			return 1; // error


		Width = GetSystemMetrics(SM_CXSCREEN);
		Height = GetSystemMetrics(SM_CYSCREEN);

		hwndDesktop = CreateWindowEx(0, DesktopClassName, TEXT("Desktop"),
								WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN,
								0, 0, Width, Height,
								NULL, NULL, hInstance, NULL);

		if (!hwndDesktop)
		{
			fprintf(stderr,"FATAL: Desktop window could not be initialized properly - Exiting.\n");
			return 1;   // error
		}

		if (SetShellWindow)
			SetShellWindow(hwndDesktop);

		 // call winefile startup routine
		GetStartupInfo(&startupinfo);

		if (startupinfo.dwFlags & STARTF_USESHOWWINDOW)
			nCmdShow = startupinfo.wShowWindow;

		 // Initializing the Explorer Bar
		if (!(hwndExplorerBar=InitializeExplorerBar(hInstance, nCmdShow)))
		{
			fprintf(stderr,"FATAL: Explorer bar could not be initialized properly ! Exiting !\n");
			return 1;
		}

		 // Load plugins
		if (!ExplorerLoadPlugins(hwndExplorerBar))
		{
			fprintf(stderr,"WARNING: No plugin for desktop bar could be loaded.\n");
		}

#ifndef _DEBUG	//MF: disabled for debugging
	    startup(argc, argv); // invoke the startup groups
#endif
	}

	return winefile_main(hInstance, hwndDesktop, nCmdShow);
}
