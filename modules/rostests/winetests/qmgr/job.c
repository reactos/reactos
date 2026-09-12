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
#include "bits2_0.h"
#include "bits2_5.h"
#include "bits5_0.h"

/* Globals used by many tests */
static WCHAR test_remotePathA[MAX_PATH];
static WCHAR test_remotePathB[MAX_PATH];
static WCHAR test_localPathA[MAX_PATH];
static WCHAR test_localPathB[MAX_PATH];
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_job;
static GUID test_jobId;
static BG_JOB_TYPE test_type;

typedef struct IBackgroundCopyCallback2Impl {
    IBackgroundCopyCallback2 IBackgroundCopyCallback2_iface;
    LONG ref;
} IBackgroundCopyCallback2Impl;

static inline IBackgroundCopyCallback2Impl *impl_from_IBackgroundCopyCallback2(IBackgroundCopyCallback2 *iface)
{
    return CONTAINING_RECORD(iface, IBackgroundCopyCallback2Impl, IBackgroundCopyCallback2_iface);
}

static HRESULT WINAPI IBackgroundCopyCallback2Impl_QueryInterface(IBackgroundCopyCallback2 *iface, REFIID riid, void **ppv)
{
    IBackgroundCopyCallback2Impl *This = impl_from_IBackgroundCopyCallback2(iface);

    if (!ppv)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IBackgroundCopyCallback) ||
        IsEqualIID(riid, &IID_IBackgroundCopyCallback2))
    {
        *ppv = &This->IBackgroundCopyCallback2_iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IBackgroundCopyCallback2Impl_AddRef(IBackgroundCopyCallback2 *iface)
{
    IBackgroundCopyCallback2Impl *This = impl_from_IBackgroundCopyCallback2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    trace("IBackgroundCopyCallback2Impl_AddRef called (%p, ref = %ld)\n", This, ref);
    return ref;
}

static ULONG WINAPI IBackgroundCopyCallback2Impl_Release(IBackgroundCopyCallback2 *iface)
{
    IBackgroundCopyCallback2Impl *This = impl_from_IBackgroundCopyCallback2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    trace("IBackgroundCopyCallback2Impl_Release called (%p, ref = %ld)\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IBackgroundCopyCallback2Impl_JobError(IBackgroundCopyCallback2 *iface, IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    trace("IBackgroundCopyCallback2Impl_JobError called (%p, %p, %p)\n", iface, pJob, pError);
    return S_OK;
}

static HRESULT WINAPI IBackgroundCopyCallback2Impl_JobModification(IBackgroundCopyCallback2 *iface, IBackgroundCopyJob *pJob, DWORD dwReserved)
{
    trace("IBackgroundCopyCallback2Impl_JobModification called (%p, %p)\n", iface, pJob);
    return S_OK;
}

static HRESULT WINAPI IBackgroundCopyCallback2Impl_JobTransferred(IBackgroundCopyCallback2 *iface, IBackgroundCopyJob *pJob)
{
    trace("IBackgroundCopyCallback2Impl_JobTransferred called (%p, %p)\n", iface, pJob);
    return S_OK;
}

static HRESULT WINAPI IBackgroundCopyCallback2Impl_FileTransferred(IBackgroundCopyCallback2 *iface, IBackgroundCopyJob *pJob, IBackgroundCopyFile *pFile)
{
    trace("IBackgroundCopyCallback2Impl_FileTransferred called (%p, %p, %p)\n", iface, pJob, pFile);
    return S_OK;
}


static const IBackgroundCopyCallback2Vtbl copyCallback_vtbl =
{
    IBackgroundCopyCallback2Impl_QueryInterface,
    IBackgroundCopyCallback2Impl_AddRef,
    IBackgroundCopyCallback2Impl_Release,
    IBackgroundCopyCallback2Impl_JobTransferred,
    IBackgroundCopyCallback2Impl_JobError,
    IBackgroundCopyCallback2Impl_JobModification,
    IBackgroundCopyCallback2Impl_FileTransferred
};

static BOOL create_background_copy_callback2(IBackgroundCopyCallback2 **copyCallback)
{
    IBackgroundCopyCallback2Impl *obj;
    *copyCallback = NULL;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));

    if (!obj)
    {
        trace("Out of memory creating IBackgroundCopyCallback2\n");
        return FALSE;
    }

    obj->IBackgroundCopyCallback2_iface.lpVtbl = &copyCallback_vtbl;
    obj->ref = 1;

    *copyCallback = &obj->IBackgroundCopyCallback2_iface;

    return TRUE;
}

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
    WCHAR prefix[] = L"qmgr";

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
    IBackgroundCopyJob5* test_job_5;
    BITS_JOB_PROPERTY_VALUE prop_val;

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

    hres = IBackgroundCopyManager_CreateJob(test_manager, L"Test", test_type, &test_jobId, &test_job);
    if(hres != S_OK)
    {
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    /* The Wine TestBot Windows 10 VMs disable Windows Update by putting
       the network connection in metered mode.

       Unfortunately, this will make BITS jobs fail, since the default transfer policy
       on Windows 10 prevents BITs job from running over a metered network

       To allow these tests in this file to run on the testbot, we
       set the BITS_JOB_PROPERTY_ID_COST_FLAGS property to BITS_COST_STATE_TRANSFER_ALWAYS,
       ensuring that BITS will still try to run the job on a metered network */
    prop_val.Dword = BITS_COST_STATE_TRANSFER_ALWAYS;
    hres = IBackgroundCopyJob_QueryInterface(test_job, &IID_IBackgroundCopyJob5, (void **)&test_job_5);
    /* BackgroundCopyJob5 was added in Windows 8, so this may not exist. The metered connection
       workaround is only applied on Windows 10, so it's fine if this fails. */
    if (SUCCEEDED(hres)) {
        hres = IBackgroundCopyJob5_SetProperty(test_job_5, BITS_JOB_PROPERTY_ID_COST_FLAGS, prop_val);
        ok(hres == S_OK, "Failed to set the cost flags: %08lx\n", hres);
        IBackgroundCopyJob5_Release(test_job_5);
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

    hres = IBackgroundCopyManager_CreateJob(manager, L"Test", test_type, &test_jobId, &job);
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

    hres = IBackgroundCopyManager_CreateJob(manager, L"Test", test_type, &test_jobId, &job);
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
    ok(hres == S_OK, "GetId failed: %08lx\n", hres);
    ok(memcmp(&tmpId, &test_jobId, sizeof tmpId) == 0, "Got incorrect GUID\n");
}

/* Test that the type is properly set */
static void test_GetType(void)
{
    HRESULT hres;
    BG_JOB_TYPE type;

    hres = IBackgroundCopyJob_GetType(test_job, &type);
    ok(hres == S_OK, "GetType failed: %08lx\n", hres);
    ok(type == test_type, "Got incorrect type\n");
}

/* Test that the display name is properly set */
static void test_GetName(void)
{
    HRESULT hres;
    LPWSTR displayName;

    hres = IBackgroundCopyJob_GetDisplayName(test_job, &displayName);
    ok(hres == S_OK, "GetName failed: %08lx\n", hres);
    ok(lstrcmpW(displayName, L"Test") == 0, "Got incorrect type\n");
    CoTaskMemFree(displayName);
}

/* Test adding a file */
static void test_AddFile(void)
{
    HRESULT hres;

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                      test_localPathA);
    ok(hres == S_OK, "First call to AddFile failed: 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathB,
                                      test_localPathB);
    ok(hres == S_OK, "Second call to AddFile failed: 0x%08lx\n", hres);
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
    ok(hres == S_OK, "AddFileSet failed: 0x%08lx\n", hres);
}

/* Test creation of a job enumerator */
static void test_EnumFiles(void)
{
    HRESULT hres;
    IEnumBackgroundCopyFiles *enumFiles;
    ULONG res;

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathA,
                                      test_localPathA);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_EnumFiles(test_job, &enumFiles);
    ok(hres == S_OK, "EnumFiles failed: 0x%08lx\n", hres);

    res = IEnumBackgroundCopyFiles_Release(enumFiles);
    ok(res == 0, "Bad ref count on release: %lu\n", res);
}

/* Test getting job progress */
static void test_GetProgress_preTransfer(void)
{
    HRESULT hres;
    BG_JOB_PROGRESS progress;

    hres = IBackgroundCopyJob_GetProgress(test_job, &progress);
    ok(hres == S_OK, "GetProgress failed: 0x%08lx\n", hres);

    ok(progress.BytesTotal == 0, "Incorrect BytesTotal: %s\n",
       wine_dbgstr_longlong(progress.BytesTotal));
    ok(progress.BytesTransferred == 0, "Incorrect BytesTransferred: %s\n",
       wine_dbgstr_longlong(progress.BytesTransferred));
    ok(progress.FilesTotal == 0, "Incorrect FilesTotal: %lu\n", progress.FilesTotal);
    ok(progress.FilesTransferred == 0, "Incorrect FilesTransferred %lu\n", progress.FilesTransferred);
}

/* Test getting job state */
static void test_GetState(void)
{
    HRESULT hres;
    BG_JOB_STATE state;

    state = BG_JOB_STATE_ERROR;
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "GetState failed: 0x%08lx\n", hres);
    ok(state == BG_JOB_STATE_SUSPENDED, "Incorrect job state: %d\n", state);
}

/* Test resuming a job */
static void test_ResumeEmpty(void)
{
    HRESULT hres;
    BG_JOB_STATE state;

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == BG_E_EMPTY, "Resume failed to return BG_E_EMPTY error: 0x%08lx\n", hres);

    state = BG_JOB_STATE_ERROR;
    hres = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
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

/* Handles a timeout in the BG_JOB_STATE_ERROR or BG_JOB_STATE_TRANSIENT_ERROR state */
static void handle_job_err(void)
{
    HRESULT hres;
    IBackgroundCopyError *err;
    BG_ERROR_CONTEXT errContext;
    HRESULT errCode;
    LPWSTR contextDesc;
    LPWSTR errDesc;

    hres = IBackgroundCopyJob_GetError(test_job, &err);
    if (SUCCEEDED(hres)) {
        hres = IBackgroundCopyError_GetError(err, &errContext, &errCode);
        if (SUCCEEDED(hres)) {
            ok(0, "Got context: %d code: %ld\n", errContext, errCode);
        } else {
            ok(0, "Failed to get error info: 0x%08lx\n", hres);
        }

        hres = IBackgroundCopyError_GetErrorContextDescription(err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), &contextDesc);
        if (SUCCEEDED(hres)) {
            ok(0, "Got context desc: %s\n", wine_dbgstr_w(contextDesc));
        } else {
            ok(0, "Failed to get context desc: 0x%08lx\n", hres);
        }

        hres = IBackgroundCopyError_GetErrorDescription(err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), &errDesc);
        if (SUCCEEDED(hres)) {
            ok(0, "Got error desc: %s\n", wine_dbgstr_w(errDesc));
        } else {
            ok(0, "Failed to get error desc: 0x%08lx\n", hres);
        }
    } else {
        ok(0, "Failed to get error: 0x%08lx\n", hres);
    }
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
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_AddFile(test_job, test_remotePathB,
                                      test_localPathB);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");

    for (i = 0; i < timeout_sec; ++i)
    {
        hres = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
        ok(state == BG_JOB_STATE_QUEUED || state == BG_JOB_STATE_CONNECTING
           || state == BG_JOB_STATE_TRANSFERRING || state == BG_JOB_STATE_TRANSFERRED
           || state == BG_JOB_STATE_TRANSIENT_ERROR,
           "Bad state: %d\n", state);

        if (state == BG_JOB_STATE_TRANSIENT_ERROR) {
            hres = IBackgroundCopyJob_Resume(test_job);
            ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");
        }

        if (state == BG_JOB_STATE_TRANSFERRED)
            break;
        Sleep(1000);
    }

    if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSIENT_ERROR)
        handle_job_err();

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

    lstrcpyW(urlA, L"file://");
    lstrcatW(urlA, test_remotePathA);
    lstrcpyW(urlB, L"file://");
    lstrcatW(urlB, test_remotePathB);

    hres = IBackgroundCopyJob_AddFile(test_job, urlA, test_localPathA);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_AddFile(test_job, urlB, test_localPathB);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IBackgroundCopyJob_Resume(test_job);
    ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");

    for (i = 0; i < timeout_sec; ++i)
    {
        hres = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hres == S_OK, "IBackgroundCopyJob_GetState\n");
        ok(state == BG_JOB_STATE_QUEUED || state == BG_JOB_STATE_CONNECTING
           || state == BG_JOB_STATE_TRANSFERRING || state == BG_JOB_STATE_TRANSFERRED
           || state == BG_JOB_STATE_TRANSIENT_ERROR,
           "Bad state: %d\n", state);

        if (state == BG_JOB_STATE_TRANSIENT_ERROR) {
            hres = IBackgroundCopyJob_Resume(test_job);
            ok(hres == S_OK, "IBackgroundCopyJob_Resume\n");
        }

        if (state == BG_JOB_STATE_TRANSFERRED)
            break;
        Sleep(1000);
    }

    if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSIENT_ERROR)
        handle_job_err();


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
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(flags == (BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_TRANSFERRED), "flags 0x%08lx\n", flags);
}

static void test_NotifyInterface(void)
{
    HRESULT hr;
    IUnknown *unk;

    unk = (IUnknown*)0xdeadbeef;
    hr = IBackgroundCopyJob_GetNotifyInterface(test_job, &unk);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(unk == NULL, "got %p\n", unk);
}

static void test_Cancel(void)
{
    HRESULT hr;
    BG_JOB_STATE state;

    state = BG_JOB_STATE_ERROR;
    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state != BG_JOB_STATE_CANCELLED, "got %u\n", state);

    hr = IBackgroundCopyJob_Cancel(test_job);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    state = BG_JOB_STATE_ERROR;
    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == BG_JOB_STATE_CANCELLED, "got %u\n", state);

    hr = IBackgroundCopyJob_Cancel(test_job);
    ok(hr == BG_E_INVALID_STATE, "got 0x%08lx\n", hr);
}

static void test_HttpOptions(void)
{
    static const WCHAR winetestW[] = L"Wine: test\r\n";
    static const unsigned int timeout = 30;
    HRESULT hr;
    IBackgroundCopyJobHttpOptions *options;
    IBackgroundCopyError *error;
    BG_JOB_STATE state;
    unsigned int i;
    WCHAR *headers;
    ULONG flags, orig_flags;
    IBackgroundCopyCallback2 *copyCallback;
    IUnknown *copyCallbackUnknown;

    ok(create_background_copy_callback2(&copyCallback) == TRUE, "create_background_copy_callback2 failed\n");

    hr = IBackgroundCopyCallback2_QueryInterface(copyCallback, &IID_IUnknown, (LPVOID*)&copyCallbackUnknown);
    ok(hr == S_OK,"IBackgroundCopyCallback_QueryInterface(IID_IUnknown) failed: %08lx\n", hr);

    hr = IBackgroundCopyJob_SetNotifyInterface(test_job, copyCallbackUnknown);
    ok(hr == S_OK,"IBackgroundCopyCallback_SetNotifyInterface failed: %08lx\n", hr);

    hr = IBackgroundCopyJob_SetNotifyFlags(test_job, BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_DISABLE | BG_NOTIFY_JOB_MODIFICATION | BG_NOTIFY_FILE_TRANSFERRED);
    ok(hr == S_OK,"IBackgroundCopyCallback_SetNotifyFlags failed: %08lx\n", hr);


    DeleteFileW(test_localPathA);
    hr = IBackgroundCopyJob_AddFile(test_job, L"http://test.winehq.org/", test_localPathA);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IBackgroundCopyJob_QueryInterface(test_job, &IID_IBackgroundCopyJobHttpOptions, (void **)&options);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (options)
    {
        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08lx\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, winetestW);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        if (hr == S_OK)
        {
            ok(!lstrcmpW(headers, winetestW), "got %s\n", wine_dbgstr_w(headers));
            CoTaskMemFree(headers);
        }

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, NULL);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08lx\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        orig_flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &orig_flags);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!orig_flags, "got 0x%08lx\n", orig_flags);

        hr = IBackgroundCopyJobHttpOptions_SetSecurityFlags(options, 0);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &flags);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!flags, "got 0x%08lx\n", flags);
    }

    hr = IBackgroundCopyJob_Resume(test_job);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    for (i = 0; i < timeout; i++)
    {
        hr = IBackgroundCopyJob_GetState(test_job, &state);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        ok(state == BG_JOB_STATE_QUEUED ||
           state == BG_JOB_STATE_CONNECTING ||
           state == BG_JOB_STATE_TRANSFERRING ||
           state == BG_JOB_STATE_TRANSFERRED ||
           state == BG_JOB_STATE_TRANSIENT_ERROR, "unexpected state: %u\n", state);

        if (state == BG_JOB_STATE_TRANSIENT_ERROR) {
            hr = IBackgroundCopyJob_Resume(test_job);
            ok(hr == S_OK, "IBackgroundCopyJob_Resume\n");
        }

        if (state == BG_JOB_STATE_TRANSFERRED) break;
        Sleep(1000);
    }

    if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSIENT_ERROR)
        handle_job_err();


    ok(i < timeout, "BITS job timed out\n");
    if (i < timeout)
    {
        hr = IBackgroundCopyJob_GetError(test_job, &error);
        ok(hr == BG_E_ERROR_INFORMATION_UNAVAILABLE, "got 0x%08lx\n", hr);
    }

    if (options)
    {
        headers = (WCHAR *)0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08lx\n", hr);
        ok(headers == NULL, "got %p\n", headers);

        hr = IBackgroundCopyJobHttpOptions_SetCustomHeaders(options, NULL);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IBackgroundCopyJobHttpOptions_GetCustomHeaders(options, &headers);
        ok(hr == S_FALSE, "got 0x%08lx\n", hr);

        flags = 0xdeadbeef;
        hr = IBackgroundCopyJobHttpOptions_GetSecurityFlags(options, &flags);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!flags, "got 0x%08lx\n", flags);

        hr = IBackgroundCopyJobHttpOptions_SetSecurityFlags(options, orig_flags);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        IBackgroundCopyJobHttpOptions_Release(options);
    }

    hr = IBackgroundCopyJob_Complete(test_job);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IBackgroundCopyJob_GetState(test_job, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == BG_JOB_STATE_ACKNOWLEDGED, "unexpected state: %u\n", state);

    hr = IBackgroundCopyJob_Complete(test_job);
    ok(hr == BG_E_INVALID_STATE, "got 0x%08lx\n", hr);

    DeleteFileW(test_localPathA);

    hr = IBackgroundCopyJob_SetNotifyInterface(test_job, NULL);
    ok(hr == BG_E_INVALID_STATE, "got 0x%08lx\n", hr);

    IUnknown_Release(copyCallbackUnknown);
    IBackgroundCopyCallback2_Release(copyCallback);
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
    HRESULT hres;

    init_paths();

    /* CoInitializeEx and CoInitializeSecurity with RPC_C_IMP_LEVEL_IMPERSONATE
     * are required to set the job transfer policy
     */
    hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hres)) {
        ok(0, "CoInitializeEx failed: %0lx\n", hres);
        return;
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
                           RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE, 0);
    if (FAILED(hres)) {
        ok(0, "CoInitializeSecurity failed: %0lx\n", hres);
        return;
    }

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
