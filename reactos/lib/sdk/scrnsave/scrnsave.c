/*
 * PROJECT:         ReactOS Screen Saver Library
 * LICENSE:         GPL v2 or any later version
 * FILE:            lib/sdk/scrnsave/scrnsave.c
 * PURPOSE:         Library for writing screen savers, compatible with MS' scrnsave.lib
 * PROGRAMMERS:     Anders Norlander <anorland@hem2.passagen.se>
 *                  Colin Finck <mail@colinfinck.de>
 */

#include <windows.h>
#include <tchar.h>
#include <scrnsave.h>

// Screen Saver window class
#define CLASS_SCRNSAVE TEXT("WindowsScreenSaverClass")

// Globals
HWND        hMainWindow = NULL;
BOOL        fChildPreview = FALSE;
HINSTANCE   hMainInstance;
TCHAR       szName[TITLEBARNAMELEN];
TCHAR       szAppName[APPNAMEBUFFERLEN];
TCHAR       szIniFile[MAXFILELEN];
TCHAR       szScreenSaver[22];
TCHAR       szHelpFile[MAXFILELEN];
TCHAR       szNoHelpMemory[BUFFLEN];
UINT        MyHelpMessage;

// Local house keeping
static POINT pt_orig;

static int ISSPACE(TCHAR c)
{
    return (c == ' ' || c == '\t');
}

#define ISNUM(c) ((c) >= '0' && c <= '9')

static ULONG_PTR _toulptr(const TCHAR *s)
{
    ULONG_PTR res;
    ULONG_PTR n;
    const TCHAR *p;

    for (p = s; *p; p++)
        if (!ISNUM(*p))
            break;

    p--;
    res = 0;

    for (n = 1; p >= s; p--, n *= 10)
        res += (*p - '0') * n;

    return res;
}

// This function takes care of *must* do tasks, like terminating screen saver
static LRESULT WINAPI SysScreenSaverProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            // Mouse is not supposed to move from this position
            GetCursorPos(&pt_orig);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_SYSCOMMAND:
            if (!fChildPreview)
            {
                switch (wParam)
                {
                    case SC_CLOSE:
                    case SC_SCREENSAVE:
                        return FALSE;
                }
            }
            break;
    }

    return ScreenSaverProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WINAPI DefScreenSaverProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Don't do any special processing when in preview mode
    if (fChildPreview)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_NCACTIVATE:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            if (!wParam)
            {
                // wParam is FALSE, so the screensaver is losing the focus.
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_MOUSEMOVE:
        {
            POINT pt;
            GetCursorPos(&pt);
            if (pt.x == pt_orig.x && pt.y == pt_orig.y)
                break;

            // Fall through
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            // Send a WM_CLOSE to close the screen saver (allows the screensaver author to do clean-up tasks)
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_SETCURSOR:
            SetCursor(NULL);
            return TRUE;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Registers the screen saver window class
static BOOL RegisterScreenSaverClass(void)
{
    WNDCLASS cls = {0,};

    cls.hIcon = LoadIcon(hMainInstance, MAKEINTATOM(ID_APP));
    cls.lpszClassName = CLASS_SCRNSAVE;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hInstance = hMainInstance;
    cls.style = CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS | CS_PARENTDC;
    cls.lpfnWndProc = (WNDPROC)SysScreenSaverProc;

    return RegisterClass(&cls) != 0;
}

static void LaunchConfig(void)
{
    // Only show the dialog if the RegisterDialogClasses function succeeded.
    // This is the same behaviour as MS' scrnsave.lib.
    if( RegisterDialogClasses(hMainInstance) )
        DialogBox(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVECONFIGURE), GetForegroundWindow(), (DLGPROC) ScreenSaverConfigureDialog);
}

static int LaunchScreenSaver(HWND hParent)
{
    UINT style;
    RECT rc;
    MSG msg;

    if (!RegisterScreenSaverClass())
    {
        MessageBox(NULL, TEXT("RegisterClass() failed"), NULL, MB_ICONHAND);
        return 1;
    }

    // A slightly different approach needs to be used when displaying in a preview window
    if (hParent)
    {
        style = WS_CHILD;
        GetClientRect(hParent, &rc);
    }
    else
    {
        style = WS_POPUP;
        rc.right = GetSystemMetrics(SM_CXSCREEN);
        rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        style |= WS_VISIBLE;
    }

    // Create the main screen saver window
    hMainWindow = CreateWindowEx(hParent ? 0 : WS_EX_TOPMOST, CLASS_SCRNSAVE,
                                 TEXT("SCREENSAVER"), style,
                                 0, 0, rc.right, rc.bottom, hParent, NULL,
                                 hMainInstance, NULL);

    if(!hMainWindow)
        return 1;

    // Display window and start pumping messages
    ShowWindow(hMainWindow, SW_SHOW);
    if (!hParent)
        SetCursor(NULL);

    while (GetMessage(&msg, NULL, 0, 0))
        DispatchMessage(&msg);

    return msg.wParam;
}

// Screen Saver entry point
int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR CmdLine, int nCmdShow)
{
    LPTSTR p;

	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInst);

    hMainInstance = hInst;

    // Parse the arguments
    for (p = CmdLine; *p; p++)
    {
        switch (*p)
        {
            case 'S':
            case 's':
                // Start the screen saver
                return LaunchScreenSaver(NULL);

            case 'P':
            case 'p':
            {
                // Start the screen saver in preview mode
                HWND hParent;
                fChildPreview = TRUE;

                while (ISSPACE(*++p));
                hParent = (HWND) _toulptr(p);

                if (hParent && IsWindow(hParent))
                    return LaunchScreenSaver(hParent);
            }
            return 0;

            case 'C':
            case 'c':
                // Display the configuration dialog
                LaunchConfig();
                return 0;

            case '-':
            case '/':
            case ' ':
            default:
                break;
        }
    }

    LaunchConfig();

    return 0;
}
