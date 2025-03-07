#ifndef APITESTS_MSGTRACE_H
#define APITESTS_MSGTRACE_H

typedef enum _MSG_TYPE
{
    SENT,
    POST,
    HOOK,
    EVENT,
    SENT_RET,
    MARKER
} MSG_TYPE;

typedef struct _MSG_ENTRY
{
    int iwnd;
    UINT msg;
    MSG_TYPE type;
    int param1;
    int param2;
} MSG_ENTRY;

typedef struct _MSG_CACHE
{
    MSG_ENTRY last_post_message;
    MSG_ENTRY message_cache[100];
    int count;
} MSG_CACHE;

extern MSG_ENTRY empty_chain[];
extern MSG_CACHE default_cache;

void record_message(MSG_CACHE* cache, int iwnd, UINT message, MSG_TYPE type, int param1,int param2);
void compare_cache(MSG_CACHE* cache, const char* file, int line, MSG_ENTRY *msg_chain);
void trace_cache(MSG_CACHE* cache, const char* file, int line);
void empty_message_cache(MSG_CACHE* cache);

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

#define COMPARE_CACHE(msg_chain) compare_cache(&default_cache, __FILE__, __LINE__, msg_chain)
#define TRACE_CACHE() trace_cache(&default_cache, __FILE__, __LINE__)
#define EMPTY_CACHE() empty_message_cache(&default_cache);
#define RECORD_MESSAGE(...) record_message(&default_cache, ##__VA_ARGS__);

#define COMPARE_CACHE_(cache, msg_chain) compare_cache(cache, __FILE__, __LINE__, msg_chain)
#define TRACE_CACHE_(cache) trace_cache(cache, __FILE__, __LINE__)
#define EMPTY_CACHE_(cache) empty_message_cache(cache);

#define EXPECT_QUEUE_STATUS(expected, notexpected)                                                                              \
    {                                                                                                                           \
        DWORD status = HIWORD(GetQueueStatus(QS_ALLEVENTS));                                                                    \
        ok(((status) & (expected))== (expected),"wrong queue status. expected %li, and got %li\n", (DWORD)(expected), status);  \
        if(notexpected)                                                                                                         \
            ok((status & (notexpected))!=(notexpected), "wrong queue status. got non expected %li\n", (DWORD)(notexpected));    \
    }

#endif
