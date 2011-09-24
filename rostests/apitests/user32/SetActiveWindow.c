/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetActiveWindow
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "helper.h"
#include <undocuser.h>

MSG_ENTRY message_cache[100];
HWND hWnd1, hWnd2, hWnd3, hWnd4;

/* FIXME: test for HWND_TOP, etc...*/
static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else if(hWnd == hWnd3) return 3;
    else if(hWnd == hWnd4) return 4;
    else return 0;
}

LRESULT CALLBACK OwnerTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
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

static void create_test_windows()
{
    RegisterSimpleClass(OwnerTestProc, L"ownertest"); 
    hWnd1 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         20, 350, 300, 300, hWnd1, NULL, 0, NULL);

    hWnd3 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);

    hWnd4 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         250, 250, 200, 200, hWnd1, NULL, 0, NULL);
}

/* the actual test begins here */

MSG_ENTRY Activate1_chain[]={{4,WM_NCACTIVATE},
                             {4,WM_ACTIVATE},
                             {4,WM_WINDOWPOSCHANGING,SENT,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
                             {2,WM_WINDOWPOSCHANGING,SENT,4,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
                             {1,WM_WINDOWPOSCHANGING,SENT,2,SWP_NOMOVE | SWP_NOSIZE},
                             {1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {4,WM_KILLFOCUS},
                             {1,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate2_chain[]={{1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {3,WM_WINDOWPOSCHANGING,SENT, 0, SWP_NOMOVE | SWP_NOSIZE},
                             {3,WM_WINDOWPOSCHANGED ,SENT, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
                             {3,WM_NCACTIVATE},
                             {3,WM_ACTIVATE},
                             {1,WM_KILLFOCUS},
                             {3,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate3_chain[]={{3,WM_NCACTIVATE},
                             {3,WM_ACTIVATE},
                             {2,WM_WINDOWPOSCHANGING, SENT,0, SWP_NOMOVE | SWP_NOSIZE },
                             {4,WM_WINDOWPOSCHANGING, SENT,2, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
                             {1,WM_WINDOWPOSCHANGING, SENT,4, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
                             {2,WM_WINDOWPOSCHANGED,  SENT,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTMOVE |SWP_NOCLIENTSIZE },
                             {4,WM_WINDOWPOSCHANGED,  SENT,2, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
                             {1,WM_WINDOWPOSCHANGED,  SENT,4, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
                             {2,WM_NCACTIVATE},
                             {2,WM_ACTIVATE},
                             {3,WM_KILLFOCUS},
                             {2,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate4_chain[]={{2,WM_NCACTIVATE, },
                             {2,WM_ACTIVATE},
                             {2,WM_WINDOWPOSCHANGING, SENT ,0 ,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
                             {4,WM_WINDOWPOSCHANGING, SENT, 2, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE} ,
                             {1,WM_WINDOWPOSCHANGING, SENT, 4, SWP_NOMOVE | SWP_NOSIZE},
                             {1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {2,WM_KILLFOCUS},
                             {1,WM_SETFOCUS},
                             {0,0}};

void Test_SetActiveWindow()
{
    SetCursorPos(0,0);

    create_test_windows();

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    FlushMessages();
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd1,hWnd3);

    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    FlushMessages();
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd3, SW_SHOW);
    UpdateWindow(hWnd3);
    FlushMessages();
    EXPECT_NEXT(hWnd3,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd4, SW_SHOW);
    UpdateWindow(hWnd4);
    FlushMessages();
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd4);

    empty_message_cache();

    SetActiveWindow(hWnd1);
    FlushMessages();
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);
    COMPARE_CACHE(Activate1_chain);

    SetActiveWindow(hWnd3);
    FlushMessages();
    EXPECT_NEXT(hWnd3,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_ACTIVE(hWnd3);
    COMPARE_CACHE(Activate2_chain);

    SetActiveWindow(hWnd2);
    FlushMessages();
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd2);
    COMPARE_CACHE(Activate3_chain);

    SetActiveWindow(hWnd1);
    FlushMessages();
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);
    COMPARE_CACHE(Activate4_chain);
}

START_TEST(SetActiveWindow)
{
    Test_SetActiveWindow();
}
