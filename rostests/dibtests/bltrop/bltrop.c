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

            hdc = BeginPaint(hWnd, &ps);
            hdcMem = CreateCompatibleDC(hdc);

            GetObject(hBmpTest, sizeof(BITMAP), &bitmap);

            /* fill destination with brush */
            brush = CreateHatchBrush(HS_DIAGCROSS, RGB(255,0,0));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 0, 4*bitmap.bmWidth, 4*bitmap.bmHeight, PATCOPY);
            /* set up a second brush */
            brush2 = CreateHatchBrush(HS_VERTICAL, RGB(127,127,127));

            /* first select brush, then set bk color */
            SelectObject(hdc, brush2);
            SetBkColor(hdc, RGB(0, 255, 0));

            /* 15 blt op's */
            SelectObject(hdcMem, hBmpTest);
            BitBlt(hdc, 0,   0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, 100, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, DSTINVERT);
            BitBlt(hdc, 200, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, MERGECOPY);
            BitBlt(hdc, 300, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, MERGEPAINT);

            BitBlt(hdc, 0,   100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, NOTSRCCOPY);
            BitBlt(hdc, 100, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, NOTSRCERASE);
            BitBlt(hdc, 200, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATCOPY);
            BitBlt(hdc, 300, 100, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATINVERT);

            BitBlt(hdc, 0,   200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, PATPAINT);
            BitBlt(hdc, 100, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCAND);
            BitBlt(hdc, 200, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCERASE);
            BitBlt(hdc, 300, 200, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCINVERT);

            BitBlt(hdc, 0,   300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, BLACKNESS);
            BitBlt(hdc, 100, 300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCPAINT);
            BitBlt(hdc, 200, 300, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, WHITENESS);

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
                         440,
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
