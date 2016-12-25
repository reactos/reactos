/*
 * Test suite for Task interface
 *
 * Copyright (C) 2008 Google (Roy Shea)
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
static ITask *test_task;
static ITaskTrigger *test_trigger;
static WORD trigger_index;

static BOOL setup_trigger(void)
{
    HRESULT hres;
    const WCHAR task_name[] = {'T','e','s','t','i','n','g', 0};

    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    if(hres != S_OK)
        return FALSE;
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &CLSID_CTask, &IID_ITask, (IUnknown**)&test_task);
    if(hres != S_OK)
    {
        ITaskScheduler_Release(test_task_scheduler);
        return FALSE;
    }
    hres = ITask_CreateTrigger(test_task, &trigger_index, &test_trigger);
    if(hres != S_OK)
    {
        ITask_Release(test_task);
        ITaskScheduler_Release(test_task_scheduler);
        return FALSE;
    }
    return TRUE;
}

static void cleanup_trigger(void)
{
    ITaskTrigger_Release(test_trigger);
    ITask_Release(test_task);
    ITaskScheduler_Release(test_task_scheduler);
}

static BOOL compare_trigger_state(TASK_TRIGGER found_state,
        TASK_TRIGGER expected_state)
{
    ok(found_state.cbTriggerSize == expected_state.cbTriggerSize,
            "cbTriggerSize: Found %d but expected %d\n",
            found_state.cbTriggerSize, expected_state.cbTriggerSize);

    ok(found_state.Reserved1 == expected_state.Reserved1,
            "Reserved1: Found %d but expected %d\n",
            found_state.Reserved1, expected_state.Reserved1);

    ok(found_state.wBeginYear == expected_state.wBeginYear,
            "wBeginYear: Found %d but expected %d\n",
            found_state.wBeginYear, expected_state.wBeginYear);

    ok(found_state.wBeginMonth == expected_state.wBeginMonth,
            "wBeginMonth: Found %d but expected %d\n",
            found_state.wBeginMonth, expected_state.wBeginMonth);

    ok(found_state.wBeginDay == expected_state.wBeginDay,
            "wBeginDay: Found %d but expected %d\n",
            found_state.wBeginDay, expected_state.wBeginDay);

    ok(found_state.wEndYear == expected_state.wEndYear,
            "wEndYear: Found %d but expected %d\n",
            found_state.wEndYear, expected_state.wEndYear);

    ok(found_state.wEndMonth == expected_state.wEndMonth,
            "wEndMonth: Found %d but expected %d\n",
            found_state.wEndMonth, expected_state.wEndMonth);

    ok(found_state.wEndDay == expected_state.wEndDay,
            "wEndDay: Found %d but expected %d\n",
            found_state.wEndDay, expected_state.wEndDay);

    ok(found_state.wStartHour == expected_state.wStartHour,
            "wStartHour: Found %d but expected %d\n",
            found_state.wStartHour, expected_state.wStartHour);

    ok(found_state.wStartMinute == expected_state.wStartMinute,
            "wStartMinute: Found %d but expected %d\n",
            found_state.wStartMinute, expected_state.wStartMinute);

    ok(found_state.MinutesDuration == expected_state.MinutesDuration,
            "MinutesDuration: Found %d but expected %d\n",
            found_state.MinutesDuration, expected_state.MinutesDuration);

    ok(found_state.MinutesInterval == expected_state.MinutesInterval,
            "MinutesInterval: Found %d but expected %d\n",
            found_state.MinutesInterval, expected_state.MinutesInterval);

    ok(found_state.rgFlags == expected_state.rgFlags,
            "rgFlags: Found %d but expected %d\n",
            found_state.rgFlags, expected_state.rgFlags);

    ok(found_state.TriggerType == expected_state.TriggerType,
            "TriggerType: Found %d but expected %d\n",
            found_state.TriggerType, expected_state.TriggerType);

    ok(found_state.Type.Daily.DaysInterval == expected_state.Type.Daily.DaysInterval,
            "Type.Daily.DaysInterval: Found %d but expected %d\n",
            found_state.Type.Daily.DaysInterval, expected_state.Type.Daily.DaysInterval);

    ok(found_state.Reserved2 == expected_state.Reserved2,
            "Reserved2: Found %d but expected %d\n",
            found_state.Reserved2, expected_state.Reserved2);

    ok(found_state.wRandomMinutesInterval == expected_state.wRandomMinutesInterval,
            "wRandomMinutesInterval: Found %d but expected %d\n",
            found_state.wRandomMinutesInterval, expected_state.wRandomMinutesInterval);

    return TRUE;
}

static void test_SetTrigger_GetTrigger(void)
{
    BOOL setup;
    HRESULT hres;
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

    setup = setup_trigger();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

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
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    compare_trigger_state(trigger_state, empty_trigger_state);

    /* Test setting basic empty trigger */
    hres = ITaskTrigger_SetTrigger(test_trigger, &empty_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    compare_trigger_state(trigger_state, empty_trigger_state);

    /* Test setting basic non-empty trigger */
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
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
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.cbTriggerSize = sizeof(trigger_state) + 1;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.cbTriggerSize = sizeof(trigger_state);

    /* Test setting trigger with invalid Reserved fields */
    normal_trigger_state.Reserved1 = 80;
    normal_trigger_state.Reserved2 = 80;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08x\n", hres);
    ok(trigger_state.Reserved1 == 0 && trigger_state.Reserved2 == 0,
            "Reserved fields should be set to zero\n");
    normal_trigger_state.Reserved1 = 0;
    normal_trigger_state.Reserved2 = 0;

    /* Test setting trigger with invalid month */
    normal_trigger_state.wBeginMonth = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wBeginMonth = 13;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wBeginMonth = 1;

    /* Test setting trigger with invalid begin date */
    normal_trigger_state.wBeginDay = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wBeginDay = 32;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wBeginMonth = 2;
    normal_trigger_state.wBeginDay = 30;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wBeginMonth = 1;
    normal_trigger_state.wBeginDay = 1;

    /* Test setting trigger invalid end date */
    normal_trigger_state.wEndYear = 0;
    normal_trigger_state.wEndMonth = 200;
    normal_trigger_state.wEndDay = 200;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08x\n", hres);
    ok(trigger_state.wEndYear == 0, "End year should be 0: %d\n",
            trigger_state.wEndYear);
    ok(trigger_state.wEndMonth == 200, "End month should be 200: %d\n",
            trigger_state.wEndMonth);
    ok(trigger_state.wEndDay == 200, "End day should be 200: %d\n",
            trigger_state.wEndDay);
    normal_trigger_state.rgFlags =
            TASK_TRIGGER_FLAG_DISABLED | TASK_TRIGGER_FLAG_HAS_END_DATE;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    normal_trigger_state.wEndYear = 2980;
    normal_trigger_state.wEndMonth = 1;
    normal_trigger_state.wEndDay = 1;

    /* Test setting trigger with invalid hour or minute*/
    normal_trigger_state.wStartHour = 24;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wStartHour = 60;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.wStartHour = 3;

    /* Test setting trigger with invalid duration / interval pairs */
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 5;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 6;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.MinutesDuration = 0;
    normal_trigger_state.MinutesInterval = 6;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == E_INVALIDARG, "Expected E_INVALIDARG: 0x%08x\n", hres);
    normal_trigger_state.MinutesDuration = 5;
    normal_trigger_state.MinutesInterval = 0;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    normal_trigger_state.MinutesDuration = 0;
    normal_trigger_state.MinutesInterval = 0;

    /* Test setting trigger with end date before start date */
    normal_trigger_state.wEndYear = 1979;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    normal_trigger_state.rgFlags =
            TASK_TRIGGER_FLAG_DISABLED | TASK_TRIGGER_FLAG_HAS_END_DATE;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    normal_trigger_state.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    normal_trigger_state.wEndYear = 2980;
    normal_trigger_state.wEndMonth = 1;
    normal_trigger_state.wEndDay = 1;


    /* Test setting trigger with invalid TriggerType and Type */
    normal_trigger_state.TriggerType = TASK_TIME_TRIGGER_ONCE;
    normal_trigger_state.Type.Weekly.WeeksInterval = 2;
    normal_trigger_state.Type.Weekly.rgfDaysOfTheWeek = (TASK_MONDAY | TASK_TUESDAY);
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08x\n", hres);
    ok(trigger_state.Type.Weekly.WeeksInterval == 0xcfcf,
            "Expected WeeksInterval set remain untouched: %d\n",
            trigger_state.Type.Weekly.WeeksInterval);
    ok(trigger_state.Type.Weekly.rgfDaysOfTheWeek == 0xcfcf,
            "Expected WeeksInterval set remain untouched: %d\n",
            trigger_state.Type.Weekly.rgfDaysOfTheWeek);
    normal_trigger_state.TriggerType = TASK_TIME_TRIGGER_DAILY;
    normal_trigger_state.Type.Daily.DaysInterval = 1;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08x\n", hres);

    /* Test setting trigger with set wRandomMinutesInterval */
    normal_trigger_state.wRandomMinutesInterval = 5;
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state);
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Expected S_OK: 0x%08x\n", hres);
    ok(trigger_state.wRandomMinutesInterval == 0,
            "wRandomMinutesInterval should be set to zero\n");
    normal_trigger_state.wRandomMinutesInterval = 0;

    /* Test GetTrigger using invalid cbTriggerSiz in pTrigger.  In
     * contrast to available documentation, this succeeds in practice. */
    hres = ITaskTrigger_SetTrigger(test_trigger, &normal_trigger_state);
    ok(hres == S_OK, "Failed to set trigger: 0x%08x\n", hres);
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = sizeof(trigger_state) - 1;
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    ok(compare_trigger_state(trigger_state, normal_trigger_state),
            "Invalid state\n");
    memset(&trigger_state, 0xcf, sizeof(trigger_state));
    trigger_state.cbTriggerSize = 0;
    hres = ITaskTrigger_GetTrigger(test_trigger, &trigger_state);
    ok(hres == S_OK, "Failed to GetTrigger\n");
    ok(compare_trigger_state(trigger_state, normal_trigger_state),
            "Invalid state\n");


    cleanup_trigger();
    return;
}


START_TEST(task_trigger)
{
    CoInitialize(NULL);
    test_SetTrigger_GetTrigger();
    CoUninitialize();
}
