/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests for QueryUserAPC and threads
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#define MAX_RECORD 30

static DWORD s_record_count = 0;
static DWORD s_record[MAX_RECORD + 1] = { 0 };
static BOOL s_terminate_all = FALSE;

static const DWORD s_expected[] =
{
    0, 1, 7, 8, 4,
    2, 1, 9, 10, 5,
    2, 1, 11, 12, 13,
    6, 2, 3, 14, 15,
    16
};
static const SIZE_T s_expected_count = sizeof(s_expected) / sizeof(s_expected[0]);

static void AddValueToRecord(DWORD dwValue)
{
    s_record[s_record_count] = dwValue;
    if (s_record_count < MAX_RECORD)
        s_record_count++;
}

static VOID CheckRecord(void)
{
    SIZE_T i, count = min(s_record_count, s_expected_count);

    for (i = 0; i < count; ++i)
    {
        ok(s_record[i] == s_expected[i], "s_record[%u]: got %lu vs expected %lu\n",
           (INT)i, s_record[i], s_expected[i]);
    }

    count = abs((int)s_record_count - (int)s_expected_count);
    for (i = 0; i < count; ++i)
    {
        ok(s_record_count == s_expected_count, "s_record_count != s_expected_count\n");
    }
}

static DWORD WINAPI ThreadFunc(LPVOID arg)
{
    AddValueToRecord(0);
    while (!s_terminate_all)
    {
        AddValueToRecord(1);
        SleepEx(INFINITE, TRUE);
        AddValueToRecord(2);
    }
    AddValueToRecord(3);
    return 0;
}

static VOID NTAPI DoUserAPC1(ULONG_PTR Parameter)
{
    ok_int((int)Parameter, 1);
    AddValueToRecord(4);
}

static VOID NTAPI DoUserAPC2(ULONG_PTR Parameter)
{
    ok_int((int)Parameter, 2);
    AddValueToRecord(5);
}

static VOID NTAPI DoUserAPC3(ULONG_PTR Parameter)
{
    ok_int((int)Parameter, 3);
    AddValueToRecord(6);
    s_terminate_all = TRUE;
}

START_TEST(QueryUserAPC)
{
    DWORD dwThreadId;
    HANDLE hThread;

    s_record_count = 0;
    ZeroMemory(s_record, sizeof(s_record));

    hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &dwThreadId);
    ok(hThread != NULL, "hThread was NULL\n");

    Sleep(200);

    AddValueToRecord(7);
    ok_long(QueueUserAPC(DoUserAPC1, hThread, 1), 1);
    AddValueToRecord(8);

    Sleep(200);

    AddValueToRecord(9);
    ok_long(QueueUserAPC(DoUserAPC2, hThread, 2), 1);
    AddValueToRecord(10);

    Sleep(200);

    AddValueToRecord(11);
    ok_long(QueueUserAPC(DoUserAPC3, hThread, 3), 1);
    AddValueToRecord(12);

    AddValueToRecord(13);
    ok_long(WaitForSingleObject(hThread, 5 * 1000), WAIT_OBJECT_0);
    AddValueToRecord(14);

    AddValueToRecord(15);
    ok_int(CloseHandle(hThread), TRUE);
    AddValueToRecord(16);

    CheckRecord();
}
