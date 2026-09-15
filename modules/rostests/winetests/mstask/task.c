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

#include "corerror.h"
#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *scheduler;
static const WCHAR task_name[] = {'T','e','s','t','i','n','g',0};
static const WCHAR empty[] = {0};

extern HRESULT taskscheduler_delete(ITaskScheduler*, const WCHAR*);


static LPCWSTR path_resolve_name(LPCWSTR base_name)
{
    static WCHAR buffer[MAX_PATH];
    int len;

    len = SearchPathW(NULL, base_name, NULL, 0, NULL, NULL);
    if (len == 0)
        return base_name;
    else if (len < MAX_PATH)
    {
        SearchPathW(NULL, base_name, NULL, MAX_PATH, buffer, NULL);
        return buffer;
    }
    return NULL;
}

static void test_SetApplicationName_GetApplicationName(void)
{
    ITask *test_task;
    HRESULT hres;
    LPWSTR stored_name;
    LPCWSTR full_name;
    const WCHAR non_application_name[] = {'N','o','S','u','c','h',
            'A','p','p','l','i','c','a','t','i','o','n', 0};
    const WCHAR notepad_exe[] = {
            'n','o','t','e','p','a','d','.','e','x','e', 0};
    const WCHAR notepad[] = {'n','o','t','e','p','a','d', 0};

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    /* Attempt getting before setting application name */
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpiW(stored_name, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    /* Set application name to a nonexistent application and then get
     * the application name that is actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    ok(hres == S_OK, "Failed setting name %s: %08lx\n",
            wine_dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                wine_dbgstr_w(stored_name), wine_dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name with program type extension and then
     * get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad_exe);
    ok(hres == S_OK, "Failed setting name %s: %08lx\n",
            wine_dbgstr_w(notepad_exe), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad_exe);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                wine_dbgstr_w(stored_name), wine_dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name without program type extension and
     * then get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad);
    ok(hres == S_OK, "Failed setting name %s: %08lx\n", wine_dbgstr_w(notepad), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad_exe);  /* XP SP1 appends .exe */
        if (lstrcmpiW(stored_name, full_name) != 0)
        {
            full_name = path_resolve_name(notepad);
            ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
               wine_dbgstr_w(stored_name), wine_dbgstr_w(full_name));
        }
        CoTaskMemFree(stored_name);
    }

    /* After having a valid application name set, set application the name
     * to a nonexistent application and then get the name that is
     * actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    ok(hres == S_OK, "Failed setting name %s: %08lx\n",
            wine_dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                wine_dbgstr_w(stored_name), wine_dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Clear application name */
    hres = ITask_SetApplicationName(test_task, empty);
    ok(hres == S_OK, "Failed setting name %s: %08lx\n", wine_dbgstr_w(empty), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpiW(stored_name, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    ITask_Release(test_task);
}

static void test_CreateTrigger(void)
{
    ITask *test_task;
    HRESULT hres;
    WORD trigger_index;
    ITaskTrigger *test_trigger;

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    hres = ITask_CreateTrigger(test_task, &trigger_index, &test_trigger);
    ok(hres == S_OK, "Failed to create trigger: 0x%08lx\n", hres);

    ITaskTrigger_Release(test_trigger);
    ITask_Release(test_task);
}

static void test_SetParameters_GetParameters(void)
{
    ITask *test_task;
    HRESULT hres;
    LPWSTR parameters;
    const WCHAR parameters_a[] = {'f','o','o','.','t','x','t', 0};
    const WCHAR parameters_b[] = {'f','o','o','.','t','x','t',' ',
        'b','a','r','.','t','x','t', 0};

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    /* Get parameters before setting them */
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(parameters));
        CoTaskMemFree(parameters);
    }

    /* Set parameters to a simple string */
    hres = ITask_SetParameters(test_task, parameters_a);
    ok(hres == S_OK, "Failed setting parameters %s: %08lx\n",
            wine_dbgstr_w(parameters_a), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, parameters_a), "Got %s, expected %s\n",
                wine_dbgstr_w(parameters), wine_dbgstr_w(parameters_a));
        CoTaskMemFree(parameters);
    }

    /* Update parameters to a different simple string */
    hres = ITask_SetParameters(test_task, parameters_b);
    ok(hres == S_OK, "Failed setting parameters %s: %08lx\n",
            wine_dbgstr_w(parameters_b), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, parameters_b), "Got %s, expected %s\n",
                wine_dbgstr_w(parameters), wine_dbgstr_w(parameters_b));
        CoTaskMemFree(parameters);
    }

    /* Clear parameters */
    hres = ITask_SetParameters(test_task, empty);
    ok(hres == S_OK, "Failed setting parameters %s: %08lx\n",
            wine_dbgstr_w(empty), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(parameters));
        CoTaskMemFree(parameters);
    }

    ITask_Release(test_task);
}

static void test_SetComment_GetComment(void)
{
    ITask *test_task;
    HRESULT hres;
    LPWSTR comment;
    const WCHAR comment_a[] = {'C','o','m','m','e','n','t','.', 0};
    const WCHAR comment_b[] = {'L','o','n','g','e','r',' ',
            'c','o','m','m','e','n','t','.', 0};

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    /* Get comment before setting it*/
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(comment));
        CoTaskMemFree(comment);
    }

    /* Set comment to a simple string */
    hres = ITask_SetComment(test_task, comment_a);
    ok(hres == S_OK, "Failed setting comment %s: %08lx\n",
            wine_dbgstr_w(comment_a), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, comment_a), "Got %s, expected %s\n",
                wine_dbgstr_w(comment), wine_dbgstr_w(comment_a));
        CoTaskMemFree(comment);
    }

    /* Update comment to a different simple string */
    hres = ITask_SetComment(test_task, comment_b);
    ok(hres == S_OK, "Failed setting comment %s: %08lx\n",
            wine_dbgstr_w(comment_b), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, comment_b), "Got %s, expected %s\n",
                wine_dbgstr_w(comment), wine_dbgstr_w(comment_b));
        CoTaskMemFree(comment);
    }

    /* Clear comment */
    hres = ITask_SetComment(test_task, empty);
    ok(hres == S_OK, "Failed setting comment %s: %08lx\n",
            wine_dbgstr_w(empty), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(comment));
        CoTaskMemFree(comment);
    }

    ITask_Release(test_task);
}

static void test_SetMaxRunTime_GetMaxRunTime(void)
{
    ITask *test_task;
    HRESULT hres;
    DWORD max_run_time;

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    /* Default time is 3 days:
     * 3 days * 24 hours * 60 minutes * 60 seconds * 1000 ms = 259200000 */
    max_run_time = 0;
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08lx\n", hres);
    ok(max_run_time == 259200000, "Expected 259200000: %ld\n", max_run_time);

    /* Basic set test */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, 1234);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08lx\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08lx\n", hres);
    ok(max_run_time == 1234, "Expected 1234: %ld\n", max_run_time);

    /* Verify that time can be set to zero */
    max_run_time = 1;
    hres = ITask_SetMaxRunTime(test_task, 0);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08lx\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08lx\n", hres);
    ok(max_run_time == 0, "Expected 0: %ld\n", max_run_time);

    /* Check resolution by setting time to one */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, 1);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08lx\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08lx\n", hres);
    ok(max_run_time == 1, "Expected 1: %ld\n", max_run_time);

    /* Verify that time can be set to INFINITE */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, INFINITE);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08lx\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08lx\n", hres);
    ok(max_run_time == INFINITE, "Expected INFINITE: %ld\n", max_run_time);

    ITask_Release(test_task);
}

static void test_SetAccountInformation_GetAccountInformation(void)
{
    ITask *test_task;
    HRESULT hres;
    LPWSTR account_name;
    const WCHAR dummy_account_name[] = {'N', 'o', 'S', 'u', 'c', 'h',
            'A', 'c', 'c', 'o', 'u', 'n', 't', 0};
    const WCHAR dummy_account_name_b[] = {'N', 'o', 'S', 'u', 'c', 'h',
            'A', 'c', 'c', 'o', 'u', 'n', 't', 'B', 0};

    hres = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                      &IID_ITask, (IUnknown **)&test_task);
    ok(hres == S_OK, "Failed to setup test_task\n");

    /* Get account information before it is set */
    hres = ITask_GetAccountInformation(test_task, &account_name);
    /* WinXP returns HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): 0x80070002 but
     * Win2K returns SCHED_E_CANNOT_OPEN_TASK: 0x8004130d
     * Win9x doesn't support security services */
    if (hres == SCHED_E_NO_SECURITY_SERVICES || hres == SCHED_E_SERVICE_NOT_RUNNING)
    {
        win_skip("Security services are not supported\n");
        ITask_Release(test_task);
    }
    ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            hres == SCHED_E_CANNOT_OPEN_TASK,
            "Unset account name generated: 0x%08lx\n", hres);

    /* Attempt to set to a dummy account without a password */
    /* This test passes on WinXP but fails on Win2K */
    hres = ITask_SetAccountInformation(test_task, dummy_account_name, NULL);
    ok(hres == S_OK,
            "Failed setting dummy account with no password: %08lx\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK ||
              hres == 0x200),  /* win2k */
       "GetAccountInformation failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, dummy_account_name),
                "Got %s, expected %s\n", wine_dbgstr_w(account_name),
                wine_dbgstr_w(dummy_account_name));
        CoTaskMemFree(account_name);
    }

    /* Attempt to set to a dummy account with a (invalid) password */
    /* This test passes on WinXP but fails on Win2K */
    hres = ITask_SetAccountInformation(test_task, dummy_account_name_b,
            dummy_account_name_b);
    ok(hres == S_OK,
            "Failed setting dummy account with password: %08lx\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK ||
              hres == 0x200),  /* win2k */
       "GetAccountInformation failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, dummy_account_name_b),
                "Got %s, expected %s\n", wine_dbgstr_w(account_name),
                wine_dbgstr_w(dummy_account_name_b));
        CoTaskMemFree(account_name);
    }

    /* Attempt to set to the local system account */
    hres = ITask_SetAccountInformation(test_task, empty, NULL);
    ok(hres == S_OK, "Failed setting system account: %08lx\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK ||
              hres == 0x200),  /* win2k */
       "GetAccountInformation failed: %08lx\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, empty),
                "Got %s, expected empty string\n", wine_dbgstr_w(account_name));
        CoTaskMemFree(account_name);
    }

    ITask_Release(test_task);
}

static void test_task_state(void)
{
    ITask *test_task;
    HRESULT hr, status;
    DWORD flags, val;
    WORD val1, val2;
    SYSTEMTIME st;

    hr = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&test_task);
    ok(hr == S_OK, "Failed to setup test_task\n");

    if (0) /* crashes under Windows */
        hr = ITask_GetFlags(test_task, NULL);

    flags = 0xdeadbeef;
    hr = ITask_GetFlags(test_task, &flags);
    ok(hr == S_OK, "GetFlags error %#lx\n", hr);
    ok(flags == 0, "got %#lx\n", flags);

    if (0) /* crashes under Windows */
        hr = ITask_GetTaskFlags(test_task, NULL);

    flags = 0xdeadbeef;
    hr = ITask_GetTaskFlags(test_task, &flags);
    ok(hr == S_OK, "GetTaskFlags error %#lx\n", hr);
    ok(flags == 0, "got %#lx\n", flags);

    if (0) /* crashes under Windows */
        hr = ITask_GetStatus(test_task, NULL);

    status = 0xdeadbeef;
    hr = ITask_GetStatus(test_task, &status);
    ok(hr == S_OK, "GetStatus error %#lx\n", hr);
    ok(status == SCHED_S_TASK_NOT_SCHEDULED, "got %#lx\n", status);

    if (0) /* crashes under Windows */
        hr = ITask_GetErrorRetryCount(test_task, NULL);

    hr = ITask_GetErrorRetryCount(test_task, &val1);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);

    if (0) /* crashes under Windows */
        hr = ITask_GetErrorRetryInterval(test_task, NULL);

    hr = ITask_GetErrorRetryInterval(test_task, &val1);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);

    if (0) /* crashes under Windows */
        hr = ITask_GetIdleWait(test_task, NULL, NULL);

    val1 = 0xdead;
    val2 = 0xbeef;
    hr = ITask_GetIdleWait(test_task, &val1, &val2);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(val1 == 10, "got %u\n", val1);
    ok(val2 == 60, "got %u\n", val2);

    if (0) /* crashes under Windows */
        hr = ITask_GetPriority(test_task, NULL);

    val = 0xdeadbeef;
    hr = ITask_GetPriority(test_task, &val);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(val == NORMAL_PRIORITY_CLASS, "got %#lx\n", val);

    if (0) /* crashes under Windows */
        hr = ITask_GetExitCode(test_task, NULL);

    val = 0xdeadbeef;
    hr = ITask_GetExitCode(test_task, &val);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);
    ok(val == 0, "got %#lx\n", val);

    if (0) /* crashes under Windows */
        hr = ITask_GetMostRecentRunTime(test_task, NULL);

    memset(&st, 0xff, sizeof(st));
    hr = ITask_GetMostRecentRunTime(test_task, &st);
    ok(hr == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", hr);
    ok(st.wYear == 0, "got %u\n", st.wYear);
    ok(st.wMonth == 0, "got %u\n", st.wMonth);
    ok(st.wDay == 0, "got %u\n", st.wDay);
    ok(st.wHour == 0, "got %u\n", st.wHour);
    ok(st.wMinute == 0, "got %u\n", st.wMinute);
    ok(st.wSecond == 0, "got %u\n", st.wSecond);

    ITask_Release(test_task);
}

static void save_job(ITask *task)
{
    HRESULT hr;
    IPersistFile *pfile;

    hr = ITask_QueryInterface(task, &IID_IPersistFile, (void **)&pfile);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IPersistFile_Save(pfile, NULL, FALSE);
    ok(hr == S_OK, "got %#lx\n", hr);

    IPersistFile_Release(pfile);
}

static void test_Run(void)
{
    static const WCHAR wine_test_runW[] = { 'w','i','n','e','_','t','e','s','t','_','r','u','n',0 };
    static const WCHAR cmdW[] = { 'c','m','d','.','e','x','e',0 };
    ITask *task;
    ITaskTrigger *trigger;
    WORD idx, i;
    TASK_TRIGGER trigger_data;
    SYSTEMTIME st;
    HRESULT hr, status;

    /* cleanup after previous runs */
    taskscheduler_delete(scheduler, wine_test_runW);

    hr = ITaskScheduler_NewWorkItem(scheduler, wine_test_runW, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_Run(task);
    ok(hr == SCHED_E_TASK_NOT_READY, "got %#lx\n", hr);

    hr = ITask_Terminate(task);
    ok(hr == SCHED_E_TASK_NOT_RUNNING, "got %#lx\n", hr);

    hr = ITask_GetStatus(task, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(status == SCHED_S_TASK_NOT_SCHEDULED, "got %#lx\n", status);

    save_job(task);

    hr = ITask_GetStatus(task, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(status == SCHED_S_TASK_NOT_SCHEDULED, "got %#lx\n", status);

    hr = ITask_CreateTrigger(task, &idx, &trigger);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&trigger_data, 0, sizeof(trigger_data));
    trigger_data.cbTriggerSize = sizeof(trigger_data);
    trigger_data.Reserved1 = 0;
    GetLocalTime(&st);
    trigger_data.wBeginYear = st.wYear;
    trigger_data.wBeginMonth = st.wMonth;
    trigger_data.wBeginDay = st.wDay;
    trigger_data.wStartHour = st.wHour;
    trigger_data.wStartMinute = st.wMinute;
    trigger_data.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
    trigger_data.Type.Weekly.WeeksInterval = 1;
    trigger_data.Type.Weekly.rgfDaysOfTheWeek = 0x7f; /* every day */
    hr = ITaskTrigger_SetTrigger(trigger, &trigger_data);
    ok(hr == S_OK, "got %#lx\n", hr);
    ITaskTrigger_Release(trigger);

    hr = ITask_SetApplicationName(task, cmdW);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_SetParameters(task, empty);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_SetWorkingDirectory(task, empty);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* Save the task so that the Scheduler service would notice the changes */
    save_job(task);

    hr = ITask_GetStatus(task, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(status == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", status);

    hr = ITask_Run(task);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* According to MSDN the task status doesn't update dynamically */
    hr = ITask_GetStatus(task, &status);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(status == SCHED_S_TASK_HAS_NOT_RUN, "got %#lx\n", status);

    ITask_Release(task);

    /* Running the process associated with the task to start up may
     * take quite a bit a of time, and waiting for it during the test
     * may be not the best idea.
     *
     * This is how it's supposed to look like in the application
     * (the loop should be infinite):
     */
    for (i = 0; i < 5; i++)
    {
        hr = ITaskScheduler_Activate(scheduler, wine_test_runW, &IID_ITask, (IUnknown **)&task);
        ok(hr == S_OK, "Activate error %#lx\n", hr);

        hr = ITask_GetStatus(task, &status);
        ok(hr == S_OK, "got %#lx\n", hr);

        ITask_Release(task);

        if (status == SCHED_S_TASK_RUNNING) break;

        Sleep(100);
    }

    hr = ITaskScheduler_Activate(scheduler, wine_test_runW, &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "Activate error %#lx\n", hr);

    hr = ITask_GetStatus(task, &status);
    ok(hr == S_OK, "got %#lx\n", hr);

    if (status == SCHED_S_TASK_RUNNING)
    {
        hr = ITask_Terminate(task);
        ok(hr == S_OK, "got %#lx\n", hr);

        ITask_Release(task);

        /* Waiting for the process associated with the task to terminate
         * may take quite a bit a of time, and waiting for it during the
         * test is not practical.
         *
         * This is how it's supposed to look like in the application
         * (the loop should be infinite):
         */
        for (i = 0; i < 5; i++)
        {
            hr = ITaskScheduler_Activate(scheduler, wine_test_runW, &IID_ITask, (IUnknown **)&task);
            ok(hr == S_OK, "Activate error %#lx\n", hr);

            hr = ITask_GetStatus(task, &status);
            ok(hr == S_OK, "got %#lx\n", hr);

            ITask_Release(task);

            if (status != SCHED_S_TASK_RUNNING) break;

            Sleep(100);
        }
    }
    else
        ITask_Release(task);

    hr = taskscheduler_delete(scheduler, wine_test_runW);
    ok(hr == S_OK, "got %#lx\n", hr);
}

static void test_SetFlags(void)
{
    HRESULT hr;
    ITask *task;
    DWORD flags;

    hr = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = ITask_SetFlags(task, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    flags = 0xdeadbeef;
    hr = ITask_GetFlags(task, &flags);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(flags == 0, "got %#lx\n", flags);

    hr = ITask_SetFlags(task, 0xffffffff);
    ok(hr == S_OK, "got %#lx\n", hr);

    flags = 0xdeadbeef;
    hr = ITask_GetFlags(task, &flags);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(flags == 0x7fff, "got %#lx\n", flags);

    hr = ITask_SetFlags(task, 0x9234);
    ok(hr == S_OK, "got %#lx\n", hr);

    flags = 0xdeadbeef;
    hr = ITask_GetFlags(task, &flags);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(flags == 0x1234, "got %#lx\n", flags);

    ITask_Release(task);
}

static void test_workitem_data(void)
{
    static BYTE hello[] = "Hello World!";
    HRESULT hr;
    ITask *task;
    WORD count;
    BYTE *data;

    hr = ITaskScheduler_NewWorkItem(scheduler, task_name, &CLSID_CTask,
                                    &IID_ITask, (IUnknown **)&task);
    ok(hr == S_OK, "got %#lx\n", hr);

    if (0) /* crashes under Windows */
        hr = ITask_GetWorkItemData(task, &count, NULL);
    if (0) /* crashes under Windows */
        hr = ITask_GetWorkItemData(task, NULL, &data);

    count = 0xdead;
    data = (BYTE *)0xdeadbeef;
    hr = ITask_GetWorkItemData(task, &count, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %u\n", count);
    ok(data == NULL, "got %p\n", data);

    hr = ITask_SetWorkItemData(task, sizeof(hello), NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = ITask_SetWorkItemData(task, 0, hello);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = ITask_SetWorkItemData(task, sizeof(hello), hello);
    ok(hr == S_OK, "got %#lx\n", hr);

    count = 0xdead;
    data = NULL;
    hr = ITask_GetWorkItemData(task, &count, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == sizeof(hello), "got %u\n", count);
    ok(data != NULL, "got NULL\n");
    ok(!memcmp(data, hello, sizeof(hello)), "data mismatch\n");
    CoTaskMemFree(data);

    hr = ITask_SetWorkItemData(task, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    count = 0xdead;
    data = (BYTE *)0xdeadbeef;
    hr = ITask_GetWorkItemData(task, &count, &data);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %u\n", count);
    ok(data == NULL, "got %p\n", data);

    ITask_Release(task);
}

START_TEST(task)
{
    HRESULT hr;

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ITaskScheduler, (void **)&scheduler);
    ok(hr == S_OK, "failed to create task scheduler: %#lx\n", hr);

    test_SetApplicationName_GetApplicationName();
    test_CreateTrigger();
    test_SetParameters_GetParameters();
    test_SetComment_GetComment();
    test_SetMaxRunTime_GetMaxRunTime();
    test_SetAccountInformation_GetAccountInformation();
    test_task_state();
    test_Run();
    test_SetFlags();
    test_workitem_data();

    ITaskScheduler_Release(scheduler);
    CoUninitialize();
}
