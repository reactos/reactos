/*
    Desktop creation example

    Silver Blade (silverblade_uk@hotmail.com)
    5th July 2003
*/

#include <stdio.h>
#include <windows.h>

#include "../utility/utility.h"

#include "../externals.h"


/* GetShellWindow is already present in the header files
static HWND (WINAPI*GetShellWindow)(); */
static BOOL (WINAPI*SetShellWindow)(HWND);


BOOL IsAnyDesktopRunning()
{
/*	POINT pt;*/
	HINSTANCE shell32 = GetModuleHandle(TEXT("user32"));

	SetShellWindow = (BOOL(WINAPI*)(HWND)) GetProcAddress(shell32, "SetShellWindow");

/* GetShellWindow is already present in the header files
	GetShellWindow = (HWND(WINAPI*)()) GetProcAddress(shell32, "GetShellWindow");

	if (GetShellWindow) */
		return GetShellWindow() != 0;
/*
	pt.x = 0;
	pt.y = 0;

	return WindowFromPoint(pt) != GetDesktopWindow(); */
}


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
			explorer_show_frame(hwnd, SW_SHOWNORMAL);
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


const TCHAR DesktopClassName[] = TEXT("DesktopWindow");


HWND create_desktop_window(HINSTANCE hInstance)
{
    WNDCLASSEX wc;
	HWND hwndDesktop;
    int Width, Height;

	wc.cbSize       = sizeof(WNDCLASSEX);
	wc.style        = CS_DBLCLKS;
	wc.lpfnWndProc  = &DeskWndProc;
	wc.cbClsExtra   = 0;
	wc.cbWndExtra   = 0;
	wc.hInstance    = hInstance;
	wc.hIcon        = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor      = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName= DesktopClassName;
	wc.hIconSm      = NULL;

	if (!RegisterClassEx(&wc))
		return 0;

	Width = GetSystemMetrics(SM_CXSCREEN);
	Height = GetSystemMetrics(SM_CYSCREEN);

	hwndDesktop = CreateWindowEx(0, DesktopClassName, TEXT("Desktop"),
							WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN,
							0, 0, Width, Height,
							NULL, NULL, hInstance, NULL);

	if (SetShellWindow)
		SetShellWindow(hwndDesktop);

	return hwndDesktop;
}


#ifdef _CONSOLE
int main(int argc, char *argv[])
#else
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
#endif
{
	int ret;
    HWND hwndDesktop = 0;

#ifdef _CONSOLE
	STARTUPINFO startupinfo;
	int nShowCmd = SW_SHOWNORMAL;

	HINSTANCE hInstance = GetModuleHandle(NULL);
#endif

	 // create desktop window and task bar only, if there is no other shell and we are
	 // the first explorer instance (just in case SetShellWindow() is not supported by the OS)
	BOOL startup_desktop = !IsAnyDesktopRunning() && !find_window_class(DesktopClassName);

#ifdef _CONSOLE
	if (argc>1 && !strcmp(argv[1],"-desktop"))
#else
	if (!lstrcmp(lpCmdLine,TEXT("-desktop")))
#endif
		startup_desktop = TRUE;

	if (startup_desktop)
	{
		HWND hwndExplorerBar;

		hwndDesktop = create_desktop_window(hInstance);

		if (!hwndDesktop)
		{
			fprintf(stderr,"FATAL: Desktop window could not be initialized properly - Exiting.\n");
			return 1;   // error
		}

#ifdef _CONSOLE
		 // call winefile startup routine
		GetStartupInfo(&startupinfo);

		if (startupinfo.dwFlags & STARTF_USESHOWWINDOW)
			nShowCmd = startupinfo.wShowWindow;
#endif

		 // Initializing the Explorer Bar
		if (!(hwndExplorerBar=InitializeExplorerBar(hInstance)))
		{
			fprintf(stderr,"FATAL: Explorer bar could not be initialized properly ! Exiting !\n");
			return 1;
		}

		 // Load plugins
		if (!LoadAvailablePlugIns(hwndExplorerBar))
		{
			fprintf(stderr,"WARNING: No plugin for desktop bar could be loaded.\n");
		}

#ifndef _DEBUG	//MF: disabled for debugging
#ifdef _CONSOLE
	    startup(argc, argv); // invoke the startup groups
#else
		{
		char* argv[] = {""};
	    startup(1, argv);
		}
#endif
#endif
	}

	ret = explorer_main(hInstance, hwndDesktop, nShowCmd);

	ReleaseAvailablePlugIns();

	return ret;
}
