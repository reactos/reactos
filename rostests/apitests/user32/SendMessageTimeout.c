/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SendMessageTimeout
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winuser.h>
#include "helper.h"

static DWORD dwThread1;
static DWORD dwThread2;
static HANDLE hThread1;
static HANDLE hThread2;
static HWND hWndThread1;
static HWND hWndThread2;

static
void
TestSendMessageTimeout(
    _In_ HWND hWnd,
    _In_ UINT Msg)
{
    LRESULT ret;
    DWORD_PTR result;

    ret = SendMessageTimeoutW(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, NULL);
    ok(ret == 0, "ret = %Id\n", ret);

    result = 0x55555555;
    ret = SendMessageTimeoutW(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, &result);
    ok(ret == 0, "ret = %Id\n", ret);
    ok(result == 0, "result = %Iu\n", result);

    ret = SendMessageTimeoutA(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, NULL);
    ok(ret == 0, "ret = %Id\n", ret);

    result = 0x55555555;
    ret = SendMessageTimeoutA(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, &result);
    ok(ret == 0, "ret = %Id\n", ret);
    ok(result == 0, "result = %Iu\n", result);
}

#define WM_SENDTOOTHERTHREAD (WM_USER + 14)

#define KILL_THREAD1_FLAG 0x40000000
#define KILL_THREAD2_FLAG 0x20000000
#define KILL_THREAD_FLAGS (KILL_THREAD1_FLAG | KILL_THREAD2_FLAG)

static
LRESULT
CALLBACK
WndProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProcW(hWnd, message, wParam, lParam);

    if (hWnd == hWndThread1)
    {
        RECOND_MESSAGE(1, message, SENT, wParam, lParam);
    }
    else if (hWnd == hWndThread2)
    {
        RECOND_MESSAGE(2, message, SENT, wParam, lParam);
    }
    else
    {
        RECOND_MESSAGE(3, message, SENT, wParam, lParam);
    }

    switch (message)
    {
    case WM_SENDTOOTHERTHREAD:
        if (GetCurrentThreadId() == dwThread1)
        {
            if ((wParam & KILL_THREAD2_FLAG) &&
                (wParam & ~KILL_THREAD_FLAGS) > 10)
            {
                TerminateThread(hThread2, 123);
            }
            if ((wParam & KILL_THREAD1_FLAG) &&
                (wParam & ~KILL_THREAD_FLAGS) > 10)
            {
                TerminateThread(hThread1, 456);
            }
            ok(lParam == dwThread2, "lParam = %Iu, expected %lu\n", lParam, dwThread2);
            return SendMessage(hWndThread2, WM_SENDTOOTHERTHREAD, wParam + 1, GetCurrentThreadId());
        }
        else
        {
            ok(lParam == dwThread1, "lParam = %Iu, expected %lu\n", lParam, dwThread1);
            return SendMessage(hWndThread1, WM_SENDTOOTHERTHREAD, wParam + 1, GetCurrentThreadId());
        }
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

static
DWORD
WINAPI
Thread1(
    _Inout_opt_ PVOID Parameter)
{
    MSG msg;

    hWndThread1 = CreateWindowExW(0, L"SendTest", NULL, 0,  10, 10, 20, 20,  NULL, NULL, 0, NULL);
    ok(hWndThread1 != NULL, "CreateWindow failed\n");

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(1, msg.message, POST, 0, 0);
        DispatchMessageA(&msg);
    }

    ResumeThread(hThread2);

    while (MsgWaitForMultipleObjectsEx(1, &hThread2, FALSE, QS_ALLEVENTS, MWMO_ALERTABLE) != WAIT_OBJECT_0)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
                RECOND_MESSAGE(1, msg.message, POST, 0, 0);
            DispatchMessageA(&msg);
        }
    }

    DestroyWindow(hWndThread1);
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(1, msg.message, POST, 0, 0);
        DispatchMessageA(&msg);
    }

    return 6;
}

static
DWORD
WINAPI
Thread2(
    _Inout_opt_ PVOID Parameter)
{
    MSG msg;
    LRESULT ret;
    WPARAM wParam;

    hWndThread2 = CreateWindowExW(0, L"SendTest", NULL, 0,  10, 10, 20, 20,  NULL, NULL, 0, NULL);
    ok(hWndThread2 != NULL, "CreateWindow failed\n");

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(2, msg.message, POST, 0, 0);
        DispatchMessageA(&msg);
    }

    wParam = (WPARAM)Parameter;
    ret = SendMessage(hWndThread1, WM_SENDTOOTHERTHREAD, wParam, dwThread2);
    ok(ret == 0, "ret = %lu\n", ret);

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(2, msg.message, POST, 0, 0);
        DispatchMessageA(&msg);
    }

    DestroyWindow(hWndThread2);
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(2, msg.message, POST, 0, 0);
        DispatchMessageA(&msg);
    }

    return 7;
}

static
void
TestRecursiveInterThreadMessages(
    _In_ BOOL KillThread1,
    _In_ BOOL KillThread2)
{
    PVOID Parameter;
    HANDLE Handles[2];
    BOOL Ret;
    DWORD ExitCode;

    Parameter = (PVOID)((KillThread1 ? KILL_THREAD1_FLAG : 0) |
                        (KillThread2 ? KILL_THREAD2_FLAG : 0));
    hThread1 = CreateThread(NULL, 0, Thread1, Parameter, CREATE_SUSPENDED, &dwThread1);
    hThread2 = CreateThread(NULL, 0, Thread2, Parameter, CREATE_SUSPENDED, &dwThread2);

    ResumeThread(hThread1);

    Handles[0] = hThread1;
    Handles[1] = hThread2;
    WaitForMultipleObjects(2, Handles, TRUE, INFINITE);

    Ret = GetExitCodeThread(hThread1, &ExitCode);
    ok(Ret == TRUE, "GetExitCodeThread failed with %lu\n", GetLastError());
    if (KillThread1)
        ok(ExitCode == 456, "Thread1 exit code is %lu\n", ExitCode);
    else
        ok(ExitCode == 6, "Thread1 exit code is %lu\n", ExitCode);

    Ret = GetExitCodeThread(hThread2, &ExitCode);
    ok(Ret == TRUE, "GetExitCodeThread failed with %lu\n", GetLastError());
    if (KillThread2)
        ok(ExitCode == 123, "Thread2 exit code is %lu\n", ExitCode);
    else
        ok(ExitCode == 7, "Thread2 exit code is %lu\n", ExitCode);

    CloseHandle(hThread2);
    CloseHandle(hThread1);

    //TRACE_CACHE();
}

START_TEST(SendMessageTimeout)
{
    TestSendMessageTimeout(NULL, WM_USER);
    TestSendMessageTimeout(NULL, WM_PAINT);
    TestSendMessageTimeout(NULL, WM_GETICON);

    RegisterSimpleClass(WndProc, L"SendTest");

    TestRecursiveInterThreadMessages(FALSE, FALSE);
    TestRecursiveInterThreadMessages(FALSE, TRUE);
    TestRecursiveInterThreadMessages(TRUE, FALSE);
    TestRecursiveInterThreadMessages(TRUE, TRUE);
}
