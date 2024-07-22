/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ScrollWindowEx
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

BOOL resultWmEraseGnd = FALSE;
BOOL resultWmNcPaint = FALSE;
int paintIndex = 0;
const wchar_t CHILD_CLASS_NAME[] = L"ChildWindowClass";

typedef struct STRUCT_TestRedrawWindow {
    const wchar_t* testName;
    DWORD flags;
    //int winw;
    //int winh;
    BOOL usePrcScroll;
    RECT prcScroll;
    BOOL usePrcClip;
    RECT prcClip;
    BOOL useHrgnUpdate;
    RECT hrgnUpdate;
    //bool forcePaint;
    BOOL redrawResult;
    int testPixelPre1x;
    int testPixelPre1y;
    int testPixelPre2x;
    int testPixelPre2y;
    int testPixelPre3x;
    int testPixelPre3y;
    int testPixelPost1x;
    int testPixelPost1y;
    int testPixelPost2x;
    int testPixelPost2y;
    int testPixelPost3x;
    int testPixelPost3y;
    int testPixelPost4x;
    int testPixelPost4y;
    int testChild;
	int scrollX;
	int scrollY;

    SCROLLINFO horScrollInfo;
    SCROLLINFO vertScrollInfo;
    SCROLLINFO ctlScrollInfo;

    COLORREF resultColorPre1;
    COLORREF resultColorPre2;
    COLORREF resultColorPre3;
    COLORREF resultColorPost1;
    COLORREF resultColorPost2;
    COLORREF resultColorPost3;
    COLORREF resultColorPost4;
    RECT resultUpdateRect;
    BOOL resultNeedsUpdate;
    BOOL resultRedraw;
    BOOL resultWmEraseGnd;
    BOOL resultWmNcPaint;
    int resultPaintIndex;
    RECT prcUpdate;
} STRUCT_TestRedrawWindow;

typedef struct STRUCT_TestRedrawWindowCompare {
    COLORREF resultColorPre1;
    COLORREF resultColorPre2;
    COLORREF resultColorPre3;
    COLORREF resultColorPost1;
    COLORREF resultColorPost2;
    COLORREF resultColorPost3;
    COLORREF resultColorPost4;
    RECT resultUpdateRect;
    BOOL resultNeedsUpdate;
    BOOL resultWmEraseGnd;
    BOOL resultWmNcPaint;
    int resultPaintIndex;
    RECT hrgnUpdate;
    RECT prcUpdate;
} STRUCT_TestRedrawWindowCompare;

void DrawContent(HDC hdc, RECT* rect, COLORREF color) {
    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hdc, rect, hBrush);
    DeleteObject(hBrush);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
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
        if (paintIndex != 0)
            resultWmEraseGnd = TRUE;
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

LRESULT CALLBACK ChildWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SYNCPAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(hwnd, &ps);
    } break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(hwnd, &ps);
    } break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

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

void TestScrollWindowEx(STRUCT_TestRedrawWindow* ptestRW) {
    //HDC hdc;
    //RECT rectWin;
    DWORD style;
    int width;
    int height;
    //RECT drect;
    //WNDCLASSW wcChild;
    HWND hChildWnd = NULL;
    HRGN RgnUpdate;
    RECT* pUsePrcScroll;
    RECT* pUsePrcClip;
    //RECT* pHrgnUpdate;

    resultWmEraseGnd = FALSE;
    resultWmNcPaint = FALSE;
    paintIndex = 0;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = ptestRW->testName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    RECT rectWin = { 0, 0, 800, 600 };
    style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&rectWin, style, FALSE, 0);
    width = rectWin.right - rectWin.left;
    height = rectWin.bottom - rectWin.top;
    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        wc.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (hwnd == NULL)
        return;

    ShowWindow(hwnd, SW_SHOW);
    if (ptestRW->flags & SW_SMOOTHSCROLL)
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
    if (ptestRW->useHrgnUpdate) {
        RgnUpdate = CreateRectRgn(ptestRW->hrgnUpdate.left, ptestRW->hrgnUpdate.top, ptestRW->hrgnUpdate.right, ptestRW->hrgnUpdate.bottom);
    }
    pUsePrcScroll = NULL;
    if (ptestRW->usePrcScroll) {
        pUsePrcScroll = &ptestRW->prcScroll;
    }
    pUsePrcClip = NULL;
    if (ptestRW->usePrcClip) {
        pUsePrcClip = &ptestRW->prcClip;
    }

    //ShowWindow(hwnd, SW_SHOW);
    ServeSomeMessages(10, 4);
    //std::this_thread::sleep_for(std::chrono::seconds(10));

    hdc = GetDC(hwnd);
    RECT drect2 = { 0, 0, 100, 100 };
    DrawContent(hdc, &drect2, RGB(0, 0, 255));
    ReleaseDC(hwnd, hdc);

    //ShowWindow(hwnd, SW_SHOW);
    //ServeSomeMessages(10, 10);

    hdc = GetDC(hwnd);
    ptestRW->resultColorPre1 = GetPixel(hdc, ptestRW->testPixelPre1x, ptestRW->testPixelPre1y);
    ptestRW->resultColorPre2 = GetPixel(hdc, ptestRW->testPixelPre2x, ptestRW->testPixelPre2y);
    ptestRW->resultColorPre3 = GetPixel(hdc, ptestRW->testPixelPre3x, ptestRW->testPixelPre3y);
    ReleaseDC(hwnd, hdc);

    ptestRW->resultRedraw = ScrollWindowEx(hwnd, ptestRW->scrollX, ptestRW->scrollY, pUsePrcScroll, pUsePrcClip, RgnUpdate, &ptestRW->prcUpdate, ptestRW->flags);
    //ptestRW->resultRedraw = ScrollWindowEx(hwnd, 0, 300, pUsePrcScroll, pUsePrcClip, RgnUpdate, NULL, ptestRW->flags);

    /*memset(&ptestRW->horScrollInfo, 0, sizeof(ptestRW->horScrollInfo));
    ptestRW->horScrollInfo.cbSize = sizeof(ptestRW->horScrollInfo);
    ptestRW->horScrollInfo.fMask = SIF_ALL;
    memset(&ptestRW->vertScrollInfo, 0, sizeof(ptestRW->vertScrollInfo));
    ptestRW->vertScrollInfo.cbSize = sizeof(ptestRW->vertScrollInfo);
    ptestRW->vertScrollInfo.fMask = SIF_ALL;
    memset(&ptestRW->ctlScrollInfo, 0, sizeof(ptestRW->ctlScrollInfo));
    ptestRW->ctlScrollInfo.cbSize = sizeof(ptestRW->ctlScrollInfo);
    ptestRW->ctlScrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hwnd, SB_HORZ, &ptestRW->horScrollInfo);
    GetScrollInfo(hwnd, SB_VERT, &ptestRW->vertScrollInfo);
    GetScrollInfo(hwnd, SB_CTL, &ptestRW->ctlScrollInfo);*/

    GetRgnBox(RgnUpdate, &ptestRW->hrgnUpdate);
    ShowWindow(hwnd, SW_SHOW);
    //UpdateWindow(hwnd);

    if (ptestRW->flags & SW_SMOOTHSCROLL)
    {
        hdc = GetDC(hwnd);
        ptestRW->resultColorPre1 = GetPixel(hdc, ptestRW->testPixelPre1x, ptestRW->testPixelPre1y);
        ptestRW->resultColorPre2 = GetPixel(hdc, ptestRW->testPixelPre2x, ptestRW->testPixelPre2y);
        ptestRW->resultColorPre3 = GetPixel(hdc, ptestRW->testPixelPre3x, ptestRW->testPixelPre3y);
        ReleaseDC(hwnd, hdc);
    }

    ServeSomeMessages(10, 4);

    //std::this_thread::sleep_for(std::chrono::seconds(10));

    ptestRW->resultNeedsUpdate = GetUpdateRect(hwnd, &ptestRW->resultUpdateRect, FALSE);

    hdc = GetDC(hwnd);
    ptestRW->resultColorPost1 = GetPixel(hdc, ptestRW->testPixelPost1x, ptestRW->testPixelPost1y);
    ptestRW->resultColorPost2 = GetPixel(hdc, ptestRW->testPixelPost2x, ptestRW->testPixelPost2y);
    ptestRW->resultColorPost3 = GetPixel(hdc, ptestRW->testPixelPost3x, ptestRW->testPixelPost3y);
    ptestRW->resultColorPost4 = GetPixel(hdc, ptestRW->testPixelPost4x, ptestRW->testPixelPost4y);
    ReleaseDC(hwnd, hdc);

    ptestRW->resultWmEraseGnd = resultWmEraseGnd;
    ptestRW->resultWmNcPaint = resultWmNcPaint;
    ptestRW->resultPaintIndex = paintIndex;

    if (RgnUpdate) DeleteObject(RgnUpdate);

    //UpdateWindow(hwnd);

    //ShowWindow(hwnd, SW_SHOW);
    //ServeSomeMessages(10, 10);

    /*MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }*/    

    if (hChildWnd != NULL)
        DestroyWindow(hChildWnd);
    if (hwnd != NULL)
        DestroyWindow(hwnd);

}

UINT TestScrollWindowEx2(STRUCT_TestRedrawWindow* ptestRW, STRUCT_TestRedrawWindowCompare* ptestRWcompare)
{
    UINT countErrors = 0;

    TestScrollWindowEx(ptestRW);

    if (ptestRW->resultRedraw)
    {
        if (ptestRWcompare->resultColorPre1 != ptestRW->resultColorPre1)
        {
            trace("ERROR-resultColorPre1 0x%06x 0x%06x\n", (int)ptestRW->resultColorPre1, (int)ptestRWcompare->resultColorPre1);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPre2 != ptestRW->resultColorPre2)
        {
            trace("ERROR-resultColorPre2 0x%06x 0x%06x\n", (int)ptestRW->resultColorPre2, (int)ptestRWcompare->resultColorPre2);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPre3 != ptestRW->resultColorPre3)
        {
            trace("ERROR-resultColorPre3 0x%06x 0x%06x\n", (int)ptestRW->resultColorPre3, (int)ptestRWcompare->resultColorPre3);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost1 != ptestRW->resultColorPost1)
        {
            trace("ERROR-resultColorPost1 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost1, (int)ptestRWcompare->resultColorPost1);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost2 != ptestRW->resultColorPost2)
        {
            trace("ERROR-resultColorPost2 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost2, (int)ptestRWcompare->resultColorPost2);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost3 != ptestRW->resultColorPost3)
        {
            trace("ERROR-resultColorPost3 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost3, (int)ptestRWcompare->resultColorPost3);
            countErrors++;
        }
        if (ptestRWcompare->resultColorPost4 != ptestRW->resultColorPost4)
        {
            trace("ERROR-resultColorPost4 0x%06x 0x%06x\n", (int)ptestRW->resultColorPost4, (int)ptestRWcompare->resultColorPost4);
            countErrors++;
        }
        if (ptestRWcompare->resultNeedsUpdate != ptestRW->resultNeedsUpdate)
        {
            trace("ERROR-resultNeedsUpdate %d %d\n", (int)ptestRW->resultNeedsUpdate, (int)ptestRWcompare->resultNeedsUpdate);
            countErrors++;
        }
        if (ptestRW->resultNeedsUpdate)
        {
            if (ptestRWcompare->resultUpdateRect.left != ptestRW->resultUpdateRect.left)
            {
                trace("ERROR-resultUpdateRect.left %d %d\n", (int)ptestRW->resultUpdateRect.left, (int)ptestRWcompare->resultUpdateRect.left);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.top != ptestRW->resultUpdateRect.top)
            {
                trace("ERROR-resultUpdateRect.top %d %d\n", (int)ptestRW->resultUpdateRect.top, (int)ptestRWcompare->resultUpdateRect.top);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.right != ptestRW->resultUpdateRect.right)
            {
                trace("ERROR-resultUpdateRect.right %d %d\n", (int)ptestRW->resultUpdateRect.right, (int)ptestRWcompare->resultUpdateRect.right);
                countErrors++;
            }
            if (ptestRWcompare->resultUpdateRect.bottom != ptestRW->resultUpdateRect.bottom)
            {
                trace("ERROR-resultUpdateRect.bottom %d %d\n", (int)ptestRW->resultUpdateRect.bottom, (int)ptestRWcompare->resultUpdateRect.bottom);
                countErrors++;
            }
        }
        if (ptestRW->useHrgnUpdate)
        {
            if (ptestRWcompare->hrgnUpdate.left != ptestRW->hrgnUpdate.left)
            {
                trace("ERROR-hrgnUpdate.left %d %d\n", (int)ptestRW->hrgnUpdate.left, (int)ptestRWcompare->hrgnUpdate.left);
                countErrors++;
            }
            if (ptestRWcompare->hrgnUpdate.top != ptestRW->hrgnUpdate.top)
            {
                trace("ERROR-hrgnUpdate.top %d %d\n", (int)ptestRW->hrgnUpdate.top, (int)ptestRWcompare->hrgnUpdate.top);
                countErrors++;
            }
            if (ptestRWcompare->hrgnUpdate.right != ptestRW->hrgnUpdate.right)
            {
                trace("ERROR-hrgnUpdate.right %d %d\n", (int)ptestRW->hrgnUpdate.right, (int)ptestRWcompare->hrgnUpdate.right);
                countErrors++;
            }
            if (ptestRWcompare->hrgnUpdate.bottom != ptestRW->hrgnUpdate.bottom)
            {
                trace("ERROR-hrgnUpdate.bottom %d %d\n", (int)ptestRW->hrgnUpdate.bottom, (int)ptestRWcompare->hrgnUpdate.bottom);
                countErrors++;
            }
        }
        if (ptestRWcompare->prcUpdate.left != ptestRW->prcUpdate.left)
        {
            trace("ERROR-prcUpdate.left %d %d\n", (int)ptestRW->prcUpdate.left, (int)ptestRWcompare->prcUpdate.left);
            countErrors++;
        }
        if (ptestRWcompare->prcUpdate.top != ptestRW->prcUpdate.top)
        {
            trace("ERROR-prcUpdate.top %d %d\n", (int)ptestRW->prcUpdate.top, (int)ptestRWcompare->prcUpdate.top);
            countErrors++;
        }
        if (ptestRWcompare->prcUpdate.right != ptestRW->prcUpdate.right)
        {
            trace("ERROR-prcUpdate.right %d %d\n", (int)ptestRW->prcUpdate.right, (int)ptestRWcompare->prcUpdate.right);
            countErrors++;
        }
        if (ptestRWcompare->prcUpdate.bottom != ptestRW->prcUpdate.bottom)
        {
            trace("ERROR-prcUpdate.bottom %d %d\n", (int)ptestRW->prcUpdate.bottom, (int)ptestRWcompare->prcUpdate.bottom);
            countErrors++;
        }
        if (ptestRWcompare->resultWmEraseGnd != ptestRW->resultWmEraseGnd)
        {
            trace("ERROR-resultWmEraseGnd %d %d\n", (int)ptestRW->resultWmEraseGnd, (int)ptestRWcompare->resultWmEraseGnd);
            countErrors++;
        }
        if (ptestRWcompare->resultWmNcPaint != ptestRW->resultWmNcPaint)
        {
            trace("ERROR-resultWmNcPaint %d %d\n", (int)ptestRW->resultWmNcPaint, (int)ptestRWcompare->resultWmNcPaint);
            countErrors++;
        }
        if (ptestRWcompare->resultPaintIndex != ptestRW->resultPaintIndex)
        {
            trace("ERROR-resultPaintIndex %d %d\n", (int)ptestRW->resultPaintIndex, (int)ptestRWcompare->resultPaintIndex);
            countErrors++;
        }
        /*swprintf(buffer, 256, L"Result OK\n");
        OutputDebugStringW(buffer);
        if (ptestRW->resultNeedsUpdate) {
            swprintf(buffer, 256, L"Area for redrawing: (%d %d) - (%d %d)\n", ptestRW->resultUpdateRect.left,
                ptestRW->resultUpdateRect.top, ptestRW->resultUpdateRect.right, ptestRW->resultUpdateRect.bottom);
        }
        else {
            swprintf(buffer, 256, L"Not area for redrawing\n");
        }
        OutputDebugStringW(buffer);
        swprintf(buffer, 256, L"Color pre-1: 0x%06X\n", ptestRW->resultColorPre1 & 0xFFFFFF);
        OutputDebugStringW(buffer);
        swprintf(buffer, 256, L"Color pre-2: 0x%06X\n", ptestRW->resultColorPre2 & 0xFFFFFF);
        OutputDebugStringW(buffer);
        swprintf(buffer, 256, L"Color post-1: 0x%06X\n", ptestRW->resultColorPost1 & 0xFFFFFF);
        OutputDebugStringW(buffer);
        swprintf(buffer, 256, L"Color post-2: 0x%06X\n", ptestRW->resultColorPost2 & 0xFFFFFF);
        OutputDebugStringW(buffer);*/
    }
    /*else
    {
        swprintf(buffer, 256, L"ERROR-Result fail\n");
        OutputDebugStringW(buffer);
        countErrors++;
    }*/
    if (countErrors > 0)
    {
        trace("ERRORS - %d\n", countErrors);
    }

    return countErrors;
}

void InitRect(RECT *rect, int left, int top, int right, int bottom) {
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

void
Test_ScrollWindowEx1()
{
	STRUCT_TestRedrawWindow testRW;
    STRUCT_TestRedrawWindowCompare testRWcompare;
    //UINT countErrors = 0;

    testRW.testPixelPre1x = 50;
    testRW.testPixelPre1y = 50;
    testRW.testPixelPre2x = 50;
    testRW.testPixelPre2y = 250;
    testRW.testPixelPre3x = 50;
    testRW.testPixelPre3y = 350;
    testRW.testPixelPost1x = 50;
    testRW.testPixelPost1y = 50;
    testRW.testPixelPost2x = 50;
    testRW.testPixelPost2y = 250;
    testRW.testPixelPost3x = 50;
    testRW.testPixelPost3y = 305;
    testRW.testPixelPost4x = 50;
    testRW.testPixelPost4y = 350;
    testRW.scrollX = 0;
	testRW.scrollY = 300;

    testRW.testName = L"Test1";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
	
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x00FF0000;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 200, 200 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test1 fail");
    

    testRW.testName = L"Test2";
    testRW.flags = 0;
    testRW.usePrcScroll = TRUE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 50, 20 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 50, 20 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test2 fail");

    testRW.testName = L"Test3";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = TRUE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 200 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 200 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test3 fail");

    testRW.testName = L"Test4";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 0, 0 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x00FF0000;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test4 fail");

    testRW.testName = L"Test5";
    testRW.flags = SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x0000FF00;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x00FF0000;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test5 fail");

    testRW.testName = L"Test6";
    testRW.flags = SW_ERASE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x00FF0000;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test6 fail");

    testRW.testName = L"Test7";
    testRW.flags = SW_ERASE | SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x0000FF00;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x00FF0000;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test7 fail");

    testRW.testPixelPre1x = 50;
    testRW.testPixelPre1y = 50;
    testRW.testPixelPre2x = 50;
    testRW.testPixelPre2y = 100;
    testRW.testPixelPre3x = 50;
    testRW.testPixelPre3y = 330;
    testRW.testPixelPost1x = 50;
    testRW.testPixelPost1y = 50;
    testRW.testPixelPost2x = 50;
    testRW.testPixelPost2y = 100;
    testRW.testPixelPost3x = 50;
    testRW.testPixelPost3y = 330;
    testRW.testPixelPost4x = 50;
    testRW.testPixelPost4y = 400;

    testRW.testName = L"Test8";
    testRW.flags = SW_SCROLLCHILDREN;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x000000FF;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test8 fail");

    testRW.testName = L"Test9";
    testRW.flags = SW_SCROLLCHILDREN | SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x0000FF00;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FF0000;
    testRWcompare.resultColorPost4 = 0x000000FF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test9 fail");

    testRW.testName = L"Test10";
    testRW.flags = SW_SMOOTHSCROLL | SW_INVALIDATE | (1000 << 16);
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FFFFFF;
    testRWcompare.resultColorPre2 = 0x00FFFFFF;
    testRWcompare.resultColorPre3 = 0x00FFFFFF;
    testRWcompare.resultColorPost1 = 0x0000FF00;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x00FFFFFF;
    testRWcompare.resultColorPost4 = 0x00FFFFFF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = TRUE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 800, 300 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 800, 300 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test10 fail");
	
	testRW.scrollX = 0;
	testRW.scrollY = -20;

    testRW.testName = L"Test11";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
	
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 200, 200 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test11 fail");
    

    testRW.testName = L"Test12";
    testRW.flags = 0;
    testRW.usePrcScroll = TRUE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 0, 50, 20 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 0, 50, 20 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test12 fail");

    testRW.testName = L"Test13";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = TRUE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 50, 70 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 180, 800, 200 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 180, 800, 200 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test13 fail");

    testRW.testName = L"Test14";
    testRW.flags = 0;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 0, 0, 0, 0 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 50, 20 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test14 fail");

    testRW.testName = L"Test15";
    testRW.flags = SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test15 fail");

    testRW.testName = L"Test16";
    testRW.flags = SW_ERASE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test16 fail");

    testRW.testName = L"Test17";
    testRW.flags = SW_ERASE | SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x0000FF00;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x0000FF00;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test17 fail");

    testRW.testPixelPre1x = 50;
    testRW.testPixelPre1y = 50;
    testRW.testPixelPre2x = 50;
    testRW.testPixelPre2y = 100;
    testRW.testPixelPre3x = 50;
    testRW.testPixelPre3y = 330;
    testRW.testPixelPost1x = 50;
    testRW.testPixelPost1y = 50;
    testRW.testPixelPost2x = 50;
    testRW.testPixelPost2y = 100;
    testRW.testPixelPost3x = 50;
    testRW.testPixelPost3y = 330;
    testRW.testPixelPost4x = 50;
    testRW.testPixelPost4y = 400;

    testRW.testName = L"Test18";
    testRW.flags = SW_SCROLLCHILDREN;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x000000FF;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 1;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test18 fail");

    testRW.testName = L"Test19";
    testRW.flags = SW_SCROLLCHILDREN | SW_INVALIDATE;
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = TRUE;

    testRWcompare.resultColorPre1 = 0x00FF0000;
    testRWcompare.resultColorPre2 = 0x000000FF;
    testRWcompare.resultColorPre3 = 0x0000FF00;
    testRWcompare.resultColorPost1 = 0x00FF0000;
    testRWcompare.resultColorPost2 = 0x000000FF;
    testRWcompare.resultColorPost3 = 0x0000FF00;
    testRWcompare.resultColorPost4 = 0x0000FF00;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = FALSE;
    testRWcompare.resultWmNcPaint = FALSE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test19 fail");

    testRW.testName = L"Test20";
    testRW.flags = SW_SMOOTHSCROLL | SW_INVALIDATE | (1000 << 16);
    testRW.usePrcScroll = FALSE;
    InitRect(&testRW.prcScroll, 0, 0, 50, 20 );
    testRW.usePrcClip = FALSE;
    InitRect(&testRW.prcClip, 0, 0, 800, 200 );
    testRW.useHrgnUpdate = TRUE;
    InitRect(&testRW.hrgnUpdate, 300, 0, 350, 500 );
    testRW.testChild = FALSE;

    testRWcompare.resultColorPre1 = 0x00FFFFFF;
    testRWcompare.resultColorPre2 = 0x00FFFFFF;
    testRWcompare.resultColorPre3 = 0x00FFFFFF;
    testRWcompare.resultColorPost1 = 0x00FFFFFF;
    testRWcompare.resultColorPost2 = 0x00FFFFFF;
    testRWcompare.resultColorPost3 = 0x00FFFFFF;
    testRWcompare.resultColorPost4 = 0x00FFFFFF;
    InitRect(&testRWcompare.resultUpdateRect, 0, 0, 800, 300 );
    testRWcompare.resultNeedsUpdate = FALSE;
    testRWcompare.resultWmEraseGnd = TRUE;
    testRWcompare.resultWmNcPaint = TRUE;
    testRWcompare.resultPaintIndex = 2;
    InitRect(&testRWcompare.prcUpdate, 0, 580, 800, 600 );
    InitRect(&testRWcompare.hrgnUpdate, 0, 580, 800, 600 );
    ok(0 == TestScrollWindowEx2(&testRW, &testRWcompare),"Test20 fail");
}

void
Test_ScrollWindowEx2()
{
    HWND hWnd, hChild1, hChild2;
    HRGN hrgn;
    int Result;
    RECT rc, rcChild1, rcChild2;
    INT x1, y1, x2, y2, dx, dy;
    DWORD style;

    /* Create a window */
    style = WS_POPUP | SS_WHITERECT | WS_VISIBLE;
    hWnd = CreateWindowW(L"STATIC", L"TestWindow", style, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, 0, 0);
    ok(hWnd != NULL, "hWnd was NULL.\n");
    UpdateWindow(hWnd);

    /* Assert that no update region is there */
    hrgn = CreateRectRgn(0, 0, 0, 0);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == NULLREGION, "Result = %d\n", Result);

    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, 0);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == NULLREGION, "Result = %d\n", Result);

    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    UpdateWindow(hWnd);

    // test invalid update region
    DeleteObject(hrgn);
    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, hrgn, NULL, SW_INVALIDATE);
    ok(Result == ERROR, "Result = %d\n", Result);
    hrgn = CreateRectRgn(0, 0, 0, 0);
    UpdateWindow(hWnd);

    // Test invalid updaterect pointer
    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, (LPRECT)1, SW_INVALIDATE);
    ok(Result == ERROR, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);

    /* create child window 1 */
    x1 = 1;
    y1 = 3;
    style = WS_CHILD | WS_VISIBLE | SS_BLACKRECT;
    hChild1 = CreateWindowW(L"STATIC", L"Child1", style, x1, y1, 10, 10, hWnd, NULL, 0, 0);
    ok(hChild1 != NULL, "hChild1 was NULL.\n");
    UpdateWindow(hChild1);

    /* create child window 2 */
    x2 = 5;
    y2 = 7;
    style = WS_CHILD | WS_VISIBLE | SS_WHITERECT;
    hChild2 = CreateWindowW(L"STATIC", L"Child2", style, x2, y2, 10, 10, hWnd, NULL, 0, 0);
    ok(hChild2 != NULL, "hChild2 was NULL.\n");
    UpdateWindow(hChild2);

    /* scroll with child windows */
    dx = 3;
    dy = 8;
    ScrollWindowEx(hWnd, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    UpdateWindow(hWnd);

    /* check the positions */
    GetWindowRect(hWnd, &rc);
    GetWindowRect(hChild1, &rcChild1);
    GetWindowRect(hChild2, &rcChild2);
    ok_long(rcChild1.left - rc.left, x1 + dx);
    ok_long(rcChild2.left - rc.left, x2 + dx);
    ok_long(rcChild1.top - rc.top, y1 + dy);
    ok_long(rcChild2.top - rc.top, y2 + dy);

    /* update */
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;

    /* scroll with child windows */
    dx = 9;
    dy = -2;
    ScrollWindowEx(hWnd, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    UpdateWindow(hWnd);

    /* check the positions */
    GetWindowRect(hWnd, &rc);
    GetWindowRect(hChild1, &rcChild1);
    GetWindowRect(hChild2, &rcChild2);
    ok_long(rcChild1.left - rc.left, x1 + dx);
    ok_long(rcChild2.left - rc.left, x2 + dx);
    ok_long(rcChild1.top - rc.top, y1 + dy);
    ok_long(rcChild2.top - rc.top, y2 + dy);

    DestroyWindow(hChild1);
    DestroyWindow(hChild2);
    DeleteObject(hrgn);
    DestroyWindow(hWnd);
}

START_TEST(ScrollWindowEx)
{
	Test_ScrollWindowEx1();
    Test_ScrollWindowEx2();	
}
