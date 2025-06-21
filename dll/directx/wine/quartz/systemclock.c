/*
 * Implementation of IReferenceClock
 *
 * Copyright 2004 Raphael Junqueira
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

#include "quartz_private.h"

#include "wine/debug.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static LONG cookie_counter;

struct advise_sink
{
    struct list entry;
    HANDLE handle;
    REFERENCE_TIME due_time, period;
    int cookie;
};

struct system_clock
{
    IReferenceClock IReferenceClock_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;

    LONG thread_created;
    BOOL thread_stopped;
    HANDLE thread;
    REFERENCE_TIME last_time;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;

    struct list sinks;
};

static REFERENCE_TIME get_current_time(void)
{
    return (REFERENCE_TIME)timeGetTime() * 10000;
}

static inline struct system_clock *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct system_clock, IUnknown_inner);
}

static HRESULT WINAPI system_clock_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    TRACE("clock %p, iid %s, out %p.\n", clock, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IReferenceClock))
        *out = &clock->IReferenceClock_iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI system_clock_inner_AddRef(IUnknown *iface)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&clock->refcount);

    TRACE("%p increasing refcount to %lu.\n", clock, refcount);

    return refcount;
}

static ULONG WINAPI system_clock_inner_Release(IUnknown *iface)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);
    struct advise_sink *sink, *cursor;

    TRACE("%p decreasing refcount to %lu.\n", clock, refcount);

    if (!refcount)
    {
        if (clock->thread)
        {
            EnterCriticalSection(&clock->cs);
            clock->thread_stopped = TRUE;
            LeaveCriticalSection(&clock->cs);
            WakeConditionVariable(&clock->cv);
            WaitForSingleObject(clock->thread, INFINITE);
            CloseHandle(clock->thread);
        }

        LIST_FOR_EACH_ENTRY_SAFE(sink, cursor, &clock->sinks, struct advise_sink, entry)
        {
            list_remove(&sink->entry);
            heap_free(sink);
        }

        clock->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&clock->cs);
        heap_free(clock);
    }
    return refcount;
}

static const IUnknownVtbl system_clock_inner_vtbl =
{
    system_clock_inner_QueryInterface,
    system_clock_inner_AddRef,
    system_clock_inner_Release,
};

static inline struct system_clock *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct system_clock, IReferenceClock_iface);
}

static DWORD WINAPI SystemClockAdviseThread(void *param)
{
    struct system_clock *clock = param;
    struct advise_sink *sink, *cursor;
    REFERENCE_TIME current_time;

    TRACE("Starting advise thread for clock %p.\n", clock);
    SetThreadDescription(GetCurrentThread(), L"wine_qz_clock_advise");

    for (;;)
    {
        DWORD timeout = INFINITE;

        EnterCriticalSection(&clock->cs);

        current_time = get_current_time();

        LIST_FOR_EACH_ENTRY_SAFE(sink, cursor, &clock->sinks, struct advise_sink, entry)
        {
            if (sink->due_time <= current_time)
            {
                if (sink->period)
                {
                    DWORD periods = ((current_time - sink->due_time) / sink->period) + 1;
                    ReleaseSemaphore(sink->handle, periods, NULL);
                    sink->due_time += periods * sink->period;
                }
                else
                {
                    SetEvent(sink->handle);
                    list_remove(&sink->entry);
                    heap_free(sink);
                    continue;
                }
            }

            timeout = min(timeout, (sink->due_time - current_time) / 10000);
        }

        SleepConditionVariableCS(&clock->cv, &clock->cs, timeout);
        if (clock->thread_stopped)
        {
            LeaveCriticalSection(&clock->cs);
            return 0;
        }
        LeaveCriticalSection(&clock->cs);
    }
}

static HRESULT add_sink(struct system_clock *clock, DWORD_PTR handle,
        REFERENCE_TIME due_time, REFERENCE_TIME period, DWORD_PTR *cookie)
{
    struct advise_sink *sink;

    if (!handle)
        return E_INVALIDARG;

    if (!cookie)
        return E_POINTER;

    if (!(sink = heap_alloc_zero(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->handle = (HANDLE)handle;
    sink->due_time = due_time;
    sink->period = period;
    sink->cookie = InterlockedIncrement(&cookie_counter);
    *cookie = sink->cookie;

    EnterCriticalSection(&clock->cs);
    list_add_tail(&clock->sinks, &sink->entry);
    LeaveCriticalSection(&clock->cs);

    if (!InterlockedCompareExchange(&clock->thread_created, TRUE, FALSE))
    {
        clock->thread = CreateThread(NULL, 0, SystemClockAdviseThread, clock, 0, NULL);
    }
    WakeConditionVariable(&clock->cv);

    return S_OK;
}

static HRESULT WINAPI SystemClockImpl_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_QueryInterface(clock->outer_unk, iid, out);
}

static ULONG WINAPI SystemClockImpl_AddRef(IReferenceClock *iface)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_AddRef(clock->outer_unk);
}

static ULONG WINAPI SystemClockImpl_Release(IReferenceClock *iface)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_Release(clock->outer_unk);
}

static HRESULT WINAPI SystemClockImpl_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    REFERENCE_TIME ret;
    HRESULT hr;

    if (!time) {
        return E_POINTER;
    }

    ret = get_current_time();

    EnterCriticalSection(&clock->cs);

    hr = (ret == clock->last_time) ? S_FALSE : S_OK;
    *time = clock->last_time = ret;

    LeaveCriticalSection(&clock->cs);

    TRACE("clock %p, time %p, returning %s.\n", clock, time, debugstr_time(ret));
    return hr;
}

static HRESULT WINAPI SystemClockImpl_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, base %s, offset %s, event %#Ix, cookie %p.\n",
            clock, debugstr_time(base), debugstr_time(offset), event, cookie);

    if (base + offset <= 0)
        return E_INVALIDARG;

    return add_sink(clock, event, base + offset, 0, cookie);
}

static HRESULT WINAPI SystemClockImpl_AdvisePeriodic(IReferenceClock* iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, start %s, period %s, semaphore %#Ix, cookie %p.\n",
            clock, debugstr_time(start), debugstr_time(period), semaphore, cookie);

    if (start <= 0 || period <= 0)
        return E_INVALIDARG;

    return add_sink(clock, semaphore, start, period, cookie);
}

static HRESULT WINAPI SystemClockImpl_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    struct advise_sink *sink;

    TRACE("clock %p, cookie %#Ix.\n", clock, cookie);

    EnterCriticalSection(&clock->cs);

    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct advise_sink, entry)
    {
        if (sink->cookie == cookie)
        {
            list_remove(&sink->entry);
            heap_free(sink);
            LeaveCriticalSection(&clock->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&clock->cs);

    return S_FALSE;
}

static const IReferenceClockVtbl SystemClock_vtbl =
{
    SystemClockImpl_QueryInterface,
    SystemClockImpl_AddRef,
    SystemClockImpl_Release,
    SystemClockImpl_GetTime,
    SystemClockImpl_AdviseTime,
    SystemClockImpl_AdvisePeriodic,
    SystemClockImpl_Unadvise
};

HRESULT system_clock_create(IUnknown *outer, IUnknown **out)
{
    struct system_clock *object;
  
    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IReferenceClock_iface.lpVtbl = &SystemClock_vtbl;
    object->IUnknown_inner.lpVtbl = &system_clock_inner_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;
    list_init(&object->sinks);
    InitializeCriticalSectionEx(&object->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SystemClockImpl.cs");

    TRACE("Created system clock %p.\n", object);
    *out = &object->IUnknown_inner;

    return S_OK;
}
