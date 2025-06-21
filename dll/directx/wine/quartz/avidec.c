/*
 * AVI Decompressor (VFW decompressors wrapper)
 *
 * Copyright 2004-2005 Christian Costa
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
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "vfw.h"
#include "dvdmedia.h"

#include <assert.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct avi_decompressor
{
    struct strmbase_filter filter;

    struct strmbase_source source;
    IQualityControl source_IQualityControl_iface;
    struct strmbase_passthrough passthrough;

    struct strmbase_sink sink;

    HIC hvid;
    BITMAPINFOHEADER* pBihIn;

    CRITICAL_SECTION late_cs;
    REFERENCE_TIME late;
};

static struct avi_decompressor *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct avi_decompressor, filter);
}

static HRESULT avi_decompressor_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT avi_decompressor_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT avi_decompressor_sink_end_flush(struct strmbase_sink *iface)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->pin.filter);
    EnterCriticalSection(&filter->late_cs);
    filter->late = -1;
    LeaveCriticalSection(&filter->late_cs);
    if (filter->source.pin.peer)
        return IPin_EndFlush(filter->source.pin.peer);
    return S_OK;
}

static int AVIDec_DropSample(struct avi_decompressor *This, REFERENCE_TIME tStart)
{
    if (This->late < 0)
        return 0;

    if (tStart < This->late) {
        TRACE("Dropping sample\n");
        return 1;
    }
    This->late = -1;
    return 0;
}

static HRESULT WINAPI avi_decompressor_sink_Receive(struct strmbase_sink *iface, IMediaSample *pSample)
{
    struct avi_decompressor *This = impl_from_strmbase_filter(iface->pin.filter);
    VIDEOINFOHEADER *source_format;
    HRESULT hr;
    IMediaSample* pOutSample = NULL;
    LONG cbDstStream, cbSrcStream;
    LPBYTE pbDstStream;
    LPBYTE pbSrcStream;
    LONGLONG tStart, tStop;
    DWORD flags = 0;
    LRESULT res;

    /* We do not expect pin connection state to change while the filter is
     * running. This guarantee is necessary, since otherwise we would have to
     * take the filter lock, and we can't take the filter lock from a streaming
     * thread. */
    if (!This->source.pMemInputPin)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    source_format = (VIDEOINFOHEADER *)This->source.pin.mt.pbFormat;

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

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = cbSrcStream;

    if (FAILED(hr = IMemAllocator_GetBuffer(This->source.pAllocator, &pOutSample, NULL, NULL, 0)))
    {
        ERR("Failed to get sample, hr %#lx.\n", hr);
        return hr;
    }

    hr = IMediaSample_SetActualDataLength(pOutSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
    if (FAILED(hr)) {
        ERR("Failed to get output buffer pointer, hr %#lx.\n", hr);
        IMediaSample_Release(pOutSample);
        return hr;
    }
    cbDstStream = IMediaSample_GetSize(pOutSample);
    if (cbDstStream < source_format->bmiHeader.biSizeImage)
    {
        ERR("Sample size is too small (%ld < %lu).\n", cbDstStream, source_format->bmiHeader.biSizeImage);
        IMediaSample_Release(pOutSample);
        return E_FAIL;
    }

    if (IMediaSample_IsPreroll(pSample) == S_OK)
        flags |= ICDECOMPRESS_PREROLL;
    if (IMediaSample_IsSyncPoint(pSample) != S_OK)
        flags |= ICDECOMPRESS_NOTKEYFRAME;
    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    EnterCriticalSection(&This->late_cs);
    if (hr == S_OK && AVIDec_DropSample(This, tStart))
        flags |= ICDECOMPRESS_HURRYUP;
    LeaveCriticalSection(&This->late_cs);

    res = ICDecompress(This->hvid, flags, This->pBihIn, pbSrcStream, &source_format->bmiHeader, pbDstStream);
    if (res != ICERR_OK)
        ERR("Failed to decompress, error %Id.\n", res);

    /* Drop sample if it's intended to be dropped */
    if (flags & ICDECOMPRESS_HURRYUP) {
        IMediaSample_Release(pOutSample);
        return S_OK;
    }

    IMediaSample_SetActualDataLength(pOutSample, source_format->bmiHeader.biSizeImage);

    IMediaSample_SetPreroll(pOutSample, (IMediaSample_IsPreroll(pSample) == S_OK));
    IMediaSample_SetDiscontinuity(pOutSample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));
    IMediaSample_SetSyncPoint(pOutSample, (IMediaSample_IsSyncPoint(pSample) == S_OK));

    if (hr == S_OK)
        IMediaSample_SetTime(pOutSample, &tStart, &tStop);
    else if (hr == VFW_S_NO_STOP_TIME)
        IMediaSample_SetTime(pOutSample, &tStart, NULL);
    else
        IMediaSample_SetTime(pOutSample, NULL, NULL);

    hr = IMemInputPin_Receive(This->source.pMemInputPin, pOutSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Failed to send sample, hr %#lx.\n", hr);

    IMediaSample_Release(pOutSample);
    return hr;
}

static HRESULT avi_decompressor_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    struct avi_decompressor *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Video)+4, sizeof(GUID)-4)))
    {
        VIDEOINFOHEADER *format1 = (VIDEOINFOHEADER *)pmt->pbFormat;
        VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)pmt->pbFormat;
        BITMAPINFOHEADER *bmi;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
            bmi = &format1->bmiHeader;
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
            bmi = &format2->bmiHeader;
        else
            goto failed;

        This->hvid = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, bmi, NULL, ICMODE_DECOMPRESS);
        if (This->hvid)
        {
            DWORD bih_size;
            LRESULT result;

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            bih_size = bmi->biSize + bmi->biClrUsed * 4;
            This->pBihIn = CoTaskMemAlloc(bih_size);
            if (!This->pBihIn)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            memcpy(This->pBihIn, bmi, bih_size);

            if ((result = ICDecompressQuery(This->hvid, This->pBihIn, NULL)))
            {
                WARN("No decompressor found, error %Id.\n", result);
                return VFW_E_TYPE_NOT_ACCEPTED;
            }

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

failed:

    TRACE("Connection refused\n");
    return hr;
}

static void avi_decompressor_sink_disconnect(struct strmbase_sink *iface)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->pin.filter);

    if (filter->hvid)
        ICClose(filter->hvid);
    CoTaskMemFree(filter->pBihIn);
    filter->hvid = NULL;
    filter->pBihIn = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = avi_decompressor_sink_query_interface,
    .base.pin_query_accept = avi_decompressor_sink_query_accept,
    .pfnReceive = avi_decompressor_sink_Receive,
    .sink_connect = avi_decompressor_sink_connect,
    .sink_disconnect = avi_decompressor_sink_disconnect,
    .sink_end_flush = avi_decompressor_sink_end_flush,
};

static HRESULT avi_decompressor_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->source_IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT avi_decompressor_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->filter);
    VIDEOINFOHEADER *sink_format, *format;

    if (!filter->sink.pin.peer || !IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
        return S_FALSE;

    sink_format = (VIDEOINFOHEADER *)filter->sink.pin.mt.pbFormat;
    format = (VIDEOINFOHEADER *)mt->pbFormat;

    if (ICDecompressQuery(filter->hvid, &sink_format->bmiHeader, &format->bmiHeader))
        return S_FALSE;

    return S_OK;
}

static HRESULT avi_decompressor_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    static const struct
    {
        const GUID *subtype;
        DWORD compression;
        WORD bpp;
    }
    formats[] =
    {
        {&MEDIASUBTYPE_CLJR, mmioFOURCC('C','L','J','R'), 8},
        {&MEDIASUBTYPE_UYVY, mmioFOURCC('U','Y','V','Y'), 16},
        {&MEDIASUBTYPE_YUY2, mmioFOURCC('Y','U','Y','2'), 16},
        {&MEDIASUBTYPE_RGB32, BI_RGB, 32},
        {&MEDIASUBTYPE_RGB24, BI_RGB, 24},
        {&MEDIASUBTYPE_RGB565, BI_BITFIELDS, 16},
        {&MEDIASUBTYPE_RGB555, BI_RGB, 16},
        {&MEDIASUBTYPE_RGB8, BI_RGB, 8},
    };

    struct avi_decompressor *filter = impl_from_strmbase_filter(iface->filter);
    const VIDEOINFOHEADER *sink_format;
    VIDEOINFO *format;

    if (!filter->sink.pin.peer)
        return VFW_S_NO_MORE_ITEMS;

    sink_format = (VIDEOINFOHEADER *)filter->sink.pin.mt.pbFormat;

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));

    if (index < ARRAY_SIZE(formats))
    {
        /* In theory we could allocate less than this, but gcc generates
         * -Warray-bounds warnings if we access the structure through a
         * VIDEOINFO pointer, even if we only access valid fields. */
        if (!(format = CoTaskMemAlloc(sizeof(*format))))
            return E_OUTOFMEMORY;
        memset(format, 0, sizeof(*format));

        format->rcSource = sink_format->rcSource;
        format->rcTarget = sink_format->rcTarget;
        format->dwBitRate = sink_format->dwBitRate;
        format->dwBitErrorRate = sink_format->dwBitErrorRate;
        format->AvgTimePerFrame = sink_format->AvgTimePerFrame;

        format->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        format->bmiHeader.biWidth = sink_format->bmiHeader.biWidth;
        format->bmiHeader.biHeight = sink_format->bmiHeader.biHeight;
        format->bmiHeader.biPlanes = sink_format->bmiHeader.biPlanes;
        format->bmiHeader.biBitCount = formats[index].bpp;
        format->bmiHeader.biCompression = formats[index].compression;
        format->bmiHeader.biSizeImage = format->bmiHeader.biHeight * (((format->bmiHeader.biWidth
                * formats[index].bpp + 31) / 8) & ~3);

        if (IsEqualGUID(formats[index].subtype, &MEDIASUBTYPE_RGB565))
        {
            format->dwBitMasks[iRED] = 0xf800;
            format->dwBitMasks[iGREEN] = 0x07e0;
            format->dwBitMasks[iBLUE] = 0x001f;
            mt->cbFormat = offsetof(VIDEOINFO, dwBitMasks[3]);
        }
        else
            mt->cbFormat = sizeof(VIDEOINFOHEADER);

        mt->majortype = MEDIATYPE_Video;
        mt->subtype = *formats[index].subtype;
        mt->bFixedSizeSamples = TRUE;
        mt->lSampleSize = format->bmiHeader.biSizeImage;
        mt->formattype = FORMAT_VideoInfo;
        mt->pbFormat = (BYTE *)format;

        return S_OK;
    }

    if (index == ARRAY_SIZE(formats))
    {
        size_t size = ICDecompressGetFormatSize(filter->hvid, &sink_format->bmiHeader);

        if (!size)
            return VFW_S_NO_MORE_ITEMS;

        mt->cbFormat = offsetof(VIDEOINFOHEADER, bmiHeader) + size;
        if (!(format = CoTaskMemAlloc(mt->cbFormat)))
            return E_OUTOFMEMORY;
        memset(format, 0, mt->cbFormat);

        format->rcSource = sink_format->rcSource;
        format->rcTarget = sink_format->rcTarget;
        format->dwBitRate = sink_format->dwBitRate;
        format->dwBitErrorRate = sink_format->dwBitErrorRate;
        format->AvgTimePerFrame = sink_format->AvgTimePerFrame;

        if (ICDecompressGetFormat(filter->hvid, &sink_format->bmiHeader, &format->bmiHeader))
        {
            CoTaskMemFree(format);
            return VFW_S_NO_MORE_ITEMS;
        }

        mt->majortype = MEDIATYPE_Video;
        mt->subtype = MEDIATYPE_Video;
        mt->subtype.Data1 = format->bmiHeader.biCompression;
        mt->bFixedSizeSamples = TRUE;
        mt->lSampleSize = format->bmiHeader.biSizeImage;
        mt->formattype = FORMAT_VideoInfo;
        mt->pbFormat = (BYTE *)format;

        return S_OK;
    }

    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI avi_decompressor_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    const VIDEOINFOHEADER *source_format = (VIDEOINFOHEADER *)iface->pin.mt.pbFormat;
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < source_format->bmiHeader.biSizeImage)
        ppropInputRequest->cbBuffer = source_format->bmiHeader.biSizeImage;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = avi_decompressor_source_query_interface,
    .base.pin_query_accept = avi_decompressor_source_query_accept,
    .base.pin_get_media_type = avi_decompressor_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = avi_decompressor_source_DecideBufferSize,
};

static struct avi_decompressor *impl_from_source_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct avi_decompressor, source_IQualityControl_iface);
}

static HRESULT WINAPI avi_decompressor_source_qc_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    struct avi_decompressor *filter = impl_from_source_IQualityControl(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI avi_decompressor_source_qc_AddRef(IQualityControl *iface)
{
    struct avi_decompressor *filter = impl_from_source_IQualityControl(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI avi_decompressor_source_qc_Release(IQualityControl *iface)
{
    struct avi_decompressor *filter = impl_from_source_IQualityControl(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI avi_decompressor_source_qc_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    struct avi_decompressor *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sender %p, type %#x, proportion %ld, late %s, timestamp %s.\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    /* can't take the stream CS here, Submarine Titans calls this function from a foreign thread while inside sink_Receive */
    EnterCriticalSection(&filter->late_cs);
    if (q.Late > 0)
        filter->late = q.Late + q.TimeStamp;
    else
        filter->late = -1;
    LeaveCriticalSection(&filter->late_cs);
    return S_OK;
}

static HRESULT WINAPI avi_decompressor_source_qc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct avi_decompressor *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sink %p.\n", filter, sink);

    return S_OK;
}

static const IQualityControlVtbl source_qc_vtbl =
{
    avi_decompressor_source_qc_QueryInterface,
    avi_decompressor_source_qc_AddRef,
    avi_decompressor_source_qc_Release,
    avi_decompressor_source_qc_Notify,
    avi_decompressor_source_qc_SetSink,
};

static struct strmbase_pin *avi_decompressor_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void avi_decompressor_destroy(struct strmbase_filter *iface)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);

    if (filter->source.pin.peer)
        IPin_Disconnect(filter->source.pin.peer);
    IPin_Disconnect(&filter->source.pin.IPin_iface);

    filter->late_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->late_cs);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_passthrough_cleanup(&filter->passthrough);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT avi_decompressor_init_stream(struct strmbase_filter *iface)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface);
    VIDEOINFOHEADER *source_format;
    LRESULT res;
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->late_cs);
    filter->late = -1;
    LeaveCriticalSection(&filter->late_cs);

    source_format = (VIDEOINFOHEADER *)filter->source.pin.mt.pbFormat;
    if ((res = ICDecompressBegin(filter->hvid, filter->pBihIn, &source_format->bmiHeader)))
    {
        ERR("ICDecompressBegin() failed, error %Id.\n", res);
        return E_FAIL;
    }

    if (FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);

    return S_OK;
}

static HRESULT avi_decompressor_cleanup_stream(struct strmbase_filter *iface)
{
    struct avi_decompressor *filter = impl_from_strmbase_filter(iface);
    LRESULT res;

    if (!filter->source.pin.peer)
        return S_OK;

    if (filter->hvid)
    {
        EnterCriticalSection(&filter->filter.stream_cs);
        res = ICDecompressEnd(filter->hvid);
        LeaveCriticalSection(&filter->filter.stream_cs);
        if (res)
        {
            ERR("ICDecompressEnd() failed, error %Id.\n", res);
            return E_FAIL;
        }
    }

    IMemAllocator_Decommit(filter->source.pAllocator);

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = avi_decompressor_get_pin,
    .filter_destroy = avi_decompressor_destroy,
    .filter_init_stream = avi_decompressor_init_stream,
    .filter_cleanup_stream = avi_decompressor_cleanup_stream,
};

HRESULT avi_dec_create(IUnknown *outer, IUnknown **out)
{
    struct avi_decompressor *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_AVIDec, &filter_ops);

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);
    wcscpy(object->sink.pin.name, L"XForm In");

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    wcscpy(object->source.pin.name, L"XForm Out");

    object->source_IQualityControl_iface.lpVtbl = &source_qc_vtbl;
    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->source.pin.IPin_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, FALSE,
            &object->sink.pin.IPin_iface);

    InitializeCriticalSectionEx(&object->late_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->late_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": object.late_cs");

    TRACE("Created AVI decompressor %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
