/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for NtUserSetTimer
 * COPYRIGHT:   Copyright 2008 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2024 Tomáš Veselý <turican0@gmail.com>
 */

#include "../win32nt.h"

#define SLEEP_TIME 500
#define MIN_MESSAGES_TIME 1
#define MAX_MESSAGES_TIME 1000

#define TEST1_COUNT 20
#define TEST1_INTERVAL 10

#define TEST2_COUNT 40000
#define TEST2_INTERVAL 10

#define TESTW1_COUNT 20
#define TESTW1_INTERVAL 10

#define TESTW2_COUNT 40000
#define TESTW2_INTERVAL 10

typedef struct TIMER_MESSAGE_STATE1
{
    UINT_PTR index;
    UINT counter;
} TIMER_MESSAGE_STATE1;

TIMER_MESSAGE_STATE1 timerId1[TEST1_COUNT];

typedef struct TIMER_MESSAGE_STATEW1
{
    UINT counter;
} TIMER_MESSAGE_STATEW1;

TIMER_MESSAGE_STATEW1 timerIdW1[TESTW1_COUNT];

/* TIMERPROC for the test1,2,3() with messages without window */
static void CALLBACK
TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    UINT i;
    for (i = 0; i < TEST1_COUNT; i++)
    {
        if (timerId1[i].index == idEvent)
            timerId1[i].counter++;
    }
}

/* TIMERPROC for the testW1,2() with messages with window */
static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        timerIdW1[wParam].counter++;
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// TEST WITH MESSAGES WITHOUT WINDOW - test count of sent messages
static BOOL test1(void)
{
    UINT i, countErrors = 0;
    ULONGLONG startTime;
    MSG msg = { NULL };

    ZeroMemory(timerId1, sizeof(timerId1));

    for (i = 0; i < TEST1_COUNT; i++)
    {
        timerId1[i].index = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
        if (timerId1[i].index == 0)
            countErrors++;
    }

    startTime = GetTickCount();

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (GetTickCount() - startTime >= SLEEP_TIME)
            PostQuitMessage(0);
    }

    for (i = 0; i < TEST1_COUNT; i++)
    {
        if ((timerId1[i].counter < MIN_MESSAGES_TIME) || (timerId1[i].counter > MAX_MESSAGES_TIME))
            countErrors++;
    }

    for (i = 0; i < TEST1_COUNT; i++)
    {
        if (KillTimer(NULL, timerId1[i].index) == 0)
            countErrors++;
    }

    return (countErrors == 0);
}

// TEST WITH MESSAGES WITHOUT WINDOW - create many timers
static BOOL test2(void)
{
    UINT i, countErrors = 0;
    UINT_PTR locIndex;

    for (i = 0; i < TEST2_COUNT; i++)
    {
        locIndex = SetTimer(NULL, 0, TEST2_INTERVAL, TimerProc);

        if (locIndex == 0)
            countErrors++;
        if (KillTimer(NULL, locIndex) == 0)
            countErrors++;
    }

    return (countErrors == 0);
}

// TEST WITH MESSAGES WITHOUT WINDOW - test different ids
static BOOL test3(void)
{
    UINT countErrors = 0;
    UINT_PTR locIndex1;
    UINT_PTR locIndex2;

    locIndex1 = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
    if (locIndex1 == 0)
        countErrors++;
    if (KillTimer(NULL, locIndex1) == 0)
        countErrors++;
    locIndex2 = SetTimer(NULL, 0, TEST1_INTERVAL, TimerProc);
    if (locIndex2 == 0)
        countErrors++;
    if (KillTimer(NULL, locIndex2) == 0)
        countErrors++;
    if (locIndex1 == locIndex2)
        countErrors++;

    return (countErrors == 0);
}

// TEST WITH MESSAGES WITH WINDOW - test count of sent messages
static BOOL testW1(HWND hwnd)
{
    UINT i, countErrors = 0;
    UINT_PTR locIndex;
    ULONGLONG startTime;
    MSG msg = { NULL };

    if (hwnd == NULL)
        return FALSE;

    ZeroMemory(timerIdW1, sizeof(timerIdW1));

    for (i = 0; i < TESTW1_COUNT; i++)
    {
        locIndex = SetTimer(hwnd, i, TESTW1_INTERVAL, NULL);
        if (locIndex == 0)
            countErrors++;
    }

    startTime = GetTickCount();

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (GetTickCount() - startTime >= SLEEP_TIME)
            PostQuitMessage(0);
    }

    for (i = 0; i < TESTW1_COUNT; i++)
    {
        if ((timerIdW1[i].counter < MIN_MESSAGES_TIME) || (timerIdW1[i].counter > MAX_MESSAGES_TIME))
            countErrors++;
    }

    for (i = 0; i < TESTW1_COUNT; i++)
    {
        if (KillTimer(hwnd, i) == 0)
            countErrors++;
    }

    return (countErrors == 0);
}

// TEST WITH MESSAGES WITH WINDOW - create many timers
static BOOL testW2(HWND hwnd)
{
    UINT i, countErrors = 0;
    UINT_PTR result;

    if (hwnd == NULL)
        return FALSE;

    for (i = 0; i < TESTW2_COUNT; i++)
    {
        result = SetTimer(hwnd, 1, TESTW2_INTERVAL, NULL);
        if (result == 0)
            countErrors++;
        if (KillTimer(hwnd, 1) == 0)
            countErrors++;
    }

    return (countErrors == 0);
}

START_TEST(NtUserSetTimer)
{
    WNDCLASSW wc = { 0 };
    HWND hwnd;

    // TEST WITH MESSAGES WITHOUT WINDOW - test count of sent messages
    TEST(test1());

    // TEST WITH MESSAGES WITHOUT WINDOW - create many timers
    TEST(test2());

    // TEST WITH MESSAGES WITHOUT WINDOW - test different ids
    TEST(test3());

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"TimerWindowClass";
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(0, L"TimerWindowClass", L"Timer Window", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_MESSAGE, NULL, GetModuleHandleW(NULL), NULL);

    // TEST WITH MESSAGES WITH WINDOW - test count of sent messages
    TEST(testW1(hwnd));

    // TEST WITH MESSAGES WITH WINDOW - create many timers
    TEST(testW2(hwnd));

    if (hwnd != NULL)
        DestroyWindow(hwnd);
    UnregisterClassW(L"TimerWindowClass", NULL);
}
