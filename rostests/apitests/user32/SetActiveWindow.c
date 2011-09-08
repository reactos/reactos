/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetActiveWindow
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

typedef struct _MSG_ENTRY
{
    int iwnd;
    UINT msg;
} MSG_ENTRY;

MSG_ENTRY message_cache[100];
int message_cache_size = 0;

HWND hWnd1, hWnd2, hWnd3, hWnd4;

/* helper functions */

static char* get_msg_name(UINT msg)
{
    switch(msg)
    {
        case WM_NCACTIVATE: return "WM_NCACTIVATE";
        case WM_ACTIVATE: return "WM_ACTIVATE";
        case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
        case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
        case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
        case WM_SETFOCUS: return "WM_SETFOCUS";
        case WM_KILLFOCUS: return "WM_KILLFOCUS";
        case WM_NCPAINT: return "WM_NCPAINT";
        case WM_PAINT: return "WM_PAINT";
        case WM_ERASEBKGND: return "WM_ERASEBKGND";
        case WM_SIZE: return "WM_SIZE";
        case WM_MOVE: return "WM_MOVE";
        case WM_SHOWWINDOW: return "WM_SHOWWINDOW";
        case WM_QUERYNEWPALETTE: return "WM_QUERYNEWPALETTE";
        default: return NULL;
    }
}

static void empty_message_cache()
{
    memset(message_cache, 0, sizeof(message_cache));
    message_cache_size = 0;
}

void trace_cache()
{
    int i;
    char *szMsgName;

    for (i=0; i < message_cache_size; i++)
    {
        if((szMsgName = get_msg_name(message_cache[i].msg)))
        {
            trace("hwnd%d, msg:%s\n",message_cache[i].iwnd, szMsgName );
        }
        else
        {
            trace("hwnd%d, msg:%d\n",message_cache[i].iwnd, message_cache[i].msg );                
        }
    }
    trace("\n");    
}

void compare_cache(char* testname, MSG_ENTRY *msg_chain)
{
    int i = 0;

    while(1)
    {
        char *szMsgExpected, *szMsgGot;
        szMsgExpected = get_msg_name(msg_chain->msg);
        szMsgGot = get_msg_name(message_cache[i].msg);
        if(szMsgExpected && szMsgGot)
        {
            ok(message_cache[i].iwnd ==  msg_chain->iwnd && 
               message_cache[i].msg ==  msg_chain->msg,
               "%s, message %d: expected %s to hwnd%d and got %s to hwnd%d\n",
               testname,i, szMsgExpected, msg_chain->iwnd, szMsgGot, message_cache[i].iwnd);    
        }
        else
        {
            ok(message_cache[i].iwnd ==  msg_chain->iwnd && 
               message_cache[i].msg ==  msg_chain->msg,
               "%s, message %d: expected msg %d to hwnd%d and got msg %d to hwnd%d\n",
               testname,i, msg_chain->msg, msg_chain->iwnd, message_cache[i].msg, message_cache[i].iwnd);
        }

        if(msg_chain->msg !=0 && msg_chain->iwnd != 0)
        {
            msg_chain++;
        }
        else
        {
            if(i>message_cache_size)
            {
                break;
            }
        }
        i++;
    }
}

LRESULT CALLBACK OwnerTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message < WM_USER &&
       message != WM_IME_SETCONTEXT && 
       message != WM_IME_NOTIFY &&
       message != WM_KEYUP &&
       message != WM_GETICON &&
       message != WM_GETTEXT && /* the following messages have to be ignroed because dwm changes the time they are sent */
       message != WM_NCPAINT &&
       message != WM_ERASEBKGND &&
       message != WM_PAINT &&
       message != 0x031f /*WM_DWMNCRENDERINGCHANGED*/)
    {
        if (message_cache_size<100)
        {
            int iwnd;
            if(hWnd == hWnd1) iwnd = 1;
            else if(hWnd == hWnd2) iwnd = 2;
            else if(hWnd == hWnd3) iwnd = 3;
            else if(hWnd == hWnd4) iwnd = 4;
            else
                return DefWindowProc(hWnd, message, wParam, lParam);

            message_cache[message_cache_size].iwnd = iwnd;
            message_cache[message_cache_size].msg = message;
            message_cache_size++;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void create_test_windows()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style            = 0;
    wcex.lpfnWndProc    = OwnerTestProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = 0;
    wcex.hIcon            = 0;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = 0;
    wcex.lpszClassName    = L"ownertest";
    wcex.hIconSm        = 0;

    RegisterClassExW(&wcex);

    hWnd1 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    hWnd2 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         20, 350, 300, 300, hWnd1, NULL, 0, NULL);

    hWnd3 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         200, 200, 300, 300, NULL, NULL, 0, NULL);

    hWnd4 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
                         250, 250, 200, 200, hWnd1, NULL, 0, NULL);

    trace("1:%p, 2:%p, 3:%p,4:%p\n", hWnd1, hWnd2, hWnd3, hWnd4);
}

/* the actual test begins here */

MSG_ENTRY Activate1_chain[]={{4,WM_NCACTIVATE},
                             {4,WM_ACTIVATE},
                             {4,WM_WINDOWPOSCHANGING},
                             {2,WM_WINDOWPOSCHANGING},
                             {1,WM_WINDOWPOSCHANGING},
                             {1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {4,WM_KILLFOCUS},
                             {1,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate2_chain[]={{1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {3,WM_WINDOWPOSCHANGING},
                             {3,WM_WINDOWPOSCHANGED},
                             {3,WM_NCACTIVATE},
                             {3,WM_ACTIVATE},
                             {1,WM_KILLFOCUS},
                             {3,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate3_chain[]={{3,WM_NCACTIVATE},
                             {3,WM_ACTIVATE},
                             {2,WM_WINDOWPOSCHANGING},
                             {4,WM_WINDOWPOSCHANGING},
                             {1,WM_WINDOWPOSCHANGING},
                             {2,WM_WINDOWPOSCHANGED},
                             {4,WM_WINDOWPOSCHANGED},
                             {1,WM_WINDOWPOSCHANGED},
                             {2,WM_NCACTIVATE},
                             {2,WM_ACTIVATE},
                             {3,WM_KILLFOCUS},
                             {2,WM_SETFOCUS},
                             {0,0}};

MSG_ENTRY Activate4_chain[]={{2,WM_NCACTIVATE},
                             {2,WM_ACTIVATE},
                             {2,WM_WINDOWPOSCHANGING},
                             {4,WM_WINDOWPOSCHANGING},
                             {1,WM_WINDOWPOSCHANGING},
                             {1,WM_NCACTIVATE},
                             {1,WM_ACTIVATE},
                             {2,WM_KILLFOCUS},
                             {1,WM_SETFOCUS},
                             {0,0}};

#define EXPECT_NEXT(hWnd1, hWnd2) ok(GetWindow(hWnd1,GW_HWNDNEXT) == hWnd2, "Expected %p after %p, not %p\n",hWnd2,hWnd1,GetWindow(hWnd1,GW_HWNDNEXT) )
#define EXPECT_ACTIVE(hwnd) ok(GetActiveWindow() == hwnd, "Expected %p to be the active window, not %p\n",hwnd,GetActiveWindow())
#define FLUSH_MESSAGES(msg) while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );


void Test_SetActiveWindow()
{
    MSG msg;

    create_test_windows();

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd1,hWnd3);

    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd3, SW_SHOW);
    UpdateWindow(hWnd3);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd3,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);

    ShowWindow(hWnd4, SW_SHOW);
    UpdateWindow(hWnd4);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd4);

    empty_message_cache();

    SetActiveWindow(hWnd1);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);
    compare_cache("Activate1", Activate1_chain);
    trace_cache();
    empty_message_cache();

    SetActiveWindow(hWnd3);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd3,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd2);
    EXPECT_NEXT(hWnd2,hWnd1);
    EXPECT_ACTIVE(hWnd3);
    compare_cache("Activate2", Activate2_chain);
    trace_cache();
    empty_message_cache();

    SetActiveWindow(hWnd2);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd2);
    compare_cache("Activate3", Activate3_chain);
    trace_cache();
    empty_message_cache();

    SetActiveWindow(hWnd1);
    FLUSH_MESSAGES(msg);
    EXPECT_NEXT(hWnd2,hWnd4);
    EXPECT_NEXT(hWnd4,hWnd1);
    EXPECT_NEXT(hWnd1,hWnd3);
    EXPECT_ACTIVE(hWnd1);
    compare_cache("Activate4", Activate4_chain);
    trace_cache();
    empty_message_cache();
}

START_TEST(SetActiveWindow)
{
    Test_SetActiveWindow();
}
