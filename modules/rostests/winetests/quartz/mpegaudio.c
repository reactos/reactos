/*
 * MPEG audio decoder filter unit tests
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

static const MPEG1WAVEFORMAT mp1_format =
{
    .wfx.wFormatTag = WAVE_FORMAT_MPEG,
    .wfx.nChannels = 1,
    .wfx.nSamplesPerSec = 32000,
    .wfx.nBlockAlign = 48,
    .wfx.nAvgBytesPerSec = 4000,
    .wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX),
    .fwHeadLayer = ACM_MPEG_LAYER1,
    .dwHeadBitrate = 32000,
    .fwHeadMode = ACM_MPEG_SINGLECHANNEL,
    .fwHeadFlags = ACM_MPEG_ID_MPEG1,
};

static const AM_MEDIA_TYPE mp1_mt =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1AudioPayload, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000050, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 1,
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(MPEG1WAVEFORMAT),
    .pbFormat = (BYTE *)&mp1_format,
};

static const MPEG1WAVEFORMAT mp2_format =
{
    .wfx.wFormatTag = WAVE_FORMAT_MPEG,
    .wfx.nChannels = 1,
    .wfx.nSamplesPerSec = 32000,
    .wfx.nBlockAlign = 144,
    .wfx.nAvgBytesPerSec = 4000,
    .wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX),
    .fwHeadLayer = ACM_MPEG_LAYER2,
    .dwHeadBitrate = 32000,
    .fwHeadMode = ACM_MPEG_SINGLECHANNEL,
    .fwHeadFlags = ACM_MPEG_ID_MPEG1,
};

static const AM_MEDIA_TYPE mp2_mt =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1AudioPayload, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000050, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 1,
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(MPEG1WAVEFORMAT),
    .pbFormat = (BYTE *)&mp2_format,
};

static const MPEG1WAVEFORMAT mp3_format0 =
{
    .wfx.wFormatTag = WAVE_FORMAT_MPEG,
    .wfx.nChannels = 1,
    .wfx.nSamplesPerSec = 32000,
    .wfx.nBlockAlign = 144,
    .wfx.nAvgBytesPerSec = 4000,
    .wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX),
    .fwHeadLayer = ACM_MPEG_LAYER3,
    .dwHeadBitrate = 32000,
    .fwHeadMode = ACM_MPEG_SINGLECHANNEL,
    .fwHeadFlags = ACM_MPEG_ID_MPEG1,
};

static const AM_MEDIA_TYPE mp3_mt0 =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1AudioPayload, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000050, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 1,
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(MPEG1WAVEFORMAT),
    .pbFormat = (BYTE *)&mp3_format0,
};

static const MPEGLAYER3WAVEFORMAT mp3_format1 =
{
    .wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3,
    .wfx.nChannels = 1,
    .wfx.nSamplesPerSec = 32000,
    .wfx.nBlockAlign = 144,
    .wfx.nAvgBytesPerSec = 4000,
    .wfx.cbSize = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX),
    .wID = MPEGLAYER3_ID_MPEG,
    .nBlockSize = 144,
    .nFramesPerBlock = 1,
};

static const AM_MEDIA_TYPE mp3_mt1 =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_MP3, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000055, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 1,
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(MPEGLAYER3WAVEFORMAT),
    .pbFormat = (BYTE *)&mp3_format1,
};

static const WAVEFORMATEXTENSIBLE pcm16ex_format =
{
    .Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE,
    .Format.nChannels = 1,
    .Format.nSamplesPerSec = 32000,
    .Format.wBitsPerSample = 16,
    .Format.nBlockAlign = 2,
    .Format.nAvgBytesPerSec = 64000,
    .Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX),
    .Samples.wValidBitsPerSample = 16,
    .dwChannelMask = KSAUDIO_SPEAKER_STEREO,
    /* KSDATAFORMAT_SUBTYPE_PCM */
    .SubFormat = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
};

static const AM_MEDIA_TYPE pcm16ex_mt =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_PCM, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .bFixedSizeSamples = TRUE,
    .lSampleSize = 2,
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(WAVEFORMATEXTENSIBLE),
    .pbFormat = (BYTE *)&pcm16ex_format,
};

static IBaseFilter *create_mpeg_audio_codec(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_CMpegAudioCodec, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static void init_pcm_mt(AM_MEDIA_TYPE *mt, WAVEFORMATEX *format,
        WORD channels, DWORD sample_rate, WORD depth)
{
    memset(format, 0, sizeof(WAVEFORMATEX));
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = channels;
    format->nSamplesPerSec = sample_rate;
    format->wBitsPerSample = depth;
    format->nBlockAlign = channels * depth / 8;
    format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
    memset(mt, 0, sizeof(AM_MEDIA_TYPE));
    mt->majortype = MEDIATYPE_Audio;
    mt->subtype = MEDIASUBTYPE_PCM;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = format->nBlockAlign;
    mt->formattype = FORMAT_WaveFormatEx;
    mt->cbFormat = sizeof(WAVEFORMATEX);
    mt->pbFormat = (BYTE *)format;
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
    IBaseFilter *filter = create_mpeg_audio_codec();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMpegAudioDecoder, TRUE);
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
    hr = CoCreateInstance(&CLSID_CMpegAudioCodec, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_CMpegAudioCodec, &test_outer, CLSCTX_INPROC_SERVER,
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
    IBaseFilter *filter = create_mpeg_audio_codec();
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
    IBaseFilter *filter = create_mpeg_audio_codec();
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
    IBaseFilter *filter = create_mpeg_audio_codec();
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
    IBaseFilter *filter = create_mpeg_audio_codec();
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
    ok(!wcscmp(info.achName, L"XForm In"), "Got name %s.\n", debugstr_w(info.achName));
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
    ok(!wcscmp(info.achName, L"XForm Out"), "Got name %s.\n", debugstr_w(info.achName));
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
    IBaseFilter *filter = create_mpeg_audio_codec();
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
    IBaseFilter *filter = create_mpeg_audio_codec();
    WAVEFORMATEX format;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryAccept(pin, &mp1_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryAccept(pin, &mp2_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.subtype = MEDIASUBTYPE_MPEG1Payload;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryAccept(pin, &mp3_mt0);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_QueryAccept(pin, &mp3_mt1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.subtype = MEDIASUBTYPE_PCM;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    mt = mp2_mt;
    mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    init_pcm_mt(&mt, &format, 1, 32000, 16);
    hr = IPin_QueryAccept(pin, &mt);
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
    BOOL todo_time;
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
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IQualityControl) && filter->qc)
        *out = &filter->qc->IQualityControl_iface;
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
    HRESULT hr;
    LONG size;

    ++filter->got_sample;

    size = IMediaSample_GetSize(sample);
    ok(size == 3072, "Got size %lu.\n", size);
    size = IMediaSample_GetActualDataLength(sample);
    ok(size == 768, "Got valid size %lu.\n", size);

    start = 0xdeadbeef;
    stop = 0xdeadbeef;
    hr = IMediaSample_GetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (filter->got_sample == 1)
    {
        todo_wine_if(filter->todo_time)
        ok(start == filter->expected_start_time, "Got start time %s.\n", wine_dbgstr_longlong(start));
        todo_wine_if(filter->todo_time)
        ok(stop == filter->expected_stop_time, "Got stop time %s.\n", wine_dbgstr_longlong(stop));
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

static void test_source_allocator(IFilterGraph2 *graph, IMediaControl *control,
        IPin *sink, IPin *source, struct testfilter *testsource, struct testfilter *testsink)
{
    ALLOCATOR_PROPERTIES props, req_props = {2, 30000, 32, 0};
    IMemAllocator *allocator;
    IMediaSample *sample;
    WAVEFORMATEX format;
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    hr = IFilterGraph2_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &mp2_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&mt, &format, 1, 32000, 16);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 8, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 9216, "Got size %ld.\n", props.cbBuffer);
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

    init_pcm_mt(&mt, &format, 1, 32000, 8);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 8, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 4608, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator);
    testsink->sink.pAllocator = allocator;

    hr = IMemAllocator_SetProperties(allocator, &req_props, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&mt, &format, 1, 32000, 16);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(testsink->sink.pAllocator == allocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 8, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 9216, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    IFilterGraph2_Disconnect(graph, sink);
    IFilterGraph2_Disconnect(graph, &testsource->source.pin.IPin_iface);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &mp1_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&mt, &format, 1, 32000, 16);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 8, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 3072, "Got size %ld.\n", props.cbBuffer);
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

    hr = IPin_QueryInterface(sink, &IID_IQualityControl, (void **)&sink_qc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_QueryInterface(source, &IID_IQualityControl, (void **)&source_qc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_Notify(sink_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    hr = IQualityControl_SetSink(sink_qc, &qc.IQualityControl_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    memset(&qc.notify_quality, 0xaa, sizeof(qc.notify_quality));
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == filter, "Got sender %p.\n", qc.notify_sender);
    ok(!memcmp(&qc.notify_quality, &quality, sizeof(quality)), "Quality didn't match.\n");
    hr = IQualityControl_SetSink(sink_qc, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_SetSink(source_qc, &qc.IQualityControl_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == (IBaseFilter *)0xdeadbeef, "Got sender %p.\n", qc.notify_sender);
    hr = IQualityControl_SetSink(source_qc, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &mp2_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    testsource->qc = &testsource_qc;

    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    hr = IQualityControl_Notify(sink_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == (IBaseFilter *)0xdeadbeef, "Got sender %p.\n", qc.notify_sender);

    testsource_qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    memset(&testsource_qc.notify_quality, 0xaa, sizeof(testsource_qc.notify_quality));
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsource_qc.notify_sender == filter, "Got sender %p.\n", testsource_qc.notify_sender);
    ok(!memcmp(&testsource_qc.notify_quality, &quality, sizeof(quality)), "Quality didn't match.\n");

    testsource_qc.notify_hr = E_FAIL;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    testsource_qc.notify_hr = S_OK;

    hr = IQualityControl_SetSink(sink_qc, &qc.IQualityControl_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    memset(&qc.notify_quality, 0xaa, sizeof(qc.notify_quality));
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == filter, "Got sender %p.\n", qc.notify_sender);
    ok(!memcmp(&qc.notify_quality, &quality, sizeof(quality)), "Quality didn't match.\n");
    hr = IQualityControl_SetSink(sink_qc, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IQualityControl_SetSink(source_qc, &qc.IQualityControl_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    qc.notify_sender = (IBaseFilter *)0xdeadbeef;
    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(qc.notify_sender == (IBaseFilter *)0xdeadbeef, "Got sender %p.\n", qc.notify_sender);
    hr = IQualityControl_SetSink(source_qc, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IFilterGraph2_Disconnect(graph, sink);
    IFilterGraph2_Disconnect(graph, &testsource->source.pin.IPin_iface);

    hr = IQualityControl_Notify(source_qc, &testsink->filter.IBaseFilter_iface, quality);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);

    IQualityControl_Release(source_qc);
    IQualityControl_Release(sink_qc);

    testsource->qc = NULL;
}

static void test_sample_processing(IMediaControl *control, IMemInputPin *input, struct testfilter *sink)
{
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    BYTE *data;
    LONG size;

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
    memset(data, 0, 48);
    data[0] = 0xff;
    data[1] = 0xff;
    data[2] = 0x18;
    data[3] = 0xc4;
    hr = IMediaSample_SetActualDataLength(sample, 48);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSample_SetTime(sample, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = 0;
    sink->expected_stop_time = 120000;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 22222;
    hr = IMediaSample_SetTime(sample, &start, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = 22222;
    sink->expected_stop_time = 142222;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
    sink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 22222;
    stop = 33333;
    hr = IMediaSample_SetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink->expected_start_time = 22222;
    sink->expected_stop_time = 142222;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink->got_sample >= 1, "Got %u calls to Receive().\n", sink->got_sample);
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
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    BYTE *data;

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
    memset(data, 0, 48);
    data[0] = 0xff;
    data[1] = 0xff;
    data[2] = 0x18;
    data[3] = 0xc4;
    hr = IMediaSample_SetActualDataLength(sample, 48);
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
    testsink->expected_stop_time = 120000;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
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

    testsink->expected_start_time = 0;
    testsink->expected_stop_time = 120000;
    testsink->todo_time = TRUE;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_sample >= 1, "Got %u calls to Receive().\n", testsink->got_sample);
    testsink->got_sample = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_connect_pin(void)
{
    IBaseFilter *filter = create_mpeg_audio_codec();
    struct testfilter testsource, testsink;
    AM_MEDIA_TYPE mt, source_mt, *pmt;
    WAVEFORMATEX source_format;
    IPin *sink, *source, *peer;
    IEnumMediaTypes *enummt;
    WAVEFORMATEX req_format;
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
    IFilterGraph2_AddFilter(graph, filter, L"MPEG audio decoder");
    IBaseFilter_FindPin(filter, L"In", &sink);
    IBaseFilter_FindPin(filter, L"Out", &source);
    IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&meminput);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    test_source_allocator(graph, control, sink, source, &testsource, &testsink);
    test_quality_control(graph, filter, sink, source, &testsource, &testsink);

    /* Test sink connection. */

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &mp1_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    req_mt = mp1_mt;
    req_mt.subtype = MEDIASUBTYPE_PCM;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &mp1_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsource.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &mp1_mt), "Media types didn't match.\n");
    ok(compare_media_types(&testsource.source.pin.mt, &mp1_mt), "Media types didn't match.\n");
    FreeMediaType(&mt);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 8);
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 2, 32000, 16);
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 16000, 16);
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 24);
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_QueryAccept(source, &pcm16ex_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_format.nBlockAlign = 333;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_format.nAvgBytesPerSec = 333;
    hr = IPin_QueryAccept(source, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_EnumMediaTypes(source, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 2; ++i)
    {
        WAVEFORMATEX expect_format;
        AM_MEDIA_TYPE expect_mt;

        init_pcm_mt(&expect_mt, &expect_format, 1, 32000, i ? 8 : 16);

        hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        if (hr != S_OK)
            break;
        ok(!memcmp(pmt, &expect_mt, offsetof(AM_MEDIA_TYPE, cbFormat)),
                "%u: Media types didn't match.\n", i);
        ok(!memcmp(pmt->pbFormat, &expect_format, sizeof(WAVEFORMATEX)),
                "%u: Format blocks didn't match.\n", i);

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

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
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

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.lSampleSize = 999;
    req_mt.bTemporalCompression = TRUE;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &req_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    /* Connection with wildcards. */

    init_pcm_mt(&source_mt, &source_format, 1, 32000, 16);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = GUID_NULL;
    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = GUID_NULL;
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &source_mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = MEDIATYPE_Video;
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    /* Test enumeration of sink media types. */

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
    req_mt.majortype = MEDIATYPE_Video;
    req_mt.subtype = GUID_NULL;
    req_mt.formattype = GUID_NULL;
    testsink.mt = &req_mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    init_pcm_mt(&req_mt, &req_format, 1, 32000, 16);
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

START_TEST(mpegaudio)
{
    IBaseFilter *filter;

    CoInitialize(NULL);

    if (FAILED(CoCreateInstance(&CLSID_CMpegAudioCodec, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        skip("Failed to create MPEG audio decoder instance.\n");
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
