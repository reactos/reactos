/*
 * Unit test suite for bits functions
 *
 * Copyright 2007 Google (Roy Shea)
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

#include <stdio.h>

#define COBJMACROS

#include "wine/test.h"
#include "bits.h"

static WCHAR progname[MAX_PATH];

static HRESULT test_create_manager(void)
{
    HRESULT hres;
    IBackgroundCopyManager *manager = NULL;

    /* Creating BITS instance */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL, CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager, (void **) &manager);

    if(hres == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED)) {
        win_skip("Needed Service is disabled\n");
        return hres;
    }

    if (hres == S_OK)
        IBackgroundCopyManager_Release(manager);

    return hres;
}

static void test_CreateJob(void)
{
    /* Job information */
    IBackgroundCopyJob* job = NULL;
    GUID tmpId;
    HRESULT hres;
    ULONG res;
    IBackgroundCopyManager* manager = NULL;

    /* Setup */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    /* Create bits job */
    hres = IBackgroundCopyManager_CreateJob(manager, L"Test", BG_JOB_TYPE_DOWNLOAD, &tmpId, &job);
    ok(hres == S_OK, "CreateJob failed: %08lx\n", hres);

    res = IBackgroundCopyJob_Release(job);
    ok(res == 0, "Bad ref count on release: %lu\n", res);
    IBackgroundCopyManager_Release(manager);
}

static void test_EnumJobs(void)
{
    /* Job Enumerator */
    IEnumBackgroundCopyJobs* enumJobs;
    IBackgroundCopyManager *manager = NULL;
    IBackgroundCopyJob *job = NULL;
    HRESULT hres;
    GUID tmpId;

    /* Setup */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyManager_CreateJob(manager, L"Test", BG_JOB_TYPE_DOWNLOAD, &tmpId, &job);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyManager_EnumJobs(manager, 0, &enumJobs);
    ok(hres == S_OK, "EnumJobs failed: %08lx\n", hres);
    IEnumBackgroundCopyJobs_Release(enumJobs);

    /* Tear down */
    IBackgroundCopyJob_Release(job);
    IBackgroundCopyManager_Release(manager);
}

static void run_child(WCHAR *secret)
{
    WCHAR cmdline[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOW startup;

    memset(&startup, 0, sizeof startup);
    startup.cb = sizeof startup;

    wsprintfW(cmdline, L"%s qmgr %s", progname, secret);
    ok(CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_child_process(info.hProcess);
    ok(CloseHandle(info.hProcess), "CloseHandle\n");
    ok(CloseHandle(info.hThread), "CloseHandle\n");
}

static void do_child(const char *secretA)
{
    WCHAR secretW[MAX_PATH];
    IBackgroundCopyManager *manager = NULL;
    GUID id;
    IBackgroundCopyJob *job;
    HRESULT hres;
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    MultiByteToWideChar(CP_ACP, 0, secretA, -1, secretW, MAX_PATH);
    hres = IBackgroundCopyManager_CreateJob(manager, secretW,
                                            BG_JOB_TYPE_DOWNLOAD, &id, &job);
    ok(hres == S_OK, "CreateJob in child process\n");
    IBackgroundCopyJob_Release(job);
    IBackgroundCopyManager_Release(manager);
}

static void test_globalness(void)
{
    WCHAR secretName[MAX_PATH];
    IEnumBackgroundCopyJobs* enumJobs;
    IBackgroundCopyManager *manager = NULL;
    HRESULT hres;
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    wsprintfW(secretName, L"test_%u", GetTickCount());
    run_child(secretName);

    hres = IBackgroundCopyManager_EnumJobs(manager, 0, &enumJobs);
    ok(hres == S_OK, "EnumJobs failed: %08lx\n", hres);
    if(hres != S_OK)
        skip("Unable to create job enumerator.\n");
    else
    {
        ULONG i, n;
        IBackgroundCopyJob *job;
        BOOL found = FALSE;

        hres = IEnumBackgroundCopyJobs_GetCount(enumJobs, &n);
        ok(hres == S_OK, "GetCount failed: %08lx\n", hres);
        for (i = 0; i < n && !found; ++i)
        {
            LPWSTR name;
            IEnumBackgroundCopyJobs_Next(enumJobs, 1, &job, NULL);
            IBackgroundCopyJob_GetDisplayName(job, &name);
            if (lstrcmpW(name, secretName) == 0)
                found = TRUE;
            CoTaskMemFree(name);
            IBackgroundCopyJob_Release(job);
        }

        IEnumBackgroundCopyJobs_Release(enumJobs);
        ok(found, "Adding a job in another process failed\n");
    }

    IBackgroundCopyManager_Release(manager);
}

START_TEST(qmgr)
{
    char **argv;
    int argc = winetest_get_mainargs(&argv);
    MultiByteToWideChar(CP_ACP, 0, argv[0], -1, progname, MAX_PATH);

    CoInitialize(NULL);

    if (FAILED(test_create_manager()))
    {
        win_skip("Failed to create Manager instance, skipping tests\n");
        CoUninitialize();
        return;
    }

    if (argc == 3)
        do_child(argv[2]);
    else
    {
        test_CreateJob();
        test_EnumJobs();
        test_globalness();
    }
    CoUninitialize();
}
