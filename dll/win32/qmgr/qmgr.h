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
#include "objbase.h"

#define COBJMACROS
#include "bits1_5.h"

#include <string.h>
#include "wine/list.h"

/* Background copy job vtbl and related data */
typedef struct
{
    const IBackgroundCopyJob2Vtbl *lpVtbl;
    LONG ref;
    LPWSTR displayName;
    BG_JOB_TYPE type;
    GUID jobId;
    struct list files;
    BG_JOB_PROGRESS jobProgress;
    BG_JOB_STATE state;
    /* Protects file list, and progress */
    CRITICAL_SECTION cs;
    struct list entryFromQmgr;
} BackgroundCopyJobImpl;

/* Enum background copy jobs vtbl and related data */
typedef struct
{
    const IEnumBackgroundCopyJobsVtbl *lpVtbl;
    LONG ref;
    IBackgroundCopyJob **jobs;
    ULONG numJobs;
    ULONG indexJobs;
} EnumBackgroundCopyJobsImpl;

/* Enum background copy files vtbl and related data */
typedef struct
{
    const IEnumBackgroundCopyFilesVtbl *lpVtbl;
    LONG ref;
    IBackgroundCopyFile **files;
    ULONG numFiles;
    ULONG indexFiles;
} EnumBackgroundCopyFilesImpl;

/* Background copy file vtbl and related data */
typedef struct
{
    const IBackgroundCopyFileVtbl *lpVtbl;
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
    const IBackgroundCopyManagerVtbl *lpVtbl;
    /* Protects job list, job states, and jobEvent  */
    CRITICAL_SECTION cs;
    HANDLE jobEvent;
    struct list jobs;
} BackgroundCopyManagerImpl;

typedef struct
{
    const IClassFactoryVtbl *lpVtbl;
} ClassFactoryImpl;

extern HANDLE stop_event;
extern ClassFactoryImpl BITS_ClassFactory;
extern BackgroundCopyManagerImpl globalMgr;

HRESULT BackgroundCopyManagerConstructor(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type,
                                     GUID *pJobId, LPVOID *ppObj);
HRESULT EnumBackgroundCopyJobsConstructor(LPVOID *ppObj,
                                          IBackgroundCopyManager* copyManager);
HRESULT BackgroundCopyFileConstructor(BackgroundCopyJobImpl *owner,
                                      LPCWSTR remoteName, LPCWSTR localName,
                                      LPVOID *ppObj);
HRESULT EnumBackgroundCopyFilesConstructor(LPVOID *ppObj,
                                           IBackgroundCopyJob2 *copyJob);
DWORD WINAPI fileTransfer(void *param);
void processJob(BackgroundCopyJobImpl *job);
BOOL processFile(BackgroundCopyFileImpl *file, BackgroundCopyJobImpl *job);

/* Little helper functions */
static inline char *
qmgr_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = HeapAlloc(GetProcessHeap(), 0, n);
    return d ? memcpy(d, s, n) : NULL;
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
