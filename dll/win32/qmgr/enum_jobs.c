/*
 * Queue Manager (BITS) Job Enumerator
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

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

typedef struct
{
    IEnumBackgroundCopyJobs IEnumBackgroundCopyJobs_iface;
    LONG ref;
    IBackgroundCopyJob4 **jobs;
    ULONG numJobs;
    ULONG indexJobs;
} EnumBackgroundCopyJobsImpl;

static inline EnumBackgroundCopyJobsImpl *impl_from_IEnumBackgroundCopyJobs(IEnumBackgroundCopyJobs *iface)
{
    return CONTAINING_RECORD(iface, EnumBackgroundCopyJobsImpl, IEnumBackgroundCopyJobs_iface);
}

static HRESULT WINAPI EnumBackgroundCopyJobs_QueryInterface(IEnumBackgroundCopyJobs *iface,
        REFIID riid, void **ppv)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumBackgroundCopyJobs))
    {
        *ppv = iface;
        IEnumBackgroundCopyJobs_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumBackgroundCopyJobs_AddRef(IEnumBackgroundCopyJobs *iface)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%ld)\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumBackgroundCopyJobs_Release(IEnumBackgroundCopyJobs *iface)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    ULONG i;

    TRACE("(%p)->(%ld)\n", This, ref);

    if (ref == 0) {
        for(i = 0; i < This->numJobs; i++)
            IBackgroundCopyJob4_Release(This->jobs[i]);
        free(This->jobs);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI EnumBackgroundCopyJobs_Next(IEnumBackgroundCopyJobs *iface, ULONG celt,
        IBackgroundCopyJob **rgelt, ULONG *pceltFetched)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);
    ULONG fetched;
    ULONG i;

    TRACE("(%p)->(%ld %p %p)\n", This, celt, rgelt, pceltFetched);

    fetched = min(celt, This->numJobs - This->indexJobs);
    if (pceltFetched)
        *pceltFetched = fetched;
    else
    {
        /* We need to initialize this array if the caller doesn't request
           the length because length_is will default to celt.  */
        for (i = 0; i < celt; ++i)
            rgelt[i] = NULL;

        /* pceltFetched can only be NULL if celt is 1 */
        if (celt != 1)
            return E_INVALIDARG;
    }

    /* Fill in the array of objects */
    for (i = 0; i < fetched; ++i)
    {
        rgelt[i] = (IBackgroundCopyJob *)This->jobs[This->indexJobs++];
        IBackgroundCopyJob_AddRef(rgelt[i]);
    }

    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumBackgroundCopyJobs_Skip(IEnumBackgroundCopyJobs *iface, ULONG celt)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);

    TRACE("(%p)->(%ld)\n", This, celt);

    if (This->numJobs - This->indexJobs < celt)
    {
        This->indexJobs = This->numJobs;
        return S_FALSE;
    }

    This->indexJobs += celt;
    return S_OK;
}

static HRESULT WINAPI EnumBackgroundCopyJobs_Reset(IEnumBackgroundCopyJobs *iface)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);

    TRACE("(%p)\n", This);

    This->indexJobs = 0;
    return S_OK;
}

static HRESULT WINAPI EnumBackgroundCopyJobs_Clone(IEnumBackgroundCopyJobs *iface,
        IEnumBackgroundCopyJobs **ppenum)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);
    FIXME("(%p)->(%p): stub\n", This, ppenum);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumBackgroundCopyJobs_GetCount(IEnumBackgroundCopyJobs *iface,
    ULONG *puCount)
{
    EnumBackgroundCopyJobsImpl *This = impl_from_IEnumBackgroundCopyJobs(iface);

    TRACE("(%p)->(%p)\n", This, puCount);

    *puCount = This->numJobs;
    return S_OK;
}

static const IEnumBackgroundCopyJobsVtbl EnumBackgroundCopyJobsVtbl =
{
    EnumBackgroundCopyJobs_QueryInterface,
    EnumBackgroundCopyJobs_AddRef,
    EnumBackgroundCopyJobs_Release,
    EnumBackgroundCopyJobs_Next,
    EnumBackgroundCopyJobs_Skip,
    EnumBackgroundCopyJobs_Reset,
    EnumBackgroundCopyJobs_Clone,
    EnumBackgroundCopyJobs_GetCount
};

HRESULT enum_copy_job_create(BackgroundCopyManagerImpl *qmgr, IEnumBackgroundCopyJobs **enumjob)
{
    EnumBackgroundCopyJobsImpl *This;
    BackgroundCopyJobImpl *job;
    ULONG i;

    TRACE("%p, %p)\n", qmgr, enumjob);

    This = malloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->IEnumBackgroundCopyJobs_iface.lpVtbl = &EnumBackgroundCopyJobsVtbl;
    This->ref = 1;

    /* Create array of jobs */
    This->indexJobs = 0;

    EnterCriticalSection(&qmgr->cs);
    This->numJobs = list_count(&qmgr->jobs);

    if (0 < This->numJobs)
    {
        This->jobs = malloc(This->numJobs * sizeof *This->jobs);
        if (!This->jobs)
        {
            LeaveCriticalSection(&qmgr->cs);
            free(This);
            return E_OUTOFMEMORY;
        }
    }
    else
        This->jobs = NULL;

    i = 0;
    LIST_FOR_EACH_ENTRY(job, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
    {
        IBackgroundCopyJob4_AddRef(&job->IBackgroundCopyJob4_iface);
        This->jobs[i++] = &job->IBackgroundCopyJob4_iface;
    }
    LeaveCriticalSection(&qmgr->cs);

    *enumjob = &This->IEnumBackgroundCopyJobs_iface;
    return S_OK;
}
