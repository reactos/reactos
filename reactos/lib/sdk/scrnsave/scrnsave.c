/*
 * PROJECT:         ReactOS Screen Saver Library
 * LICENSE:         GPL v2 or any later version
 * FILE:            lib/sdk/scrnsave/scrnsave.c
 * PURPOSE:         Library for writing screen savers, compatible with
 *                  MS' scrnsave.lib without Win9x support.
 * PROGRAMMERS:     Anders Norlander <anorland@hem2.passagen.se>
 *                  Colin Finck <mail@colinfinck.de>
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
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

#define ISNUM(c) ((c) >= '0' && (c) <= '9')

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
                    case SC_CLOSE:      // - Closing the screen saver, or...
                    case SC_NEXTWINDOW: // - Switching to
                    case SC_PREVWINDOW: //   different windows, or...
                    case SC_SCREENSAVE: // - Starting another screen saver:
                        return FALSE;   // Fail it!
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
                // wParam is FALSE, so the screen saver is losing the focus.
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_MOUSEMOVE:
        {
            POINT pt;
            GetCursorPos(&pt);
            // TODO: Implement mouse move threshold. See:
            // http://svn.reactos.org/svn/reactos/trunk/rosapps/applications/screensavers/starfield/screensaver.c?r1=67455&r2=67454&pathrev=67455
            if (pt.x == pt_orig.x && pt.y == pt_orig.y)
                break;

            // Fall through
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
            // Send a WM_CLOSE to close the screen saver (allows
            // the screen saver to perform clean-up tasks)
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
    WNDCLASS cls;

    cls.hCursor       = NULL;
    cls.hIcon         = LoadIcon(hMainInstance, MAKEINTATOM(ID_APP));
    cls.lpszMenuName  = NULL;
    cls.lpszClassName = CLASS_SCRNSAVE;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hInstance     = hMainInstance;
    cls.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_SAVEBITS | CS_PARENTDC;
    cls.lpfnWndProc   = SysScreenSaverProc;
    cls.cbWndExtra    = 0;
    cls.cbClsExtra    = 0;

    return (RegisterClass(&cls) != 0);
}

static int LaunchConfig(HWND hParent)
{
    // Only show the dialog if the RegisterDialogClasses function succeeded.
    // This is the same behaviour as MS' scrnsave.lib.
    if (!RegisterDialogClasses(hMainInstance))
        return -1;

    return DialogBox(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVECONFIGURE),
                     hParent, (DLGPROC)ScreenSaverConfigureDialog);
}

static int LaunchScreenSaver(HWND hParent)
{
    LPCTSTR lpWindowName;
    UINT style, exstyle;
    RECT rc;
    MSG msg;

    if (!RegisterScreenSaverClass())
    {
        MessageBox(NULL, TEXT("RegisterClass() failed"), NULL, MB_ICONHAND);
        return -1;
    }

    // A slightly different approach needs to be used when displaying in a preview window
    if (hParent)
    {
        fChildPreview = TRUE;
        lpWindowName  = TEXT("Preview");

        style   = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
        exstyle = 0;

        GetClientRect(hParent, &rc);
        rc.left = 0;
        rc.top  = 0;
    }
    else
    {
        fChildPreview = FALSE;
        lpWindowName  = TEXT("Screen Saver");

        style   = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        exstyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;

        // Get the left & top side coordinates of the virtual screen
        rc.left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
        rc.top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
        // Get the width and height of the virtual screen
        rc.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }

    // Create the main screen saver window
    hMainWindow = CreateWindowEx(exstyle, CLASS_SCRNSAVE, lpWindowName, style,
                                 rc.left, rc.top, rc.right, rc.bottom,
                                 hParent, NULL, hMainInstance, NULL);
    if (!hMainWindow)
        return -1;

    // Display window and start pumping messages
    ShowWindow(hMainWindow, SW_SHOW);
    if (!hParent)
        SetCursor(NULL);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

// Screen Saver entry point
int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR CmdLine, int nCmdShow)
{
    LPTSTR p;

    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(hPrevInst);

    hMainInstance = hInst;

    // Parse the arguments:
    //   -a <hwnd>  (Change the password; only for Win9x, unused on WinNT)
    //   -s         (Run the screensaver)
    //   -p <hwnd>  (Preview)
    //   -c <hwnd>  (Configure)
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
                HWND hParent;

                while (ISSPACE(*++p));
                hParent = (HWND)_toulptr(p);

                // Start the screen saver in preview mode
                if (hParent && IsWindow(hParent))
                    return LaunchScreenSaver(hParent);
                else
                    return -1;
            }

            case 'C':
            case 'c':
            {
                HWND hParent;

                if (p[1] == ':')
                    hParent = (HWND)_toulptr(p + 2);
                else
                    hParent = GetForegroundWindow();

                // Display the configuration dialog
                if (hParent && IsWindow(hParent))
                    return LaunchConfig(hParent);
                else
                    return -1;
            }

            case '-':
            case '/':
            case ' ':
            default:
                break;
        }
    }

    return LaunchConfig(NULL);
}
