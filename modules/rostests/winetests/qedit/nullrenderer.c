/*
 * Null renderer filter unit tests
 *
 * Copyright 2018 Zebediah Figura
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
#include "wine/strmbase.h"
#include "wine/test.h"

static IBaseFilter *create_null_renderer(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return ret;
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
    IBaseFilter *filter = create_null_renderer();
    ULONG ref;
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    todo_wine check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, L"In", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_null_renderer();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[2];
    HRESULT hr;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IBaseFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);
    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_find_pin(void)
{
    IBaseFilter *filter = create_null_renderer();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_null_renderer();
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"In"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"In"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IBaseFilter)
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
    IBaseFilter *filter, *filter2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_NullRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_NullRenderer, &test_outer, CLSCTX_INPROC_SERVER,
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

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter2 == (IBaseFilter *)0xdeadbeef, "Got unexpected IBaseFilter %p.\n", filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

static void test_media_types(void)
{
    IBaseFilter *filter = create_null_renderer();
    AM_MEDIA_TYPE mt = {}, *pmt;
    IEnumMediaTypes *enummt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_null_renderer();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"", &testsource_ops);
}

static void test_allocator(IMemInputPin *input)
{
    IMemAllocator *req_allocator, *ret_allocator;
    ALLOCATOR_PROPERTIES props;
    HRESULT hr;

    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        hr = IMemAllocator_GetProperties(ret_allocator, &props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!props.cBuffers, "Got %ld buffers.\n", props.cBuffers);
        ok(!props.cbBuffer, "Got size %ld.\n", props.cbBuffer);
        ok(!props.cbAlign, "Got alignment %ld.\n", props.cbAlign);
        ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

        hr = IMemInputPin_NotifyAllocator(input, ret_allocator, TRUE);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMemAllocator_Release(ret_allocator);
    }

    hr = IMemInputPin_NotifyAllocator(input, NULL, TRUE);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&req_allocator);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_allocator == req_allocator, "Allocators didn't match.\n");

    IMemAllocator_Release(req_allocator);
    IMemAllocator_Release(ret_allocator);
}

struct frame_thread_params
{
    IMemInputPin *sink;
    IMediaSample *sample;
};

static DWORD WINAPI frame_thread(void *arg)
{
    struct frame_thread_params *params = arg;
    HRESULT hr;

    if (winetest_debug > 1) trace("%04lx: Sending frame.\n", GetCurrentThreadId());
    hr = IMemInputPin_Receive(params->sink, params->sample);
    if (winetest_debug > 1) trace("%04lx: Returned %#lx.\n", GetCurrentThreadId(), hr);
    IMediaSample_Release(params->sample);
    free(params);
    return hr;
}

static HANDLE send_frame_time(IMemInputPin *sink, REFERENCE_TIME start_time, unsigned char color)
{
    struct frame_thread_params *params = malloc(sizeof(*params));
    IMemAllocator *allocator;
    REFERENCE_TIME end_time;
    IMediaSample *sample;
    HANDLE thread;
    HRESULT hr;
    BYTE *data;

    hr = IMemInputPin_GetAllocator(sink, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    memset(data, color, 32 * 16 * 2);

    hr = IMediaSample_SetActualDataLength(sample, 32 * 16 * 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start_time *= 10000000;
    end_time = start_time + 10000000;
    hr = IMediaSample_SetTime(sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params->sink = sink;
    params->sample = sample;
    thread = CreateThread(NULL, 0, frame_thread, params, 0, NULL);

    IMemAllocator_Release(allocator);
    return thread;
}

static HANDLE send_frame(IMemInputPin *sink)
{
    return send_frame_time(sink, 0, 0x55); /* purple */
}

static HRESULT join_thread_(int line, HANDLE thread)
{
    DWORD ret;
    ok_(__FILE__, line)(!WaitForSingleObject(thread, 1000), "Wait failed.\n");
    GetExitCodeThread(thread, &ret);
    CloseHandle(thread);
    return ret;
}
#define join_thread(a) join_thread_(__LINE__, a)

static void test_filter_state(IMemInputPin *input, IFilterGraph2 *graph)
{
    IMemAllocator *allocator;
    IMediaControl *control;
    IMediaSample *sample;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* The renderer is not fully paused until it receives a sample. The thread
     * sending the sample blocks in IMemInputPin_Receive() until the filter is
     * stopped or run. */

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* The sink will decommit our allocator for us when stopping, and recommit
     * it when pausing. */
    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    todo_wine ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);
    if (hr == S_OK) IMediaSample_Release(sample);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemAllocator_Release(allocator);
    IMediaControl_Release(control);
}

static void test_flushing(IPin *pin, IMemInputPin *input, IFilterGraph2 *graph)
{
    IMediaControl *control;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* We dropped the sample we were holding, so now we need a new one... */

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame(input));
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(control);
}

static unsigned int check_event_code(IMediaEvent *eventsrc, DWORD timeout, LONG expected_code, LONG_PTR expected1, LONG_PTR expected2)
{
    LONG_PTR param1, param2;
    unsigned int ret = 0;
    HRESULT hr;
    LONG code;

    while ((hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, timeout)) == S_OK)
    {
        if (code == expected_code)
        {
            ok(param1 == expected1, "Got param1 %#Ix.\n", param1);
            ok(param2 == expected2, "Got param2 %#Ix.\n", param2);
            ret++;
        }
        IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
        timeout = 0;
    }
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    return ret;
}

static unsigned int check_ec_complete(IMediaEvent *eventsrc, DWORD timeout)
{
    return check_event_code(eventsrc, timeout, EC_COMPLETE, S_OK, 0);
}

static void test_connect_pin(void)
{
    static const AM_MEDIA_TYPE req_mt =
    {
        .majortype = {0x111},
        .subtype = {0x222},
        .formattype = {0x333},
    };
    ALLOCATOR_PROPERTIES req_props = {1, 32 * 16 * 4, 1, 0}, ret_props;
    IBaseFilter *filter = create_null_renderer();
    struct testfilter source;
    IMemAllocator *allocator;
    IMediaControl *control;
    IFilterGraph2 *graph;
    IMemInputPin *input;
    AM_MEDIA_TYPE mt;
    IPin *pin, *peer;
    HRESULT hr;
    ULONG ref;

    testfilter_init(&source);

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"sink");
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    IBaseFilter_FindPin(filter, L"In", &pin);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &source.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!memcmp(&mt, &req_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    test_allocator(input);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMemAllocator_Release(allocator);

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_filter_state(input, graph);
    test_flushing(pin, input, graph);

    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(source.source.pin.peer == pin, "Got peer %p.\n", peer);
    IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    IMemInputPin_Release(input);
    IPin_Release(pin);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_unconnected_eos(void)
{
    IBaseFilter *filter = create_null_renderer();
    IFilterGraph2 *graph = create_graph();
    IMediaControl *control;
    IMediaEvent *eventsrc;
    unsigned int ret;
    HRESULT hr;
    ULONG ref;

    hr = IFilterGraph2_AddFilter(graph, filter, L"renderer");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(nullrenderer)
{
    IBaseFilter *filter;
    HRESULT hr;

    CoInitialize(NULL);

    if (FAILED(hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        /* qedit.dll does not exist on 2003. */
        win_skip("Failed to create null renderer filter, hr %#lx.\n", hr);
        return;
    }
    IBaseFilter_Release(filter);

    test_interfaces();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_aggregation();
    test_media_types();
    test_enum_media_types();
    test_connect_pin();
    test_unconnected_eos();

    CoUninitialize();
}
