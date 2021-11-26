/*
 * Shows the 15 well known BitBlt raster operations
 * using src, dest, pattern, a background brush and color.
 *
 * Created by Gregor Schneider <grschneider AT gmail DOT com>, November 2008
*/

#include <windows.h>
#include <tchar.h>

HINSTANCE hInst;
TCHAR szWindowClass[] = _T("testclass");

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hBmpTest;

    switch (message)
    {
        case WM_CREATE:
        {
            hBmpTest = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(100), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;
            BITMAP bitmap;
            HBRUSH brush, brush2;
            INT l;

            hdc = BeginPaint(hWnd, &ps);
            hdcMem = CreateCompatibleDC(hdc);

            GetObject(hBmpTest, sizeof(BITMAP), &bitmap);

            /* fill destination with brush */
            brush = CreateHatchBrush(HS_DIAGCROSS, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 30, 0, 4*bitmap.bmWidth*2, 4*bitmap.bmHeight, PATCOPY);

            /* hatched brushes */
            l = 66;
            brush = CreateHatchBrush(HS_DIAGCROSS, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 0, 30, l, PATCOPY);
            DeleteObject(brush);

            brush = CreateHatchBrush(HS_CROSS, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 1*l, 30, l, PATCOPY);
            DeleteObject(brush);

            brush = CreateHatchBrush(HS_FDIAGONAL, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 2*l, 30, l, PATCOPY);
            DeleteObject(brush);

            brush = CreateHatchBrush(HS_BDIAGONAL, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 3*l, 30, l, PATCOPY);
            DeleteObject(brush);

            brush = CreateHatchBrush(HS_VERTICAL, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 4*l, 30, l, PATCOPY);
            DeleteObject(brush);

            brush = CreateHatchBrush(HS_HORIZONTAL, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 5*l, 30, l, PATCOPY);
            DeleteObject(brush);

            /* set up a second brush */
            brush2 = CreateHatchBrush(HS_VERTICAL, RGB(127,127,127));

            /* first select brush, then set bk color */
            SelectObject(hdc, brush2);
            SetBkColor(hdc, RGB(0, 255, 0));

            /* 15 blt op's with bitblt */
            SelectObject(hdcMem, hBmpTest);
            /* offset coordinates */
            SetWindowOrgEx(hdc, -10, -10, NULL);
            BitBlt(hdc, 30,  0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, 130, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, DSTINVERT);
            BitBlt(hdc, 230, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, MERGECOPY);
            BitBlt(hdc, 330, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, MERGEPAINT);

            BitBlt(hdc, 30,  100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, NOTSRCCOPY);
            BitBlt(hdc, 130, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, NOTSRCERASE);
            BitBlt(hdc, 230, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATCOPY);
            BitBlt(hdc, 330, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATINVERT);

            BitBlt(hdc, 30,  200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATPAINT);
            BitBlt(hdc, 130, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCAND);
            BitBlt(hdc, 230, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCERASE);
            BitBlt(hdc, 330, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCINVERT);

            BitBlt(hdc, 30,  300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, BLACKNESS);
            BitBlt(hdc, 130, 300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCPAINT);
            BitBlt(hdc, 230, 300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, WHITENESS);

            /* 15 blt op's with stretchblt */
            StretchBlt(hdc, 30+400,  0, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
            StretchBlt(hdc, 130+400, 0, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, DSTINVERT);
            StretchBlt(hdc, 230+400, 0, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, MERGECOPY);
            StretchBlt(hdc, 330+400, 0, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, MERGEPAINT);

            StretchBlt(hdc, 30+400,  100, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, NOTSRCCOPY);
            StretchBlt(hdc, 130+400, 100, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, NOTSRCERASE);
            StretchBlt(hdc, 230+400, 100, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, PATCOPY);
            StretchBlt(hdc, 330+400, 100, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, PATINVERT);

            StretchBlt(hdc, 30+400,  200, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, PATPAINT);
            StretchBlt(hdc, 130+400, 200, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCAND);
            StretchBlt(hdc, 230+400, 200, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCERASE);
            StretchBlt(hdc, 330+400, 200, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCINVERT);

            StretchBlt(hdc, 30+400,  300, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, BLACKNESS);
            StretchBlt(hdc, 130+400, 300, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCPAINT);
            StretchBlt(hdc, 230+400, 300, bitmap.bmWidth/2, bitmap.bmHeight/2, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, WHITENESS);

            DeleteObject(brush);
            DeleteObject(brush2);
            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


static ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = NULL;

    return RegisterClassEx(&wcex);
}


static BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance;

   hWnd = CreateWindowEx(0,
                         szWindowClass,
                         _T("BitBlt raster operation test"),
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         840,
                         440,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


int WINAPI
_tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
