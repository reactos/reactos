/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for RedrawWindow
 * COPYRIGHT:   Copyright 2018 Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static DWORD dwThreadId;
static BOOL got_paint;

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


START_TEST(RedrawWindow)
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
