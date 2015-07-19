/*
 * Test suite for TaskScheduler interface
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

#include "initguid.h"
#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *test_task_scheduler;

static const WCHAR does_not_existW[] = {'\\','\\','d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};

static void test_NewWorkItem(void)
{
    HRESULT hres;
    ITask *task;
    const WCHAR task_name[] = {'T', 'e', 's', 't', 'i', 'n', 'g', 0};
    GUID GUID_BAD;

    /* Initialize a GUID that will not be a recognized CLSID or a IID */
    CoCreateGuid(&GUID_BAD);

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    /* Test basic task creation */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &CLSID_CTask, &IID_ITask, (IUnknown**)&task);
    ok(hres == S_OK, "NewNetworkItem failed: %08x\n", hres);
    if (hres == S_OK)
        ITask_Release(task);

    /* Task creation attempt using invalid work item class ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &GUID_BAD, &IID_ITask, (IUnknown**)&task);
    ok(hres == CLASS_E_CLASSNOTAVAILABLE,
            "Expected CLASS_E_CLASSNOTAVAILABLE: %08x\n", hres);

    /* Task creation attempt using invalid interface ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &CLSID_CTask, &GUID_BAD, (IUnknown**)&task);
    ok(hres == E_NOINTERFACE, "Expected E_NOINTERFACE: %08x\n", hres);

    /* Task creation attempt using invalid work item class and interface ID */
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name,
            &GUID_BAD, &GUID_BAD, (IUnknown**)&task);
    ok(hres == CLASS_E_CLASSNOTAVAILABLE,
            "Expected CLASS_E_CLASSNOTAVAILABLE: %08x\n", hres);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_Activate(void)
{
    HRESULT hres;
    ITask *task = NULL;
    const WCHAR not_task_name[] =
            {'N', 'o', 'S', 'u', 'c', 'h', 'T', 'a', 's', 'k', 0};

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    /* Attempt to activate a nonexistent task */
    hres = ITaskScheduler_Activate(test_task_scheduler, not_task_name,
            &IID_ITask, (IUnknown**)&task);
    ok(hres == COR_E_FILENOTFOUND, "Expected COR_E_FILENOTFOUND: %08x\n", hres);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_GetTargetComputer(void)
{
    HRESULT hres;
    WCHAR *oldname;

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.\n");
        return;
    }

    if (0)
    {
        /* This crashes on w2k */
        hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, NULL);
        ok(hres == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hres);
    }

    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &oldname);
    ok((hres == S_OK) && oldname && oldname[0] == '\\' && oldname[1] == '\\' && oldname[2],
        "got 0x%x and %s (expected S_OK and an unc name)\n", hres, wine_dbgstr_w(oldname));

    CoTaskMemFree(oldname);

    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_SetTargetComputer(void)
{
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 3];  /* extra space for two '\' and a zero */
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1;    /* extra space for a zero */
    WCHAR *oldname = NULL;
    WCHAR *name = NULL;
    HRESULT hres;


    buffer[0] = '\\';
    buffer[1] = '\\';
    if (!GetComputerNameW(buffer + 2, &len))
        return;

    /* Create TaskScheduler */
    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    ok(hres == S_OK, "CTaskScheduler CoCreateInstance failed: %08x\n", hres);
    if (hres != S_OK)
    {
        skip("Failed to create task scheduler.  Skipping tests.\n");
        return;
    }

    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &oldname);
    ok(hres == S_OK, "got 0x%x and %s (expected S_OK)\n", hres, wine_dbgstr_w(oldname));

    /* NULL is an alias for the local computer */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, NULL);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);
    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &name);
    ok((hres == S_OK && !lstrcmpiW(name, buffer)),
        "got 0x%x with %s (expected S_OK and %s)\n",
        hres, wine_dbgstr_w(name), wine_dbgstr_w(buffer));
    CoTaskMemFree(name);

    /* The name must be valid */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, does_not_existW);
    ok(hres == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH), "got 0x%x (expected 0x80070035)\n", hres);
    /* the name of the target computer is unchanged */
    hres = ITaskScheduler_GetTargetComputer(test_task_scheduler, &name);
    ok((hres == S_OK && !lstrcmpiW(name, buffer)),
        "got 0x%x with %s (expected S_OK and %s)\n",
        hres, wine_dbgstr_w(name), wine_dbgstr_w(buffer));
    CoTaskMemFree(name);

    /* the two backslashes are optional */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, oldname + 2);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

    /* the case is ignored */
    CharUpperW(buffer);
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, buffer);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);
    CharLowerW(buffer);
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, buffer);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

    /* cleanup */
    hres = ITaskScheduler_SetTargetComputer(test_task_scheduler, oldname);
    ok(hres == S_OK, "got 0x%x (expected S_OK)\n", hres);

    CoTaskMemFree(oldname);
    ITaskScheduler_Release(test_task_scheduler);
    return;
}

static void test_Enum(void)
{
    ITaskScheduler *scheduler;
    IEnumWorkItems *tasks;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **)&scheduler);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) { /* crashes on win2k */
    hr = ITaskScheduler_Enum(scheduler, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = ITaskScheduler_Enum(scheduler, &tasks);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IEnumWorkItems_Release(tasks);

    ITaskScheduler_Release(scheduler);
}

START_TEST(task_scheduler)
{
    CoInitialize(NULL);
    test_NewWorkItem();
    test_Activate();
    test_GetTargetComputer();
    test_SetTargetComputer();
    test_Enum();
    CoUninitialize();
}
