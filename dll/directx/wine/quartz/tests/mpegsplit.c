/*
 * MPEG-1 splitter filter unit tests
 *
 * Copyright 2015 Anton Baskanov
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
#include "mmreg.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static const GUID testguid = {0xfacade};
static const GUID MEDIASUBTYPE_mp3 = {0x00000055,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};

static IBaseFilter *create_mpeg_splitter(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_MPEG1Splitter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource, error %lu.\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource.\n");
    CloseHandle(file);

    return pathW;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static IFilterGraph2 *connect_input(IBaseFilter *splitter, const WCHAR *filename)
{
    IFileSourceFilter *filesource;
    IFilterGraph2 *graph;
    IBaseFilter *reader;
    IPin *source, *sink;
    HRESULT hr;

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&reader);
    IBaseFilter_QueryInterface(reader, &IID_IFileSourceFilter, (void **)&filesource);
    IFileSourceFilter_Load(filesource, filename, NULL);

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    IFilterGraph2_AddFilter(graph, reader, NULL);
    IFilterGraph2_AddFilter(graph, splitter, NULL);

    IBaseFilter_FindPin(splitter, L"Input", &sink);
    IBaseFilter_FindPin(reader, L"Output", &source);

    hr = IFilterGraph2_ConnectDirect(graph, source, sink, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_Release(source);
    IPin_Release(sink);
    IBaseFilter_Release(reader);
    IFileSourceFilter_Release(filesource);
    return graph;
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
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IPin *pin;

    check_interface(filter, &IID_IAMStreamSelect, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, L"Input", &pin);

    todo_wine check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Audio", &pin);

    check_interface(pin, &IID_IMediaSeeking, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);

    IPin_Release(pin);

    IBaseFilter_Release(filter);
    IFilterGraph2_Release(graph);
    DeleteFileW(filename);
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
    hr = CoCreateInstance(&CLSID_MPEG1Splitter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_MPEG1Splitter, &test_outer, CLSCTX_INPROC_SERVER,
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
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IEnumPins *enum1, *enum2;
    IFilterGraph2 *graph;
    ULONG count, ref;
    IPin *pins[3];
    HRESULT hr;
    BOOL ret;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    IPin_Release(pins[0]);

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

    graph = connect_input(filter, filename);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 3, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    IEnumPins_Release(enum1);
    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_find_pin(void)
{
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph;
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"Input", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Audio", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    graph = connect_input(filter, filename);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"Input", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"Audio", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    IEnumPins_Release(enum_pins);
    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_pin_info(void)
{
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    ULONG ref, expect_ref;
    IFilterGraph2 *graph;
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;
    BOOL ret;

    graph = connect_input(filter, filename);

    hr = IBaseFilter_FindPin(filter, L"Input", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_ref = get_refcount(filter);
    ref = get_refcount(pin);
    ok(ref == expect_ref, "Got unexpected refcount %ld.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"Input"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == expect_ref + 1, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == expect_ref + 1, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"Input"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Audio", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"Audio"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"Audio"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_media_types(void)
{
    MPEG1WAVEFORMAT expect_wfx =
    {
        {WAVE_FORMAT_MPEG, 1, 48000, 4000, 96, 0, sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX)},
        ACM_MPEG_LAYER3, 32000, ACM_MPEG_SINGLECHANNEL, 4096, 1, ACM_MPEG_ORIGINALHOME | ACM_MPEG_PROTECTIONBIT | ACM_MPEG_ID_MPEG1, 0, 0
    };
    static const MPEGLAYER3WAVEFORMAT expect_mp3_wfx =
    {
        {WAVE_FORMAT_MPEGLAYER3, 1, 48000, 4000, 1, 0, sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX)},
        MPEGLAYER3_ID_MPEG, 0, 96, 1, 0
    };

    const WCHAR *filename = load_resource(L"test.mp3");
    AM_MEDIA_TYPE mt = {{0}}, *pmt, expect_mt = {{0}};
    IBaseFilter *filter = create_mpeg_splitter();
    MPEGLAYER3WAVEFORMAT *mp3wfx;
    IEnumMediaTypes *enummt;
    MPEG1WAVEFORMAT *wfx;
    IFilterGraph2 *graph;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, L"Input", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    expect_mt.majortype = MEDIATYPE_Stream;
    expect_mt.bFixedSizeSamples = TRUE;
    expect_mt.bTemporalCompression = TRUE;
    expect_mt.lSampleSize = 1;

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1System;
    ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1VideoCD;
    ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1Video;
    ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1Audio;
    ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    mt.majortype = MEDIATYPE_Stream;
    mt.subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1VideoCD;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1System;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1Payload;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_MPEG1Audio;

    mt.majortype = MEDIATYPE_Audio;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = MEDIATYPE_Stream;

    mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.formattype = FORMAT_VideoInfo;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.formattype = FORMAT_WaveFormatEx;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    graph = connect_input(filter, filename);

    /* Connecting input doesn't change the reported media types. */
    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1System;
    ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
    CoTaskMemFree(pmt);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Audio", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    todo_wine ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    todo_wine ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    todo_wine ok(pmt->lSampleSize == 1, "Got sample size %lu.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    todo_wine ok(pmt->cbFormat == sizeof(MPEG1WAVEFORMAT), "Got format size %lu.\n", pmt->cbFormat);
    if (pmt->cbFormat == sizeof(MPEG1WAVEFORMAT))
    {
        /* Native will sometimes leave junk in the joint stereo flags. */
        expect_wfx.fwHeadModeExt = ((MPEG1WAVEFORMAT *)pmt->pbFormat)->fwHeadModeExt;
        ok(!memcmp(pmt->pbFormat, &expect_wfx, sizeof(MPEG1WAVEFORMAT)), "Format blocks didn't match.\n");
    }

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;

    pmt->subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pmt->formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->formattype = FORMAT_WaveFormatEx;

    wfx = (MPEG1WAVEFORMAT *)pmt->pbFormat;

    wfx->fwHeadLayer = ACM_MPEG_LAYER2;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    wfx->fwHeadLayer = ACM_MPEG_LAYER3;

    wfx->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    wfx->wfx.wFormatTag = WAVE_FORMAT_MPEG;

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto done;
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Payload), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %lu.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(MPEG1WAVEFORMAT), "Got format size %lu.\n", pmt->cbFormat);
    /* Native will sometimes leave junk in the joint stereo flags. */
    expect_wfx.fwHeadModeExt = ((MPEG1WAVEFORMAT *)pmt->pbFormat)->fwHeadModeExt;
    ok(!memcmp(pmt->pbFormat, &expect_wfx, sizeof(MPEG1WAVEFORMAT)), "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_mp3), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %lu.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(MPEGLAYER3WAVEFORMAT), "Got format size %lu.\n", pmt->cbFormat);
    ok(!memcmp(pmt->pbFormat, &expect_mp3_wfx, sizeof(MPEGLAYER3WAVEFORMAT)),
            "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;

    pmt->subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pmt->formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->formattype = FORMAT_WaveFormatEx;

    mp3wfx = (MPEGLAYER3WAVEFORMAT *)pmt->pbFormat;

    mp3wfx->fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mp3wfx->fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;

    mp3wfx->wfx.wFormatTag = WAVE_FORMAT_MPEG;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mp3wfx->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

done:
    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_enum_media_types(void)
{
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[5];
    ULONG ref, count;
    unsigned int i;
    HRESULT hr;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, L"Input", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(count == 1, "Got count %lu.\n", count);
        CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]);
    CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 3, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]);
    CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 5);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 4);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Audio", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 3; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
        todo_wine_if(i) ok(hr == S_OK, "Got hr %#lx.\n", hr);
        if (hr == S_OK) CoTaskMemFree(mts[0]->pbFormat);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 3; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
        todo_wine_if(i) ok(hr == S_OK, "Got hr %#lx.\n", hr);
        todo_wine_if(i) ok(count == 1, "Got count %lu.\n", count);
        if (hr == S_OK) CoTaskMemFree(mts[0]->pbFormat);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 2, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);
    if (count > 1) CoTaskMemFree(mts[1]->pbFormat);
    if (count > 1) CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 1, "Got count %lu.\n", count);
    if (count) CoTaskMemFree(mts[0]->pbFormat);
    if (count) CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 4);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 3);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_mpeg_splitter();
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

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    struct strmbase_sink sink;
    IAsyncReader IAsyncReader_iface, *reader;
    const AM_MEDIA_TYPE *mt;
    HANDLE eos_event;
    unsigned int sample_count, eos_count, new_segment_count, byte_count;
    REFERENCE_TIME segment_start, segment_end_min, segment_end_max, seek_start, seek_end;
    LONGLONG read_position;
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
    else if (index == 1)
        return &filter->sink.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
    CloseHandle(filter->eos_event);
}

static HRESULT testfilter_init_stream(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    filter->new_segment_count = 0;
    filter->eos_count = 0;
    filter->sample_count = 0;
    return S_OK;
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
    .filter_init_stream = testfilter_init_stream,
};

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IAsyncReader))
        *out = &filter->IAsyncReader_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI testsource_AttemptConnection(struct strmbase_source *iface,
        IPin *peer, const AM_MEDIA_TYPE *mt)
{
    HRESULT hr;

    iface->pin.peer = peer;
    IPin_AddRef(peer);
    CopyMediaType(&iface->pin.mt, mt);

    if (FAILED(hr = IPin_ReceiveConnection(peer, &iface->pin.IPin_iface, mt)))
    {
        ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
        IPin_Release(peer);
        iface->pin.peer = NULL;
        FreeMediaType(&iface->pin.mt);
    }

    return hr;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_interface = testsource_query_interface,
    .pfnAttemptConnection = testsource_AttemptConnection,
};

static HRESULT testsink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsink_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);
    if (!index && filter->mt)
    {
        CopyMediaType(mt, filter->mt);
        return S_OK;
    }
    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT testsink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    if (filter->mt && !IsEqualGUID(&mt->majortype, &filter->mt->majortype))
        return VFW_E_TYPE_NOT_ACCEPTED;
    return S_OK;
}

static HRESULT WINAPI testsink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    REFERENCE_TIME start, end;
    IMediaSeeking *seeking;
    HRESULT hr;

    hr = IMediaSample_GetTime(sample, &start, &end);
    ok(hr == S_OK || (filter->sample_count > 0 && hr == VFW_E_SAMPLE_TIME_NOT_SET), "Got hr %#lx.\n", hr);

    if (winetest_debug > 1)
        trace("%04lx: Got sample with timestamps %I64d-%I64d.\n", GetCurrentThreadId(), start, end);

    ok(filter->new_segment_count, "Expected NewSegment() before Receive().\n");

    IPin_QueryInterface(iface->pin.peer, &IID_IMediaSeeking, (void **)&seeking);
    hr = IMediaSeeking_GetPositions(seeking, &start, &end);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(start == filter->seek_start, "Expected start position %I64u, got %I64u.\n", filter->seek_start, start);
    ok(end == filter->seek_end, "Expected end position %I64u, got %I64u.\n", filter->seek_end, end);
    IMediaSeeking_Release(seeking);

    ok(!filter->eos_count, "Got a sample after EOS.\n");
    ++filter->sample_count;
    filter->byte_count += IMediaSample_GetActualDataLength(sample);
    return S_OK;
}

static HRESULT testsink_eos(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);

    if (winetest_debug > 1)
        trace("%04lx: Got EOS.\n", GetCurrentThreadId());

    ok(!filter->eos_count, "Got %u EOS events.\n", filter->eos_count + 1);
    ++filter->eos_count;
    SetEvent(filter->eos_event);
    return S_OK;
}

static HRESULT testsink_new_segment(struct strmbase_sink *iface,
        REFERENCE_TIME start, REFERENCE_TIME end, double rate)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    IMediaSeeking *seeking;
    HRESULT hr;

    if (winetest_debug > 1)
        trace("%04lx: Got segment with timestamps %I64d-%I64d.\n", GetCurrentThreadId(), start, end);

    ++filter->new_segment_count;

    IPin_QueryInterface(iface->pin.peer, &IID_IMediaSeeking, (void **)&seeking);
    hr = IMediaSeeking_GetPositions(seeking, &filter->seek_start, &filter->seek_end);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSeeking_Release(seeking);

    ok(start == filter->segment_start, "Expected start %I64d, got %I64d.\n", filter->segment_start, start);
    ok(end >= filter->segment_end_min && end <= filter->segment_end_max,
        "Expected end %I64d to %I64d, got %I64d.\n", filter->segment_end_min, filter->segment_end_max, end);
    ok(rate == 1.0, "Got rate %.16e.\n", rate);

    return S_OK;
}

static const struct strmbase_sink_ops testsink_ops =
{
    .base.pin_query_interface = testsink_query_interface,
    .base.pin_get_media_type = testsink_get_media_type,
    .sink_connect = testsink_connect,
    .pfnReceive = testsink_Receive,
    .sink_eos = testsink_eos,
    .sink_new_segment = testsink_new_segment,
};

static struct testfilter *impl_from_IAsyncReader(IAsyncReader *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IAsyncReader_iface);
}

static HRESULT WINAPI async_reader_QueryInterface(IAsyncReader *iface, REFIID iid, void **out)
{
    return IPin_QueryInterface(&impl_from_IAsyncReader(iface)->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI async_reader_AddRef(IAsyncReader *iface)
{
    return IPin_AddRef(&impl_from_IAsyncReader(iface)->source.pin.IPin_iface);
}

static ULONG WINAPI async_reader_Release(IAsyncReader *iface)
{
    return IPin_Release(&impl_from_IAsyncReader(iface)->source.pin.IPin_iface);
}

static HRESULT WINAPI async_reader_RequestAllocator(IAsyncReader *iface,
        IMemAllocator *preferred, ALLOCATOR_PROPERTIES *props, IMemAllocator **allocator)
{
    return IAsyncReader_RequestAllocator(impl_from_IAsyncReader(iface)->reader, preferred, props, allocator);
}

static HRESULT WINAPI async_reader_Request(IAsyncReader *iface, IMediaSample *sample, DWORD_PTR cookie)
{
    return IAsyncReader_Request(impl_from_IAsyncReader(iface)->reader, sample, cookie);
}

static HRESULT WINAPI async_reader_WaitForNext(IAsyncReader *iface,
        DWORD timeout, IMediaSample **sample, DWORD_PTR *cookie)
{
    return IAsyncReader_WaitForNext(impl_from_IAsyncReader(iface)->reader, timeout, sample, cookie);
}

static HRESULT WINAPI async_reader_SyncReadAligned(IAsyncReader *iface, IMediaSample *sample)
{
    return IAsyncReader_SyncReadAligned(impl_from_IAsyncReader(iface)->reader, sample);
}

static HRESULT WINAPI async_reader_SyncRead(IAsyncReader *iface, LONGLONG position, LONG length, BYTE *buffer)
{
    struct testfilter *filter = impl_from_IAsyncReader(iface);
    filter->read_position = position + length;
    return IAsyncReader_SyncRead(filter->reader, position, length, buffer);
}

static HRESULT WINAPI async_reader_Length(IAsyncReader *iface, LONGLONG *total, LONGLONG *available)
{
    return IAsyncReader_Length(impl_from_IAsyncReader(iface)->reader, total, available);
}

static HRESULT WINAPI async_reader_BeginFlush(IAsyncReader *iface)
{
    return IAsyncReader_BeginFlush(impl_from_IAsyncReader(iface)->reader);
}

static HRESULT WINAPI async_reader_EndFlush(IAsyncReader *iface)
{
    return IAsyncReader_EndFlush(impl_from_IAsyncReader(iface)->reader);
}

static const struct IAsyncReaderVtbl async_reader_vtbl =
{
    async_reader_QueryInterface,
    async_reader_AddRef,
    async_reader_Release,
    async_reader_RequestAllocator,
    async_reader_Request,
    async_reader_WaitForNext,
    async_reader_SyncReadAligned,
    async_reader_SyncRead,
    async_reader_Length,
    async_reader_BeginFlush,
    async_reader_EndFlush,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"source", &testsource_ops);
    strmbase_sink_init(&filter->sink, &filter->filter, L"sink", &testsink_ops, NULL);
    filter->IAsyncReader_iface.lpVtbl = &async_reader_vtbl;
    filter->eos_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    filter->segment_end_min = 5000000; /* 5392500 on native */
    filter->segment_end_max = 5500000;
}

static void test_connect_pin(void)
{
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Stream,
        .subtype = MEDIASUBTYPE_MPEG1Audio,
        .formattype = FORMAT_WaveFormatEx,
        .lSampleSize = 888,
    };
    IBaseFilter *filter = create_mpeg_splitter(), *reader;
    const WCHAR *filename = load_resource(L"test.mp3");
    struct testfilter testsource, testsink;
    IFileSourceFilter *filesource;
    AM_MEDIA_TYPE mt, *source_mt;
    IPin *sink, *source, *peer;
    IEnumMediaTypes *enummt;
    IMediaControl *control;
    IFilterGraph2 *graph;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    testfilter_init(&testsource);
    testfilter_init(&testsink);
    IFilterGraph2_AddFilter(graph, &testsink.filter.IBaseFilter_iface, L"sink");
    IFilterGraph2_AddFilter(graph, &testsource.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"splitter");
    IBaseFilter_FindPin(filter, L"Input", &sink);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&reader);
    IBaseFilter_QueryInterface(reader, &IID_IFileSourceFilter, (void **)&filesource);
    IFileSourceFilter_Load(filesource, filename, NULL);
    IFileSourceFilter_Release(filesource);
    IBaseFilter_FindPin(reader, L"Output", &source);
    IPin_QueryInterface(source, &IID_IAsyncReader, (void **)&testsource.reader);
    IPin_Release(source);

    /* Test sink connection. */

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    req_mt.majortype = MEDIATYPE_Video;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.majortype = MEDIATYPE_Stream;

    req_mt.subtype = MEDIASUBTYPE_RGB8;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_MPEG1Audio;

    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsource.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");
    ok(compare_media_types(&testsource.source.pin.mt, &req_mt), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test source connection. */

    IBaseFilter_FindPin(filter, L"Audio", &source);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    /* Exact connection. */

    IPin_EnumMediaTypes(source, &enummt);
    IEnumMediaTypes_Next(enummt, 1, &source_mt, NULL);
    IEnumMediaTypes_Release(enummt);
    CopyMediaType(&req_mt, source_mt);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsink.sink.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");
    ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsink.sink.pin.peer == source, "Got peer %p.\n", testsink.sink.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.lSampleSize = 999;
    req_mt.bTemporalCompression = req_mt.bFixedSizeSamples = TRUE;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = MEDIATYPE_Stream;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.majortype = MEDIATYPE_Audio;

    req_mt.subtype = MEDIASUBTYPE_PCM;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    /* Connection with wildcards. */

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_media_types(&testsink.sink.pin.mt, source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_PCM;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.majortype = MEDIATYPE_Audio;
    req_mt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(compare_media_types(&testsink.sink.pin.mt, source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_PCM;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = MEDIATYPE_Stream;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    /* Test enumeration of sink media types. */

    testsink.mt = &req_mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.majortype = MEDIATYPE_Audio;
    req_mt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    req_mt.formattype = FORMAT_WaveFormatEx;
    req_mt.lSampleSize = 444;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");

    IPin_Release(source);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsource.source.pin.peer == sink, "Got peer %p.\n", testsource.source.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsource.source.pin.IPin_iface);

    CoTaskMemFree(req_mt.pbFormat);
    CoTaskMemFree(source_mt->pbFormat);
    CoTaskMemFree(source_mt);

    IAsyncReader_Release(testsource.reader);
    IPin_Release(sink);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(reader);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsource.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_seeking(void)
{
    LONGLONG time, current, stop, earliest, latest, duration;
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IMediaSeeking *seeking;
    unsigned int i;
    double rate;
    GUID format;
    HRESULT hr;
    DWORD caps;
    ULONG ref;
    IPin *pin;
    BOOL ret;

    static const struct
    {
        const GUID *guid;
        HRESULT hr;
    }
    format_tests[] =
    {
        {&TIME_FORMAT_MEDIA_TIME, S_OK},

        {&TIME_FORMAT_SAMPLE, S_FALSE},
        {&TIME_FORMAT_BYTE, S_FALSE},
        {&TIME_FORMAT_NONE, S_FALSE},
        {&TIME_FORMAT_FRAME, S_FALSE},
        {&TIME_FORMAT_FIELD, S_FALSE},
        {&testguid, S_FALSE},
    };

    IBaseFilter_FindPin(filter, L"Audio", &pin);
    IPin_QueryInterface(pin, &IID_IMediaSeeking, (void **)&seeking);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == (AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards
            | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetStopPos
            | AM_SEEKING_CanGetDuration), "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == (AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards), "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanSeekAbsolute, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(!caps, "Got caps %#lx.\n", caps);

    caps = 0;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(!caps, "Got caps %#lx.\n", caps);

    for (i = 0; i < ARRAY_SIZE(format_tests); ++i)
    {
        hr = IMediaSeeking_IsFormatSupported(seeking, format_tests[i].guid);
        ok(hr == format_tests[i].hr, "Got hr %#lx for format %s.\n",
                hr, wine_dbgstr_guid(format_tests[i].guid));
    }

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_SAMPLE);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_FRAME);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    duration = 0;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(duration >= 5000000 && duration <= 5500000, "Got duration %I64d.\n", duration);

    stop = current = 0xdeadbeef;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stop == duration, "Expected time %s, got %s.\n",
            wine_dbgstr_longlong(duration), wine_dbgstr_longlong(stop));
    hr = IMediaSeeking_GetCurrentPosition(seeking, &current);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    stop = current = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == duration, "Expected time %s, got %s.\n",
            wine_dbgstr_longlong(duration), wine_dbgstr_longlong(stop));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));
    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    earliest = latest = 0xdeadbeef;
    hr = IMediaSeeking_GetAvailable(seeking, &earliest, &latest);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!earliest, "Got time %s.\n", wine_dbgstr_longlong(earliest));
    ok(latest == duration, "Expected time %s, got %s.\n",
            wine_dbgstr_longlong(duration), wine_dbgstr_longlong(latest));

    rate = 0;
    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(rate == 1.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_SetRate(seeking, 200.0);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    rate = 0;
    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(rate == 200.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_SetRate(seeking, -1.0);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetPreroll(seeking, &time);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    current = 200 * 10000;
    stop = 400 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 200 * 10000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 400 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    stop = current = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 200 * 10000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 400 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    current = 200 * 10000;
    stop = 400 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 200 * 10000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 400 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    current = 100 * 10000;
    stop = 200 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 100 * 10000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 200 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    current = 50 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    stop = current = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 50 * 10000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 200 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    IMediaSeeking_Release(seeking);
    IPin_Release(pin);
    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_streaming(void)
{
    const WCHAR *filename = load_resource(L"test.mp3");
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    struct testfilter testsink;
    REFERENCE_TIME start, end;
    IMediaSeeking *seeking;
    IMediaControl *control;
    IPin *source;
    HRESULT hr;
    ULONG ref;
    DWORD ret;

    testfilter_init(&testsink);
    IFilterGraph2_AddFilter(graph, &testsink.filter.IBaseFilter_iface, L"sink");
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IBaseFilter_FindPin(filter, L"Audio", &source);
    IPin_QueryInterface(source, &IID_IMediaSeeking, (void **)&seeking);

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(testsink.eos_event, 100) == WAIT_TIMEOUT, "Expected timeout.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!WaitForSingleObject(testsink.eos_event, 1000), "Did not receive EOS.\n");
    ok(WaitForSingleObject(testsink.eos_event, 100) == WAIT_TIMEOUT, "Got more than one EOS.\n");
    ok(testsink.sample_count, "Expected at least one sample.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!WaitForSingleObject(testsink.eos_event, 1000), "Did not receive EOS.\n");
    ok(WaitForSingleObject(testsink.eos_event, 100) == WAIT_TIMEOUT, "Got more than one EOS.\n");
    ok(testsink.sample_count, "Expected at least one sample.\n");

    testsink.new_segment_count = testsink.sample_count = testsink.eos_count = 0;
    testsink.segment_start = 100 * 10000;
    testsink.segment_end_min = 300 * 10000;
    testsink.segment_end_max = 300 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &testsink.segment_start, AM_SEEKING_AbsolutePositioning,
            &testsink.segment_end_min, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!WaitForSingleObject(testsink.eos_event, 1000), "Did not receive EOS.\n");
    ok(WaitForSingleObject(testsink.eos_event, 100) == WAIT_TIMEOUT, "Got more than one EOS.\n");
    ok(testsink.sample_count, "Expected at least one sample.\n");

    start = end = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &start, &end);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(start == testsink.seek_start, "Expected start position %I64u, got %I64u.\n", testsink.seek_start, start);
    ok(end == testsink.seek_end, "Expected end position %I64u, got %I64u.\n", testsink.seek_end, end);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!WaitForSingleObject(testsink.eos_event, 1000), "Did not receive EOS.\n");
    ok(WaitForSingleObject(testsink.eos_event, 100) == WAIT_TIMEOUT, "Got more than one EOS.\n");
    ok(testsink.sample_count, "Expected at least one sample.\n");

    start = end = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &start, &end);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(start == testsink.seek_start, "Expected start position %I64u, got %I64u.\n", testsink.seek_start, start);
    ok(end == testsink.seek_end, "Expected end position %I64u, got %I64u.\n", testsink.seek_end, end);

    IMediaSeeking_Release(seeking);
    IPin_Release(source);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_large_file(void)
{
    static const BYTE frame[96] = {0xff, 0xfb, 0x14, 0xc4};
    IBaseFilter *filter = create_mpeg_splitter();
    static WCHAR path[MAX_PATH];
    REFERENCE_TIME duration;
    IMediaSeeking *seeking;
    IFilterGraph2 *graph;
    unsigned int i;
    IPin *source;
    BYTE *buffer;
    HRESULT hr;
    ULONG ref;
    DWORD ret;
    FILE *f;

    GetTempPathW(ARRAY_SIZE(path), path);
    wcscat(path, L"big_test.mp3");

    /* allocate a larger buffer so I/O is faster on the testbot */
    buffer = malloc(1000 * sizeof(frame));
    for (i = 0; i < 1000; ++i)
        memcpy(buffer + i * 96, frame, sizeof(frame));
    f = _wfopen(path, L"w");
    for (i = 0; i < 100; ++i)
        fwrite(buffer, 1000 * sizeof(frame), 1, f);
    fclose(f);
    free(buffer);

    graph = connect_input(filter, path);
    IBaseFilter_FindPin(filter, L"Audio", &source);
    IPin_QueryInterface(source, &IID_IMediaSeeking, (void **)&seeking);

    duration = 0xdeadbeef;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(duration == 2400 * 10000000ull, "Got duration %I64d.\n", duration);

    IMediaSeeking_Release(seeking);
    IPin_Release(source);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(path);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_streamselect(IAMStreamSelect *sel)
{
    AM_MEDIA_TYPE mt_pin, *mt;
    IBaseFilter *filter;
    IUnknown *unk_video;
    IUnknown *unk_audio;
    DWORD stream_count;
    IUnknown *object;
    IPin *pin_video;
    IPin *pin_audio;
    IUnknown *unk;
    DWORD flags;
    DWORD group;
    LPWSTR name;
    HRESULT hr;
    LCID lcid;

    IAMStreamSelect_QueryInterface(sel, &IID_IBaseFilter, (void **)&filter);
    IBaseFilter_FindPin(filter, L"Video", &pin_video);
    IBaseFilter_FindPin(filter, L"Audio", &pin_audio);
    IPin_QueryInterface(pin_video, &IID_IUnknown, (void **)&unk_video);
    IPin_QueryInterface(pin_audio, &IID_IUnknown, (void **)&unk_audio);

    hr = IAMStreamSelect_Count(sel, &stream_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream_count == 2, "Got %lu streams.\n", stream_count);

    hr = IAMStreamSelect_Info(sel, 0, &mt, &flags, &lcid, &group, &name, &object, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_ConnectionMediaType(pin_video, &mt_pin);
    ok(compare_media_types(mt, &mt_pin), "Media types don't match\n");
    FreeMediaType(&mt_pin);
    todo_wine ok(flags == AMSTREAMSELECTINFO_ENABLED, "Got flags %lx.\n", flags);
    ok(lcid == 0, "Got LCID %lx.\n", lcid);
    ok(group == 0, "Got group %lx.\n", group);
    todo_wine ok(name && !wcscmp(name, L"Stream(E0)"), "Got name %ls.\n", name ? name : L"(null)");
    todo_wine ok(object == unk_video, "Got object %p, expected %p.\n", object, unk_video);
    ok(unk == NULL, "Got unknown %p.\n", unk);
    CoTaskMemFree(name);
    if (mt)
        DeleteMediaType(mt);
    if (object)
        IUnknown_Release(object);

    hr = IAMStreamSelect_Info(sel, 1, &mt, &flags, &lcid, &group, &name, &object, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_ConnectionMediaType(pin_audio, &mt_pin);
    ok(compare_media_types(mt, &mt_pin), "Media types don't match\n");
    FreeMediaType(&mt_pin);
    todo_wine ok(flags == AMSTREAMSELECTINFO_ENABLED, "Got flags %lx.\n", flags);
    ok(lcid == 0, "Got LCID %lx.\n", lcid);
    todo_wine ok(group == 1, "Got group %lx.\n", group);
    todo_wine ok(name && !wcscmp(name, L"Stream(C0)"), "Got name %ls.\n", name ? name : L"(null)");
    todo_wine ok(object == unk_audio, "Got object %p, expected %p.\n", object, unk_audio);
    ok(unk == NULL, "Got unknown %p.\n", unk);
    CoTaskMemFree(name);
    if (mt)
        DeleteMediaType(mt);
    if (object)
        IUnknown_Release(object);

    hr = IAMStreamSelect_Info(sel, 2, &mt, &flags, &lcid, &group, &name, &object, &unk);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IBaseFilter_Release(filter);
    IUnknown_Release(pin_video);
    IUnknown_Release(pin_audio);
    IUnknown_Release(unk_video);
    IUnknown_Release(unk_audio);
}

static void test_video_file(void)
{
    const WCHAR *filename = load_resource(L"test.mpg");
    IBaseFilter *filter = create_mpeg_splitter();
    struct testfilter testsink_video;
    struct testfilter testsink_audio;
    IPin *source_video = NULL;
    IPin *source_audio = NULL;
    IMediaControl *control;
    IFilterGraph2 *graph;
    IAMStreamSelect *sel;
    DWORD stream_count;
    HRESULT hr;
    ULONG ref;
    DWORD ret;

    IBaseFilter_QueryInterface(filter, &IID_IAMStreamSelect, (void **)&sel);
    hr = IAMStreamSelect_Count(sel, &stream_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream_count == 0, "Got %lu streams.\n", stream_count);

    graph = connect_input(filter, filename);
    hr = IBaseFilter_FindPin(filter, L"Video", &source_video);
    ok(source_video != NULL, "No video pin, hr %#lx.\n", hr);
    hr = IBaseFilter_FindPin(filter, L"Audio", &source_audio);
    ok(source_audio != NULL, "No audio pin, hr %#lx.\n", hr);

    testfilter_init(&testsink_video);
    testfilter_init(&testsink_audio);
    testsink_video.segment_end_min = 1000000; /* 11232612 on native, 1197000 in Wine */
    testsink_video.segment_end_max = 20000000;
    testsink_audio.segment_end_min = 1000000;
    testsink_audio.segment_end_max = 20000000;

    hr = IFilterGraph2_AddFilter(graph, &testsink_video.filter.IBaseFilter_iface, L"sink_video");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, &testsink_audio.filter.IBaseFilter_iface, L"sink_audio");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, source_video, &testsink_video.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, source_audio, &testsink_audio.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_streamselect(sel);

    ok(IsEqualGUID(&testsink_video.sink.pin.mt.majortype, &MEDIATYPE_Video), "Media types didn't match.\n");
    ok(IsEqualGUID(&testsink_video.sink.pin.mt.subtype, &MEDIASUBTYPE_MPEG1Payload), "Media types didn't match.\n");
    ok(IsEqualGUID(&testsink_video.sink.pin.mt.formattype, &FORMAT_MPEGVideo), "Media types didn't match.\n");

    ok(IsEqualGUID(&testsink_audio.sink.pin.mt.majortype, &MEDIATYPE_Audio), "Media types didn't match.\n");
    ok(IsEqualGUID(&testsink_audio.sink.pin.mt.subtype, &MEDIASUBTYPE_MPEG1AudioPayload), "Media types didn't match.\n");
    ok(IsEqualGUID(&testsink_audio.sink.pin.mt.formattype, &FORMAT_WaveFormatEx), "Media types didn't match.\n");

    testsink_video.new_segment_count = 0;
    testsink_audio.new_segment_count = 0;
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!WaitForSingleObject(testsink_video.eos_event, 1000), "Video sink did not receive EOS.\n");
    ok(!WaitForSingleObject(testsink_audio.eos_event, 1000), "Audio sink did not receive EOS.\n");
    ok(testsink_video.new_segment_count == 1, "Video sink got %u segments.\n", testsink_video.new_segment_count);
    ok(testsink_audio.new_segment_count == 1, "Audio sink got %u segments.\n", testsink_audio.new_segment_count);

    /* Native also supports subtype MEDIASUBTYPE_MPEG1Packet, yielding 1230 and 8828 bytes, respectively */
    ok(testsink_video.byte_count == 1214, "Video sink got %u bytes.\n", testsink_video.byte_count);
    ok(testsink_audio.byte_count == 8777, "Audio sink got %u bytes.\n", testsink_audio.byte_count);

    IAMStreamSelect_Release(sel);
    IPin_Release(source_video);
    IPin_Release(source_audio);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void test_no_acceptable_type(void)
{
    const WCHAR *filename = load_resource(L"test.wav");
    IBaseFilter *splitter = create_mpeg_splitter();
    IFileSourceFilter *filesource;
    IFilterGraph2 *graph;
    IBaseFilter *reader;
    IPin *source, *sink;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&reader);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_QueryInterface(reader, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, reader, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, splitter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(splitter, L"Input", &sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_FindPin(reader, L"Output", &source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, source, sink, NULL);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    IPin_Release(source);
    IPin_Release(sink);
    IBaseFilter_Release(reader);
    IBaseFilter_Release(splitter);
    IFileSourceFilter_Release(filesource);
    IFilterGraph2_Release(graph);
    DeleteFileW(filename);
}

static void test_video_read_position(void)
{
    IBaseFilter *filter = create_mpeg_splitter(), *reader;
    const WCHAR *filename = load_resource(L"test2.mpg");
    struct IFileSourceFilter *filesource;
    struct testfilter testsource;
    LONGLONG total, avail;
    IFilterGraph2 *graph;
    IPin *sink, *source;
    HRESULT hr;
    BOOL ret;

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    testfilter_init(&testsource);
    IFilterGraph2_AddFilter(graph, &testsource.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"splitter");
    hr = IBaseFilter_FindPin(filter, L"Input", &sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&reader);
    IBaseFilter_QueryInterface(reader, &IID_IFileSourceFilter, (void **)&filesource);
    IFileSourceFilter_Load(filesource, filename, NULL);
    IFileSourceFilter_Release(filesource);
    IBaseFilter_FindPin(reader, L"Output", &source);
    IPin_QueryInterface(source, &IID_IAsyncReader, (void **)&testsource.reader);
    IAsyncReader_Length(testsource.reader, &total, &avail);
    IPin_Release(source);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsource.read_position == total, "Got 0x%s, expected 0x%s.\n", wine_dbgstr_longlong(testsource.read_position), wine_dbgstr_longlong(total));

    IAsyncReader_Release(testsource.reader);
    IPin_Release(sink);
    IFilterGraph2_Release(graph);
    IBaseFilter_Release(reader);
    IBaseFilter_Release(filter);
    IBaseFilter_Release(&testsource.filter.IBaseFilter_iface);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

START_TEST(mpegsplit)
{
    IBaseFilter *filter;

    CoInitialize(NULL);

    if (FAILED(CoCreateInstance(&CLSID_MPEG1Splitter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        skip("Failed to create MPEG-1 splitter.\n");
        return;
    }
    IBaseFilter_Release(filter);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_unconnected_filter_state();
    test_connect_pin();
    test_seeking();
    test_streaming();
    test_large_file();
    test_video_file();
    test_no_acceptable_type();
    test_video_read_position();

    CoUninitialize();
}
