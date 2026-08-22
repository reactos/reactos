/*
 * AVI decompressor filter unit tests
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
#include "vfw.h"
#include "wmcodecdsp.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static const DWORD test_handler = mmioFOURCC('w','t','s','t');
static const GUID test_subtype = {mmioFOURCC('w','t','s','t'),0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};

static int testmode;

static IBaseFilter *create_avi_dec(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AVIDec, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static BITMAPINFOHEADER sink_bitmap_info, source_bitmap_info;

static LRESULT CALLBACK vfw_driver_proc(DWORD_PTR id, HDRVR driver, UINT msg,
        LPARAM lparam1, LPARAM lparam2)
{
    static int in_begin;

    if (winetest_debug > 1)
        trace("id %#Ix, driver %p, msg %#x, lparam1 %#Ix, lparam2 %#Ix.\n",
                id, driver, msg, lparam1, lparam2);

    switch (msg)
    {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
        return 1;

    case ICM_DECOMPRESS_GET_FORMAT:
    {
        BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lparam2;
        BITMAPINFOHEADER *in = (BITMAPINFOHEADER *)lparam1;

        if (!out)
            return sizeof(BITMAPINFOHEADER);

        ok(!memcmp(in, &sink_bitmap_info, sizeof(BITMAPINFOHEADER)), "Input types didn't match.\n");

        out->biSize = sizeof(BITMAPINFOHEADER);
        out->biCompression = mmioFOURCC('I','4','2','0');
        out->biWidth = 29;
        out->biHeight = 24;
        out->biBitCount = 12;
        out->biSizeImage = 24 * (((29 * 12 + 31) / 8) & ~3);

        return ICERR_OK;
    }

    case ICM_DECOMPRESS_QUERY:
    {
        BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lparam2;
        BITMAPINFOHEADER *in = (BITMAPINFOHEADER *)lparam1;

        if (in->biCompression != test_handler || in->biBitCount != 16)
            return ICERR_BADFORMAT;

        if (out && out->biCompression != mmioFOURCC('I','4','2','0'))
            return ICERR_BADFORMAT;

        return ICERR_OK;
    }

    case ICM_DECOMPRESS_GET_PALETTE:
    case ICM_DECOMPRESS_SET_PALETTE:
        return ICERR_UNSUPPORTED;

    case ICM_DECOMPRESS_BEGIN:
    {
        BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lparam2;
        BITMAPINFOHEADER *in = (BITMAPINFOHEADER *)lparam1;

        todo_wine_if (in->biSizeImage != sink_bitmap_info.biSizeImage)
            ok(!memcmp(in, &sink_bitmap_info, sizeof(BITMAPINFOHEADER)),
                    "Input types didn't match.\n");
        ok(!memcmp(out, &source_bitmap_info, sizeof(BITMAPINFOHEADER)),
                "Output types didn't match.\n");
        ok(!in_begin, "Got multiple ICM_DECOMPRESS_BEGIN messages.\n");
        in_begin = 1;
        return ICERR_OK;
    }

    case ICM_DECOMPRESS_END:
        ok(in_begin, "Got unmatched ICM_DECOMPRESS_END message.\n");
        in_begin = 0;
        return ICERR_OK;

    case ICM_DECOMPRESS:
    {
        BITMAPINFOHEADER expect_sink_format = sink_bitmap_info;
        ICDECOMPRESS *params = (ICDECOMPRESS *)lparam1;
        BYTE *output = params->lpOutput, expect[200];
        unsigned int i;

        ok(in_begin, "Got ICM_DECOMPRESS without ICM_DECOMPRESS_BEGIN.\n");

        ok(lparam2 == sizeof(ICDECOMPRESS), "Got size %Iu.\n", lparam2);

        if (testmode == 5 || testmode == 6)
            ok(params->dwFlags == ICDECOMPRESS_NOTKEYFRAME, "Got flags %#lx.\n", params->dwFlags);
        else if (testmode == 3)
            ok(params->dwFlags == ICDECOMPRESS_PREROLL, "Got flags %#lx.\n", params->dwFlags);
        else
            ok(params->dwFlags == 0, "Got flags %#lx.\n", params->dwFlags);

        expect_sink_format.biSizeImage = 200;
        ok(!memcmp(params->lpbiInput, &expect_sink_format, sizeof(BITMAPINFOHEADER)),
                "Input types didn't match.\n");
        ok(!memcmp(params->lpbiOutput, &source_bitmap_info, sizeof(BITMAPINFOHEADER)),
                "Output types didn't match.\n");
        ok(!params->ckid, "Got chunk id %#lx.\n", params->ckid);

        for (i = 0; i < 200; ++i)
            expect[i] = i;
        ok(!memcmp(params->lpInput, expect, 200), "Data didn't match.\n");
        for (i = 0; i < 24 * (((29 * 12 + 31) / 8) & ~3); ++i)
            output[i] = 111 - i;

        return ICERR_OK;
    }

    case ICM_GETINFO:
    {
        ICINFO *info = (ICINFO *)lparam1;
        info->fccType = ICTYPE_VIDEO;
        info->fccHandler = test_handler;
        return sizeof(ICINFO);
    }

    default:
        ok(0, "Got unexpected message %#x.\n", msg);
        return ICERR_UNSUPPORTED;
    }
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
    IBaseFilter *filter = create_avi_dec();
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
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, L"In", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IMediaPosition, TRUE);
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
    hr = CoCreateInstance(&CLSID_AVIDec, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_AVIDec, &test_outer, CLSCTX_INPROC_SERVER,
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
    IBaseFilter *filter = create_avi_dec();
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
    IBaseFilter *filter = create_avi_dec();
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
    IBaseFilter *filter = create_avi_dec();
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
    ok(!wcscmp(info.achName, L"XForm In"), "Got name %s.\n", wine_dbgstr_w(info.achName));
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

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"XForm Out"), "Got name %s.\n", wine_dbgstr_w(info.achName));
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

static void test_media_types(void)
{
    IBaseFilter *filter = create_avi_dec();
    AM_MEDIA_TYPE mt = {{0}}, *pmt;
    VIDEOINFOHEADER vih = {{0}};
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

    mt.majortype = MEDIATYPE_Video;
    mt.subtype = test_subtype;
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(VIDEOINFOHEADER);
    mt.pbFormat = (BYTE *)&vih;
    vih.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    vih.bmiHeader.biCompression = test_handler;
    vih.bmiHeader.biWidth = 32;
    vih.bmiHeader.biHeight = 24;
    vih.bmiHeader.biBitCount = 16;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    vih.bmiHeader.biBitCount = 32;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    vih.bmiHeader.biBitCount = 16;

    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Some versions of quartz check the major type; some do not. */

    mt.subtype = MEDIASUBTYPE_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = MEDIASUBTYPE_RGB24;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.subtype = test_subtype;

    mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.formattype = FORMAT_VideoInfo;

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    mt.subtype = MEDIASUBTYPE_RGB24;
    vih.bmiHeader.biCompression = BI_RGB;
    vih.bmiHeader.biBitCount = 24;
    vih.bmiHeader.biSizeImage= 32 * 24 * 3;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_avi_dec();
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

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_avi_dec();
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
    const AM_MEDIA_TYPE *mt;
    unsigned int got_sample, got_new_segment, got_eos, got_begin_flush, got_end_flush;
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

static DWORD WINAPI call_qc_notify(void *ptr)
{
    struct testfilter *filter = ptr;
    IQualityControl *qc;
    Quality q = { Famine, 2000, -10000000, 10000000 };

    IPin_QueryInterface(filter->sink.pin.peer, &IID_IQualityControl, (void**)&qc);
    /* don't worry too much about what it returns, just check that it doesn't deadlock */
    IQualityControl_Notify(qc, &filter->filter.IBaseFilter_iface, q);
    IQualityControl_Release(qc);

    return 0;
}

static HRESULT WINAPI testsink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    BYTE *data, expect[24 * (((29 * 12 + 31) / 8) & ~3)];
    REFERENCE_TIME start, stop;
    LONG size, i;
    HRESULT hr;

    ++filter->got_sample;

    size = IMediaSample_GetSize(sample);
    ok(size == 24 * (((29 * 12 + 31) / 8) & ~3), "Got size %lu.\n", size);
    size = IMediaSample_GetActualDataLength(sample);
    ok(size == 24 * (((29 * 12 + 31) / 8) & ~3), "Got valid size %lu.\n", size);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; i < size; ++i)
        expect[i] = 111 - i;
    ok(!memcmp(data, expect, size), "Data didn't match.\n");

    hr = IMediaSample_GetTime(sample, &start, &stop);
    if (testmode == 0)
    {
        ok(hr == VFW_E_SAMPLE_TIME_NOT_SET, "Got hr %#lx.\n", hr);
    }
    else if (testmode == 1)
    {
        ok(hr == VFW_S_NO_STOP_TIME, "Got hr %#lx.\n", hr);
        ok(start == 20000, "Got start time %s.\n", wine_dbgstr_longlong(start));
    }
    else
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(start == 20000, "Got start time %s.\n", wine_dbgstr_longlong(start));
        ok(stop == 30000, "Got stop time %s.\n", wine_dbgstr_longlong(stop));
    }

    hr = IMediaSample_GetMediaTime(sample, &start, &stop);
    ok(hr == VFW_E_MEDIA_TIME_NOT_SET, "Got hr %#lx.\n", hr);
    hr = IMediaSample_IsDiscontinuity(sample);
    todo_wine_if (testmode == 5) ok(hr == (testmode == 4) ? S_OK : S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSample_IsPreroll(sample);
    todo_wine_if (testmode == 3) ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSample_IsSyncPoint(sample);
    todo_wine_if (testmode == 5 || testmode == 6) ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (testmode == 7)
    {
        HANDLE h = CreateThread(NULL, 0, call_qc_notify, filter, 0, NULL);
        ok(WaitForSingleObject(h, 1000) == WAIT_OBJECT_0, "Didn't expect deadlock.\n");
        CloseHandle(h);
    }

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

static void test_sample_processing(IMediaControl *control, IMemInputPin *input, struct testfilter *sink)
{
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    LONG size, i;
    HRESULT hr;
    BYTE *data;

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

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    size = IMediaSample_GetSize(sample);
    ok(size == 256, "Got size %ld.\n", size);
    for (i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 10000;
    stop = 20000;
    hr = IMediaSample_SetMediaTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetSyncPoint(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 0;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample == 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    start = 20000;
    hr = IMediaSample_SetTime(sample, &start, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 1;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample == 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    stop = 30000;
    hr = IMediaSample_SetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 2;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample == 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    hr = IMediaSample_SetPreroll(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 3;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!sink->got_sample, "Got %u calls to Receive().\n", sink->got_sample);

    hr = IMediaSample_SetPreroll(sample, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetDiscontinuity(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 4;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(sink->got_sample == 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    hr = IMediaSample_SetSyncPoint(sample, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 5;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!sink->got_sample, "Got %u calls to Receive().\n", sink->got_sample);

    hr = IMediaSample_SetDiscontinuity(sample, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 6;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!sink->got_sample, "Got %u calls to Receive().\n", sink->got_sample);

    testmode = 7;
    hr = IMediaSample_SetSyncPoint(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink->got_sample = 0;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample == 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_streaming_events(IMediaControl *control, IPin *sink,
        IMemInputPin *input, struct testfilter *testsink)
{
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    BYTE *data;
    LONG i;

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetSyncPoint(sample, TRUE);
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

    testmode = 0;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_sample == 1, "Got %u calls to Receive().\n", testsink->got_sample);
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

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_sample == 1, "Got %u calls to Receive().\n", testsink->got_sample);
    testsink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_connect_pin(void)
{
    VIDEOINFOHEADER req_format =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biCompression = test_handler,
        .bmiHeader.biWidth = 29,
        .bmiHeader.biHeight = 24,
        .bmiHeader.biBitCount = 16,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = test_subtype,
        .formattype = FORMAT_VideoInfo,
        .lSampleSize = 888,
        .cbFormat = sizeof(req_format),
        .pbFormat = (BYTE *)&req_format,
    };
    struct testfilter testsource, testsink;
    IBaseFilter *filter = create_avi_dec();
    AM_MEDIA_TYPE mt, source_mt, *pmt;
    IPin *sink, *source, *peer;
    IEnumMediaTypes *enummt;
    IMediaControl *control;
    IMemInputPin *meminput;
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
    IFilterGraph2_AddFilter(graph, filter, L"AVI decoder");
    IBaseFilter_FindPin(filter, L"In", &sink);
    IBaseFilter_FindPin(filter, L"Out", &source);
    IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&meminput);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

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

    req_mt.subtype = MEDIASUBTYPE_RGB24;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = test_subtype;

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
    FreeMediaType(&mt);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink_bitmap_info = req_format.bmiHeader;

    hr = IPin_EnumMediaTypes(source, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 9; ++i)
    {
        static const struct
        {
            const GUID *subtype;
            DWORD compression;
            WORD bpp;
        }
        expect[9] =
        {
            {&MEDIASUBTYPE_CLJR, mmioFOURCC('C','L','J','R'), 8},
            {&MEDIASUBTYPE_UYVY, mmioFOURCC('U','Y','V','Y'), 16},
            {&MEDIASUBTYPE_YUY2, mmioFOURCC('Y','U','Y','2'), 16},
            {&MEDIASUBTYPE_RGB32, BI_RGB, 32},
            {&MEDIASUBTYPE_RGB24, BI_RGB, 24},
            {&MEDIASUBTYPE_RGB565, BI_BITFIELDS, 16},
            {&MEDIASUBTYPE_RGB555, BI_RGB, 16},
            {&MEDIASUBTYPE_RGB8, BI_RGB, 8},
            {&MEDIASUBTYPE_I420, mmioFOURCC('I','4','2','0'), 12},
        };

        VIDEOINFOHEADER expect_format =
        {
            .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
            .bmiHeader.biWidth = 29,
            .bmiHeader.biHeight = 24,
            .bmiHeader.biBitCount = expect[i].bpp,
            .bmiHeader.biCompression = expect[i].compression,
            .bmiHeader.biSizeImage = 24 * (((29 * expect[i].bpp + 31) / 8) & ~3),
        };

        AM_MEDIA_TYPE expect_mt =
        {
            .majortype = MEDIATYPE_Video,
            .subtype = *expect[i].subtype,
            .bFixedSizeSamples = TRUE,
            .lSampleSize = 24 * (((29 * expect[i].bpp + 31) / 8) & ~3),
            .formattype = FORMAT_VideoInfo,
            .cbFormat = sizeof(VIDEOINFOHEADER),
            .pbFormat = (BYTE *)&expect_format,
        };

        hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!memcmp(pmt, &expect_mt, offsetof(AM_MEDIA_TYPE, cbFormat)),
                "%u: Media types didn't match.\n", i);
        ok(!memcmp(pmt->pbFormat, &expect_format, sizeof(VIDEOINFOHEADER)),
                "%u: Format blocks didn't match.\n", i);
        if (i == 5)
        {
            const VIDEOINFO *format = (VIDEOINFO *)pmt->pbFormat;

            ok(pmt->cbFormat == offsetof(VIDEOINFO, dwBitMasks[3]), "Got format size %lu.\n", pmt->cbFormat);
            ok(format->dwBitMasks[iRED] == 0xf800, "Got red bit mask %#lx.\n", format->dwBitMasks[iRED]);
            ok(format->dwBitMasks[iGREEN] == 0x07e0, "Got green bit mask %#lx.\n", format->dwBitMasks[iGREEN]);
            ok(format->dwBitMasks[iBLUE] == 0x001f, "Got blue bit mask %#lx.\n", format->dwBitMasks[iBLUE]);
        }

        hr = IPin_QueryAccept(source, pmt);
        ok(hr == (i == 8 ? S_OK : S_FALSE), "Got hr %#lx.\n", hr);

        if (i == 8)
            CopyMediaType(&source_mt, pmt);

        DeleteMediaType(pmt);
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    test_sink_allocator(meminput);

    /* Test source connection. */

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    /* Exact connection. */

    CopyMediaType(&req_mt, &source_mt);

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
    FreeMediaType(&mt);

    source_bitmap_info = ((VIDEOINFOHEADER *)req_mt.pbFormat)->bmiHeader;

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_sample_processing(control, meminput, &testsink);
    test_streaming_events(control, sink, meminput, &testsink);

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

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.formattype = FORMAT_VideoInfo;

    /* Connection with wildcards. */

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.subtype = MEDIASUBTYPE_I420;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = MEDIATYPE_Audio;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    /* Test enumeration of sink media types. */

    /* Our sink's proposed media type is sort of broken, but Windows 8+ returns
     * VFW_E_INVALIDMEDIATYPE for even perfectly reasonable ones. */
    testsink.mt = &req_mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES
            || broken(hr == VFW_E_INVALIDMEDIATYPE) /* Win8+ */, "Got hr %#lx.\n", hr);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.subtype = MEDIASUBTYPE_I420;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.lSampleSize = 444;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");

    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsource.source.pin.peer == sink, "Got peer %p.\n", testsource.source.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsource.source.pin.IPin_iface);

    FreeMediaType(&source_mt);
    FreeMediaType(&req_mt);
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

START_TEST(avidec)
{
    BOOL ret;

    ret = ICInstall(ICTYPE_VIDEO, test_handler, (LPARAM)vfw_driver_proc, NULL, ICINSTALL_FUNCTION);
    ok(ret, "Failed to install driver.\n");

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

    CoUninitialize();

    ret = ICRemove(ICTYPE_VIDEO, test_handler, 0);
    ok(ret, "Failed to remove driver.\n");
}
