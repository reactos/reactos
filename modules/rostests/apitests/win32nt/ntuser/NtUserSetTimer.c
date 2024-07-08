/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for NtUserSetTimer
 * COPYRIGHT:   Copyright 2008 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2024 Tomas Vesely <turican0@gmail.com>
 */

#include "../win32nt.h"

#define SLEEP_TIME 500
#define TIME_TOLERANCE 0.5

#define TEST1_COUNT 20
#define TEST1_INTERVAL 10

#define TEST2_COUNT 20
#define TEST2_INTERVAL 10

#define TEST3_COUNT 40000
#define TEST3_INTERVAL 10

#define TEST4_COUNT 40000
#define TEST4_INTERVAL 10

typedef struct TIMER_MESSAGE_STATE1
{
    UINT_PTR index;
    UINT counter;
} TIMER_MESSAGE_STATE1;

TIMER_MESSAGE_STATE1 timerId1[TEST1_COUNT];

typedef struct TIMER_MESSAGE_STATE2
{
    int counter;
} TIMER_MESSAGE_STATE2;

TIMER_MESSAGE_STATE2 timerId2[TEST2_COUNT];

void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    for (int i = 0; i < TEST1_COUNT; i++)
        if (timerId1[i].index == idEvent)
            timerId1[i].counter++;
}

void MessageLoop(void)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        timerId2[wParam].counter++;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL test1(void)
{
    int countErrors = 0;

    int minMessages = (SLEEP_TIME / TEST1_INTERVAL) * (1 - TIME_TOLERANCE);
    int maxMessages = (SLEEP_TIME / TEST1_INTERVAL) * (1 + TIME_TOLERANCE);

    for (int i = 0; i < TEST1_COUNT; i++)
    {
        timerId1[i].index = 0;
        timerId1[i].counter = 0;
    }

    for (int i = 0; i < TEST1_COUNT; i++)
    {
        timerId1[i].index = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
        if (timerId1[i].index == 0)
        {
            countErrors++;
        }
    }

    ULONGLONG startTime = GetTickCount();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (GetTickCount() - startTime >= SLEEP_TIME)
        {
            PostQuitMessage(0);
        }
    }

    for (int i = 0; i < TEST1_COUNT; i++)
    {
        if((timerId1[i].counter < minMessages) || (timerId1[i].counter > maxMessages))
        {
            countErrors++;
        }
    }

    for (int i = 0; i < TEST1_COUNT; i++)
    {
        if (KillTimer(NULL, timerId1[i].index) == 0)
        {
            countErrors++;
        }
    }
    if (countErrors == 0)
        return TRUE;
    return FALSE;
}

BOOL test2(void)
{
    int countErrors = 0;

    int minMessages = (SLEEP_TIME / TEST2_INTERVAL) * (1 - TIME_TOLERANCE);
    int maxMessages = (SLEEP_TIME / TEST2_INTERVAL) * (1 + TIME_TOLERANCE);
    
    for (int i = 0; i < TEST2_COUNT; i++)
    {
        timerId2[i].counter = 0;
    }

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TimerWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "TimerWindowClass", "Timer Window", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

    if (hwnd == NULL)
    {
        return FALSE;
    }

    for (int i = 0; i < TEST2_COUNT; i++)
    {
        UINT_PTR locIndex = SetTimer(hwnd, i, TEST2_INTERVAL, NULL);
        if (locIndex == 0)
        {
            countErrors++;
        }
    }

    ULONGLONG startTime = GetTickCount();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (GetTickCount() - startTime >= SLEEP_TIME)
        {
            PostQuitMessage(0);
        }
    }

    for (int i = 0; i < TEST2_COUNT; i++)
    {
        if ((timerId2[i].counter < minMessages) || (timerId2[i].counter > maxMessages))
        {
            countErrors++;
        }
    }

    for (int i = 0; i < TEST2_COUNT; i++)
    {
        if (KillTimer(hwnd, i) == 0)
        {
            countErrors++;
        }
    }
    DestroyWindow(hwnd);
    UnregisterClass("TimerWindowClass", GetModuleHandle(NULL));

    if (countErrors == 0)
        return TRUE;
    return FALSE;
}

BOOL test3(void)
{
    int countErrors = 0;

    for (long i = 0; i < TEST3_COUNT; i++)
    {
        UINT_PTR locIndex = SetTimer(NULL, 0, TEST3_INTERVAL, TimerProc);
        if (locIndex == 0)
        {
            countErrors++;
        }
        if (KillTimer(NULL, locIndex) == 0)
        {
            countErrors++;
        }
    }

    if (countErrors == 0)
        return TRUE;
    return FALSE;
}

BOOL test4(void)
{
    int countErrors = 0;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TimerWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "TimerWindowClass", "Timer Window", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

    if (hwnd == NULL)
    {
        return FALSE;
    }

    for (int i = 0; i < TEST4_COUNT; i++)
    {
        UINT_PTR result = SetTimer(hwnd, 1, TEST4_INTERVAL, NULL);
        if (result == 0)
            countErrors++;
        if (KillTimer(hwnd, 1) == 0)
            countErrors++;
    }

    DestroyWindow(hwnd);
    UnregisterClass("TimerWindowClass", GetModuleHandle(NULL));

    if (countErrors == 0)
        return TRUE;
    return FALSE;
}

BOOL test5(void)
{
    int countErrors = 0;

    UINT_PTR locIndex1 = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
    if (locIndex1 == 0)
        countErrors++;
    if (KillTimer(NULL, locIndex1) == 0)
        countErrors++;
    UINT_PTR locIndex2 = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
    if (locIndex2 == 0)
        countErrors++;
    if (KillTimer(NULL, locIndex2) == 0)
        countErrors++;
    if(locIndex1 == locIndex2)
        countErrors++;
    if (countErrors == 0)
        return TRUE;
    return FALSE;
}

START_TEST(NtUserSetTimer)
{
    // TEST WITH MESSAGES WITHOUT WINDOW - test count of sent messages
    TEST(test1());

    // TEST WITH MESSAGES WITH WINDOW - test count of sent messages
    TEST(test2());

    // TEST WITH MESSAGES WITHOUT WINDOW - create many timers
    TEST(test3());

    // TEST WITH MESSAGES WITH WINDOW - create many timers
    TEST(test4());

    // TEST WITH MESSAGES WITHOUT WINDOW - test different ids
    TEST(test5());
}
