/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SystemParametersInfo function family
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "helper.h"
#include <undocuser.h>

HWND hWnd1, hWnd2;

/* FIXME: test for HWND_TOP, etc...*/
static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else return 0;
}

LRESULT CALLBACK SysParamsTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_GETICON :
    case WM_SETICON:
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS* pwp = (WINDOWPOS*)lParam;
            ok(wParam==0,"expected wParam=0\n");
            record_message(iwnd, message, SENT, get_iwnd(pwp->hwndInsertAfter), pwp->flags);
            break;
        }
    default:
        record_message(iwnd, message, SENT, 0,0);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if(!(msg.message > WM_USER || !iwnd || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            record_message(iwnd, msg.message, POST,0,0);
        DispatchMessageA( &msg );
    }
}

MSG_ENTRY NcMetricsChange_chain[]={
    {2,WM_WINDOWPOSCHANGING, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW},
    {2,WM_GETMINMAXINFO},
    {2,WM_NCCALCSIZE},
    {2,WM_WINDOWPOSCHANGED, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW | SWP_NOCLIENTSIZE},
    {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW},
    {1,WM_GETMINMAXINFO},
    {1,WM_NCCALCSIZE},
    {1,WM_WINDOWPOSCHANGED, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW | SWP_NOCLIENTSIZE},
    {0,0}};

MSG_ENTRY CaptionHeight_chain[]={
    {2,WM_WINDOWPOSCHANGING, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW},
    {2,WM_GETMINMAXINFO},
    {2,WM_NCCALCSIZE},
    {2,WM_WINDOWPOSCHANGED, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW | SWP_NOCLIENTSIZE},
    {2,WM_MOVE},
    {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW},
    {1,WM_GETMINMAXINFO},
    {1,WM_NCCALCSIZE},
    {1,WM_WINDOWPOSCHANGED, SENT, 0, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW | SWP_NOCLIENTSIZE},
    {1,WM_MOVE},
    {0,0}};

static void Test_NonClientMetrics()
{
    NONCLIENTMETRICS NonClientMetrics;

    SetCursorPos(0,0);

    NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
    FlushMessages();
    COMPARE_CACHE(NcMetricsChange_chain);

    NonClientMetrics.iCaptionHeight += 1;
    SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
    FlushMessages();
    COMPARE_CACHE(CaptionHeight_chain);

    NonClientMetrics.iCaptionHeight -= 1;
    SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
    FlushMessages();
    COMPARE_CACHE(CaptionHeight_chain);
}

START_TEST(SystemParametersInfo)
{
    RegisterSimpleClass(SysParamsTestProc, L"sysparamstest"); 
    hWnd1 = CreateWindowW(L"sysparamstest", L"sysparamstest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"sysparamstest", L"sysparamstest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);

    Test_NonClientMetrics();

    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
    UnregisterClassW(L"sysparamstest", 0);
}