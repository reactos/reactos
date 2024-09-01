/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for RedrawWindow
 * COPYRIGHT:   Copyright 2018 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2024 Tomáš Veselý <turican0@gmail.com>
 */

#include "precomp.h"

static DWORD dwThreadId;
static BOOL got_paint;

BOOL resultWmEraseGnd = FALSE;
BOOL resultWmNcPaint = FALSE;
int paintIndex = 0;
const wchar_t CHILD_CLASS_NAME[] = L"ChildWindowClass";

static
LRESULT
CALLBACK
WndProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    disable_success_count
    ok(GetCurrentThreadId() == dwThreadId, "Thread 0x%lx instead of 0x%lx\n", GetCurrentThreadId(), dwThreadId);
    if (message == WM_PAINT)
    {
        got_paint = TRUE;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void GetMessageRedrawWindowTest()
{
    HWND hWnd;
    MSG msg;
    HRGN hRgn;
    BOOL ret;
    int i;

    SetCursorPos(0,0);

    dwThreadId = GetCurrentThreadId();
    RegisterSimpleClass(WndProc, L"CreateTest");

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, 0,  10, 10, 20, 20,  NULL, NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");

    ShowWindow(hWnd, SW_SHOW);

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        DispatchMessageA( &msg );
    }

    ok(got_paint == TRUE, "Did not process WM_PAINT message\n");
    got_paint = FALSE;

    hRgn = CreateRectRgn(0, 0, 1, 1);
    ok(hRgn != NULL, "CreateRectRgn failed\n");
    ret = RedrawWindow(hWnd, NULL, hRgn, RDW_INTERNALPAINT | RDW_INVALIDATE);
    ok(ret == TRUE, "RedrawWindow failed\n");

    i = 0;
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        RECORD_MESSAGE(1, msg.message, POST, 0, 0);
        if (msg.message == WM_PAINT)
        {
            i++;
            if (i == 10)
            {
                ok(got_paint == FALSE, "Received unexpected WM_PAINT message\n");
            }
        }
        if (msg.message != WM_PAINT || i >= 10)
        {
            DispatchMessageA( &msg );
        }
    }

    ok(i == 10, "Received %d WM_PAINT messages\n", i);
    ok(got_paint == TRUE, "Did not process WM_PAINT message\n");

    TRACE_CACHE();

    DeleteObject(hRgn);
    DestroyWindow(hWnd);
}

void DrawContent(HDC hdc, RECT* rect, COLORREF color)
{
    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hdc, rect, hBrush);
    DeleteObject(hBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextW(hdc, L"Test RedrawWindow", -1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            DrawContent(hdc, &rect, RGB(0, 255, 0));
            EndPaint(hwnd, &ps);
            paintIndex++;
            return 0;
        }
        case WM_ERASEBKGND:
        {
            if(paintIndex != 0)
                resultWmEraseGnd = TRUE;
            return 0;
        }
        case WM_NCPAINT:
        {
            if (paintIndex != 0)
                resultWmNcPaint = TRUE;
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ChildWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_SYNCPAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
            FillRect(hdc, &ps.rcPaint, brush);
            DeleteObject(brush);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
            FillRect(hdc, &ps.rcPaint, brush);
            DeleteObject(brush);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

typedef struct STRUCT_TestRedrawWindow
{
    const wchar_t* testName;
    DWORD flags;
    BOOL useRegion;
    RECT regRect;
    BOOL useRect;
    RECT rectRect;
    BOOL forcePaint;
    BOOL redrawResult;
    int testPixelPre1x;
    int testPixelPre1y;
    int testPixelPre2x;
    int testPixelPre2y;
    int testPixelPost1x;
    int testPixelPost1y;
    int testPixelPost2x;
    int testPixelPost2y;
    int testChild;
    COLORREF resultColorPre1;
    COLORREF resultColorPre2;
    COLORREF resultColorPost1;
    COLORREF resultColorPost2;
    RECT resultUpdateRect;
    BOOL resultNeedsUpdate;
    BOOL resultRedraw;
    BOOL resultWmEraseGnd;
    BOOL resultWmNcPaint;
    int resultPaintIndex;
} STRUCT_TestRedrawWindow;

typedef struct STRUCT_TestRedrawWindowCompare
{
    COLORREF resultColorPre1;
    COLORREF resultColorPre2;
    COLORREF resultColorPost1;
    COLORREF resultColorPost2;
    RECT resultUpdateRect;
    BOOL resultNeedsUpdate;
    BOOL resultWmEraseGnd;
    BOOL resultWmNcPaint;
    int resultPaintIndex;
} STRUCT_TestRedrawWindowCompare;

void ServeSomeMessages(int messageTime, int messageCount)
{
    DWORD startTime;

    MSG msg = { 0 };
    startTime = GetTickCount();
    while (GetTickCount() - startTime < messageTime * messageCount)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Sleep(messageTime);
        }
    }
}

void TestRedrawWindow(STRUCT_TestRedrawWindow* ptestRW) {
    DWORD style;
    int width;
    int height;
    HWND hChildWnd = NULL;
    HRGN RgnUpdate;
    RECT* prect;

    resultWmEraseGnd = FALSE;
    resultWmNcPaint = FALSE;
    paintIndex = 0;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = ptestRW->testName;
    RegisterClassW(&wc);
    RECT rectWin = { 0, 0, 800, 600 };
    style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&rectWin, style, FALSE, 0);
    width = rectWin.right - rectWin.left;
    height = rectWin.bottom - rectWin.top;
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (hwnd == NULL)
        return;

    ShowWindow(hwnd, SW_SHOW);
    if(!ptestRW->testChild)
        UpdateWindow(hwnd);

    if (ptestRW->testChild)
    {
        WNDCLASSW wcChild = { 0 };
        wcChild.lpfnWndProc = ChildWindowProc;
        wcChild.hInstance = GetModuleHandle(NULL);
        wcChild.lpszClassName = CHILD_CLASS_NAME;
        RegisterClassW(&wcChild);

        hChildWnd = CreateWindowExW(
            0,
            CHILD_CLASS_NAME,
            L"Child Window",
            WS_CHILD | WS_VISIBLE,
            10, 10, 200, 200,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
    }

    HDC hdc = GetDC(hwnd);
    RECT drect = { 0, 0, 800, 600 };
    DrawContent(hdc, &drect, RGB(255, 0, 0));
    ReleaseDC(hwnd, hdc);

    RgnUpdate = NULL;
    if (ptestRW->useRegion)
    {
        RgnUpdate = CreateRectRgn(ptestRW->regRect.left, ptestRW->regRect.top, ptestRW->regRect.right, ptestRW->regRect.bottom);
    }

    prect=NULL;
    if (ptestRW->useRect)
    {
        prect = &ptestRW->rectRect;
    }

    if (ptestRW->testChild)
    {
        ServeSomeMessages(10, 10);
    }

    ptestRW->resultRedraw = RedrawWindow(hwnd, prect, RgnUpdate, ptestRW->flags);

    if (ptestRW->testChild)
    {
        ServeSomeMessages(10, 10);
    }

    hdc = GetDC(hwnd);
    ptestRW->resultColorPre1 = GetPixel(hdc, ptestRW->testPixelPre1x, ptestRW->testPixelPre1y);
    ptestRW->resultColorPre2 = GetPixel(hdc, ptestRW->testPixelPre2x, ptestRW->testPixelPre2y);
    ReleaseDC(hwnd, hdc);

    ptestRW->resultNeedsUpdate = GetUpdateRect(hwnd, &ptestRW->resultUpdateRect, FALSE);

    if (ptestRW->forcePaint)
    {
        UpdateWindow(hwnd);
    }

    hdc = GetDC(hwnd);
    ptestRW->resultColorPost1 = GetPixel(hdc, ptestRW->testPixelPost1x, ptestRW->testPixelPost1y);
    ptestRW->resultColorPost2 = GetPixel(hdc, ptestRW->testPixelPost2x, ptestRW->testPixelPost2y);
    ReleaseDC(hwnd, hdc);

    ptestRW->resultWmEraseGnd = resultWmEraseGnd;
    ptestRW->resultWmNcPaint = resultWmNcPaint;
    ptestRW->resultPaintIndex = paintIndex;

    if (RgnUpdate) DeleteObject(RgnUpdate);

    if (hChildWnd != NULL)
        DestroyWindow(hChildWnd);
    if (hwnd != NULL)
        DestroyWindow(hwnd);
}

UINT TestRedrawWindow2(STRUCT_TestRedrawWindow* ptestRW, STRUCT_TestRedrawWindowCompare* ptestRWcompare)
{
    UINT countErrors = 0;

    TestRedrawWindow(ptestRW);

    if (ptestRW->resultRedraw)
    {
        if (ptestRWcompare->resultColorPre1 != ptestRW->resultColorPre1)
        {
            trace("DIFFERENCE-resultColorPre1 0x%06x 0x%06x\n", (int)ptestRW->resultColorPre1, (int)ptestRWcompare->resultColorPre1);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPre2 != ptestRW->resultColorPre2)
        {
            trace("DIFFERENCE-resultColorPre2 0x%06x 0x%06x\n", (int)ptestRW->resultColorPre2, (int)ptestRWcompare->resultColorPre2);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost1 != ptestRW->resultColorPost1)
        {
            trace("DIFFERENCE-resultColorPost1 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost1, (int)ptestRWcompare->resultColorPost1);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost2 != ptestRW->resultColorPost2)
        {
            trace("DIFFERENCE-resultColorPost2 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost2, (int)ptestRWcompare->resultColorPost2);
            countErrors++;
        }
        if (ptestRWcompare->resultNeedsUpdate != ptestRW->resultNeedsUpdate)
        {
            trace("DIFFERENCE-resultNeedsUpdate %d %d\n", ptestRW->resultNeedsUpdate, ptestRWcompare->resultNeedsUpdate);
            countErrors++;
        }
        if (ptestRW->resultNeedsUpdate)
        {
            if (ptestRWcompare->resultUpdateRect.left != ptestRW->resultUpdateRect.left)
            {
                trace("DIFFERENCE-resultUpdateRect.left %d %d\n", (int)ptestRW->resultUpdateRect.left, (int)ptestRWcompare->resultUpdateRect.left);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.top != ptestRW->resultUpdateRect.top)
            {
                trace("DIFFERENCE-resultUpdateRect.top %d %d\n", (int)ptestRW->resultUpdateRect.top, (int)ptestRWcompare->resultUpdateRect.top);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.right != ptestRW->resultUpdateRect.right)
            {
                trace("DIFFERENCE-resultUpdateRect.right %d %d\n", (int)ptestRW->resultUpdateRect.right, (int)ptestRWcompare->resultUpdateRect.right);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.bottom != ptestRW->resultUpdateRect.bottom)
            {
                trace("DIFFERENCE-resultUpdateRect.bottom %d %d\n", (int)ptestRW->resultUpdateRect.bottom, (int)ptestRWcompare->resultUpdateRect.bottom);
                countErrors++;
            }
        }
        if (ptestRWcompare->resultWmEraseGnd != ptestRW->resultWmEraseGnd)
        {
            trace("DIFFERENCE-resultWmEraseGnd %d %d\n", ptestRW->resultWmEraseGnd, ptestRWcompare->resultWmEraseGnd);
            countErrors++;
        }
        if (ptestRWcompare->resultWmNcPaint != ptestRW->resultWmNcPaint)
        {
            trace("DIFFERENCE-resultWmNcPaint %d %d\n", ptestRW->resultWmNcPaint, ptestRWcompare->resultWmNcPaint);
            countErrors++;
        }
        if (ptestRWcompare->resultPaintIndex != ptestRW->resultPaintIndex)
        {
            trace("DIFFERENCE-resultPaintIndex %d %d\n", ptestRW->resultPaintIndex, ptestRWcompare->resultPaintIndex);
            countErrors++;
        }
    }
    if (countErrors > 0)
    {
        trace("COUNT OF DIFFERENCES - %d\n", countErrors);
    }

    return countErrors;
}

void InitRect(RECT *rect, int left, int top, int right, int bottom) {
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

void FlagsRedrawWindowTest()
{
    STRUCT_TestRedrawWindow testRW;
    STRUCT_TestRedrawWindowCompare testRWcompare;

    testRW.testPixelPre1x = 50;
    testRW.testPixelPre1y = 50;
    testRW.testPixelPre2x = 50;
    testRW.testPixelPre2y = 550;
    testRW.testPixelPost1x = 50;
    testRW.testPixelPost1y = 50;
    testRW.testPixelPost2x = 50;
    testRW.testPixelPost2y = 550;
    
    // RDW_ERASE tests    
    testRW.testName = L"Test1";
    testRW.flags = 0;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 200, 200);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test1 fail\n");

    testRW.testName = L"Test2";
    testRW.flags = RDW_ERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 200, 200);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test2 fail\n");

    testRW.testName = L"Test3";
    testRW.flags = RDW_INVALIDATE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test3 fail\n");

    testRW.testName = L"Test4";
    testRW.flags = RDW_INVALIDATE | RDW_ERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test4 fail\n");

    // RDW_FRAME tests
    testRW.testName = L"Test5";
    testRW.flags = RDW_FRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test5 fail\n");

    testRW.testName = L"Test6";
    testRW.flags = RDW_INVALIDATE | RDW_FRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = TRUE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test6 fail\n");

    // RDW_INTERNALPAINT tests
    testRW.testName = L"Test7";
    testRW.flags = RDW_INTERNALPAINT;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test7 fail\n");

    testRW.testName = L"Test8";
    testRW.flags = RDW_INVALIDATE | RDW_INTERNALPAINT;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test8 fail\n");

    // RDW_NOERASE tests
    testRW.testName = L"Test9";
    testRW.flags = RDW_NOERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test9 fail\n");

    testRW.testName = L"Test10";
    testRW.flags = RDW_INVALIDATE | RDW_NOERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test10 fail\n");

    testRW.testName = L"Test11";
    testRW.flags = RDW_NOERASE | RDW_ERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test11 fail\n");

    testRW.testName = L"Test12";
    testRW.flags = RDW_INVALIDATE | RDW_NOERASE | RDW_ERASE;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test12 fail\n");

    // RDW_NOFRAME tests
    testRW.testName = L"Test13";
    testRW.flags = RDW_NOFRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test13 fail\n");

    testRW.testName = L"Test14";
    testRW.flags = RDW_INVALIDATE | RDW_NOFRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test14 fail\n");

    testRW.testName = L"Test15";
    testRW.flags = RDW_INVALIDATE | RDW_VALIDATE | RDW_NOFRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test15 fail\n");

    testRW.testName = L"Test16";
    testRW.flags = RDW_VALIDATE | RDW_NOFRAME;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test16 fail\n");

    // RDW_NOINTERNALPAINT tests
    testRW.testName = L"Test17";
    testRW.flags = RDW_NOINTERNALPAINT;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test17 fail\n");

    testRW.testName = L"Test18";
    testRW.flags = RDW_INVALIDATE | RDW_NOINTERNALPAINT;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test18 fail\n");

    testRW.testName = L"Test19";
    testRW.flags = RDW_VALIDATE | RDW_NOINTERNALPAINT;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test19 fail\n");

    // RDW_ERASENOW tests
    testRW.testName = L"Test20";
    testRW.flags = RDW_ERASENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test20 fail\n");

    testRW.testName = L"Test21";
    testRW.flags = RDW_INVALIDATE | RDW_ERASENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test21 fail\n");

    testRW.testName = L"Test22";
    testRW.flags = RDW_VALIDATE | RDW_ERASENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test22 fail\n");

    // RDW_UPDATENOW tests
    testRW.testName = L"Test23";
    testRW.flags = RDW_UPDATENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test23 fail\n");

    testRW.testName = L"Test24";
    testRW.flags = RDW_INVALIDATE | RDW_UPDATENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test24 fail\n");

    testRW.testName = L"Test25";
    testRW.flags = RDW_VALIDATE | RDW_UPDATENOW;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test25 fail\n");

    // RDW_ALLCHILDREN/RDW_NOCHILDREN tests
    testRW.testName = L"Test26";
    testRW.flags = RDW_NOCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test26 fail\n");

    testRW.testName = L"Test27";
    testRW.flags = RDW_INVALIDATE | RDW_NOCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x0000FF00;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x0000FF00;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test27 fail\n");

    testRW.testName = L"Test28";
    testRW.flags = RDW_VALIDATE | RDW_NOCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test28 fail\n");

    testRW.testName = L"Test29";
    testRW.flags = RDW_ALLCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test29 fail\n");

    testRW.testName = L"Test30";
    testRW.flags = RDW_INVALIDATE | RDW_ALLCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test30 fail\n");

    testRW.testName = L"Test31";
    testRW.flags = RDW_VALIDATE | RDW_ALLCHILDREN;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test31 fail\n");

    testRW.testName = L"Test32";
    testRW.flags = RDW_NOCHILDREN;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test32 fail\n");

    testRW.testName = L"Test33";
    testRW.flags = RDW_INVALIDATE | RDW_NOCHILDREN;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test33 fail\n");

    testRW.testName = L"Test34";
    testRW.flags = RDW_VALIDATE | RDW_NOCHILDREN;
    testRW.useRegion = TRUE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = FALSE;
    InitRect(&testRW.rectRect, 0, 0, 200, 200);
    testRW.forcePaint = TRUE;
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test34 fail\n");

    // rect defined area tests
    testRW.testName = L"Test35";
    testRW.flags = 0;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = TRUE;
    InitRect(&testRW.rectRect, 0, 500, 800, 600);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 200, 200);
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test35 fail\n");

    testRW.testName = L"Test36";
    testRW.flags = RDW_INVALIDATE | RDW_ERASENOW;
    testRW.useRegion = FALSE;
    InitRect(&testRW.regRect, 0, 500, 800, 600);
    testRW.useRect = TRUE;
    InitRect(&testRW.rectRect, 0, 500, 800, 600);
    testRW.forcePaint = TRUE;
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x000000FF;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPost1 = 0x000000FF;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 500, 800, 600);
    testRWcompare.resultNeedsUpdate = TRUE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    ok(0 == TestRedrawWindow2(&testRW, &testRWcompare),"Test36 fail\n");
}

START_TEST(RedrawWindow)
{    
    FlagsRedrawWindowTest();
    GetMessageRedrawWindowTest();
}
