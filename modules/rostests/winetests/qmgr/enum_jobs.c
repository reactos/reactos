/*
 * Unit test suite for Enum Background Copy Jobs Interface
 *
 * Copyright 2007 Google (Roy Shea, Dan Hipschman)
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

/* Globals used by many tests */
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_jobA;
static IBackgroundCopyJob *test_jobB;
static ULONG test_jobCountB;
static IEnumBackgroundCopyJobs *test_enumJobsA;
static IEnumBackgroundCopyJobs *test_enumJobsB;
static GUID test_jobIdA;
static GUID test_jobIdB;

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;

    test_manager = NULL;
    test_jobA = NULL;
    test_jobB = NULL;
    memset(&test_jobIdA, 0, sizeof test_jobIdA);
    memset(&test_jobIdB, 0, sizeof test_jobIdB);

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, L"TestA", BG_JOB_TYPE_DOWNLOAD,
                                            &test_jobIdA, &test_jobA);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_EnumJobs(test_manager, 0, &test_enumJobsA);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, L"TestB", BG_JOB_TYPE_DOWNLOAD,
                                            &test_jobIdB, &test_jobB);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_EnumJobs(test_manager, 0, &test_enumJobsB);
    if(hres != S_OK)
        return FALSE;

    hres = IEnumBackgroundCopyJobs_GetCount(test_enumJobsB, &test_jobCountB);
    if (hres != S_OK)
        return FALSE;

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    if (test_enumJobsB)
        IEnumBackgroundCopyJobs_Release(test_enumJobsB);
    test_enumJobsB = NULL;
    if (test_jobB)
        IBackgroundCopyJob_Release(test_jobB);
    test_jobB = NULL;
    if (test_enumJobsA)
        IEnumBackgroundCopyJobs_Release(test_enumJobsA);
    test_enumJobsA = NULL;
    if (test_jobA)
        IBackgroundCopyJob_Release(test_jobA);
    test_jobA = NULL;
    if (test_manager)
        IBackgroundCopyManager_Release(test_manager);
    test_manager = NULL;
}

/* We can't assume the job count will start at any fixed number since esp
   when testing on Windows there may be other jobs created by other
   processes.  Even this approach of creating two jobs and checking the
   difference in counts could fail if a job was created in between, but
   it's probably not worth worrying about in sane test environments.  */
static void test_GetCount(void)
{
    HRESULT hres;
    ULONG jobCountA, jobCountB;

    hres = IEnumBackgroundCopyJobs_GetCount(test_enumJobsA, &jobCountA);
    ok(hres == S_OK, "GetCount failed: %08lx\n", hres);

    hres = IEnumBackgroundCopyJobs_GetCount(test_enumJobsB, &jobCountB);
    ok(hres == S_OK, "GetCount failed: %08lx\n", hres);

    ok(jobCountB == jobCountA + 1, "Got incorrect count\n");
}

/* Test Next with a NULL pceltFetched*/
static void test_Next_walkListNull(void)
{
    HRESULT hres;
    IBackgroundCopyJob *job;
    ULONG i;

    /* Fetch the available jobs */
    for (i = 0; i < test_jobCountB; i++)
    {
        hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, 1, &job, NULL);
        ok(hres == S_OK, "Next failed: %08lx\n", hres);
        IBackgroundCopyJob_Release(job);
    }

    /* Attempt to fetch one more than the number of available jobs */
    hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, 1, &job, NULL);
    ok(hres == S_FALSE, "Next off end of available jobs failed: %08lx\n", hres);
}

/* Test Next */
static void test_Next_walkList_1(void)
{
    HRESULT hres;
    IBackgroundCopyJob *job;
    ULONG fetched;
    ULONG i;

    /* Fetch the available jobs */
    for (i = 0; i < test_jobCountB; i++)
    {
        fetched = 0;
        hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, 1, &job, &fetched);
        ok(hres == S_OK, "Next failed: %08lx\n", hres);
        ok(fetched == 1, "Next returned the incorrect number of jobs: %08lx\n", hres);
        IBackgroundCopyJob_Release(job);
    }

    /* Attempt to fetch one more than the number of available jobs */
    fetched = 0;
    hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, 1, &job, &fetched);
    ok(hres == S_FALSE, "Next off end of available jobs failed: %08lx\n", hres);
    ok(fetched == 0, "Next returned the incorrect number of jobs: %08lx\n", hres);
}

/* Test Next by requesting multiple files at a time */
static void test_Next_walkList_2(void)
{
    HRESULT hres;
    IBackgroundCopyJob **jobs;
    ULONG fetched;
    ULONG i;

    jobs = HeapAlloc(GetProcessHeap(), 0, test_jobCountB * sizeof *jobs);
    for (i = 0; i < test_jobCountB; i++)
        jobs[i] = NULL;

    fetched = 0;
    hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, test_jobCountB, jobs, &fetched);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetched == test_jobCountB, "Next returned the incorrect number of jobs: %08lx\n", hres);

    for (i = 0; i < test_jobCountB; i++)
    {
        ok(jobs[i] != NULL, "Next returned NULL\n");
        if (jobs[i])
            IBackgroundCopyJob_Release(jobs[i]);
    }

    HeapFree(GetProcessHeap(), 0, jobs);
}

/* Test Next Error conditions */
static void test_Next_errors(void)
{
    HRESULT hres;
    IBackgroundCopyJob *jobs[2];

    /* E_INVALIDARG: pceltFetched can ONLY be NULL if celt is 1 */
    hres = IEnumBackgroundCopyJobs_Next(test_enumJobsB, 2, jobs, NULL);
    ok(hres != S_OK, "Invalid call to Next succeeded: %08lx\n", hres);
}

/* Test skipping through the jobs in a list */
static void test_Skip_walkList(void)
{
    HRESULT hres;
    ULONG i;

    for (i = 0; i < test_jobCountB; i++)
    {
        hres = IEnumBackgroundCopyJobs_Skip(test_enumJobsB, 1);
        ok(hres == S_OK, "Skip failed: %08lx\n", hres);
    }

    hres = IEnumBackgroundCopyJobs_Skip(test_enumJobsB, 1);
    ok(hres == S_FALSE, "Skip expected end of list: %08lx\n", hres);
}

/* Test skipping off the end of the list */
static void test_Skip_offEnd(void)
{
    HRESULT hres;

    hres = IEnumBackgroundCopyJobs_Skip(test_enumJobsB, test_jobCountB + 1);
    ok(hres == S_FALSE, "Skip expected end of list: %08lx\n", hres);
}

/* Test reset */
static void test_Reset(void)
{
    HRESULT hres;

    hres = IEnumBackgroundCopyJobs_Skip(test_enumJobsB, test_jobCountB);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);

    hres = IEnumBackgroundCopyJobs_Reset(test_enumJobsB);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    hres = IEnumBackgroundCopyJobs_Skip(test_enumJobsB, test_jobCountB);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);
}

typedef void (*test_t)(void);

START_TEST(enum_jobs)
{
    static const test_t tests[] = {
        test_GetCount,
        test_Next_walkListNull,
        test_Next_walkList_1,
        test_Next_walkList_2,
        test_Next_errors,
        test_Skip_walkList,
        test_Skip_offEnd,
        test_Reset,
        0
    };
    const test_t *test;
    int i;

    CoInitialize(NULL);
    for (test = tests, i = 0; *test; ++test, ++i)
    {
        /* Keep state separate between tests */
        if (!setup())
        {
            teardown();
            ok(0, "tests:%d: Unable to setup test\n", i);
            break;
        }
        (*test)();
        teardown();
    }
    CoUninitialize();
}
