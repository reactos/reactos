#include <precomp.h>

static const TCHAR szFloatWndClass[] = TEXT("ImageSoftFloatWndClass");



BOOL
ShowHideToolbar(HWND hwnd)
{
    static BOOL Hidden = FALSE;

    ShowWindow(hwnd, Hidden ? SW_SHOW : SW_HIDE);
    Hidden = ~Hidden;

    return Hidden;
}


LRESULT CALLBACK
FloatToolbarWndProc(HWND hwnd,
                    UINT Message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    switch(Message)
    {
        case WM_CREATE:

        break;

        /*case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
                ShowHideToolbar(hwnd);

            switch(LOWORD(wParam))
            {
                case IDC_PRESS:
                    MessageBox(hwnd, _T("Kapow!"), _T("Hit test"),
                    MB_OK | MB_ICONEXCLAMATION);
                break;
            }
        break;*/

        case WM_NCACTIVATE:
            return DefWindowProc(hwnd, Message, TRUE, lParam);

        case WM_CLOSE:
            ShowHideToolbar(hwnd);
        break;

        default:
            return DefWindowProc(hwnd,
                                 Message,
                                 wParam,
                                 lParam);
    }

    return 0;
}


BOOL
InitFloatWndClass(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FloatToolbarWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szFloatWndClass;
    wc.hIconSm = NULL;

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
UninitFloatWndImpl(VOID)
{
    UnregisterClass(szFloatWndClass,
                    hInstance);
}

