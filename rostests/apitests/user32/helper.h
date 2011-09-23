

typedef struct _MSG_ENTRY
{
    int iwnd;
    UINT msg;
    BOOL hook;
    int param1;
    int param2;
} MSG_ENTRY;

void record_message(int iwnd, UINT message, BOOL hook, int param1,int param2);
void compare_cache(const char* file, int line, MSG_ENTRY *msg_chain);
void trace_cache(const char* file, int line);
void empty_message_cache();
ATOM RegisterSimpleClass(WNDPROC lpfnWndProc, LPCWSTR lpszClassName);

#define COMPARE_CACHE(...) compare_cache(__FILE__, __LINE__, ##__VA_ARGS__)
#define TRACE_CACHE() trace_cache(__FILE__, __LINE__)

#define FLUSH_MESSAGES(msg) while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
#define EXPECT_NEXT(hWnd1, hWnd2) ok(GetWindow(hWnd1,GW_HWNDNEXT) == hWnd2, "Expected %p after %p, not %p\n",hWnd2,hWnd1,GetWindow(hWnd1,GW_HWNDNEXT) )
#define EXPECT_ACTIVE(hwnd) ok(GetActiveWindow() == hwnd, "Expected %p to be the active window, not %p\n",hwnd,GetActiveWindow())

#define EXPECT_QUEUE_STATUS(expected, notexpected)                                                                              \
    {                                                                                                                           \
        DWORD status = HIWORD(GetQueueStatus(QS_ALLEVENTS));                                                                    \
        ok(((status) & (expected))== (expected),"wrong queue status. expected %li, and got %li\n", (DWORD)(expected), status);  \
        if(notexpected)                                                                                                         \
            ok((status & (notexpected))!=(notexpected), "wrong queue status. got non expected %li\n", (DWORD)(notexpected));    \
    }
