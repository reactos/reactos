/*
 * System clock unit tests
 *
 * Copyright (C) 2007 Alex Villacís Lasso
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

#define COBJMACROS
#include "dshow.h"
#include "wine/test.h"

static IReferenceClock *create_system_clock(void)
{
    IReferenceClock *clock = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER,
            &IID_IReferenceClock, (void **)&clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return clock;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IReferenceClock *clock = create_system_clock();
    ULONG ref;

    check_interface(clock, &IID_IReferenceClock, TRUE);
    check_interface(clock, &IID_IUnknown, TRUE);

    check_interface(clock, &IID_IDirectDraw, FALSE);

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IReferenceClock)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IReferenceClock *clock, *clock2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    clock = (IReferenceClock *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_SystemClock, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IReferenceClock, (void **)&clock);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!clock, "Got interface %p.\n", clock);

    hr = CoCreateInstance(&CLSID_SystemClock, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IReferenceClock, (void **)&clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_QueryInterface(clock, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IReferenceClock_QueryInterface(clock, &IID_IReferenceClock, (void **)&clock2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock2 == (IReferenceClock *)0xdeadbeef, "Got unexpected IReferenceClock %p.\n", clock2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IReferenceClock_QueryInterface(clock, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IReferenceClock_Release(clock);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

static void test_get_time(void)
{
    IReferenceClock *clock = create_system_clock();
    REFERENCE_TIME time1, ticks, time2;
    HRESULT hr;
    ULONG ref;

    hr = IReferenceClock_GetTime(clock, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_GetTime(clock, &time1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time1 % 10000 == 0, "Expected no less than 1ms coarseness, but got time %s.\n",
            wine_dbgstr_longlong(time1));

    ticks = (REFERENCE_TIME)timeGetTime() * 10000;

    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == (time2 == time1 ? S_FALSE : S_OK), "Got hr %#lx.\n", hr);
    ok(time2 % 10000 == 0, "Expected no less than 1ms coarseness, but got time %s.\n",
            wine_dbgstr_longlong(time1));

    ok(time2 >= ticks && ticks >= time1, "Got timestamps %I64d, %I64d, %I64d.\n", time1, ticks, time2);

    Sleep(100);
    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time2 - time1 > 80 * 10000, "Expected about %s, but got %s.\n",
            wine_dbgstr_longlong(time1 + 80 * 10000), wine_dbgstr_longlong(time2));

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_advise(void)
{
    IReferenceClock *clock = create_system_clock();
    HANDLE event, semaphore;
    REFERENCE_TIME current;
    DWORD_PTR cookie;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    semaphore = CreateSemaphoreA(NULL, 0, 10, NULL);

    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, -1000 * 10000, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(WaitForSingleObject(event, 460) == WAIT_TIMEOUT, "Event should not be signaled.\n");
    ok(!WaitForSingleObject(event, 80), "Event should be signaled.\n");

    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    ResetEvent(event);
    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(WaitForSingleObject(event, 540) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    ResetEvent(event);
    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IReferenceClock_AdviseTime(clock, current + 500 * 10000, 0, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(WaitForSingleObject(event, 460) == WAIT_TIMEOUT, "Event should not be signaled.\n");
    ok(!WaitForSingleObject(event, 80), "Event should be signaled.\n");

    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 500 * 10000, (HSEMAPHORE)semaphore, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 0, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, -500 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, -500 * 10000, 1000 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 100 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!WaitForSingleObject(semaphore, 50), "Semaphore should be signaled.\n");
    for (i = 0; i < 5; ++i)
        ok(!WaitForSingleObject(semaphore, 500), "Semaphore should be signaled.\n");

    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(WaitForSingleObject(semaphore, 200) == WAIT_TIMEOUT, "Semaphore should not be signaled.\n");

    ResetEvent(event);
    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    CloseHandle(event);
    CloseHandle(semaphore);
}

START_TEST(systemclock)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_get_time();
    test_advise();

    CoUninitialize();
}
