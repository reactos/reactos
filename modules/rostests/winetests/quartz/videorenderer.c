/*
 * Unit tests for Video Renderer functions
 *
 * Copyright (C) 2007 Google (Lei Zhang)
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
#include "videoacc.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static IBaseFilter *create_video_renderer(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
        && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
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
    IBaseFilter *filter = create_video_renderer();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IBasicVideo, TRUE);
    todo_wine check_interface(filter, &IID_IBasicVideo2, TRUE);
    todo_wine check_interface(filter, &IID_IDirectDrawVideo, TRUE);
    todo_wine check_interface(filter, &IID_IKsPropertySet, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IQualityControl, TRUE);
    todo_wine check_interface(filter, &IID_IQualProp, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);
    check_interface(filter, &IID_IVideoWindow, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IAMVideoAccelerator, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IDispatch, FALSE);
    check_interface(filter, &IID_IOverlay, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);

    IBaseFilter_FindPin(filter, L"In", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IPinConnection, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAMVideoAccelerator, FALSE);
    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);
    IBaseFilter_Release(filter);
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
    hr = CoCreateInstance(&CLSID_VideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_VideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
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

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_video_renderer();
    IEnumPins *enum1, *enum2;
    IPin *pins[2];
    ULONG count;
    HRESULT hr;
    ULONG ref;

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
    IBaseFilter *filter = create_video_renderer();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_video_renderer();
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
    ok(!wcscmp(info.achName, L"Input"), "Got name %s.\n", debugstr_w(info.achName));
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

static void test_media_types(void)
{
    IBaseFilter *filter = create_video_renderer();
    AM_MEDIA_TYPE *mt, req_mt = {{0}};
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, BI_RGB}
    };
    IEnumMediaTypes *enummt;
    unsigned int i;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB8,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.cbFormat = sizeof(VIDEOINFOHEADER);
    req_mt.pbFormat = (BYTE *)&vih;

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IPin_QueryAccept(pin, &req_mt);
        ok(hr == S_OK, "Got hr %#lx for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));
    }

    req_mt.subtype = MEDIASUBTYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_RGB24;

    req_mt.majortype = MEDIATYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    req_mt.majortype = MEDIATYPE_Video;

    req_mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    req_mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_video_renderer();
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
    IMediaSeeking IMediaSeeking_iface;
};

static inline struct testfilter *impl_from_BaseFilter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_BaseFilter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_interface = testsource_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static struct testfilter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSeeking_iface);
}

static HRESULT WINAPI testseek_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI testseek_AddRef(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI testseek_Release(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI testseek_GetCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_CheckCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    if (winetest_debug > 1) trace("IsFormatSupported()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    if (winetest_debug > 1) trace("%p->QueryPreferredFormat()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    if (winetest_debug > 1) trace("%p->GetTimeFormat()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    if (winetest_debug > 1) trace("GetStopPosition()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    if (winetest_debug > 1) trace("GetCurrentPosition()\n");
    return 0xdeadbeef;
}

static HRESULT WINAPI testseek_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
    const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    if (winetest_debug > 1) trace("ConvertTimeFormat()\n");
    ok(IsEqualGUID(source_format, &TIME_FORMAT_MEDIA_TIME),
            "Got source format %s.\n", debugstr_guid(source_format));
    ok(!target_format, "Got target format %s.\n", debugstr_guid(target_format));
    *target = source;
    return S_OK;
}

static HRESULT WINAPI testseek_SetPositions(IMediaSeeking *iface, LONGLONG *current,
    DWORD current_flags, LONGLONG *stop, DWORD stop_flags )
{
    if (winetest_debug > 1) trace("SetPositions()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPositions(IMediaSeeking *iface, LONGLONG *current, LONGLONG *stop)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetAvailable(IMediaSeeking *iface, LONGLONG *earliest, LONGLONG *latest)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetRate(IMediaSeeking *iface, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetRate(IMediaSeeking *iface, double *rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    if (winetest_debug > 1) trace("%p->GetPreroll()\n", iface);
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl testseek_vtbl =
{
    testseek_QueryInterface,
    testseek_AddRef,
    testseek_Release,
    testseek_GetCapabilities,
    testseek_CheckCapabilities,
    testseek_IsFormatSupported,
    testseek_QueryPreferredFormat,
    testseek_GetTimeFormat,
    testseek_IsUsingTimeFormat,
    testseek_SetTimeFormat,
    testseek_GetDuration,
    testseek_GetStopPosition,
    testseek_GetCurrentPosition,
    testseek_ConvertTimeFormat,
    testseek_SetPositions,
    testseek_GetPositions,
    testseek_GetAvailable,
    testseek_SetRate,
    testseek_GetRate,
    testseek_GetPreroll,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"", &testsource_ops);
    filter->IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
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

    hr = IMediaSample_SetPreroll(sample, TRUE);
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

static void test_filter_state(IMemInputPin *input, IMediaControl *control)
{
    IMemAllocator *allocator;
    IMediaSample *sample;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

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

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

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
}

static void test_flushing(IPin *pin, IMemInputPin *input, IMediaControl *control)
{
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_GetState(control, 1000, &state);
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
}

static void test_sample_time(IBaseFilter *filter, IPin *pin, IMemInputPin *input, IMediaControl *control)
{
    IMediaSeeking *seeking;
    REFERENCE_TIME time;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IBaseFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == 0xdeadbeef, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1, 0x11); /* dark blue */

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 10000000, "Got time %s.\n", wine_dbgstr_longlong(time));

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(thread, 500) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Sample time is relative to the time passed to Run(). Thus a sample
     * stamped at or earlier than 1s will now be displayed immediately, because
     * that time has already passed.
     * One may manually verify that all of the frames in this function are
     * rendered, including (by adding a Sleep() after sending the frame) the
     * dark and light green frames. Thus the video renderer does not attempt to
     * drop any frames that it considers late. This remains true if the frames
     * are marked as discontinuous. */

    hr = join_thread(send_frame_time(input, 1, 0x22)); /* dark green */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame_time(input, 0, 0x33)); /* light green */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame_time(input, -2, 0x44)); /* dark red */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 2, 0x66); /* orange */
    ok(WaitForSingleObject(thread, 500) == WAIT_TIMEOUT, "Thread should block in Receive().\n");
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1000000, 0xff); /* white */
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    /* If the frame makes it to Receive() in time to be rendered, we get S_OK. */
    ok(hr == S_OK || hr == E_FAIL, "Got hr %#lx.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1000000, 0xff);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    /* If the frame makes it to Receive() in time to be rendered, we get S_OK. */
    ok(hr == S_OK || hr == E_FAIL, "Got hr %#lx.\n", hr);

    IMediaSeeking_Release(seeking);
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

static inline unsigned int check_ec_complete(IMediaEvent *eventsrc, DWORD timeout)
{
    return check_event_code(eventsrc, timeout, EC_COMPLETE, S_OK, 0);
}

static void test_eos(IPin *pin, IMemInputPin *input, IMediaControl *control)
{
    IMediaEvent *eventsrc;
    OAFilterState state;
    HRESULT hr;
    BOOL ret;

    IMediaControl_QueryInterface(control, &IID_IMediaEvent, (void **)&eventsrc);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = join_thread(send_frame(input));
    todo_wine ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Expected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* We do not receive an EC_COMPLETE notification until the last sample is
     * done rendering. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");
    ret = check_ec_complete(eventsrc, 1600);
    todo_wine ok(ret == 1, "Expected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* Test sending EOS while flushing. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_EndOfStream(pin);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* Test sending EOS and then flushing or stopping. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    IMediaEvent_Release(eventsrc);
}

static void test_current_image(IBaseFilter *filter, IMemInputPin *input,
        IMediaControl *control, const BITMAPINFOHEADER *expect_bih)
{
    LONG buffer[(sizeof(BITMAPINFOHEADER) + 32 * 16 * 2) / 4];
    const BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)buffer;
    OAFilterState state;
    IBasicVideo *video;
    unsigned int i;
    HANDLE thread;
    HRESULT hr;
    LONG size;

    IBaseFilter_QueryInterface(filter, &IID_IBasicVideo, (void **)&video);

    hr = IBasicVideo_GetCurrentImage(video, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetCurrentImage(video, NULL, buffer);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    size = 0xdeadbeef;
    hr = IBasicVideo_GetCurrentImage(video, &size, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(size == sizeof(BITMAPINFOHEADER) + 32 * 16 * 2, "Got size %ld.\n", size);

    size = 0xdeadbeef;
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == VFW_E_NOT_PAUSED, "Got hr %#lx.\n", hr);
    ok(size == 0xdeadbeef, "Got size %ld.\n", size);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    size = 0xdeadbeef;
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);
    ok(size == 0xdeadbeef, "Got size %ld.\n", size);

    thread = send_frame(input);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    size = sizeof(BITMAPINFOHEADER) + 32 * 16 * 2 - 1;
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == E_OUTOFMEMORY, "Got hr %#lx.\n", hr);
    ok(size == sizeof(BITMAPINFOHEADER) + 32 * 16 * 2 - 1, "Got size %ld.\n", size);

    size = sizeof(BITMAPINFOHEADER) + 32 * 16 * 2;
    memset(buffer, 0xcc, sizeof(buffer));
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == sizeof(BITMAPINFOHEADER) + 32 * 16 * 2, "Got size %ld.\n", size);
    ok(!memcmp(bih, expect_bih, sizeof(BITMAPINFOHEADER)), "Bitmap headers didn't match.\n");
    for (i = 0; i < 32 * 16 * 2; ++i)
    {
        const unsigned char *data = (unsigned char *)buffer + sizeof(BITMAPINFOHEADER);
        ok(data[i] == 0x55, "Got unexpected byte %02x at %u.\n", data[i], i);
    }

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    join_thread(thread);

    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == VFW_E_NOT_PAUSED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IBasicVideo_Release(video);
}

static inline unsigned int check_ec_userabort(IMediaEvent *eventsrc, DWORD timeout)
{
    return check_event_code(eventsrc, timeout, EC_USERABORT, 0, 0);
}

static void test_window_close(IPin *pin, IMemInputPin *input, IMediaControl *control)
{
    IMediaEvent *eventsrc;
    OAFilterState state;
    IOverlay *overlay;
    HANDLE thread;
    HRESULT hr;
    HWND hwnd;
    BOOL ret;

    IMediaControl_QueryInterface(control, &IID_IMediaEvent, (void **)&eventsrc);
    IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);

    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IOverlay_Release(overlay);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(!ret, "Got unexpected EC_USERABORT.\n");

    SendMessageW(hwnd, WM_CLOSE, 0, 0);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(ret == 1, "Expected EC_USERABORT.\n");

    ok(IsWindow(hwnd), "Window should exist.\n");
    ok(!IsWindowVisible(hwnd), "Window should be visible.\n");

    thread = send_frame(input);
    ret = WaitForSingleObject(thread, 1000);
    todo_wine ok(ret == WAIT_OBJECT_0, "Wait failed\n");
    if (ret == WAIT_OBJECT_0)
    {
        GetExitCodeThread(thread, (DWORD *)&hr);
        ok(hr == E_UNEXPECTED, "Got hr %#lx.\n", hr);
    }
    CloseHandle(thread);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(!ret, "Got unexpected EC_USERABORT.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(!ret, "Got unexpected EC_USERABORT.\n");

    /* We receive an EC_USERABORT notification immediately. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(!ret, "Got unexpected EC_USERABORT.\n");

    SendMessageW(hwnd, WM_CLOSE, 0, 0);

    ret = check_ec_userabort(eventsrc, 0);
    ok(ret == 1, "Expected EC_USERABORT.\n");

    ok(IsWindow(hwnd), "Window should exist.\n");
    ok(!IsWindowVisible(hwnd), "Window should be visible.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ret = check_ec_userabort(eventsrc, 0);
    ok(!ret, "Got unexpected EC_USERABORT.\n");

    IMediaEvent_Release(eventsrc);
}

static void test_connect_pin(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biBitCount = 16,
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    ALLOCATOR_PROPERTIES req_props = {1, 32 * 16 * 2, 1, 0}, ret_props;
    IBaseFilter *filter = create_video_renderer();
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    IMemAllocator *allocator;
    IMediaControl *control;
    IMemInputPin *input;
    AM_MEDIA_TYPE mt;
    IPin *pin, *peer;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB8,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    testfilter_init(&source);

    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    IBaseFilter_FindPin(filter, L"In", &pin);

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
        ok(hr == S_OK, "Got hr %#lx for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));

        hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IFilterGraph2_Disconnect(graph, pin);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.formattype = FORMAT_VideoInfo;

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
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    test_allocator(input);

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
    hr = IMemInputPin_NotifyAllocator(input, allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_filter_state(input, control);
    test_flushing(pin, input, control);
    test_sample_time(filter, pin, input, control);
    test_eos(pin, input, control);
    test_current_image(filter, input, control, &vih.bmiHeader);
    test_window_close(pin, input, control);

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

    ref = IMemAllocator_Release(allocator);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
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

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_video_renderer();
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_overlay(void)
{
    IBaseFilter *filter = create_video_renderer();
    IOverlay *overlay;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    HWND hwnd;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hwnd = (HWND)0xdeadbeef;
    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(hwnd && hwnd != (HWND)0xdeadbeef, "Got invalid window %p.\n", hwnd);

    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    int diff = 200;
    DWORD time;
    MSG msg;

    time = GetTickCount() + diff;
    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (winetest_debug > 1)
        trace("hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix.\n", hwnd, msg, wparam, lparam);

    if (wparam == 0xdeadbeef)
        return 0;

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_video_window_caption(IVideoWindow *window, HWND hwnd)
{
    WCHAR text[50];
    BSTR caption;
    HRESULT hr;

    hr = IVideoWindow_get_Caption(window, &caption);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(caption, L"ActiveMovie Window"), "Got caption %s.\n", wine_dbgstr_w(caption));
    SysFreeString(caption);

    GetWindowTextW(hwnd, text, ARRAY_SIZE(text));
    ok(!wcscmp(text, L"ActiveMovie Window"), "Got caption %s.\n", wine_dbgstr_w(text));

    caption = SysAllocString(L"foo");
    hr = IVideoWindow_put_Caption(window, caption);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SysFreeString(caption);

    hr = IVideoWindow_get_Caption(window, &caption);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(caption, L"foo"), "Got caption %s.\n", wine_dbgstr_w(caption));
    SysFreeString(caption);

    GetWindowTextW(hwnd, text, ARRAY_SIZE(text));
    ok(!wcscmp(text, L"foo"), "Got caption %s.\n", wine_dbgstr_w(text));
}

static void test_video_window_style(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HRESULT hr;
    LONG style;

    hr = IVideoWindow_get_WindowStyle(window, &style);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(style == (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW),
            "Got style %#lx.\n", style);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style == (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW),
            "Got style %#lx.\n", style);

    hr = IVideoWindow_put_WindowStyle(window, style | WS_DISABLED);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_HSCROLL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_VSCROLL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_MAXIMIZE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_MINIMIZE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_WindowStyle(window, style & ~WS_CLIPCHILDREN);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowStyle(window, &style);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(style == (WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW), "Got style %#lx.\n", style);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style == (WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW), "Got style %#lx.\n", style);

    flaky_wine
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_get_WindowStyleEx(window, &style);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(style == WS_EX_WINDOWEDGE, "Got style %#lx.\n", style);

    style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == WS_EX_WINDOWEDGE, "Got style %#lx.\n", style);

    hr = IVideoWindow_put_WindowStyleEx(window, style | WS_EX_TRANSPARENT);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowStyleEx(window, &style);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(style == (WS_EX_WINDOWEDGE | WS_EX_TRANSPARENT), "Got style %#lx.\n", style);

    style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (WS_EX_WINDOWEDGE | WS_EX_TRANSPARENT), "Got style %#lx.\n", style);
}

static BOOL CALLBACK top_window_cb(HWND hwnd, LPARAM ctx)
{
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() && (GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE))
    {
        *(HWND *)ctx = hwnd;
        return FALSE;
    }
    return TRUE;
}

static HWND get_top_window(void)
{
    HWND hwnd;
    EnumWindows(top_window_cb, (LPARAM)&hwnd);
    return hwnd;
}

static void test_video_window_state(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HRESULT hr;
    LONG state;
    HWND top;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_HIDE, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %ld.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");

    hr = IVideoWindow_put_WindowState(window, SW_SHOWNA);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_SHOW, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %ld.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    hr = IVideoWindow_put_WindowState(window, SW_MINIMIZE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_MINIMIZE, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %ld.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(IsIconic(hwnd), "Window should be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_RESTORE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_SHOW, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %ld.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_MAXIMIZE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_MAXIMIZE, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %ld.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should be minimized.\n");
    ok(IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_RESTORE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_WindowState(window, SW_HIDE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_HIDE, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %ld.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_Visible(window, OATRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_SHOW, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %ld.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == SW_HIDE, "Got state %ld.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %ld.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_SHOWNA);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_SetWindowForeground(window, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    hr = IVideoWindow_SetWindowForeground(window, OATRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == our_hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == our_hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);
}

static void test_video_window_position(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    LONG left, width, top, height, expect_width, expect_height;
    RECT rect = {0, 0, 640, 480};
    HWND top_hwnd;
    HRESULT hr;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    AdjustWindowRect(&rect, GetWindowLongA(hwnd, GWL_STYLE), FALSE);
    expect_width = rect.right - rect.left;
    expect_height = rect.bottom - rect.top;

    hr = IVideoWindow_put_Left(window, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_put_Top(window, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 0, "Got left %ld.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == 0, "Got top %ld.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == expect_width, "Got width %ld.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == expect_height, "Got height %ld.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 0, "Got left %ld.\n", left);
    ok(top == 0, "Got top %ld.\n", top);
    ok(width == expect_width, "Got width %ld.\n", width);
    ok(height == expect_height, "Got height %ld.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 0, "Got window left %ld.\n", rect.left);
    ok(rect.top == 0, "Got window top %ld.\n", rect.top);
    ok(rect.right == expect_width, "Got window right %ld.\n", rect.right);
    ok(rect.bottom == expect_height, "Got window bottom %ld.\n", rect.bottom);

    hr = IVideoWindow_put_Left(window, 10);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 10, "Got left %ld.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == 0, "Got top %ld.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == expect_width, "Got width %ld.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == expect_height, "Got height %ld.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 10, "Got left %ld.\n", left);
    ok(top == 0, "Got top %ld.\n", top);
    ok(width == expect_width, "Got width %ld.\n", width);
    ok(height == expect_height, "Got height %ld.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 10, "Got window left %ld.\n", rect.left);
    ok(rect.top == 0, "Got window top %ld.\n", rect.top);
    ok(rect.right == 10 + expect_width, "Got window right %ld.\n", rect.right);
    ok(rect.bottom == expect_height, "Got window bottom %ld.\n", rect.bottom);

    hr = IVideoWindow_put_Height(window, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 10, "Got left %ld.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == 0, "Got top %ld.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == expect_width, "Got width %ld.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == 200, "Got height %ld.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 10, "Got left %ld.\n", left);
    ok(top == 0, "Got top %ld.\n", top);
    ok(width == expect_width, "Got width %ld.\n", width);
    ok(height == 200, "Got height %ld.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 10, "Got window left %ld.\n", rect.left);
    ok(rect.top == 0, "Got window top %ld.\n", rect.top);
    ok(rect.right == 10 + expect_width, "Got window right %ld.\n", rect.right);
    ok(rect.bottom == 200, "Got window bottom %ld.\n", rect.bottom);

    hr = IVideoWindow_SetWindowPosition(window, 100, 200, 300, 400);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 100, "Got left %ld.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == 200, "Got top %ld.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == 300, "Got width %ld.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == 400, "Got height %ld.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 100, "Got left %ld.\n", left);
    ok(top == 200, "Got top %ld.\n", top);
    ok(width == 300, "Got width %ld.\n", width);
    ok(height == 400, "Got height %ld.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 100, "Got window left %ld.\n", rect.left);
    ok(rect.top == 200, "Got window top %ld.\n", rect.top);
    ok(rect.right == 400, "Got window right %ld.\n", rect.right);
    ok(rect.bottom == 600, "Got window bottom %ld.\n", rect.bottom);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == our_hwnd, "Got top window %p.\n", top_hwnd);
}

static void test_video_window_owner(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HWND parent, top_hwnd;
    LONG style, state;
    OAHWND oahwnd;
    HRESULT hr;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!oahwnd, "Got owner %#Ix.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == GetDesktopWindow(), "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_CHILD), "Got style %#lx.\n", style);

    hr = IVideoWindow_put_Owner(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(oahwnd == (OAHWND)our_hwnd, "Got owner %#Ix.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == our_hwnd, "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & WS_CHILD), "Got style %#lx.\n", style);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == our_hwnd, "Got top window %p.\n", top_hwnd);

    ShowWindow(our_hwnd, SW_HIDE);

    hr = IVideoWindow_put_Visible(window, OATRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == OAFALSE, "Got state %ld.\n", state);

    hr = IVideoWindow_put_Owner(window, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!oahwnd, "Got owner %#Ix.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == GetDesktopWindow(), "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_CHILD), "Got style %#lx.\n", style);

    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == hwnd, "Got top window %p.\n", top_hwnd);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == OATRUE, "Got state %ld.\n", state);
}

struct notify_message_params
{
    IVideoWindow *window;
    HWND hwnd;
    UINT message;
};

static DWORD CALLBACK notify_message_proc(void *arg)
{
    const struct notify_message_params *params = arg;
    HRESULT hr = IVideoWindow_NotifyOwnerMessage(params->window, (OAHWND)params->hwnd, params->message, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return 0;
}

static void test_video_window_messages(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    struct notify_message_params params;
    unsigned int i;
    OAHWND oahwnd;
    HANDLE thread;
    HRESULT hr;
    BOOL ret;
    MSG msg;

    static UINT drain_tests[] =
    {
        WM_MOUSEACTIVATE,
        WM_NCLBUTTONDOWN,
        WM_NCLBUTTONUP,
        WM_NCLBUTTONDBLCLK,
        WM_NCRBUTTONDOWN,
        WM_NCRBUTTONUP,
        WM_NCRBUTTONDBLCLK,
        WM_NCMBUTTONDOWN,
        WM_NCMBUTTONUP,
        WM_NCMBUTTONDBLCLK,
        WM_KEYDOWN,
        WM_KEYUP,
        WM_MOUSEMOVE,
        WM_LBUTTONDOWN,
        WM_LBUTTONUP,
        WM_LBUTTONDBLCLK,
        WM_RBUTTONDOWN,
        WM_RBUTTONUP,
        WM_RBUTTONDBLCLK,
        WM_MBUTTONDOWN,
        WM_MBUTTONUP,
        WM_MBUTTONDBLCLK,
    };

    flush_events();

    hr = IVideoWindow_get_MessageDrain(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!oahwnd, "Got window %#Ix.\n", oahwnd);

    hr = IVideoWindow_put_MessageDrain(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_MessageDrain(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(oahwnd == (OAHWND)our_hwnd, "Got window %#Ix.\n", oahwnd);

    for (i = 0; i < ARRAY_SIZE(drain_tests); ++i)
    {
        SendMessageA(hwnd, drain_tests[i], 0xdeadbeef, 0);
        ret = PeekMessageA(&msg, 0, drain_tests[i], drain_tests[i], PM_REMOVE);
        ok(ret, "Expected a message.\n");
        ok(msg.hwnd == our_hwnd, "Got hwnd %p.\n", msg.hwnd);
        ok(msg.message == drain_tests[i], "Got message %#x.\n", msg.message);
        ok(msg.wParam == 0xdeadbeef, "Got wparam %#Ix.\n", msg.wParam);
        ok(!msg.lParam, "Got lparam %#Ix.\n", msg.lParam);
        DispatchMessageA(&msg);

        ret = PeekMessageA(&msg, 0, drain_tests[i], drain_tests[i], PM_REMOVE);
        ok(!ret, "Got unexpected message %#x.\n", msg.message);
    }

    hr = IVideoWindow_put_MessageDrain(window, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_Owner(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    flush_events();

    /* Demonstrate that messages should be sent, not posted, and that only some
     * messages should be forwarded. A previous implementation unconditionally
     * posted all messages. */

    hr = IVideoWindow_NotifyOwnerMessage(window, (OAHWND)our_hwnd, WM_SYSCOLORCHANGE, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        ok(msg.message != WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE should not be posted.\n");
        DispatchMessageA(&msg);
    }

    hr = IVideoWindow_NotifyOwnerMessage(window, (OAHWND)our_hwnd, WM_FONTCHANGE, 0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        ok(msg.message != WM_FONTCHANGE, "WM_FONTCHANGE should not be posted.\n");
        DispatchMessageA(&msg);
    }

    params.window = window;
    params.hwnd = our_hwnd;
    params.message = WM_SYSCOLORCHANGE;
    thread = CreateThread(NULL, 0, notify_message_proc, &params, 0, NULL);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block.\n");

    while ((ret = MsgWaitForMultipleObjects(1, &thread, FALSE, 1000, QS_ALLINPUT)) == 1)
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            ok(msg.message != WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE should not be posted.\n");
            DispatchMessageA(&msg);
        }
    }
    ok(!ret, "Wait timed out.\n");
    CloseHandle(thread);

    params.message = WM_FONTCHANGE;
    thread = CreateThread(NULL, 0, notify_message_proc, &params, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "Thread should not block.\n");
    CloseHandle(thread);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        ok(msg.message != WM_FONTCHANGE, "WM_FONTCHANGE should not be posted.\n");
        DispatchMessageA(&msg);
    }

    hr = IVideoWindow_put_Owner(window, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_video_window_autoshow(IVideoWindow *window, IFilterGraph2 *graph, HWND hwnd)
{
    IMediaControl *control;
    HRESULT hr;
    LONG l;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    hr = IVideoWindow_get_AutoShow(window, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(l == OATRUE, "Got %ld.\n", l);

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(l == OATRUE, "Got %ld.\n", l);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(l == OATRUE, "Got %ld.\n", l);

    hr = IVideoWindow_put_AutoShow(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(l == OAFALSE, "Got %ld.\n", l);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(control);
}

static void test_video_window(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biBitCount = 24,
        .bmiHeader.biWidth = 640,
        .bmiHeader.biHeight = 480,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB24,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IFilterGraph2 *graph = create_graph();
    WNDCLASSA window_class = {0};
    struct testfilter source;
    LONG width, height, l;
    ULONG_PTR background;
    IVideoWindow *window;
    IBaseFilter *filter;
    HWND hwnd, our_hwnd;
    IOverlay *overlay;
    BSTR caption;
    HRESULT hr;
    DWORD tid;
    ULONG ref;
    IPin *pin;
    RECT rect;

    window_class.lpszClassName = "wine_test_class";
    window_class.lpfnWndProc = window_proc;
    RegisterClassA(&window_class);
    our_hwnd = CreateWindowA("wine_test_class", "test window", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            100, 200, 300, 400, NULL, NULL, NULL, NULL);
    flush_events();

    filter = create_video_renderer();
    flush_events();

    flaky_wine
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_debug > 1) trace("ours %p, theirs %p\n", our_hwnd, hwnd);
    GetWindowRect(hwnd, &rect);

    tid = GetWindowThreadProcessId(hwnd, NULL);
    ok(tid == GetCurrentThreadId(), "Expected tid %#lx, got %#lx.\n", GetCurrentThreadId(), tid);

    background = GetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND);
    ok(!background, "Expected NULL brush, got %#Ix\n", background);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVideoWindow, (void **)&window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Visible(window, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_Caption(window, &caption);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    caption = SysAllocString(L"foo");
    hr = IVideoWindow_put_Caption(window, caption);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    SysFreeString(caption);

    hr = IVideoWindow_get_WindowStyle(window, &l);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_WindowStyle(window, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_AutoShow(window, &l);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_AutoShow(window, OAFALSE);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_Owner(window, (OAHWND)our_hwnd);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_MessageDrain(window, (OAHWND)our_hwnd);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_put_Visible(window, OATRUE);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_SetWindowPosition(window, 100, 200, 300, 400);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    testfilter_init(&source);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    flaky_wine
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    test_video_window_caption(window, hwnd);
    test_video_window_style(window, hwnd, our_hwnd);
    test_video_window_state(window, hwnd, our_hwnd);
    test_video_window_position(window, hwnd, our_hwnd);
    test_video_window_autoshow(window, graph, hwnd);
    test_video_window_owner(window, hwnd, our_hwnd);
    test_video_window_messages(window, hwnd, our_hwnd);

    hr = IVideoWindow_put_FullScreenMode(window, OATRUE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_get_FullScreenMode(window, &l);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_GetMinIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == 640, "Got width %ld.\n", width);
    ok(height == 480, "Got height %ld.\n", height);

    hr = IVideoWindow_GetMaxIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == 640, "Got width %ld.\n", width);
    ok(height == 480, "Got height %ld.\n", height);

    IFilterGraph2_Release(graph);
    IVideoWindow_Release(window);
    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(our_hwnd);
}

static void check_source_position_(int line, IBasicVideo *video,
        LONG expect_left, LONG expect_top, LONG expect_width, LONG expect_height)
{
    LONG left, top, width, height, l;
    HRESULT hr;

    left = top = width = height = 0xdeadbeef;
    hr = IBasicVideo_GetSourcePosition(video, &left, &top, &width, &height);
    ok_(__FILE__,line)(hr == S_OK, "Got hr %#lx.\n", hr);
    ok_(__FILE__,line)(left == expect_left, "Got left %ld.\n", left);
    ok_(__FILE__,line)(top == expect_top, "Got top %ld.\n", top);
    ok_(__FILE__,line)(width == expect_width, "Got width %ld.\n", width);
    ok_(__FILE__,line)(height == expect_height, "Got height %ld.\n", height);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_SourceLeft(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get left, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == left, "Got left %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_SourceTop(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get top, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == top, "Got top %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_SourceWidth(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get width, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == width, "Got width %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_SourceHeight(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get height, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == height, "Got height %ld.\n", l);
}
#define check_source_position(a,b,c,d,e) check_source_position_(__LINE__,a,b,c,d,e)

static void test_basic_video_source(IBasicVideo *video)
{
    HRESULT hr;

    check_source_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceLeft(video, -10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceLeft(video, 10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceTop(video, -10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceTop(video, 10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceWidth(video, -500);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceWidth(video, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceWidth(video, 700);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceWidth(video, 500);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 0, 0, 500, 400);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceHeight(video, -300);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceHeight(video, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceHeight(video, 600);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceHeight(video, 300);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 0, 0, 500, 300);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceLeft(video, -10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceLeft(video, 10);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 10, 0, 500, 300);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_SourceTop(video, -10);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceTop(video, 20);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 10, 20, 500, 300);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(video, 4, 5, 60, 40);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 4, 5, 60, 40);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(video, 0, 0, 600, 400);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(video, 4, 5, 60, 40);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDefaultSourcePosition(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_source_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultSource(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void check_destination_position_(int line, IBasicVideo *video,
        LONG expect_left, LONG expect_top, LONG expect_width, LONG expect_height)
{
    LONG left, top, width, height, l;
    HRESULT hr;

    left = top = width = height = 0xdeadbeef;
    hr = IBasicVideo_GetDestinationPosition(video, &left, &top, &width, &height);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get position, hr %#lx.\n", hr);
    ok_(__FILE__,line)(left == expect_left, "Got left %ld.\n", left);
    ok_(__FILE__,line)(top == expect_top, "Got top %ld.\n", top);
    ok_(__FILE__,line)(width == expect_width, "Got width %ld.\n", width);
    ok_(__FILE__,line)(height == expect_height, "Got height %ld.\n", height);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_DestinationLeft(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get left, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == left, "Got left %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_DestinationTop(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get top, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == top, "Got top %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_DestinationWidth(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get width, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == width, "Got width %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_DestinationHeight(video, &l);
    ok_(__FILE__,line)(hr == S_OK, "Failed to get height, hr %#lx.\n", hr);
    ok_(__FILE__,line)(l == height, "Got height %ld.\n", l);
}
#define check_destination_position(a,b,c,d,e) check_destination_position_(__LINE__,a,b,c,d,e)

static void test_basic_video_destination(IBasicVideo *video)
{
    IVideoWindow *window;
    HRESULT hr;
    RECT rect;

    IBasicVideo_QueryInterface(video, &IID_IVideoWindow, (void **)&window);

    check_destination_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationLeft(video, -10);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, -10, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationLeft(video, 10);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationTop(video, -20);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, -20, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationTop(video, 20);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 20, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationWidth(video, -700);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationWidth(video, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationWidth(video, 700);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 20, 700, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationWidth(video, 500);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 20, 500, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationHeight(video, -500);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationHeight(video, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationHeight(video, 500);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 20, 500, 500);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationHeight(video, 300);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 10, 20, 500, 300);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(video, 4, 5, 60, 40);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 4, 5, 60, 40);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(video, 0, 0, 600, 400);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(video, 4, 5, 60, 40);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDefaultDestinationPosition(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 0, 0, 600, 400);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    SetRect(&rect, 100, 200, 500, 500);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    hr = IVideoWindow_SetWindowPosition(window, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 0, 0, 400, 300);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(video, 0, 0, 400, 300);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 0, 0, 400, 300);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    SetRect(&rect, 100, 200, 600, 600);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    hr = IVideoWindow_SetWindowPosition(window, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_destination_position(video, 0, 0, 400, 300);
    hr = IBasicVideo_IsUsingDefaultDestination(video);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IVideoWindow_Release(window);
}

static void test_basic_video(void)
{
    VIDEOINFOHEADER vih =
    {
        .AvgTimePerFrame = 200000,
        .rcSource = {4, 6, 16, 12},
        .rcTarget = {40, 60, 120, 160},
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biBitCount = 32,
        .bmiHeader.biWidth = 600,
        .bmiHeader.biHeight = 400,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB32,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IBaseFilter *filter = create_video_renderer();
    IFilterGraph2 *graph = create_graph();
    LONG left, top, width, height, l;
    struct testfilter source;
    ITypeInfo *typeinfo;
    IBasicVideo *video;
    TYPEATTR *typeattr;
    REFTIME reftime;
    HRESULT hr;
    UINT count;
    ULONG ref;
    IPin *pin;
    RECT rect;

    IBaseFilter_QueryInterface(filter, &IID_IBasicVideo, (void **)&video);
    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IBasicVideo_GetTypeInfoCount(video, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IBasicVideo_GetTypeInfo(video, 0, 0, &typeinfo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(typeattr->typekind == TKIND_DISPATCH, "Got kind %u.\n", typeattr->typekind);
    ok(IsEqualGUID(&typeattr->guid, &IID_IBasicVideo), "Got IID %s.\n", wine_dbgstr_guid(&typeattr->guid));
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);

    hr = IBasicVideo_get_AvgTimePerFrame(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_AvgTimePerFrame(video, &reftime);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_BitRate(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_BitRate(video, &l);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_BitErrorRate(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_BitErrorRate(video, &l);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_VideoWidth(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_VideoHeight(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_SourceLeft(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_SourceWidth(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_SourceTop(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_SourceHeight(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_DestinationLeft(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_DestinationWidth(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_DestinationTop(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_get_DestinationHeight(video, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetSourcePosition(video, NULL, &top, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(video, &left, NULL, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(video, &left, &top, NULL, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(video, &left, &top, &width, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetDestinationPosition(video, NULL, &top, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(video, &left, NULL, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(video, &left, &top, NULL, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(video, &left, &top, &width, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetVideoSize(video, &width, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoSize(video, NULL, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoSize(video, &width, &height);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetVideoPaletteEntries(video, 0, 1, NULL, &l);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoPaletteEntries(video, 0, 1, &l, NULL);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    testfilter_init(&source);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, L"vmr9");
    IFilterGraph2_AddFilter(graph, filter, L"source");
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    reftime = 0.0;
    hr = IBasicVideo_get_AvgTimePerFrame(video, &reftime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(reftime == 0.02, "Got frame rate %.16e.\n", reftime);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_BitRate(video, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!l, "Got bit rate %ld.\n", l);

    l = 0xdeadbeef;
    hr = IBasicVideo_get_BitErrorRate(video, &l);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!l, "Got bit rate %ld.\n", l);

    hr = IBasicVideo_GetVideoPaletteEntries(video, 0, 1, &l, NULL);
    todo_wine ok(hr == VFW_E_NO_PALETTE_AVAILABLE, "Got hr %#lx.\n", hr);

    width = height = 0xdeadbeef;
    hr = IBasicVideo_GetVideoSize(video, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == 600, "Got width %ld.\n", width);
    ok(height == 400, "Got height %ld.\n", height);

    test_basic_video_source(video);
    test_basic_video_destination(video);

    hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    vih.bmiHeader.biWidth = 16;
    vih.bmiHeader.biHeight = 16;
    vih.bmiHeader.biSizeImage = 0;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_source_position(video, 0, 0, 16, 16);

    SetRect(&rect, 0, 0, 0, 0);
    AdjustWindowRectEx(&rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, FALSE, 0);
    check_destination_position(video, 0, 0, max(16, GetSystemMetrics(SM_CXMIN) - (rect.right - rect.left)),
            max(16, GetSystemMetrics(SM_CYMIN) - (rect.bottom - rect.top)));

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBasicVideo_Release(video);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_unconnected_eos(void)
{
    IBaseFilter *filter = create_video_renderer();
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

START_TEST(videorenderer)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_unconnected_filter_state();
    test_connect_pin();
    test_overlay();
    test_video_window();
    test_basic_video();
    test_unconnected_eos();

    CoUninitialize();
}
