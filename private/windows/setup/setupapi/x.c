#include <windows.h>
#include "setupapi.h"


LPCTSTR pszAppName = TEXT("ViewLog");                // class name
HANDLE  hModule;                                // handle of this instance
HANDLE  hRichedDLL;                             // DLL used for rich edit
HANDLE  hWndMain;                               // handle to main window
BOOL g_LogOpen;
INT g_LogCount;

LONG
MainWndProc (
    IN HWND     hwnd,
    IN UINT     message,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rect;
    TCHAR Msg[256];

    switch (message) {

    case WM_CREATE:
        break;

    case WM_PAINT:
        hdc = BeginPaint (hwnd, &ps);
        GetClientRect (hwnd, &rect);
        if (g_LogOpen) {
            DrawText (hdc, TEXT("Log is open!"), -1, &rect,
                      DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        } else {
            DrawText (hdc, TEXT("Log is not open!"), -1, &rect,
                      DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        if (g_LogCount > 0) {
            wsprintf (Msg, TEXT("Messages logged: %i"), g_LogCount);
            rect.top += 30;
            DrawText (hdc, Msg, -1, &rect,
                      DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }


        EndPaint (hwnd, &ps);
        break;

    case WM_LBUTTONDOWN:
        MessageBeep(0);
        if (SetupLogError (TEXT("foo\r\n"), LogSevFatalError)) {
            g_LogCount++;
            InvalidateRect (hwnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage (0);
        break;

    default:
        return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}


static 
BOOL
InitMainWindow (
    VOID
    )
{
    WNDCLASS wc;

    //
    // Initialize the window class.
    //

    hModule = GetModuleHandle (NULL);

    if (TRUE || FindWindow (pszAppName, NULL) == NULL) {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = MainWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = (HINSTANCE) hModule;
        wc.hIcon            = LoadIcon (NULL, IDI_APPLICATION);
        wc.hCursor          = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName     = pszAppName;
        wc.lpszClassName    = pszAppName;

        if (!RegisterClass (&wc)) {
            return FALSE;
        }
    }


    //
    // Create the window and display it.
    //
    hWndMain = CreateWindow (
        pszAppName,
        TEXT("The Hello Program"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        NULL, NULL,
        (HINSTANCE) hModule,
        NULL
    );
    if (!hWndMain) {
        return FALSE;
    }

    ShowWindow (hWndMain, SW_SHOWNORMAL);
    UpdateWindow (hWndMain);
    return TRUE;

}


INT 
WINAPI 
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    INT nCmdShow
    )

{
    MSG     msg;

    g_LogOpen = SetupOpenLog (TRUE);


    // Initialize everything
    //
    if (!InitMainWindow ()) {
        return 0;
    }

    // Process messages
    //
    while (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    SetupCloseLog();

    return (msg.wParam);
}

