/*
 * A basic example of Win32 programming in C.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Colin Peters <colinp at ma.kcom.ne.jp>
 */
#include <windows.h>
#include <string.h>

/*
 * This is the window function for the main window. Whenever a message is
 * dispatched using DispatchMessage (or sent with SendMessage) this function
 * gets called with the contents of the message.
 */
LRESULT CALLBACK
MainWndProc (HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
        /* The window handle for the "Click Me" button. */
        static HWND   hwndButton = 0;
        static int    cx, cy;          /* Height and width of our button. */

        HDC           hdc;             /* A device context used for drawing */
        PAINTSTRUCT   ps;              /* Also used during window drawing */
        RECT          rc;              /* A rectangle used during drawing */

        /*
         * Perform processing based on what kind of message we got.
         */
        switch (nMsg)
        {
                case WM_CREATE:
                {
                        /* The window is being created. Create our button
                         * window now. */
                        TEXTMETRIC        tm;

                        /* First we use the system fixed font size to choose
                         * a nice button size. */
                        hdc = GetDC (hwnd);
                        SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));
                        GetTextMetrics (hdc, &tm);
                        cx = tm.tmAveCharWidth * 30;
                        cy = (tm.tmHeight + tm.tmExternalLeading) * 2;
                        ReleaseDC (hwnd, hdc);

                        /* Now create the button */
                        hwndButton = CreateWindow (
                                "button",         /* Builtin button class */
                                "Click Here",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                0, 0, cx, cy,
                                hwnd,             /* Parent is this window. */
                                (HMENU) 1,        /* Control ID: 1 */
                                ((LPCREATESTRUCT) lParam)->hInstance,
                                NULL
                                );

                        return 0;
                        break;
                }

                case WM_DESTROY:
                        /* The window is being destroyed, close the application
                         * (the child button gets destroyed automatically). */
                        PostQuitMessage (0);
                        return 0;
                        break;

                case WM_PAINT:
                        /* The window needs to be painted (redrawn). */
                        hdc = BeginPaint (hwnd, &ps);
                        GetClientRect (hwnd, &rc);

                        /* Draw "Hello, World" in the middle of the upper
                         * half of the window. */
                        rc.bottom = rc.bottom / 2;
                        DrawText (hdc, "Hello, World", -1, &rc,
                                DT_SINGLELINE | DT_CENTER | DT_VCENTER);

                        EndPaint (hwnd, &ps);
                        return 0;
                        break;

                case WM_SIZE:
                        /* The window size is changing. If the button exists
                         * then place it in the center of the bottom half of
                         * the window. */
                        if (hwndButton &&
                                (wParam == SIZEFULLSCREEN ||
                                 wParam == SIZENORMAL)
                           )
                        {
                                rc.left = (LOWORD(lParam) - cx) / 2;
                                rc.top = HIWORD(lParam) * 3 / 4 - cy / 2;
                                MoveWindow (
                                        hwndButton,
                                        rc.left, rc.top, cx, cy, TRUE);
                        }
                        break;

                case WM_COMMAND:
                        /* Check the control ID, notification code and
                         * control handle to see if this is a button click
                         * message from our child button. */
                        if (LOWORD(wParam) == 1 &&
                            HIWORD(wParam) == BN_CLICKED &&
                            (HWND) lParam == hwndButton)
                        {
                                /* Our button was clicked. Close the window. */
                                DestroyWindow (hwnd);
                        }
                        return 0;
                        break;
        }

        /* If we don't handle a message completely we hand it to the system
         * provided default window function. */
        return DefWindowProc (hwnd, nMsg, wParam, lParam);
}


int STDCALL
WinMain (HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
        HWND         hwndMain;        /* Handle for the main window. */
        MSG          msg;             /* A Win32 message structure. */
        WNDCLASSEX   wndclass;        /* A window class structure. */
        char*        szMainWndClass = "WinTestWin";
                                      /* The name of the main window class */

        /*
         * First we create a window class for our main window.
         */

        /* Initialize the entire structure to zero. */
        memset (&wndclass, 0, sizeof(WNDCLASSEX));

        /* This class is called WinTestWin */
        wndclass.lpszClassName = szMainWndClass;

        /* cbSize gives the size of the structure for extensibility. */
        wndclass.cbSize = sizeof(WNDCLASSEX);

        /* All windows of this class redraw when resized. */
        wndclass.style = CS_HREDRAW | CS_VREDRAW;

        /* All windows of this class use the MainWndProc window function. */
        wndclass.lpfnWndProc = MainWndProc;

        /* This class is used with the current program instance. */
        wndclass.hInstance = hInst;

        /* Use standard application icon and arrow cursor provided by the OS */
        wndclass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
        wndclass.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);

        /* Color the background white */
        wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);

        /*
         * Now register the window class for use.
         */
        RegisterClassEx (&wndclass);

        /*
         * Create our main window using that window class.
         */
        hwndMain = CreateWindow (
                szMainWndClass,             /* Class name */
                "Hello",                    /* Caption */
                WS_OVERLAPPEDWINDOW,        /* Style */
                CW_USEDEFAULT,              /* Initial x (use default) */
                CW_USEDEFAULT,              /* Initial y (use default) */
                CW_USEDEFAULT,              /* Initial x size (use default) */
                CW_USEDEFAULT,              /* Initial y size (use default) */
                NULL,                       /* No parent window */
                NULL,                       /* No menu */
                hInst,                      /* This program instance */
                NULL                        /* Creation parameters */
                );
        
        /*
         * Display the window which we just created (using the nShow
         * passed by the OS, which allows for start minimized and that
         * sort of thing).
         */
        ShowWindow (hwndMain, nShow);
        UpdateWindow (hwndMain);

        /*
         * The main message loop. All messages being sent to the windows
         * of the application (or at least the primary thread) are retrieved
         * by the GetMessage call, then translated (mainly for keyboard
         * messages) and dispatched to the appropriate window procedure.
         * This is the simplest kind of message loop. More complex loops
         * are required for idle processing or handling modeless dialog
         * boxes. When one of the windows calls PostQuitMessage GetMessage
         * will return zero and the wParam of the message will be filled
         * with the argument to PostQuitMessage. The loop will end and
         * the application will close.
         */
        while (GetMessage (&msg, NULL, 0, 0))
        {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
        }
        return msg.wParam;
}

/* EOF */
