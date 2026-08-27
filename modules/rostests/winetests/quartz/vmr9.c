/*
 * Video Mixing Renderer 9 unit tests
 *
 * Copyright 2019 Zebediah Figura
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

#include <stdint.h>
#define COBJMACROS
#include "ocidl.h"
#include "olectl.h"
#include "initguid.h"
#include "dshow.h"
#include "qedit.h"
#include "d3d9.h"
#include "vmr9.h"
#include "videoacc.h"
#include "wmcodecdsp.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static IBaseFilter *create_vmr9(DWORD mode)
{
    IBaseFilter *filter = NULL;
    IVMRFilterConfig9 *config;
    HRESULT hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (mode)
    {
        hr = IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig9, (void **)&config);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IVMRFilterConfig9_SetRenderingMode(config, mode);
        ok(hr == S_OK || broken(hr == E_FAIL), "Got hr %#lx.\n", hr);
        IVMRFilterConfig9_Release(config);
    }
    return filter;
}

static HRESULT set_mixing_mode(IBaseFilter *filter, DWORD count)
{
    IVMRFilterConfig9 *config;
    HRESULT hr;

    hr = IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_SetNumberOfStreams(config, count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IVMRFilterConfig9_Release(config);
    return hr;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
        && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static BOOL compare_double(double f, double g, unsigned int ulps)
{
    uint64_t x = *(ULONGLONG *)&f;
    uint64_t y = *(ULONGLONG *)&g;

    if (f < 0)
        x = ~x + 1;
    else
        x |= ((ULONGLONG)1)<<63;
    if (g < 0)
        y = ~y + 1;
    else
        y |= ((ULONGLONG)1)<<63;

    return (x>y ? x-y : y-x) <= ulps;
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

static void test_filter_config(void)
{
    IVMRFilterConfig9 *config;
    DWORD count, mode;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#lx.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == S_OK || broken(hr == E_FAIL), "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMR9Mode_Windowed, "Got mode %#lx.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == S_OK || broken(hr == E_FAIL), "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMR9Mode_Windowless, "Got mode %#lx.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Renderless);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMR9Mode_Renderless, "Got mode %#lx.\n", mode);

    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig9, (void **)&config);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    ok(hr == VFW_E_VMR_NOT_IN_MIXER_MODE, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_SetNumberOfStreams(config, 3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 3, "Got count %lu.\n", count);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMR9Mode_Windowed, "Got mode %#lx.\n", mode);

    /* Despite MSDN, you can still change the rendering mode after setting the
     * stream count. */
    hr = IVMRFilterConfig9_SetRenderingMode(config, VMR9Mode_Windowless);
    ok(hr == S_OK || broken(hr == E_FAIL), "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mode == VMR9Mode_Windowless, "Got mode %#lx.\n", mode);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 3, "Got count %lu.\n", count);

    ref = IVMRFilterConfig9_Release(config);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

#define check_interface_broken(a, b, c) check_interface_(__LINE__, a, b, c, TRUE)
#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c, FALSE)
static HRESULT check_interface_(unsigned int line, void *iface, REFIID riid, BOOL supported, BOOL is_broken)
{
    HRESULT hr, expected_hr, broken_hr;
    IUnknown *unknown = iface, *out;

    if (supported)
    {
        expected_hr = S_OK;
        broken_hr = E_NOINTERFACE;
    }
    else
    {
        expected_hr = E_NOINTERFACE;
        broken_hr = S_OK;
    }

    hr = IUnknown_QueryInterface(unknown, riid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr || broken(is_broken && hr == broken_hr),
            "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
    return hr;
}

static void test_common_interfaces(IBaseFilter *filter)
{
    IPin *pin;

    check_interface(filter, &IID_IAMCertifiedOutputProtection, TRUE);
    check_interface(filter, &IID_IAMFilterMiscFlags, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    todo_wine check_interface(filter, &IID_IKsPropertySet, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IQualityControl, TRUE);
    todo_wine check_interface(filter, &IID_IQualProp, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);
    check_interface(filter, &IID_IVMRAspectRatioControl9, TRUE);
    todo_wine check_interface(filter, &IID_IVMRDeinterlaceControl9, TRUE);
    check_interface(filter, &IID_IVMRFilterConfig9, TRUE);
    check_interface(filter, &IID_IVMRMixerBitmap9, TRUE);

    check_interface(filter, &IID_IAMVideoAccelerator, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IDirectDrawVideo, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVMRAspectRatioControl, FALSE);
    check_interface(filter, &IID_IVMRDeinterlaceControl, FALSE);
    check_interface(filter, &IID_IVMRFilterConfig, FALSE);
    check_interface(filter, &IID_IVMRMixerBitmap, FALSE);
    check_interface(filter, &IID_IVMRMixerControl, FALSE);
    check_interface(filter, &IID_IVMRMonitorConfig, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl, FALSE);

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

    check_interface(pin, &IID_IAMVideoAccelerator, TRUE);
    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);
}

static void test_interfaces(void)
{
    IBaseFilter *filter = create_vmr9(0);
    ULONG ref;

    test_common_interfaces(filter);

    check_interface(filter, &IID_IBasicVideo, TRUE);
    todo_wine check_interface(filter, &IID_IBasicVideo2, TRUE);
    check_interface(filter, &IID_IVideoWindow, TRUE);
    /* IVMRMonitorConfig9 may not be available if the d3d9 device has
     * insufficient support. */
    check_interface_broken(filter, &IID_IVMRMonitorConfig9, TRUE);

    check_interface(filter, &IID_IVMRMixerControl9, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    filter = create_vmr9(VMR9Mode_Windowless);
    test_common_interfaces(filter);

    /* IVMRMonitorConfig9 may not be available if the d3d9 device has
     * insufficient support. */
    check_interface_broken(filter, &IID_IVMRMonitorConfig9, TRUE);
    check_interface(filter, &IID_IVMRWindowlessControl9, TRUE);

    todo_wine check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IBasicVideo2, FALSE);
    todo_wine check_interface(filter, &IID_IVideoWindow, FALSE);
    check_interface(filter, &IID_IVMRMixerControl9, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    filter = create_vmr9(VMR9Mode_Renderless);
    test_common_interfaces(filter);

    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, TRUE);

    todo_wine check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IBasicVideo2, FALSE);
    todo_wine check_interface(filter, &IID_IVideoWindow, FALSE);
    check_interface(filter, &IID_IVMRMonitorConfig9, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    filter = create_vmr9(VMR9Mode_Windowed);
    set_mixing_mode(filter, 1);
    test_common_interfaces(filter);

    check_interface(filter, &IID_IBasicVideo, TRUE);
    todo_wine check_interface(filter, &IID_IBasicVideo2, TRUE);
    check_interface(filter, &IID_IVideoWindow, TRUE);
    check_interface(filter, &IID_IVMRMixerControl9, TRUE);
    /* IVMRMonitorConfig9 may not be available if the d3d9 device has
     * insufficient support. */
    check_interface_broken(filter, &IID_IVMRMonitorConfig9, TRUE);

    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

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
    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, &test_outer, CLSCTX_INPROC_SERVER,
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
    IBaseFilter *filter = create_vmr9(0);
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

    set_mixing_mode(filter, 2);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK) IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    if (count > 1) IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 3, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    if (count > 1) IPin_Release(pins[1]);

    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_find_pin(void)
{
    IBaseFilter *filter = create_vmr9(0);
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    IBaseFilter_EnumPins(filter, &enum_pins);

    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins did not match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IBaseFilter_FindPin(filter, L"VMR Input1", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    set_mixing_mode(filter, 2);

    IEnumPins_Reset(enum_pins);

    hr = IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == pin2, "Pins did not match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IBaseFilter_FindPin(filter, L"VMR Input1", &pin);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(pin == pin2, "Pins did not match.\n");
        IPin_Release(pin);
        IPin_Release(pin2);
    }

    hr = IBaseFilter_FindPin(filter, L"VMR Input2", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_vmr9(0);
    PIN_DIRECTION dir;
    ULONG count, ref;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    hr = IPin_QueryPinInfo(pin, &info);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"VMR Input0"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"VMR Input0"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    set_mixing_mode(filter, 2);

    hr = IBaseFilter_FindPin(filter, L"VMR Input1", &pin);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IPin_QueryPinInfo(pin, &info);
        ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
        ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
        ok(!wcscmp(info.achName, L"VMR Input1"), "Got name %s.\n", wine_dbgstr_w(info.achName));
        IBaseFilter_Release(info.pFilter);

        hr = IPin_QueryDirection(pin, &dir);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

        hr = IPin_QueryId(pin, &id);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!wcscmp(id, L"VMR Input1"), "Got id %s.\n", wine_dbgstr_w(id));
        CoTaskMemFree(id);

        hr = IPin_QueryInternalConnections(pin, NULL, &count);
        ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

        IPin_Release(pin);
    }

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_media_types(void)
{
    IBaseFilter *filter = create_vmr9(0);
    AM_MEDIA_TYPE *mt, req_mt = {{0}};
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, 0xdeadbeef}
    };
    IEnumMediaTypes *enummt;
    unsigned int i;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

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

    req_mt.subtype = MEDIASUBTYPE_RGB8;
    hr = IPin_QueryAccept(pin, &req_mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
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
    IBaseFilter *filter = create_vmr9(0);
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

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
    IBaseFilter *filter = create_vmr9(0);
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
    ALLOCATOR_PROPERTIES props, req_props;
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

        req_props.cBuffers = 1;
        req_props.cbBuffer = 32 * 16 * 4;
        req_props.cbAlign = 1;
        req_props.cbPrefix = 0;
        hr = IMemAllocator_SetProperties(ret_allocator, &req_props, &props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
        ok(props.cbBuffer == 32 * 16 * 4, "Got size %ld.\n", props.cbBuffer);
        ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
        ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

        IMemAllocator_Release(ret_allocator);
    }

    hr = IMemInputPin_NotifyAllocator(input, NULL, TRUE);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&req_allocator);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    IMemAllocator_Release(req_allocator);
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

static HANDLE send_frame_time(IMemInputPin *sink, REFERENCE_TIME start_time, DWORD color)
{
    struct frame_thread_params *params = malloc(sizeof(*params));
    IMemAllocator *allocator;
    REFERENCE_TIME end_time;
    IMediaSample *sample;
    HANDLE thread;
    LONG size, i;
    HRESULT hr;
    BYTE *data;

    hr = IMemInputPin_GetAllocator(sink, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    size = IMediaSample_GetSize(sample);
    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; i < size / sizeof(DWORD); ++i)
        ((DWORD *)data)[i] = color;

    hr = IMediaSample_SetActualDataLength(sample, size);
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
    return send_frame_time(sink, 0, 0x007f007f);
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

static void commit_allocator(IMemInputPin *input)
{
    IMemAllocator *allocator;
    HRESULT hr;

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMemAllocator_Release(allocator);
}

static void test_filter_state(IMemInputPin *input, IMediaControl *control)
{
    IMemAllocator *allocator;
    IMediaSample *sample;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

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

    /* The sink will decommit our allocator for us when stopping, however it
     * will not recommit it when pausing. */
    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    todo_wine ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);
    if (hr == S_OK) IMediaSample_Release(sample);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

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

    commit_allocator(input);
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

    commit_allocator(input);
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
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* We dropped the sample we were holding, so now we need a new one... */

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %#lx.\n", state);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %#lx.\n", state);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
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

    commit_allocator(input);
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

    commit_allocator(input);
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

    commit_allocator(input);
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

    commit_allocator(input);
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

static void test_sample_time(IPin *pin, IMemInputPin *input, IMediaControl *control)
{
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    commit_allocator(input);
    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1, 0x000000ff); /* blue */

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

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
     * cyan and green frames. Thus the VMR does not attempt to drop any frames
     * that it considers late. This remains true if the frames are marked as
     * discontinuous. */

    hr = join_thread(send_frame_time(input, 1, 0x0000ffff)); /* cyan */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame_time(input, 0, 0x0000ff00)); /* green */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = join_thread(send_frame_time(input, -2, 0x00ff0000)); /* red */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1000000, 0x00ffffff); /* white */
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    /* If the frame makes it to Receive() in time to be rendered, we get S_OK. */
    ok(hr == S_OK || hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    thread = send_frame_time(input, 1000000, 0x00ffff00); /* yellow */
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    /* If the frame makes it to Receive() in time to be rendered, we get S_OK. */
    ok(hr == S_OK || hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);
}

static void test_current_image(IBaseFilter *filter, IMemInputPin *input,
        IMediaControl *control, const BITMAPINFOHEADER *req_bih)
{
    LONG buffer[(sizeof(BITMAPINFOHEADER) + 32 * 16 * 4) / 4];
    const BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)buffer;
    const DWORD *data = (DWORD *)((char *)buffer + sizeof(BITMAPINFOHEADER));
    BITMAPINFOHEADER expect_bih = *req_bih;
    OAFilterState state;
    IBasicVideo *video;
    unsigned int i;
    HANDLE thread;
    HRESULT hr;
    LONG size;

    expect_bih.biSizeImage = 32 * 16 * 4;

    IBaseFilter_QueryInterface(filter, &IID_IBasicVideo, (void **)&video);

    hr = IBasicVideo_GetCurrentImage(video, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_GetCurrentImage(video, NULL, buffer);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    size = 0xdeadbeef;
    hr = IBasicVideo_GetCurrentImage(video, &size, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(size == sizeof(BITMAPINFOHEADER) + 32 * 16 * 4, "Got size %ld.\n", size);

    size = sizeof(buffer);
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == sizeof(buffer), "Got size %ld.\n", size);
    ok(!memcmp(bih, &expect_bih, sizeof(BITMAPINFOHEADER)), "Bitmap headers didn't match.\n");
    /* The contents seem to reflect the last frame rendered. */

    commit_allocator(input);
    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    size = sizeof(buffer);
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == sizeof(buffer), "Got size %ld.\n", size);
    ok(!memcmp(bih, &expect_bih, sizeof(BITMAPINFOHEADER)), "Bitmap headers didn't match.\n");
    /* The contents seem to reflect the last frame rendered. */

    thread = send_frame(input);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    size = 1;
    memset(buffer, 0xcc, sizeof(buffer));
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == 1, "Got size %ld.\n", size);

    size = sizeof(buffer);
    memset(buffer, 0xcc, sizeof(buffer));
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == sizeof(buffer), "Got size %ld.\n", size);
    ok(!memcmp(bih, &expect_bih, sizeof(BITMAPINFOHEADER)), "Bitmap headers didn't match.\n");
    for (i = 0; i < 32 * 16; ++i)
        ok((data[i] & 0xffffff) == 0x7f007f, "Got unexpected color %08lx at %u.\n", data[i], i);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    join_thread(thread);

    size = sizeof(buffer);
    memset(buffer, 0xcc, sizeof(buffer));
    hr = IBasicVideo_GetCurrentImage(video, &size, buffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == sizeof(buffer), "Got size %ld.\n", size);
    ok(!memcmp(bih, &expect_bih, sizeof(BITMAPINFOHEADER)), "Bitmap headers didn't match.\n");
    for (i = 0; i < 32 * 16; ++i)
        ok((data[i] & 0xffffff) == 0x7f007f, "Got unexpected color %08lx at %u.\n", data[i], i);

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

    commit_allocator(input);
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
    ok(!IsWindowVisible(hwnd), "Window should be invisible.\n");

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

    commit_allocator(input);
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
        .bmiHeader.biBitCount = 32,
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
    ALLOCATOR_PROPERTIES req_props = {1, 32 * 16 * 4, 1, 0}, ret_props;
    IBaseFilter *filter = create_vmr9(VMR9Mode_Windowed);
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    IMemAllocator *allocator;
    IMediaControl *control;
    IMemInputPin *input;
    unsigned int i, j;
    AM_MEDIA_TYPE mt;
    IPin *pin, *peer;
    HRESULT hr;
    ULONG ref;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB555,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };
    static const WORD bpp_tests[] = {15, 16, 24, 32};

    testfilter_init(&source);

    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];

        for (j = 0; j < ARRAY_SIZE(bpp_tests); ++j)
        {
            vih.bmiHeader.biBitCount = bpp_tests[j];

            hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
            if (hr == E_FAIL)
            {
                skip("Got E_FAIL when connecting.\n");
                goto out;
            }
            ok(hr == S_OK, "Got hr %#lx for subtype %s and bpp %u.\n", hr,
                    wine_dbgstr_guid(subtype_tests[i]), bpp_tests[j]);

            hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            hr = IFilterGraph2_Disconnect(graph, pin);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }
    }

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.formattype = FORMAT_VideoInfo;

    req_mt.subtype = MEDIASUBTYPE_RGB8;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_WAVE;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_RGB32;

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

    /* Disconnecting while not stopped is broken: it returns S_OK, but
     * subsequent attempts to connect return VFW_E_ALREADY_CONNECTED. */

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

    test_filter_state(input, control);
    test_flushing(pin, input, control);
    test_eos(pin, input, control);
    test_sample_time(pin, input, control);
    test_current_image(filter, input, control, &vih.bmiHeader);
    test_window_close(pin, input, control);

    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(source.source.pin.peer == pin, "Got peer %p.\n", source.source.pin.peer);
    IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

out:
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IMemInputPin_Release(input);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_overlay(void)
{
    IBaseFilter *filter = create_vmr9(0);
    IOverlay *overlay;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    HWND hwnd;

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

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

    filter = create_vmr9(VMR9Mode_Windowless);
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hwnd = (HWND)0xdeadbeef;
    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);
    ok(hwnd == (HWND)0xdeadbeef, "Got window %p.\n", hwnd);

    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    filter = create_vmr9(VMR9Mode_Renderless);
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hwnd = (HWND)0xdeadbeef;
    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);
    ok(hwnd == (HWND)0xdeadbeef, "Got window %p.\n", hwnd);

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
    RECT rect = {0, 0, 600, 400};
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
    ALLOCATOR_PROPERTIES req_props = {1, 600 * 400 * 4, 1, 0}, ret_props;
    VIDEOINFOHEADER vih =
    {
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
    IFilterGraph2 *graph = create_graph();
    WNDCLASSA window_class = {0};
    struct testfilter source;
    IMemAllocator *allocator;
    MONITORINFO monitorinfo;
    IMediaControl *control;
    LONG width, height, l;
    IVideoWindow *window;
    IMemInputPin *input;
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

    filter = create_vmr9(VMR9Mode_Windowed);
    flush_events();

    flaky_wine
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_debug > 1) trace("ours %p, theirs %p\n", our_hwnd, hwnd);
    GetWindowRect(hwnd, &rect);

    tid = GetWindowThreadProcessId(hwnd, NULL);
    ok(tid == GetCurrentThreadId(), "Expected tid %#lx, got %#lx.\n", GetCurrentThreadId(), tid);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVideoWindow, (void **)&window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

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
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    if (hr == E_FAIL)
    {
        skip("Got E_FAIL when connecting.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
        hr = IMemAllocator_Commit(allocator);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMemAllocator_Release(allocator);
    }

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
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_get_FullScreenMode(window, &l);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_GetMinIdealImageSize(window, &width, &height);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_GetMaxIdealImageSize(window, &width, &height);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    monitorinfo.cbSize = sizeof(monitorinfo);
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitorinfo);

    hr = IVideoWindow_GetMinIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(width == 1, "Got width %ld.\n", width);
    todo_wine ok(height == 1, "Got height %ld.\n", height);
    hr = IVideoWindow_GetMaxIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(width == monitorinfo.rcMonitor.right + 1, "Expected width %ld, got %ld.\n",
            monitorinfo.rcMonitor.right + 1, width);
    todo_wine ok(height == monitorinfo.rcMonitor.bottom + 1, "Expected height %ld, got %ld.\n",
            monitorinfo.rcMonitor.bottom + 1, height);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

out:
    IMediaControl_Release(control);
    IFilterGraph2_Release(graph);
    IVideoWindow_Release(window);
    IOverlay_Release(overlay);
    IMemInputPin_Release(input);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(our_hwnd);
}

static IDirect3DDevice9 *create_device(HWND window)
{
    D3DPRESENT_PARAMETERS present_parameters =
    {
        .Windowed = TRUE,
        .hDeviceWindow = window,
        .SwapEffect = D3DSWAPEFFECT_DISCARD,
        .BackBufferWidth = 640,
        .BackBufferHeight = 480,
        .BackBufferFormat = D3DFMT_A8R8G8B8,
    };
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    IDirect3D9_Release(d3d);
    if (FAILED(hr))
    {
        skip("Failed to create a 3D device, hr %#lx.\n", hr);
        return NULL;
    }
    return device;
}

static void test_allocate_surface_helper(void)
{
    VMR9AllocationInfo info =
    {
        .dwFlags = VMR9AllocFlag_OffscreenSurface,
        .dwWidth = 32,
        .dwHeight = 16,
        .Format = D3DFMT_X8R8G8B8,
        .Pool = D3DPOOL_DEFAULT,
        .MinBuffers = 2,
        .szAspectRatio = {32, 16},
        .szNativeSize = {32, 16},
    };
    IBaseFilter *filter = create_vmr9(VMR9Mode_Renderless);
    IVMRSurfaceAllocatorNotify9 *notify;
    IDirect3DSurface9 *surfaces[2] = {};
    IDirect3DDevice9 *device, *device2;
    RECT rect = {0, 0, 640, 480};
    IDirect3DTexture9 *container;
    D3DSURFACE_DESC desc;
    DWORD count;
    HWND window;
    HRESULT hr;
    ULONG ref;

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    if (!(device = create_device(window)))
    {
        IBaseFilter_Release(filter);
        DestroyWindow(window);
        return;
    }

    IBaseFilter_QueryInterface(filter, &IID_IVMRSurfaceAllocatorNotify9, (void **)&notify);

    count = 2;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IVMRSurfaceAllocatorNotify9_SetD3DDevice(notify, device, MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY));
    if (hr == E_NOINTERFACE)
    {
        win_skip("Direct3D does not support video rendering.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (0) /* crashes on Windows */
    {
        hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, NULL, &count, surfaces);
        ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

        hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, NULL, surfaces);
        ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    }

    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    ok(!!surfaces[0], "Surface 0 was not allocated.\n");
    ok(!!surfaces[1], "Surface 1 was not allocated.\n");

    hr = IDirect3DSurface9_GetDevice(surfaces[0], &device2);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(device2 == device, "Devices did not match.\n");
    IDirect3DDevice9_Release(device2);

    hr = IDirect3DSurface9_GetContainer(surfaces[0], &IID_IDirect3DTexture9, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetDesc(surfaces[0], &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(desc.Format == info.Format, "Got format %#x.\n", desc.Format);
    ok(desc.Type == D3DRTYPE_SURFACE, "Got type %u.\n", desc.Type);
    ok(!desc.Usage, "Got usage %#lx.\n", desc.Usage);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Got pool %u.\n", desc.Pool);
    ok(desc.MultiSampleType == D3DMULTISAMPLE_NONE, "Got multisample type %u.\n", desc.MultiSampleType);
    ok(!desc.MultiSampleQuality, "Got multisample quality %lu.\n", desc.MultiSampleQuality);
    ok(desc.Width == 32, "Got width %u.\n", desc.Width);
    ok(desc.Height == 16, "Got height %u.\n", desc.Height);

    IDirect3DSurface9_Release(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[1]);

    surfaces[0] = surfaces[1] = NULL;
    count = 1;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!count, "Got count %lu.\n", count);
    ok(!surfaces[0], "Surface 0 was allocated.\n");
    ok(!surfaces[1], "Surface 1 was allocated.\n");

    count = 2;
    info.MinBuffers = 1;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    ok(!!surfaces[0], "Surface 0 was not allocated.\n");
    ok(!!surfaces[1], "Surface 1 was not allocated.\n");
    IDirect3DSurface9_Release(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[1]);

    count = 2;
    info.dwFlags = VMR9AllocFlag_TextureSurface;
    surfaces[0] = surfaces[1] = NULL;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    ok(!!surfaces[0], "Surface 0 was not allocated.\n");
    ok(!!surfaces[1], "Surface 1 was not allocated.\n");

    hr = IDirect3DSurface9_GetDevice(surfaces[0], &device2);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(device2 == device, "Devices did not match.\n");
    IDirect3DDevice9_Release(device2);

    hr = IDirect3DSurface9_GetContainer(surfaces[0], &IID_IDirect3DTexture9, (void **)&container);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3DTexture9_Release(container);

    hr = IDirect3DSurface9_GetDesc(surfaces[1], &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(desc.Format == info.Format, "Got format %#x.\n", desc.Format);
    ok(desc.Type == D3DRTYPE_SURFACE, "Got type %u.\n", desc.Type);
    ok(desc.Usage == D3DUSAGE_DYNAMIC, "Got usage %#lx.\n", desc.Usage);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Got pool %u.\n", desc.Pool);
    ok(desc.MultiSampleType == D3DMULTISAMPLE_NONE, "Got multisample type %u.\n", desc.MultiSampleType);
    ok(!desc.MultiSampleQuality, "Got multisample quality %lu.\n", desc.MultiSampleQuality);
    ok(desc.Width == 32, "Got width %u.\n", desc.Width);
    ok(desc.Height == 16, "Got height %u.\n", desc.Height);

    IDirect3DSurface9_Release(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[1]);

    info.Format = D3DFMT_R8G8B8;
    surfaces[0] = surfaces[1] = NULL;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);
    ok(!surfaces[0], "Surface 0 was allocated.\n");
    ok(!surfaces[1], "Surface 1 was allocated.\n");

    info.Format = 0;
    info.dwFlags = VMR9AllocFlag_3DRenderTarget;
    count = 1;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    ok(!!surfaces[0], "Surface 0 was not allocated.\n");
    ok(info.Format != 0, "Expected a format.\n");

    hr = IDirect3DSurface9_GetDesc(surfaces[0], &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(desc.Format == info.Format, "Expected format %#x, got %#x.\n", info.Format, desc.Format);
    ok(desc.Type == D3DRTYPE_SURFACE, "Got type %u.\n", desc.Type);
    ok(desc.Usage == D3DUSAGE_RENDERTARGET, "Got usage %#lx.\n", desc.Usage);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Got pool %u.\n", desc.Pool);
    ok(desc.MultiSampleType == D3DMULTISAMPLE_NONE, "Got multisample type %u.\n", desc.MultiSampleType);
    ok(!desc.MultiSampleQuality, "Got multisample quality %lu.\n", desc.MultiSampleQuality);
    ok(desc.Width == 32, "Got width %u.\n", desc.Width);
    ok(desc.Height == 16, "Got height %u.\n", desc.Height);

    IDirect3DSurface9_Release(surfaces[0]);

    info.Format = D3DFMT_A8R8G8B8;
    info.dwFlags = VMR9AllocFlag_OffscreenSurface | VMR9AllocFlag_TextureSurface;
    count = 1;
    hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(notify, &info, &count, surfaces);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);

out:
    IVMRSurfaceAllocatorNotify9_Release(notify);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

struct presenter
{
    IVMRSurfaceAllocator9 IVMRSurfaceAllocator9_iface;
    IVMRImagePresenter9 IVMRImagePresenter9_iface;
    LONG refcount;

    D3DFORMAT format;
    DWORD accept_flags;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *surfaces[5];
    IVMRSurfaceAllocatorNotify9 *notify;
    unsigned int got_PresentImage, got_TerminateDevice;
};

static struct presenter *impl_from_IVMRImagePresenter9(IVMRImagePresenter9 *iface)
{
    return CONTAINING_RECORD(iface, struct presenter, IVMRImagePresenter9_iface);
}

static HRESULT WINAPI presenter_QueryInterface(IVMRImagePresenter9 *iface, REFIID iid, void **out)
{
    struct presenter *presenter = impl_from_IVMRImagePresenter9(iface);
    return IVMRSurfaceAllocator9_QueryInterface(&presenter->IVMRSurfaceAllocator9_iface, iid, out);
}

static ULONG WINAPI presenter_AddRef(IVMRImagePresenter9 *iface)
{
    struct presenter *presenter = impl_from_IVMRImagePresenter9(iface);
    return IVMRSurfaceAllocator9_AddRef(&presenter->IVMRSurfaceAllocator9_iface);
}

static ULONG WINAPI presenter_Release(IVMRImagePresenter9 *iface)
{
    struct presenter *presenter = impl_from_IVMRImagePresenter9(iface);
    return IVMRSurfaceAllocator9_Release(&presenter->IVMRSurfaceAllocator9_iface);
}

static HRESULT WINAPI presenter_StartPresenting(IVMRImagePresenter9 *iface, DWORD_PTR cookie)
{
    if (winetest_debug > 1) trace("StartPresenting()\n");
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI presenter_StopPresenting(IVMRImagePresenter9 *iface, DWORD_PTR cookie)
{
    if (winetest_debug > 1) trace("StopPresenting()\n");
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI presenter_PresentImage(IVMRImagePresenter9 *iface, DWORD_PTR cookie, VMR9PresentationInfo *info)
{
    struct presenter *presenter = impl_from_IVMRImagePresenter9(iface);
    IDirect3DDevice9 *device;
    static const RECT rect;

    if (winetest_debug > 1) trace("PresentImage()\n");
    IDirect3DSurface9_GetDevice(info->lpSurf, &device);
    ok(device == presenter->device, "got %p, expected %p\n", device, presenter->device);
    IDirect3DDevice9_Release(device);
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    todo_wine ok(info->dwFlags == VMR9Sample_TimeValid, "Got flags %#lx.\n", info->dwFlags);
    ok(!info->rtStart, "Got start time %s.\n", wine_dbgstr_longlong(info->rtStart));
    ok(info->rtEnd == 10000000, "Got end time %s.\n", wine_dbgstr_longlong(info->rtEnd));
    todo_wine ok(info->szAspectRatio.cx == 120, "Got aspect ratio width %ld.\n", info->szAspectRatio.cx);
    todo_wine ok(info->szAspectRatio.cy == 60, "Got aspect ratio height %ld.\n", info->szAspectRatio.cy);
    ok(EqualRect(&info->rcSrc, &rect), "Got source rect %s.\n", wine_dbgstr_rect(&info->rcSrc));
    ok(EqualRect(&info->rcDst, &rect), "Got dest rect %s.\n", wine_dbgstr_rect(&info->rcDst));
    ok(!info->dwReserved1, "Got dwReserved1 %#lx.\n", info->dwReserved1);
    ok(!info->dwReserved2, "Got dwReserved2 %#lx.\n", info->dwReserved2);

    ++presenter->got_PresentImage;
    return S_OK;
}

static const IVMRImagePresenter9Vtbl presenter_vtbl =
{
    presenter_QueryInterface,
    presenter_AddRef,
    presenter_Release,
    presenter_StartPresenting,
    presenter_StopPresenting,
    presenter_PresentImage,
};

static struct presenter *impl_from_IVMRSurfaceAllocator9(IVMRSurfaceAllocator9 *iface)
{
    return CONTAINING_RECORD(iface, struct presenter, IVMRSurfaceAllocator9_iface);
}

static HRESULT WINAPI allocator_QueryInterface(IVMRSurfaceAllocator9 *iface, REFIID iid, void **out)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);

    if (winetest_debug > 1) trace("QueryInterface(%s)\n", wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IVMRImagePresenter9))
    {
        *out = &presenter->IVMRImagePresenter9_iface;
        IVMRImagePresenter9_AddRef(&presenter->IVMRImagePresenter9_iface);
        return S_OK;
    }
    ok(!IsEqualGUID(iid, &IID_IVMRSurfaceAllocatorEx9), "Unexpected query for IVMRSurfaceAllocatorEx9.\n");
    *out = NULL;
    return E_NOTIMPL;
}

static ULONG WINAPI allocator_AddRef(IVMRSurfaceAllocator9 *iface)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);
    return InterlockedIncrement(&presenter->refcount);
}

static ULONG WINAPI allocator_Release(IVMRSurfaceAllocator9 *iface)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);
    return InterlockedDecrement(&presenter->refcount);
}

static HRESULT WINAPI allocator_InitializeDevice(IVMRSurfaceAllocator9 *iface,
        DWORD_PTR cookie, VMR9AllocationInfo *info, DWORD *buffer_count)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);

    if (winetest_debug > 1) trace("InitializeDevice(flags %#lx, format %u)\n",
            info->dwFlags, info->Format);
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    ok(info->dwWidth == 32, "Got width %lu.\n", info->dwWidth);
    ok(info->dwHeight == 16, "Got height %lu.\n", info->dwHeight);
    todo_wine ok(info->MinBuffers == 5, "Got buffer count %lu.\n", info->MinBuffers);
    ok(info->Pool == D3DPOOL_DEFAULT, "Got pool %u\n", info->Pool);
    todo_wine ok(info->szAspectRatio.cx == 120, "Got aspect ratio width %ld.\n", info->szAspectRatio.cx);
    todo_wine ok(info->szAspectRatio.cy == 60, "Got aspect ratio height %ld.\n", info->szAspectRatio.cy);
    ok(info->szNativeSize.cx == 32, "Got native width %ld.\n", info->szNativeSize.cx);
    ok(info->szNativeSize.cy == 16, "Got native height %ld.\n", info->szNativeSize.cy);
    todo_wine ok(*buffer_count == 5, "Got buffer count %lu.\n", *buffer_count);

    presenter->format = info->Format;

    if (info->dwFlags != presenter->accept_flags)
        return 0xdeadbeef;
    return IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(presenter->notify,
            info, buffer_count, presenter->surfaces);
}

static HRESULT WINAPI allocator_TerminateDevice(IVMRSurfaceAllocator9 *iface, DWORD_PTR cookie)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);

    if (winetest_debug > 1) trace("TerminateDevice()\n");
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    /* Don't dereference the surfaces here, to mimic How to Survive. */
    ++presenter->got_TerminateDevice;
    return E_NOTIMPL;
}

static HRESULT WINAPI allocator_GetSurface(IVMRSurfaceAllocator9 *iface,
        DWORD_PTR cookie, DWORD index, DWORD flags, IDirect3DSurface9 **surface)
{
    struct presenter *presenter = impl_from_IVMRSurfaceAllocator9(iface);

    if (winetest_debug > 1) trace("GetSurface(index %lu)\n", index);
    ok(cookie == 0xabacab, "Got cookie %#Ix.\n", cookie);
    ok(!flags, "Got flags %#lx.\n", flags);
    ok(index < 5, "Got index %lu.\n", index);

    /* Don't reference the surface here, to mimic How to Survive. */
    *surface = presenter->surfaces[index];
    return S_OK;
}

static HRESULT WINAPI allocator_AdviseNotify(IVMRSurfaceAllocator9 *iface, IVMRSurfaceAllocatorNotify9 *notify)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IVMRSurfaceAllocator9Vtbl allocator_vtbl =
{
    allocator_QueryInterface,
    allocator_AddRef,
    allocator_Release,
    allocator_InitializeDevice,
    allocator_TerminateDevice,
    allocator_GetSurface,
    allocator_AdviseNotify,
};

static void test_renderless_present(struct presenter *presenter,
        IFilterGraph2 *graph, IMemInputPin *input)
{
    IMediaControl *control;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    presenter->got_PresentImage = 0;

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    thread = send_frame(input);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    /* Atelier Sophie uses the VMR in renderless mode, calls
     * IMediaControl::Run() from a stopped state and expects that
     * IMediaControl::GetState() returns S_OK only after PresentImage() has
     * been called. */
    ok(presenter->got_PresentImage == 1, "Got %u calls to PresentImage().\n", presenter->got_PresentImage);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(presenter->got_PresentImage == 1, "Got %u calls to PresentImage().\n", presenter->got_PresentImage);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(control);
}

static void test_renderless_formats(void)
{
    VIDEOINFOHEADER vih =
    {
        .rcSource = {4, 6, 16, 12},
        .rcTarget = {40, 60, 160, 120},
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    ALLOCATOR_PROPERTIES req_props = {5, 32 * 16 * 4, 1, 0}, ret_props;
    struct presenter presenter =
    {
        .IVMRSurfaceAllocator9_iface.lpVtbl = &allocator_vtbl,
        .IVMRImagePresenter9_iface.lpVtbl = &presenter_vtbl,
        .refcount = 1,
    };
    struct presenter presenter2 = presenter;
    IVMRSurfaceAllocatorNotify9 *notify;
    RECT rect = {0, 0, 640, 480};
    struct testfilter source;
    IDirect3DDevice9 *device;
    IMemAllocator *allocator;
    IFilterGraph2 *graph;
    IMemInputPin *input;
    IBaseFilter *filter;
    unsigned int i;
    HWND window;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const struct
    {
        const GUID *subtype;
        D3DFORMAT format;
        DWORD flags;
    }
    tests[] =
    {
        {&MEDIASUBTYPE_ARGB1555, D3DFMT_A1R5G5B5, VMR9AllocFlag_TextureSurface},
        {&MEDIASUBTYPE_ARGB32, D3DFMT_A8R8G8B8, VMR9AllocFlag_TextureSurface},
        {&MEDIASUBTYPE_ARGB4444, D3DFMT_A4R4G4B4, VMR9AllocFlag_TextureSurface},

        {&MEDIASUBTYPE_RGB555, D3DFMT_X1R5G5B5, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_RGB555, D3DFMT_X1R5G5B5, VMR9AllocFlag_TextureSurface},
        {&MEDIASUBTYPE_RGB565, D3DFMT_R5G6B5, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_RGB565, D3DFMT_R5G6B5, VMR9AllocFlag_TextureSurface},
        {&MEDIASUBTYPE_RGB24, D3DFMT_R8G8B8, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_RGB24, D3DFMT_R8G8B8, VMR9AllocFlag_TextureSurface},
        {&MEDIASUBTYPE_RGB32, D3DFMT_X8R8G8B8, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_RGB32, D3DFMT_X8R8G8B8, VMR9AllocFlag_TextureSurface},

        {&MEDIASUBTYPE_NV12, MAKEFOURCC('N','V','1','2'), VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_UYVY, D3DFMT_UYVY, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_YUY2, D3DFMT_YUY2, VMR9AllocFlag_OffscreenSurface},
        {&MEDIASUBTYPE_YV12, MAKEFOURCC('Y','V','1','2'), VMR9AllocFlag_OffscreenSurface},
    };

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    if (!(device = create_device(window)))
    {
        DestroyWindow(window);
        return;
    }

    filter = create_vmr9(VMR9Mode_Renderless);
    IBaseFilter_QueryInterface(filter, &IID_IVMRSurfaceAllocatorNotify9, (void **)&notify);

    hr = IVMRSurfaceAllocatorNotify9_SetD3DDevice(notify, device, MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY));
    presenter.device = device;
    if (hr == E_NOINTERFACE)
    {
        win_skip("Direct3D does not support video rendering.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRSurfaceAllocatorNotify9_AdviseSurfaceAllocator(notify, 0xabacab,
            &presenter.IVMRSurfaceAllocator9_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    presenter.notify = notify;

    testfilter_init(&source);
    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        req_mt.subtype = *tests[i].subtype;
        presenter.accept_flags = tests[i].flags;

        hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
        /* Connection never fails on Native, but Wine currently creates D3D
         * surfaces during IPin::ReceiveConnection() instead of
         * IMemAllocator::SetProperties(), so let that fail here for now. */
        if (hr != S_OK)
        {
            skip("Format %u (%#x), flags %#lx are not supported, hr %#lx.\n",
                    tests[i].format, tests[i].format, tests[i].flags, hr);
            continue;
        }
        ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);

        hr = IMemInputPin_GetAllocator(input, &allocator);
        todo_wine ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
        if (hr != S_OK)
        {
            test_allocator(input);
            hr = IMemInputPin_GetAllocator(input, &allocator);
        }

        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        if (hr != S_OK)
        {
            skip("Format %u (%#x), flags %#lx are not supported, hr %#lx.\n",
                    tests[i].format, tests[i].format, tests[i].flags, hr);
            IMemAllocator_Release(allocator);
            hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
            ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
            hr = IFilterGraph2_Disconnect(graph, pin);
            ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
            continue;
        }
        ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
        hr = IMemAllocator_Commit(allocator);
        ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);

        hr = IVMRSurfaceAllocatorNotify9_AdviseSurfaceAllocator(notify, 0xabacab,
                &presenter2.IVMRSurfaceAllocator9_iface);
        ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

        ok(presenter.format == tests[i].format, "Test %u: Got format %u (%#x).\n",
                i, presenter.format, presenter.format);

        test_renderless_present(&presenter, graph, input);

        hr = IMemAllocator_Decommit(allocator);
        ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
        IMemAllocator_Release(allocator);

        hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
        hr = IFilterGraph2_Disconnect(graph, pin);
        ok(hr == S_OK, "Test %u: Got hr %#lx.\n", i, hr);
    }

    hr = IVMRSurfaceAllocatorNotify9_AdviseSurfaceAllocator(notify, 0xabacab, &presenter2.IVMRSurfaceAllocator9_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IMemInputPin_Release(input);
    IPin_Release(pin);

out:
    IVMRSurfaceAllocatorNotify9_Release(notify);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(presenter.refcount == 1, "Got outstanding refcount %ld.\n", presenter.refcount);
    ok(presenter2.refcount == 1, "Got outstanding refcount %ld.\n", presenter2.refcount);
    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

static void test_mixing_mode(void)
{
    IVMRWindowlessControl9 *windowless_control;
    IVMRMixerControl9 *mixer_control;
    IVMRFilterConfig9 *config;
    DWORD stream_count = 0;
    IBaseFilter *filter;
    unsigned int i;
    HWND window;
    HRESULT hr;
    ULONG ref;

    static const VMR9Mode modes[] =
    {
        0,
        VMR9Mode_Windowed,
        VMR9Mode_Windowless,
        VMR9Mode_Renderless,
    };

    for (i = 0; i < ARRAY_SIZE(modes); ++i)
    {
        filter = create_vmr9(modes[i]);
        IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig9, (void **)&config);

        hr = IBaseFilter_QueryInterface(filter, &IID_IVMRMixerControl9, (void **)&mixer_control);
        ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

        hr = IVMRFilterConfig9_GetNumberOfStreams(config, &stream_count);
        ok(hr == VFW_E_VMR_NOT_IN_MIXER_MODE, "Got hr %#lx.\n", hr);

        hr = IVMRFilterConfig9_SetNumberOfStreams(config, 1);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IVMRFilterConfig9_GetNumberOfStreams(config, &stream_count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_count == 1, "Got %lu streams.\n", stream_count);

        hr = IBaseFilter_QueryInterface(filter, &IID_IVMRMixerControl9, (void **)&mixer_control);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IVMRMixerControl9_Release(mixer_control);

        hr = IVMRFilterConfig9_SetNumberOfStreams(config, 2);
        ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

        hr = IVMRFilterConfig9_GetNumberOfStreams(config, &stream_count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(stream_count == 1, "Got %lu streams.\n", stream_count);

        IVMRFilterConfig9_Release(config);
        ref = IBaseFilter_Release(filter);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
    }

    filter = create_vmr9(VMR9Mode_Windowless);
    IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig9, (void **)&config);
    IBaseFilter_QueryInterface(filter, &IID_IVMRWindowlessControl9, (void **)&windowless_control);

    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create a window.\n");
    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &stream_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream_count == 4, "Got %lu streams.\n", stream_count);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVMRMixerControl9, (void **)&mixer_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IVMRMixerControl9_Release(mixer_control);

    hr = IVMRFilterConfig9_SetNumberOfStreams(config, 2);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    hr = IVMRFilterConfig9_GetNumberOfStreams(config, &stream_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stream_count == 4, "Got %lu streams.\n", stream_count);

    IVMRWindowlessControl9_Release(windowless_control);
    IVMRFilterConfig9_Release(config);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

static void test_clipping_window(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
        .bmiHeader.biBitCount = 32,
    };
    AM_MEDIA_TYPE mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB32,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IBaseFilter *filter = create_vmr9(VMR9Mode_Windowless);
    IVMRWindowlessControl9 *windowless_control;
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    HWND window;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_QueryInterface(filter, &IID_IVMRWindowlessControl9, (void **)&windowless_control);
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    testfilter_init(&source);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"vmr9");
    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create a window.\n");

    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, (HWND)0xdeadbeef);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, window);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IPin_Release(pin);
    IVMRWindowlessControl9_Release(windowless_control);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

static void test_surface_allocator_notify_refcount(void)
{
    struct presenter presenter =
    {
        .IVMRSurfaceAllocator9_iface.lpVtbl = &allocator_vtbl,
        .IVMRImagePresenter9_iface.lpVtbl = &presenter_vtbl,
        .refcount = 1,
    };
    IBaseFilter *filter = create_vmr9(VMR9Mode_Renderless);
    IVMRSurfaceAllocatorNotify9 *notify;
    HRESULT hr;
    ULONG ref;

    set_mixing_mode(filter, 2);

    IBaseFilter_QueryInterface(filter, &IID_IVMRSurfaceAllocatorNotify9, (void **)&notify);

    hr = IVMRSurfaceAllocatorNotify9_AdviseSurfaceAllocator(notify, 0xabacab,
            &presenter.IVMRSurfaceAllocator9_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(presenter.got_TerminateDevice == 1, "Got %u calls to TerminateDevice().\n",
            presenter.got_TerminateDevice);
    ok(presenter.refcount == 1, "Got outstanding refcount %ld.\n", presenter.refcount);

    ref = IVMRSurfaceAllocatorNotify9_Release(notify);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
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
    ALLOCATOR_PROPERTIES req_props = {1, 600 * 400 * 4, 1, 0}, ret_props;
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
    IBaseFilter *filter = create_vmr9(VMR9Mode_Windowed);
    IFilterGraph2 *graph = create_graph();
    LONG left, top, width, height, l;
    struct testfilter source;
    IMemAllocator *allocator;
    IMemInputPin *input;
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
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

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
    if (hr == E_FAIL)
    {
        skip("Got E_FAIL when connecting.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
        hr = IMemAllocator_Commit(allocator);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMemAllocator_Release(allocator);
    }

    reftime = 0.0;
    hr = IBasicVideo_get_AvgTimePerFrame(video, &reftime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_double(reftime, 0.02, 1 << 28), "Got frame rate %.16e.\n", reftime);

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
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
        hr = IMemAllocator_Commit(allocator);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMemAllocator_Release(allocator);
    }

    check_source_position(video, 0, 0, 16, 16);

    SetRect(&rect, 0, 0, 0, 0);
    AdjustWindowRectEx(&rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, FALSE, 0);
    check_destination_position(video, 0, 0, max(16, GetSystemMetrics(SM_CXMIN) - (rect.right - rect.left)),
            max(16, GetSystemMetrics(SM_CYMIN) - (rect.bottom - rect.top)));

out:
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBasicVideo_Release(video);
    IMemInputPin_Release(input);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_windowless_size(void)
{
    ALLOCATOR_PROPERTIES req_props = {1, 32 * 16 * 4, 1, 0}, ret_props;
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
        .bmiHeader.biBitCount = 32,
        .bmiHeader.biPlanes = 1,
    };
    AM_MEDIA_TYPE mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB32,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IBaseFilter *filter = create_vmr9(VMR9Mode_Windowless);
    LONG width, height, aspect_width, aspect_height;
    IVMRAspectRatioControl9 *aspect_ratio_control;
    IVMRWindowlessControl9 *windowless_control;
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    IMemAllocator *allocator;
    RECT src, dst, expect;
    IMemInputPin *input;
    DWORD aspect_mode;
    HWND window;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_QueryInterface(filter, &IID_IVMRWindowlessControl9, (void **)&windowless_control);
    IBaseFilter_QueryInterface(filter, &IID_IVMRAspectRatioControl9, (void **)&aspect_ratio_control);
    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);
    testfilter_init(&source);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, filter, L"vmr9");
    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create a window.\n");

    hr = IVMRWindowlessControl9_SetVideoClippingWindow(windowless_control, window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_GetAllocator(input, &allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        IMemAllocator_Release(allocator);
        if (hr == E_FAIL)
        {
            skip("Got E_FAIL when setting allocator properties.\n");
            goto out;
        }
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
    }

    hr = IVMRWindowlessControl9_GetNativeVideoSize(windowless_control, NULL, &height, &aspect_width, &aspect_height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IVMRWindowlessControl9_GetNativeVideoSize(windowless_control, &width, NULL, &aspect_width, &aspect_height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    aspect_mode = 0xdeadbeef;
    hr = IVMRWindowlessControl9_GetAspectRatioMode(windowless_control, &aspect_mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(aspect_mode == VMR9ARMode_None, "Got mode %lu.\n", aspect_mode);

    aspect_mode = 0xdeadbeef;
    hr = IVMRAspectRatioControl9_GetAspectRatioMode(aspect_ratio_control, &aspect_mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(aspect_mode == VMR9ARMode_None, "Got mode %lu.\n", aspect_mode);

    width = height = 0xdeadbeef;
    hr = IVMRWindowlessControl9_GetNativeVideoSize(windowless_control, &width, &height, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == 32, "Got width %ld.\n", width);
    ok(height == 16, "Got height %ld.\n", height);

    aspect_width = aspect_height = 0xdeadbeef;
    hr = IVMRWindowlessControl9_GetNativeVideoSize(windowless_control, &width, &height, &aspect_width, &aspect_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(aspect_width == 32, "Got width %ld.\n", aspect_width);
    ok(aspect_height == 16, "Got height %ld.\n", aspect_height);

    memset(&src, 0xcc, sizeof(src));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, &src, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 0, 0, 32, 16);
    ok(EqualRect(&src, &expect), "Got source rect %s.\n", wine_dbgstr_rect(&src));

    memset(&dst, 0xcc, sizeof(dst));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, NULL, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 0, 0, 0, 0);
    ok(EqualRect(&dst, &expect), "Got dest rect %s.\n", wine_dbgstr_rect(&dst));

    SetRect(&src, 4, 6, 16, 12);
    hr = IVMRWindowlessControl9_SetVideoPosition(windowless_control, &src, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&src, 0xcc, sizeof(src));
    memset(&dst, 0xcc, sizeof(dst));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, &src, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 4, 6, 16, 12);
    ok(EqualRect(&src, &expect), "Got source rect %s.\n", wine_dbgstr_rect(&src));
    SetRect(&expect, 0, 0, 0, 0);
    ok(EqualRect(&dst, &expect), "Got dest rect %s.\n", wine_dbgstr_rect(&dst));

    SetRect(&dst, 40, 60, 120, 160);
    hr = IVMRWindowlessControl9_SetVideoPosition(windowless_control, NULL, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&src, 0xcc, sizeof(src));
    memset(&dst, 0xcc, sizeof(dst));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, &src, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 4, 6, 16, 12);
    ok(EqualRect(&src, &expect), "Got source rect %s.\n", wine_dbgstr_rect(&src));
    SetRect(&expect, 40, 60, 120, 160);
    ok(EqualRect(&dst, &expect), "Got dest rect %s.\n", wine_dbgstr_rect(&dst));

    GetWindowRect(window, &src);
    SetRect(&expect, 0, 0, 640, 480);
    ok(EqualRect(&src, &expect), "Got window rect %s.\n", wine_dbgstr_rect(&src));

    hr = IVMRAspectRatioControl9_SetAspectRatioMode(aspect_ratio_control, VMR9ARMode_LetterBox);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    aspect_mode = 0xdeadbeef;
    hr = IVMRWindowlessControl9_GetAspectRatioMode(windowless_control, &aspect_mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(aspect_mode == VMR9ARMode_LetterBox, "Got mode %lu.\n", aspect_mode);

    hr = IVMRWindowlessControl9_SetAspectRatioMode(windowless_control, VMR9ARMode_None);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    aspect_mode = 0xdeadbeef;
    hr = IVMRAspectRatioControl9_GetAspectRatioMode(aspect_ratio_control, &aspect_mode);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(aspect_mode == VMR9ARMode_None, "Got mode %lu.\n", aspect_mode);

    hr = IVMRWindowlessControl9_SetAspectRatioMode(windowless_control, VMR9ARMode_LetterBox);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&src, 0xcc, sizeof(src));
    memset(&dst, 0xcc, sizeof(dst));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, &src, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 4, 6, 16, 12);
    ok(EqualRect(&src, &expect), "Got source rect %s.\n", wine_dbgstr_rect(&src));
    SetRect(&expect, 40, 60, 120, 160);
    ok(EqualRect(&dst, &expect), "Got dest rect %s.\n", wine_dbgstr_rect(&dst));

    SetRect(&src, 0, 0, 32, 16);
    SetRect(&dst, 0, 0, 640, 480);
    hr = IVMRWindowlessControl9_SetVideoPosition(windowless_control, &src, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&src, 0xcc, sizeof(src));
    memset(&dst, 0xcc, sizeof(dst));
    hr = IVMRWindowlessControl9_GetVideoPosition(windowless_control, &src, &dst);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SetRect(&expect, 0, 0, 32, 16);
    ok(EqualRect(&src, &expect), "Got source rect %s.\n", wine_dbgstr_rect(&src));
    SetRect(&expect, 0, 0, 640, 480);
    ok(EqualRect(&dst, &expect), "Got dest rect %s.\n", wine_dbgstr_rect(&dst));

out:
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IMemInputPin_Release(input);
    IPin_Release(pin);
    IVMRWindowlessControl9_Release(windowless_control);
    IVMRAspectRatioControl9_Release(aspect_ratio_control);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

static void test_mixing_prefs(void)
{
    IBaseFilter *filter = create_vmr9(VMR9Mode_Windowed);
    IVMRMixerControl9 *mixer_control;
    DWORD flags;
    HRESULT hr;
    ULONG ref;

    set_mixing_mode(filter, 1);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVMRMixerControl9, (void **)&mixer_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRMixerControl9_GetMixingPrefs(mixer_control, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(flags == (MixerPref9_NoDecimation | MixerPref9_ARAdjustXorY | MixerPref9_BiLinearFiltering
            | MixerPref9_RenderTargetRGB), "Got flags %#lx.\n", flags);

    hr = IVMRMixerControl9_SetMixingPrefs(mixer_control, MixerPref9_NoDecimation
            | MixerPref9_ARAdjustXorY | MixerPref9_PointFiltering | MixerPref9_RenderTargetRGB);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRMixerControl9_GetMixingPrefs(mixer_control, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(flags == (MixerPref9_NoDecimation | MixerPref9_ARAdjustXorY | MixerPref9_PointFiltering
            | MixerPref9_RenderTargetRGB), "Got flags %#lx.\n", flags);

    IVMRMixerControl9_Release(mixer_control);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_unconnected_eos(void)
{
    IFilterGraph2 *graph = create_graph();
    IBaseFilter *filter = create_vmr9(0);
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

static void test_notifyevent(void)
{
    IFilterGraph2 *graph = create_graph();
    IBaseFilter *filter = create_vmr9(VMR9Mode_Renderless);
    IVMRSurfaceAllocatorNotify9 *notify;
    IMediaEvent *eventsrc;
    unsigned int ret;
    HRESULT hr;
    ULONG ref;

    hr = IFilterGraph2_AddFilter(graph, filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_event_code(eventsrc, 0, 0x12345678, 0x9ABC, 0xDEF0);
    ok(ret == 0, "Got %u custom events.\n", ret);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVMRSurfaceAllocatorNotify9, (void **)&notify);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IVMRSurfaceAllocatorNotify9_NotifyEvent(notify, 0x12345678, 0x9ABC, 0xDEF0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_event_code(eventsrc, 0, 0x12345678, 0x9ABC, 0xDEF0);
    ok(ret == 1, "Got %u custom events.\n", ret);

    IMediaEvent_Release(eventsrc);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IVMRSurfaceAllocatorNotify9_Release(notify);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_changed3ddevice(void)
{
    VIDEOINFOHEADER vih =
    {
        .rcSource = {4, 6, 16, 12},
        .rcTarget = {40, 60, 160, 120},
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB32,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    struct presenter presenter =
    {
        .IVMRSurfaceAllocator9_iface.lpVtbl = &allocator_vtbl,
        .IVMRImagePresenter9_iface.lpVtbl = &presenter_vtbl,
        .refcount = 1,
        .accept_flags = VMR9AllocFlag_TextureSurface,
    };
    ALLOCATOR_PROPERTIES req_props = {5, 32 * 16 * 4, 1, 0}, ret_props;
    IBaseFilter *filter = create_vmr9(VMR9Mode_Renderless);
    IFilterGraph2 *graph = create_graph();
    IVMRSurfaceAllocatorNotify9 *notify;
    RECT rect = {0, 0, 640, 480};
    IDirect3DDevice9 *device;
    struct testfilter source;
    IMemAllocator *allocator;
    IMediaControl *control;
    IMemInputPin *input;
    OAFilterState state;
    IPin *pin = NULL;
    HWND window;
    HRESULT hr;
    ULONG ref;

    testfilter_init(&source);

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowA("static", "quartz_test", WS_OVERLAPPEDWINDOW, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    if (!(device = create_device(window)))
    {
        IBaseFilter_Release(filter);
        DestroyWindow(window);
        return;
    }

    IBaseFilter_QueryInterface(filter, &IID_IVMRSurfaceAllocatorNotify9, (void **)&notify);
    presenter.notify = notify;

    hr = IVMRSurfaceAllocatorNotify9_AdviseSurfaceAllocator(notify, 0xabacab,
            &presenter.IVMRSurfaceAllocator9_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IVMRSurfaceAllocatorNotify9_SetD3DDevice(notify, device, MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY));
    if (hr == E_NOINTERFACE)
    {
        win_skip("Direct3D does not support video rendering.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device);

    device = create_device(window);
    ok(device != NULL, "Couldn't create device\n");

    hr = IVMRSurfaceAllocatorNotify9_ChangeD3DDevice(notify, device, MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    presenter.device = device;
    presenter.got_PresentImage = 0;

    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == VFW_E_NO_ALLOCATOR)
    {
        CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
                &IID_IMemAllocator, (void **)&allocator);

        hr = IMemInputPin_NotifyAllocator(input, allocator, TRUE);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }
    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    join_thread(send_frame(input));

    hr = IMediaControl_GetState(control, 100, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(presenter.got_PresentImage >= 1, "Got %u calls to PresentImage().\n", presenter.got_PresentImage);
    IMediaControl_Release(control);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemAllocator_Release(allocator);
    IMemInputPin_Release(input);
    IPin_Release(pin);

out:
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IVMRSurfaceAllocatorNotify9_Release(notify);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    DestroyWindow(window);
}

START_TEST(vmr9)
{
    IBaseFilter *filter;
    HRESULT hr;

    CoInitialize(NULL);

    if (FAILED(hr = CoCreateInstance(&CLSID_VideoMixingRenderer9, NULL,
            CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&filter)))
    {
        skip("Failed to create VMR9, hr %#lx.\n", hr);
        return;
    }
    IBaseFilter_Release(filter);

    test_filter_config();
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
    test_allocate_surface_helper();
    test_renderless_formats();
    test_mixing_mode();
    test_clipping_window();
    test_surface_allocator_notify_refcount();
    test_basic_video();
    test_windowless_size();
    test_mixing_prefs();
    test_unconnected_eos();
    test_notifyevent();
    test_changed3ddevice();

    CoUninitialize();
}
