/*
 * MPEG video decoder filter unit tests
 *
 * Copyright 2022 Anton Baskanov
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
#include "mmreg.h"
#include "ks.h"
#include "ksmedia.h"
#include "wine/strmbase.h"
#include "wine/test.h"

/* same as normal MPEG1VIDEOINFO, except bSequenceHeader is 12 bytes */
typedef struct tagMPEG1VIDEOINFO_12 {
    VIDEOINFOHEADER hdr;
    DWORD dwStartTimeCode;
    DWORD cbSequenceHeader;
    BYTE bSequenceHeader[12];
} MPEG1VIDEOINFO_12;

static const MPEG1VIDEOINFO_12 mpg_format =
{
    .hdr.rcSource = { 0, 0, 32, 24 },
    .hdr.rcTarget = { 0, 0, 0, 0 },
    .hdr.dwBitRate = 0,
    .hdr.dwBitErrorRate = 0,
    .hdr.AvgTimePerFrame = 400000, /* 25fps, 40ms */
    .hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .hdr.bmiHeader.biWidth = 32,
    .hdr.bmiHeader.biHeight = 24,
    .hdr.bmiHeader.biPlanes = 0,
    .hdr.bmiHeader.biBitCount = 0,
    .hdr.bmiHeader.biCompression = 0,
    .hdr.bmiHeader.biSizeImage = 0,
    .hdr.bmiHeader.biXPelsPerMeter = 2000,
    .hdr.bmiHeader.biYPelsPerMeter = 2000,
    .hdr.bmiHeader.biClrUsed = 0,
    .hdr.bmiHeader.biClrImportant = 0,
    .dwStartTimeCode = 4096,
    .cbSequenceHeader = 12,
    .bSequenceHeader = { 0x00, 0x00, 0x01, 0xb3, 0x02, 0x00, 0x18, 0x13, 0xff, 0xff, 0xe0, 0x18 },
};

static const AM_MEDIA_TYPE mpeg_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_MPEG1Payload, FORMAT_MPEGVideo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb81, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 1,
    .formattype = {0x05589f82, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(MPEG1VIDEOINFO_12),
    .pbFormat = (BYTE *)&mpg_format,
};

static const VIDEOINFOHEADER yuy2_format =
{
    .rcSource = { 0, 0, 32, 24 },
    .rcTarget = { 0, 0, 0, 0 },
    .dwBitRate = 32 * 24 * 16 * 25,
    .dwBitErrorRate = 0,
    .AvgTimePerFrame = 400000,
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 32,
    .bmiHeader.biHeight = 24,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 16,
    .bmiHeader.biCompression = MAKEFOURCC('Y','U','Y','2'),
    .bmiHeader.biSizeImage = 32 * 24 * 16 / 8,
    .bmiHeader.biXPelsPerMeter = 2000,
    .bmiHeader.biYPelsPerMeter = 2000,
};

static const AM_MEDIA_TYPE yuy2_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_YUY2, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x32595559, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 32 * 24 * 16 / 8,
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFOHEADER),
    .pbFormat = (BYTE *)&yuy2_format,
};

static IBaseFilter *create_mpeg_video_codec(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_CMpegVideoCodec, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static const struct
{
    const GUID *subtype;
    int bits_per_pixel;
    DWORD compression;
    BOOL works_native;
    BOOL works_wine;
}
video_types[] =
{
    /* native's EnumMediaTypes offers YV12, but that media type refuses to connect */
    /* Wine doesn't enumerate unsupported ones (Y41P and RGB8) at all */
    { &MEDIASUBTYPE_YV12, 12, MAKEFOURCC('Y','V','1','2'), FALSE, TRUE },
    { &MEDIASUBTYPE_Y41P, 12, MAKEFOURCC('Y','4','1','P'), TRUE, FALSE },
    { &MEDIASUBTYPE_YUY2, 16, MAKEFOURCC('Y','U','Y','2'), TRUE, TRUE },
    { &MEDIASUBTYPE_UYVY, 16, MAKEFOURCC('U','Y','V','Y'), TRUE, TRUE },
    { &MEDIASUBTYPE_RGB24, 24, BI_RGB, TRUE, TRUE },
    { &MEDIASUBTYPE_RGB32, 32, BI_RGB, TRUE, TRUE },
    { &MEDIASUBTYPE_RGB565, 16, BI_BITFIELDS, TRUE, TRUE },
    { &MEDIASUBTYPE_RGB555, 16, BI_RGB, TRUE, TRUE },
    { &MEDIASUBTYPE_RGB8, 8, BI_RGB, TRUE, FALSE },
};

static void init_video_mt(AM_MEDIA_TYPE *mt, VIDEOINFOHEADER *format, unsigned type_idx)
{
    memcpy(mt, &yuy2_mt, sizeof(AM_MEDIA_TYPE));
    memcpy(format, &yuy2_format, sizeof(VIDEOINFOHEADER));
    mt->subtype = *video_types[type_idx].subtype;
    mt->pbFormat = (BYTE*)format;
    format->bmiHeader.biBitCount = video_types[type_idx].bits_per_pixel;
    format->bmiHeader.biCompression = video_types[type_idx].compression;
    format->bmiHeader.biSizeImage = format->bmiHeader.biBitCount * format->bmiHeader.biWidth * format->bmiHeader.biHeight / 8;
    format->dwBitRate = format->bmiHeader.biSizeImage*8 * 25;
    mt->lSampleSize = format->bmiHeader.biSizeImage;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB8))
        format->bmiHeader.biClrUsed = 0x100;
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
    IBaseFilter *filter = create_mpeg_video_codec();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);

    IBaseFilter_FindPin(filter, L"In", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IMediaPosition, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);

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
    hr = CoCreateInstance(&CLSID_CMpegVideoCodec, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_CMpegVideoCodec, &test_outer, CLSCTX_INPROC_SERVER,
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

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
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

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[3];
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
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 3, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 3);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
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
    IBaseFilter *filter = create_mpeg_video_codec();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins didn't match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IBaseFilter_FindPin(filter, L"Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins didn't match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IBaseFilter_FindPin(filter, L"XForm In", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_FindPin(filter, L"XForm Out", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_FindPin(filter, L"output pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
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

    hr = IBaseFilter_FindPin(filter, L"Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"Output"), "Got name %s.\n", debugstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"Out"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[1];
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

    IBaseFilter_FindPin(filter, L"Out", &pin);

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

static void test_media_types(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryAccept(pin, &mpeg_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt = mpeg_mt;
    mt.subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt = mpeg_mt;
    mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    hr = IPin_QueryAccept(pin, &yuy2_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

struct testqc
{
    IQualityControl IQualityControl_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
    IBaseFilter *notify_sender;
    Quality notify_quality;
    HRESULT notify_hr;
};

static struct testqc *impl_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct testqc, IQualityControl_iface);
}

static HRESULT WINAPI testqc_QueryInterface(IQualityControl *iface, REFIID iid, void **out)
{
    struct testqc *qc = impl_from_IQualityControl(iface);
    return IUnknown_QueryInterface(qc->outer_unk, iid, out);
}

static ULONG WINAPI testqc_AddRef(IQualityControl *iface)
{
    struct testqc *qc = impl_from_IQualityControl(iface);
    return IUnknown_AddRef(qc->outer_unk);
}

static ULONG WINAPI testqc_Release(IQualityControl *iface)
{
    struct testqc *qc = impl_from_IQualityControl(iface);
    return IUnknown_Release(qc->outer_unk);
}

static HRESULT WINAPI testqc_Notify(IQualityControl *iface, IBaseFilter *sender, Quality q)
{
    struct testqc *qc = impl_from_IQualityControl(iface);

    qc->notify_sender = sender;
    qc->notify_quality = q;

    return qc->notify_hr;
}

static HRESULT WINAPI testqc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IQualityControlVtbl testqc_vtbl =
{
    testqc_QueryInterface,
    testqc_AddRef,
    testqc_Release,
    testqc_Notify,
    testqc_SetSink,
};

static struct testqc *impl_from_qc_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct testqc, IUnknown_inner);
}

static HRESULT WINAPI testqc_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct testqc *qc = impl_from_qc_IUnknown(iface);

    if (IsEqualIID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualIID(iid, &IID_IQualityControl))
        *out = &qc->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI testqc_inner_AddRef(IUnknown *iface)
{
    struct testqc *qc = impl_from_qc_IUnknown(iface);
    return InterlockedIncrement(&qc->refcount);
}

static ULONG WINAPI testqc_inner_Release(IUnknown *iface)
{
    struct testqc *qc = impl_from_qc_IUnknown(iface);
    return InterlockedDecrement(&qc->refcount);
}

static const IUnknownVtbl testqc_inner_vtbl =
{
    testqc_inner_QueryInterface,
    testqc_inner_AddRef,
    testqc_inner_Release,
};

static void testqc_init(struct testqc *qc, IUnknown *outer)
{
    memset(qc, 0, sizeof(*qc));
    qc->IQualityControl_iface.lpVtbl = &testqc_vtbl;
    qc->IUnknown_inner.lpVtbl = &testqc_inner_vtbl;
    qc->outer_unk = outer ? outer : &qc->IUnknown_inner;
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    struct strmbase_sink sink;
    struct testqc *qc;
    const AM_MEDIA_TYPE *mt;
    unsigned int got_sample, got_new_segment, got_eos, got_begin_flush, got_end_flush;
    REFERENCE_TIME expected_start_time;
    REFERENCE_TIME expected_stop_time;
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
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    return E_NOINTERFACE;
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
    REFERENCE_TIME start, stop;
    AM_MEDIA_TYPE *mt = (AM_MEDIA_TYPE *)0xdeadbeef;
    HRESULT hr;
    LONG size;

    size = IMediaSample_GetSize(sample);
    ok(size == filter->sink.pin.mt.lSampleSize, "Got size %lu, expected %lu.\n", size, filter->sink.pin.mt.lSampleSize);
    size = IMediaSample_GetActualDataLength(sample);
    ok(size == filter->sink.pin.mt.lSampleSize, "Got actual size %lu, expected %lu.\n", size, filter->sink.pin.mt.lSampleSize);
    ok(IMediaSample_GetActualDataLength(sample) <= IMediaSample_GetSize(sample), "Buffer overflow");

    start = 0xdeadbeef;
    stop = 0xdeadbeef;
    hr = IMediaSample_GetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSample_GetMediaType(sample, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    if (mt)
        DeleteMediaType(mt);

    if (filter->got_sample == 0 && filter->expected_start_time != (REFERENCE_TIME)-1)
    {
        ok(start == filter->expected_start_time, "Got start time %s, expected %s.\n",
                wine_dbgstr_longlong(start), wine_dbgstr_longlong(filter->expected_start_time));
        todo_wine
            ok(stop == filter->expected_stop_time, "Got stop time %s, expected %s.\n",
                wine_dbgstr_longlong(stop), wine_dbgstr_longlong(filter->expected_stop_time));
    }

    ++filter->got_sample;

    return S_OK;
}

static HRESULT testsink_new_segment(struct strmbase_sink *iface,
        REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_new_segment;
    ok(start == 10000, "Got start %s.\n", wine_dbgstr_longlong(start));
    ok(stop == 20000, "Got stop %s.\n", wine_dbgstr_longlong(stop));
    ok(rate == 1.0, "Got rate %.16e.\n", rate);
    return S_OK;
}

static HRESULT testsink_eos(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_eos;
    return S_OK;
}

static HRESULT testsink_begin_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_begin_flush;
    return S_OK;
}

static HRESULT testsink_end_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_end_flush;
    return S_OK;
}

static const struct strmbase_sink_ops testsink_ops =
{
    .base.pin_query_interface = testsink_query_interface,
    .base.pin_get_media_type = testsink_get_media_type,
    .sink_connect = testsink_connect,
    .pfnReceive = testsink_Receive,
    .sink_new_segment = testsink_new_segment,
    .sink_eos = testsink_eos,
    .sink_begin_flush = testsink_begin_flush,
    .sink_end_flush = testsink_end_flush,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"source", &testsource_ops);
    strmbase_sink_init(&filter->sink, &filter->filter, L"sink", &testsink_ops, NULL);
}

static void test_sink_allocator(IMemInputPin *input)
{
    IMemAllocator *req_allocator, *ret_allocator;
    ALLOCATOR_PROPERTIES props, ret_props;
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

    props.cBuffers = 1;
    props.cbBuffer = 256;
    props.cbAlign = 1;
    props.cbPrefix = 0;
    hr = IMemAllocator_SetProperties(req_allocator, &props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_allocator == req_allocator, "Allocators didn't match.\n");

    IMemAllocator_Release(req_allocator);
    IMemAllocator_Release(ret_allocator);
}

static void test_source_allocator(IFilterGraph2 *graph, IMediaControl *control,
        IPin *sink, IPin *source, struct testfilter *testsource, struct testfilter *testsink,
        unsigned type_idx, BOOL should_work)
{
    ALLOCATOR_PROPERTIES props, req_props = {2, 30000, 32, 0};
    IMemAllocator *allocator;
    IMediaSample *sample;
    VIDEOINFOHEADER format;
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    hr = IFilterGraph2_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &mpeg_mt);
    ok(hr == S_OK, "(%u) Got hr %#lx.\n", type_idx, hr);

    init_video_mt(&mt, &format, type_idx);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    if (!should_work)
    {
        ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "(%u) Got hr %#lx.\n", type_idx, hr);
        IFilterGraph2_Disconnect(graph, sink);
        IFilterGraph2_Disconnect(graph, &testsource->source.pin.IPin_iface);
        return;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == mt.lSampleSize, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        IMediaSample_Release(sample);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    init_video_mt(&mt, &format, type_idx);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == mt.lSampleSize, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator);
    testsink->sink.pAllocator = allocator;

    hr = IMemAllocator_SetProperties(allocator, &req_props, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_video_mt(&mt, &format, type_idx);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(testsink->sink.pAllocator == allocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == mt.lSampleSize, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    IFilterGraph2_Disconnect(graph, sink);
    IFilterGraph2_Disconnect(graph, &testsource->source.pin.IPin_iface);
}

static void test_quality_control(IFilterGraph2 *graph, IBaseFilter *filter,
        IPin *sink, IPin *source, struct testfilter *testsource, struct testfilter *testsink)
{
    struct testqc testsource_qc;
    IQualityControl *source_qc;
    IQualityControl *sink_qc;
    Quality quality = {0};
    struct testqc qc;
    HRESULT hr;

    testqc_init(&testsource_qc, testsource->filter.outer_unk);
    testqc_init(&qc, NULL);

    testsource->qc = &testsource_qc;

    hr = IPin_QueryInterface(sink, &IID_IQualityControl, (void **)&sink_qc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_QueryInterface(source, &IID_IQualityControl, (void **)&source_qc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &mpeg_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testsource_qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsource_qc.notify_sender == (IBaseFilter *)0xdeadbeef, "Got sender %p.\n",
            testsource_qc.notify_sender);

    hr = IQualityControl_SetSink(sink_qc, &qc.IQualityControl_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == (IBaseFilter *)0xdeadbeef, "Got sender %p.\n", qc.notify_sender);

    qc.notify_hr = E_FAIL;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_hr = S_OK;

    IFilterGraph2_Disconnect(graph, sink);
    IFilterGraph2_Disconnect(graph, &testsource->source.pin.IPin_iface);

    IQualityControl_Release(source_qc);
    IQualityControl_Release(sink_qc);

    testsource->qc = NULL;
}

static void test_send_sample(IMemInputPin *input, IMediaSample *sample, const BYTE *data, LONG len)
{
    BYTE *target_data;
    HRESULT hr;
    LONG size;
    hr = IMediaSample_GetPointer(sample, &target_data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    size = IMediaSample_GetSize(sample);
    ok(size >= len, "Got size %ld, expected at least %ld.\n", size, len);

    memcpy(target_data, data, len);
    hr = IMediaSample_SetActualDataLength(sample, len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_send_video(IMemInputPin *input, IMediaSample *sample)
{
    /* gst-launch-1.0 -v videotestsrc pattern=black num-buffers=10 ! video/x-raw,width=32,height=24 ! mpeg2enc ! filesink location=empty-es2.mpg */
    /* then truncate to taste */
    /* each 00 00 01 b3 or 00 00 01 00 starts a new frame, except the first 00 00 01 00 after a 00 00 01 b3 */
    static const BYTE empty_mpg_frames[] = {
        0x00, 0x00, 0x01, 0xb3, 0x02, 0x00, 0x18, 0x15, 0x02, 0xbf, 0x60, 0x9c,
        0x00, 0x00, 0x01, 0xb8, 0x00, 0x08, 0x00, 0x40,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x0f, 0xff, 0xf8,
        0x00, 0x00, 0x01, 0x01, 0x0b, 0xf8, 0x7d, 0x29, 0x48, 0x8b, 0x94, 0xa5, 0x22, 0x20,
        0x00, 0x00, 0x01, 0x02, 0x0b, 0xf8, 0x7d, 0x2e, 0x7d, 0x2f, 0xcf, 0xc1, 0x04, 0x03, 0xa0, 0x11, 0xb1,
              0x41, 0x28, 0x88, 0x13, 0xb9, 0x6f, 0xcf, 0xc1, 0x04, 0x03, 0xa0, 0x11, 0xb1, 0x41, 0x28, 0x88,
              0x13, 0xb9, 0x6f, 0xa1, 0x4b, 0x9f, 0x48, 0x04, 0x10, 0x0e, 0x80, 0x46, 0xc5, 0x04, 0xa2,
              0x20, 0x4e, 0xe5, 0x80, 0x41, 0x00, 0xe8, 0x04, 0x6c, 0x50, 0x4a, 0x22, 0x04, 0xee, 0x58,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x57, 0xff, 0xf9, 0x80,
        0x00, 0x00, 0x01, 0x01, 0x0a, 0x79, 0xc0,
        0x00, 0x00, 0x01, 0x02, 0x0a, 0x79, 0xc0,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x97, 0xff, 0xf9, 0x80,
        0x00, 0x00, 0x01, 0x01, 0x0a, 0x79, 0xc0,
        0x00, 0x00, 0x01, 0x02, 0x0a, 0x79, 0xc0,
    };
    static const BYTE empty_mpg_eos[] = {
        0x00, 0x00, 0x01, 0xb7,
    };
    HRESULT hr;
    IPin *pin;

    /* native won't emit anything until an unknown-sized internal buffer is filled, or EOS is announced */
    test_send_sample(input, sample, empty_mpg_frames, ARRAY_SIZE(empty_mpg_frames));
    test_send_sample(input, sample, empty_mpg_eos, ARRAY_SIZE(empty_mpg_eos));

    hr = IMemInputPin_QueryInterface(input, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pin);
}

static void test_sample_processing(IMediaControl *control, IMemInputPin *input, struct testfilter *sink)
{
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    IPin *pin;
    LONG size;

    sink->got_sample = 0;
    sink->got_eos = 0;

    hr = IMemInputPin_QueryInterface(input, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    size = IMediaSample_GetSize(sample);
    ok(size == 256, "Got size %ld.\n", size);

    hr = IMediaSample_SetTime(sample, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = 0;
    sink->expected_stop_time = 0;
    hr = IMediaSample_SetTime(sample, &sink->expected_start_time, &sink->expected_stop_time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_send_video(input, sample);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
    ok(sink->got_eos == 1, "Got %u calls to EndOfStream().\n", sink->got_eos);
    sink->got_sample = 0;
    sink->got_eos = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 22222;
    hr = IMediaSample_SetTime(sample, &start, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = -1; /* native returns start and stop 0xff80000000000001 */
    test_send_video(input, sample);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
    ok(sink->got_eos == 1, "Got %u calls to EndOfStream().\n", sink->got_eos);
    sink->got_sample = 0;
    sink->got_eos = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 22222;
    stop = 33333;
    hr = IMediaSample_SetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = 22222;
    sink->expected_stop_time = 22222;
    test_send_video(input, sample);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
    ok(sink->got_eos == 1, "Got %u calls to EndOfStream().\n", sink->got_eos);
    sink->got_sample = 0;
    sink->got_eos = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    hr=IMemAllocator_Decommit(allocator);
    IPin_Release(pin);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_streaming_events(IMediaControl *control, IPin *sink,
        IMemInputPin *input, struct testfilter *testsink)
{
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    IPin *pin;

    testsink->got_new_segment = 0;
    testsink->got_sample = 0;
    testsink->got_eos = 0;
    testsink->got_begin_flush = 0;
    testsink->got_end_flush = 0;

    hr = IMemInputPin_QueryInterface(input, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 0;
    stop = 120000;
    hr = IMediaSample_SetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!testsink->got_new_segment, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);
    hr = IPin_NewSegment(sink, 10000, 20000, 1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_new_segment == 1, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);

    ok(!testsink->got_eos, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!testsink->got_sample, "Got %u calls to Receive().\n", testsink->got_sample);
    ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    testsink->got_eos = 0;

    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);

    testsink->expected_start_time = 0;
    testsink->expected_stop_time = 0;
    test_send_video(input, sample);
    ok(testsink->got_sample >= 1, "Got %u calls to Receive().\n", testsink->got_sample);
    testsink->got_sample = 0;

    ok(!testsink->got_begin_flush, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);
    hr = IPin_BeginFlush(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_begin_flush == 1, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_EndOfStream(sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    ok(!testsink->got_end_flush, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);
    hr = IPin_EndFlush(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_end_flush == 1, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testsink->expected_start_time = 0;
    testsink->expected_stop_time = 0;
    test_send_video(input, sample);
    ok(testsink->got_sample >= 1, "Got %u calls to Receive().\n", testsink->got_sample);
    testsink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemAllocator_Decommit(allocator);
    IPin_Release(pin);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_connect_pin(void)
{
    IBaseFilter *filter = create_mpeg_video_codec();
    struct testfilter testsource, testsink;
    AM_MEDIA_TYPE mt, source_mt, *pmt;
    IPin *sink, *source, *peer;
    VIDEOINFOHEADER req_format;
    IEnumMediaTypes *enummt;
    IMediaControl *control;
    IMemInputPin *meminput;
    AM_MEDIA_TYPE req_mt;
    IFilterGraph2 *graph;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    testfilter_init(&testsource);
    testfilter_init(&testsink);
    IFilterGraph2_AddFilter(graph, &testsink.filter.IBaseFilter_iface, L"sink");
    IFilterGraph2_AddFilter(graph, &testsource.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"MPEG video decoder");
    IBaseFilter_FindPin(filter, L"In", &sink);
    IBaseFilter_FindPin(filter, L"Out", &source);
    IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&meminput);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    test_quality_control(graph, filter, sink, source, &testsource, &testsink);

    for (i=0;i<ARRAY_SIZE(video_types);++i)
    {
        BOOL should_work;
        if (winetest_platform_is_wine) should_work = video_types[i].works_wine;
        else should_work = video_types[i].works_native;
        test_source_allocator(graph, control, sink, source, &testsource, &testsink, i, should_work);
    }

    /* Test sink connection. */

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &mpeg_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    req_mt = mpeg_mt;
    req_mt.subtype = MEDIASUBTYPE_RGB24;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &mpeg_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsource.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &mpeg_mt), "Media types didn't match.\n");
    ok(compare_media_types(&testsource.source.pin.mt, &mpeg_mt), "Media types didn't match.\n");
    FreeMediaType(&mt);

    hr = IPin_QueryAccept(source, &yuy2_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    req_mt = yuy2_mt;
    req_mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    req_mt = yuy2_mt;
    req_mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    for (i=0;i<ARRAY_SIZE(video_types);++i)
    {
        init_video_mt(&req_mt, &req_format, i);
        hr = IPin_QueryAccept(source, &req_mt);
        if (!winetest_platform_is_wine && !video_types[i].works_native)
            continue;
        todo_wine_if(!video_types[i].works_wine)
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    req_mt = yuy2_mt;
    req_mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_sink_allocator(meminput);

    hr = IPin_EnumMediaTypes(source, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    i = 0;
    while (i < ARRAY_SIZE(video_types))
    {
        VIDEOINFOHEADER expect_format;
        AM_MEDIA_TYPE expect_mt;

        hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        while (IsEqualGUID(&pmt->formattype, &FORMAT_VideoInfo2))
        {
            DeleteMediaType(pmt);
            hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
            ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        }

        init_video_mt(&expect_mt, &expect_format, i);

        ok(!memcmp(pmt, &expect_mt, offsetof(AM_MEDIA_TYPE, cbFormat)),
                "%u: Media types didn't match.\n", i);
        ok(!memcmp(pmt->pbFormat, &expect_format, sizeof(VIDEOINFOHEADER)),
                "%u: Format blocks didn't match.\n", i);
        DeleteMediaType(pmt);

        ++i;
        while (winetest_platform_is_wine && !video_types[i].works_wine)
            ++i;
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumMediaTypes_Release(enummt);

    /* Test source connection. */

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    /* Exact connection. */

    for (i=0;i<ARRAY_SIZE(video_types);i++)
    {
        if (!winetest_platform_is_wine && !video_types[i].works_native)
            continue;

        hr = IMediaControl_Pause(control);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        init_video_mt(&req_mt, &req_format, i);
        hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
        ok(hr == VFW_E_NOT_STOPPED, "%u: Got hr %#lx.\n", i, hr);
        hr = IMediaControl_Stop(control);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);

        init_video_mt(&req_mt, &req_format, i);
        hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
        todo_wine_if(!video_types[i].works_wine)
            ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        if (hr != S_OK)
            continue;

        hr = IPin_ConnectedTo(source, &peer);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        ok(peer == &testsink.sink.pin.IPin_iface, "%u: Got peer %p.\n", i, peer);
        IPin_Release(peer);

        hr = IPin_ConnectionMediaType(source, &mt);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        ok(compare_media_types(&mt, &req_mt), "%u: Media types didn't match.\n", i);
        ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "%u: Media types didn't match.\n", i);
        FreeMediaType(&mt);

        hr = IMediaControl_Pause(control);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        hr = IFilterGraph2_Disconnect(graph, source);
        ok(hr == VFW_E_NOT_STOPPED, "%u: Got hr %#lx.\n", i, hr);
        hr = IMediaControl_Stop(control);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);

        test_sample_processing(control, meminput, &testsink);
        test_streaming_events(control, sink, meminput, &testsink);

        hr = IFilterGraph2_Disconnect(graph, source);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        hr = IFilterGraph2_Disconnect(graph, source);
        ok(hr == S_FALSE, "%u: Got hr %#lx.\n", i, hr);
        ok(testsink.sink.pin.peer == source, "%u: Got peer %p.\n", i, testsink.sink.pin.peer);
        IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

        init_video_mt(&req_mt, &req_format, i);
        req_mt.lSampleSize = 999;
        req_mt.bTemporalCompression = TRUE;
        hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
        ok(hr == S_OK, "%u: Got hr %#lx.\n", i, hr);
        ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "%u: Media types didn't match.\n", i);
        IFilterGraph2_Disconnect(graph, source);
        IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);
    }

    req_mt = yuy2_mt;
    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    /* Connection with wildcards. */

    source_mt = yuy2_mt;

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&testsink.sink.pin.mt.majortype, &source_mt.majortype), "Media types didn't match.\n");
    /* don't worry too much about sub/format type */
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt = yuy2_mt;
    req_format = yuy2_format;
    req_mt.pbFormat = (BYTE *)&req_format;
    req_mt.majortype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&testsink.sink.pin.mt.majortype, &MEDIATYPE_Video), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt = yuy2_mt;
    req_format = yuy2_format;
    req_mt.pbFormat = (BYTE *)&req_format;
    req_mt.majortype = GUID_NULL;
    req_format.dwBitRate = 0; /* native looks at this specific field if the major type is GUID_NULL */
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    todo_wine ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        IFilterGraph2_Disconnect(graph, source);
        IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);
    }

    req_mt = yuy2_mt;
    req_mt.majortype = MEDIATYPE_Audio;
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    /* Test enumeration of sink media types. */

    req_mt = yuy2_mt;
    req_mt.majortype = MEDIATYPE_Audio;
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = GUID_NULL;
    testsink.mt = &req_mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt = yuy2_mt;
    req_mt.lSampleSize = 444;
    testsink.mt = &req_mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");

    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsource.source.pin.peer == sink, "Got peer %p.\n", testsource.source.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsource.source.pin.IPin_iface);

    IMemInputPin_Release(meminput);
    IPin_Release(sink);
    IPin_Release(source);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsource.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(mpegvideo)
{
    IBaseFilter *filter;

    CoInitialize(NULL);

    if (FAILED(CoCreateInstance(&CLSID_CMpegVideoCodec, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        skip("Failed to create MPEG video decoder instance.\n");
        return;
    }
    IBaseFilter_Release(filter);

    test_interfaces();
    test_aggregation();
    test_unconnected_filter_state();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_enum_media_types();
    test_media_types();
    test_connect_pin();

    CoUninitialize();
}
