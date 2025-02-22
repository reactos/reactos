/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for EmptyClipboard
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

static HWND hWnd1, hWnd2;

static CRITICAL_SECTION CritSect;

/* FIXME: test for HWND_TOP, etc...*/
static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd2 && hWnd == hWnd2) return 2;
    else return 0;
}

LRESULT CALLBACK ClipTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_GETICON :
    case WM_SETICON:
        break;
    case WM_DESTROYCLIPBOARD:
        Sleep(1000);
    default:
      	EnterCriticalSection(&CritSect);
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
        LeaveCriticalSection(&CritSect);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void RecordAndDispatch(MSG* pmsg)
{
    int iwnd = get_iwnd(pmsg->hwnd);
    if(!(pmsg->message > WM_USER || !iwnd || IsDWmMsg(pmsg->message) || IseKeyMsg(pmsg->message)))
    {
        EnterCriticalSection(&CritSect);
        RECORD_MESSAGE(iwnd, pmsg->message, POST,0,0);
        LeaveCriticalSection(&CritSect);
    }
    DispatchMessageA( pmsg );
}

static void FlushMessages()
{
    MSG msg;
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
        RecordAndDispatch(&msg);
}

static DWORD WINAPI ClipThread(PVOID Parameter)
{
    BOOL ret;

    hWnd2 = CreateWindowW(L"clipstest", L"clipstest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);

    /* Get ownership of the clipboard and record the messages we get */
    ret = OpenClipboard(hWnd2);
    ok (ret, "OpenClipboard failed\n");

    EnterCriticalSection(&CritSect);
    RECORD_MESSAGE(1, WM_DESTROYCLIPBOARD, MARKER,0,0);
    LeaveCriticalSection(&CritSect);

    ret = EmptyClipboard();
    ok (ret, "EmptyClipboard failed\n");

    EnterCriticalSection(&CritSect);
    RECORD_MESSAGE(1, WM_DESTROYCLIPBOARD, MARKER,1,0);
    LeaveCriticalSection(&CritSect);

    ret = CloseClipboard();
    ok (ret, "CloseClipboard failed\n");

    /* Record if we got any post messages */
    FlushMessages();

    /* Force the parent thread out of its loop */
    PostMessageW(hWnd1, WM_QUIT, 0, 0);

    return 0;
}

static MSG_ENTRY EmptyClipboard_chain[]={
    {1,WM_DESTROYCLIPBOARD, MARKER, 0},
    {1,WM_DESTROYCLIPBOARD, SENT},
    {1,WM_DESTROYCLIPBOARD, MARKER, 1},
    {0,0}};

void TestMessages()
{
    HANDLE hThread;
    DWORD dwThread;
    BOOL ret;
    MSG msg;

    /* Initialize the test */
    InitializeCriticalSection(&CritSect);

    RegisterSimpleClass(ClipTestProc, L"clipstest");

    hWnd2 = NULL;
    hWnd1 = CreateWindowW(L"clipstest", L"clipstest", WS_OVERLAPPEDWINDOW,
                        20, 20, 300, 300, NULL, NULL, 0, NULL);
    ok (hWnd1 != NULL, "CreateWindowW failed\n");

    EMPTY_CACHE();

    /* Get ownership of the clipboard and record the messages we get */
    ret = OpenClipboard(hWnd1);
    ok (ret, "OpenClipboard failed\n");

    ret = EmptyClipboard();
    ok (ret, "EmptyClipboard failed\n");

    ret = CloseClipboard();
    ok (ret, "CloseClipboard failed\n");

    /* Record posted messages */
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* Create the child thread and record messages till we get the WM_QUIT */
    hThread = CreateThread(NULL, 0, ClipThread, NULL, 0, &dwThread);

    while (GetMessage(&msg, 0, 0 ,0))
        RecordAndDispatch(&msg);

    COMPARE_CACHE(EmptyClipboard_chain);

    CloseHandle(hThread);
}

void TestOwnership()
{
    BOOL ret;
    HWND hWnd, hWndClipOwner;

    ret = OpenClipboard(NULL);
    ok (ret, "OpenClipboard failed\n");

    ret = EmptyClipboard();
    ok (ret, "EmptyClipboard failed\n");

    ret = CloseClipboard();
    ok (ret, "CloseClipboard failed\n");

    hWndClipOwner = GetClipboardOwner();
    ok (hWndClipOwner == NULL, "Expected NULL owner\n");

    hWnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW, 20, 20, 300, 300, NULL, NULL, 0, NULL);
    ok (hWnd != 0 , "CreateWindowW failed\n");

    ret = OpenClipboard(hWnd);
    ok (ret, "OpenClipboard failed\n");

    hWndClipOwner = GetClipboardOwner();
    ok (hWndClipOwner == NULL, "Expected NULL owner\n");

    ret = EmptyClipboard();
    ok (ret, "EmptyClipboard failed\n");

    ret = CloseClipboard();
    ok (ret, "CloseClipboard failed\n");

    hWndClipOwner = GetClipboardOwner();
    ok (hWndClipOwner == hWnd, "Expected hWnd owner\n");

    DestroyWindow(hWnd);
}

START_TEST(EmptyClipboard)
{
    TestOwnership();
    TestMessages();
}
