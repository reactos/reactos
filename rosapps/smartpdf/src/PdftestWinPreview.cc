/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "win_util.h"
#include "PdfEngine.h"

#include <assert.h>

#define WIN_CLASS_NAME  "PDFTEST_PDF_WIN"
#define COL_WINDOW_BG RGB(0xff, 0xff, 0xff)

static HWND             gHwndSplash;
static HWND             gHwndFitz;
static HBRUSH           gBrushBg;

static RenderedBitmap *gBmpSplash, *gBmpFitz;

/* Set the client area size of the window 'hwnd' to 'dx'/'dy'. */
static void resizeClientArea(HWND hwnd, int x, int dx, int dy, int *dx_out)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if ((rect_dx(&rc) == dx) && (rect_dy(&rc) == dy))
        return;

    RECT rw;
    GetWindowRect(hwnd, &rw);
    int win_dx = rect_dx(&rw) + (dx - rect_dx(&rc));
    int win_dy = rect_dy(&rw) + (dy - rect_dy(&rc));
    SetWindowPos(hwnd, NULL, x, 0, win_dx, win_dy, SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOZORDER);
    if (dx_out)
        *dx_out = win_dx;
}

static void resizeClientAreaToRenderedBitmap(HWND hwnd, RenderedBitmap *bmp, int x, int *dxOut)
{
    int dx = bmp->dx();
    int dy = bmp->dy();
    resizeClientArea(hwnd, x, dx, dy, dxOut);
}

static void drawBitmap(HWND hwnd, RenderedBitmap *bmp)
{
    PAINTSTRUCT     ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    SetBkMode(hdc, TRANSPARENT);
    FillRect(hdc, &ps.rcPaint, gBrushBg);

    HBITMAP hbmp = bmp->createDIBitmap(hdc);
    if (hbmp) {
        HDC bmpDC = CreateCompatibleDC(hdc);
        if (bmpDC) {
            SelectObject(bmpDC, hbmp);
            int xSrc = 0, ySrc = 0;
            int xDest = 0, yDest = 0;
            int bmpDx = bmp->dx();
            int bmpDy = bmp->dy();
            BitBlt(hdc, xDest, yDest, bmpDx, bmpDy, bmpDC, xSrc, ySrc, SRCCOPY);
            DeleteDC(bmpDC);
            bmpDC = NULL;
        }
        DeleteObject(hbmp);
        hbmp = NULL;
    }
    EndPaint(hwnd, &ps);
}

static void onPaint(HWND hwnd)
{
    if (hwnd == gHwndSplash)
        if (gBmpSplash)
            drawBitmap(hwnd, gBmpSplash);
    if (hwnd == gHwndFitz)
        if (gBmpFitz)
            drawBitmap(hwnd, gBmpFitz);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            // do nothing
            break;

        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
            /* it might happen that we get WM_PAINT after destroying a window */
            onPaint(hwnd);
            break;

        case WM_DESTROY:
            /* WM_DESTROY might be sent as a result of File\Close, in which case CloseWindow() has already been called */
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

static BOOL registerWinClass(void)
{
    WNDCLASSEX  wcex;
    ATOM        atom;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = NULL;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = WIN_CLASS_NAME;
    wcex.hIconSm        = NULL;

    atom = RegisterClassEx(&wcex);
    if (atom)
        return TRUE;
    return FALSE;
}

static bool initWinIfNecessary(void)
{
    if (gHwndSplash)
        return true;

    if (!registerWinClass())
        return false;

    gBrushBg = CreateSolidBrush(COL_WINDOW_BG);

    gHwndSplash = CreateWindow(
        WIN_CLASS_NAME, "Splash",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        NULL, NULL,
        NULL, NULL);

    if (!gHwndSplash)
        return false;

    gHwndFitz = CreateWindow(
        WIN_CLASS_NAME, "Fitz",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        NULL, NULL,
        NULL, NULL);

    if (!gHwndFitz)
        return false;

    ShowWindow(gHwndSplash, SW_HIDE);
    ShowWindow(gHwndFitz, SW_HIDE);
    return true;
}

static void pumpMessages(void)
{
    BOOL    isMessage;
    MSG     msg;

    for (;;) {
        isMessage = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (!isMessage)
            return;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void PreviewBitmapInit(void)
{
    /* no need to do anything */
}

static void deleteRenderedBitmaps()
{
    delete gBmpSplash;
    delete gBmpFitz;
}

void PreviewBitmapDestroy(void)
{
    PostQuitMessage(0);
    pumpMessages();
    deleteRenderedBitmaps();
    DeleteObject(gBrushBg);
}

static void UpdateWindows(void)
{
    int fitzDx = 0;

    if (gBmpFitz) {
        resizeClientAreaToRenderedBitmap(gHwndFitz, gBmpFitz, 0, &fitzDx);
        ShowWindow(gHwndFitz, SW_SHOW);
        InvalidateRect(gHwndFitz, NULL, FALSE);
        UpdateWindow(gHwndFitz);
    } else {
        ShowWindow(gHwndFitz, SW_HIDE);
    }

    if (gBmpSplash) {
        resizeClientAreaToRenderedBitmap(gHwndSplash, gBmpSplash, fitzDx, NULL);
        ShowWindow(gHwndSplash, SW_SHOW);
        InvalidateRect(gHwndSplash, NULL, FALSE);
        UpdateWindow(gHwndSplash);
    } else {
        ShowWindow(gHwndSplash, SW_HIDE);
    }

    pumpMessages();
}

void PreviewBitmapSplashFitz(RenderedBitmap *bmpSplash, RenderedBitmap *bmpFitz)
{
    if (!initWinIfNecessary())
        return;

    deleteRenderedBitmaps();
    gBmpSplash = bmpSplash;
    gBmpFitz = bmpFitz;
    UpdateWindows();
}

