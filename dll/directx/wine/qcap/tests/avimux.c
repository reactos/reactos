/*
 * AVI muxer filter unit tests
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
#include "wine/test.h"

static const GUID testguid = {0xfacade};

static IBaseFilter *create_avi_mux(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
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
    IBaseFilter *filter = create_avi_mux();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IConfigAviMux, TRUE);
    check_interface(filter, &IID_IConfigInterleaving, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IPersistMediaPropertyBag, TRUE);
    check_interface(filter, &IID_ISpecifyPropertyPages, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, L"AVI Out", &pin);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Input 01", &pin);

    check_interface(pin, &IID_IAMStreamControl, TRUE);
    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IPropertyBag, TRUE);
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);

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
    hr = CoCreateInstance(&CLSID_AviDest, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_AviDest, &test_outer, CLSCTX_INPROC_SERVER,
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
    IBaseFilter *filter = create_avi_mux();
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
    IBaseFilter *filter = create_avi_mux();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"AVI Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins didn't match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IBaseFilter_FindPin(filter, L"Input 01", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins didn't match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_avi_mux();
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;

    hr = IBaseFilter_FindPin(filter, L"AVI Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, L"AVI Out"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!lstrcmpW(id, L"AVI Out"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Input 01", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, L"Input 01"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!lstrcmpW(id, L"Input 01"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_media_types(void)
{
    IBaseFilter *filter = create_avi_mux();
    AM_MEDIA_TYPE mt = {}, *pmt;
    VIDEOINFOHEADER vih = {};
    IEnumMediaTypes *enummt;
    WAVEFORMATEX wfx = {};
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"AVI Out", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Stream), "Got major type %s\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_Avi), "Got subtype %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %lu.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &GUID_NULL), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(!pmt->cbFormat, "Got format size %lu.\n", pmt->cbFormat);
    ok(!pmt->pbFormat, "Got format block %lu.\n", pmt->cbFormat);

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    pmt->formattype = FORMAT_VideoInfo;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pmt->majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    pmt->majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->majortype = MEDIATYPE_Stream;

    pmt->subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_RGB8;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    pmt->subtype = MEDIASUBTYPE_Avi;

    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Input 01", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    mt.majortype = MEDIATYPE_Audio;
    mt.formattype = FORMAT_WaveFormatEx;
    mt.cbFormat = sizeof(wfx);
    mt.pbFormat = (BYTE *)&wfx;
    wfx.nBlockAlign = 1;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt.subtype = MEDIASUBTYPE_RGB8;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt.majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = MEDIATYPE_Interleaved;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    mt.majortype = MEDIATYPE_Video;
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(vih);
    mt.pbFormat = (BYTE *)&vih;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt.majortype = MEDIATYPE_Audio;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    mt.majortype = MEDIATYPE_Interleaved;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_avi_mux();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"AVI Out", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
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

    IBaseFilter_FindPin(filter, L"Input 01", &pin);

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

static void test_seeking(void)
{
    IBaseFilter *filter = create_avi_mux();
    LONGLONG time, current, stop;
    IMediaSeeking *seeking;
    unsigned int i;
    GUID format;
    HRESULT hr;
    DWORD caps;
    ULONG ref;

    static const struct
    {
        const GUID *guid;
        HRESULT hr;
    }
    format_tests[] =
    {
        {&TIME_FORMAT_MEDIA_TIME, S_OK},
        {&TIME_FORMAT_BYTE, S_OK},

        {&TIME_FORMAT_NONE, S_FALSE},
        {&TIME_FORMAT_FRAME, S_FALSE},
        {&TIME_FORMAT_SAMPLE, S_FALSE},
        {&TIME_FORMAT_FIELD, S_FALSE},
        {&testguid, S_FALSE},
    };

    IBaseFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(caps == (AM_SEEKING_CanGetCurrentPos | AM_SEEKING_CanGetDuration), "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanGetCurrentPos, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(caps == AM_SEEKING_CanGetCurrentPos, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanDoSegments;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!caps, "Got caps %#lx.\n", caps);

    for (i = 0; i < ARRAY_SIZE(format_tests); ++i)
    {
        hr = IMediaSeeking_IsFormatSupported(seeking, format_tests[i].guid);
        todo_wine ok(hr == format_tests[i].hr, "Got hr %#lx for format %s.\n", hr, wine_dbgstr_guid(format_tests[i].guid));
    }

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_SAMPLE);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_BYTE), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_BYTE);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_NoPositioning,
            &stop, AM_SEEKING_NoPositioning);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetPositions(seeking, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, NULL, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetDuration(seeking, &time);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!time, "Got duration %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(!time, "Got duration %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IMediaSeeking_Release(seeking);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
}

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_avi_mux();
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(avimux)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_seeking();
    test_unconnected_filter_state();

    CoUninitialize();
}
