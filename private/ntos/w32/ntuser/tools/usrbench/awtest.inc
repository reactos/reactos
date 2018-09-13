// awtest.inc

#include "aw.h"

ULONGLONG AW(msProfPeekMessage)(HDC hdc, ULONG Iter)
{

    MSG msg;
    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        AW(PeekMessage)(&msg, NULL, 0, 0, PM_REMOVE);
    }

    END_TIMER;
}

ULONGLONG AW(msProfDispatchMessage)(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    MSG msg;
    INIT_TIMER;
    AW(WNDCLASS) wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = L("CreateDestroyWindow");

    if (!AW(RegisterClass)(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "14RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = AW(CreateWindow)(L("CreateDestroyWindow"), NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    msg.hwnd = hwnd;
    msg.message = WM_MOUSEMOVE;
    msg.wParam = 1;
    msg.lParam = 2;
    msg.time = 3;
    msg.pt.x = 4;
    msg.pt.y = 5;

    START_TIMER;

    while (ix--)
    {
        AW(DispatchMessage)(&msg);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    AW(UnregisterClass)(L("CreateDestroyWindow"), ghinst);

    RETURN_STOP_TIME;

}

ULONGLONG AW(msProfCharXXX)(HDC hDC, ULONG Iter,
                            BOOL (WINAPI *tester)(IF(char,WCHAR)))
{
    static const IF(char,WCHAR) sample[] =
        L("The quick brown fox jumps over the lazy dog")
        L("0123456789")
        L("Jackdaws love my big sphinx of quartz");
    INIT_TIMER;

    START_TIMER;
    while (ix--) {
        const IF(char,WCHAR)* p = sample;
        while (*p) {
            tester(*p++);
        }
    }
    END_TIMER;
}


ULONGLONG AW(msProfCharUpper)(HDC hDC, ULONG Iter)
{
    return AW(msProfCharXXX)(hDC, Iter, AW(IsCharUpper));
}

ULONGLONG AW(msProfCharLower)(HDC hDC, ULONG Iter)
{
    return AW(msProfCharXXX)(hDC, Iter, AW(IsCharLower));
}

ULONGLONG AW(msProfCharNext)(HDC hDC, ULONG Iter)
{
    static const IF(char,WCHAR) sample[] =
        L("The quick brown fox jumps over the lazy dog")
        L("0123456789")
        L("Jackdaws love my big sphinx of quartz");
    INIT_TIMER;

    START_TIMER;
    while (ix--) {
        const IF(char,WCHAR)* p = sample;
        while (*p) {
            AW(CharNext)(p++);
        }
    }
    END_TIMER;
}

ULONGLONG AW(msProfGetMessage)(HDC hDC, ULONG Iter)
{
    MSG msg;
    AW(WNDCLASS) wc;
    HWND hwnd;
    RECT rect;
    static const IF(char, WCHAR)* className = L("GetMessageTest");
    INIT_TIMER;

    wc.style            = 0;
    wc.lpfnWndProc      = IF(CreateDestroyWndProc, CreateDestroyWndProcW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = className;

    if (!AW(RegisterClass)(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "RegisterClass call failed.",
                "ERROR!", MB_OK);
        return FALSE;
    }

    hwnd = AW(CreateWindow)(className, NULL,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, ghinst, NULL);
    if (hwnd == NULL) {
        MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
            "ERROR!", MB_OK);
            return 0;
    }

    START_TIMER;
    while (ix--) {
        START_OVERHEAD;
        AW(PostMessage)(hwnd, WM_MOUSEMOVE, 0, 0);
        END_OVERHEAD;
        AW(GetMessage)(&msg, hwnd, 0, 0);
    }
    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    AW(UnregisterClass)(className, ghinst);

    RETURN_STOP_TIME;
}

ULONGLONG AW(msProfGetWindowTextLength)(HDC hdc, ULONG Iter)
{
    INIT_TIMER;
    static const IF(CHAR, WCHAR)* className = L("GetWindowTextLengthTest");
    AW(WNDCLASS) wc;
    HWND hwnd;

    wc.style            = 0;
    wc.lpfnWndProc      = AW(DefWindowProc);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = className;

    if (!AW(RegisterClass)(&wc)) {
        DispErrorMsg("RegisterClass");
        return FALSE;
    }

    hwnd = AW(CreateWindow)(className, NULL,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, ghinst, NULL);
    if (hwnd == NULL) {
        DispErrorMsg("CreateWindow");
    }

    AW(SetWindowText)(hwnd, L("The quick brown fox jumps over the lazy dog"));

    START_TIMER;

    while (ix--)
    {
        AW(GetWindowTextLength)(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    AW(UnregisterClass)(className, ghinst);

    RETURN_STOP_TIME;
}
