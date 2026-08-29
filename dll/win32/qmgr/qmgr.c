/*
 * Queue Manager (BITS) core functions
 *
 * Copyright 2007, 2008 Google (Roy Shea, Dan Hipschman)
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

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

BackgroundCopyManagerImpl globalMgr;

static HRESULT WINAPI BackgroundCopyManager_QueryInterface(IBackgroundCopyManager *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IBackgroundCopyManager))
    {
        *ppv = iface;
        IBackgroundCopyManager_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BackgroundCopyManager_AddRef(IBackgroundCopyManager *iface)
{
    return 2;
}

static ULONG WINAPI BackgroundCopyManager_Release(IBackgroundCopyManager *iface)
{
    return 1;
}

/*** IBackgroundCopyManager interface methods ***/

static HRESULT WINAPI BackgroundCopyManager_CreateJob(IBackgroundCopyManager *iface,
        LPCWSTR DisplayName, BG_JOB_TYPE Type, GUID *pJobId, IBackgroundCopyJob **ppJob)
{
    BackgroundCopyJobImpl *job;
    HRESULT hres;

    TRACE("(%s %d %p %p)\n", debugstr_w(DisplayName), Type, pJobId, ppJob);

    hres = BackgroundCopyJobConstructor(DisplayName, Type, pJobId, &job);
    if (FAILED(hres))
        return hres;

    /* Add a reference to the job to job list */
    *ppJob = (IBackgroundCopyJob *)&job->IBackgroundCopyJob4_iface;
    IBackgroundCopyJob_AddRef(*ppJob);
    EnterCriticalSection(&globalMgr.cs);
    list_add_head(&globalMgr.jobs, &job->entryFromQmgr);
    LeaveCriticalSection(&globalMgr.cs);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyManager_GetJob(IBackgroundCopyManager *iface,
        REFGUID jobID, IBackgroundCopyJob **job)
{
    BackgroundCopyManagerImpl *qmgr = &globalMgr;
    HRESULT hr = BG_E_NOT_FOUND;
    BackgroundCopyJobImpl *cur;

    TRACE("(%s %p)\n", debugstr_guid(jobID), job);

    if (!job || !jobID) return E_INVALIDARG;

    *job = NULL;

    EnterCriticalSection(&qmgr->cs);

    LIST_FOR_EACH_ENTRY(cur, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
    {
        if (IsEqualGUID(&cur->jobId, jobID))
        {
            *job = (IBackgroundCopyJob *)&cur->IBackgroundCopyJob4_iface;
            IBackgroundCopyJob_AddRef(*job);
            hr = S_OK;
            break;
        }
    }

    LeaveCriticalSection(&qmgr->cs);

    return hr;
}

static HRESULT WINAPI BackgroundCopyManager_EnumJobs(IBackgroundCopyManager *iface,
        DWORD flags, IEnumBackgroundCopyJobs **ppEnum)
{
    TRACE("(0x%lx %p)\n", flags, ppEnum);
    return enum_copy_job_create(&globalMgr, ppEnum);
}

static HRESULT WINAPI BackgroundCopyManager_GetErrorDescription(IBackgroundCopyManager *iface,
        HRESULT hr, DWORD langid, LPWSTR *error_description)
{
    FIXME("(0x%08lx 0x%lx %p): stub\n", hr, langid, error_description);
    return E_NOTIMPL;
}

static const IBackgroundCopyManagerVtbl BackgroundCopyManagerVtbl =
{
    BackgroundCopyManager_QueryInterface,
    BackgroundCopyManager_AddRef,
    BackgroundCopyManager_Release,
    BackgroundCopyManager_CreateJob,
    BackgroundCopyManager_GetJob,
    BackgroundCopyManager_EnumJobs,
    BackgroundCopyManager_GetErrorDescription
};

BackgroundCopyManagerImpl globalMgr = {
    { &BackgroundCopyManagerVtbl },
    { NULL, -1, 0, 0, 0, 0 },
    NULL,
    LIST_INIT(globalMgr.jobs)
};

/* Constructor for instances of background copy manager */
HRESULT BackgroundCopyManagerConstructor(LPVOID *ppObj)
{
    TRACE("(%p)\n", ppObj);
    *ppObj = &globalMgr;
    return S_OK;
}

DWORD WINAPI fileTransfer(void *param)
{
    BackgroundCopyManagerImpl *qmgr = &globalMgr;
    HANDLE events[2];

    events[0] = stop_event;
    events[1] = qmgr->jobEvent;

    for (;;)
    {
        BackgroundCopyJobImpl *job, *jobCur;
        BOOL haveJob = FALSE;

        /* Check if it's the stop_event */
        if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0)
        {
            LIST_FOR_EACH_ENTRY_SAFE(job, jobCur, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
            {
                list_remove(&job->entryFromQmgr);
                IBackgroundCopyJob4_Release(&job->IBackgroundCopyJob4_iface);
            }
            return 0;
        }

        /* Note that other threads may add files to the job list, but only
           this thread ever deletes them so we don't need to worry about jobs
           magically disappearing from the list.  */
        EnterCriticalSection(&qmgr->cs);

        LIST_FOR_EACH_ENTRY_SAFE(job, jobCur, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
        {
            if (job->state == BG_JOB_STATE_ACKNOWLEDGED || job->state == BG_JOB_STATE_CANCELLED)
            {
                list_remove(&job->entryFromQmgr);
                IBackgroundCopyJob4_Release(&job->IBackgroundCopyJob4_iface);
            }
            else if (job->state == BG_JOB_STATE_QUEUED)
            {
                haveJob = TRUE;
                break;
            }
            else if (job->state == BG_JOB_STATE_CONNECTING
                     || job->state == BG_JOB_STATE_TRANSFERRING)
            {
                ERR("Invalid state for job %p: %d\n", job, job->state);
            }
        }

        if (!haveJob)
            ResetEvent(qmgr->jobEvent);

        LeaveCriticalSection(&qmgr->cs);

        if (haveJob)
            processJob(job);
    }
}
