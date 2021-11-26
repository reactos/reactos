/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AttachThreadInput
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

typedef struct {
    DWORD tid;
    HANDLE hThread;
    HWND hWnd;
    WCHAR* Desktop;
    HANDLE StartEvent;
    HANDLE QueueStatusEvent;
    DWORD LastQueueStatus;

    MSG_CACHE cache;
} THREAD_DATA;

DWORD tidMouseMove;
THREAD_DATA data[6];
HHOOK hMouseHookLL = NULL;
HHOOK hKbdHookLL = NULL;


#define EXPECT_FOREGROUND(expected) ok(GetForegroundWindow() == expected, \
                                       "Expected hwnd%d at the foreground, got hwnd%d\n", \
                                       get_iwnd(expected), get_iwnd(GetForegroundWindow()));

#define EXPECT_ACTIVE(expected) ok(GetActiveWindow() == expected, \
                                   "Expected hwnd%d to be active, got hwnd%d\n", \
                                   get_iwnd(expected), get_iwnd(GetActiveWindow()));

/*
 *  Helper functions
 */

static int get_iwnd(HWND hWnd)
{
    if(hWnd == data[0].hWnd) return 0;
    else if(hWnd == data[1].hWnd) return 1;
    else if(hWnd == data[2].hWnd) return 2;
    else if(hWnd == data[3].hWnd) return 3;
    else if(hWnd == data[4].hWnd) return 4;
    else return -1;
}

LRESULT CALLBACK TestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(iwnd >= 0 && message > 0 && message < WM_APP && message != WM_TIMER)
        record_message(&data[iwnd].cache, iwnd, message, SENT, wParam,0);

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;
    LRESULT res;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if( iwnd >= 0 && msg.message > 0 && msg.message < WM_APP && msg.message != WM_TIMER)
            record_message(&data[0].cache, iwnd, msg.message, POST, msg.wParam,0);
        DispatchMessageA( &msg );
    }

    /* Use SendMessage to sync with the other queues */
    res = SendMessageTimeout(data[1].hWnd, WM_APP, 0,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
    res = SendMessageTimeout(data[2].hWnd, WM_APP, 0,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
    res = SendMessageTimeout(data[3].hWnd, WM_APP, 0,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
    res = SendMessageTimeout(data[4].hWnd, WM_APP, 0,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
}

static DWORD WINAPI thread_proc(void *param)
{
    THREAD_DATA* current_data = (THREAD_DATA*)param;
    MSG msg;
    HDESK hdesk = NULL;
    int iwnd;

    if(current_data->Desktop)
    {
        hdesk = CreateDesktopW(current_data->Desktop, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
        SetThreadDesktop(hdesk);
    }

    /* create test window */
    current_data->hWnd = CreateWindowW(L"TestClass", L"test", WS_OVERLAPPEDWINDOW,
                                       100, 100, 500, 500, NULL, NULL, 0, NULL);
    SetEvent( current_data->StartEvent );

    iwnd = get_iwnd(current_data->hWnd);

    /* Use MsgWaitForMultipleObjects to let the thread process apcs */
    while( GetMessage(&msg, 0,0,0) )
    {
        if(msg.message > 0 && msg.message < WM_APP && msg.message != WM_TIMER )
            record_message(&data[iwnd].cache, iwnd, msg.message, POST, msg.wParam,0);
        DispatchMessage(&msg);
    }

    if(hdesk)
        CloseDesktop(hdesk);

    return 0;
}

BOOL CreateTestThread(int i, WCHAR* Desktop)
{
    DWORD ret;

    data[i].StartEvent = CreateEventW(NULL, 0, 0, NULL);
    data[i].Desktop = Desktop;
    data[i].hThread = CreateThread(NULL, 0, thread_proc, &data[i], 0, &data[i].tid);
    if(!data[i].hThread) goto fail;
    ret = WaitForSingleObject(data[i].StartEvent, 1000);
    CloseHandle(data[i].StartEvent);
    if(ret == WAIT_TIMEOUT)
    {
fail:
        win_skip("child thread failed to initialize\n");
        return FALSE;
    }
    return TRUE;
}

static LRESULT CALLBACK MouseLLHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    MSLLHOOKSTRUCT* params = (MSLLHOOKSTRUCT*) lParam;

    ret = CallNextHookEx(hMouseHookLL, nCode, wParam, lParam);

    if((params->flags & LLKHF_INJECTED) == 0)
        return TRUE;

    return ret;
}

LRESULT CALLBACK KbdLLHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    KBDLLHOOKSTRUCT* params = (KBDLLHOOKSTRUCT*) lParam;

    ret = CallNextHookEx(hMouseHookLL, nCode, wParam, lParam);

    if((params->flags & LLKHF_INJECTED) == 0)
        return TRUE;

    return ret;
}

BOOLEAN InitThreads()
{
    /* Create a LL hook that drops any physical keyboard and mouse action
       and prevent the user from interfering with the test results */
    if(!IsDebuggerPresent())
    {
        hMouseHookLL = SetWindowsHookExW(WH_MOUSE_LL, MouseLLHookProc, GetModuleHandleW( NULL ), 0);
        ok(hMouseHookLL!=NULL,"failed to set hook\n");
        hKbdHookLL = SetWindowsHookExW(WH_KEYBOARD_LL, KbdLLHookProc, GetModuleHandleW( NULL ), 0);
        ok(hKbdHookLL!=NULL,"failed to set hook\n");
    }

    /* create test clases */
    RegisterSimpleClass(TestProc, L"TestClass");

    memset(&data[0], 0, sizeof(data[0]));

    data[0].tid = GetCurrentThreadId();

    /* create test window */
    data[0].hWnd = CreateWindowW(L"TestClass", L"test", WS_OVERLAPPEDWINDOW,
                                 100, 100, 500, 500, NULL, NULL, 0, NULL);
    if(!data[0].hWnd)
    {
        win_skip("CreateWindowW failed\n");
        return FALSE;
    }

    /* create thread1(same desktop) */
    if(!CreateTestThread(1, NULL)) return FALSE;

    /* create thread2(same desktop) */
    if(!CreateTestThread(2, NULL)) return FALSE;

    /* ugly ros hack to bypass desktop crapiness */
    if(!CreateTestThread(6, L"ThreadTestDesktop")) return FALSE;

    /* create thread3(different desktop) */
    if(!CreateTestThread(3, L"ThreadTestDesktop")) return FALSE;

    /* create thread4(different desktop) */
    if(!CreateTestThread(4, L"ThreadTestDesktop")) return FALSE;

    return TRUE;
}

static void cleanup_attachments()
{
    int i,j;
    BOOL ret;

    for(i = 0; i< 4; i++)
    {
        for(j = 0; j< 4; j++)
        {
            ret = AttachThreadInput(data[i].tid,data[j].tid, FALSE);
            ok(ret==0, "expected AttachThreadInput to fail\n");
        }
    }
}




/*
 *  The actual tests
 */

void Test_SimpleParameters()
{
    BOOL ret;
    /* FIXME: acording to msdn xp doesn't set last error but vista+ does*/

    /* test wrong thread */
    ret = AttachThreadInput( 0, 1, TRUE);
    ok(ret==0, "expected AttachThreadInput to fail\n");

    /* test same thread */
    ret = AttachThreadInput( data[1].tid, data[1].tid, TRUE);
    ok(ret==0, "expected AttachThreadInput to fail\n");

    /* try to attach to a thread on another desktop*/
    ret = AttachThreadInput( data[2].tid,data[3].tid, TRUE);
    ok(ret==0, "expected AttachThreadInput to fail\n");
    if(ret == 1 )
        AttachThreadInput( data[2].tid,data[3].tid, FALSE);

    /* test other desktop to this */
    ret = AttachThreadInput( data[3].tid,data[2].tid, TRUE);
    ok(ret==0, "expected AttachThreadInput to fail\n");
    if(ret == 1 )
        AttachThreadInput( data[3].tid,data[2].tid, FALSE);

    /* attach two threads that are both in ThreadTestDesktop */
    {
        /* Attach thread 3 and 4 */
        ret = AttachThreadInput( data[3].tid,data[4].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* cleanup previous attachment */
        ret = AttachThreadInput( data[3].tid,data[4].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    {
        /* Attach thread 1 and 2 */
        ret = AttachThreadInput( data[1].tid,data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* attach already attached*/
        ret = AttachThreadInput( data[1].tid,data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* attach in the opposite order */
        ret = AttachThreadInput( data[2].tid,data[1].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* Now try to detach 0 from 1 */
        ret = AttachThreadInput( data[0].tid,data[1].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");

        /* also try to detach 3 from 2 */
        ret = AttachThreadInput( data[3].tid,data[2].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");

        /* cleanup previous attachment */
        ret = AttachThreadInput( data[1].tid,data[2].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        ret = AttachThreadInput( data[2].tid,data[1].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        ret = AttachThreadInput( data[1].tid,data[2].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* test triple attach */
    {
        ret = AttachThreadInput( data[0].tid, data[1].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        ret = AttachThreadInput( data[1].tid, data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* try to detach 2 and 0 */
        ret = AttachThreadInput( data[0].tid, data[2].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");
        ret = AttachThreadInput( data[2].tid, data[0].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");

        /* try to to attach 0 to 2. it works! */
        ret = AttachThreadInput( data[0].tid, data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        ret = AttachThreadInput( data[0].tid, data[2].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        /* detach in inverse order */
        ret = AttachThreadInput( data[0].tid, data[1].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        ret = AttachThreadInput( data[1].tid, data[2].tid, FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* test detaching in thread cleanup */
    {
        ret = AttachThreadInput( data[0].tid, data[1].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        ret = AttachThreadInput( data[0].tid, data[1].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        ret = AttachThreadInput( data[1].tid, data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        ret = AttachThreadInput( data[1].tid, data[2].tid, TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        TerminateThread(data[1].hThread, 0);

        ret = AttachThreadInput( data[0].tid, data[1].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");
        ret = AttachThreadInput( data[1].tid, data[2].tid, FALSE);
        ok(ret==0, "expected AttachThreadInput to fail\n");

        /* Create Thread1 again */
        CreateTestThread(1, NULL);
    }

}

void Test_Focus() //Focus Active Capture Foreground Capture
{
    BOOL ret;

    trace("Thread hWnd0 0x%p hWnd1 0x%p\n",data[0].hWnd, data[1].hWnd);
    /* Window 1 is in the foreground */
    SetForegroundWindow(data[1].hWnd);
    SetActiveWindow(data[0].hWnd);
    FlushMessages();

    EXPECT_FOREGROUND(data[1].hWnd);
    EXPECT_ACTIVE(data[0].hWnd);

    /* attach thread 0 to 1 */
    {
        ret = AttachThreadInput( data[0].tid, data[1].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        FlushMessages();

        EXPECT_FOREGROUND(data[1].hWnd);
        EXPECT_ACTIVE(data[1].hWnd);

        ret = AttachThreadInput( data[0].tid, data[1].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    EXPECT_FOREGROUND(data[1].hWnd);
    EXPECT_ACTIVE(0);

    SetForegroundWindow(data[1].hWnd);
    SetActiveWindow(data[0].hWnd);
    FlushMessages();

    EXPECT_FOREGROUND(data[1].hWnd);
    EXPECT_ACTIVE(data[0].hWnd);

    /* attach thread 1 to 0 */
    {
        ret = AttachThreadInput( data[1].tid, data[0].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        FlushMessages();

        EXPECT_FOREGROUND(data[1].hWnd);
        EXPECT_ACTIVE(data[1].hWnd);

        ret = AttachThreadInput( data[1].tid, data[0].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* Window 0 is in the foreground */
    SetForegroundWindow(data[0].hWnd);
    SetActiveWindow(data[1].hWnd);
    FlushMessages();

    EXPECT_FOREGROUND(data[0].hWnd);
    EXPECT_ACTIVE(data[0].hWnd);

    /* attach thread 0 to 1 */
    {
        ret = AttachThreadInput( data[0].tid, data[1].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        FlushMessages();

        EXPECT_FOREGROUND(data[0].hWnd);
        EXPECT_ACTIVE(data[0].hWnd);

        SetForegroundWindow(data[0].hWnd);
        SetActiveWindow(data[1].hWnd);
        FlushMessages();

        EXPECT_FOREGROUND(data[1].hWnd);
        EXPECT_ACTIVE(data[1].hWnd);

        ret = AttachThreadInput( data[0].tid, data[1].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    EXPECT_FOREGROUND(data[1].hWnd);
    EXPECT_ACTIVE(0);

    SetForegroundWindow(data[0].hWnd);
    SetActiveWindow(data[1].hWnd);
    FlushMessages();

    EXPECT_FOREGROUND(data[0].hWnd);
    EXPECT_ACTIVE(data[0].hWnd);

    /* attach thread 1 to 0 */
    {
        ret = AttachThreadInput( data[1].tid, data[0].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
        FlushMessages();

        EXPECT_FOREGROUND(data[0].hWnd);
        EXPECT_ACTIVE(data[0].hWnd);

        SetForegroundWindow(data[0].hWnd);
        SetActiveWindow(data[1].hWnd);
        FlushMessages();

        EXPECT_FOREGROUND(data[1].hWnd);
        EXPECT_ACTIVE(data[1].hWnd);

        ret = AttachThreadInput( data[1].tid, data[0].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }
}

/* test some functions like PostMessage and SendMessage that shouldn't be affected */
void Test_UnaffectedMessages()
{
    BOOL ret;
    LRESULT res;

    EMPTY_CACHE_(&data[0].cache);
    EMPTY_CACHE_(&data[1].cache);

    /* test that messages posted before and after attachment are unaffected
       and that we don't receive a meassage from a window we shouldn't */
    PostMessage(data[0].hWnd, WM_USER, 0,0);
    PostMessage(data[1].hWnd, WM_USER, 1,0);

    {
        MSG_ENTRY Thread0_chain[]={
          {0,WM_USER, POST, 0, 0},
          {0,WM_USER, POST, 2, 0},
          {0,0}};
        MSG_ENTRY Thread1_chain[]={
          {1,WM_USER, POST, 1, 0},
          {1,WM_USER, POST, 3, 0},
          {0,0}};

        ret = AttachThreadInput( data[1].tid, data[0].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        PostMessage(data[0].hWnd, WM_USER, 2,0);
        PostMessage(data[1].hWnd, WM_USER, 3,0);

        FlushMessages();
        Sleep(100);

        COMPARE_CACHE_(&data[0].cache, Thread0_chain);
        COMPARE_CACHE_(&data[1].cache, Thread1_chain);

        ret = AttachThreadInput( data[1].tid, data[0].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* test messages send to the wrong thread */
    res = SendMessageTimeout(data[0].hWnd, WM_USER, 0,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
    res = SendMessageTimeout(data[1].hWnd, WM_USER, 1,0, SMTO_NORMAL, 1000, NULL);
    ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");

    {
        MSG_ENTRY Thread0_chain[]={
          {0,WM_USER, SENT, 0, 0},
          {0,WM_USER, SENT, 2, 0},
          {0,0}};
        MSG_ENTRY Thread1_chain[]={
          {1,WM_USER, SENT, 1, 0},
          {1,WM_USER, SENT, 3, 0},
          {1,WM_MOUSEMOVE, SENT, 0, 0},
          {0,0}};

        ret = AttachThreadInput( data[2].tid, data[1].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        res = SendMessageTimeout(data[0].hWnd, WM_USER, 2,0, SMTO_NORMAL, 1000, NULL);
        ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");
        res = SendMessageTimeout(data[1].hWnd, WM_USER, 3,0, SMTO_NORMAL, 1000, NULL);
        ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");

        /* Try to send a fake input message */
        res = SendMessageTimeout(data[1].hWnd, WM_MOUSEMOVE, 0,0, SMTO_NORMAL, 1000, NULL);
        ok (res != ERROR_TIMEOUT, "SendMessageTimeout timed out\n");

        COMPARE_CACHE_(&data[0].cache, Thread0_chain);
        COMPARE_CACHE_(&data[1].cache, Thread1_chain);

        ret = AttachThreadInput( data[2].tid, data[1].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* todo: test keyboard layout that shouldn't be affected */
}

void Test_SendInput()
{
    MSG_ENTRY Thread1_chain[]={
          {1,WM_KEYDOWN, POST, VK_SHIFT, 0},
          {1,WM_KEYUP, POST, VK_SHIFT, 0},
          {0,0}};
    MSG_ENTRY Thread0_chain[]={
          {0,WM_KEYDOWN, POST, VK_SHIFT, 0},
          {0,WM_KEYUP, POST, VK_SHIFT, 0},
          {0,0}};

    BOOL ret;

    //trace("Thread hWnd0 0x%p hWnd1 0x%p\n",data[0].hWnd, data[1].hWnd);

    /* First try sending input without attaching. It will go to the foreground */
    {
        SetForegroundWindow(data[1].hWnd);
        SetActiveWindow(data[0].hWnd);

        ok(GetForegroundWindow() == data[1].hWnd, "wrong foreground got 0x%p\n",GetForegroundWindow());
        ok(GetActiveWindow() == data[0].hWnd, "wrong active got 0x%p\n",GetActiveWindow());

        FlushMessages();
        EMPTY_CACHE_(&data[0].cache);
        EMPTY_CACHE_(&data[1].cache);

        keybd_event(VK_SHIFT, 0,0,0);
        keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP,0);
        Sleep(100);
        FlushMessages();

        COMPARE_CACHE_(&data[0].cache, empty_chain);
        COMPARE_CACHE_(&data[1].cache, Thread1_chain);
    }

    /* Next attach and send input. It will go to the same thread as before */
    { //                          from           to
        ret = AttachThreadInput( data[1].tid, data[0].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        FlushMessages();
        EMPTY_CACHE_(&data[0].cache);
        EMPTY_CACHE_(&data[1].cache);

        keybd_event(VK_SHIFT, 0,0,0);
        keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP,0);
        Sleep(100);
        FlushMessages();

        COMPARE_CACHE_(&data[0].cache, empty_chain);
        COMPARE_CACHE_(&data[1].cache, Thread1_chain);
    }

    /* Now set foreground and active again. Input will go to thread 0 */
    {
        SetForegroundWindow(data[1].hWnd);
        SetActiveWindow(data[0].hWnd);
        FlushMessages();

        ok(GetForegroundWindow() == data[0].hWnd, "wrong foreground got 0x%p\n",GetForegroundWindow());
        ok(GetActiveWindow() == data[0].hWnd, "wrong active got 0x%p\n",GetActiveWindow());

        EMPTY_CACHE_(&data[0].cache);
        EMPTY_CACHE_(&data[1].cache);

        keybd_event(VK_SHIFT, 0,0,0);
        keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP,0);
        Sleep(100);
        FlushMessages();

        COMPARE_CACHE_(&data[0].cache, Thread0_chain);
        COMPARE_CACHE_(&data[1].cache, empty_chain);

        ret = AttachThreadInput( data[1].tid, data[0].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }

    /* Attach in the opposite order and send input */
    {
        ret = AttachThreadInput( data[0].tid, data[1].tid , TRUE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");

        FlushMessages();
        EMPTY_CACHE_(&data[0].cache);
        EMPTY_CACHE_(&data[1].cache);

        keybd_event(VK_SHIFT, 0,0,0);
        keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP,0);
        Sleep(100);
        FlushMessages();

        COMPARE_CACHE_(&data[0].cache, Thread0_chain);
        COMPARE_CACHE_(&data[1].cache, empty_chain);
    }

    /* Now set foreground and active again. Input will go to thread 0 */
    {
        SetForegroundWindow(data[1].hWnd);
        SetActiveWindow(data[0].hWnd);
        FlushMessages();

        ok(GetForegroundWindow() == data[0].hWnd, "wrong foreground got 0x%p\n",GetForegroundWindow());
        ok(GetActiveWindow() == data[0].hWnd, "wrong active got 0x%p\n",GetActiveWindow());

        EMPTY_CACHE_(&data[0].cache);
        EMPTY_CACHE_(&data[1].cache);

        keybd_event(VK_SHIFT, 0,0,0);
        keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP,0);
        Sleep(100);
        FlushMessages();

        COMPARE_CACHE_(&data[0].cache, Thread0_chain);
        COMPARE_CACHE_(&data[1].cache, empty_chain);

        ret = AttachThreadInput( data[0].tid, data[1].tid , FALSE);
        ok(ret==1, "expected AttachThreadInput to succeed\n");
    }
}

START_TEST(AttachThreadInput)
{
    if(!InitThreads())
        return;

    Test_SimpleParameters();
    cleanup_attachments();
    Test_Focus();
    cleanup_attachments();
    Test_UnaffectedMessages();
    cleanup_attachments();
    Test_SendInput();
    cleanup_attachments();

    if(hMouseHookLL)
        UnhookWindowsHookEx(hMouseHookLL);
    if(hKbdHookLL)
        UnhookWindowsHookEx(hKbdHookLL);

    /* Stop all threads and exit gratefully */
    PostThreadMessage(data[1].tid, WM_QUIT,0,0);
    PostThreadMessage(data[2].tid, WM_QUIT,0,0);
    PostThreadMessage(data[3].tid, WM_QUIT,0,0);
    PostThreadMessage(data[4].tid, WM_QUIT,0,0);
}

