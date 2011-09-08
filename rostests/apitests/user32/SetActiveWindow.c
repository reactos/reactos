/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetActiveWindow
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>


#define EXPECT_NEXT(hWnd1, hWnd2) ok(GetWindow(hWnd1,GW_HWNDNEXT) == hWnd2, "Expected %p after %p, not %p\n",hWnd2,hWnd1,GetWindow(hWnd1,GW_HWNDNEXT) )
#define EXPECT_ACTIVE(hwnd) ok(GetActiveWindow() == hwnd, "Expected %p to be the active window, not %p\n",hwnd,GetActiveWindow())

void Test_SetActiveWindow()
{
    MSG msg;
    HWND hWnd1, hWnd2, hWnd3, hWnd4;

    hWnd1 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                         20, 350, 300, 300, hWnd1, NULL, 0, NULL);

    hWnd3 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);

    hWnd4 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                         250, 250, 200, 200, hWnd1, NULL, 0, NULL);

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd1,hWnd3);

    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd3, SW_SHOW);
    UpdateWindow(hWnd3);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd3,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd4, SW_SHOW);
    UpdateWindow(hWnd4);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd4);

    SetActiveWindow(hWnd1);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);

    SetActiveWindow(hWnd3);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd3,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_ACTIVE(hWnd3);

    SetActiveWindow(hWnd2);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd2);

    SetActiveWindow(hWnd1);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);
}

START_TEST(SetActiveWindow)
{
    Test_SetActiveWindow();
}
