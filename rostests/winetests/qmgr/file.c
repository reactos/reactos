/*
 * Unit test suite for Background Copy File Interface
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

#include <shlwapi.h>

#define COBJMACROS

#include "wine/test.h"
#include "bits.h"

/* Globals used by many tests */
#define NUM_FILES 1
static const WCHAR test_remoteName[] = {'r','e','m','o','t','e', 0};
static const WCHAR test_localName[] = {'l','o','c','a','l', 0};
static WCHAR test_localFile[MAX_PATH];
static  WCHAR test_remoteUrl[MAX_PATH];
static const ULONG test_fileCount = NUM_FILES;
static const WCHAR test_displayName[] = {'T','e','s','t', 0};
static IBackgroundCopyJob *test_job;
static IBackgroundCopyManager *test_manager;
static IEnumBackgroundCopyFiles *test_enumFiles;
static IBackgroundCopyFile *test_file;

/* Helper function to add a file to a job.  The helper function takes base
   file name and creates properly formed path and URL strings for creation of
   the file. */
static HRESULT addFileHelper(IBackgroundCopyJob* job,
        const WCHAR *localName, const WCHAR *remoteName)
{
    DWORD urlSize;

    GetCurrentDirectoryW(MAX_PATH, test_localFile);
    PathAppendW(test_localFile, localName);
    GetCurrentDirectoryW(MAX_PATH, test_remoteUrl);
    PathAppendW(test_remoteUrl, remoteName);
    urlSize = MAX_PATH;
    UrlCreateFromPathW(test_remoteUrl, test_remoteUrl, &urlSize, 0);
    UrlUnescapeW(test_remoteUrl, NULL, &urlSize, URL_UNESCAPE_INPLACE);
    return IBackgroundCopyJob_AddFile(test_job, test_remoteUrl, test_localFile);
}

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;
    GUID test_jobId;

    test_manager = NULL;
    test_job = NULL;
    memset(&test_jobId, 0, sizeof test_jobId);

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayName,
            BG_JOB_TYPE_DOWNLOAD, &test_jobId, &test_job);
    if(hres != S_OK)
    {
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    if (addFileHelper(test_job, test_localName, test_remoteName) != S_OK
        || IBackgroundCopyJob_EnumFiles(test_job, &test_enumFiles) != S_OK)
    {
        IBackgroundCopyJob_Release(test_job);
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    hres = IEnumBackgroundCopyFiles_Next(test_enumFiles, 1, &test_file, NULL);
    if(hres != S_OK)
    {
        IEnumBackgroundCopyFiles_Release(test_enumFiles);
        IBackgroundCopyJob_Release(test_job);
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    IBackgroundCopyFile_Release(test_file);
    IEnumBackgroundCopyFiles_Release(test_enumFiles);
    IBackgroundCopyJob_Release(test_job);
    IBackgroundCopyManager_Release(test_manager);
}

/* Test that the remote name is properly set */
static void test_GetRemoteName(void)
{
    HRESULT hres;
    LPWSTR name;

    hres = IBackgroundCopyFile_GetRemoteName(test_file, &name);
    ok(hres == S_OK, "GetRemoteName failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get remote name of test_file.\n");
        return;
    }
    ok(lstrcmpW(name, test_remoteUrl) == 0, "Got incorrect remote name\n");
    CoTaskMemFree(name);
}

/* Test that the local name is properly set */
static void test_GetLocalName(void)
{
    HRESULT hres;
    LPWSTR name;

    hres = IBackgroundCopyFile_GetLocalName(test_file, &name);
    ok(hres == S_OK, "GetLocalName failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get local name of test_file.\n");
        return;
    }
    ok(lstrcmpW(name, test_localFile) == 0, "Got incorrect local name\n");
    CoTaskMemFree(name);
}

/* Test getting the progress of a file*/
static void test_GetProgress_PreTransfer(void)
{
    HRESULT hres;
    BG_FILE_PROGRESS progress;

    hres = IBackgroundCopyFile_GetProgress(test_file, &progress);
    ok(hres == S_OK, "GetProgress failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get progress of test_file.\n");
        return;
    }
    ok(progress.BytesTotal == BG_SIZE_UNKNOWN, "Got incorrect total size: %x%08x\n",
       (DWORD)(progress.BytesTotal >> 32), (DWORD)progress.BytesTotal);
    ok(progress.BytesTransferred == 0, "Got incorrect number of transferred bytes: %x%08x\n",
       (DWORD)(progress.BytesTransferred >> 32), (DWORD)progress.BytesTransferred);
    ok(progress.Completed == FALSE, "Got incorret completion status\n");
}

typedef void (*test_t)(void);

START_TEST(file)
{
    static const test_t tests[] = {
        test_GetRemoteName,
        test_GetLocalName,
        test_GetProgress_PreTransfer,
        0
    };
    const test_t *test;

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
    CoUninitialize();
}
