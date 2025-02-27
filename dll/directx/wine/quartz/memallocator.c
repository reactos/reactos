/*
 * Memory Allocator and Media Sample Implementation
 *
 * Copyright 2003 Robert Shearman
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

#include <assert.h>
#include <limits.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "vfwmsgs.h"

#include "quartz_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct StdMediaSample2
{
    IMediaSample2 IMediaSample2_iface;
    LONG ref;
    AM_SAMPLE2_PROPERTIES props;
    IMemAllocator * pParent;
    struct list listentry;
    LONGLONG tMediaStart;
    LONGLONG tMediaEnd;
    BOOL media_time_valid;
} StdMediaSample2;

typedef struct BaseMemAllocator
{
    IMemAllocator IMemAllocator_iface;

    LONG ref;
    ALLOCATOR_PROPERTIES props;
    HRESULT (* fnAlloc) (IMemAllocator *);
    HRESULT (* fnFree)(IMemAllocator *);
    HRESULT (* fnVerify)(IMemAllocator *, ALLOCATOR_PROPERTIES *);
    HRESULT (* fnBufferPrepare)(IMemAllocator *, StdMediaSample2 *, DWORD flags);
    HRESULT (* fnBufferReleased)(IMemAllocator *, StdMediaSample2 *);
    void (* fnDestroyed)(IMemAllocator *);
    HANDLE hSemWaiting;
    BOOL bDecommitQueued;
    BOOL bCommitted;
    LONG lWaiting;
    struct list free_list;
    struct list used_list;
    CRITICAL_SECTION *pCritSect;
} BaseMemAllocator;

static inline BaseMemAllocator *impl_from_IMemAllocator(IMemAllocator *iface)
{
    return CONTAINING_RECORD(iface, BaseMemAllocator, IMemAllocator_iface);
}

static const IMemAllocatorVtbl BaseMemAllocator_VTable;
static const IMediaSample2Vtbl StdMediaSample2_VTable;
static inline StdMediaSample2 *unsafe_impl_from_IMediaSample(IMediaSample * iface);

#define AM_SAMPLE2_PROP_SIZE_WRITABLE FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, pbBuffer)

static HRESULT BaseMemAllocator_Init(HRESULT (* fnAlloc)(IMemAllocator *),
                                     HRESULT (* fnFree)(IMemAllocator *),
                                     HRESULT (* fnVerify)(IMemAllocator *, ALLOCATOR_PROPERTIES *),
                                     HRESULT (* fnBufferPrepare)(IMemAllocator *, StdMediaSample2 *, DWORD),
                                     HRESULT (* fnBufferReleased)(IMemAllocator *, StdMediaSample2 *),
                                     void (* fnDestroyed)(IMemAllocator *),
                                     CRITICAL_SECTION *pCritSect,
                                     BaseMemAllocator * pMemAlloc)
{
    assert(fnAlloc && fnFree && fnDestroyed);

    pMemAlloc->IMemAllocator_iface.lpVtbl = &BaseMemAllocator_VTable;

    pMemAlloc->ref = 1;
    ZeroMemory(&pMemAlloc->props, sizeof(pMemAlloc->props));
    list_init(&pMemAlloc->free_list);
    list_init(&pMemAlloc->used_list);
    pMemAlloc->fnAlloc = fnAlloc;
    pMemAlloc->fnFree = fnFree;
    pMemAlloc->fnVerify = fnVerify;
    pMemAlloc->fnBufferPrepare = fnBufferPrepare;
    pMemAlloc->fnBufferReleased = fnBufferReleased;
    pMemAlloc->fnDestroyed = fnDestroyed;
    pMemAlloc->bDecommitQueued = FALSE;
    pMemAlloc->bCommitted = FALSE;
    pMemAlloc->hSemWaiting = NULL;
    pMemAlloc->lWaiting = 0;
    pMemAlloc->pCritSect = pCritSect;

    return S_OK;
}

static HRESULT WINAPI BaseMemAllocator_QueryInterface(IMemAllocator * iface, REFIID riid, LPVOID * ppv)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    TRACE("(%p)->(%s, %p)\n", This, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IMemAllocator_iface;
    else if (IsEqualIID(riid, &IID_IMemAllocator))
        *ppv = &This->IMemAllocator_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI BaseMemAllocator_AddRef(IMemAllocator * iface)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %lu.\n", This, ref);

    return ref;
}

static ULONG WINAPI BaseMemAllocator_Release(IMemAllocator * iface)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %lu.\n", This, ref);

    if (!ref)
    {
        CloseHandle(This->hSemWaiting);
        if (This->bCommitted)
            This->fnFree(iface);

        This->fnDestroyed(iface);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI BaseMemAllocator_SetProperties(IMemAllocator * iface, ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pRequest, pActual);

    TRACE("Requested %ld buffers, size %ld, alignment %ld, prefix %ld.\n",
            pRequest->cBuffers, pRequest->cbBuffer, pRequest->cbAlign, pRequest->cbPrefix);

    EnterCriticalSection(This->pCritSect);
    {
        if (!list_empty(&This->used_list))
            hr = VFW_E_BUFFERS_OUTSTANDING;
        else if (This->bCommitted)
            hr = VFW_E_ALREADY_COMMITTED;
        else if (pRequest->cbAlign == 0)
            hr = VFW_E_BADALIGN;
        else
        {
            if (This->fnVerify)
                 hr = This->fnVerify(iface, pRequest);
            else
                 hr = S_OK;

            if (SUCCEEDED(hr))
                 This->props = *pRequest;

            *pActual = This->props;
        }
    }
    LeaveCriticalSection(This->pCritSect);

    return hr;
}

static HRESULT WINAPI BaseMemAllocator_GetProperties(IMemAllocator * iface, ALLOCATOR_PROPERTIES *pProps)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);

    TRACE("(%p)->(%p)\n", This, pProps);

    EnterCriticalSection(This->pCritSect);
    {
         memcpy(pProps, &This->props, sizeof(*pProps));
    }
    LeaveCriticalSection(This->pCritSect);

    return S_OK;
}

static HRESULT WINAPI BaseMemAllocator_Commit(IMemAllocator * iface)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pCritSect);
    {
        if (!This->props.cbAlign)
            hr = VFW_E_BADALIGN;
        else if (!This->props.cbBuffer)
            hr = VFW_E_SIZENOTSET;
        else if (!This->props.cBuffers)
            hr = VFW_E_BUFFER_NOTSET;
        else if (This->bDecommitQueued && This->bCommitted)
        {
            This->bDecommitQueued = FALSE;
            hr = S_OK;
        }
        else if (This->bCommitted)
            hr = S_OK;
        else
        {
            if (!(This->hSemWaiting = CreateSemaphoreW(NULL, This->props.cBuffers, This->props.cBuffers, NULL)))
            {
                ERR("Failed to create semaphore, error %lu.\n", GetLastError());
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                hr = This->fnAlloc(iface);
                if (SUCCEEDED(hr))
                    This->bCommitted = TRUE;
                else
                    ERR("Failed to allocate, hr %#lx.\n", hr);
            }
        }
    }
    LeaveCriticalSection(This->pCritSect);

    return hr;
}

static HRESULT WINAPI BaseMemAllocator_Decommit(IMemAllocator * iface)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pCritSect);
    {
        if (!This->bCommitted)
            hr = S_OK;
        else
        {
            if (!list_empty(&This->used_list))
            {
                This->bDecommitQueued = TRUE;
                /* notify ALL waiting threads that they cannot be allocated a buffer any more */
                ReleaseSemaphore(This->hSemWaiting, This->lWaiting, NULL);
                
                hr = S_OK;
            }
            else
            {
                if (This->lWaiting != 0)
                    ERR("Waiting: %ld\n", This->lWaiting);

                This->bCommitted = FALSE;
                CloseHandle(This->hSemWaiting);
                This->hSemWaiting = NULL;

                hr = This->fnFree(iface);
            }
        }
    }
    LeaveCriticalSection(This->pCritSect);

    return hr;
}

static HRESULT WINAPI BaseMemAllocator_GetBuffer(IMemAllocator * iface, IMediaSample ** pSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    HRESULT hr = S_OK;

    /* NOTE: The pStartTime and pEndTime parameters are not applied to the sample. 
     * The allocator might use these values to determine which buffer it retrieves */

    TRACE("allocator %p, sample %p, start_time %p, end_time %p, flags %#lx.\n",
            This, pSample, pStartTime, pEndTime, dwFlags);

    *pSample = NULL;

    EnterCriticalSection(This->pCritSect);
    if (!This->bCommitted || This->bDecommitQueued)
    {
        WARN("Not committed\n");
        hr = VFW_E_NOT_COMMITTED;
    }
    else
        ++This->lWaiting;
    LeaveCriticalSection(This->pCritSect);
    if (FAILED(hr))
        return hr;

    if (WaitForSingleObject(This->hSemWaiting, (dwFlags & AM_GBF_NOWAIT) ? 0 : INFINITE) != WAIT_OBJECT_0)
    {
        EnterCriticalSection(This->pCritSect);
        --This->lWaiting;
        LeaveCriticalSection(This->pCritSect);
        WARN("Timed out\n");
        return VFW_E_TIMEOUT;
    }

    EnterCriticalSection(This->pCritSect);
    {
        --This->lWaiting;
        if (!This->bCommitted)
            hr = VFW_E_NOT_COMMITTED;
        else if (This->bDecommitQueued)
            hr = VFW_E_TIMEOUT;
        else
        {
            StdMediaSample2 *ms;
            struct list * free = list_head(&This->free_list);
            list_remove(free);
            list_add_head(&This->used_list, free);

            ms = LIST_ENTRY(free, StdMediaSample2, listentry);
            assert(ms->ref == 0);
            *pSample = (IMediaSample *)&ms->IMediaSample2_iface;
            IMediaSample_AddRef(*pSample);
        }
    }
    LeaveCriticalSection(This->pCritSect);

    if (hr != S_OK)
        WARN("Returning hr %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI BaseMemAllocator_ReleaseBuffer(IMemAllocator * iface, IMediaSample * pSample)
{
    BaseMemAllocator *This = impl_from_IMemAllocator(iface);
    StdMediaSample2 * pStdSample = unsafe_impl_from_IMediaSample(pSample);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", This, pSample);

    /* FIXME: make sure that sample is currently on the used list */

    /* FIXME: we should probably check the ref count on the sample before freeing
     * it to make sure that it is not still in use */
    EnterCriticalSection(This->pCritSect);
    {
        if (!This->bCommitted)
            ERR("Releasing a buffer when the allocator is not committed?!?\n");

		/* remove from used_list */
        list_remove(&pStdSample->listentry);

        list_add_head(&This->free_list, &pStdSample->listentry);

        if (list_empty(&This->used_list) && This->bDecommitQueued && This->bCommitted)
        {
            if (This->lWaiting != 0)
                ERR("Waiting: %ld\n", This->lWaiting);

            This->bCommitted = FALSE;
            This->bDecommitQueued = FALSE;

            CloseHandle(This->hSemWaiting);
            This->hSemWaiting = NULL;
            
            This->fnFree(iface);
        }
    }
    LeaveCriticalSection(This->pCritSect);

    /* notify a waiting thread that there is now a free buffer */
    if (This->hSemWaiting && !ReleaseSemaphore(This->hSemWaiting, 1, NULL))
    {
        ERR("Failed to release semaphore, error %lu.\n", GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

static const IMemAllocatorVtbl BaseMemAllocator_VTable = 
{
    BaseMemAllocator_QueryInterface,
    BaseMemAllocator_AddRef,
    BaseMemAllocator_Release,
    BaseMemAllocator_SetProperties,
    BaseMemAllocator_GetProperties,
    BaseMemAllocator_Commit,
    BaseMemAllocator_Decommit,
    BaseMemAllocator_GetBuffer,
    BaseMemAllocator_ReleaseBuffer
};

static HRESULT StdMediaSample2_Construct(BYTE * pbBuffer, LONG cbBuffer, IMemAllocator * pParent, StdMediaSample2 ** ppSample)
{
    assert(pbBuffer && pParent && (cbBuffer > 0));

    if (!(*ppSample = CoTaskMemAlloc(sizeof(StdMediaSample2))))
        return E_OUTOFMEMORY;

    (*ppSample)->IMediaSample2_iface.lpVtbl = &StdMediaSample2_VTable;
    (*ppSample)->ref = 0;
    ZeroMemory(&(*ppSample)->props, sizeof((*ppSample)->props));

    /* NOTE: no need to AddRef as the parent is guaranteed to be around
     * at least as long as us and we don't want to create circular
     * dependencies on the ref count */
    (*ppSample)->pParent = pParent;
    (*ppSample)->props.cbData = sizeof(AM_SAMPLE2_PROPERTIES);
    (*ppSample)->props.cbBuffer = (*ppSample)->props.lActual = cbBuffer;
    (*ppSample)->props.pbBuffer = pbBuffer;
    (*ppSample)->media_time_valid = FALSE;

    return S_OK;
}

static void StdMediaSample2_Delete(StdMediaSample2 * This)
{
    if (This->props.pMediaType)
        DeleteMediaType(This->props.pMediaType);

    /* NOTE: does not remove itself from the list it belongs to */
    CoTaskMemFree(This);
}

static inline StdMediaSample2 *impl_from_IMediaSample2(IMediaSample2 * iface)
{
    return CONTAINING_RECORD(iface, StdMediaSample2, IMediaSample2_iface);
}

static HRESULT WINAPI StdMediaSample2_QueryInterface(IMediaSample2 * iface, REFIID riid, void ** ppv)
{
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMediaSample) ||
            IsEqualIID(riid, &IID_IMediaSample2))
    {
        *ppv = iface;
        IMediaSample2_AddRef(iface);
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI StdMediaSample2_AddRef(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %lu.\n", This, ref);

    return ref;
}

static ULONG WINAPI StdMediaSample2_Release(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %lu.\n", This, ref);

    if (!ref)
    {
        if (This->props.pMediaType)
            DeleteMediaType(This->props.pMediaType);
        This->props.pMediaType = NULL;
        This->props.dwSampleFlags = 0;
        This->media_time_valid = FALSE;

        if (This->pParent)
            IMemAllocator_ReleaseBuffer(This->pParent, (IMediaSample *)iface);
        else
            StdMediaSample2_Delete(This);
    }
    return ref;
}

static HRESULT WINAPI StdMediaSample2_GetPointer(IMediaSample2 * iface, BYTE ** ppBuffer)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%p)\n", iface, ppBuffer);

    *ppBuffer = This->props.pbBuffer;

    if (!*ppBuffer)
    {
        ERR("Requested an unlocked surface and trying to lock regardless\n");
        return E_FAIL;
    }

    return S_OK;
}

static LONG WINAPI StdMediaSample2_GetSize(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("StdMediaSample2_GetSize()\n");

    return This->props.cbBuffer;
}

static HRESULT WINAPI StdMediaSample2_GetTime(IMediaSample2 * iface, REFERENCE_TIME * pStart, REFERENCE_TIME * pEnd)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", iface, pStart, pEnd);

    if (!(This->props.dwSampleFlags & AM_SAMPLE_TIMEVALID))
        hr = VFW_E_SAMPLE_TIME_NOT_SET;
    else if (!(This->props.dwSampleFlags & AM_SAMPLE_STOPVALID))
    {
        *pStart = This->props.tStart;
        *pEnd = This->props.tStart + 1;
        
        hr = VFW_S_NO_STOP_TIME;
    }
    else
    {
        *pStart = This->props.tStart;
        *pEnd = This->props.tStop;

        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI StdMediaSample2_SetTime(IMediaSample2 *iface, REFERENCE_TIME *start, REFERENCE_TIME *end)
{
    StdMediaSample2 *sample = impl_from_IMediaSample2(iface);

    TRACE("sample %p, start %s, end %s.\n", sample, start ? debugstr_time(*start) : "(null)",
            end ? debugstr_time(*end) : "(null)");

    if (start)
    {
        sample->props.tStart = *start;
        sample->props.dwSampleFlags |= AM_SAMPLE_TIMEVALID;

        if (end)
        {
            sample->props.tStop = *end;
            sample->props.dwSampleFlags |= AM_SAMPLE_STOPVALID;
        }
        else
            sample->props.dwSampleFlags &= ~AM_SAMPLE_STOPVALID;
    }
    else
        sample->props.dwSampleFlags &= ~(AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID);

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_IsSyncPoint(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->()\n", iface);

    return (This->props.dwSampleFlags & AM_SAMPLE_SPLICEPOINT) ? S_OK : S_FALSE;
}

static HRESULT WINAPI StdMediaSample2_SetSyncPoint(IMediaSample2 * iface, BOOL bIsSyncPoint)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%s)\n", iface, bIsSyncPoint ? "TRUE" : "FALSE");

    if (bIsSyncPoint)
        This->props.dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
    else
        This->props.dwSampleFlags &= ~AM_SAMPLE_SPLICEPOINT;

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_IsPreroll(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->()\n", iface);

    return (This->props.dwSampleFlags & AM_SAMPLE_PREROLL) ? S_OK : S_FALSE;
}

static HRESULT WINAPI StdMediaSample2_SetPreroll(IMediaSample2 * iface, BOOL bIsPreroll)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%s)\n", iface, bIsPreroll ? "TRUE" : "FALSE");

    if (bIsPreroll)
        This->props.dwSampleFlags |= AM_SAMPLE_PREROLL;
    else
        This->props.dwSampleFlags &= ~AM_SAMPLE_PREROLL;

    return S_OK;
}

static LONG WINAPI StdMediaSample2_GetActualDataLength(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->()\n", iface);

    return This->props.lActual;
}

static HRESULT WINAPI StdMediaSample2_SetActualDataLength(IMediaSample2 * iface, LONG len)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("sample %p, len %ld.\n", This, len);

    if ((len > This->props.cbBuffer) || (len < 0))
    {
        ERR("Length %ld exceeds maximum %ld.\n", len, This->props.cbBuffer);
        return VFW_E_BUFFER_OVERFLOW;
    }
    else
    {
        This->props.lActual = len;
        return S_OK;
    }
}

static HRESULT WINAPI StdMediaSample2_GetMediaType(IMediaSample2 * iface, AM_MEDIA_TYPE ** ppMediaType)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%p)\n", iface, ppMediaType);

    if (!This->props.pMediaType) {
        /* Make sure we return a NULL pointer (required by native Quartz dll) */
        if (ppMediaType)
            *ppMediaType = NULL;
        return S_FALSE;
    }

    if (!(*ppMediaType = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    return CopyMediaType(*ppMediaType, This->props.pMediaType);
}

static HRESULT WINAPI StdMediaSample2_SetMediaType(IMediaSample2 * iface, AM_MEDIA_TYPE * pMediaType)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%p)\n", iface, pMediaType);

    if (This->props.pMediaType)
    {
        DeleteMediaType(This->props.pMediaType);
        This->props.pMediaType = NULL;
    }

    if (!pMediaType)
    {
        This->props.dwSampleFlags &= ~AM_SAMPLE_TYPECHANGED;
        return S_OK;
    }

    This->props.dwSampleFlags |= AM_SAMPLE_TYPECHANGED;

    if (!(This->props.pMediaType = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    return CopyMediaType(This->props.pMediaType, pMediaType);
}

static HRESULT WINAPI StdMediaSample2_IsDiscontinuity(IMediaSample2 * iface)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->()\n", iface);

    return (This->props.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) ? S_OK : S_FALSE;
}

static HRESULT WINAPI StdMediaSample2_SetDiscontinuity(IMediaSample2 * iface, BOOL bIsDiscontinuity)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%s)\n", iface, bIsDiscontinuity ? "TRUE" : "FALSE");

    if (bIsDiscontinuity)
        This->props.dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
    else
        This->props.dwSampleFlags &= ~AM_SAMPLE_DATADISCONTINUITY;

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_GetMediaTime(IMediaSample2 * iface, LONGLONG * pStart, LONGLONG * pEnd)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("(%p)->(%p, %p)\n", iface, pStart, pEnd);

    if (!This->media_time_valid)
        return VFW_E_MEDIA_TIME_NOT_SET;

    *pStart = This->tMediaStart;
    *pEnd = This->tMediaEnd;

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_SetMediaTime(IMediaSample2 *iface, LONGLONG *start, LONGLONG *end)
{
    StdMediaSample2 *sample = impl_from_IMediaSample2(iface);

    TRACE("sample %p, start %s, end %s.\n", sample, start ? debugstr_time(*start) : "(null)",
            end ? debugstr_time(*end) : "(null)");

    if (start)
    {
        if (!end) return E_POINTER;
        sample->tMediaStart = *start;
        sample->tMediaEnd = *end;
        sample->media_time_valid = TRUE;
    }
    else
        sample->media_time_valid = FALSE;

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_GetProperties(IMediaSample2 * iface, DWORD cbProperties, BYTE * pbProperties)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("sample %p, size %lu, properties %p.\n", This, cbProperties, pbProperties);

    memcpy(pbProperties, &This->props, min(cbProperties, sizeof(This->props)));

    return S_OK;
}

static HRESULT WINAPI StdMediaSample2_SetProperties(IMediaSample2 * iface, DWORD cbProperties, const BYTE * pbProperties)
{
    StdMediaSample2 *This = impl_from_IMediaSample2(iface);

    TRACE("sample %p, size %lu, properties %p.\n", This, cbProperties, pbProperties);

    /* NOTE: pbBuffer and cbBuffer are read-only */
    memcpy(&This->props, pbProperties, min(cbProperties, AM_SAMPLE2_PROP_SIZE_WRITABLE));

    return S_OK;
}

static const IMediaSample2Vtbl StdMediaSample2_VTable = 
{
    StdMediaSample2_QueryInterface,
    StdMediaSample2_AddRef,
    StdMediaSample2_Release,
    StdMediaSample2_GetPointer,
    StdMediaSample2_GetSize,
    StdMediaSample2_GetTime,
    StdMediaSample2_SetTime,
    StdMediaSample2_IsSyncPoint,
    StdMediaSample2_SetSyncPoint,
    StdMediaSample2_IsPreroll,
    StdMediaSample2_SetPreroll,
    StdMediaSample2_GetActualDataLength,
    StdMediaSample2_SetActualDataLength,
    StdMediaSample2_GetMediaType,
    StdMediaSample2_SetMediaType,
    StdMediaSample2_IsDiscontinuity,
    StdMediaSample2_SetDiscontinuity,
    StdMediaSample2_GetMediaTime,
    StdMediaSample2_SetMediaTime,
    StdMediaSample2_GetProperties,
    StdMediaSample2_SetProperties
};

static inline StdMediaSample2 *unsafe_impl_from_IMediaSample(IMediaSample * iface)
{
    IMediaSample2 *iface2 = (IMediaSample2 *)iface;

    if (!iface)
        return NULL;
    assert(iface2->lpVtbl == &StdMediaSample2_VTable);
    return impl_from_IMediaSample2(iface2);
}

typedef struct StdMemAllocator
{
    BaseMemAllocator base;
    CRITICAL_SECTION csState;
    LPVOID pMemory;
} StdMemAllocator;

static inline StdMemAllocator *StdMemAllocator_from_IMemAllocator(IMemAllocator * iface)
{
    return CONTAINING_RECORD(iface, StdMemAllocator, base.IMemAllocator_iface);
}

static HRESULT StdMemAllocator_Alloc(IMemAllocator * iface)
{
    StdMemAllocator *This = StdMemAllocator_from_IMemAllocator(iface);
    StdMediaSample2 * pSample = NULL;
    SYSTEM_INFO si;
    LONG i;

    assert(list_empty(&This->base.free_list));

    /* check alignment */
    GetSystemInfo(&si);

    /* we do not allow a courser alignment than the OS page size */
    if ((si.dwPageSize % This->base.props.cbAlign) != 0)
        return VFW_E_BADALIGN;

    /* FIXME: each sample has to have its buffer start on the right alignment.
     * We don't do this at the moment */

    /* allocate memory */
    This->pMemory = VirtualAlloc(NULL, (This->base.props.cbBuffer + This->base.props.cbPrefix) * This->base.props.cBuffers, MEM_COMMIT, PAGE_READWRITE);

    if (!This->pMemory)
        return E_OUTOFMEMORY;

    for (i = This->base.props.cBuffers - 1; i >= 0; i--)
    {
        /* pbBuffer does not start at the base address, it starts at base + cbPrefix */
        BYTE * pbBuffer = (BYTE *)This->pMemory + i * (This->base.props.cbBuffer + This->base.props.cbPrefix) + This->base.props.cbPrefix;
        
        StdMediaSample2_Construct(pbBuffer, This->base.props.cbBuffer, iface, &pSample);

        list_add_head(&This->base.free_list, &pSample->listentry);
    }

    return S_OK;
}

static HRESULT StdMemAllocator_Free(IMemAllocator * iface)
{
    StdMemAllocator *This = StdMemAllocator_from_IMemAllocator(iface);
    struct list * cursor;

    if (!list_empty(&This->base.used_list))
    {
        WARN("Freeing allocator with outstanding samples!\n");
        while ((cursor = list_head(&This->base.used_list)) != NULL)
        {
            StdMediaSample2 *pSample;
            list_remove(cursor);
            pSample = LIST_ENTRY(cursor, StdMediaSample2, listentry);
            pSample->pParent = NULL;
        }
    }

    while ((cursor = list_head(&This->base.free_list)) != NULL)
    {
        list_remove(cursor);
        StdMediaSample2_Delete(LIST_ENTRY(cursor, StdMediaSample2, listentry));
    }
    
    /* free memory */
    if (!VirtualFree(This->pMemory, 0, MEM_RELEASE))
    {
        ERR("Failed to free memory, error %lu.\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static void StdMemAllocator_Destroy(IMemAllocator *iface)
{
    StdMemAllocator *This = StdMemAllocator_from_IMemAllocator(iface);

    This->csState.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->csState);

    CoTaskMemFree(This);
}

HRESULT mem_allocator_create(IUnknown *lpUnkOuter, IUnknown **out)
{
    StdMemAllocator * pMemAlloc;
    HRESULT hr;

    if (lpUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(pMemAlloc = CoTaskMemAlloc(sizeof(*pMemAlloc))))
        return E_OUTOFMEMORY;

    InitializeCriticalSectionEx(&pMemAlloc->csState, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    pMemAlloc->csState.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": StdMemAllocator.csState");

    pMemAlloc->pMemory = NULL;

    if (SUCCEEDED(hr = BaseMemAllocator_Init(StdMemAllocator_Alloc, StdMemAllocator_Free, NULL, NULL, NULL, StdMemAllocator_Destroy, &pMemAlloc->csState, &pMemAlloc->base)))
        *out = (IUnknown *)&pMemAlloc->base.IMemAllocator_iface;
    else
        CoTaskMemFree(pMemAlloc);

    return hr;
}
