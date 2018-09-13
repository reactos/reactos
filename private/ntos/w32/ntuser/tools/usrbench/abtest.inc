// abtest.inc

#include "ab.h"

ULONGLONG FAW(msProfSendMessage)(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    AW2(WNDCLASS) wc;

    wc.style            = 0;
    wc.lpfnWndProc      = IF2(CreateDestroyWndProc, CreateDestroyWndProcW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = L2("CreateDestroyWindow");

    if (!AW2(RegisterClass)(&wc)) {
        MessageBox(NULL, "18RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = AW2(CreateWindow)(L2("CreateDestroyWindow"), NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        AW1(SendMessage)(hwnd, WM_SETTEXT, 0, (LPARAM)L1("A fairly reasonable bit of text"));
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    AW2(UnregisterClass)(L2("CreateDestroyWindow"), ghinst);

    RETURN_STOP_TIME;

}

#undef A1
#undef A2
