/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetActiveWindow
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

static HWND hWnd1, hWnd2;

/* FIXME: test for HWND_TOP, etc...*/
static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else return 0;
}

LRESULT CALLBACK OwnerTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_QUERYNEWPALETTE:
        {
            HDC hdc = GetDC(0);
            int bits = GetDeviceCaps(hdc,BITSPIXEL);
            ok( bits == 8 , "expected WM_QUERYNEWPALETTE only on 8bpp\n");
            ReleaseDC(0, hdc);
            return FALSE;
        }
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY :
    case WM_GETICON :
    case WM_GETTEXT:
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS* pwp = (WINDOWPOS*)lParam;
            ok(wParam==0,"expected wParam=0\n");
            RECORD_MESSAGE(iwnd, message, SENT, get_iwnd(pwp->hwndInsertAfter), pwp->flags);
            break;
        }
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
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
            RECORD_MESSAGE(iwnd, msg.message, POST,0,0);
        DispatchMessageA( &msg );
    }
}

static void create_test_windows()
{
    RegisterSimpleClass(OwnerTestProc, L"ownertest");
    hWnd1 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);
}

static void set_default_zorder()
{
    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    SetWindowPos(hWnd2, 0, 0,0,0,0, SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);

    FlushMessages();
    EMPTY_CACHE();
}

static void destroy_test_window()
{
    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
    UnregisterClassW(L"testClass", 0);
}

static MSG_ENTRY activate2to1_chain[]={
      {2,WM_NCACTIVATE},
      {2,WM_ACTIVATE},
      {1,WM_WINDOWPOSCHANGING, SENT,0, SWP_NOSIZE|SWP_NOMOVE},
      {1,WM_WINDOWPOSCHANGED, SENT,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {1,WM_NCACTIVATE},
      {1,WM_ACTIVATE},
      {2,WM_KILLFOCUS},
      {1,WM_SETFOCUS},
      {0,0}};

static MSG_ENTRY activate1to2_chain[]={
      {1,WM_NCACTIVATE},
      {1,WM_ACTIVATE},
      {2,WM_WINDOWPOSCHANGING, SENT,0, SWP_NOSIZE|SWP_NOMOVE},
      {2,WM_WINDOWPOSCHANGED, SENT,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {2,WM_NCACTIVATE},
      {2,WM_ACTIVATE},
      {1,WM_KILLFOCUS},
      {2,WM_SETFOCUS},
      {0,0}};

void Test_msg_simple()
{
    SetCursorPos(0,0);

    create_test_windows();
    set_default_zorder();

    SetActiveWindow(hWnd1);
    FlushMessages();
    COMPARE_CACHE(activate2to1_chain);

    SetActiveWindow(hWnd2);
    FlushMessages();
    COMPARE_CACHE(activate1to2_chain);

    destroy_test_window();
}

START_TEST(SetActiveWindow)
{
    Test_msg_simple();
}
