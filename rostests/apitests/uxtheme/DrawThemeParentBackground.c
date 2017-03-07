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
    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);

    hWnd2 = CreateWindowW(L"testClass", L"test window", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd1, NULL, NULL, NULL);
    ok (hWnd2 != NULL, "Expected CreateWindowW to succeed\n");
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
    
    hdc = GetDC(hWnd1);
    
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

START_TEST(DrawThemeParentBackground)
{
    Test_Messages();
}