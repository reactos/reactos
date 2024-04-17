// Released to the Public Domain by Doug Lyons on April 16th, 2024.

#include <windows.h>
#include <stdio.h>

WCHAR szWindowClass[] = L"testclass";

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hBmp;
    HANDLE handle;
    CHAR buffer[32];

    switch (message)
    {
        case WM_CREATE:
        {
            handle = LoadLibraryExW(L"image.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
            sprintf(buffer, "%p", handle);
            MessageBoxA(NULL, buffer, "handle", 0);
            hBmp = (HBITMAP)LoadImage(handle, MAKEINTRESOURCE(130), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            sprintf(buffer, "%p", hBmp);
            MessageBoxA(NULL, buffer, "Bmp", 0);
            sprintf(buffer, "%ld", GetLastError());
            MessageBoxA(NULL, buffer, "LastError", 0);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;
            BITMAP bitmap;
            BITMAPINFO bmi;
            hdc = BeginPaint(hWnd, &ps);
            HGLOBAL hMem;
            LPVOID lpBits;

            hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, hBmp);
            GetObject(hBmp, sizeof(BITMAP), &bitmap);

            memset(&bmi, 0, sizeof(bmi));
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = bitmap.bmWidth;
            bmi.bmiHeader.biHeight      = bitmap.bmHeight;
            bmi.bmiHeader.biPlanes      = bitmap.bmPlanes;
            bmi.bmiHeader.biBitCount    = bitmap.bmBitsPixel;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage   = 0;

            hMem = GlobalAlloc(GMEM_MOVEABLE, ((bitmap.bmWidth *
                bmi.bmiHeader.biBitCount + 31) / 32) * 4 * bitmap.bmHeight);
            lpBits = GlobalLock(hMem);
            GetDIBits(hdc, hBmp, 0, bitmap.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);

            // increasing the multiplier makes the image larger
            StretchDIBits(hdc, 0, 0, bitmap.bmWidth * 3, bitmap.bmHeight * 3, 0, 0,
                bitmap.bmWidth, bitmap.bmHeight,lpBits, &bmi, DIB_RGB_COLORS, SRCCOPY);
            GlobalUnlock(hMem);
            GlobalFree(hMem);

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

    hWnd = CreateWindowEx(0,
                          szWindowClass,
                          L"Bmp test",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          300,
                          120,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

int WINAPI
wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    MSG msg;

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
