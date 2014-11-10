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

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <objbase.h>
#include <bits1_5.h>
#include <bits3_0.h>

#include <wine/list.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

/* Background copy job vtbl and related data */
typedef struct
{
    IBackgroundCopyJob2 IBackgroundCopyJob2_iface;
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
} BackgroundCopyJobImpl;

/* Background copy file vtbl and related data */
typedef struct
{
    IBackgroundCopyFile IBackgroundCopyFile_iface;
    LONG ref;
    BG_FILE_INFO info;
    BG_FILE_PROGRESS fileProgress;
    WCHAR tempFileName[MAX_PATH];
    struct list entryFromJob;
    BackgroundCopyJobImpl *owner;
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

extern HANDLE stop_event DECLSPEC_HIDDEN;
extern ClassFactoryImpl BITS_ClassFactory DECLSPEC_HIDDEN;
extern BackgroundCopyManagerImpl globalMgr DECLSPEC_HIDDEN;

HRESULT BackgroundCopyManagerConstructor(LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type,
                                     GUID *pJobId, BackgroundCopyJobImpl **job) DECLSPEC_HIDDEN;
HRESULT enum_copy_job_create(BackgroundCopyManagerImpl *qmgr,
        IEnumBackgroundCopyJobs **enumjob) DECLSPEC_HIDDEN;
HRESULT BackgroundCopyFileConstructor(BackgroundCopyJobImpl *owner,
                                      LPCWSTR remoteName, LPCWSTR localName,
                                      BackgroundCopyFileImpl **file) DECLSPEC_HIDDEN;
HRESULT EnumBackgroundCopyFilesConstructor(BackgroundCopyJobImpl*, IEnumBackgroundCopyFiles**) DECLSPEC_HIDDEN;
DWORD WINAPI fileTransfer(void *param) DECLSPEC_HIDDEN;
void processJob(BackgroundCopyJobImpl *job) DECLSPEC_HIDDEN;
BOOL processFile(BackgroundCopyFileImpl *file, BackgroundCopyJobImpl *job) DECLSPEC_HIDDEN;

/* Little helper functions */
static inline char *
qmgr_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = HeapAlloc(GetProcessHeap(), 0, n);
    return d ? memcpy(d, s, n) : NULL;
}

static inline HRESULT return_strval(const WCHAR *str, WCHAR **ret)
{
    int len;

    if (!ret) return E_INVALIDARG;

    len = strlenW(str);
    *ret = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
    if (!*ret) return E_OUTOFMEMORY;
    strcpyW(*ret, str);
    return S_OK;
}

static inline BOOL
transitionJobState(BackgroundCopyJobImpl *job, BG_JOB_STATE fromState,
                   BG_JOB_STATE toState)
{
    BOOL rv = FALSE;
    EnterCriticalSection(&globalMgr.cs);
    if (job->state == fromState)
    {
        job->state = toState;
        rv = TRUE;
    }
    LeaveCriticalSection(&globalMgr.cs);
    return rv;
}

#endif /* __QMGR_H__ */
