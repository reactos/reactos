/*
 * Test suite for Task interface
 *
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <corerror.h>

#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *test_task_scheduler;

static DWORD obj_refcount(void *obj_to_check)
{
    IUnknown *obj = obj_to_check;
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

#define compare_trigger_state(found,expected) compare_trigger_state_(__LINE__,found,expected)
static void compare_trigger_state_(int line, TASK_TRIGGER found_state,
                                   TASK_TRIGGER expected_state)
{
    ok_(__FILE__, line)(found_state.cbTriggerSize == expected_state.cbTriggerSize,
            "cbTriggerSize: Found %d but expected %d\n",
            found_state.cbTriggerSize, expected_state.cbTriggerSize);

    ok_(__FILE__, line)(found_state.Reserved1 == expected_state.Reserved1,
            "Reserved1: Found %d but expected %d\n",
            found_state.Reserved1, expected_state.Reserved1);

    ok_(__FILE__, line)(found_state.wBeginYear == expected_state.wBeginYear,
            "wBeginYear: Found %d but expected %d\n",
            found_state.wBeginYear, expected_state.wBeginYear);

    ok_(__FILE__, line)(found_state.wBeginMonth == expected_state.wBeginMonth,
            "wBeginMonth: Found %d but expected %d\n",
            found_state.wBeginMonth, expected_state.wBeginMonth);

    ok_(__FILE__, line)(found_state.wBeginDay == expected_state.wBeginDay,
            "wBeginDay: Found %d but expected %d\n",
            found_state.wBeginDay, expected_state.wBeginDay);

    ok_(__FILE__, line)(found_state.wEndYear == expected_state.wEndYear,
            "wEndYear: Found %d but expected %d\n",
            found_state.wEndYear, expected_state.wEndYear);

    ok_(__FILE__, line)(found_state.wEndMonth == expected_state.wEndMonth,
            "wEndMonth: Found %d but expected %d\n",
            found_state.wEndMonth, expected_state.wEndMonth);

    ok_(__FILE__, line)(found_state.wEndDay == expected_state.wEndDay,
            "wEndDay: Found %d but expected %d\n",
            found_state.wEndDay, expected_state.wEndDay);

    ok_(__FILE__, line)(found_state.wStartHour == expected_state.wStartHour,
            "wStartHour: Found %d but expected %d\n",
            found_state.wStartHour, expected_state.wStartHour);

    ok_(__FILE__, line)(found_state.wStartMinute == expected_state.wStartMinute,
            "wStartMinute: Found %d but expected %d\n",
            found_state.wStartMinute, expected_state.wStartMinute);

    ok_(__FILE__, line)(found_state.MinutesDuration == expected_state.MinutesDuration,
            "MinutesDuration: Found %ld but expected %ld\n",
            found_state.MinutesDuration, expected_state.MinutesDuration);

    ok_(__FILE__, line)(found_state.MinutesInterval == expected_state.MinutesInterval,
            "MinutesInterval: Found %ld but expected %ld\n",
            found_state.MinutesInterval, expected_state.MinutesInterval);

    ok_(__FILE__, line)(found_state.rgFlags == expected_state.rgFlags,
            "rgFlags: Found %ld but expected %ld\n",
            found_state.rgFlags, expected_state.rgFlags);

    ok_(__FILE__, line)(found_state.TriggerType == expected_state.TriggerType,
            "TriggerType: Found %d but expected %d\n",
            found_state.TriggerType, expected_state.TriggerType);

    ok_(__FILE__, line)(found_state.Type.Daily.DaysInterval == expected_state.Type.Daily.DaysInterval,
            "Type.Daily.DaysInterval: Found %d but expected %d\n",
            found_state.Type.Daily.DaysInterval, expected_state.Type.Daily.DaysInterval);

    ok_(__FILE__, line)(found_state.Reserved2 == expected_state.Reserved2,
            "Reserved2: Found %d but expected %d\n",
            found_state.Reserved2, expected_state.Reserved2);

    ok_(__FILE__, line)(found_state.wRandomMinutesInterval == expected_state.wRandomMinutesInterval,
            "wRandomMinutesInterval: Found %d but expected %d\n",
            found_state.wRandomMinutesInterval, expected_state.wRandomMinutesInterval);
}

static void test_SetTrigger_GetTrigger(void)
{
    static const WCHAR task_name[] = { 'T','e','s','t','i','n','g',0 };
    ITask *test_task;
    ITaskTrigger *test_trigger;
    HRESULT hres;
    WORD idx;
    TASK_TRIGGER trigger_state;
    TASK_TRIGGER empty_trigger_state = {
        sizeof(trigger_state), 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0, 0,
        TASK_TRIGGER_FLAG_DISABLED, TASK_TIME_TRIGGER_DAILY, {{1}},
        0, 0
    };
    TASK_TRIGGER normal_trigger_state = {
        sizeof(trigger_state), 0,
        1980, 1, 1,
        2980, 2, 2,
        3, 3,
        0, 0,
        TASK_TRIGGER_FLAG_DISABLED, TASK_TIME_TRIGGER_DAILY, {{1}},
        0, 0
    };
    SYSTEMTIME time;

    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "got %#lx\n", hres);

    hres = ITask_CreateTrigger(test_task, &idx, &test_trigger);
    ok(hres == S_OK, "got %#lx\n", hres);

    hres = ITaskTrigger_SetTrigger(test_trigger, NULL);
    ok(hres == E_INVALIDARG, "got %#lx\n", hres);

    hres = ITaskTrigger_GetTrigger(test_trigger, NULL);
    ok(hres == E_INVALIDARG, "got %#lx\n", hres);

    /* Setup a trigger with base values for this test run */
    GetLocalTime(&time);
    empty_trigger_state.wStartHour = time.wHour;
    empty_trigger_state.wStartMinute = time.wMinute;
    empty_trigger_state.wBeginYear = time.wYear;
    empty_trigger_state.wBeginMonth = time.wMonth;
    empty_trigger_state.wBeginDay = time.wDay;

    /* Test trigger state after trigger creation but before setting * state */
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    compare_trigger_state(trigger_state, empty_trigger_state);

    /* Test setting basic empty trigger */
    hres = ITaskTrigger_SetTrigger(test_trigger, &empty_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    compare_trigger_state(trigger_state, empty_trigger_state);

    /* Test setting basic non-empty trigger */
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    compare_trigger_state(trigger_state, normal_trigger_state);

    /* The following tests modify the normal_trigger_state structure
     * before each test, and return the normal_trigger_state structure
     * back to its original valid state after each test.  This keeps
     * each test run independent. */

    /* Test setting trigger with invalid cbTriggerSize */
    normal_trigger_state.cbTriggerSize = sizeof(trigger_state) - 1;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.cbTriggerSize = sizeof(trigger_state) + 1;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.cbTriggerSize = sizeof(trigger_state);

    /* Test setting trigger with invalid Reserved fields */
    normal_trigger_state.Reserved1 = 80;
    normal_trigger_state.Reserved2 = 80;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08lx\n", hres);
    ok(trigger_state.Reserved1 == 0 && trigger_state.Reserved2 == 0,
            "Reserved fields should be set to zero\n");
    normal_trigger_state.Reserved1 = 0;
    normal_trigger_state.Reserved2 = 0;

    /* Test setting trigger with invalid month */
    normal_trigger_state.wBeginMonth = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wBeginMonth = 13;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wBeginMonth = 1;

    /* Test setting trigger with invalid begin date */
    normal_trigger_state.wBeginDay = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wBeginDay = 32;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wBeginMonth = 2;
    normal_trigger_state.wBeginDay = 30;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wBeginMonth = 1;
    normal_trigger_state.wBeginDay = 1;

    /* Test setting trigger invalid end date */
    normal_trigger_state.wEndYear = 0;
    normal_trigger_state.wEndMonth = 200;
    normal_trigger_state.wEndDay = 200;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08lx\n", hres);
    ok(trigger_state.wEndYear == 0, "End year should be 0: %d\n",
            trigger_state.wEndYear);
    ok(trigger_state.wEndMonth == 200, "End month should be 200: %d\n",
            trigger_state.wEndMonth);
    ok(trigger_state.wEndDay == 200, "End day should be 200: %d\n",
            trigger_state.wEndDay);
    normal_trigger_state.rgFlags =
            TASK_TRIGGER_FLAG_DISABLED | TASK_TRIGGER_FLAG_HAS_END_DATE;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    normal_trigger_state.wEndYear = 2980;
    normal_trigger_state.wEndMonth = 1;
    normal_trigger_state.wEndDay = 1;

    /* Test setting trigger with invalid hour or minute*/
    normal_trigger_state.wStartHour = 24;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wStartHour = 60;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.wStartHour = 3;

    /* Test setting trigger with invalid duration / interval pairs */
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 5;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 6;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.MinutesDuration = 0;
    normal_trigger_state.MinutesInterval = 6;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08lx\n", hres);
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    normal_trigger_state.MinutesDuration = 0;
    normal_trigger_state.MinutesInterval = 0;

    /* Test setting trigger with end date before start date */
    normal_trigger_state.wEndYear = 1979;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    normal_trigger_state.rgFlags =
            TASK_TRIGGER_FLAG_DISABLED | TASK_TRIGGER_FLAG_HAS_END_DATE;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    normal_trigger_state.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    normal_trigger_state.wEndYear = 2980;
    normal_trigger_state.wEndMonth = 1;
    normal_trigger_state.wEndDay = 1;


    /* Test setting trigger with invalid TriggerType and Type */
    normal_trigger_state.TriggerType = TASK_TIME_TRIGGER_ONCE;
    normal_trigger_state.Type.Weekly.WeeksInterval = 2;
    normal_trigger_state.Type.Weekly.rgfDaysOfTheWeek = (TASK_MONDAY | TASK_TUESDAY);
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08lx\n", hres);
    ok(trigger_state.Type.Weekly.WeeksInterval == 0xcfcf,
            "Expected WeeksInterval set remain untouched: %d\n",
            trigger_state.Type.Weekly.WeeksInterval);
    ok(trigger_state.Type.Weekly.rgfDaysOfTheWeek == 0xcfcf,
            "Expected WeeksInterval set remain untouched: %d\n",
            trigger_state.Type.Weekly.rgfDaysOfTheWeek);
    normal_trigger_state.TriggerType = TASK_TIME_TRIGGER_DAILY;
    normal_trigger_state.Type.Daily.DaysInterval = 1;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08lx\n", hres);

    /* Test setting trigger with set wRandomMinutesInterval */
    normal_trigger_state.wRandomMinutesInterval = 5;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08lx\n", hres);
    ok(trigger_state.wRandomMinutesInterval == 0,
            "wRandomMinutesInterval should be set to zero\n");
    normal_trigger_state.wRandomMinutesInterval = 0;

    /* Test GetTrigger using invalid cbTriggerSiz in pTrigger.  In
     * contrast to available documentation, this succeeds in practice. */
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08lx\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state) - 1;
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    compare_trigger_state(trigger_state, normal_trigger_state);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = 0;
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    compare_trigger_state(trigger_state, normal_trigger_state);

    ITaskTrigger_Release(test_trigger);
    ITask_Release(test_task);
}

static void test_task_trigger(void)
{
    static const WCHAR task_name[] = { 'T','e','s','t','i','n','g',0 };
    HRESULT hr;
    ITask *task;
    ITaskTrigger *trigger, *trigger2;
    WORD count, idx;
    DWORD ref;

    hr = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %u\n", count);

    hr = ITask_DeleteTrigger(task, 0);
    ok(hr == SCHED_E_TRIGGER_NOT_FOUND, "got %#lx\n", hr);

    hr = ITask_GetTrigger(task, 0, &trigger);
    ok(hr == SCHED_E_TRIGGER_NOT_FOUND, "got %#lx\n", hr);

    idx = 0xdead;
    hr = ITask_CreateTrigger(task, &idx, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(idx == 0, "got %u\n", idx);

    hr = ITask_GetTrigger(task, 0, &trigger2);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(trigger != trigger2, "%p != %p\n", trigger, trigger2);

    ref = ITaskTrigger_Release(trigger2);
    ok(ref == 0, "got %lu\n", ref);

    ref = ITaskTrigger_Release(trigger);
    ok(ref == 0, "got %lu\n", ref);

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 1, "got %u\n", count);

    hr = ITask_DeleteTrigger(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    idx = 0xdead;
    hr = ITask_CreateTrigger(task, &idx, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(idx == 0, "got %u\n", idx);

    hr = ITask_DeleteTrigger(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %u\n", count);

    ref = ITaskTrigger_Release(trigger);
    ok(ref == 0, "got %lu\n", ref);

    ref = ITask_Release(task);
    ok(ref == 0, "got %lu\n", ref);
}

static void time_add_ms(SYSTEMTIME *st, DWORD ms)
{
    union
    {
        FILETIME ft;
        ULONGLONG ll;
    } ftll;
    BOOL ret;

    trace("old: %u/%u/%u wday %u %u:%02u:%02u.%03u\n",
          st->wDay, st->wMonth, st->wYear, st->wDayOfWeek,
          st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
    ret = SystemTimeToFileTime(st, &ftll.ft);
    ok(ret, "SystemTimeToFileTime error %lu\n", GetLastError());

    ftll.ll += ms * (ULONGLONG)10000;
    ret = FileTimeToSystemTime(&ftll.ft, st);
    ok(ret, "FileTimeToSystemTime error %lu\n", GetLastError());
    trace("new: %u/%u/%u wday %u %u:%02u:%02u.%03u\n",
          st->wDay, st->wMonth, st->wYear, st->wDayOfWeek,
          st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
}

static void trigger_add_ms(TASK_TRIGGER *data, DWORD ms, SYSTEMTIME *ret)
{
    SYSTEMTIME st;

    st.wYear = data->wBeginYear;
    st.wMonth = data->wBeginMonth;
    st.wDayOfWeek = 0;
    st.wDay = data->wBeginDay;
    st.wHour = data->wStartHour;
    st.wMinute = data->wStartMinute;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    time_add_ms(&st, ms);

    data->wBeginYear = st.wYear;
    data->wBeginMonth = st.wMonth;
    data->wBeginDay = st.wDay;
    data->wStartHour = st.wHour;
    data->wStartMinute = st.wMinute;

    *ret = st;
}

static void test_GetNextRunTime(void)
{
    static const WCHAR task_name[] = { 'T','e','s','t','i','n','g',0 };
    static const SYSTEMTIME st_empty;
    HRESULT hr;
    ITask *task;
    ITaskTrigger *trigger;
    TASK_TRIGGER data;
    WORD idx, i;
    SYSTEMTIME st, cmp;

    hr = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);

    if (0) /* crashes under Windows */
        hr = ITask_GetNextRunTime(task, NULL);

    hr = ITask_SetFlags(task, TASK_FLAG_DISABLED);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == SCHED_S_TASK_DISABLED, "got %#lx\n", hr);
    ok(!memcmp(&st, &st_empty, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    hr = ITask_SetFlags(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == SCHED_S_TASK_NO_VALID_TRIGGERS, "got %#lx\n", hr);
    ok(!memcmp(&st, &st_empty, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    hr = ITask_CreateTrigger(task, &idx, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == SCHED_S_TASK_NO_VALID_TRIGGERS, "got %#lx\n", hr);
    ok(!memcmp(&st, &st_empty, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    /* TASK_TIME_TRIGGER_ONCE */

    hr = ITaskTrigger_GetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
    data.TriggerType = TASK_TIME_TRIGGER_ONCE;
    /* add 5 minutes to avoid races */
    trigger_add_ms(&data, 5 * 60 * 1000, &cmp);
    hr = ITaskTrigger_SetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!memcmp(&st, &cmp, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    hr = ITaskTrigger_GetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
    data.TriggerType = TASK_TIME_TRIGGER_ONCE;
    /* add 1 day to test triggers in the far future */
    trigger_add_ms(&data, 24 * 60 * 60 * 1000, &cmp);
    hr = ITaskTrigger_SetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!memcmp(&st, &cmp, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    /* TASK_TIME_TRIGGER_DAILY */

    hr = ITaskTrigger_GetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
    data.TriggerType = TASK_TIME_TRIGGER_DAILY;
    data.Type.Daily.DaysInterval = 1;
    hr = ITaskTrigger_SetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!memcmp(&st, &cmp, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    /* TASK_TIME_TRIGGER_WEEKLY */

    hr = ITaskTrigger_GetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
    data.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
    data.Type.Weekly.WeeksInterval = 1;
    /* add 3 days */
    time_add_ms(&cmp, 3 * 24 * 60 * 60 * 1000);
    /* bits: TASK_SUNDAY = 1, TASK_MONDAY = 2, TASK_TUESDAY = 4, etc. */
    data.Type.Weekly.rgfDaysOfTheWeek = 1 << cmp.wDayOfWeek; /* wDayOfWeek is 0 based */
    hr = ITaskTrigger_SetTrigger(trigger, &data);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetNextRunTime(task, &st);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!memcmp(&st, &cmp, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
       st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
       st.wHour, st.wMinute, st.wSecond);

    /* FIXME: TASK_TIME_TRIGGER_MONTHLYDATE */
    /* FIXME: TASK_TIME_TRIGGER_MONTHLYDOW */

    ITaskTrigger_Release(trigger);
    /* do not delete a valid trigger */

    idx = 0xdead;
    hr = ITask_CreateTrigger(task, &idx, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(idx == 1, "got %u\n", idx);

    /* TASK_EVENT_TRIGGER_ON_IDLE = 5
     * TASK_EVENT_TRIGGER_AT_SYSTEMSTART = 6
     * TASK_EVENT_TRIGGER_AT_LOGON = 7
     */
    for (i = 5; i <= 7; i++)
    {
        hr = ITaskTrigger_GetTrigger(trigger, &data);
        ok(hr == S_OK, "got %#lx\n", hr);
        data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
        data.TriggerType = i;
        hr = ITaskTrigger_SetTrigger(trigger, &data);
        ok(hr == S_OK, "got %#lx\n", hr);

        memset(&st, 0xff, sizeof(st));
        hr = ITask_GetNextRunTime(task, &st);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(!memcmp(&st, &cmp, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
           st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
           st.wHour, st.wMinute, st.wSecond);
    }

    ITaskTrigger_Release(trigger);

    hr = ITask_DeleteTrigger(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_GetTrigger(task, 0, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);

    for (i = 5; i <= 7; i++)
    {
        hr = ITaskTrigger_GetTrigger(trigger, &data);
        ok(hr == S_OK, "got %#lx\n", hr);
        data.rgFlags &= ~TASK_TRIGGER_FLAG_DISABLED;
        data.TriggerType = i;
        hr = ITaskTrigger_SetTrigger(trigger, &data);
        ok(hr == S_OK, "got %#lx\n", hr);

        memset(&st, 0xff, sizeof(st));
        hr = ITask_GetNextRunTime(task, &st);
        ok(hr == SCHED_S_EVENT_TRIGGER, "got %#lx\n", hr);
        ok(!memcmp(&st, &st_empty, sizeof(st)), "got %u/%u/%u wday %u %u:%02u:%02u\n",
           st.wDay, st.wMonth, st.wYear, st.wDayOfWeek,
           st.wHour, st.wMinute, st.wSecond);
    }

    ITaskTrigger_Release(trigger);
    ITask_Release(task);
}

static HRESULT get_task_trigger(ITask *task, WORD idx, TASK_TRIGGER *state)
{
    HRESULT hr;
    ITaskTrigger *trigger;

    hr = ITask_GetTrigger(task, idx, &trigger);
    if (hr != S_OK) return hr;

    memset(state, 0x11, sizeof(*state));
    hr = ITaskTrigger_GetTrigger(trigger, state);

    ITaskTrigger_Release(trigger);
    return hr;
}

static void test_trigger_manager(void)
{
    static const WCHAR task_name[] = { 'T','e','s','t','i','n','g',0 };
    HRESULT hr;
    ITask *task;
    ITaskTrigger *trigger0, *trigger1;
    TASK_TRIGGER state0, state1, state;
    WORD count, idx;
    DWORD ref;

    hr = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(obj_refcount(task) == 1, "got %lu\n", obj_refcount(task));

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %u\n", count);

    idx = 0xdead;
    hr = ITask_CreateTrigger(task, &idx, &trigger0);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(idx == 0, "got %u\n", idx);
    ok(obj_refcount(task) == 2, "got %lu\n", obj_refcount(task));

    idx = 0xdead;
    hr = ITask_CreateTrigger(task, &idx, &trigger1);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(idx == 1, "got %u\n", idx);
    ok(obj_refcount(task) == 3, "got %lu\n", obj_refcount(task));

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 2, "got %u\n", count);

    hr = ITaskTrigger_GetTrigger(trigger0, &state0);
    ok(hr == S_OK, "got %#lx\n", hr);
    state0.wBeginYear = 3000;
    state0.rgFlags = 0;
    state0.TriggerType = TASK_TIME_TRIGGER_ONCE;
    hr = ITaskTrigger_SetTrigger(trigger0, &state0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = get_task_trigger(task, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state.wBeginYear == 3000, "got %u\n", state.wBeginYear);
    ok(state.TriggerType == TASK_TIME_TRIGGER_ONCE, "got %u\n", state.TriggerType);

    hr = ITaskTrigger_GetTrigger(trigger1, &state1);
    ok(hr == S_OK, "got %#lx\n", hr);
    state1.wBeginYear = 2000;
    state1.rgFlags = 0;
    state1.TriggerType = TASK_TIME_TRIGGER_DAILY;
    hr = ITaskTrigger_SetTrigger(trigger1, &state1);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = get_task_trigger(task, 1, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state.wBeginYear == 2000, "got %u\n", state.wBeginYear);
    ok(state.TriggerType == TASK_TIME_TRIGGER_DAILY, "got %u\n", state.TriggerType);

    ref = ITaskTrigger_Release(trigger0);
    ok(ref == 0, "got %lu\n", ref);
    ref = ITaskTrigger_Release(trigger1);
    ok(ref == 0, "got %lu\n", ref);

    ok(obj_refcount(task) == 1, "got %lu\n", obj_refcount(task));

    hr = get_task_trigger(task, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state.wBeginYear == 3000, "got %u\n", state.wBeginYear);
    ok(state.TriggerType == TASK_TIME_TRIGGER_ONCE, "got %u\n", state.TriggerType);

    hr = get_task_trigger(task, 1, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state.wBeginYear == 2000, "got %u\n", state.wBeginYear);
    ok(state.TriggerType == TASK_TIME_TRIGGER_DAILY, "got %u\n", state.TriggerType);

    hr = ITask_GetTrigger(task, 0, &trigger0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = ITask_GetTrigger(task, 1, &trigger1);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_DeleteTrigger(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = get_task_trigger(task, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state.wBeginYear == 2000, "got %u\n", state.wBeginYear);
    ok(state.TriggerType == TASK_TIME_TRIGGER_DAILY, "got %u\n", state.TriggerType);

    hr = get_task_trigger(task, 1, &state);
    ok(hr == SCHED_E_TRIGGER_NOT_FOUND, "got %#lx\n", hr);

    hr = ITaskTrigger_SetTrigger(trigger0, &state0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITaskTrigger_SetTrigger(trigger1, &state1);
    ok(hr == E_FAIL, "got %#lx\n", hr);

    count = 0xdead;
    hr = ITask_GetTriggerCount(task, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 1, "got %u\n", count);

    ok(obj_refcount(task) == 3, "got %lu\n", obj_refcount(task));

    ref = ITaskTrigger_Release(trigger0);
    ok(ref == 0, "got %lu\n", ref);

    ref = ITaskTrigger_Release(trigger1);
    ok(ref == 0, "got %lu\n", ref);

    ref = ITask_Release(task);
    ok(ref == 0, "got %lu\n", ref);
}

START_TEST(task_trigger)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ITaskScheduler, (void **)&test_task_scheduler);
    ok(hr == S_OK, "error creating TaskScheduler instance %#lx\n", hr);

    test_SetTrigger_GetTrigger();
    test_task_trigger();
    test_GetNextRunTime();
    test_trigger_manager();

    ITaskScheduler_Release(test_task_scheduler);
    CoUninitialize();
}
