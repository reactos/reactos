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

#include "corerror.h"
#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *test_task_scheduler;
static ITask *test_task;
static const WCHAR empty[] = {0};

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_tmp_space(int size)
{
    static char *list[16];
    static long pos;
    char *ret;
    int idx;

    idx = ++pos % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc(list[idx], size)))
        list[idx] = ret;
    return ret;
}

static const char *dbgstr_w(LPCWSTR str)
{
    char *buf;
    int len;
    if(!str)
        return "(null)";
    len = lstrlenW(str) + 1;
    buf = get_tmp_space(len);
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, len, NULL, NULL);
    return buf;
}

static BOOL setup_task(void)
{
    HRESULT hres;
    const WCHAR task_name[] = {'T','e','s','t','i','n','g', 0};

    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    if(hres != S_OK)
        return FALSE;
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
            &IID_ITask, (IUnknown**)&test_task);
    if(hres != S_OK)
    {
        ITaskScheduler_Release(test_task_scheduler);
        return FALSE;
    }
    return TRUE;
}

static void cleanup_task(void)
{
    ITask_Release(test_task);
    ITaskScheduler_Release(test_task_scheduler);
}

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
    BOOL setup;
    HRESULT hres;
    LPWSTR stored_name;
    LPCWSTR full_name;
    const WCHAR non_application_name[] = {'N','o','S','u','c','h',
            'A','p','p','l','i','c','a','t','i','o','n', 0};
    const WCHAR notepad_exe[] = {
            'n','o','t','e','p','a','d','.','e','x','e', 0};
    const WCHAR notepad[] = {'n','o','t','e','p','a','d', 0};

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Attempt getting before setting application name */
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpiW(stored_name, empty),
                "Got %s, expected empty string\n", dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    /* Set application name to a nonexistent application and then get
     * the application name that is actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name with program type extension and then
     * get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad_exe);
    ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(notepad_exe), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad_exe);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name without program type extension and
     * then get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad);
    ok(hres == S_OK, "Failed setting name %s: %08x\n", dbgstr_w(notepad), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad_exe);  /* XP SP1 appends .exe */
        if (lstrcmpiW(stored_name, full_name) != 0)
        {
            full_name = path_resolve_name(notepad);
            ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
               dbgstr_w(stored_name), dbgstr_w(full_name));
        }
        CoTaskMemFree(stored_name);
    }

    /* After having a valid application name set, set application the name
     * to a nonexistent application and then get the name that is
     * actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        ok(!lstrcmpiW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Clear application name */
    hres = ITask_SetApplicationName(test_task, empty);
    ok(hres == S_OK, "Failed setting name %s: %08x\n", dbgstr_w(empty), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpiW(stored_name, empty),
                "Got %s, expected empty string\n", dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    cleanup_task();
    return;
}

static void test_CreateTrigger(void)
{
    BOOL setup;
    HRESULT hres;
    WORD trigger_index;
    ITaskTrigger *test_trigger;

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    hres = ITask_CreateTrigger(test_task, &trigger_index, &test_trigger);
    ok(hres == S_OK, "Failed to create trigger: 0x%08x\n", hres);
    if (hres != S_OK)
    {
        cleanup_task();
        return;
    }

    ITaskTrigger_Release(test_trigger);
    cleanup_task();
    return;
}

static void test_SetParameters_GetParameters(void)
{
    BOOL setup;
    HRESULT hres;
    LPWSTR parameters;
    const WCHAR parameters_a[] = {'f','o','o','.','t','x','t', 0};
    const WCHAR parameters_b[] = {'f','o','o','.','t','x','t',' ',
        'b','a','r','.','t','x','t', 0};

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Get parameters before setting them */
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, empty),
                "Got %s, expected empty string\n", dbgstr_w(parameters));
        CoTaskMemFree(parameters);
    }

    /* Set parameters to a simple string */
    hres = ITask_SetParameters(test_task, parameters_a);
    ok(hres == S_OK, "Failed setting parameters %s: %08x\n",
            dbgstr_w(parameters_a), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, parameters_a), "Got %s, expected %s\n",
                dbgstr_w(parameters), dbgstr_w(parameters_a));
        CoTaskMemFree(parameters);
    }

    /* Update parameters to a different simple string */
    hres = ITask_SetParameters(test_task, parameters_b);
    ok(hres == S_OK, "Failed setting parameters %s: %08x\n",
            dbgstr_w(parameters_b), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, parameters_b), "Got %s, expected %s\n",
                dbgstr_w(parameters), dbgstr_w(parameters_b));
        CoTaskMemFree(parameters);
    }

    /* Clear parameters */
    hres = ITask_SetParameters(test_task, empty);
    ok(hres == S_OK, "Failed setting parameters %s: %08x\n",
            dbgstr_w(empty), hres);
    hres = ITask_GetParameters(test_task, &parameters);
    ok(hres == S_OK, "GetParameters failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(parameters, empty),
                "Got %s, expected empty string\n", dbgstr_w(parameters));
        CoTaskMemFree(parameters);
    }

    cleanup_task();
    return;
}

static void test_SetComment_GetComment(void)
{
    BOOL setup;
    HRESULT hres;
    LPWSTR comment;
    const WCHAR comment_a[] = {'C','o','m','m','e','n','t','.', 0};
    const WCHAR comment_b[] = {'L','o','n','g','e','r',' ',
            'c','o','m','m','e','n','t','.', 0};

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Get comment before setting it*/
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, empty),
                "Got %s, expected empty string\n", dbgstr_w(comment));
        CoTaskMemFree(comment);
    }

    /* Set comment to a simple string */
    hres = ITask_SetComment(test_task, comment_a);
    ok(hres == S_OK, "Failed setting comment %s: %08x\n",
            dbgstr_w(comment_a), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, comment_a), "Got %s, expected %s\n",
                dbgstr_w(comment), dbgstr_w(comment_a));
        CoTaskMemFree(comment);
    }

    /* Update comment to a different simple string */
    hres = ITask_SetComment(test_task, comment_b);
    ok(hres == S_OK, "Failed setting comment %s: %08x\n",
            dbgstr_w(comment_b), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, comment_b), "Got %s, expected %s\n",
                dbgstr_w(comment), dbgstr_w(comment_b));
        CoTaskMemFree(comment);
    }

    /* Clear comment */
    hres = ITask_SetComment(test_task, empty);
    ok(hres == S_OK, "Failed setting comment %s: %08x\n",
            dbgstr_w(empty), hres);
    hres = ITask_GetComment(test_task, &comment);
    ok(hres == S_OK, "GetComment failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(comment, empty),
                "Got %s, expected empty string\n", dbgstr_w(comment));
        CoTaskMemFree(comment);
    }

    cleanup_task();
    return;
}

static void test_SetMaxRunTime_GetMaxRunTime(void)
{
    BOOL setup;
    HRESULT hres;
    DWORD max_run_time;

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Default time is 3 days:
     * 3 days * 24 hours * 60 minutes * 60 seconds * 1000 ms = 259200000 */
    max_run_time = 0;
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08x\n", hres);
    ok(max_run_time == 259200000, "Expected 259200000: %d\n", max_run_time);

    /* Basic set test */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, 1234);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08x\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08x\n", hres);
    ok(max_run_time == 1234, "Expected 1234: %d\n", max_run_time);

    /* Verify that time can be set to zero */
    max_run_time = 1;
    hres = ITask_SetMaxRunTime(test_task, 0);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08x\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08x\n", hres);
    ok(max_run_time == 0, "Expected 0: %d\n", max_run_time);

    /* Check resolution by setting time to one */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, 1);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08x\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08x\n", hres);
    ok(max_run_time == 1, "Expected 1: %d\n", max_run_time);

    /* Verify that time can be set to INFINITE */
    max_run_time = 0;
    hres = ITask_SetMaxRunTime(test_task, INFINITE);
    ok(hres == S_OK, "Failed to set max runtime: 0x%08x\n", hres);
    hres = ITask_GetMaxRunTime(test_task, &max_run_time);
    ok(hres == S_OK, "Failed to get max runtime: 0x%08x\n", hres);
    ok(max_run_time == INFINITE, "Expected INFINITE: %d\n", max_run_time);

    cleanup_task();
    return;
}

static void test_SetAccountInformation_GetAccountInformation(void)
{
    BOOL setup;
    HRESULT hres;
    LPWSTR account_name;
    const WCHAR dummy_account_name[] = {'N', 'o', 'S', 'u', 'c', 'h',
            'A', 'c', 'c', 'o', 'u', 'n', 't', 0};
    const WCHAR dummy_account_name_b[] = {'N', 'o', 'S', 'u', 'c', 'h',
            'A', 'c', 'c', 'o', 'u', 'n', 't', 'B', 0};

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Get account information before it is set */
    hres = ITask_GetAccountInformation(test_task, &account_name);
    /* WinXP returns HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): 0x80070002 but
     * Win2K returns SCHED_E_CANNOT_OPEN_TASK: 0x8004130d
     * Win9x doesn't support security services */
    if (hres == SCHED_E_NO_SECURITY_SERVICES)
    {
        win_skip("Security services are not supported\n");
        cleanup_task();
        return;
    }
    ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            hres == SCHED_E_CANNOT_OPEN_TASK,
            "Unset account name generated: 0x%08x\n", hres);

    /* Attempt to set to a dummy account without a password */
    /* This test passes on WinXP but fails on Win2K */
    hres = ITask_SetAccountInformation(test_task, dummy_account_name, NULL);
    ok(hres == S_OK,
            "Failed setting dummy account with no password: %08x\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK),
       "GetAccountInformation failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, dummy_account_name),
                "Got %s, expected %s\n", dbgstr_w(account_name),
                dbgstr_w(dummy_account_name));
        CoTaskMemFree(account_name);
    }

    /* Attempt to set to a dummy account with a (invalid) password */
    /* This test passes on WinXP but fails on Win2K */
    hres = ITask_SetAccountInformation(test_task, dummy_account_name_b,
            dummy_account_name_b);
    ok(hres == S_OK,
            "Failed setting dummy account with password: %08x\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK),
       "GetAccountInformation failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, dummy_account_name_b),
                "Got %s, expected %s\n", dbgstr_w(account_name),
                dbgstr_w(dummy_account_name_b));
        CoTaskMemFree(account_name);
    }

    /* Attempt to set to the local system account */
    hres = ITask_SetAccountInformation(test_task, empty, NULL);
    ok(hres == S_OK, "Failed setting system account: %08x\n", hres);
    hres = ITask_GetAccountInformation(test_task, &account_name);
    ok(hres == S_OK ||
       broken(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
              hres == SCHED_E_CANNOT_OPEN_TASK),
       "GetAccountInformation failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok(!lstrcmpW(account_name, empty),
                "Got %s, expected empty string\n", dbgstr_w(account_name));
        CoTaskMemFree(account_name);
    }

    cleanup_task();
    return;
}

START_TEST(task)
{
    CoInitialize(NULL);
    test_SetApplicationName_GetApplicationName();
    test_CreateTrigger();
    test_SetParameters_GetParameters();
    test_SetComment_GetComment();
    test_SetMaxRunTime_GetMaxRunTime();
    test_SetAccountInformation_GetAccountInformation();
    CoUninitialize();
}
