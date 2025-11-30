/*
 * Audio capture filter
 *
 * Copyright 2015 Damjan Jovanovic
 * Copyright 2023 Zeb Figura for CodeWeavers
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

#include "qcap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct audio_record
{
    struct strmbase_filter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;

    struct strmbase_source source;
    IAMBufferNegotiation IAMBufferNegotiation_iface;
    IAMStreamConfig IAMStreamConfig_iface;
    IKsPropertySet IKsPropertySet_iface;

    unsigned int id;
    HWAVEIN device;
    HANDLE event;
    HANDLE thread;

    /* FIXME: It would be nice to avoid duplicating this variable with strmbase.
     * However, synchronization is tricky; we need access to be protected by a
     * separate lock. */
    FILTER_STATE state;
    CONDITION_VARIABLE state_cv;
    CRITICAL_SECTION state_cs;

    AM_MEDIA_TYPE format;
    ALLOCATOR_PROPERTIES props;
};

static struct audio_record *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, struct audio_record, filter);
}

static HRESULT audio_record_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IAMBufferNegotiation))
        *out = &filter->IAMBufferNegotiation_iface;
    else if (IsEqualGUID(iid, &IID_IAMStreamConfig))
        *out = &filter->IAMStreamConfig_iface;
    else if (IsEqualGUID(iid, &IID_IKsPropertySet))
        *out = &filter->IKsPropertySet_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT audio_record_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
        return S_FALSE;

    return S_OK;
}

static const struct
{
    unsigned int rate;
    unsigned int depth;
    unsigned int channels;
}
audio_formats[] =
{
    {44100, 16, 2},
    {44100, 16, 1},
    {32000, 16, 2},
    {32000, 16, 1},
    {22050, 16, 2},
    {22050, 16, 1},
    {11025, 16, 2},
    {11025, 16, 1},
    { 8000, 16, 2},
    { 8000, 16, 1},
    {44100,  8, 2},
    {44100,  8, 1},
    {22050,  8, 2},
    {22050,  8, 1},
    {11025,  8, 2},
    {11025,  8, 1},
    { 8000,  8, 2},
    { 8000,  8, 1},
    {48000, 16, 2},
    {48000, 16, 1},
    {96000, 16, 2},
    {96000, 16, 1},
};

static HRESULT fill_media_type(struct audio_record *filter, unsigned int index, AM_MEDIA_TYPE *mt)
{
    WAVEFORMATEX *format;

    if (index >= 1 + ARRAY_SIZE(audio_formats))
        return VFW_S_NO_MORE_ITEMS;

    if (!index)
        return CopyMediaType(mt, &filter->format);
    --index;

    if (!(format = CoTaskMemAlloc(sizeof(*format))))
        return E_OUTOFMEMORY;

    memset(format, 0, sizeof(WAVEFORMATEX));
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = audio_formats[index].channels;
    format->nSamplesPerSec = audio_formats[index].rate;
    format->wBitsPerSample = audio_formats[index].depth;
    format->nBlockAlign = audio_formats[index].channels * audio_formats[index].depth / 8;
    format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));
    mt->majortype = MEDIATYPE_Audio;
    mt->subtype = MEDIASUBTYPE_PCM;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = format->nBlockAlign;
    mt->formattype = FORMAT_WaveFormatEx;
    mt->cbFormat = sizeof(WAVEFORMATEX);
    mt->pbFormat = (BYTE *)format;

    return S_OK;
}

static HRESULT audio_record_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface->filter);

    return fill_media_type(filter, index, mt);
}

static HRESULT WINAPI audio_record_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface->pin.filter);
    const WAVEFORMATEX *format = (void *)filter->source.pin.mt.pbFormat;
    ALLOCATOR_PROPERTIES ret_props;
    MMRESULT ret;
    HRESULT hr;

    props->cBuffers = (filter->props.cBuffers == -1) ? 4 : filter->props.cBuffers;
    /* This is the algorithm that native uses. The alignment to an even number
     * doesn't make much sense, and may be a bug. */
    if (filter->props.cbBuffer == -1)
        props->cbBuffer = (format->nAvgBytesPerSec / 2) & ~1;
    else
        props->cbBuffer = filter->props.cbBuffer & ~1;
    if (filter->props.cbAlign == -1 || filter->props.cbAlign == 0)
        props->cbAlign = 1;
    else
        props->cbAlign = filter->props.cbAlign;
    props->cbPrefix = (filter->props.cbPrefix == -1) ? 0 : filter->props.cbPrefix;

    if (FAILED(hr = IMemAllocator_SetProperties(allocator, props, &ret_props)))
        return hr;

    if ((ret = waveInOpen(&filter->device, filter->id, format,
            (DWORD_PTR)filter->event, 0, CALLBACK_EVENT)) != MMSYSERR_NOERROR)
    {
        ERR("Failed to open device %u, error %u.\n", filter->id, ret);
        return E_FAIL;
    }

    return S_OK;
}

static void audio_record_source_disconnect(struct strmbase_source *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface->pin.filter);

    waveInClose(filter->device);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_get_media_type = audio_record_source_get_media_type,
    .base.pin_query_accept = audio_record_source_query_accept,
    .base.pin_query_interface = audio_record_source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = audio_record_source_DecideBufferSize,
    .source_disconnect = audio_record_source_disconnect,
};

static struct audio_record *impl_from_IAMStreamConfig(IAMStreamConfig *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IAMStreamConfig_iface);
}

static HRESULT WINAPI stream_config_QueryInterface(IAMStreamConfig *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI stream_config_AddRef(IAMStreamConfig *iface)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI stream_config_Release(IAMStreamConfig *iface)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI stream_config_SetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE *mt)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    HRESULT hr;

    TRACE("iface %p, mt %p.\n", iface, mt);
    strmbase_dump_media_type(mt);

    if (!mt)
        return E_POINTER;

    EnterCriticalSection(&filter->filter.filter_cs);

    if ((hr = CopyMediaType(&filter->format, mt)))
    {
        LeaveCriticalSection(&filter->filter.filter_cs);
        return hr;
    }

    LeaveCriticalSection(&filter->filter.filter_cs);

    return S_OK;
}

static HRESULT WINAPI stream_config_GetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE **ret_mt)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    AM_MEDIA_TYPE *mt;
    HRESULT hr;

    TRACE("iface %p, mt %p.\n", iface, ret_mt);

    if (!(mt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    EnterCriticalSection(&filter->filter.filter_cs);

    if ((hr = CopyMediaType(mt, &filter->format)))
    {
        LeaveCriticalSection(&filter->filter.filter_cs);
        CoTaskMemFree(mt);
        return hr;
    }

    LeaveCriticalSection(&filter->filter.filter_cs);

    *ret_mt = mt;
    return S_OK;
}

static HRESULT WINAPI stream_config_GetNumberOfCapabilities(IAMStreamConfig *iface,
        int *count, int *size)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);

    TRACE("filter %p, count %p, size %p.\n", filter, count, size);

    *count = 1 + ARRAY_SIZE(audio_formats);
    *size = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    return S_OK;
}

static HRESULT WINAPI stream_config_GetStreamCaps(IAMStreamConfig *iface,
        int index, AM_MEDIA_TYPE **ret_mt, BYTE *caps)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    AUDIO_STREAM_CONFIG_CAPS *audio_caps = (void *)caps;
    AM_MEDIA_TYPE *mt;
    HRESULT hr;

    TRACE("filter %p, index %d, ret_mt %p, caps %p.\n", filter, index, ret_mt, caps);

    if (index >= 1 + ARRAY_SIZE(audio_formats))
        return S_FALSE;

    if (!(mt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    if ((hr = fill_media_type(filter, index, mt)) != S_OK)
    {
        CoTaskMemFree(mt);
        return hr;
    }

    *ret_mt = mt;

    audio_caps->guid = MEDIATYPE_Audio;
    audio_caps->MinimumChannels = 1;
    audio_caps->MaximumChannels = 2;
    audio_caps->ChannelsGranularity = 1;
    audio_caps->MinimumBitsPerSample = 8;
    audio_caps->MaximumBitsPerSample = 16;
    audio_caps->BitsPerSampleGranularity = 8;
    audio_caps->MinimumSampleFrequency = 11025;
    audio_caps->MaximumSampleFrequency = 44100;
    audio_caps->SampleFrequencyGranularity = 11025;

    return S_OK;
}

static const IAMStreamConfigVtbl stream_config_vtbl =
{
    stream_config_QueryInterface,
    stream_config_AddRef,
    stream_config_Release,
    stream_config_SetFormat,
    stream_config_GetFormat,
    stream_config_GetNumberOfCapabilities,
    stream_config_GetStreamCaps,
};

static struct audio_record *impl_from_IAMBufferNegotiation(IAMBufferNegotiation *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IAMBufferNegotiation_iface);
}

static HRESULT WINAPI buffer_negotiation_QueryInterface(IAMBufferNegotiation *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_IAMBufferNegotiation(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI buffer_negotiation_AddRef(IAMBufferNegotiation *iface)
{
    struct audio_record *filter = impl_from_IAMBufferNegotiation(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI buffer_negotiation_Release(IAMBufferNegotiation *iface)
{
    struct audio_record *filter = impl_from_IAMBufferNegotiation(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI buffer_negotiation_SuggestAllocatorProperties(
        IAMBufferNegotiation *iface, const ALLOCATOR_PROPERTIES *props)
{
    struct audio_record *filter = impl_from_IAMBufferNegotiation(iface);

    TRACE("filter %p, props %p.\n", filter, props);
    TRACE("Requested %ld buffers, size %ld, alignment %ld, prefix %ld.\n",
            props->cBuffers, props->cbBuffer, props->cbAlign, props->cbPrefix);

    EnterCriticalSection(&filter->state_cs);
    filter->props = *props;
    LeaveCriticalSection(&filter->state_cs);

    return S_OK;
}

static HRESULT WINAPI buffer_negotiation_GetAllocatorProperties(
        IAMBufferNegotiation *iface, ALLOCATOR_PROPERTIES *props)
{
    FIXME("iface %p, props %p, stub!\n", iface, props);
    return E_NOTIMPL;
}

static const IAMBufferNegotiationVtbl buffer_negotiation_vtbl =
{
    buffer_negotiation_QueryInterface,
    buffer_negotiation_AddRef,
    buffer_negotiation_Release,
    buffer_negotiation_SuggestAllocatorProperties,
    buffer_negotiation_GetAllocatorProperties,
};

static struct audio_record *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IKsPropertySet_iface);
}

static HRESULT WINAPI property_set_QueryInterface(IKsPropertySet *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_IKsPropertySet(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI property_set_AddRef(IKsPropertySet *iface)
{
    struct audio_record *filter = impl_from_IKsPropertySet(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI property_set_Release(IKsPropertySet *iface)
{
    struct audio_record *filter = impl_from_IKsPropertySet(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI property_set_Set(IKsPropertySet *iface, const GUID *set, DWORD id,
        void *instance, DWORD instance_size, void *data, DWORD size)
{
    FIXME("iface %p, set %s, id %lu, instance %p, instance_size %lu, data %p, size %lu, stub!\n",
            iface, debugstr_guid(set), id, instance, instance_size, data, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_set_Get(IKsPropertySet *iface, const GUID *set, DWORD id,
        void *instance, DWORD instance_size, void *data, DWORD size, DWORD *ret_size)
{
    struct audio_record *filter = impl_from_IKsPropertySet(iface);

    TRACE("filter %p, set %s, id %lu, instance %p, instance_size %lu, data %p, size %lu, ret_size %p.\n",
            filter, debugstr_guid(set), id, instance, instance_size, data, size, ret_size);

    if (!IsEqualGUID(set, &AMPROPSETID_Pin))
    {
        FIXME("Unknown set %s, returning E_PROP_SET_UNSUPPORTED.\n", debugstr_guid(set));
        return E_PROP_SET_UNSUPPORTED;
    }

    if (id != AMPROPERTY_PIN_CATEGORY)
    {
        FIXME("Unknown id %lu, returning E_PROP_ID_UNSUPPORTED.\n", id);
        return E_PROP_ID_UNSUPPORTED;
    }

    if (instance || instance_size)
        FIXME("Unexpected instance data %p, size %lu.\n", instance, instance_size);

    *ret_size = sizeof(GUID);

    if (size < sizeof(GUID))
        return E_UNEXPECTED;

    *(GUID *)data = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

static HRESULT WINAPI property_set_QuerySupported(IKsPropertySet *iface,
        const GUID *set, DWORD id, DWORD *support)
{
    FIXME("iface %p, set %s, id %lu, support %p, stub!\n", iface, debugstr_guid(set), id, support);
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl property_set_vtbl =
{
    property_set_QueryInterface,
    property_set_AddRef,
    property_set_Release,
    property_set_Set,
    property_set_Get,
    property_set_QuerySupported,
};

static struct audio_record *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IPersistPropertyBag_iface);
}

static struct strmbase_pin *audio_record_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void audio_record_destroy(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    filter->state_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->state_cs);
    CloseHandle(filter->event);
    FreeMediaType(&filter->format);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT audio_record_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static DWORD WINAPI stream_thread(void *arg)
{
    struct audio_record *filter = arg;
    const WAVEFORMATEX *format = (void *)filter->source.pin.mt.pbFormat;
    bool started = false;
    MMRESULT ret;

    /* FIXME: We should probably queue several buffers instead of just one. */

    EnterCriticalSection(&filter->state_cs);

    for (;;)
    {
        IMediaSample *sample;
        WAVEHDR header = {0};
        HRESULT hr;
        BYTE *data;

        while (filter->state == State_Paused)
            SleepConditionVariableCS(&filter->state_cv, &filter->state_cs, INFINITE);

        if (filter->state == State_Stopped)
            break;

        if (FAILED(hr = IMemAllocator_GetBuffer(filter->source.pAllocator, &sample, NULL, NULL, 0)))
        {
            ERR("Failed to get sample, hr %#lx.\n", hr);
            break;
        }

        IMediaSample_GetPointer(sample, &data);

        header.lpData = (void *)data;
        header.dwBufferLength = IMediaSample_GetSize(sample);
        if ((ret = waveInPrepareHeader(filter->device, &header, sizeof(header))) != MMSYSERR_NOERROR)
            ERR("Failed to prepare header, error %u.\n", ret);

        if ((ret = waveInAddBuffer(filter->device, &header, sizeof(header))) != MMSYSERR_NOERROR)
            ERR("Failed to add buffer, error %u.\n", ret);

        if (!started)
        {
            if ((ret = waveInStart(filter->device)) != MMSYSERR_NOERROR)
                ERR("Failed to start, error %u.\n", ret);
            started = true;
        }

        while (!(header.dwFlags & WHDR_DONE) && filter->state == State_Running)
        {
            LeaveCriticalSection(&filter->state_cs);

            if ((ret = WaitForSingleObject(filter->event, INFINITE)))
                ERR("Failed to wait, error %u.\n", ret);

            EnterCriticalSection(&filter->state_cs);
        }

        if (filter->state != State_Running)
        {
            TRACE("State is %#x; resetting.\n", filter->state);
            if ((ret = waveInReset(filter->device)) != MMSYSERR_NOERROR)
                ERR("Failed to reset, error %u.\n", ret);
        }

        if ((ret = waveInUnprepareHeader(filter->device, &header, sizeof(header))) != MMSYSERR_NOERROR)
            ERR("Failed to unprepare header, error %u.\n", ret);

        IMediaSample_SetActualDataLength(sample, header.dwBytesRecorded);

        if (filter->state == State_Running)
        {
            REFERENCE_TIME start_pts, end_pts;
            MMTIME time;

            time.wType = TIME_BYTES;
            if ((ret = waveInGetPosition(filter->device, &time, sizeof(time))) != MMSYSERR_NOERROR)
                ERR("Failed to get position, error %u.\n", ret);
            if (time.wType != TIME_BYTES)
                ERR("Got unexpected type %#x.\n", time.wType);
            end_pts = MulDiv(time.u.cb, 10000000, format->nAvgBytesPerSec);
            start_pts = MulDiv(time.u.cb - header.dwBytesRecorded, 10000000, format->nAvgBytesPerSec);
            IMediaSample_SetTime(sample, &start_pts, &end_pts);

            TRACE("Sending buffer %p.\n", sample);
            if (FAILED(hr = IMemInputPin_Receive(filter->source.pMemInputPin, sample)))
            {
                ERR("IMemInputPin::Receive() returned %#lx.\n", hr);
                IMediaSample_Release(sample);
                break;
            }
        }

        IMediaSample_Release(sample);
    }

    LeaveCriticalSection(&filter->state_cs);

    if (started && (ret = waveInStop(filter->device)) != MMSYSERR_NOERROR)
        ERR("Failed to stop, error %u.\n", ret);

    return 0;
}

static HRESULT audio_record_init_stream(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    if (FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->state_cs);

    filter->thread = CreateThread(NULL, 0, stream_thread, filter, 0, NULL);

    return S_OK;
}

static HRESULT audio_record_start_stream(struct strmbase_filter *iface, REFERENCE_TIME time)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Running;
    LeaveCriticalSection(&filter->state_cs);
    WakeConditionVariable(&filter->state_cv);
    return S_OK;
}

static HRESULT audio_record_stop_stream(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->state_cs);
    SetEvent(filter->event);
    return S_OK;
}

static HRESULT audio_record_cleanup_stream(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Stopped;
    LeaveCriticalSection(&filter->state_cs);
    WakeConditionVariable(&filter->state_cv);
    SetEvent(filter->event);

    WaitForSingleObject(filter->thread, INFINITE);
    CloseHandle(filter->thread);
    filter->thread = NULL;

    hr = IMemAllocator_Decommit(filter->source.pAllocator);
    if (hr != S_OK && hr != VFW_E_NOT_COMMITTED)
        ERR("Failed to decommit allocator, hr %#lx.\n", hr);

    return S_OK;
}

static HRESULT audio_record_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (filter->filter.state == State_Paused)
        return VFW_S_CANT_CUE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = audio_record_get_pin,
    .filter_destroy = audio_record_destroy,
    .filter_query_interface = audio_record_query_interface,
    .filter_init_stream = audio_record_init_stream,
    .filter_start_stream = audio_record_start_stream,
    .filter_stop_stream = audio_record_stop_stream,
    .filter_cleanup_stream = audio_record_cleanup_stream,
    .filter_wait_state = audio_record_wait_state,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, LPVOID *ppv)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI PPB_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pClassID);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag *iface)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(): stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *bag, IErrorLog *error_log)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);
    VARIANT var;
    HRESULT hr;

    TRACE("filter %p, bag %p, error_log %p.\n", filter, bag, error_log);

    V_VT(&var) = VT_I4;
    if (FAILED(hr = IPropertyBag_Read(bag, L"WaveInID", &var, error_log)))
        return hr;

    EnterCriticalSection(&filter->filter.filter_cs);
    filter->id = V_I4(&var);
    LeaveCriticalSection(&filter->filter.filter_cs);

    return hr;
}

static HRESULT WINAPI PPB_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(%p, %u, %u): stub\n", iface, This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

HRESULT audio_record_create(IUnknown *outer, IUnknown **out)
{
    struct audio_record *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (!(object->event = CreateEventW(NULL, FALSE, FALSE, NULL)))
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    if ((hr = fill_media_type(object, 1, &object->format)))
    {
        CloseHandle(object->event);
        free(object);
        return hr;
    }

    object->props.cBuffers = -1;
    object->props.cbBuffer = -1;
    object->props.cbAlign = -1;
    object->props.cbPrefix = -1;

    object->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    strmbase_filter_init(&object->filter, outer, &CLSID_AudioRecord, &filter_ops);

    strmbase_source_init(&object->source, &object->filter, L"Capture", &source_ops);
    object->IAMBufferNegotiation_iface.lpVtbl = &buffer_negotiation_vtbl;
    object->IAMStreamConfig_iface.lpVtbl = &stream_config_vtbl;
    object->IKsPropertySet_iface.lpVtbl = &property_set_vtbl;

    object->state = State_Stopped;
    InitializeConditionVariable(&object->state_cv);
    InitializeCriticalSectionEx(&object->state_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->state_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": audio_record.state_cs");

    TRACE("Created audio recorder %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
