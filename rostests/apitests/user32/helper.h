
typedef enum _MSG_TYPE
{
    SENT,
    POST,
    HOOK,
    EVENT
} MSG_TYPE;

typedef struct _MSG_ENTRY
{
    int iwnd;
    UINT msg;
    MSG_TYPE type;
    int param1;
    int param2;
} MSG_ENTRY;


extern MSG_ENTRY empty_chain[];

void record_message(int iwnd, UINT message, MSG_TYPE type, int param1,int param2);
void compare_cache(const char* file, int line, MSG_ENTRY *msg_chain);
void trace_cache(const char* file, int line);
void empty_message_cache();
ATOM RegisterSimpleClass(WNDPROC lpfnWndProc, LPCWSTR lpszClassName);

/* filter messages that are affected by dwm */
static inline BOOL IsDWmMsg(UINT msg)
{
    switch(msg)
    {
    case WM_NCPAINT:
    case WM_ERASEBKGND:
    case WM_PAINT:
    case 0x031f:  /*WM_DWMNCRENDERINGCHANGED*/
        return TRUE;
    }
    return FALSE;
}

static inline BOOL IseKeyMsg(UINT msg)
{
    return (msg == WM_KEYUP || msg == WM_KEYDOWN);
}

#define COMPARE_CACHE(...) compare_cache(__FILE__, __LINE__, ##__VA_ARGS__)
#define TRACE_CACHE() trace_cache(__FILE__, __LINE__)

#define EXPECT_ACTIVE(hwnd) ok(GetActiveWindow() == hwnd, "Expected %p to be the active window, not %p\n",hwnd,GetActiveWindow())

#define EXPECT_QUEUE_STATUS(expected, notexpected)                                                                              \
    {                                                                                                                           \
        DWORD status = HIWORD(GetQueueStatus(QS_ALLEVENTS));                                                                    \
        ok(((status) & (expected))== (expected),"wrong queue status. expected %li, and got %li\n", (DWORD)(expected), status);  \
        if(notexpected)                                                                                                         \
            ok((status & (notexpected))!=(notexpected), "wrong queue status. got non expected %li\n", (DWORD)(notexpected));    \
    }
