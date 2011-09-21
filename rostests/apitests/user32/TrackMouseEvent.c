/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for TrackMouseEvent
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
static int message_cache_size = 0;

HWND hWnd1, hWnd2, hWnd3;

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
		case WM_MOUSELEAVE: return "WM_MOUSELEAVE";
		case WM_MOUSEHOVER: return "WM_MOUSEHOVER";
		case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
		case WM_NCMOUSEHOVER: return "WM_NCMOUSEHOVER";
		case WM_NCHITTEST: return "WM_NCHITTEST";
		case WM_SETCURSOR: return "WM_SETCURSOR";
		case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
        default: return NULL;
    }
}

static void empty_message_cache()
{
    memset(message_cache, 0, sizeof(message_cache));
    message_cache_size = 0;
}
#if 0
static void trace_cache()
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
#endif

static void compare_cache(char* testname, MSG_ENTRY *msg_chain)
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
	
	empty_message_cache();
}

static void record_message(int iwnd, UINT message)
{
    message_cache[message_cache_size].iwnd = iwnd;
    message_cache[message_cache_size].msg = message;
    message_cache_size++;
}

LRESULT CALLBACK TmeTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            else
                return DefWindowProc(hWnd, message, wParam, lParam);

			record_message(iwnd, message);
        }
    }

	if(message == WM_PAINT)
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
        Rectangle(ps.hdc,  ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
		EndPaint(hWnd, &ps);
	}
	
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void create_test_windows()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style            = 0;
    wcex.lpfnWndProc    = TmeTestProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = 0;
    wcex.hIcon            = 0;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = 0;
    wcex.lpszClassName    = L"testClass";
    wcex.hIconSm        = 0;

    RegisterClassExW(&wcex);

    hWnd1 = CreateWindowW(L"testClass", L"test", WS_OVERLAPPEDWINDOW,
                         100, 100, 500, 500, NULL, NULL, 0, NULL);
    hWnd2 = CreateWindowW(L"testClass", L"test", WS_CHILD,
                         50, 50, 200, 200, hWnd1, NULL, 0, NULL);
    hWnd3 = CreateWindowW(L"testClass", L"test", WS_CHILD,
                         150, 150, 200, 200, hWnd1, NULL, 0, NULL);

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    ShowWindow(hWnd3, SW_SHOWNORMAL);
    UpdateWindow(hWnd3);
	//SetWindowPos (hWnd3, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOREDRAW);
}							   
								 
static void TmeStartTracking(HWND hwnd, DWORD Flags)
{
    TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
    tme.dwFlags = Flags;
	tme.hwndTrack  = hwnd;
	tme.dwHoverTime = 1;
	if(!TrackMouseEvent(&tme))
	{
	    trace("failed!\n");
	}	
}

DWORD TmeQuery(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_QUERY|TME_HOVER|TME_LEAVE;
	tme.hwndTrack  = hwnd;
	TrackMouseEvent(&tme);
	return tme.dwFlags;
}

#define FLUSH_MESSAGES(msg) while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
#define EXPECT_TME_FLAGS(hWnd, expected)                                                                 \
    { DWORD flags = TmeQuery(hWnd);                                                                      \
	  ok(flags == (expected),"wrong tme flags. expected %li, and got %li\n", (DWORD)(expected), flags);   \
	}
	
#define EXPECT_QUEUE_STATUS(expected, notexpected)                                                                                  \
    {                                                                                                                  \
	    DWORD status = HIWORD(GetQueueStatus(QS_ALLEVENTS));                                                                       \
		ok(((status) & (expected))== (expected),"wrong queue status. expected %li, and got %li\n", (DWORD)(expected), status);         \
		if(notexpected)                                                                                                              \
		    ok((status & (notexpected))!=(notexpected), "wrong queue status. got non expected %li\n", (DWORD)(notexpected));         \
	}
	
MSG_ENTRY empty_chain[]= {{0,0}};

/* the mouse moves over hwnd2 */
MSG_ENTRY mousemove2_chain[]={{2, WM_NCHITTEST},
                              {2, WM_SETCURSOR},
		                      {1, WM_SETCURSOR},
		                      {2, WM_MOUSEMOVE},
						 	  {0,0}};	

/* the mouse hovers hwnd2 */
MSG_ENTRY mousehover2_chain[]={{2, WM_NCHITTEST},
                               {2, WM_SETCURSOR},
		                       {1, WM_SETCURSOR},
		                       {2, WM_MOUSEMOVE},
		                       {2, WM_MOUSEHOVER},
						 	   {0,0}};			

/* the mouse leaves hwnd2 and moves to hwnd1 */
MSG_ENTRY mouseleave2to1_chain[]={{1, WM_NCHITTEST},
                                  {1, WM_SETCURSOR},
		                          {1, WM_MOUSEMOVE},
		                          {2, WM_MOUSELEAVE},
						 	      {0,0}};	

/* the mouse leaves hwnd2 and moves to hwnd3 */
MSG_ENTRY mouseleave2to3_chain[]={{3, WM_NCHITTEST},
                                  {3, WM_SETCURSOR},
                                  {1, WM_SETCURSOR},
		                          {3, WM_MOUSEMOVE},
		                          {2, WM_MOUSELEAVE},
						 	      {0,0}};									  

void Test_TrackMouseEvent()
{
    MSG msg;
	
	SetCursorPos(0,0);
    create_test_windows();
	FLUSH_MESSAGES(msg);
	empty_message_cache();

    /* the mouse moves over hwnd2 */
	SetCursorPos(220,220);
	FLUSH_MESSAGES(msg);
	compare_cache("mousemove2", mousemove2_chain);
	EXPECT_TME_FLAGS(hWnd2, 0);
	TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
	EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
	FLUSH_MESSAGES(msg);
	compare_cache("empty1", empty_chain);
	
    /* the mouse hovers hwnd2 */
	SetCursorPos(221,221);
	Sleep(100);
	EXPECT_QUEUE_STATUS(QS_TIMER|QS_MOUSEMOVE, QS_POSTMESSAGE);
	EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
	FLUSH_MESSAGES(msg);
	EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
	compare_cache("mousehover2", mousehover2_chain);
	
	/* the mouse leaves hwnd2 and moves to hwnd1 */
	SetCursorPos(150,150);
	EXPECT_QUEUE_STATUS(QS_MOUSEMOVE,QS_TIMER|QS_POSTMESSAGE );
	EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
	FLUSH_MESSAGES(msg);
	EXPECT_TME_FLAGS(hWnd2, 0);
	compare_cache("mouseleave2to1", mouseleave2to1_chain);
		
	FLUSH_MESSAGES(msg);
	compare_cache("empty2", empty_chain);
		
    /* the mouse moves over hwnd2 */
	SetCursorPos(220,220);
	EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER|QS_POSTMESSAGE);
	FLUSH_MESSAGES(msg);
	compare_cache("mousemove2", mousemove2_chain);
	EXPECT_TME_FLAGS(hWnd2, 0);
	compare_cache("empty3", empty_chain);
	TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
	TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
	TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
	TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
	EXPECT_QUEUE_STATUS(0, QS_TIMER|QS_MOUSEMOVE|QS_POSTMESSAGE);
	FLUSH_MESSAGES(msg);
	compare_cache("empty4", empty_chain);
	EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);

	FLUSH_MESSAGES(msg);
	compare_cache("empty5", empty_chain);
	
    /* the mouse moves from hwnd2 to the intersection of hwnd2 and hwnd3 */
	SetCursorPos(300,300);
	EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER|QS_POSTMESSAGE);
	FLUSH_MESSAGES(msg);
	EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
	compare_cache("mousemove2", mousemove2_chain);
	
	FLUSH_MESSAGES(msg);
	compare_cache("empty6", empty_chain);
    
	/* the mouse moves from hwnd2 to hwnd3 */
	SetCursorPos(400,400);
	EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER|QS_POSTMESSAGE);
	FLUSH_MESSAGES(msg);
	EXPECT_TME_FLAGS(hWnd2, 0);
	compare_cache("mousemove2to3", mouseleave2to3_chain);
	
	FLUSH_MESSAGES(msg);
	compare_cache("empty7", empty_chain);
	
	//trace_cache();
	
	//Sleep(2000);
}

START_TEST(TrackMouseEvent)
{
    Test_TrackMouseEvent();
}
