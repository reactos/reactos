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

//#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <wine/test.h>
#include <objbase.h>
#include <bits.h>
#include <initguid.h>
#include <bits2_0.h>
#include <bits2_5.h>

/* Globals used by many tests */
static const WCHAR test_displayName[] = {'T', 'e', 's', 't', 0};
static WCHAR test_remotePathA[MAX_PATH];
static WCHAR test_remotePathB[MAX_PATH];
static WCHAR test_localPathA[MAX_PATH];
static WCHAR test_localPathB[MAX_PATH];
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_job;
static GUID test_jobId;
static BG_JOB_TYPE test_type;

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

static void init_paths(void)
{
    WCHAR tmpDir[MAX_PATH];
    WCHAR prefix[] = {'q', 'm', 'g', 'r', 0};

    GetTempPathW(MAX_PATH, tmpDir);

    GetTempFileNameW(tmpDir, prefix, 0, test_localPathA);
    GetTempFileNameW(tmpDir, prefix, 0, test_localPathB);
    GetTempFileNameW(tmpDir, prefix, 0, test_remotePathA);
    GetTempFileNameW(tmpDir, prefix, 0, test_remotePathB);
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
    IBackgroundCopyJob_Cancel(test_job);
    IBackgroundCopyJob_Release(test_job);
    IBackgroundCopyManager_Release(test_manager);
}

static BOOL check_bits20(void)
{
    HRESULT hres;
    IBackgroundCopyManager *manager;
    IBackgroundCopyJob *job, *job3;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **)&manager);
    if (hres != S_OK) return FALSE;

    hres = IBackgroundCopyManager_CreateJob(manager, test_displayName, test_type, &test_jobId, &job);
    if (hres != S_OK)
    {
        IBackgroundCopyManager_Release(manager);
        return FALSE;
    }

    hres = IBackgroundCopyJob_QueryInterface(job, &IID_IBackgroundCopyJob3, (void **)&job3);
    IBackgroundCopyJob_Cancel(job);
    IBackgroundCopyJob_Release(job);
    if (hres != S_OK)
    {
        IBackgroundCopyManager_Release(manager);
        return FALSE;
    }

    IBackgroundCopyJob_Release(job3);
    IBackgroundCopyManager_Release(manager);
    return TRUE;
}

static BOOL check_bits25(void)
{
    HRESULT hres;
    IBackgroundCopyManager *manager;
    IBackgroundCopyJob *job;
    IBackgroundCopyJobHttpOptions *options;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **)&manager);
    if (hres != S_OK) return FALSE;

    hres = IBackgroundCopyManager_CreateJob(manager, test_displayName, test_type, &test_jobId, &job);
    if (hres != S_OK)
    {
        IBackgroundCopyManager_Release(manager);
        return FALSE;
    }

    hres = IBackgroundCopyJob_QueryInterface(job, &IID_IBackgroundCopyJobHttpOptions, (void **)&options);
    IBackgroundCopyJob_Cancel(job);
    IBackgroundCopyJob_Release(job);
    if (hres != S_OK)
    {
        IBackgroundCopyManager_Release(manager);
        return FALSE;
    }

    IBackgroundCopyJobHttpOptions_Release(options);
    IBackgroundCopyManager_Release(manager);
    return TRUE;
}

/* Test that the jobId is properly set */
static void test_GetId(void)
{
    HRESULT hres;
    GUID tmpId;

    hres = IBackgroundCopyJob_GetId(test_job, &tmpId);
    ok(hres == S_OK, "GetId failed: %08x\n", hres);
    ok(memcmp(&tmpId, &test_jobId, sizeof tmpId) == 0, "Got incorrect GUID\n");
}

/* Test that the type is properly set */
static void test_GetType(void)
{
    HRESULT hres;
    BG_JOB_TYPE type;

    hres = IBackgroundCopyJob_GetType(test_job, &type);
    ok(hres == S_OK, "GetType failed: %08x\n", hres);
    ok(type == test_type, "Got incorrect type\n");
}

/* Test that the display name is properly set */
static void test_GetName(void)
{
    HRESULT hres;
    LPWSTR displayName;

    hres = IBackgroundCopyJob_GetDisplayName(test_job, &displayName);
    ok(hres == S_OK, "GetName failed: %08x\n", hres);
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
    ok(hres == S_OK, "got 0x%08x\n", hres);

    hres = IBackgroundCopyJob_EnumFiles(test_job, &enumFiles);
    ok(hres == S_OK, "EnumFiles failed: 0x%08x\n", hres);

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
    ok(state == BG_JOB_STATE_SUSPENDED, "Incorrect job state: %d\n", state);
}

/* Test resuming a job */
static void test_ResumeEmpty(void)
{
    HRESULT hres;
    BG_JOB_STATE state;

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == BG_E_EMPTY, "Resume failed to return BG_E_EMPTY error: 0x%08x\n", hres);

    state = BG_JOB_STATE_ERROR;
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "got 0x%08x\n", hres);
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
    ok(hres == S_OK, "got 0x%08x\n", hres);

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathB,
                                      test_localPathB);
    ok(hres == S_OK, "got 0x%08x\n", hres);

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
    ok(hres == S_OK, "got 0x%08x\n", hres);

    hres = IBackgroundCopyJob_AddFile(test_job, urlB, test_localPathB);
    ok(hres == S_OK, "got 0x%08x\n", hres);

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

static void test_NotifyFlags(void)
{
    ULONG flags;
    HRESULT hr;

    /* check default flags */
    flags = 0;
    hr = IBackgroundCopyJob_GetNotifyFlags(test_job, &flags);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(flags == (BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_TRANSFERRED), "flags 0x%08x\n", flags);
}

static void test_NotifyInterface(void)
{
    HRESULT hr;
    IUnknown *unk;

    unk = (IUnknown*)0xdeadbeef;
    hr = IBackgroundCopyJob_GetNotifyInterface(test_job, &unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(unk == NULL, "got %p\n", unk);
}

static void test_Cancel(void)
{
    HRESULT hr;
    BG_JOB_STATE state;

    state = BG_JOB_STATE_ERROR;
    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state != BG_JOB_STATE_CANCELLED, "got %u\n", state);

    hr = IBackgroundCopyJob_Cancel(test_job);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    state = BG_JOB_STATE_ERROR;
    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == BG_JOB_STATE_CANCELLED, "got %u\n", state);

    hr = IBackgroundCopyJob_Cancel(test_job);
    ok(hr == BG_E_INVALID_STATE, "got 0x%08x\n", hr);
}

static void test_HttpOptions(void)
{
    static const WCHAR urlW[] =
        {'h','t','t','p','s',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',0};
    static const WCHAR winetestW[] =
        {'W','i','n','e',':',' ','t','e','s','t','\r','\n',0};
    static const unsigned int timeout = 30;
    HRESULT hr;
    IBackgroundCopyJobHttpOptions *options;
    IBackgroundCopyError *error;
    BG_JOB_STATE state;
    unsigned int i;
    WCHAR *headers;
    ULONG flags, orig_flags;

    DeleteFileW(test_localPathA);
    hr = IBackgroundCopyJob_AddFile(test_job, urlW, test_localPathA);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IBackgroundCopyJob_QueryInterface(test_job, &IID_IBackgroundCopyJobHttpOptions, (void **)&options);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (options)
    {
        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08x\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, winetestW);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if (hr == S_OK)
        {
            ok(!lstrcmpW(headers, winetestW), "got %s\n", wine_dbgstr_w(headers));
            CoTaskMemFree(headers);
        }

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08x\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        orig_flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &orig_flags);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!orig_flags, "got 0x%08x\n", orig_flags);

        hr = IBackgroundCopyJobHttpOptions_SetSecurityFlags(options, 0);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &flags);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!flags, "got 0x%08x\n", flags);
    }

    hr = IBackgroundCopyJob_Resume(test_job);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    for (i = 0; i < timeout; i++)
    {
        hr = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        ok(state == BG_JOB_STATE_QUEUED ||
           state == BG_JOB_STATE_CONNECTING ||
           state == BG_JOB_STATE_TRANSFERRING ||
           state == BG_JOB_STATE_TRANSFERRED, "unexpected state: %u\n", state);

        if (state == BG_JOB_STATE_TRANSFERRED) break;
        Sleep(1000);
    }
    ok(i < timeout, "BITS job timed out\n");
    if (i < timeout)
    {
        hr = IBackgroundCopyJob_GetError(test_job, &error);
        ok(hr == BG_E_ERROR_INFORMATION_UNAVAILABLE, "got 0x%08x\n", hr);
    }

    if (options)
    {
        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08x\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08x\n", hr);

        flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &flags);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!flags, "got 0x%08x\n", flags);

        hr = IBackgroundCopyJobHttpOptions_SetSecurityFlags(options, orig_flags);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        IBackgroundCopyJobHttpOptions_Release(options);
    }

    hr = IBackgroundCopyJob_Complete(test_job);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == BG_JOB_STATE_ACKNOWLEDGED, "unexpected state: %u\n", state);

    hr = IBackgroundCopyJob_Complete(test_job);
    ok(hr == BG_E_INVALID_STATE, "got 0x%08x\n", hr);

    DeleteFileW(test_localPathA);
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
        test_NotifyFlags,
        test_NotifyInterface,
        0
    };
    static const test_t tests_bits20[] = {
        test_AddFile,
        test_AddFileSet,
        test_EnumFiles,
        test_CompleteLocal,
        test_CompleteLocalURL,
        test_Cancel, /* must be last */
        0
    };
    static const test_t tests_bits25[] = {
        test_HttpOptions,
        0
    };
    const test_t *test;
    int i;

    init_paths();

    CoInitialize(NULL);

    if (FAILED(test_create_manager()))
    {
        CoUninitialize();
        win_skip("Failed to create Manager instance, skipping tests\n");
        return;
    }

    for (test = tests, i = 0; *test; ++test, ++i)
    {
        /* Keep state separate between tests. */
        if (!setup())
        {
            ok(0, "tests:%d: Unable to setup test\n", i);
            break;
        }
        (*test)();
        teardown();
    }

    if (check_bits20())
    {
        for (test = tests_bits20, i = 0; *test; ++test, ++i)
        {
            /* Keep state separate between tests. */
            if (!setup())
            {
                ok(0, "tests_bits20:%d: Unable to setup test\n", i);
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

    if (check_bits25())
    {
        for (test = tests_bits25, i = 0; *test; ++test, ++i)
        {
            /* Keep state separate between tests. */
            if (!setup())
            {
                ok(0, "tests_bits25:%d: Unable to setup test\n", i);
                break;
            }
            (*test)();
            teardown();
        }
    }
    else
    {
        win_skip("Tests need BITS 2.5 or higher\n");
    }

    CoUninitialize();
}
