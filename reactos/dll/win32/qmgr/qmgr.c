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

/* Add a reference to the iface pointer */
static ULONG WINAPI BITS_IBackgroundCopyManager_AddRef(
        IBackgroundCopyManager* iface)
{
    return 2;
}

/* Attempt to provide a new interface to interact with iface */
static HRESULT WINAPI BITS_IBackgroundCopyManager_QueryInterface(
        IBackgroundCopyManager* iface,
        REFIID riid,
        LPVOID *ppvObject)
{
    BackgroundCopyManagerImpl * This = (BackgroundCopyManagerImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IBackgroundCopyManager))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IBackgroundCopyManager_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

/* Release an interface to iface */
static ULONG WINAPI BITS_IBackgroundCopyManager_Release(
        IBackgroundCopyManager* iface)
{
    return 1;
}

/*** IBackgroundCopyManager interface methods ***/

static HRESULT WINAPI BITS_IBackgroundCopyManager_CreateJob(
        IBackgroundCopyManager* iface,
        LPCWSTR DisplayName,
        BG_JOB_TYPE Type,
        GUID *pJobId,
        IBackgroundCopyJob **ppJob)
{
    BackgroundCopyManagerImpl * This = (BackgroundCopyManagerImpl *) iface;
    BackgroundCopyJobImpl *job;
    HRESULT hres;
    TRACE("\n");

    hres = BackgroundCopyJobConstructor(DisplayName, Type, pJobId,
                                        (LPVOID *) ppJob);
    if (FAILED(hres))
        return hres;

    /* Add a reference to the job to job list */
    IBackgroundCopyJob_AddRef(*ppJob);
    job = (BackgroundCopyJobImpl *) *ppJob;
    EnterCriticalSection(&This->cs);
    list_add_head(&This->jobs, &job->entryFromQmgr);
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_GetJob(
        IBackgroundCopyManager* iface,
        REFGUID jobID,
        IBackgroundCopyJob **ppJob)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_EnumJobs(
        IBackgroundCopyManager* iface,
        DWORD dwFlags,
        IEnumBackgroundCopyJobs **ppEnum)
{
    TRACE("\n");
    return EnumBackgroundCopyJobsConstructor((LPVOID *) ppEnum, iface);
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_GetErrorDescription(
        IBackgroundCopyManager* iface,
        HRESULT hResult,
        DWORD LanguageId,
        LPWSTR *pErrorDescription)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}


static const IBackgroundCopyManagerVtbl BITS_IBackgroundCopyManager_Vtbl =
{
    BITS_IBackgroundCopyManager_QueryInterface,
    BITS_IBackgroundCopyManager_AddRef,
    BITS_IBackgroundCopyManager_Release,
    BITS_IBackgroundCopyManager_CreateJob,
    BITS_IBackgroundCopyManager_GetJob,
    BITS_IBackgroundCopyManager_EnumJobs,
    BITS_IBackgroundCopyManager_GetErrorDescription
};

BackgroundCopyManagerImpl globalMgr = {
    &BITS_IBackgroundCopyManager_Vtbl,
    { NULL, -1, 0, 0, 0, 0 },
    NULL,
    LIST_INIT(globalMgr.jobs)
};

/* Constructor for instances of background copy manager */
HRESULT BackgroundCopyManagerConstructor(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    TRACE("(%p,%p)\n", pUnkOuter, ppObj);
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
            return 0;

        /* Note that other threads may add files to the job list, but only
           this thread ever deletes them so we don't need to worry about jobs
           magically disappearing from the list.  */
        EnterCriticalSection(&qmgr->cs);

        LIST_FOR_EACH_ENTRY_SAFE(job, jobCur, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
        {
            if (job->state == BG_JOB_STATE_ACKNOWLEDGED || job->state == BG_JOB_STATE_CANCELLED)
            {
                list_remove(&job->entryFromQmgr);
                IBackgroundCopyJob_Release((IBackgroundCopyJob *) job);
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
