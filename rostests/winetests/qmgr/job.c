/*
 * Unit test suite for Background Copy Job Interface
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
#include "initguid.h"

/* Globals used by many tests */
static const WCHAR test_displayName[] = {'T', 'e', 's', 't', 0};
static const WCHAR test_remoteNameA[] = {'r','e','m','o','t','e','A', 0};
static const WCHAR test_remoteNameB[] = {'r','e','m','o','t','e','B', 0};
static const WCHAR test_localNameA[] = {'l','o','c','a','l','A', 0};
static const WCHAR test_localNameB[] = {'l','o','c','a','l','B', 0};
static WCHAR *test_currentDir;
static WCHAR *test_remotePathA;
static WCHAR *test_remotePathB;
static WCHAR *test_localPathA;
static WCHAR *test_localPathB;
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_job;
static GUID test_jobId;
static BG_JOB_TYPE test_type;

static BOOL init_paths(void)
{
    static const WCHAR format[] = {'%','s','\\','%','s', 0};
    DWORD n;

    n = GetCurrentDirectoryW(0, NULL);
    if (n == 0)
    {
        skip("Couldn't get current directory size\n");
        return FALSE;
    }

    test_currentDir = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    test_localPathA
        = HeapAlloc(GetProcessHeap(), 0,
                    (n + 1 + lstrlenW(test_localNameA)) * sizeof(WCHAR));
    test_localPathB
        = HeapAlloc(GetProcessHeap(), 0,
                    (n + 1 + lstrlenW(test_localNameB)) * sizeof(WCHAR));
    test_remotePathA
        = HeapAlloc(GetProcessHeap(), 0,
                    (n + 1 + lstrlenW(test_remoteNameA)) * sizeof(WCHAR));
    test_remotePathB
        = HeapAlloc(GetProcessHeap(), 0,
                    (n + 1 + lstrlenW(test_remoteNameB)) * sizeof(WCHAR));

    if (!test_currentDir || !test_localPathA || !test_localPathB
        || !test_remotePathA || !test_remotePathB)
    {
        skip("Couldn't allocate memory for full paths\n");
        return FALSE;
    }

    if (GetCurrentDirectoryW(n, test_currentDir) != n - 1)
    {
        skip("Couldn't get current directory\n");
        return FALSE;
    }

    wsprintfW(test_localPathA, format, test_currentDir, test_localNameA);
    wsprintfW(test_localPathB, format, test_currentDir, test_localNameB);
    wsprintfW(test_remotePathA, format, test_currentDir, test_remoteNameA);
    wsprintfW(test_remotePathB, format, test_currentDir, test_remoteNameB);

    return TRUE;
}

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;

    test_manager = NULL;
    test_job = NULL;
    memset(&test_jobId, 0, sizeof test_jobId);
    test_type = BG_JOB_TYPE_DOWNLOAD;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager,
                            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayName,
                                            test_type, &test_jobId, &test_job);
    if(hres != S_OK)
    {
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    IBackgroundCopyJob_Release(test_job);
    IBackgroundCopyManager_Release(test_manager);
}

/* FIXME: Remove when Wine has implemented this */
DEFINE_GUID(CLSID_BackgroundCopyManager2_0, 0x6d18ad12, 0xbde3, 0x4393, 0xb3,0x11, 0x09,0x9c,0x34,0x6e,0x6d,0xf9);

static BOOL check_bits20(void)
{
    HRESULT hres;
    IBackgroundCopyManager *manager;
    BOOL ret = TRUE;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager2_0, NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager,
                            (void **) &manager);

    if (hres == REGDB_E_CLASSNOTREG)
    {
        ret = FALSE;

        /* FIXME: Wine implements 2.0 functionality but doesn't advertise 2.0
         *
         * Remove when Wine is fixed
         */
        if (setup())
        {
            HRESULT hres2;

            hres2 = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                               test_localPathA);
            if (hres2 == S_OK)
            {
                trace("Running on Wine, claim 2.0 is present\n");
                ret = TRUE;
            }
            teardown();
        }
    }

    if (manager)
        IBackgroundCopyManager_Release(manager);

    return ret;
}

/* Test that the jobId is properly set */
static void test_GetId(void)
{
    HRESULT hres;
    GUID tmpId;

    hres = IBackgroundCopyJob_GetId(test_job, &tmpId);
    ok(hres == S_OK, "GetId failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get ID of test_job.\n");
        return;
    }
    ok(memcmp(&tmpId, &test_jobId, sizeof tmpId) == 0, "Got incorrect GUID\n");
}

/* Test that the type is properly set */
static void test_GetType(void)
{
    HRESULT hres;
    BG_JOB_TYPE type;

    hres = IBackgroundCopyJob_GetType(test_job, &type);
    ok(hres == S_OK, "GetType failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get type of test_job.\n");
        return;
    }
    ok(type == test_type, "Got incorrect type\n");
}

/* Test that the display name is properly set */
static void test_GetName(void)
{
    HRESULT hres;
    LPWSTR displayName;

    hres = IBackgroundCopyJob_GetDisplayName(test_job, &displayName);
    ok(hres == S_OK, "GetName failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get display name of test_job.\n");
        return;
    }
    ok(lstrcmpW(displayName, test_displayName) == 0, "Got incorrect type\n");
    CoTaskMemFree(displayName);
}

/* Test adding a file */
static void test_AddFile(void)
{
    HRESULT hres;

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                      test_localPathA);
    ok(hres == S_OK, "First call to AddFile failed: 0x%08x\n", hres);
    if (hres != S_OK)
    {
        skip("Unable to add first file to job\n");
        return;
    }

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathB,
                                      test_localPathB);
    ok(hres == S_OK, "Second call to AddFile failed: 0x%08x\n", hres);
}

/* Test adding a set of files */
static void test_AddFileSet(void)
{
    HRESULT hres;
    BG_FILE_INFO files[2] =
        {
            {test_remotePathA, test_localPathA},
            {test_remotePathB, test_localPathB}
        };
    hres = IBackgroundCopyJob_AddFileSet(test_job, 2, files);
    ok(hres == S_OK, "AddFileSet failed: 0x%08x\n", hres);
}

/* Test creation of a job enumerator */
static void test_EnumFiles(void)
{
    HRESULT hres;
    IEnumBackgroundCopyFiles *enumFiles;
    ULONG res;

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                      test_localPathA);
    if (hres != S_OK)
    {
        skip("Unable to add file to job\n");
        return;
    }

    hres = IBackgroundCopyJob_EnumFiles(test_job, &enumFiles);
    ok(hres == S_OK, "EnumFiles failed: 0x%08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to create file enumerator.\n");
        return;
    }

    res = IEnumBackgroundCopyFiles_Release(enumFiles);
    ok(res == 0, "Bad ref count on release: %u\n", res);
}

/* Test getting job progress */
static void test_GetProgress_preTransfer(void)
{
    HRESULT hres;
    BG_JOB_PROGRESS progress;

    hres = IBackgroundCopyJob_GetProgress(test_job, &progress);
    ok(hres == S_OK, "GetProgress failed: 0x%08x\n", hres);
    if (hres != S_OK)
    {
        skip("Unable to get job progress\n");
        teardown();
        return;
    }

    ok(progress.BytesTotal == 0, "Incorrect BytesTotal: %x%08x\n",
       (DWORD)(progress.BytesTotal >> 32), (DWORD)progress.BytesTotal);
    ok(progress.BytesTransferred == 0, "Incorrect BytesTransferred: %x%08x\n",
       (DWORD)(progress.BytesTransferred >> 32), (DWORD)progress.BytesTransferred);
    ok(progress.FilesTotal == 0, "Incorrect FilesTotal: %u\n", progress.FilesTotal);
    ok(progress.FilesTransferred == 0, "Incorrect FilesTransferred %u\n", progress.FilesTransferred);
}

/* Test getting job state */
static void test_GetState(void)
{
    HRESULT hres;
    BG_JOB_STATE state;

    state = BG_JOB_STATE_ERROR;
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "GetState failed: 0x%08x\n", hres);
    if (hres != S_OK)
    {
        skip("Unable to get job state\n");
        return;
    }
    ok(state == BG_JOB_STATE_SUSPENDED, "Incorrect job state: %d\n", state);
}

/* Test resuming a job */
static void test_ResumeEmpty(void)
{
    HRESULT hres;
    BG_JOB_STATE state;

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == BG_E_EMPTY, "Resume failed to return BG_E_EMPTY error: 0x%08x\n", hres);
    if (hres != BG_E_EMPTY)
    {
        skip("Failed calling resume job\n");
        return;
    }

    state = BG_JOB_STATE_ERROR;
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    if (hres != S_OK)
    {
        skip("Unable to get job state\n");
        return;
    }
    ok(state == BG_JOB_STATE_SUSPENDED, "Incorrect job state: %d\n", state);
}

static void makeFile(WCHAR *name, const char *contents)
{
    HANDLE file;
    DWORD w, len = strlen(contents);

    DeleteFileW(name);
    file = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile\n");
    ok(WriteFile(file, contents, len, &w, NULL), "WriteFile\n");
    CloseHandle(file);
}

static void compareFiles(WCHAR *n1, WCHAR *n2)
{
    char b1[256];
    char b2[256];
    DWORD s1, s2;
    HANDLE f1, f2;

    f1 = CreateFileW(n1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    ok(f1 != INVALID_HANDLE_VALUE, "CreateFile\n");

    f2 = CreateFileW(n2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    ok(f2 != INVALID_HANDLE_VALUE, "CreateFile\n");

    /* Neither of these files is very big */
    ok(ReadFile(f1, b1, sizeof b1, &s1, NULL), "ReadFile\n");
    ok(ReadFile(f2, b2, sizeof b2, &s2, NULL), "ReadFile\n");

    CloseHandle(f1);
    CloseHandle(f2);

    ok(s1 == s2, "Files differ in length\n");
    ok(memcmp(b1, b2, s1) == 0, "Files differ in contents\n");
}

/* Test a complete transfer for local files */
static void test_CompleteLocal(void)
{
    static const int timeout_sec = 30;
    HRESULT hres;
    BG_JOB_STATE state;
    int i;

    DeleteFileW(test_localPathA);
    DeleteFileW(test_localPathB);
    makeFile(test_remotePathA, "This is a WINE test file for BITS\n");
    makeFile(test_remotePathB, "This is another WINE test file for BITS\n");

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                      test_localPathA);
    if (hres != S_OK)
    {
        skip("Unable to add file to job\n");
        return;
    }

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathB,
                                      test_localPathB);
    if (hres != S_OK)
    {
        skip("Unable to add file to job\n");
        return;
    }

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");

    for (i = 0; i < timeout_sec; ++i)
    {
        hres = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
        ok(state == BG_JOB_STATE_QUEUED || state == BG_JOB_STATE_CONNECTING
           || state == BG_JOB_STATE_TRANSFERRING || state == BG_JOB_STATE_TRANSFERRED,
           "Bad state: %d\n", state);
        if (state == BG_JOB_STATE_TRANSFERRED)
            break;
        Sleep(1000);
    }

    ok(i < timeout_sec, "BITS jobs timed out\n");
    hres = IBackgroundCopyJob_Complete(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Complete\n");
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
    ok(state == BG_JOB_STATE_ACKNOWLEDGED, "Bad state: %d\n", state);

    compareFiles(test_remotePathA, test_localPathA);
    compareFiles(test_remotePathB, test_localPathB);

    ok(DeleteFileW(test_remotePathA), "DeleteFile\n");
    ok(DeleteFileW(test_remotePathB), "DeleteFile\n");
    DeleteFileW(test_localPathA);
    DeleteFileW(test_localPathB);
}

/* Test a complete transfer for local files */
static void test_CompleteLocalURL(void)
{
    static const WCHAR prot[] = {'f','i','l','e',':','/','/', 0};
    static const int timeout_sec = 30;
    WCHAR *urlA, *urlB;
    HRESULT hres;
    BG_JOB_STATE state;
    int i;

    DeleteFileW(test_localPathA);
    DeleteFileW(test_localPathB);
    makeFile(test_remotePathA, "This is a WINE test file for BITS\n");
    makeFile(test_remotePathB, "This is another WINE test file for BITS\n");

    urlA = HeapAlloc(GetProcessHeap(), 0,
                     (7 + lstrlenW(test_remotePathA) + 1) * sizeof urlA[0]);
    urlB = HeapAlloc(GetProcessHeap(), 0,
                     (7 + lstrlenW(test_remotePathB) + 1) * sizeof urlB[0]);
    if (!urlA || !urlB)
    {
        skip("Unable to allocate memory for URLs\n");
        HeapFree(GetProcessHeap(), 0, urlA);
        HeapFree(GetProcessHeap(), 0, urlB);
        return;
    }

    lstrcpyW(urlA, prot);
    lstrcatW(urlA, test_remotePathA);
    lstrcpyW(urlB, prot);
    lstrcatW(urlB, test_remotePathB);

    hres = IBackgroundCopyJob_AddFile(test_job, urlA, test_localPathA);
    if (hres != S_OK)
    {
        skip("Unable to add file to job\n");
        HeapFree(GetProcessHeap(), 0, urlA);
        HeapFree(GetProcessHeap(), 0, urlB);
        return;
    }

    hres = IBackgroundCopyJob_AddFile(test_job, urlB, test_localPathB);
    if (hres != S_OK)
    {
        skip("Unable to add file to job\n");
        HeapFree(GetProcessHeap(), 0, urlA);
        HeapFree(GetProcessHeap(), 0, urlB);
        return;
    }

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");

    for (i = 0; i < timeout_sec; ++i)
    {
        hres = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
        ok(state == BG_JOB_STATE_QUEUED || state == BG_JOB_STATE_CONNECTING
           || state == BG_JOB_STATE_TRANSFERRING || state == BG_JOB_STATE_TRANSFERRED,
           "Bad state: %d\n", state);
        if (state == BG_JOB_STATE_TRANSFERRED)
            break;
        Sleep(1000);
    }

    ok(i < timeout_sec, "BITS jobs timed out\n");
    hres = IBackgroundCopyJob_Complete(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Complete\n");
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
    ok(state == BG_JOB_STATE_ACKNOWLEDGED, "Bad state: %d\n", state);

    compareFiles(test_remotePathA, test_localPathA);
    compareFiles(test_remotePathB, test_localPathB);

    ok(DeleteFileW(test_remotePathA), "DeleteFile\n");
    ok(DeleteFileW(test_remotePathB), "DeleteFile\n");
    DeleteFileW(test_localPathA);
    DeleteFileW(test_localPathB);

    HeapFree(GetProcessHeap(), 0, urlA);
    HeapFree(GetProcessHeap(), 0, urlB);
}

typedef void (*test_t)(void);

START_TEST(job)
{
    static const test_t tests[] = {
        test_GetId,
        test_GetType,
        test_GetName,
        test_GetProgress_preTransfer,
        test_GetState,
        test_ResumeEmpty,
        0
    };
    static const test_t tests_bits20[] = {
        test_AddFile,
        test_AddFileSet,
        test_EnumFiles,
        test_CompleteLocal,
        test_CompleteLocalURL,
        0
    };
    const test_t *test;

    if (!init_paths())
        return;

    CoInitialize(NULL);

    for (test = tests; *test; ++test)
    {
        /* Keep state separate between tests. */
        if (!setup())
        {
            skip("Unable to setup test\n");
            break;
        }
        (*test)();
        teardown();
    }

    if (check_bits20())
    {
        for (test = tests_bits20; *test; ++test)
        {
            /* Keep state separate between tests. */
            if (!setup())
            {
                skip("Unable to setup test\n");
                break;
            }
            (*test)();
            teardown();
        }
    }
    else
    {
        win_skip("Tests need BITS 2.0 or higher\n");
    }

    CoUninitialize();
}
