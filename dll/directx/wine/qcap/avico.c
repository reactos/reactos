/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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
#include "vfw.h"
#include "aviriff.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    struct strmbase_filter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;

    struct strmbase_sink sink;
    struct strmbase_source source;

    DWORD fcc_handler;
    HIC hic;

    VIDEOINFOHEADER *videoinfo;
    size_t videoinfo_size;
    DWORD driver_flags;
    DWORD max_frame_size;

    DWORD frame_cnt;
} AVICompressor;

static inline AVICompressor *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, AVICompressor, filter);
}

static inline AVICompressor *impl_from_strmbase_pin(struct strmbase_pin *pin)
{
    return impl_from_strmbase_filter(pin->filter);
}

static HRESULT ensure_driver(AVICompressor *This)
{
    if(This->hic)
        return S_OK;

    This->hic = ICOpen(FCC('v','i','d','c'), This->fcc_handler, ICMODE_COMPRESS);
    if(!This->hic) {
        FIXME("ICOpen failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT fill_format_info(AVICompressor *This, VIDEOINFOHEADER *src_videoinfo)
{
    DWORD size;
    ICINFO icinfo;
    HRESULT hres;

    hres = ensure_driver(This);
    if(hres != S_OK)
        return hres;

    size = ICGetInfo(This->hic, &icinfo, sizeof(icinfo));
    if(size != sizeof(icinfo))
        return E_FAIL;

    size = ICCompressGetFormatSize(This->hic, &src_videoinfo->bmiHeader);
    if(!size) {
        FIXME("ICCompressGetFormatSize failed\n");
        return E_FAIL;
    }

    size += FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
    if (!(This->videoinfo = calloc(1, size)))
        return E_OUTOFMEMORY;

    This->videoinfo_size = size;
    This->driver_flags = icinfo.dwFlags;
    ICCompressGetFormat(This->hic, &src_videoinfo->bmiHeader, &This->videoinfo->bmiHeader);

    This->videoinfo->dwBitRate = 10000000/src_videoinfo->AvgTimePerFrame * This->videoinfo->bmiHeader.biSizeImage * 8;
    This->videoinfo->AvgTimePerFrame = src_videoinfo->AvgTimePerFrame;
    This->max_frame_size = This->videoinfo->bmiHeader.biSizeImage;
    return S_OK;
}

static struct strmbase_pin *avi_compressor_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    AVICompressor *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void avi_compressor_destroy(struct strmbase_filter *iface)
{
    AVICompressor *filter = impl_from_strmbase_filter(iface);

    if (filter->hic)
        ICClose(filter->hic);
    free(filter->videoinfo);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT avi_compressor_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    AVICompressor *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT avi_compressor_init_stream(struct strmbase_filter *iface)
{
    AVICompressor *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (filter->source.pAllocator && FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
    {
        ERR("Failed to commit allocator, hr %#lx.\n", hr);
        return hr;
    }

    filter->frame_cnt = 0;

    return S_OK;
}

static HRESULT avi_compressor_cleanup_stream(struct strmbase_filter *iface)
{
    AVICompressor *filter = impl_from_strmbase_filter(iface);

    ICCompressEnd(filter->hic);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = avi_compressor_get_pin,
    .filter_destroy = avi_compressor_destroy,
    .filter_query_interface = avi_compressor_query_interface,
    .filter_init_stream = avi_compressor_init_stream,
    .filter_cleanup_stream = avi_compressor_cleanup_stream,
};

static AVICompressor *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AVICompressor, IPersistPropertyBag_iface);
}

static HRESULT WINAPI AVICompressorPropertyBag_QueryInterface(IPersistPropertyBag *iface, REFIID riid, void **ppv)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI AVICompressorPropertyBag_AddRef(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AVICompressorPropertyBag_Release(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AVICompressorPropertyBag_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI AVICompressorPropertyBag_InitNew(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressorPropertyBag_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    BSTR str;
    VARIANT v;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pPropBag, pErrorLog);

    V_VT(&v) = VT_BSTR;
    hres = IPropertyBag_Read(pPropBag, L"FccHandler", &v, NULL);
    if(FAILED(hres)) {
        ERR("Failed to read FccHandler value, hr %#lx.\n", hres);
        return hres;
    }

    if(V_VT(&v) != VT_BSTR) {
        FIXME("Got vt %d\n", V_VT(&v));
        VariantClear(&v);
        return E_FAIL;
    }

    str = V_BSTR(&v);
    TRACE("FccHandler = %s\n", debugstr_w(str));
    if(SysStringLen(str) != 4) {
        FIXME("Invalid FccHandler len\n");
        SysFreeString(str);
        return E_FAIL;
    }

    This->fcc_handler = FCC(str[0], str[1], str[2], str[3]);
    SysFreeString(str);
    return S_OK;
}

static HRESULT WINAPI AVICompressorPropertyBag_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p)->(%p %x %x)\n", This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl = {
    AVICompressorPropertyBag_QueryInterface,
    AVICompressorPropertyBag_AddRef,
    AVICompressorPropertyBag_Release,
    AVICompressorPropertyBag_GetClassID,
    AVICompressorPropertyBag_InitNew,
    AVICompressorPropertyBag_Load,
    AVICompressorPropertyBag_Save
};

static HRESULT sink_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *pmt)
{
    AVICompressor *This = impl_from_strmbase_pin(base);
    VIDEOINFOHEADER *videoinfo;
    HRESULT hres;
    DWORD res;

    TRACE("(%p)->(AM_MEDIA_TYPE(%p))\n", base, pmt);

    if(!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if(!IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        FIXME("formattype %s unsupported\n", debugstr_guid(&pmt->formattype));
        return S_FALSE;
    }

    hres = ensure_driver(This);
    if(hres != S_OK)
        return hres;

    videoinfo = (VIDEOINFOHEADER*)pmt->pbFormat;
    res = ICCompressQuery(This->hic, &videoinfo->bmiHeader, NULL);
    return res == ICERR_OK ? S_OK : S_FALSE;
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    AVICompressor *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI AVICompressorIn_Receive(struct strmbase_sink *base, IMediaSample *pSample)
{
    AVICompressor *This = impl_from_strmbase_pin(&base->pin);
    IMemInputPin *meminput = This->source.pMemInputPin;
    VIDEOINFOHEADER *src_videoinfo;
    REFERENCE_TIME start, stop;
    IMediaSample *out_sample;
    AM_MEDIA_TYPE *mt;
    IMediaSample2 *sample2;
    DWORD comp_flags = 0;
    BOOL is_preroll;
    BOOL sync_point;
    BYTE *ptr, *buf;
    LRESULT res;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", base, pSample);

    if (!meminput)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    if(!This->hic) {
        FIXME("Driver not loaded\n");
        return E_UNEXPECTED;
    }

    hres = IMediaSample_QueryInterface(pSample, &IID_IMediaSample2, (void**)&sample2);
    if(SUCCEEDED(hres)) {
        FIXME("Use IMediaSample2\n");
        IMediaSample2_Release(sample2);
    }

    is_preroll = IMediaSample_IsPreroll(pSample) == S_OK;
    sync_point = IMediaSample_IsSyncPoint(pSample) == S_OK;

    hres = IMediaSample_GetTime(pSample, &start, &stop);
    if(FAILED(hres)) {
        WARN("Failed to get sample time, hr %#lx.\n", hres);
        return hres;
    }

    hres = IMediaSample_GetMediaType(pSample, &mt);
    if(FAILED(hres))
        return hres;

    hres = IMediaSample_GetPointer(pSample, &ptr);
    if(FAILED(hres)) {
        ERR("Failed to get input buffer pointer, hr %#lx.\n", hres);
        return hres;
    }

    if (FAILED(hres = IMemAllocator_GetBuffer(This->source.pAllocator, &out_sample, &start, &stop, 0)))
    {
        ERR("Failed to get sample, hr %#lx.\n", hres);
        return hres;
    }

    if (FAILED(hres = IMediaSample_SetTime(out_sample, &start, &stop)))
        ERR("Failed to set time, hr %#lx.\n", hres);

    hres = IMediaSample_GetPointer(out_sample, &buf);
    if(FAILED(hres))
        return hres;

    if((This->driver_flags & VIDCF_TEMPORAL) && !(This->driver_flags & VIDCF_FASTTEMPORALC))
        FIXME("Unsupported temporal compression\n");

    src_videoinfo = (VIDEOINFOHEADER *)This->sink.pin.mt.pbFormat;
    This->videoinfo->bmiHeader.biSizeImage = This->max_frame_size;
    res = ICCompress(This->hic, sync_point ? ICCOMPRESS_KEYFRAME : 0, &This->videoinfo->bmiHeader, buf,
            &src_videoinfo->bmiHeader, ptr, 0, &comp_flags, This->frame_cnt, 0, 0, NULL, NULL);
    if(res != ICERR_OK) {
        ERR("Failed to compress frame, error %Id.\n", res);
        IMediaSample_Release(out_sample);
        return E_FAIL;
    }

    IMediaSample_SetActualDataLength(out_sample, This->videoinfo->bmiHeader.biSizeImage);
    IMediaSample_SetPreroll(out_sample, is_preroll);
    IMediaSample_SetSyncPoint(out_sample, (comp_flags&AVIIF_KEYFRAME) != 0);
    IMediaSample_SetDiscontinuity(out_sample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));

    if (IMediaSample_GetMediaTime(pSample, &start, &stop) == S_OK)
        IMediaSample_SetMediaTime(out_sample, &start, &stop);
    else
        IMediaSample_SetMediaTime(out_sample, NULL, NULL);

    hres = IMemInputPin_Receive(meminput, out_sample);
    if(FAILED(hres))
        WARN("Failed to deliver sample, hr %#lx.\n", hres);

    IMediaSample_Release(out_sample);
    This->frame_cnt++;
    return hres;
}

static HRESULT sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    AVICompressor *filter = impl_from_strmbase_pin(&iface->pin);
    return fill_format_info(filter, (VIDEOINFOHEADER *)mt->pbFormat);
}

static void sink_disconnect(struct strmbase_sink *iface)
{
    AVICompressor *filter = impl_from_strmbase_pin(&iface->pin);
    free(filter->videoinfo);
    filter->videoinfo = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_query_interface = sink_query_interface,
    .pfnReceive = AVICompressorIn_Receive,
    .sink_connect = sink_connect,
    .sink_disconnect = sink_disconnect,
};

static HRESULT source_get_media_type(struct strmbase_pin *base, unsigned int iPosition, AM_MEDIA_TYPE *amt)
{
    AVICompressor *This = impl_from_strmbase_filter(base->filter);

    if(iPosition || !This->videoinfo)
        return S_FALSE;

    amt->majortype = MEDIATYPE_Video;
    amt->subtype = MEDIASUBTYPE_PCM;
    amt->bFixedSizeSamples = FALSE;
    amt->bTemporalCompression = (This->driver_flags & VIDCF_TEMPORAL) != 0;
    amt->lSampleSize = This->sink.pin.mt.lSampleSize;
    amt->formattype = FORMAT_VideoInfo;
    amt->pUnk = NULL;
    amt->cbFormat = This->videoinfo_size;
    amt->pbFormat = (BYTE*)This->videoinfo;
    return S_OK;
}

static HRESULT WINAPI AVICompressorOut_DecideBufferSize(struct strmbase_source *base,
        IMemAllocator *alloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    AVICompressor *This = impl_from_strmbase_pin(&base->pin);
    ALLOCATOR_PROPERTIES actual;

    TRACE("(%p)\n", This);

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;
    if (ppropInputRequest->cbBuffer < This->max_frame_size)
        ppropInputRequest->cbBuffer = This->max_frame_size;
    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    return IMemAllocator_SetProperties(alloc, ppropInputRequest, &actual);
}

static HRESULT WINAPI AVICompressorOut_DecideAllocator(struct strmbase_source *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    TRACE("(%p)->(%p %p)\n", base, pPin, pAlloc);
    return BaseOutputPinImpl_DecideAllocator(base, pPin, pAlloc);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = AVICompressorOut_DecideBufferSize,
    .pfnDecideAllocator = AVICompressorOut_DecideAllocator,
};

HRESULT avi_compressor_create(IUnknown *outer, IUnknown **out)
{
    AVICompressor *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_AVICo, &filter_ops);
    object->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);
    wcscpy(object->sink.pin.name, L"Input");

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    wcscpy(object->source.pin.name, L"Output");

    TRACE("Created AVI compressor %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
