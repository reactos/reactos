/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for DrawThemeParentBackground
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>
#include <stdio.h>
#include <windows.h>
#include <uxtheme.h>
#include <undocuser.h>
#include <msgtrace.h>
#include <user32testhelpers.h>

HWND hWnd1, hWnd2;

static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else return 0;
}

static LRESULT CALLBACK TestProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hwnd);

    if(message > WM_USER || !iwnd || message == WM_GETICON)
        return DefWindowProc(hwnd, message, wParam, lParam);

    RECORD_MESSAGE(iwnd, message, SENT, 0,0);
    return DefWindowProc(hwnd, message, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if(iwnd && msg.message <= WM_USER)
            RECORD_MESSAGE(iwnd, msg.message, POST,0,0);
        DispatchMessageW( &msg );
    }
}

MSG_ENTRY draw_parent_chain[]={{1, WM_ERASEBKGND},
                               {1, WM_PRINTCLIENT},
                               {0,0}};

void Test_Messages()
{
    HDC hdc;
    RECT rc;

    RegisterSimpleClass(TestProc, L"testClass");

    hWnd1 = CreateWindowW(L"testClass", L"Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, NULL, NULL, NULL);
    ok (hWnd1 != NULL, "Expected CreateWindowW to succeed\n");

    hWnd2 = CreateWindowW(L"testClass", L"test window", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd1, NULL, NULL, NULL);
    ok (hWnd2 != NULL, "Expected CreateWindowW to succeed\n");

    FlushMessages();
    EMPTY_CACHE();

    hdc = GetDC(hWnd1);

    DrawThemeParentBackground(hWnd2, hdc, NULL);
    FlushMessages();
    COMPARE_CACHE(draw_parent_chain);

    DrawThemeParentBackground(hWnd1, hdc, NULL);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);

    FlushMessages();
    EMPTY_CACHE();

    DrawThemeParentBackground(hWnd2, NULL, NULL);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    DrawThemeParentBackground(hWnd1, NULL, NULL);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    DrawThemeParentBackground(hWnd2, hdc, NULL);
    FlushMessages();
    COMPARE_CACHE(draw_parent_chain);

    DrawThemeParentBackground(hWnd1, hdc, NULL);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    memset(&rc, 0, sizeof(rc));

    DrawThemeParentBackground(hWnd2, hdc, &rc);
    FlushMessages();
    COMPARE_CACHE(draw_parent_chain);

    DrawThemeParentBackground(hWnd1, hdc, &rc);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
}

BOOL bGotException;

static LONG WINAPI VEHandler_1(PEXCEPTION_POINTERS ExceptionInfo)
{
    ok(FALSE, "VEHandler_1 called!\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI VEHandler_2(PEXCEPTION_POINTERS ExceptionInfo)
{
    bGotException = TRUE;
    return EXCEPTION_CONTINUE_SEARCH;
}

void Test_Params()
{
    HRESULT hr;
    HDC hdc;
    PVOID pVEH;

    bGotException = FALSE;

    pVEH = AddVectoredExceptionHandler(1, VEHandler_1);

    hr = DrawThemeParentBackground(NULL, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground((HWND)(ULONG_PTR)0xdeaddeaddeaddeadULL, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground(NULL, (HDC)(ULONG_PTR)0xdeaddeaddeaddeadULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground((HWND)(ULONG_PTR)0xdeaddeaddeaddeadULL, (HDC)(ULONG_PTR)0xdeaddeaddeaddeadULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    RemoveVectoredExceptionHandler(pVEH);

    RegisterSimpleClass(DefWindowProcW, L"testClass2");

    hWnd1 = CreateWindowW(L"testClass2", L"Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, NULL, NULL, NULL);
    ok (hWnd1 != NULL, "Expected CreateWindowW to succeed\n");
    hWnd2 = CreateWindowW(L"testClass2", L"test window", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd1, NULL, NULL, NULL);
    ok (hWnd2 != NULL, "Expected CreateWindowW to succeed\n");

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);

    hr = DrawThemeParentBackground(hWnd1, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hdc = GetDC(hWnd1);
    ok (hdc != NULL, "Expected GetDC to succeed\n");

    hr = DrawThemeParentBackground(NULL, hdc, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground(hWnd1, hdc, NULL);
    ok (hr == S_OK, "Expected success got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground(hWnd1, (HDC)(ULONG_PTR)0xdeaddeaddeaddeadULL, NULL);
    ok (hr == S_OK, "Expected success got 0x%lx error\n", hr);

    pVEH = AddVectoredExceptionHandler(1, VEHandler_2);
    hr = DrawThemeParentBackground(hWnd1, hdc, (RECT*)(ULONG_PTR)0xdeaddeaddeaddeadULL);
    ok (hr == E_POINTER, "Expected success got 0x%lx error\n", hr);
    RemoveVectoredExceptionHandler(pVEH);
    ok (bGotException == TRUE, "Excepted a handled exception\n");

    hr = DrawThemeParentBackground(hWnd2, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground(hWnd2, hdc, NULL);
    if (IsThemeActive())
        ok (hr == S_FALSE, "Expected S_FALSE got 0x%lx error\n", hr);
    else
        skip("Theme not active\n");

    ReleaseDC(hWnd1, hdc);
    hdc = GetDC(hWnd2);
    ok (hdc != NULL, "Expected GetDC to succeed\n");

    hr = DrawThemeParentBackground(hWnd1, hdc, NULL);
    ok (hr == S_OK, "Expected success got 0x%lx error\n", hr);

    hr = DrawThemeParentBackground(hWnd2, hdc, NULL);
    if (IsThemeActive())
        ok (hr == S_FALSE, "Expected S_FALSE got 0x%lx error\n", hr);
    else
        skip("Theme not active\n");
    ReleaseDC(hWnd2, hdc);


}

START_TEST(DrawThemeParentBackground)
{
    Test_Messages();
    Test_Params();
}
