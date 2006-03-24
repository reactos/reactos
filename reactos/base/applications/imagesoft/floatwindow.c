#include <precomp.h>

static const TCHAR szFloatWndClass[] = TEXT("ImageSoftFloatWndClass");

#define ID_TIMER 1

BOOL
ShowHideWindow(HWND hwnd)
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
        static BOOL bOpaque = FALSE;

        case WM_CREATE:

            SetWindowLong(hwnd,
                          GWL_EXSTYLE,
                          GetWindowLong(hwnd,
                                        GWL_EXSTYLE) | WS_EX_LAYERED);

            /* set the tranclucency to 60% */
            SetLayeredWindowAttributes(hwnd,
                                       0,
                                       (255 * 60) / 100,
                                       LWA_ALPHA);

        break;

        case WM_TIMER:
        {
            POINT pt;

            if (bOpaque != TRUE)
            {
                KillTimer(hwnd,
                          ID_TIMER);
                break;
            }

            if (GetCursorPos(&pt))
            {
                RECT rect;

                if (GetWindowRect(hwnd,
                                  &rect))
                {
                    if (! PtInRect(&rect,
                                   pt))
                    {
                        KillTimer(hwnd,
                                  ID_TIMER);

                        bOpaque = FALSE;

                        SetWindowLong(hwnd,
                                      GWL_EXSTYLE,
                                      GetWindowLong(hwnd,
                                                    GWL_EXSTYLE) | WS_EX_LAYERED);

                        /* set the tranclucency to 60% */
                        SetLayeredWindowAttributes(hwnd,
                                                   0,
                                                   (255 * 60) / 100,
                                                   LWA_ALPHA);

                    }
                }
            }
        }
        break;

        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
            if (bOpaque == FALSE)
            {
                SetWindowLong(hwnd,
                          GWL_EXSTYLE,
                          GetWindowLong(hwnd,
                                        GWL_EXSTYLE) & ~WS_EX_LAYERED);

                RedrawWindow(hwnd,
                             NULL,
                             NULL,
                             RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

                bOpaque = TRUE;
                SetTimer(hwnd,
                         ID_TIMER,
                         200,
                         NULL);
            }
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
                ShowHideWindow(hwnd);

            switch(LOWORD(wParam))
            {
                case IDC_PRESS:
                    MessageBox(hwnd, _T("Kapow!"), _T("Hit test"),
                    MB_OK | MB_ICONEXCLAMATION);
                break;
            }
        break;

        case WM_NCACTIVATE:
            /* FIXME: needs fully implementing */
            return DefWindowProc(hwnd,
                                 Message,
                                 TRUE,
                                 lParam);

        case WM_CLOSE:
            ShowHideWindow(hwnd);
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

