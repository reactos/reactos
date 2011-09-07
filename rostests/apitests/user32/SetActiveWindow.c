/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetActiveWindow
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void Test_SetActiveWindow()
{
    MSG msg;
    HWND hWnd, hWnd1, hWnd2;

    hWnd = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd1 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                         20, 350, 300, 300, hWnd, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"BUTTON", L"ownertest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    ok(GetWindow(hWnd2,GW_HWNDNEXT) == hWnd1, "Expected %p after %p, not %p\n",hWnd1,hWnd2,GetWindow(hWnd2,GW_HWNDNEXT) );
    ok(GetWindow(hWnd1,GW_HWNDNEXT) == hWnd, "Expected %p after %p, not %p\n",hWnd,hWnd1,GetWindow(hWnd1,GW_HWNDNEXT));
    ok(GetActiveWindow() == hWnd2, "Expected %p to be the active window, not %p\n",hWnd2,GetActiveWindow());

    SetActiveWindow(hWnd);

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    /* note: the owned is moved on top of the three windows */   
    ok(GetActiveWindow() == hWnd, "Expected %p to be the active window, not %p\n",hWnd,GetActiveWindow());
    ok(GetWindow(hWnd1,GW_HWNDNEXT) == hWnd, "Expected %p after %p, not %p\n",hWnd,hWnd1,GetWindow(hWnd1,GW_HWNDNEXT) );
    ok(GetWindow(hWnd,GW_HWNDNEXT) == hWnd2, "Expected %p after %p, not %p\n",hWnd2,hWnd,GetWindow(hWnd,GW_HWNDNEXT) );
}

START_TEST(SetActiveWindow)
{
    Test_SetActiveWindow();
}
