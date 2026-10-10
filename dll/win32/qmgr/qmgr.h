/*
 * Queue Manager definitions
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

#ifndef __QMGR_H__
#define __QMGR_H__

#include "windef.h"
#define COBJMACROS
#include "bits.h"
#include "bits1_5.h"
#include "bits2_0.h"
#include "bits2_5.h"
#include "bits3_0.h"

#include <string.h>
#include "wine/list.h"

/* Background copy job vtbl and related data */
typedef struct
{
    IBackgroundCopyJob4 IBackgroundCopyJob4_iface;
    IBackgroundCopyJobHttpOptions IBackgroundCopyJobHttpOptions_iface;
    LONG ref;
    LPWSTR displayName;
    LPWSTR description;
    BG_JOB_TYPE type;
    GUID jobId;
    struct list files;
    BG_JOB_PROGRESS jobProgress;
    BG_JOB_STATE state;
    ULONG notify_flags;
    IBackgroundCopyCallback2 *callback;
    BOOL callback2; /* IBackgroundCopyCallback2 is supported in addition to IBackgroundCopyCallback */
    /* Protects file list, and progress */
    CRITICAL_SECTION cs;
    struct list entryFromQmgr;
    struct
    {
        WCHAR              *headers;
        ULONG               flags;
        BG_AUTH_CREDENTIALS creds[BG_AUTH_TARGET_PROXY][BG_AUTH_SCHEME_PASSPORT];
    } http_options;
    struct
    {
        BG_ERROR_CONTEXT      context;
        HRESULT               code;
        IBackgroundCopyFile2 *file;
    } error;
    HANDLE wait;
    HANDLE cancel;
    HANDLE done;
} BackgroundCopyJobImpl;

/* Background copy file vtbl and related data */
typedef struct
{
    IBackgroundCopyFile2 IBackgroundCopyFile2_iface;
    LONG ref;
    BG_FILE_INFO info;
    BG_FILE_PROGRESS fileProgress;
    WCHAR tempFileName[MAX_PATH];
    struct list entryFromJob;
    BackgroundCopyJobImpl *owner;
    DWORD read_size;
} BackgroundCopyFileImpl;

/* Background copy manager vtbl and related data */
typedef struct
{
    IBackgroundCopyManager IBackgroundCopyManager_iface;
    /* Protects job list, job states, and jobEvent  */
    CRITICAL_SECTION cs;
    HANDLE jobEvent;
    struct list jobs;
} BackgroundCopyManagerImpl;

typedef struct
{
    IClassFactory IClassFactory_iface;
} ClassFactoryImpl;

extern HANDLE stop_event;
extern ClassFactoryImpl BITS_ClassFactory;
extern BackgroundCopyManagerImpl globalMgr;

HRESULT BackgroundCopyManagerConstructor(LPVOID *ppObj);
HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type,
                                     GUID *pJobId, BackgroundCopyJobImpl **job);
HRESULT enum_copy_job_create(BackgroundCopyManagerImpl *qmgr,
        IEnumBackgroundCopyJobs **enumjob);
HRESULT BackgroundCopyFileConstructor(BackgroundCopyJobImpl *owner,
                                      LPCWSTR remoteName, LPCWSTR localName,
                                      BackgroundCopyFileImpl **file);
HRESULT EnumBackgroundCopyFilesConstructor(BackgroundCopyJobImpl*, IEnumBackgroundCopyFiles**);
DWORD WINAPI fileTransfer(void *param);
void processJob(BackgroundCopyJobImpl *job);
BOOL processFile(BackgroundCopyFileImpl *file, BackgroundCopyJobImpl *job);
BOOL transitionJobState(BackgroundCopyJobImpl *job, BG_JOB_STATE from, BG_JOB_STATE to);

/* Little helper functions */
static inline WCHAR *co_strdupW(const WCHAR *src)
{
    WCHAR *dst = CoTaskMemAlloc((lstrlenW(src) + 1) * sizeof(WCHAR));
    if (dst) lstrcpyW(dst, src);
    return dst;
}

static inline HRESULT return_strval(const WCHAR *str, WCHAR **ret)
{
    int len;

    if (!ret) return E_INVALIDARG;

    len = lstrlenW(str);
    *ret = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
    if (!*ret) return E_OUTOFMEMORY;
    lstrcpyW(*ret, str);
    return S_OK;
}

#endif /* __QMGR_H__ */
