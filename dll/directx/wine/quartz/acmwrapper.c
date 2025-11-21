/*
 * ACM Wrapper
 *
 * Copyright 2005 Christian Costa
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

#include "quartz_private.h"

#include "uuids.h"
#include "mmreg.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "msacm.h"

#include <assert.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct acm_wrapper
{
    struct strmbase_filter filter;

    struct strmbase_source source;
    IQualityControl source_IQualityControl_iface;
    IQualityControl *source_qc_sink;
    struct strmbase_passthrough passthrough;

    struct strmbase_sink sink;

    AM_MEDIA_TYPE mt;
    HACMSTREAM has;
    LPWAVEFORMATEX pWfOut;

    LONGLONG lasttime_real;
    LONGLONG lasttime_sent;
};

static struct acm_wrapper *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct acm_wrapper, filter);
}

static HRESULT acm_wrapper_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT acm_wrapper_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT WINAPI acm_wrapper_sink_Receive(struct strmbase_sink *iface, IMediaSample *pSample)
{
    struct acm_wrapper *This = impl_from_strmbase_filter(iface->pin.filter);
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream, cbSrcStream;
    LPBYTE pbDstStream;
    LPBYTE pbSrcStream = NULL;
    ACMSTREAMHEADER ash;
    BOOL unprepare_header = FALSE, preroll;
    MMRESULT res;
    HRESULT hr;
    LONGLONG tStart = -1, tStop = -1, tMed;
    LONGLONG mtStart = -1, mtStop = -1, mtMed;

    /* We do not expect pin connection state to change while the filter is
     * running. This guarantee is necessary, since otherwise we would have to
     * take the filter lock, and we can't take the filter lock from a streaming
     * thread. */
    if (!This->source.pMemInputPin)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    if (This->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (This->sink.flushing)
        return S_FALSE;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Failed to get input buffer pointer, hr %#lx.\n", hr);
        return hr;
    }

    preroll = (IMediaSample_IsPreroll(pSample) == S_OK);

    IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (IMediaSample_GetMediaTime(pSample, &mtStart, &mtStop) != S_OK)
        mtStart = mtStop = -1;
    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* Prevent discontinuities when codecs 'absorb' data but not give anything back in return */
    if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
    {
        This->lasttime_real = tStart;
        This->lasttime_sent = tStart;
    }
    else if (This->lasttime_real == tStart)
        tStart = This->lasttime_sent;
    else
        WARN("Discontinuity\n");

    tMed = tStart;
    mtMed = mtStart;

    ash.pbSrc = pbSrcStream;
    ash.cbSrcLength = cbSrcStream;

    while(hr == S_OK && ash.cbSrcLength)
    {
        if (FAILED(hr = IMemAllocator_GetBuffer(This->source.pAllocator, &pOutSample, NULL, NULL, 0)))
        {
            ERR("Failed to get sample, hr %#lx.\n", hr);
            return hr;
        }
        IMediaSample_SetPreroll(pOutSample, preroll);

	hr = IMediaSample_SetActualDataLength(pOutSample, 0);
	assert(hr == S_OK);

	hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
	if (FAILED(hr)) {
            ERR("Failed to get output buffer pointer, hr %#lx.\n", hr);
	    goto error;
	}
	cbDstStream = IMediaSample_GetSize(pOutSample);

	ash.cbStruct = sizeof(ash);
	ash.fdwStatus = 0;
	ash.dwUser = 0;
	ash.pbDst = pbDstStream;
	ash.cbDstLength = cbDstStream;

	if ((res = acmStreamPrepareHeader(This->has, &ash, 0))) {
	    ERR("Cannot prepare header %d\n", res);
	    goto error;
	}
	unprepare_header = TRUE;

        if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
        {
            res = acmStreamConvert(This->has, &ash, ACM_STREAMCONVERTF_START);
            IMediaSample_SetDiscontinuity(pOutSample, TRUE);
            /* One sample could be converted to multiple packets */
            IMediaSample_SetDiscontinuity(pSample, FALSE);
        }
        else
        {
            res = acmStreamConvert(This->has, &ash, 0);
            IMediaSample_SetDiscontinuity(pOutSample, FALSE);
        }

        if (res)
        {
            if(res != MMSYSERR_MOREDATA)
                ERR("Cannot convert data header %d\n", res);
            goto error;
        }

        TRACE("used in %lu/%lu, used out %lu/%lu\n", ash.cbSrcLengthUsed, ash.cbSrcLength, ash.cbDstLengthUsed, ash.cbDstLength);

        hr = IMediaSample_SetActualDataLength(pOutSample, ash.cbDstLengthUsed);
        assert(hr == S_OK);

        /* Bug in acm codecs? It apparently uses the input, but doesn't necessarily output immediately */
        if (!ash.cbSrcLengthUsed)
        {
            WARN("Sample was skipped? Outputted: %lu\n", ash.cbDstLengthUsed);
            ash.cbSrcLength = 0;
            goto error;
        }

        TRACE("Sample start time: %s.\n", debugstr_time(tStart));
        if (ash.cbSrcLengthUsed == cbSrcStream)
        {
            IMediaSample_SetTime(pOutSample, &tStart, &tStop);
            tStart = tMed = tStop;
        }
        else if (tStop != tStart)
        {
            tMed = tStop - tStart;
            tMed = tStart + tMed * ash.cbSrcLengthUsed / cbSrcStream;
            IMediaSample_SetTime(pOutSample, &tStart, &tMed);
            tStart = tMed;
        }
        else
        {
            ERR("No valid timestamp found\n");
            IMediaSample_SetTime(pOutSample, NULL, NULL);
        }

        if (mtStart < 0) {
            IMediaSample_SetMediaTime(pOutSample, NULL, NULL);
        } else if (ash.cbSrcLengthUsed == cbSrcStream) {
            IMediaSample_SetMediaTime(pOutSample, &mtStart, &mtStop);
            mtStart = mtMed = mtStop;
        } else if (mtStop >= mtStart) {
            mtMed = mtStop - mtStart;
            mtMed = mtStart + mtMed * ash.cbSrcLengthUsed / cbSrcStream;
            IMediaSample_SetMediaTime(pOutSample, &mtStart, &mtMed);
            mtStart = mtMed;
        } else {
            IMediaSample_SetMediaTime(pOutSample, NULL, NULL);
        }

        TRACE("Sample stop time: %s\n", debugstr_time(tStart));

        hr = IMemInputPin_Receive(This->source.pMemInputPin, pOutSample);
        if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
            if (FAILED(hr))
                ERR("Failed to send sample, hr %#lx.\n", hr);
            goto error;
        }

error:
        if (unprepare_header && (res = acmStreamUnprepareHeader(This->has, &ash, 0)))
            ERR("Cannot unprepare header %d\n", res);
        unprepare_header = FALSE;
        ash.pbSrc += ash.cbSrcLengthUsed;
        ash.cbSrcLength -= ash.cbSrcLengthUsed;

        IMediaSample_Release(pOutSample);
        pOutSample = NULL;

    }

    This->lasttime_real = tStop;
    This->lasttime_sent = tMed;

    return hr;
}

static BOOL is_audio_subtype(const GUID *guid)
{
    return !memcmp(&guid->Data2, &MEDIATYPE_Audio.Data2, sizeof(GUID) - sizeof(int));
}

static HRESULT acm_wrapper_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    const WAVEFORMATEX *wfx = (WAVEFORMATEX *)mt->pbFormat;
    HACMSTREAM drv;
    MMRESULT res;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio) || !is_audio_subtype(&mt->subtype)
            || !IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx) || !wfx
            || wfx->wFormatTag == WAVE_FORMAT_PCM || wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return VFW_E_TYPE_NOT_ACCEPTED;

    CopyMediaType(&filter->mt, mt);
    filter->mt.subtype.Data1 = WAVE_FORMAT_PCM;
    filter->pWfOut = (WAVEFORMATEX *)filter->mt.pbFormat;
    filter->pWfOut->wFormatTag = WAVE_FORMAT_PCM;
    filter->pWfOut->wBitsPerSample = 16;
    filter->pWfOut->nBlockAlign = filter->pWfOut->wBitsPerSample * filter->pWfOut->nChannels / 8;
    filter->pWfOut->cbSize = 0;
    filter->pWfOut->nAvgBytesPerSec = filter->pWfOut->nChannels * filter->pWfOut->nSamplesPerSec
            * (filter->pWfOut->wBitsPerSample / 8);

    if ((res = acmStreamOpen(&drv, NULL, (WAVEFORMATEX *)wfx, filter->pWfOut, NULL, 0, 0, 0)))
    {
        ERR("Failed to open stream, error %u.\n", res);
        FreeMediaType(&filter->mt);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    filter->has = drv;

    return S_OK;
}

static void acm_wrapper_sink_disconnect(struct strmbase_sink *iface)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);

    if (filter->has)
        acmStreamClose(filter->has, 0);
    filter->has = 0;
    filter->lasttime_real = filter->lasttime_sent = -1;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = acm_wrapper_sink_query_interface,
    .base.pin_query_accept = acm_wrapper_sink_query_accept,
    .pfnReceive = acm_wrapper_sink_Receive,
    .sink_connect = acm_wrapper_sink_connect,
    .sink_disconnect = acm_wrapper_sink_disconnect,
};

static HRESULT acm_wrapper_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->source_IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT acm_wrapper_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(&mt->majortype, &filter->mt.majortype)
            && (IsEqualGUID(&mt->subtype, &filter->mt.subtype)
            || IsEqualGUID(&filter->mt.subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}

static HRESULT acm_wrapper_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->filter);

    if (index)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(mt, &filter->mt);
    return S_OK;
}

static HRESULT WINAPI acm_wrapper_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < filter->pWfOut->nAvgBytesPerSec / 2)
            ppropInputRequest->cbBuffer = filter->pWfOut->nAvgBytesPerSec / 2;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = acm_wrapper_source_query_interface,
    .base.pin_query_accept = acm_wrapper_source_query_accept,
    .base.pin_get_media_type = acm_wrapper_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = acm_wrapper_source_DecideBufferSize,
};

static struct acm_wrapper *impl_from_source_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct acm_wrapper, source_IQualityControl_iface);
}

static HRESULT WINAPI acm_wrapper_source_qc_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    struct acm_wrapper *filter = impl_from_source_IQualityControl(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI acm_wrapper_source_qc_AddRef(IQualityControl *iface)
{
    struct acm_wrapper *filter = impl_from_source_IQualityControl(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI acm_wrapper_source_qc_Release(IQualityControl *iface)
{
    struct acm_wrapper *filter = impl_from_source_IQualityControl(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI acm_wrapper_source_qc_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    struct acm_wrapper *filter = impl_from_source_IQualityControl(iface);
    IQualityControl *peer;
    HRESULT hr = S_OK;

    TRACE("filter %p, sender %p, type %#x, proportion %ld, late %s, timestamp %s.\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    if (filter->source_qc_sink)
        return IQualityControl_Notify(filter->source_qc_sink, &filter->filter.IBaseFilter_iface, q);

    if (filter->sink.pin.peer
            && SUCCEEDED(IPin_QueryInterface(filter->sink.pin.peer, &IID_IQualityControl, (void **)&peer)))
    {
        hr = IQualityControl_Notify(peer, &filter->filter.IBaseFilter_iface, q);
        IQualityControl_Release(peer);
    }
    return hr;
}

static HRESULT WINAPI acm_wrapper_source_qc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct acm_wrapper *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sink %p.\n", filter, sink);

    filter->source_qc_sink = sink;

    return S_OK;
}

static const IQualityControlVtbl source_qc_vtbl =
{
    acm_wrapper_source_qc_QueryInterface,
    acm_wrapper_source_qc_AddRef,
    acm_wrapper_source_qc_Release,
    acm_wrapper_source_qc_Notify,
    acm_wrapper_source_qc_SetSink,
};

static struct strmbase_pin *acm_wrapper_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void acm_wrapper_destroy(struct strmbase_filter *iface)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);

    if (filter->source.pin.peer)
        IPin_Disconnect(filter->source.pin.peer);
    IPin_Disconnect(&filter->source.pin.IPin_iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_passthrough_cleanup(&filter->passthrough);

    FreeMediaType(&filter->mt);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT acm_wrapper_init_stream(struct strmbase_filter *iface)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (filter->source.pin.peer && FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);
    return S_OK;
}

static HRESULT acm_wrapper_cleanup_stream(struct strmbase_filter *iface)
{
    struct acm_wrapper *filter = impl_from_strmbase_filter(iface);

    if (filter->source.pin.peer)
        IMemAllocator_Decommit(filter->source.pAllocator);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = acm_wrapper_get_pin,
    .filter_destroy = acm_wrapper_destroy,
    .filter_init_stream = acm_wrapper_init_stream,
    .filter_cleanup_stream = acm_wrapper_cleanup_stream,
};

HRESULT acm_wrapper_create(IUnknown *outer, IUnknown **out)
{
    struct acm_wrapper *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_ACMWrapper, &filter_ops);

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);
    wcscpy(object->sink.pin.name, L"Input");

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    wcscpy(object->source.pin.name, L"Output");

    object->source_IQualityControl_iface.lpVtbl = &source_qc_vtbl;
    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->source.pin.IPin_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, FALSE,
            &object->sink.pin.IPin_iface);

    object->lasttime_real = object->lasttime_sent = -1;

    TRACE("Created ACM wrapper %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
