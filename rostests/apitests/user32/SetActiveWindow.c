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

static void set_default_zorder()
{
    /* show the windows */
    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    ShowWindow(hWnd3, SW_SHOW);
    UpdateWindow(hWnd3);
    ShowWindow(hWnd4, SW_SHOW);
    UpdateWindow(hWnd4);

    SetWindowPos(hWnd3, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd2, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd4, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);

    FlushMessages();
    empty_message_cache();
}

static void destroy_test_window()
{
    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
    DestroyWindow(hWnd3);
    DestroyWindow(hWnd4);
    UnregisterClassW(L"testClass", 0);
}

#define EXPECT_NEXT(hWnd1, hWnd2)                                         \
       ok(get_iwnd(GetWindow(hWnd1,GW_HWNDNEXT)) == get_iwnd(hWnd2),      \
       "After hwnd%d, expected hwnd%d not hwnd%d\n",                      \
        get_iwnd(hWnd1), get_iwnd(hWnd2),get_iwnd(GetWindow(hWnd1,GW_HWNDNEXT)) )

#define EXPECT_CHAIN(A,B,C,D,X)     \
    EXPECT_NEXT(hWnd##A, hWnd##B);  \
    EXPECT_NEXT(hWnd##B, hWnd##C);  \
    EXPECT_NEXT(hWnd##C, hWnd##D);  \
    EXPECT_NEXT(hWnd##D, 0);        \
    EXPECT_ACTIVE(hWnd##X);

/* the actual test begins here */

MSG_ENTRY Activate1_chain[]=
     {{4,WM_NCACTIVATE},
      {4,WM_ACTIVATE},
      {4,WM_WINDOWPOSCHANGING,SENT,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
      {2,WM_WINDOWPOSCHANGING,SENT,4,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
      {1,WM_WINDOWPOSCHANGING,SENT,2,SWP_NOMOVE | SWP_NOSIZE},
      {1,WM_NCACTIVATE},
      {1,WM_ACTIVATE},
      {4,WM_KILLFOCUS},
      {1,WM_SETFOCUS},
      {0,0}};

MSG_ENTRY Activate2_chain[]=
     {{1,WM_NCACTIVATE},
      {1,WM_ACTIVATE},
      {3,WM_WINDOWPOSCHANGING,SENT, 0, SWP_NOMOVE | SWP_NOSIZE},
      {3,WM_WINDOWPOSCHANGED ,SENT, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {3,WM_NCACTIVATE},
      {3,WM_ACTIVATE},
      {1,WM_KILLFOCUS},
      {3,WM_SETFOCUS},
      {0,0}};

MSG_ENTRY Activate3_chain[]=
     {{3,WM_NCACTIVATE},
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

MSG_ENTRY Activate4_chain[]=
     {{2,WM_NCACTIVATE, },
      {2,WM_ACTIVATE},
      {2,WM_WINDOWPOSCHANGING, SENT ,0 ,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
      {4,WM_WINDOWPOSCHANGING, SENT, 2, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE} ,
      {1,WM_WINDOWPOSCHANGING, SENT, 4, SWP_NOMOVE | SWP_NOSIZE},
      {1,WM_NCACTIVATE},
      {1,WM_ACTIVATE},
      {2,WM_KILLFOCUS},
      {1,WM_SETFOCUS},
      {0,0}};

MSG_ENTRY Activate5_chain[]=
     {{1,WM_NCACTIVATE, },
      {1,WM_ACTIVATE},
      {4,WM_WINDOWPOSCHANGING, SENT, 0 ,SWP_NOMOVE | SWP_NOSIZE},
      {2,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
      {1,WM_WINDOWPOSCHANGING, SENT, 2, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE},
      {4,WM_WINDOWPOSCHANGED,  SENT, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {4,WM_NCACTIVATE},
      {4,WM_ACTIVATE},
      {1,WM_KILLFOCUS},
      {4,WM_SETFOCUS},
      {0,0}};

/*
   some notes about this testcase:
   from the expected series of messages it is obbvious that SetActiveWindow uses SetWindowPos. 
   So the big question is, that if this behaviour (of changing the z order of owner and owned windows) 
   is implemented in SetActiveWindow or SetWindowPos.
   More tests reveal that calling SetWindowPos like this, has the same effect in the zorder:
   SetWindowPos(hWndX, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
   However if it is called like this it does NOT change the z-order of owner or owned windows:
   SetWindowPos(hWndX, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
   So the conclusion is that SetActiveWindow calls Begin/Defer/EndDeferWindow pos
   moving all affected windows in a batch move operation
*/

void Test_OwnerWindows()
{
    SetCursorPos(0,0);

    create_test_windows();
    set_default_zorder();

    /* before we start testing SetActiveWindow we have to be sure they are on the ocrrect order*/
    EXPECT_CHAIN(4,2,1,3,4);

    /* move the windows on top in the following order 1 2 3 1 4 */
    SetActiveWindow(hWnd1);
    FlushMessages();
    EXPECT_CHAIN(4,2,1,3,1);
    COMPARE_CACHE(Activate1_chain);

    SetActiveWindow(hWnd3);
    FlushMessages();
    EXPECT_CHAIN(3,4,2,1,3);
    COMPARE_CACHE(Activate2_chain);

    SetActiveWindow(hWnd2);
    FlushMessages();
    EXPECT_CHAIN(2,4,1,3,2);
    COMPARE_CACHE(Activate3_chain);

    SetActiveWindow(hWnd1);
    FlushMessages();
    EXPECT_CHAIN(2,4,1,3,1);
    COMPARE_CACHE(Activate4_chain);

    SetActiveWindow(hWnd4);
    FlushMessages();
    EXPECT_CHAIN(4,2,1,3,4);
    /* in this test xp and 7 give different results :/ */
    /*COMPARE_CACHE(Activate5_chain);*/

    /* move the windows on top with the same order but now using SetWindowPos */
    /* note: the zorder of windows change in the same way they change with SetActiveWindow */
    set_default_zorder();

    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(4,2,1,3,1);

    SetWindowPos(hWnd3, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(3,4,2,1,3);

    SetWindowPos(hWnd2, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(2,4,1,3,2);

    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(2,4,1,3,1);

    SetWindowPos(hWnd4, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(4,2,1,3,4);

    /* now do the same thing one more time with SWP_NOACTIVATE */
    set_default_zorder();

    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(1,4,2,3,4);

    SetWindowPos(hWnd3, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(3,1,4,2,4);

    SetWindowPos(hWnd2, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(2,3,1,4,4);

    SetWindowPos(hWnd1, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    FlushMessages();
    empty_message_cache();
    EXPECT_CHAIN(1,2,3,4,4);

    SetWindowPos(hWnd4, 0, 0,0,0,0, SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    FlushMessages();
    EXPECT_CHAIN(4,1,2,3,4);

    destroy_test_window();
}

START_TEST(SetActiveWindow)
{
    Test_OwnerWindows();
}
