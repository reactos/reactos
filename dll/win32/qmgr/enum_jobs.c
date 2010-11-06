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

static void EnumBackgroundCopyJobsDestructor(EnumBackgroundCopyJobsImpl *This)
{
    ULONG i;

    for(i = 0; i < This->numJobs; i++)
        IBackgroundCopyJob_Release(This->jobs[i]);

    HeapFree(GetProcessHeap(), 0, This->jobs);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IEnumBackgroundCopyJobs_AddRef(
    IEnumBackgroundCopyJobs* iface)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_QueryInterface(
    IEnumBackgroundCopyJobs* iface,
    REFIID riid,
    void **ppvObject)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumBackgroundCopyJobs))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IEnumBackgroundCopyJobs_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IEnumBackgroundCopyJobs_Release(
    IEnumBackgroundCopyJobs* iface)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        EnumBackgroundCopyJobsDestructor(This);

    return ref;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Next(
    IEnumBackgroundCopyJobs* iface,
    ULONG celt,
    IBackgroundCopyJob **rgelt,
    ULONG *pceltFetched)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    ULONG fetched;
    ULONG i;
    IBackgroundCopyJob *job;

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
        job = This->jobs[This->indexJobs++];
        IBackgroundCopyJob_AddRef(job);
        rgelt[i] = job;
    }

    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Skip(
    IEnumBackgroundCopyJobs* iface,
    ULONG celt)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;

    if (This->numJobs - This->indexJobs < celt)
    {
        This->indexJobs = This->numJobs;
        return S_FALSE;
    }

    This->indexJobs += celt;
    return S_OK;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Reset(
    IEnumBackgroundCopyJobs* iface)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    This->indexJobs = 0;
    return S_OK;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Clone(
    IEnumBackgroundCopyJobs* iface,
    IEnumBackgroundCopyJobs **ppenum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_GetCount(
    IEnumBackgroundCopyJobs* iface,
    ULONG *puCount)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    *puCount = This->numJobs;
    return S_OK;
}

static const IEnumBackgroundCopyJobsVtbl BITS_IEnumBackgroundCopyJobs_Vtbl =
{
    BITS_IEnumBackgroundCopyJobs_QueryInterface,
    BITS_IEnumBackgroundCopyJobs_AddRef,
    BITS_IEnumBackgroundCopyJobs_Release,
    BITS_IEnumBackgroundCopyJobs_Next,
    BITS_IEnumBackgroundCopyJobs_Skip,
    BITS_IEnumBackgroundCopyJobs_Reset,
    BITS_IEnumBackgroundCopyJobs_Clone,
    BITS_IEnumBackgroundCopyJobs_GetCount
};

HRESULT EnumBackgroundCopyJobsConstructor(LPVOID *ppObj,
                                          IBackgroundCopyManager* copyManager)
{
    BackgroundCopyManagerImpl *qmgr = (BackgroundCopyManagerImpl *) copyManager;
    EnumBackgroundCopyJobsImpl *This;
    BackgroundCopyJobImpl *job;
    ULONG i;

    TRACE("%p, %p)\n", ppObj, copyManager);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &BITS_IEnumBackgroundCopyJobs_Vtbl;
    This->ref = 1;

    /* Create array of jobs */
    This->indexJobs = 0;

    EnterCriticalSection(&qmgr->cs);
    This->numJobs = list_count(&qmgr->jobs);

    if (0 < This->numJobs)
    {
        This->jobs = HeapAlloc(GetProcessHeap(), 0,
                               This->numJobs * sizeof *This->jobs);
        if (!This->jobs)
        {
            LeaveCriticalSection(&qmgr->cs);
            HeapFree(GetProcessHeap(), 0, This);
            return E_OUTOFMEMORY;
        }
    }
    else
        This->jobs = NULL;

    i = 0;
    LIST_FOR_EACH_ENTRY(job, &qmgr->jobs, BackgroundCopyJobImpl, entryFromQmgr)
    {
        IBackgroundCopyJob *iJob = (IBackgroundCopyJob *) job;
        IBackgroundCopyJob_AddRef(iJob);
        This->jobs[i++] = iJob;
    }
    LeaveCriticalSection(&qmgr->cs);

    *ppObj = &This->lpVtbl;
    return S_OK;
}
